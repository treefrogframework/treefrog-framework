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

#include <mongoc/mongoc-structured-log-private.h>

#include <mongoc/mongoc.h>

#include <TestSuite.h>
#include <test-libmongoc.h>

typedef struct log_assumption {
   mongoc_structured_log_envelope_t expected_envelope;
   bson_t *expected_bson;
   int expected_calls;
   int calls;
} log_assumption;

static void
structured_log_func(const mongoc_structured_log_entry_t *entry, void *user_data)
{
   struct log_assumption *assumption = (struct log_assumption *)user_data;

   int calls = ++assumption->calls;
   ASSERT_CMPINT(calls, <=, assumption->expected_calls);

   ASSERT_CMPINT(entry->envelope.level, ==, assumption->expected_envelope.level);
   ASSERT_CMPINT(entry->envelope.component, ==, assumption->expected_envelope.component);
   ASSERT_CMPSTR(entry->envelope.message, assumption->expected_envelope.message);

   ASSERT_CMPSTR(entry->envelope.message, mongoc_structured_log_entry_get_message_string(entry));
   ASSERT_CMPINT(entry->envelope.level, ==, mongoc_structured_log_entry_get_level(entry));
   ASSERT_CMPINT(entry->envelope.component, ==, mongoc_structured_log_entry_get_component(entry));

   // Each call to message_as_bson allocates an identical copy
   bson_t *bson_1 = mongoc_structured_log_entry_message_as_bson(entry);
   bson_t *bson_2 = mongoc_structured_log_entry_message_as_bson(entry);

   // Compare for exact bson equality *after* comparing json strings, to give a more user friendly error on most
   // failures
   char *json_actual = bson_as_relaxed_extended_json(bson_1, NULL);
   char *json_expected = bson_as_relaxed_extended_json(assumption->expected_bson, NULL);
   ASSERT_CMPSTR(json_actual, json_expected);

   ASSERT(bson_equal(bson_1, assumption->expected_bson));
   ASSERT(bson_equal(bson_2, assumption->expected_bson));
   bson_destroy(bson_2);
   bson_destroy(bson_1);
   bson_free(json_actual);
   bson_free(json_expected);
}
void

test_structured_log_opts(void)
{
   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();

   ASSERT(mongoc_structured_log_opts_set_max_level_for_all_components(opts, MONGOC_STRUCTURED_LOG_LEVEL_WARNING));
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
                 ==,
                 mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_CONNECTION));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_SERVER_SELECTION));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_EMERGENCY,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, (mongoc_structured_log_component_t)12345));

   ASSERT(!mongoc_structured_log_opts_set_max_level_for_all_components(opts, (mongoc_structured_log_level_t)-1));
   ASSERT(mongoc_structured_log_opts_set_max_level_for_all_components(opts, MONGOC_STRUCTURED_LOG_LEVEL_INFO));
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_INFO,
                 ==,
                 mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_INFO,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_CONNECTION));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_INFO,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_SERVER_SELECTION));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_INFO,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_EMERGENCY,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, (mongoc_structured_log_component_t)12345));

   ASSERT(!mongoc_structured_log_opts_set_max_level_for_component(
      opts, (mongoc_structured_log_component_t)-1, MONGOC_STRUCTURED_LOG_LEVEL_WARNING));
   ASSERT(!mongoc_structured_log_opts_set_max_level_for_component(
      opts, MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND, (mongoc_structured_log_level_t)-1));
   ASSERT(mongoc_structured_log_opts_set_max_level_for_component(
      opts, MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND, MONGOC_STRUCTURED_LOG_LEVEL_WARNING));

   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
                 ==,
                 mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_INFO,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_CONNECTION));

   mongoc_structured_log_opts_destroy(opts);
}

