/*
 * Copyright 2017 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "mongoc/mongoc-client-session-private.h"
#include "mongoc/mongoc-trace-private.h"
#include "mongoc/mongoc-client-private.h"
#include "mongoc/mongoc-rand-private.h"
#include "mongoc/mongoc-util-private.h"
#include "mongoc/mongoc-read-concern-private.h"
#include "mongoc/mongoc-read-prefs-private.h"

#define SESSION_NEVER_USED (-1)


static void
txn_opts_set (mongoc_transaction_opt_t *opts,
              const mongoc_read_concern_t *read_concern,
              const mongoc_write_concern_t *write_concern,
              const mongoc_read_prefs_t *read_prefs)
{
   if (read_concern) {
      mongoc_transaction_opts_set_read_concern (opts, read_concern);
   }

   if (write_concern) {
      mongoc_transaction_opts_set_write_concern (opts, write_concern);
   }

   if (read_prefs) {
      mongoc_transaction_opts_set_read_prefs (opts, read_prefs);
   }
}


static void
txn_opts_cleanup (mongoc_transaction_opt_t *opts)
{
   /* null inputs are ok */
   mongoc_read_concern_destroy (opts->read_concern);
   mongoc_write_concern_destroy (opts->write_concern);
   mongoc_read_prefs_destroy (opts->read_prefs);
   /* prepare opts for reuse */
   opts->read_concern = NULL;
   opts->write_concern = NULL;
   opts->read_prefs = NULL;
}


static void
txn_opts_copy (const mongoc_transaction_opt_t *src,
               mongoc_transaction_opt_t *dst)
{
   txn_opts_cleanup (dst);
   /* null inputs are ok for these copy functions */
   dst->read_concern = mongoc_read_concern_copy (src->read_concern);
   dst->write_concern = mongoc_write_concern_copy (src->write_concern);
   dst->read_prefs = mongoc_read_prefs_copy (src->read_prefs);
}


static void
copy_labels_plus_unknown_commit_result (const bson_t *src, bson_t *dst)
{
   bson_iter_t iter;
   bson_iter_t src_label;
   bson_t dst_labels;
   char str[16];
   uint32_t i = 0;
   const char *key;

   BSON_APPEND_ARRAY_BEGIN (dst, "errorLabels", &dst_labels);
   BSON_APPEND_UTF8 (&dst_labels, "0", UNKNOWN_COMMIT_RESULT);

   /* append any other errorLabels already in "src" */
   if (bson_iter_init_find (&iter, src, "errorLabels") &&
       bson_iter_recurse (&iter, &src_label)) {
      while (bson_iter_next (&src_label) && BSON_ITER_HOLDS_UTF8 (&src_label)) {
         if (strcmp (bson_iter_utf8 (&src_label, NULL),
                     UNKNOWN_COMMIT_RESULT) != 0) {
            i++;
            bson_uint32_to_string (i, &key, str, sizeof str);
            BSON_APPEND_UTF8 (
               &dst_labels, key, bson_iter_utf8 (&src_label, NULL));
         }
      }
   }

   bson_append_array_end (dst, &dst_labels);
}


static bool
txn_abort (mongoc_client_session_t *session, bson_t *reply, bson_error_t *error)
{
   bson_t cmd = BSON_INITIALIZER;
   bson_t opts = BSON_INITIALIZER;
   bson_error_t err_local;
   bson_error_t *err_ptr = error ? error : &err_local;
   bson_t reply_local = BSON_INITIALIZER;
   mongoc_write_err_type_t error_type;
   bool r = false;

   _mongoc_bson_init_if_set (reply);

   if (!mongoc_client_session_append (session, &opts, err_ptr)) {
      GOTO (done);
   }

   if (session->txn.opts.write_concern) {
      if (!mongoc_write_concern_append (session->txn.opts.write_concern,
                                        &opts)) {
         bson_set_error (err_ptr,
                         MONGOC_ERROR_TRANSACTION,
                         MONGOC_ERROR_TRANSACTION_INVALID_STATE,
                         "Invalid transaction write concern");
         GOTO (done);
      }
   }

   BSON_APPEND_INT32 (&cmd, "abortTransaction", 1);

   /* will be reinitialized by mongoc_client_write_command_with_opts */
   bson_destroy (&reply_local);
   r = mongoc_client_write_command_with_opts (
      session->client, "admin", &cmd, &opts, &reply_local, err_ptr);

   /* Transactions Spec: "Drivers MUST retry the commitTransaction command once
    * after it fails with a retryable error", same for abort */
   error_type = _mongoc_write_error_get_type (r, err_ptr, &reply_local);
   if (error_type == MONGOC_WRITE_ERR_RETRY) {
      bson_destroy (&reply_local);
      r = mongoc_client_write_command_with_opts (
         session->client, "admin", &cmd, &opts, &reply_local, err_ptr);
   }

   if (!r) {
      /* we won't return an error from abortTransaction, so warn */
      MONGOC_WARNING ("Error in abortTransaction: %s", err_ptr->message);
   }

done:
   bson_destroy (&reply_local);
   bson_destroy (&cmd);
   bson_destroy (&opts);

   return r;
}


