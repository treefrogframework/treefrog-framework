/*
 * Copyright 2020 MongoDB, Inc.
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

#include <mongoc/mongoc.h>

#include "mongoc/mongoc-client-private.h"

#include "TestSuite.h"
#include "test-conveniences.h"
#include "mock_server/mock-server.h"
#include "mock_server/future-functions.h"
#include "test-libmongoc.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "cmd-test-options"


/* CDRIVER-3303 - mongoc_cmd_parts_assemble sometimes fails to set options;
 * the fix was to refactor the code and this test guards against regressions
 */
static void
test_client_cmd_options (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_read_concern_t *rc;
   bson_t opts;
   future_t *future;
   request_t *request;
   bson_error_t error;

   server = mock_server_with_auto_hello (WIRE_VERSION_OP_MSG);
   mock_server_run (server);
   client =
      test_framework_client_new_from_uri (mock_server_get_uri (server), NULL);

   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, MONGOC_READ_CONCERN_LEVEL_MAJORITY);
   bson_init (&opts);
   mongoc_read_concern_append (rc, &opts);

   future =
      future_client_command_with_opts (client,
                                       "db",
                                       tmp_bson ("{'ping': 1, '$db': 'db'}"),
                                       NULL,
                                       &opts,
                                       NULL,
                                       &error);

   request = mock_server_receives_msg (
      server,
      MONGOC_QUERY_NONE,
      tmp_bson ("{'readConcern': { '$exists': true }}"));

   mock_server_replies_simple (request, "{'ok': 1, 'n': 1}");
   ASSERT_OR_PRINT (future_get_bool (future), error);

   request_destroy (request);
   future_destroy (future);

   bson_destroy (&opts);
   mongoc_read_concern_destroy (rc);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


void
test_client_cmd_install (TestSuite *suite)
{
   TestSuite_AddMockServerTest (
      suite, "/Client/cmd/options", test_client_cmd_options);
}