void
test_structured_log_plain(void)
{
   struct log_assumption assumption = {
      .expected_envelope.level = MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      .expected_envelope.component = MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
      .expected_envelope.message = "Plain log entry",
      .expected_bson = BCON_NEW("message", BCON_UTF8("Plain log entry")),
      .expected_calls = 1,
   };

   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();
   mongoc_structured_log_opts_set_handler(opts, structured_log_func, &assumption);

   /* Note about these max_document_length settings: We want a consistent value so the test is isolated from external
    * environment variable settings. The default (MONGOC_STRUCTURED_LOG_DEFAULT_MAX_DOCUMENT_LENGTH) is verified as 1000
    * bytes elsewhere. The Command Logging and Monitoring spec recommends that tests run with a larger-than-default
    * setting of 10000 bytes. We choose that value here, but it's really quite arbitrary. */
   ASSERT(mongoc_structured_log_opts_set_max_document_length(opts, 10000));
   ASSERT(mongoc_structured_log_opts_set_max_level_for_all_components(opts, MONGOC_STRUCTURED_LOG_LEVEL_DEBUG));

   mongoc_structured_log_instance_t *instance = mongoc_structured_log_instance_new(opts);
   mongoc_structured_log_opts_destroy(opts);

   mongoc_structured_log(
      instance, MONGOC_STRUCTURED_LOG_LEVEL_WARNING, MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND, "Plain log entry");

   mongoc_structured_log_instance_destroy(instance);
   ASSERT_CMPINT(assumption.calls, ==, 1);
   bson_destroy(assumption.expected_bson);
}

void
test_structured_log_plain_with_extra_data(void)
{
   struct log_assumption assumption = {
      .expected_envelope.level = MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      .expected_envelope.component = MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
      .expected_envelope.message = "Plain log entry with extra data",
      .expected_bson = BCON_NEW("message", BCON_UTF8("Plain log entry with extra data"), "extra", BCON_INT32(1)),
      .expected_calls = 1,
   };

   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();
   mongoc_structured_log_opts_set_handler(opts, structured_log_func, &assumption);
   ASSERT(mongoc_structured_log_opts_set_max_document_length(opts, 10000));
   ASSERT(mongoc_structured_log_opts_set_max_level_for_all_components(opts, MONGOC_STRUCTURED_LOG_LEVEL_DEBUG));

   mongoc_structured_log_instance_t *instance = mongoc_structured_log_instance_new(opts);
   mongoc_structured_log_opts_destroy(opts);

   mongoc_structured_log(instance,
                         MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
                         MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
                         "Plain log entry with extra data",
                         int32("extra", 1));

   mongoc_structured_log_instance_destroy(instance);
   ASSERT_CMPINT(assumption.calls, ==, 1);
   bson_destroy(assumption.expected_bson);
}

void
test_structured_log_basic_data_types(void)
{
   const char non_terminated_test_string[] = {0, 1, 2, 3, 'a', '\\'};
   bson_t *bson_str_n = bson_new();
   bson_append_utf8(bson_str_n, "kStrN1", -1, non_terminated_test_string, sizeof non_terminated_test_string);
   bson_append_utf8(bson_str_n, "kStrN2", -1, non_terminated_test_string, sizeof non_terminated_test_string);

   struct log_assumption assumption = {
      .expected_envelope.level = MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      .expected_envelope.component = MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
      .expected_envelope.message = "Log entry with all basic data types",
      .expected_bson = BCON_NEW("message",
                                BCON_UTF8("Log entry with all basic data types"),
                                "kStr",
                                BCON_UTF8("string value"),
                                "kNullStr",
                                BCON_NULL,
                                BCON(bson_str_n),
                                "kNullStrN1",
                                BCON_NULL,
                                "kNullStrN2",
                                BCON_NULL,
                                "kNullStrN3",
                                BCON_NULL,
                                "kInt32",
                                BCON_INT32(-12345),
                                "kInt64",
                                BCON_INT64(0x76543210aabbccdd),
                                "kDouble",
                                BCON_DOUBLE(3.14159265358979323846),
                                "kTrue",
                                BCON_BOOL(true),
                                "kFalse",
                                BCON_BOOL(false)),
      .expected_calls = 1,
   };

   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();
   mongoc_structured_log_opts_set_handler(opts, structured_log_func, &assumption);
   ASSERT(mongoc_structured_log_opts_set_max_document_length(opts, 10000));
   ASSERT(mongoc_structured_log_opts_set_max_level_for_all_components(opts, MONGOC_STRUCTURED_LOG_LEVEL_DEBUG));

   mongoc_structured_log_instance_t *instance = mongoc_structured_log_instance_new(opts);
   mongoc_structured_log_opts_destroy(opts);

   mongoc_structured_log(instance,
                         MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
                         MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
                         "Log entry with all basic data types",
                         utf8("kStr", "string value"),
                         utf8("kNullStr", NULL),
                         utf8(NULL, NULL),
                         utf8_nn("kStrN1ZZZ", 6, non_terminated_test_string, sizeof non_terminated_test_string),
                         utf8_n("kStrN2", non_terminated_test_string, sizeof non_terminated_test_string),
                         utf8_nn("kNullStrN1ZZZ", 10, NULL, 12345),
                         utf8_nn("kNullStrN2", -1, NULL, 12345),
                         utf8_nn(NULL, 999, NULL, 999),
                         utf8_n("kNullStrN3", NULL, 12345),
                         int32("kInt32", -12345),
                         int32(NULL, 9999),
                         int64("kInt64", 0x76543210aabbccdd),
                         int64(NULL, -1),
                         double("kDouble", 3.14159265358979323846),
                         double(NULL, 1),
                         boolean("kTrue", true),
                         boolean("kFalse", false),
                         boolean(NULL, true));

   mongoc_structured_log_instance_destroy(instance);
   ASSERT_CMPINT(assumption.calls, ==, 1);
   bson_destroy(assumption.expected_bson);
   bson_destroy(bson_str_n);
}

