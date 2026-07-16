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

#include <common-bson-dsl-private.h>
#include <mongoc/mongoc-client-private.h>
#include <mongoc/mongoc-cluster-private.h>
#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-oidc-cache-private.h>
#include <mongoc/mongoc-server-description-private.h>
#include <mongoc/mongoc-stream-private.h>

#define SET_ERROR(...) _mongoc_set_error(error, MONGOC_ERROR_CLIENT, MONGOC_ERROR_CLIENT_AUTHENTICATE, __VA_ARGS__)

bool
mongoc_oidc_append_speculative_auth(const char *access_token, uint32_t server_id, bson_t *cmd, bson_error_t *error)
{
   BSON_ASSERT_PARAM(access_token);
   BSON_ASSERT_PARAM(cmd);
   BSON_OPTIONAL_PARAM(error);

   bool ok = false;

   // Build `saslStart` command:
   {
      bsonBuildDecl(jwt_doc, kv("jwt", cstr(access_token)));
      if (bsonBuildError) {
         SET_ERROR("BSON error: %s", bsonBuildError);
         goto fail;
      }

      bson_init(cmd);
      bsonBuild(*cmd,
                kv("saslStart", int32(1)),
                kv("mechanism", cstr("MONGODB-OIDC")),
                kv("payload", binary(BSON_SUBTYPE_BINARY, bson_get_data(&jwt_doc), jwt_doc.len)),
                kv("db", cstr("$external")));

      if (bsonBuildError) {
         SET_ERROR("BSON error: %s", bsonBuildError);
         bson_destroy(&jwt_doc);
         goto fail;
      }

      bson_destroy(&jwt_doc);
   }

   ok = true;
fail:
   return ok;
}

static bool
run_sasl_start(mongoc_cluster_t *cluster,
               mongoc_stream_t *stream,
               const mongoc_server_description_t *sd,
               const char *access_token,
               bson_error_t *error)
{
   BSON_ASSERT_PARAM(cluster);
   BSON_ASSERT_PARAM(stream);
   BSON_ASSERT_PARAM(sd);
   BSON_ASSERT_PARAM(access_token);
   BSON_OPTIONAL_PARAM(error);

   mongoc_server_stream_t *server_stream = NULL;
   bson_t cmd = BSON_INITIALIZER;
   bson_t reply = BSON_INITIALIZER;
   bool ok = false;

   // Build `saslStart` command:
   {
      bsonBuildDecl(jwt_doc, kv("jwt", cstr(access_token)));
      if (bsonBuildError) {
         SET_ERROR("BSON error: %s", bsonBuildError);
         goto fail;
      }

      bsonBuild(cmd,
                kv("saslStart", int32(1)),
                kv("mechanism", cstr("MONGODB-OIDC")),
                kv("payload", binary(BSON_SUBTYPE_BINARY, bson_get_data(&jwt_doc), jwt_doc.len)));

      if (bsonBuildError) {
         SET_ERROR("BSON error: %s", bsonBuildError);
         bson_destroy(&jwt_doc);
         goto fail;
      }

      bson_destroy(&jwt_doc);
   }

   // Send command:
   {
      mongoc_cmd_parts_t parts;

      mc_shared_tpld td = mc_tpld_take_ref(BSON_ASSERT_PTR_INLINE(cluster)->client->topology);

      mongoc_cmd_parts_init(&parts, cluster->client, "$external", MONGOC_QUERY_NONE /* unused for OP_MSG */, &cmd);
      parts.prohibit_lsid = true; // Do not append session ids to auth commands per session spec.
      server_stream = _mongoc_cluster_create_server_stream(td.ptr, sd, stream);
      mc_tpld_drop_ref(&td);
      if (!mongoc_cluster_run_command_parts(cluster, server_stream, &parts, &reply, error)) {
         goto fail;
      }
   }

   // Expect successful reply to include `done: true`:
   {
      bsonParse(reply, require(allOf(key("done"), isTrue), nop));
      if (bsonParseError) {
         SET_ERROR("Error in OIDC reply: %s", bsonParseError);
         goto fail;
      }
   }

   ok = true;

fail:
   bson_destroy(&reply);
   mongoc_server_stream_cleanup(server_stream);
   bson_destroy(&cmd);
   return ok;
}

bool
_mongoc_cluster_auth_node_oidc(mongoc_cluster_t *cluster,
                               mongoc_stream_t *stream,
                               mongoc_oidc_connection_cache_t *conn_cache,
                               const mongoc_server_description_t *sd,
                               bson_error_t *error)
{
   BSON_ASSERT_PARAM(cluster);
   BSON_ASSERT_PARAM(stream);
   BSON_ASSERT_PARAM(sd);
   BSON_ASSERT_PARAM(error);

   bool ok = false;
   char *access_token = NULL;

   bool is_cache = false;
   access_token = mongoc_oidc_cache_get_token(cluster->client->topology->oidc_cache, &is_cache, error);
   if (!access_token) {
      goto done;
   }

   if (is_cache) {
      mongoc_oidc_connection_cache_set(conn_cache, access_token);
      if (!run_sasl_start(cluster, stream, sd, access_token, error)) {
         if (error->code != MONGOC_SERVER_ERR_AUTHENTICATION) {
            goto done;
         }
         // Retry getting the access token once:
         mongoc_oidc_cache_invalidate_token(cluster->client->topology->oidc_cache, access_token);
         bson_free(access_token);
         access_token = mongoc_oidc_cache_get_token(cluster->client->topology->oidc_cache, &is_cache, error);
      } else {
         ok = true;
         goto done;
      }
   }

   if (!access_token) {
      goto done;
   }
   mongoc_oidc_connection_cache_set(conn_cache, access_token);
   if (!run_sasl_start(cluster, stream, sd, access_token, error)) {
      goto done;
   }

   ok = true;
done:
   bson_free(access_token);
   return ok;
}

bool
_mongoc_cluster_reauth_node_oidc(mongoc_cluster_t *cluster,
                                 mongoc_stream_t *stream,
                                 mongoc_oidc_connection_cache_t *oidc_connection_cache,
                                 const mongoc_server_description_t *sd,
                                 bson_error_t *error)
{
   char *connection_cached_token = mongoc_oidc_connection_cache_get(oidc_connection_cache);
   if (connection_cached_token) {
      // Invalidate shared cache:
      mongoc_oidc_cache_invalidate_token(cluster->client->topology->oidc_cache,
                                         connection_cached_token); // Does nothing if token was already invalidated.
      // Clear connection cached:
      mongoc_oidc_connection_cache_set(oidc_connection_cache, NULL);
   }
   bson_free(connection_cached_token);
   return _mongoc_cluster_auth_node_oidc(cluster, stream, oidc_connection_cache, sd, error);
}