static mongoc_write_concern_t *
create_commit_retry_wc (const mongoc_write_concern_t *existing_wc)
{
   mongoc_write_concern_t *wc;
   int32_t wtimeout;

   wc = existing_wc ? mongoc_write_concern_copy (existing_wc)
                    : mongoc_write_concern_new ();

   wtimeout = mongoc_write_concern_get_wtimeout (wc);

   /* Transactions spec: "If the modified write concern does not include a
    * wtimeout value, drivers MUST also apply wtimeout: 10000 to the write
    * concern in order to avoid waiting forever if the majority write concern
    * cannot be satisfied." */
   if (wtimeout <= 0) {
      wtimeout = MONGOC_DEFAULT_WTIMEOUT_FOR_COMMIT_RETRY;
   }

   /* Transactions spec: "If the transaction is using a write concern that is
    * not the server default, any other write concern options MUST be left as-is
    * when applying w:majority. */
   mongoc_write_concern_set_wmajority (wc, wtimeout);

   return wc;
}


static bool
txn_commit (mongoc_client_session_t *session,
            bool explicitly_retrying,
            bson_t *reply,
            bson_error_t *error)
{
   bson_t cmd = BSON_INITIALIZER;
   bson_t opts = BSON_INITIALIZER;
   bson_error_t err_local;
   bson_error_t *err_ptr = error ? error : &err_local;
   bson_t reply_local = BSON_INITIALIZER;
   mongoc_write_err_type_t error_type;
   bool r = false;
   bool retrying_after_error = false;
   mongoc_write_concern_t *retry_wc = NULL;

   _mongoc_bson_init_if_set (reply);

   BSON_APPEND_INT32 (&cmd, "commitTransaction", 1);

retry:
   if (!mongoc_client_session_append (session, &opts, err_ptr)) {
      GOTO (done);
   }

   /* Transactions Spec: "When commitTransaction is retried, either by the
    * driver's internal retry-once logic or explicitly by the user calling
    * commitTransaction again, drivers MUST apply w:majority to the write
    * concern of the commitTransaction command." */
   if (!retry_wc && (retrying_after_error || explicitly_retrying)) {
      retry_wc = create_commit_retry_wc (session->txn.opts.write_concern
                                            ? session->txn.opts.write_concern
                                            : session->client->write_concern);
   }

   if (retry_wc || session->txn.opts.write_concern) {
      if (!mongoc_write_concern_append (
             retry_wc ? retry_wc : session->txn.opts.write_concern, &opts)) {
         bson_set_error (err_ptr,
                         MONGOC_ERROR_TRANSACTION,
                         MONGOC_ERROR_TRANSACTION_INVALID_STATE,
                         "Invalid transaction write concern");
         GOTO (done);
      }
   }

   /* will be reinitialized by mongoc_client_write_command_with_opts */
   bson_destroy (&reply_local);
   r = mongoc_client_write_command_with_opts (
      session->client, "admin", &cmd, &opts, &reply_local, err_ptr);

   /* Transactions Spec: "Drivers MUST retry the commitTransaction command once
    * after it fails with a retryable error", same for abort */
   error_type = _mongoc_write_error_get_type (r, err_ptr, &reply_local);
   if (!retrying_after_error && error_type == MONGOC_WRITE_ERR_RETRY) {
      retrying_after_error = true; /* retry after error only once */
      bson_reinit (&opts);
      GOTO (retry);
   }

   /* Transactions Spec: "add the UnknownTransactionCommitResult error label
    * when commitTransaction fails with a network error, server selection
    * error, or write concern failed / timeout." */
   if (reply) {
      if ((!r && err_ptr->domain == MONGOC_ERROR_SERVER_SELECTION) ||
          error_type == MONGOC_WRITE_ERR_RETRY ||
          error_type == MONGOC_WRITE_ERR_WRITE_CONCERN) {
         bson_copy_to_excluding_noinit (
            &reply_local, reply, "errorLabels", NULL);
         copy_labels_plus_unknown_commit_result (&reply_local, reply);
      } else {
         /* maintain invariants: reply & reply_local are valid until the end */
         bson_destroy (reply);
         bson_steal (reply, &reply_local);
         bson_init (&reply_local);
      }
   }

done:
   bson_destroy (&reply_local);
   bson_destroy (&cmd);
   bson_destroy (&opts);

   if (retry_wc) {
      mongoc_write_concern_destroy (retry_wc);
   }

   return r;
}