void
test_structured_log_json(void)
{
   struct log_assumption assumption = {
      .expected_envelope.level = MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      .expected_envelope.component = MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
      .expected_envelope.message = "Log entry with deferred BSON-to-JSON",
      .expected_bson = BCON_NEW("message",
                                BCON_UTF8("Log entry with deferred BSON-to-JSON"),
                                "kJSON",
                                BCON_UTF8("{ \"k\" : \"v\" }"),
                                "kNull",
                                BCON_NULL),
      .expected_calls = 1,
   };

   bson_t *json_doc = BCON_NEW("k", BCON_UTF8("v"));

   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();
   mongoc_structured_log_opts_set_handler(opts, structured_log_func, &assumption);
   ASSERT(mongoc_structured_log_opts_set_max_document_length(opts, 10000));
   ASSERT(mongoc_structured_log_opts_set_max_level_for_all_components(opts, MONGOC_STRUCTURED_LOG_LEVEL_DEBUG));

   mongoc_structured_log_instance_t *instance = mongoc_structured_log_instance_new(opts);
   mongoc_structured_log_opts_destroy(opts);

   mongoc_structured_log(instance,
                         MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
                         MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
                         "Log entry with deferred BSON-to-JSON",
                         bson_as_json("kJSON", json_doc),
                         bson_as_json("kNull", NULL),
                         bson_as_json(NULL, NULL));

   mongoc_structured_log_instance_destroy(instance);
   ASSERT_CMPINT(assumption.calls, ==, 1);
   bson_destroy(assumption.expected_bson);
   bson_destroy(json_doc);
}

void
test_structured_log_oid(void)
{
   bson_oid_t oid;
   bson_oid_init_from_string(&oid, "112233445566778899aabbcc");

   struct log_assumption assumption = {
      .expected_envelope.level = MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      .expected_envelope.component = MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
      .expected_envelope.message = "Log entry with deferred OID-to-hex conversion",
      .expected_bson = BCON_NEW("message",
                                BCON_UTF8("Log entry with deferred OID-to-hex conversion"),
                                "kOID",
                                BCON_OID(&oid),
                                "kNull1",
                                BCON_NULL,
                                "kOIDHex",
                                BCON_UTF8("112233445566778899aabbcc"),
                                "kNull2",
                                BCON_NULL),
      .expected_calls = 1,
   };

   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();
   mongoc_structured_log_opts_set_handler(opts, structured_log_func, &assumption);
   ASSERT(mongoc_structured_log_opts_set_max_document_length(opts, 10000));
   ASSERT(mongoc_structured_log_opts_set_max_level_for_all_components(opts, MONGOC_STRUCTURED_LOG_LEVEL_DEBUG));

   mongoc_structured_log_instance_t *instance = mongoc_structured_log_instance_new(opts);
   mongoc_structured_log_opts_destroy(opts);

   mongoc_structured_log(instance,
                         MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
                         MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
                         "Log entry with deferred OID-to-hex conversion",
                         oid("kOID", &oid),
                         oid("kNull1", NULL),
                         oid(NULL, NULL),
                         oid_as_hex("kOIDHex", &oid),
                         oid_as_hex("kNull2", NULL),
                         oid_as_hex(NULL, NULL));

   mongoc_structured_log_instance_destroy(instance);
   ASSERT_CMPINT(assumption.calls, ==, 1);
   bson_destroy(assumption.expected_bson);
}

