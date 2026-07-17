/*
 * Copyright 2009-present MongoDB, Inc.
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

#include <mongoc/mongoc-error.h>

#include <mongoc/mongoc-client-private.h>
#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-rpc-private.h>
#include <mongoc/mongoc-server-description-private.h>
#include <mongoc/mongoc-util-private.h> // _mongoc_bson_array_add_label

#include <bson/bson.h>

bool
mongoc_error_has_label(const bson_t *reply, const char *label)
{
   bson_iter_t iter;
   bson_iter_t error_labels;

   BSON_ASSERT(reply);
   BSON_ASSERT(label);

   if (bson_iter_init_find(&iter, reply, "errorLabels") && bson_iter_recurse(&iter, &error_labels)) {
      while (bson_iter_next(&error_labels)) {
         if (BSON_ITER_HOLDS_UTF8(&error_labels) && !strcmp(bson_iter_utf8(&error_labels, NULL), label)) {
            return true;
         }
      }
   }

   return false;
}

bool
_mongoc_error_is_server(const bson_error_t *error)
{
   if (!error) {
      return false;
   }

   return error->domain == MONGOC_ERROR_SERVER || error->domain == MONGOC_ERROR_WRITE_CONCERN;
}

static bool
_mongoc_write_error_is_retryable(bson_error_t *error)
{
   if (!_mongoc_error_is_server(error)) {
      return false;
   }

   switch (error->code) {
   case MONGOC_SERVER_ERR_HOSTUNREACHABLE:
   case MONGOC_SERVER_ERR_HOSTNOTFOUND:
   case MONGOC_SERVER_ERR_NETWORKTIMEOUT:
   case MONGOC_SERVER_ERR_SHUTDOWNINPROGRESS:
   case MONGOC_SERVER_ERR_PRIMARYSTEPPEDDOWN:
   case MONGOC_SERVER_ERR_EXCEEDEDTIMELIMIT:
   case MONGOC_SERVER_ERR_SOCKETEXCEPTION:
   case MONGOC_SERVER_ERR_NOTPRIMARY:
   case MONGOC_SERVER_ERR_INTERRUPTEDATSHUTDOWN:
   case MONGOC_SERVER_ERR_INTERRUPTEDDUETOREPLSTATECHANGE:
   case MONGOC_SERVER_ERR_NOTPRIMARYNOSECONDARYOK:
   case MONGOC_SERVER_ERR_NOTPRIMARYORSECONDARY:
      return true;
   default:
      return false;
   }
}

void
_mongoc_add_error_label(bson_t *reply, const char *label)
{
   BSON_OPTIONAL_PARAM(reply);
   BSON_ASSERT_PARAM(label);

   if (!reply) {
      return;
   }

   bson_t labels = BSON_INITIALIZER;
   _mongoc_bson_array_copy_labels_to(reply, &labels);
   _mongoc_bson_array_add_label(&labels, label);

   bson_t new_reply = BSON_INITIALIZER;
   bson_copy_to_excluding_noinit(reply, &new_reply, "errorLabels", NULL);
   BSON_APPEND_ARRAY(&new_reply, "errorLabels", &labels);

   bson_reinit(reply);
   bson_concat(reply, &new_reply);

   bson_destroy(&labels);
   bson_destroy(&new_reply);
}

void
_mongoc_write_error_append_retryable_label(bson_t *reply)
{
   bson_t reply_local = BSON_INITIALIZER;

   if (!reply) {
      bson_destroy(&reply_local);
      return;
   }

   bson_copy_to_excluding_noinit(reply, &reply_local, "errorLabels", NULL);
   _mongoc_error_copy_labels_and_upsert(reply, &reply_local, MONGOC_ERROR_LABEL_RETRYABLEWRITEERROR);

   bson_destroy(reply);
   bson_steal(reply, &reply_local);
}

void
_mongoc_write_error_handle_labels(bool cmd_ret,
                                  const bson_error_t *cmd_err,
                                  bson_t *reply,
                                  const mongoc_server_description_t *sd)
{
   bson_error_t error;

   /* check for a client error. */
   if (!cmd_ret && _mongoc_error_is_network(cmd_err)) {
      /* Retryable writes spec: When the driver encounters a network error
       * communicating with any server version that supports retryable
       * writes, it MUST add a RetryableWriteError label to that error. */
      _mongoc_write_error_append_retryable_label(reply);
      return;
   }

   if (sd->max_wire_version >= WIRE_VERSION_ERROR_LABEL_RETRYABLEWRITEERROR) {
      return;
   }

   /* Check for a server error. Do not consult writeConcernError for pre-4.4
    * mongos. */
   if (sd->type == MONGOC_SERVER_MONGOS) {
      if (_mongoc_cmd_check_ok(reply, MONGOC_ERROR_API_VERSION_2, &error)) {
         return;
      }
   } else {
      if (_mongoc_cmd_check_ok_no_wce(reply, MONGOC_ERROR_API_VERSION_2, &error)) {
         return;
      }
   }

   if (_mongoc_write_error_is_retryable(&error)) {
      _mongoc_write_error_append_retryable_label(reply);
   }
}