mongoc_transaction_opt_t *
mongoc_transaction_opts_new (void)
{
   return (mongoc_transaction_opt_t *) bson_malloc0 (
      sizeof (mongoc_transaction_opt_t));
}


mongoc_transaction_opt_t *
mongoc_transaction_opts_clone (const mongoc_transaction_opt_t *opts)
{
   mongoc_transaction_opt_t *cloned_opts;

   ENTRY;

   BSON_ASSERT (opts);

   cloned_opts = mongoc_transaction_opts_new ();
   txn_opts_copy (opts, cloned_opts);

   RETURN (cloned_opts);
}


void
mongoc_transaction_opts_destroy (mongoc_transaction_opt_t *opts)
{
   ENTRY;

   if (!opts) {
      EXIT;
   }

   txn_opts_cleanup (opts);
   bson_free (opts);

   EXIT;
}


void
mongoc_transaction_opts_set_read_concern (
   mongoc_transaction_opt_t *opts, const mongoc_read_concern_t *read_concern)
{
   BSON_ASSERT (opts);
   mongoc_read_concern_destroy (opts->read_concern);
   opts->read_concern = mongoc_read_concern_copy (read_concern);
}


const mongoc_read_concern_t *
mongoc_transaction_opts_get_read_concern (const mongoc_transaction_opt_t *opts)
{
   BSON_ASSERT (opts);
   return opts->read_concern;
}


void
mongoc_transaction_opts_set_write_concern (
   mongoc_transaction_opt_t *opts, const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT (opts);
   mongoc_write_concern_destroy (opts->write_concern);
   opts->write_concern = mongoc_write_concern_copy (write_concern);
}


const mongoc_write_concern_t *
mongoc_transaction_opts_get_write_concern (const mongoc_transaction_opt_t *opts)
{
   BSON_ASSERT (opts);
   return opts->write_concern;
}


void
mongoc_transaction_opts_set_read_prefs (mongoc_transaction_opt_t *opts,
                                        const mongoc_read_prefs_t *read_prefs)
{
   BSON_ASSERT (opts);
   mongoc_read_prefs_destroy (opts->read_prefs);
   opts->read_prefs = mongoc_read_prefs_copy (read_prefs);
}


const mongoc_read_prefs_t *
mongoc_transaction_opts_get_read_prefs (const mongoc_transaction_opt_t *opts)
{
   BSON_ASSERT (opts);
   return opts->read_prefs;
}


mongoc_session_opt_t *
mongoc_session_opts_new (void)
{
   mongoc_session_opt_t *opts = bson_malloc0 (sizeof (mongoc_session_opt_t));

   /* Driver Sessions Spec: causal consistency is true by default */
   mongoc_session_opts_set_causal_consistency (opts, true);

   return opts;
}


void
mongoc_session_opts_set_causal_consistency (mongoc_session_opt_t *opts,
                                            bool causal_consistency)
{
   ENTRY;

   BSON_ASSERT (opts);

   if (causal_consistency) {
      opts->flags |= MONGOC_SESSION_CAUSAL_CONSISTENCY;
   } else {
      opts->flags &= ~MONGOC_SESSION_CAUSAL_CONSISTENCY;
   }

   EXIT;
}

bool
mongoc_session_opts_get_causal_consistency (const mongoc_session_opt_t *opts)
{
   ENTRY;

   BSON_ASSERT (opts);

   RETURN (!!(opts->flags & MONGOC_SESSION_CAUSAL_CONSISTENCY));
}


void
mongoc_session_opts_set_default_transaction_opts (
   mongoc_session_opt_t *opts, const mongoc_transaction_opt_t *txn_opts)
{
   ENTRY;

   BSON_ASSERT (opts);
   BSON_ASSERT (txn_opts);

   txn_opts_set (&opts->default_txn_opts,
                 txn_opts->read_concern,
                 txn_opts->write_concern,
                 txn_opts->read_prefs);

   EXIT;
}


const mongoc_transaction_opt_t *
mongoc_session_opts_get_default_transaction_opts (
   const mongoc_session_opt_t *opts)
{
   ENTRY;

   BSON_ASSERT (opts);

   RETURN (&opts->default_txn_opts);
}