void
test_structured_log_error(void)
{
   struct log_assumption assumption = {
      .expected_envelope.level = MONGOC_STRUCTURED_LOG_LEVEL_INFO,
      .expected_envelope.component = MONGOC_STRUCTURED_LOG_COMPONENT_SERVER_SELECTION,
      .expected_envelope.message = "Log entry with bson_error_t values",
      .expected_bson = BCON_NEW("message",
                                BCON_UTF8("Log entry with bson_error_t values"),
                                "failure",
                                "{",
                                "code",
                                BCON_INT32(0xabab5555),
                                "domain",
                                BCON_INT32(0x87654321),
                                "message",
                                BCON_UTF8("Some Text"),
                                "}",
                                "null",
                                BCON_NULL),
      .expected_calls = 1,
   };

   const bson_error_t err = {
      .domain = 0x87654321,
      .code = 0xabab5555,
      .message = "Some Text",
   };

   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();
   mongoc_structured_log_opts_set_handler(opts, structured_log_func, &assumption);
   ASSERT(mongoc_structured_log_opts_set_max_document_length(opts, 10000));
   ASSERT(mongoc_structured_log_opts_set_max_level_for_all_components(opts, MONGOC_STRUCTURED_LOG_LEVEL_INFO));

   mongoc_structured_log_instance_t *instance = mongoc_structured_log_instance_new(opts);
   mongoc_structured_log_opts_destroy(opts);

   mongoc_structured_log(instance,
                         MONGOC_STRUCTURED_LOG_LEVEL_INFO,
                         MONGOC_STRUCTURED_LOG_COMPONENT_SERVER_SELECTION,
                         "Log entry with bson_error_t values",
                         error("failure", &err),
                         error(NULL, NULL),
                         error("null", NULL));

   mongoc_structured_log_instance_destroy(instance);
   ASSERT_CMPINT(assumption.calls, ==, 1);
   bson_destroy(assumption.expected_bson);
}