/*--------------------------------------------------------------------------
 *
 * _mongoc_read_error_get_type --
 *
 *       Checks if the error or reply from a read command is considered
 *       retryable according to the retryable reads spec. Checks both
 *       for a client error (a network exception) and a server error in
 *       the reply. @cmd_ret and @cmd_err come from the result of a
 *       read_command function.
 *
 *
 * Return:
 *       A mongoc_read_error_type_t indicating the type of error (if any).
 *
 *--------------------------------------------------------------------------
 */
mongoc_read_err_type_t
_mongoc_read_error_get_type(bool cmd_ret, const bson_error_t *cmd_err, const bson_t *reply)
{
   bson_error_t error;

   /* check for a client error. */
   if (!cmd_ret && cmd_err && _mongoc_error_is_network(cmd_err)) {
      /* Retryable reads spec: "considered retryable if [...] any network
       * exception (e.g. socket timeout or error) */
      return MONGOC_READ_ERR_RETRY;
   }

   /* check for a server error. */
   if (_mongoc_cmd_check_ok_no_wce(reply, MONGOC_ERROR_API_VERSION_2, &error)) {
      return MONGOC_READ_ERR_NONE;
   }

   switch (error.code) {
   case MONGOC_SERVER_ERR_EXCEEDEDTIMELIMIT:
   case MONGOC_SERVER_ERR_INTERRUPTEDATSHUTDOWN:
   case MONGOC_SERVER_ERR_INTERRUPTEDDUETOREPLSTATECHANGE:
   case MONGOC_SERVER_ERR_NOTPRIMARY:
   case MONGOC_SERVER_ERR_NOTPRIMARYNOSECONDARYOK:
   case MONGOC_SERVER_ERR_NOTPRIMARYORSECONDARY:
   case MONGOC_SERVER_ERR_PRIMARYSTEPPEDDOWN:
   case MONGOC_SERVER_ERR_READCONCERNMAJORITYNOTAVAILABLEYET:
   case MONGOC_SERVER_ERR_SHUTDOWNINPROGRESS:
   case MONGOC_SERVER_ERR_HOSTNOTFOUND:
   case MONGOC_SERVER_ERR_HOSTUNREACHABLE:
   case MONGOC_SERVER_ERR_NETWORKTIMEOUT:
   case MONGOC_SERVER_ERR_SOCKETEXCEPTION:
      return MONGOC_READ_ERR_RETRY;
   default:
      if (strstr(error.message, "not master") || strstr(error.message, "node is recovering")) {
         return MONGOC_READ_ERR_RETRY;
      }
      return MONGOC_READ_ERR_OTHER;
   }
}