static void
_mongoc_session_opts_copy (const mongoc_session_opt_t *src,
                           mongoc_session_opt_t *dst)
{
   dst->flags = src->flags;
   txn_opts_copy (&src->default_txn_opts, &dst->default_txn_opts);
}


mongoc_session_opt_t *
mongoc_session_opts_clone (const mongoc_session_opt_t *opts)
{
   mongoc_session_opt_t *cloned_opts;

   ENTRY;

   BSON_ASSERT (opts);

   cloned_opts = bson_malloc0 (sizeof (mongoc_session_opt_t));
   _mongoc_session_opts_copy (opts, cloned_opts);

   RETURN (cloned_opts);
}


void
mongoc_session_opts_destroy (mongoc_session_opt_t *opts)
{
   ENTRY;

   if (!opts) {
      EXIT;
   }

   txn_opts_cleanup (&opts->default_txn_opts);
   bson_free (opts);

   EXIT;
}


static bool
_mongoc_server_session_uuid (uint8_t *data /* OUT */, bson_error_t *error)
{
#ifdef MONGOC_ENABLE_CRYPTO
   /* https://tools.ietf.org/html/rfc4122#page-14
    *   o  Set the two most significant bits (bits 6 and 7) of the
    *      clock_seq_hi_and_reserved to zero and one, respectively.
    *
    *   o  Set the four most significant bits (bits 12 through 15) of the
    *      time_hi_and_version field to the 4-bit version number from
    *      Section 4.1.3.
    *
    *   o  Set all the other bits to randomly (or pseudo-randomly) chosen
    *      values.
    */

   if (!_mongoc_rand_bytes (data, 16)) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_SESSION_FAILURE,
                      "Could not generate UUID for logical session id");

      return false;
   }

   data[6] = (uint8_t) (0x40 | (data[6] & 0xf));
   data[8] = (uint8_t) (0x80 | (data[8] & 0x3f));

   return true;
#else
   /* no _mongoc_rand_bytes without a crypto library */
   bson_set_error (error,
                   MONGOC_ERROR_CLIENT,
                   MONGOC_ERROR_CLIENT_SESSION_FAILURE,
                   "Could not generate UUID for logical session id, we need a"
                   " cryptography library like libcrypto, Common Crypto, or"
                   " CNG");

   return false;
#endif
}


bool
_mongoc_parse_cluster_time (const bson_t *cluster_time,
                            uint32_t *timestamp,
                            uint32_t *increment)
{
   bson_iter_t iter;
   char *s;

   if (!cluster_time ||
       !bson_iter_init_find (&iter, cluster_time, "clusterTime") ||
       !BSON_ITER_HOLDS_TIMESTAMP (&iter)) {
      s = bson_as_json (cluster_time, NULL);
      MONGOC_ERROR ("Cannot parse cluster time from %s\n", s);
      bson_free (s);
      return false;
   }

   bson_iter_timestamp (&iter, timestamp, increment);

   return true;
}


bool
_mongoc_cluster_time_greater (const bson_t *new, const bson_t *old)
{
   uint32_t new_t, new_i, old_t, old_i;

   if (!_mongoc_parse_cluster_time (new, &new_t, &new_i) ||
       !_mongoc_parse_cluster_time (old, &old_t, &old_i)) {
      return false;
   }

   return (new_t > old_t) || (new_t == old_t && new_i > old_i);
}


void
_mongoc_client_session_handle_reply (mongoc_client_session_t *session,
                                     bool is_acknowledged,
                                     const bson_t *reply)
{
   bson_iter_t iter;
   uint32_t len;
   const uint8_t *data;
   bson_t cluster_time;
   uint32_t t;
   uint32_t i;

   BSON_ASSERT (session);

   if (!reply || !bson_iter_init (&iter, reply)) {
      return;
   }

   while (bson_iter_next (&iter)) {
      if (!strcmp (bson_iter_key (&iter), "$clusterTime") &&
          BSON_ITER_HOLDS_DOCUMENT (&iter)) {
         bson_iter_document (&iter, &len, &data);
         BSON_ASSERT (bson_init_static (&cluster_time, data, (size_t) len));

         mongoc_client_session_advance_cluster_time (session, &cluster_time);
      } else if (!strcmp (bson_iter_key (&iter), "operationTime") &&
                 BSON_ITER_HOLDS_TIMESTAMP (&iter) && is_acknowledged) {
         bson_iter_timestamp (&iter, &t, &i);
         mongoc_client_session_advance_operation_time (session, t, i);
      }
   }
}