void
test_structured_log_server_description(void)
{
   struct log_assumption assumption = {
      .expected_envelope.level = MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      .expected_envelope.component = MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
      .expected_envelope.message = "Log entry with server description",
      .expected_bson = BCON_NEW("message",
                                BCON_UTF8("Log entry with server description"),
                                "serverHost",
                                BCON_UTF8("db1.example.com"),
                                "serverHost",
                                BCON_UTF8("db2.example.com"),
                                "serverPort",
                                BCON_INT32(2340),
                                "serverConnectionId",
                                BCON_INT64(0x3deeff00112233f0),
                                "serverHost",
                                BCON_UTF8("db1.example.com"),
                                "serverPort",
                                BCON_INT32(2340),
                                "serverHost",
                                BCON_UTF8("db1.example.com"),
                                "serverPort",
                                BCON_INT32(2340),
                                "serverConnectionId",
                                BCON_INT64(0x3deeff00112233f0),
                                "serviceId",
                                BCON_UTF8("2233445566778899aabbccdd"),
                                "serverHost",
                                BCON_UTF8("db2.example.com"),
                                "serverPort",
                                BCON_INT32(2341),
                                "serverConnectionId",
                                BCON_INT64(0x3deeff00112233f1)),
      .expected_calls = 1,
   };

   mongoc_server_description_t server_description_1 = {
      .host.host = "db1.example.com",
      .host.port = 2340,
      .server_connection_id = 0x3deeff00112233f0,
   };
   bson_oid_init_from_string(&server_description_1.service_id, "2233445566778899aabbccdd");

   mongoc_server_description_t server_description_2 = {
      .host.host = "db2.example.com",
      .host.port = 2341,
      .server_connection_id = 0x3deeff00112233f1,
      .service_id = {{0}},
   };

   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();
   mongoc_structured_log_opts_set_handler(opts, structured_log_func, &assumption);
   ASSERT(mongoc_structured_log_opts_set_max_document_length(opts, 10000));
   ASSERT(mongoc_structured_log_opts_set_max_level_for_all_components(opts, MONGOC_STRUCTURED_LOG_LEVEL_DEBUG));

   mongoc_structured_log_instance_t *instance = mongoc_structured_log_instance_new(opts);
   mongoc_structured_log_opts_destroy(opts);

   mongoc_structured_log(
      instance,
      MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
      "Log entry with server description",
      server_description(&server_description_1, SERVER_HOST),
      server_description(&server_description_2, SERVICE_ID),
      server_description(&server_description_2, SERVER_HOST),
      server_description(&server_description_1, SERVER_PORT),
      server_description(&server_description_1, SERVER_CONNECTION_ID),
      server_description(&server_description_1, SERVER_HOST, SERVER_PORT),
      server_description(&server_description_1, SERVER_HOST, SERVER_PORT, SERVER_CONNECTION_ID, SERVICE_ID),
      server_description(&server_description_2, SERVER_HOST, SERVER_PORT, SERVER_CONNECTION_ID, SERVICE_ID));

   mongoc_structured_log_instance_destroy(instance);
   ASSERT_CMPINT(assumption.calls, ==, 1);
   bson_destroy(assumption.expected_bson);
}