void
_mongoc_error_copy_labels_and_upsert(const bson_t *src, bson_t *dst, const char *label)
{
   bson_iter_t iter;
   bson_iter_t src_label;
   bson_array_builder_t *dst_labels;

   BSON_APPEND_ARRAY_BUILDER_BEGIN(dst, "errorLabels", &dst_labels);
   bson_array_builder_append_utf8(dst_labels, label, -1);

   /* append any other errorLabels already in "src" */
   if (bson_iter_init_find(&iter, src, "errorLabels") && bson_iter_recurse(&iter, &src_label)) {
      while (bson_iter_next(&src_label) && BSON_ITER_HOLDS_UTF8(&src_label)) {
         if (strcmp(bson_iter_utf8(&src_label, NULL), label) != 0) {
            bson_array_builder_append_utf8(dst_labels, bson_iter_utf8(&src_label, NULL), -1);
         }
      }
   }

   bson_append_array_builder_end(dst, dst_labels);
}

/* Defined in SDAM spec under "Application Errors".
 * @error should have been obtained from a command reply, e.g. with
 * _mongoc_cmd_check_ok.
 */
bool
_mongoc_error_is_shutdown(bson_error_t *error)
{
   if (!_mongoc_error_is_server(error)) {
      return false;
   }
   switch (error->code) {
   case 11600: /* InterruptedAtShutdown */
   case 91:    /* ShutdownInProgress */
      return true;
   default:
      return false;
   }
}

bool
_mongoc_error_is_not_primary(bson_error_t *error)
{
   if (!_mongoc_error_is_server(error)) {
      return false;
   }

   if (_mongoc_error_is_recovering(error)) {
      return false;
   }
   switch (error->code) {
   case MONGOC_SERVER_ERR_NOTPRIMARY:
   case MONGOC_SERVER_ERR_NOTPRIMARYNOSECONDARYOK:
   case MONGOC_SERVER_ERR_LEGACYNOTPRIMARY:
      return true;
      /* All errors where no code was found are marked as
       * MONGOC_ERROR_QUERY_FAILURE */
   case MONGOC_ERROR_QUERY_FAILURE:
      return NULL != strstr(error->message, "not master");
   default:
      return false;
   }
}

bool
_mongoc_error_is_recovering(bson_error_t *error)
{
   if (!_mongoc_error_is_server(error)) {
      return false;
   }
   switch (error->code) {
   case MONGOC_SERVER_ERR_INTERRUPTEDATSHUTDOWN:
   case MONGOC_SERVER_ERR_INTERRUPTEDDUETOREPLSTATECHANGE:
   case MONGOC_SERVER_ERR_NOTPRIMARYORSECONDARY:
   case MONGOC_SERVER_ERR_PRIMARYSTEPPEDDOWN:
   case MONGOC_SERVER_ERR_SHUTDOWNINPROGRESS:
      return true;
   /* All errors where no code was found are marked as
    * MONGOC_ERROR_QUERY_FAILURE */
   case MONGOC_ERROR_QUERY_FAILURE:
      return NULL != strstr(error->message, "not master or secondary") ||
             NULL != strstr(error->message, "node is recovering");
   default:
      return false;
   }
}

/* Assumes @error was parsed as an API V2 error. */
bool
_mongoc_error_is_state_change(bson_error_t *error)
{
   return _mongoc_error_is_recovering(error) || _mongoc_error_is_not_primary(error);
}

bool
_mongoc_error_is_network(const bson_error_t *error)
{
   if (!error) {
      return false;
   }
   if (error->domain == MONGOC_ERROR_STREAM) {
      return true;
   }

   if (error->domain == MONGOC_ERROR_PROTOCOL && error->code == MONGOC_ERROR_PROTOCOL_INVALID_REPLY) {
      return true;
   }

   return false;
}

bool
_mongoc_error_is_dns(const bson_error_t *error)
{
   return error && error->domain == MONGOC_ERROR_STREAM && error->code == MONGOC_ERROR_STREAM_NAME_RESOLUTION;
}