mongoc_server_session_t *
_mongoc_server_session_new (bson_error_t *error)
{
   uint8_t uuid_data[16];
   mongoc_server_session_t *s;

   ENTRY;

   if (!_mongoc_server_session_uuid (uuid_data, error)) {
      RETURN (NULL);
   }

   s = bson_malloc0 (sizeof (mongoc_server_session_t));
   s->last_used_usec = SESSION_NEVER_USED;
   s->prev = NULL;
   s->next = NULL;
   bson_init (&s->lsid);
   bson_append_binary (
      &s->lsid, "id", 2, BSON_SUBTYPE_UUID, uuid_data, sizeof uuid_data);

   /* transaction number is a positive integer and will be incremented before
    * each use, so ensure it is initialized to zero. */
   s->txn_number = 0;

   RETURN (s);
}


bool
_mongoc_server_session_timed_out (const mongoc_server_session_t *server_session,
                                  int64_t session_timeout_minutes)
{
   int64_t timeout_usec;
   const int64_t minute_to_usec = 60 * 1000 * 1000;

   ENTRY;

   if (session_timeout_minutes == MONGOC_NO_SESSIONS) {
      /* not connected right now; keep the session */
      return false;
   }

   if (server_session->last_used_usec == SESSION_NEVER_USED) {
      return false;
   }

   /* Driver Sessions Spec: if a session has less than one minute left before
    * becoming stale, discard it */
   timeout_usec =
      server_session->last_used_usec + session_timeout_minutes * minute_to_usec;

   RETURN (timeout_usec - bson_get_monotonic_time () < 1 * minute_to_usec);
}


void
_mongoc_server_session_destroy (mongoc_server_session_t *server_session)
{
   ENTRY;

   bson_destroy (&server_session->lsid);
   bson_free (server_session);

   EXIT;
}


mongoc_client_session_t *
_mongoc_client_session_new (mongoc_client_t *client,
                            mongoc_server_session_t *server_session,
                            const mongoc_session_opt_t *opts,
                            uint32_t client_session_id)
{
   mongoc_client_session_t *session;

   ENTRY;

   BSON_ASSERT (client);

   session = bson_malloc0 (sizeof (mongoc_client_session_t));
   session->client = client;
   session->client_generation = client->generation;
   session->server_session = server_session;
   session->client_session_id = client_session_id;
   bson_init (&session->cluster_time);

   txn_opts_set (&session->opts.default_txn_opts,
                 client->read_concern,
                 client->write_concern,
                 client->read_prefs);

   if (opts) {
      session->opts.flags = opts->flags;
      txn_opts_set (&session->opts.default_txn_opts,
                    opts->default_txn_opts.read_concern,
                    opts->default_txn_opts.write_concern,
                    opts->default_txn_opts.read_prefs);
   } else {
      /* sessions are causally consistent by default */
      session->opts.flags = MONGOC_SESSION_CAUSAL_CONSISTENCY;
   }

   RETURN (session);
}


mongoc_client_t *
mongoc_client_session_get_client (const mongoc_client_session_t *session)
{
   BSON_ASSERT (session);

   return session->client;
}


const mongoc_session_opt_t *
mongoc_client_session_get_opts (const mongoc_client_session_t *session)
{
   BSON_ASSERT (session);

   return &session->opts;
}


const bson_t *
mongoc_client_session_get_lsid (const mongoc_client_session_t *session)
{
   BSON_ASSERT (session);

   return &session->server_session->lsid;
}

const bson_t *
mongoc_client_session_get_cluster_time (const mongoc_client_session_t *session)
{
   BSON_ASSERT (session);

   if (bson_empty (&session->cluster_time)) {
      return NULL;
   }

   return &session->cluster_time;
}

void
mongoc_client_session_advance_cluster_time (mongoc_client_session_t *session,
                                            const bson_t *cluster_time)
{
   uint32_t t, i;

   ENTRY;

   if (bson_empty (&session->cluster_time) &&
       _mongoc_parse_cluster_time (cluster_time, &t, &i)) {
      bson_destroy (&session->cluster_time);
      bson_copy_to (cluster_time, &session->cluster_time);
      EXIT;
   }

   if (_mongoc_cluster_time_greater (cluster_time, &session->cluster_time)) {
      bson_destroy (&session->cluster_time);
      bson_copy_to (cluster_time, &session->cluster_time);
   }

   EXIT;
}

void
mongoc_client_session_get_operation_time (
   const mongoc_client_session_t *session,
   uint32_t *timestamp,
   uint32_t *increment)
{
   BSON_ASSERT (session);
   BSON_ASSERT (timestamp);
   BSON_ASSERT (increment);

   *timestamp = session->operation_timestamp;
   *increment = session->operation_increment;
}