void
test_structured_log_command(void)
{
   struct log_assumption assumption = {
      .expected_envelope.level = MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      .expected_envelope.component = MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
      .expected_envelope.message = "Log entry with command and reply fields",
      .expected_bson =
         BCON_NEW("message",
                  BCON_UTF8("Log entry with command and reply fields"),
                  "commandName",
                  BCON_UTF8("Not a command"),
                  "databaseName",
                  BCON_UTF8("Some database"),
                  "commandName",
                  BCON_UTF8("Not a command"),
                  "operationId",
                  BCON_INT64(0x12345678eeff0011),
                  "command",
                  BCON_UTF8("{ \"c\" : \"d\", \"first_payload\" : [ { \"i\" : 0, \"x\" : 0 }, { \"i\" : 0, \"x\" : 1 "
                            "}, { \"i\" : 0, \"x\" : 2 }, { \"i\" : 0, \"x\" : 3 }, { \"i\" : 0, \"x\" : 4 } ], "
                            "\"second_payload\" : [ { \"i\" : 1, \"x\" : 0 }, { \"i\" : 1, \"x\" : 1 }, { \"i\" : 1, "
                            "\"x\" : 2 }, { \"i\" : 1, \"x\" : 3 }, { \"i\" : 1, \"x\" : 4 } ] }"),
                  "reply", // Un-redacted successful reply (not-a-command)
                  BCON_UTF8("{ \"r\" : \"s\", \"code\" : 1 }"),
                  "reply", // Un-redacted successful reply (ping)
                  BCON_UTF8("{ \"r\" : \"s\", \"code\" : 1 }"),
                  "reply", // Redacted successful reply (auth)
                  BCON_UTF8("{}"),
                  "failure", // Un-redacted server side error (not-a-command)
                  "{",
                  "r",
                  BCON_UTF8("s"),
                  "code",
                  BCON_INT32(1),
                  "}",
                  "failure", // Un-redacted server side error (ping)
                  "{",
                  "r",
                  BCON_UTF8("s"),
                  "code",
                  BCON_INT32(1),
                  "}",
                  "failure", // Redacted server side error (auth)
                  "{",
                  "code",
                  BCON_INT32(1),
                  "}",
                  "failure", // Client side error
                  "{",
                  "code",
                  BCON_INT32(123),
                  "domain",
                  BCON_INT32(456),
                  "message",
                  BCON_UTF8("oh no"),
                  "}"),
      .expected_calls = 1,
   };

   bson_t *cmd_doc = BCON_NEW("c", BCON_UTF8("d"));
   bson_t *reply_doc = BCON_NEW("r", BCON_UTF8("s"), "code", BCON_INT32(1));

   const bson_error_t server_error = {
      .domain = MONGOC_ERROR_SERVER,
      .code = 99,
      .message = "unused",
   };
   const bson_error_t client_error = {
      .domain = 456,
      .code = 123,
      .message = "oh no",
   };

   // Current value of MONGOC_CMD_PAYLOADS_COUNT_MAX is 2.
   // Write two payloads, each with multiple documents in sequence.
   uint8_t *payload_buf[2] = {NULL, NULL};
   size_t payload_buflen[2] = {0, 0};
   bson_writer_t *payload_writer[2] = {
      bson_writer_new(&payload_buf[0], &payload_buflen[0], 0, bson_realloc_ctx, NULL),
      bson_writer_new(&payload_buf[1], &payload_buflen[1], 0, bson_realloc_ctx, NULL),
   };
   for (unsigned x = 0; x < 5; x++) {
      for (unsigned i = 0; i < sizeof payload_writer / sizeof payload_writer[0]; i++) {
         bson_t *doc;
         bson_writer_begin(payload_writer[i], &doc);
         BCON_APPEND(doc, "i", BCON_INT32(i), "x", BCON_INT32(x));
         bson_writer_end(payload_writer[i]);
      }
   }

   mongoc_cmd_t cmd = {
      .db_name = "Some database",
      .command_name = "Not a command",
      .operation_id = 0x12345678eeff0011,
      .command = cmd_doc,
      .payloads =
         {
            {.identifier = "first_payload",
             .documents = payload_buf[0],
             .size = bson_writer_get_length(payload_writer[0])},
            {.identifier = "second_payload",
             .documents = payload_buf[1],
             .size = bson_writer_get_length(payload_writer[0])},
         },
      .payloads_count = 2,
   };

   for (unsigned i = 0; i < sizeof payload_writer / sizeof payload_writer[0]; i++) {
      bson_writer_destroy(payload_writer[i]);
   }

   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();
   mongoc_structured_log_opts_set_handler(opts, structured_log_func, &assumption);
   ASSERT(mongoc_structured_log_opts_set_max_document_length(opts, 10000));
   ASSERT(mongoc_structured_log_opts_set_max_level_for_all_components(opts, MONGOC_STRUCTURED_LOG_LEVEL_DEBUG));

   mongoc_structured_log_instance_t *instance = mongoc_structured_log_instance_new(opts);
   mongoc_structured_log_opts_destroy(opts);

   mongoc_structured_log(instance,
                         MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
                         MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
                         "Log entry with command and reply fields",
                         cmd(&cmd, COMMAND_NAME),
                         cmd(&cmd, DATABASE_NAME, COMMAND_NAME, OPERATION_ID, COMMAND),
                         cmd_reply(&cmd, reply_doc),
                         cmd_name_reply("ping", reply_doc),
                         cmd_name_reply("authenticate", reply_doc),
                         cmd_failure(&cmd, reply_doc, &server_error),
                         cmd_name_failure("ping", reply_doc, &server_error),
                         cmd_name_failure("authenticate", reply_doc, &server_error),
                         cmd_name_failure("authenticate", reply_doc, &client_error));

   mongoc_structured_log_instance_destroy(instance);
   ASSERT_CMPINT(assumption.calls, ==, 1);
   bson_destroy(assumption.expected_bson);
   bson_destroy(cmd_doc);
   bson_destroy(reply_doc);
   for (unsigned i = 0; i < sizeof payload_buf / sizeof payload_buf[0]; i++) {
      bson_free(payload_buf[i]);
   }
}