bool
_mongoc_error_is_auth(const bson_error_t *error)
{
   if (!error) {
      return false;
   }

   return error->domain == MONGOC_ERROR_CLIENT && error->code == MONGOC_ERROR_CLIENT_AUTHENTICATE;
}

bool
_mongoc_error_is_reauth(const bson_error_t *error, int error_api_version)
{
   if (!error) {
      return false;
   }

   uint32_t expected_domain =
      error_api_version == MONGOC_ERROR_API_VERSION_2 ? MONGOC_ERROR_SERVER : MONGOC_ERROR_QUERY;
   return error->domain == expected_domain && error->code == MONGOC_SERVER_ERR_REAUTHENTICATION_REQUIRED;
}

void
_mongoc_error_append(bson_error_t *error, const char *s)
{
   BSON_ASSERT(error);
   const size_t error_len = strlen(error->message);
   const size_t remaining = sizeof(error->message) - error_len;
   bson_strncpy(error->message + error_len, s, remaining);
}

bool
mongoc_error_append_contents_to_bson(const bson_error_t *error, bson_t *bson, mongoc_error_content_flags_t flags)
{
   BSON_ASSERT_PARAM(error);
   BSON_ASSERT_PARAM(bson);

   if ((flags & MONGOC_ERROR_CONTENT_FLAG_CODE) && !BSON_APPEND_INT32(bson, "code", error->code)) {
      return false;
   }
   if ((flags & MONGOC_ERROR_CONTENT_FLAG_DOMAIN) && !BSON_APPEND_INT32(bson, "domain", error->domain)) {
      return false;
   }
   if ((flags & MONGOC_ERROR_CONTENT_FLAG_MESSAGE) && !BSON_APPEND_UTF8(bson, "message", error->message)) {
      return false;
   }
   return true;
}

void
_mongoc_set_error(bson_error_t *error, uint32_t domain, uint32_t code, const char *format, ...)
{
   if (error) {
      error->domain = domain;
      error->code = code;
      _mongoc_set_error_category(error, MONGOC_ERROR_CATEGORY);

      va_list args;
      va_start(args, format);
      // Format into a temporary buf before copying into the error, as the existing
      // error message may be an input to our formatting string
      char buffer[sizeof(error->message)] = {0};
      bson_vsnprintf(buffer, sizeof error->message, format, args);
      memcpy(&error->message, buffer, sizeof buffer);
      va_end(args);
   }
}

void
_mongoc_set_error_with_category(
   bson_error_t *error, uint8_t category, uint32_t domain, uint32_t code, const char *format, ...)
{
   if (error) {
      error->domain = domain;
      error->code = code;
      _mongoc_set_error_category(error, category);

      va_list args;
      va_start(args, format);
      char buffer[sizeof(error->message)] = {0};
      bson_vsnprintf(buffer, sizeof error->message, format, args);
      memcpy(&error->message, buffer, sizeof buffer);
      va_end(args);
   }
}

#ifdef _WIN32

char *
mongoc_winerr_to_string(DWORD err_code)
{
   LPSTR msg = NULL;
   if (0 == FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL,
                           err_code,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPSTR)&msg,
                           0,
                           NULL)) {
      LocalFree(msg);
      return bson_strdup_printf("(0x%.8lX) (Failed to get error message)", err_code);
   }

   // Remove trailing newline.
   size_t msglen = strlen(msg);
   if (msglen >= 1 && msg[msglen - 1] == '\n') {
      if (msglen >= 2 && msg[msglen - 2] == '\r') {
         // Remove trailing \r\n.
         msg[msglen - 2] = '\0';
      } else {
         // Just remove trailing \n.
         msg[msglen - 1] = '\0';
      }
   }

   char *ret = bson_strdup_printf("(0x%.8lX) %s", err_code, msg);
   LocalFree(msg);
   return ret;
}

#endif // _WIN32