void
mongoc_client_session_advance_operation_time (mongoc_client_session_t *session,
                                              uint32_t timestamp,
                                              uint32_t increment)
{
   ENTRY;

   BSON_ASSERT (session);

   if (timestamp > session->operation_timestamp ||
       (timestamp == session->operation_timestamp &&
        increment > session->operation_increment)) {
      session->operation_timestamp = timestamp;
      session->operation_increment = increment;
   }

   EXIT;
}


bool
mongoc_client_session_start_transaction (mongoc_client_session_t *session,
                                         const mongoc_transaction_opt_t *opts,
                                         bson_error_t *error)
{
   ENTRY;

   BSON_ASSERT (session);

   /* use "switch" so that static checkers ensure we handle all states */
   switch (session->txn.state) {
   case MONGOC_TRANSACTION_STARTING:
   case MONGOC_TRANSACTION_IN_PROGRESS:
      bson_set_error (error,
                      MONGOC_ERROR_TRANSACTION,
                      MONGOC_ERROR_TRANSACTION_INVALID_STATE,
                      "Transaction already in progress");
      RETURN (false);
   case MONGOC_TRANSACTION_ENDING:
      MONGOC_ERROR ("starting txn in invalid state MONGOC_TRANSACTION_ENDING");
      abort ();
   case MONGOC_TRANSACTION_COMMITTED:
   case MONGOC_TRANSACTION_COMMITTED_EMPTY:
   case MONGOC_TRANSACTION_ABORTED:
   case MONGOC_TRANSACTION_NONE:
   default:
      break;
   }

   session->server_session->txn_number++;

   txn_opts_set (&session->txn.opts,
                 session->opts.default_txn_opts.read_concern,
                 session->opts.default_txn_opts.write_concern,
                 session->opts.default_txn_opts.read_prefs);

   if (opts) {
      txn_opts_set (&session->txn.opts,
                    opts->read_concern,
                    opts->write_concern,
                    opts->read_prefs);
   }

   if (!mongoc_write_concern_is_acknowledged (
          session->txn.opts.write_concern)) {
      bson_set_error (
         error,
         MONGOC_ERROR_TRANSACTION,
         MONGOC_ERROR_TRANSACTION_INVALID_STATE,
         "Transactions do not support unacknowledged write concern");
      RETURN (false);
   }

   session->txn.state = MONGOC_TRANSACTION_STARTING;

   RETURN (true);
}


bool
mongoc_client_session_in_transaction (const mongoc_client_session_t *session)
{
   ENTRY;

   BSON_ASSERT (session);

   /* call the internal function, which would allow a NULL session */
   RETURN (_mongoc_client_session_in_txn (session));
}


bool
mongoc_client_session_commit_transaction (mongoc_client_session_t *session,
                                          bson_t *reply,
                                          bson_error_t *error)
{
   bool r = false;

   ENTRY;

   BSON_ASSERT (session);

   /* See Transactions Spec for state diagram. In COMMITTED state, user can call
    * commit again to retry after network error */

   switch (session->txn.state) {
   case MONGOC_TRANSACTION_NONE:
      bson_set_error (error,
                      MONGOC_ERROR_TRANSACTION,
                      MONGOC_ERROR_TRANSACTION_INVALID_STATE,
                      "No transaction started");
      _mongoc_bson_init_if_set (reply);
      break;
   case MONGOC_TRANSACTION_STARTING:
   case MONGOC_TRANSACTION_COMMITTED_EMPTY:
      /* we sent no commands, not actually started on server */
      session->txn.state = MONGOC_TRANSACTION_COMMITTED_EMPTY;
      _mongoc_bson_init_if_set (reply);
      r = true;
      break;
   case MONGOC_TRANSACTION_COMMITTED:
   case MONGOC_TRANSACTION_IN_PROGRESS: {
      bool explicitly_retrying =
         (session->txn.state == MONGOC_TRANSACTION_COMMITTED);
      /* in MONGOC_TRANSACTION_ENDING we add txnNumber and autocommit: false
       * to the commitTransaction command, but if it fails with network error
       * we add UnknownTransactionCommitResult not TransientTransactionError */
      session->txn.state = MONGOC_TRANSACTION_ENDING;
      r = txn_commit (session, explicitly_retrying, reply, error);
      session->txn.state = MONGOC_TRANSACTION_COMMITTED;
      break;
   }
   case MONGOC_TRANSACTION_ENDING:
      MONGOC_ERROR ("commit called in invalid state MONGOC_TRANSACTION_ENDING");
      abort ();
   case MONGOC_TRANSACTION_ABORTED:
   default:
      bson_set_error (
         error,
         MONGOC_ERROR_TRANSACTION,
         MONGOC_ERROR_TRANSACTION_INVALID_STATE,
         "Cannot call commitTransaction after calling abortTransaction");
      _mongoc_bson_init_if_set (reply);
      break;
   }

   RETURN (r);
}