void
test_structured_log_duration(void)
{
   struct log_assumption assumption = {
      .expected_envelope.level = MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      .expected_envelope.component = MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
      .expected_envelope.message = "Log entry with duration",
      .expected_bson = BCON_NEW("message",
                                BCON_UTF8("Log entry with duration"),
                                "durationMS",
                                BCON_DOUBLE(1.999),
                                "durationMS",
                                BCON_DOUBLE(0.01),
                                "durationMS",
                                BCON_DOUBLE(10000000.999)),
      .expected_calls = 1,
   };

   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();
   mongoc_structured_log_opts_set_handler(opts, structured_log_func, &assumption);
   ASSERT(mongoc_structured_log_opts_set_max_document_length(opts, 10000));
   ASSERT(mongoc_structured_log_opts_set_max_level_for_all_components(opts, MONGOC_STRUCTURED_LOG_LEVEL_DEBUG));

   mongoc_structured_log_instance_t *instance = mongoc_structured_log_instance_new(opts);
   mongoc_structured_log_opts_destroy(opts);

   mongoc_structured_log(instance,
                         MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
                         MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND,
                         "Log entry with duration",
                         monotonic_time_duration(1999),
                         monotonic_time_duration(10),
                         monotonic_time_duration(10000000999));

   mongoc_structured_log_instance_destroy(instance);
   ASSERT_CMPINT(assumption.calls, ==, 1);
   bson_destroy(assumption.expected_bson);
}