bool
mongoc_client_session_abort_transaction (mongoc_client_session_t *session,
                                         bson_error_t *error)
{
   ENTRY;

   BSON_ASSERT (session);

   switch (session->txn.state) {
   case MONGOC_TRANSACTION_STARTING:
      /* we sent no commands, not actually started on server */
      session->txn.state = MONGOC_TRANSACTION_ABORTED;
      RETURN (true);
   case MONGOC_TRANSACTION_IN_PROGRESS:
      session->txn.state = MONGOC_TRANSACTION_ENDING;
      /* Transactions Spec: ignore errors from abortTransaction command */
      txn_abort (session, NULL, NULL);
      session->txn.state = MONGOC_TRANSACTION_ABORTED;
      RETURN (true);
   case MONGOC_TRANSACTION_COMMITTED:
   case MONGOC_TRANSACTION_COMMITTED_EMPTY:
      bson_set_error (
         error,
         MONGOC_ERROR_TRANSACTION,
         MONGOC_ERROR_TRANSACTION_INVALID_STATE,
         "Cannot call abortTransaction after calling commitTransaction");
      RETURN (false);
   case MONGOC_TRANSACTION_ABORTED:
      bson_set_error (error,
                      MONGOC_ERROR_TRANSACTION,
                      MONGOC_ERROR_TRANSACTION_INVALID_STATE,
                      "Cannot call abortTransaction twice");
      RETURN (false);
   case MONGOC_TRANSACTION_ENDING:
      MONGOC_ERROR ("abort called in invalid state MONGOC_TRANSACTION_ENDING");
      abort ();
   case MONGOC_TRANSACTION_NONE:
   default:
      bson_set_error (error,
                      MONGOC_ERROR_TRANSACTION,
                      MONGOC_ERROR_TRANSACTION_INVALID_STATE,
                      "No transaction started");
      RETURN (false);
   }
}


bool
_mongoc_client_session_from_iter (mongoc_client_t *client,
                                  const bson_iter_t *iter,
                                  mongoc_client_session_t **cs,
                                  bson_error_t *error)
{
   ENTRY;

   /* must be int64 that fits in uint32 */
   if (!BSON_ITER_HOLDS_INT64 (iter) || bson_iter_int64 (iter) > 0xffffffff) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "Invalid sessionId");
      RETURN (false);
   }

   RETURN (_mongoc_client_lookup_session (
      client, (uint32_t) bson_iter_int64 (iter), cs, error));
}


bool
_mongoc_client_session_in_txn (const mongoc_client_session_t *session)
{
   if (!session) {
      return false;
   }

   /* use "switch" so that static checkers ensure we handle all states */
   switch (session->txn.state) {
   case MONGOC_TRANSACTION_STARTING:
   case MONGOC_TRANSACTION_IN_PROGRESS:
      return true;
   case MONGOC_TRANSACTION_NONE:
   case MONGOC_TRANSACTION_ENDING:
   case MONGOC_TRANSACTION_COMMITTED:
   case MONGOC_TRANSACTION_COMMITTED_EMPTY:
   case MONGOC_TRANSACTION_ABORTED:
   default:
      return false;
   }
}