void
test_structured_log_level_names(void)
{
   mongoc_structured_log_level_t level = (mongoc_structured_log_level_t)-1;

   // Alias, off = 0
   ASSERT(mongoc_structured_log_get_named_level("off", &level));
   ASSERT_CMPINT(0, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_EMERGENCY, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Emergency");

   ASSERT(mongoc_structured_log_get_named_level("emergency", &level));
   ASSERT_CMPINT(0, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_EMERGENCY, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Emergency");

   ASSERT(mongoc_structured_log_get_named_level("alert", &level));
   ASSERT_CMPINT(1, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_ALERT, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Alert");

   ASSERT(mongoc_structured_log_get_named_level("critical", &level));
   ASSERT_CMPINT(2, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_CRITICAL, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Critical");

   ASSERT(mongoc_structured_log_get_named_level("error", &level));
   ASSERT_CMPINT(3, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_ERROR, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Error");

   // Alias, warn = Warning
   ASSERT(mongoc_structured_log_get_named_level("warn", &level));
   ASSERT_CMPINT(4, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_WARNING, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Warning");

   ASSERT(mongoc_structured_log_get_named_level("warning", &level));
   ASSERT_CMPINT(4, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_WARNING, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Warning");

   ASSERT(mongoc_structured_log_get_named_level("notice", &level));
   ASSERT_CMPINT(5, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_NOTICE, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Notice");

   // Alias, info = Informational
   ASSERT(mongoc_structured_log_get_named_level("info", &level));
   ASSERT_CMPINT(6, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_INFO, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Informational");

   ASSERT(mongoc_structured_log_get_named_level("informational", &level));
   ASSERT_CMPINT(6, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_INFO, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Informational");

   ASSERT(mongoc_structured_log_get_named_level("debug", &level));
   ASSERT_CMPINT(7, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_DEBUG, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Debug");

   ASSERT(mongoc_structured_log_get_named_level("trace", &level));
   ASSERT_CMPINT(8, ==, level);
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_TRACE, ==, level);
   ASSERT_CMPSTR(mongoc_structured_log_get_level_name(level), "Trace");
}

void
test_structured_log_component_names(void)
{
   mongoc_structured_log_component_t component = (mongoc_structured_log_component_t)-1;

   ASSERT(mongoc_structured_log_get_named_component("Command", &component));
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND, ==, component);
   ASSERT_CMPSTR(mongoc_structured_log_get_component_name(component), "command");

   ASSERT(mongoc_structured_log_get_named_component("Topology", &component));
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY, ==, component);
   ASSERT_CMPSTR(mongoc_structured_log_get_component_name(component), "topology");

   ASSERT(mongoc_structured_log_get_named_component("ServerSelection", &component));
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_COMPONENT_SERVER_SELECTION, ==, component);
   ASSERT_CMPSTR(mongoc_structured_log_get_component_name(component), "serverSelection");

   ASSERT(mongoc_structured_log_get_named_component("Connection", &component));
   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_COMPONENT_CONNECTION, ==, component);
   ASSERT_CMPSTR(mongoc_structured_log_get_component_name(component), "connection");
}

void
test_structured_log_max_document_length(void)
{
   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();

   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_DEFAULT_MAX_DOCUMENT_LENGTH, ==, 1000);

   ASSERT(mongoc_structured_log_opts_set_max_document_length(opts, 0));
   ASSERT(!mongoc_structured_log_opts_set_max_document_length(opts, INT_MAX));
   ASSERT(mongoc_structured_log_opts_set_max_document_length(opts, INT_MAX / 2));
   ASSERT_CMPINT(INT_MAX / 2, ==, mongoc_structured_log_opts_get_max_document_length(opts));

   mongoc_structured_log_opts_destroy(opts);
}

int
test_structured_log_skip_if_env_not_default(void)
{
   // Skip testing env defaults if any options have been set externally
   const char *expected_unset[] = {
      "MONGODB_LOG_MAX_DOCUMENT_LENGTH",
      "MONGODB_LOG_COMMAND",
      "MONGODB_LOG_TOPOLOGY",
      "MONGODB_LOG_SERVER_SELECTION",
      "MONGODB_LOG_CONNECTION",
      "MONGODB_LOG_ALL",
   };

   for (size_t i = 0u; i < sizeof expected_unset / sizeof expected_unset[0]; i++) {
      const char *var = expected_unset[i];
      char *value = test_framework_getenv(var);
      bson_free(value);
      if (value) {
         MONGOC_DEBUG("Skipping test because environment var '%s' is set", var);
         return 0;
      }
   }
   return 1;
}

void
test_structured_log_env_defaults(void *test_context)
{
   BSON_UNUSED(test_context);

   mongoc_structured_log_opts_t *opts = mongoc_structured_log_opts_new();

   ASSERT_CMPINT(MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
                 ==,
                 mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_CONNECTION));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_SERVER_SELECTION));
   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_LEVEL_WARNING,
      ==,
      mongoc_structured_log_opts_get_max_level_for_component(opts, MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY));

   ASSERT_CMPINT(
      MONGOC_STRUCTURED_LOG_DEFAULT_MAX_DOCUMENT_LENGTH, ==, mongoc_structured_log_opts_get_max_document_length(opts));

   mongoc_structured_log_opts_destroy(opts);
}

void
test_structured_log_install(TestSuite *suite)
{
   TestSuite_Add(suite, "/structured_log/opts", test_structured_log_opts);
   TestSuite_Add(suite, "/structured_log/plain", test_structured_log_plain);
   TestSuite_Add(suite, "/structured_log/plain_with_extra_data", test_structured_log_plain_with_extra_data);
   TestSuite_Add(suite, "/structured_log/basic_data_types", test_structured_log_basic_data_types);
   TestSuite_Add(suite, "/structured_log/json", test_structured_log_json);
   TestSuite_Add(suite, "/structured_log/oid", test_structured_log_oid);
   TestSuite_Add(suite, "/structured_log/error", test_structured_log_error);
   TestSuite_Add(suite, "/structured_log/server_description", test_structured_log_server_description);
   TestSuite_Add(suite, "/structured_log/command", test_structured_log_command);
   TestSuite_Add(suite, "/structured_log/duration", test_structured_log_duration);
   TestSuite_Add(suite, "/structured_log/level_names", test_structured_log_level_names);
   TestSuite_Add(suite, "/structured_log/component_names", test_structured_log_component_names);
   TestSuite_Add(suite, "/structured_log/max_document_length", test_structured_log_max_document_length);
   TestSuite_AddFull(suite,
                     "/structured_log/env_defaults",
                     test_structured_log_env_defaults,
                     NULL,
                     NULL,
                     test_structured_log_skip_if_env_not_default);
}