bool
_mongoc_client_session_txn_in_progress (const mongoc_client_session_t *session)
{
   if (!session) {
      return false;
   }

   return session->txn.state == MONGOC_TRANSACTION_IN_PROGRESS;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_client_session_append_txn --
 *
 *       Add transaction fields besides "readConcern" to @cmd.
 *
 * Returns:
 *       Returns false and sets @error if @cmd is empty, otherwise returns
 *       true.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_client_session_append_txn (mongoc_client_session_t *session,
                                   bson_t *cmd,
                                   bson_error_t *error)
{
   mongoc_transaction_t *txn;

   ENTRY;

   if (!session) {
      RETURN (true);
   }

   if (bson_empty0 (cmd)) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "Empty command in transaction");
      RETURN (false);
   }

   txn = &session->txn;

   /* See Transactions Spec for state transitions. In COMMITTED / ABORTED, the
    * next operation resets the session and moves to TRANSACTION_NONE */
   switch (session->txn.state) {
   case MONGOC_TRANSACTION_STARTING:
      txn->state = MONGOC_TRANSACTION_IN_PROGRESS;
      bson_append_bool (cmd, "startTransaction", 16, true);
   /* FALL THROUGH */
   case MONGOC_TRANSACTION_IN_PROGRESS:
   case MONGOC_TRANSACTION_ENDING:
      bson_append_int64 (
         cmd, "txnNumber", 9, session->server_session->txn_number);
      bson_append_bool (cmd, "autocommit", 10, false);
      RETURN (true);
   case MONGOC_TRANSACTION_COMMITTED:
      if (!strcmp (_mongoc_get_command_name (cmd), "commitTransaction")) {
         /* send commitTransaction again */
         bson_append_int64 (
            cmd, "txnNumber", 9, session->server_session->txn_number);
         bson_append_bool (cmd, "autocommit", 10, false);
         RETURN (true);
      }
   /* FALL THROUGH */
   case MONGOC_TRANSACTION_COMMITTED_EMPTY:
   case MONGOC_TRANSACTION_ABORTED:
      txn_opts_cleanup (&session->txn.opts);
      txn->state = MONGOC_TRANSACTION_NONE;
      RETURN (true);
   case MONGOC_TRANSACTION_NONE:
   default:
      RETURN (true);
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_client_session_append_read_concern --
 *
 *       Add read concern if we're doing a read outside a transaction, or if
 *       we're starting a transaction, or if the user explicitly passed a read
 *       concern in some function's "opts". The contents of the read concern
 *       are "level" and/or "afterClusterTime" - if both are empty, don't add
 *       read concern.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_client_session_append_read_concern (const mongoc_client_session_t *cs,
                                            const bson_t *rc,
                                            bool is_read_command,
                                            bson_t *cmd)
{
   const mongoc_read_concern_t *txn_rc;
   mongoc_transaction_state_t txn_state;
   bool user_rc_has_level;
   bool txn_has_level;
   bool has_timestamp;
   bool has_level;
   bson_t child;

   ENTRY;

   BSON_ASSERT (cs);

   txn_state = cs->txn.state;
   txn_rc = cs->txn.opts.read_concern;

   if (txn_state == MONGOC_TRANSACTION_IN_PROGRESS) {
      return;
   }

   has_timestamp =
      (txn_state == MONGOC_TRANSACTION_STARTING || is_read_command) &&
      mongoc_session_opts_get_causal_consistency (&cs->opts) &&
      cs->operation_timestamp;
   user_rc_has_level = rc && bson_has_field (rc, "level");
   txn_has_level = txn_state == MONGOC_TRANSACTION_STARTING &&
                   !mongoc_read_concern_is_default (txn_rc);
   has_level = user_rc_has_level || txn_has_level;

   if (!has_timestamp && !has_level) {
      return;
   }

   bson_append_document_begin (cmd, "readConcern", 11, &child);
   if (rc) {
      bson_concat (&child, rc);
   }

   if (txn_state == MONGOC_TRANSACTION_STARTING) {
      /* add transaction's read concern level unless user overrides */
      if (txn_has_level && !user_rc_has_level) {
         bson_append_utf8 (&child, "level", 5, txn_rc->level, -1);
      }
   }

   if (has_timestamp) {
      bson_append_timestamp (&child,
                             "afterClusterTime",
                             16,
                             cs->operation_timestamp,
                             cs->operation_increment);
   }

   bson_append_document_end (cmd, &child);
}


bool
mongoc_client_session_append (const mongoc_client_session_t *client_session,
                              bson_t *opts,
                              bson_error_t *error)
{
   ENTRY;

   BSON_ASSERT (client_session);
   BSON_ASSERT (opts);

   if (!bson_append_int64 (
          opts, "sessionId", 9, client_session->client_session_id)) {
      bson_set_error (
         error, MONGOC_ERROR_BSON, MONGOC_ERROR_BSON_INVALID, "invalid opts");

      RETURN (false);
   }

   RETURN (true);
}


void
mongoc_client_session_destroy (mongoc_client_session_t *session)
{
   ENTRY;

   if (!session) {
      EXIT;
   }

   if (session->client_generation == session->client->generation) {
      if (mongoc_client_session_in_transaction (session)) {
         mongoc_client_session_abort_transaction (session, NULL);
      }

      _mongoc_client_unregister_session (session->client, session);
      _mongoc_client_push_server_session (session->client,
                                          session->server_session);
   } else {
      /* If the client has been reset, destroy the server session instead of
    pushing it back into the topology's pool. */
      _mongoc_server_session_destroy (session->server_session);
   }

   txn_opts_cleanup (&session->opts.default_txn_opts);
   txn_opts_cleanup (&session->txn.opts);

   bson_destroy (&session->cluster_time);
   bson_free (session);

   EXIT;
}
