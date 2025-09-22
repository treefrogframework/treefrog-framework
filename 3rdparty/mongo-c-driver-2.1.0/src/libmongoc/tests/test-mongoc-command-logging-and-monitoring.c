#include <mongoc/mongoc-array-private.h>

#include <mongoc/mongoc.h>

#include <mlib/loop.h>

#include <TestSuite.h>
#include <test-conveniences.h>
#include <test-libmongoc.h>

static void
stored_log_handler (const mongoc_structured_log_entry_t *entry, void *user_data)
{
   mongoc_array_t *log_array = (mongoc_array_t *) user_data;
   bson_t *doc = mongoc_structured_log_entry_message_as_bson (entry);
   MONGOC_DEBUG ("stored log: %s", tmp_json (doc));
   _mongoc_array_append_val (log_array, doc);
}

static void
stored_log_clear (mongoc_array_t *log_array)
{
   for (size_t i = 0; i < log_array->len; i++) {
      bson_t *doc = _mongoc_array_index (log_array, bson_t *, i);
      bson_destroy (doc);
   }
   _mongoc_array_clear (log_array);
}

/* specifications/source/command-logging-and-monitoring/tests/README.md
 * Test 1: Default truncation limit */
static void
prose_test_1 (void)
{
   // 1. Configure logging with a minimum severity level of "debug" for the "command" component. Do not explicitly
   // configure the max document length.
   mongoc_client_t *client = test_framework_new_default_client ();
   mongoc_array_t stored_log;
   _mongoc_array_init (&stored_log, sizeof (bson_t *));
   {
      mongoc_structured_log_opts_t *log_opts = mongoc_structured_log_opts_new ();

      ASSERT (mongoc_structured_log_opts_set_max_level_for_component (
         log_opts, MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND, MONGOC_STRUCTURED_LOG_LEVEL_DEBUG));

      mongoc_structured_log_opts_set_handler (log_opts, stored_log_handler, &stored_log);

      ASSERT (mongoc_client_set_structured_log_opts (client, log_opts));
      mongoc_structured_log_opts_destroy (log_opts);
   }
   mongoc_collection_t *coll = mongoc_client_get_collection (client, "db", "coll");

   // 2. Construct an array docs containing the document {"x" : "y"} repeated 100 times.
   bson_t *docs[100];
   for (unsigned i = 0; i < sizeof docs / sizeof docs[0]; i++) {
      docs[i] = tmp_bson (BSON_STR ({"x" : "y"}));
   }

   // 3. Insert docs to a collection via insertMany.
   bson_error_t error;
   ASSERT_OR_PRINT (
      mongoc_collection_insert_many (coll, (const bson_t **) docs, sizeof docs / sizeof docs[0], NULL, NULL, &error),
      error);

   // 4. Inspect the resulting "command started" log message and assert that the "command" value is a string of length
   // 1000 + (length of trailing ellipsis).
   {
      ASSERT (stored_log.len >= 1);
      bson_t *log = _mongoc_array_index (&stored_log, bson_t *, 0);
      bson_iter_t iter;
      ASSERT (bson_iter_init (&iter, log));
      ASSERT (bson_iter_find (&iter, "message"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "Command started");
      ASSERT (bson_iter_find (&iter, "command"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      uint32_t command_len;
      const char *command = bson_iter_utf8 (&iter, &command_len);
      ASSERT (command);
      ASSERT_CMPUINT32 (command_len, ==, 1003);
   }

   // 5. Inspect the resulting "command succeeded" log message and assert that the "reply" value is a string of length
   // <= 1000 + (length of trailing ellipsis).
   {
      ASSERT (stored_log.len == 2);
      bson_t *log = _mongoc_array_index (&stored_log, bson_t *, 1);
      bson_iter_t iter;
      ASSERT (bson_iter_init (&iter, log));
      ASSERT (bson_iter_find (&iter, "message"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "Command succeeded");
      ASSERT (bson_iter_find (&iter, "reply"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      uint32_t reply_len;
      const char *reply = bson_iter_utf8 (&iter, &reply_len);
      ASSERT (reply);
      ASSERT_CMPUINT32 (reply_len, <=, 1003);
   }

   // 6. Run find() on the collection where the document was inserted.
   stored_log_clear (&stored_log);
   mongoc_cursor_t *cursor = mongoc_collection_find_with_opts (coll, tmp_bson ("{}"), NULL, NULL);
   ASSERT (cursor);
   {
      const bson_t *doc;
      ASSERT (mongoc_cursor_next (cursor, &doc));
   }

   // 7. Inspect the resulting "command succeeded" log message and assert that the reply is a string of length 1000 +
   // (length of trailing ellipsis).
   {
      ASSERT (stored_log.len >= 1);
      bson_t *log = _mongoc_array_index (&stored_log, bson_t *, 0);
      bson_iter_t iter;
      ASSERT (bson_iter_init (&iter, log));
      ASSERT (bson_iter_find (&iter, "message"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "Command started");
   }
   {
      ASSERT (stored_log.len == 2);
      bson_t *log = _mongoc_array_index (&stored_log, bson_t *, 1);
      bson_iter_t iter;
      ASSERT (bson_iter_init (&iter, log));
      ASSERT (bson_iter_find (&iter, "message"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "Command succeeded");
      ASSERT (bson_iter_find (&iter, "reply"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      uint32_t reply_len;
      const char *reply = bson_iter_utf8 (&iter, &reply_len);
      ASSERT (reply);
      ASSERT_CMPUINT32 (reply_len, ==, 1003);
   }

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
   stored_log_clear (&stored_log);
   _mongoc_array_destroy (&stored_log);
}

/* Test 2: Explicitly configured truncation limit */
static void
prose_test_2 (void)
{
   // 1. Configure logging with a minimum severity level of "debug" for the "command" component. Set the max document
   // length to 5.
   mongoc_client_t *client = test_framework_new_default_client ();
   mongoc_array_t stored_log;
   _mongoc_array_init (&stored_log, sizeof (bson_t *));
   {
      mongoc_structured_log_opts_t *log_opts = mongoc_structured_log_opts_new ();

      ASSERT (mongoc_structured_log_opts_set_max_document_length (log_opts, 5));
      ASSERT (mongoc_structured_log_opts_set_max_level_for_component (
         log_opts, MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND, MONGOC_STRUCTURED_LOG_LEVEL_DEBUG));

      mongoc_structured_log_opts_set_handler (log_opts, stored_log_handler, &stored_log);

      ASSERT (mongoc_client_set_structured_log_opts (client, log_opts));
      mongoc_structured_log_opts_destroy (log_opts);
   }

   // 2. Run the command {"hello": true}.
   {
      bson_error_t error;
      ASSERT_OR_PRINT (
         mongoc_client_command_simple (client, "db", tmp_bson (BSON_STR ({"hello" : true})), NULL, NULL, &error),
         error);
   }

   // 3. Inspect the resulting "command started" log message and assert that the "command" value is a string of length 5
   // + (length of trailing ellipsis).
   {
      ASSERT (stored_log.len >= 1);
      bson_t *log = _mongoc_array_index (&stored_log, bson_t *, 0);
      bson_iter_t iter;
      ASSERT (bson_iter_init (&iter, log));
      ASSERT (bson_iter_find (&iter, "message"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "Command started");
      ASSERT (bson_iter_find (&iter, "command"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      uint32_t command_len;
      const char *command = bson_iter_utf8 (&iter, &command_len);
      ASSERT (command);
      ASSERT_CMPUINT32 (command_len, ==, 5 + 3);
      ASSERT_CMPSTR (command, "{ \"he...");
   }

   // 4. Inspect the resulting "command succeeded" log message and assert that the "reply" value is a string of length 5
   // + (length of trailing ellipsis).
   {
      ASSERT (stored_log.len == 2);
      bson_t *log = _mongoc_array_index (&stored_log, bson_t *, 1);
      bson_iter_t iter;
      ASSERT (bson_iter_init (&iter, log));
      ASSERT (bson_iter_find (&iter, "message"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "Command succeeded");
      ASSERT (bson_iter_find (&iter, "reply"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      uint32_t reply_len;
      const char *reply = bson_iter_utf8 (&iter, &reply_len);
      ASSERT (reply);
      ASSERT_CMPUINT32 (reply_len, ==, 5 + 3);
   }

   // 5. If the driver attaches raw server responses to failures and can access these via log messages to assert on, run
   // the command {"notARealCommand": true}. Inspect the resulting "command failed" log message and confirm that the
   // server error is a string of length 5 + (length of trailing ellipsis).
   //
   // This is not applicable to libmongoc. The spec allows flexible data type for "failure", and here we chose a
   // document rather than a string. The document is not subject to truncation.
   //
   // While we're here, test that the proposed fake command itself is truncated as expected, and the "failure" is a
   // document.
   stored_log_clear (&stored_log);
   ASSERT (
      !mongoc_client_command_simple (client, "db", tmp_bson (BSON_STR ({"notARealCommand" : true})), NULL, NULL, NULL));
   {
      ASSERT (stored_log.len >= 1);
      bson_t *log = _mongoc_array_index (&stored_log, bson_t *, 0);
      bson_iter_t iter;
      ASSERT (bson_iter_init (&iter, log));
      ASSERT (bson_iter_find (&iter, "message"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "Command started");
      ASSERT (bson_iter_find (&iter, "command"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      uint32_t command_len;
      const char *command = bson_iter_utf8 (&iter, &command_len);
      ASSERT (command);
      ASSERT_CMPUINT32 (command_len, ==, 5 + 3);
      ASSERT_CMPSTR (command, "{ \"no...");
   }
   {
      ASSERT (stored_log.len == 2);
      bson_t *log = _mongoc_array_index (&stored_log, bson_t *, 1);
      bson_iter_t iter;
      ASSERT (bson_iter_init (&iter, log));
      ASSERT (bson_iter_find (&iter, "message"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "Command failed");
      ASSERT (bson_iter_find (&iter, "failure"));
      ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));
   }

   mongoc_client_destroy (client);
   stored_log_clear (&stored_log);
   _mongoc_array_destroy (&stored_log);
}

/* Test 3: Truncation with multi-byte code points */
static void
prose_test_3 (void)
{
   // "Drivers MUST write language-specific tests that confirm truncation of commands, replies, and (if applicable)
   // server responses included in error messages work as expected when the data being truncated includes multi-byte
   // Unicode codepoints." "If the driver uses anything other than Unicode codepoints as the unit for max document
   // length, there also MUST be tests confirming that cases where the max length falls in the middle of a multi-byte
   // codepoint are handled gracefully."
   //
   // For libmongoc, our max length is in bytes and truncation will round lengths down if necessary to avoid splitting a
   // valid UTF-8 sequence. This test repeatedly sends a fake command to the server using every possible maximum
   // length, checking for the expected truncations.

   bson_t command = BSON_INITIALIZER;
   BSON_APPEND_BOOL (&command, "notARealCommand", true);
   BSON_APPEND_UTF8 (&command, "twoByteUtf8", "\xc2\xa9");
   BSON_APPEND_UTF8 (&command, "threeByteUtf8", "\xef\xbf\xbd");
   BSON_APPEND_UTF8 (&command, "fourByteUtf8", "\xf4\x8f\xbf\xbf");

   // Stop testing after $db, before we reach lsid. The result will always be truncated.
   const char *expected_json = "{ \"notARealCommand\" : true, \"twoByteUtf8\" : \"\xc2\xa9\", \"threeByteUtf8\" : "
                               "\"\xef\xbf\xbd\", \"fourByteUtf8\" : \"\xf4\x8f\xbf\xbf\", \"$db\" : \"db\"";
   const int max_expected_length = strlen (expected_json);

   // List of lengths we expect not to see when trying every max_expected_length
   static const int expect_missing_lengths[] = {46, 70, 71, 94, 95, 96};

   mongoc_client_t *client = test_framework_new_default_client ();

   int expected_length = 0;
   for (int test_length = 0; test_length <= max_expected_length; test_length++) {
      MONGOC_DEBUG ("testing length %d of %d", test_length, max_expected_length);

      // Track the expected length of a serialized string with the max_document_length set to 'test_length'.
      // When a length is mentioned in expect_missing_lengths, we let the expected_length lag behind the test_length.
      // At this point, the ellipsis length is not included.
      bool expect_missing = false;
      if (test_length > max_expected_length) {
         expect_missing = true;
      } else {
         mlib_foreach_arr (const int, len, expect_missing_lengths) {
            if (*len == test_length) {
               expect_missing = true;
               break;
            }
         }
      }
      if (!expect_missing) {
         expected_length = test_length;
      }

      // Set up the log options for each command, to test this new max_document_length
      mongoc_structured_log_opts_t *log_opts = mongoc_structured_log_opts_new ();
      ASSERT (mongoc_structured_log_opts_set_max_document_length (log_opts, test_length));
      ASSERT (mongoc_structured_log_opts_set_max_level_for_component (
         log_opts, MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND, MONGOC_STRUCTURED_LOG_LEVEL_DEBUG));

      mongoc_array_t stored_log;
      _mongoc_array_init (&stored_log, sizeof (bson_t *));
      mongoc_structured_log_opts_set_handler (log_opts, stored_log_handler, &stored_log);
      ASSERT (mongoc_client_set_structured_log_opts (client, log_opts));
      mongoc_structured_log_opts_destroy (log_opts);

      ASSERT (!mongoc_client_command_simple (client, "db", &command, NULL, NULL, NULL));

      ASSERT (stored_log.len >= 1);
      bson_t *log = _mongoc_array_index (&stored_log, bson_t *, 0);
      bson_iter_t iter;
      ASSERT (bson_iter_init (&iter, log));
      ASSERT (bson_iter_find (&iter, "message"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "Command started");
      ASSERT (bson_iter_find (&iter, "command"));
      ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      uint32_t logged_command_len;
      const char *logged_command_str = bson_iter_utf8 (&iter, &logged_command_len);
      ASSERT (logged_command_str);
      ASSERT_CMPUINT32 (logged_command_len, ==, expected_length + 3);

      // Note that here we do not use mcommon_string to truncate, just as a convenient way to represent the
      // expected string with ellipsis for ASSERT_CMPSTR. (The code under test internally uses mcommon_string_append_t
      // also.)
      mcommon_string_append_t expected_json_truncated;
      mcommon_string_new_as_append (&expected_json_truncated);
      mcommon_string_append_bytes (&expected_json_truncated, expected_json, expected_length);
      mcommon_string_append (&expected_json_truncated, "...");
      ASSERT_CMPSTR (logged_command_str, mcommon_str_from_append (&expected_json_truncated));
      mcommon_string_from_append_destroy (&expected_json_truncated);

      ASSERT (mongoc_client_set_structured_log_opts (client, NULL));
      stored_log_clear (&stored_log);
      _mongoc_array_destroy (&stored_log);
   }

   mongoc_client_destroy (client);
   bson_destroy (&command);
}

void
test_command_logging_and_monitoring_install (TestSuite *suite)
{
   TestSuite_AddLive (suite, "/command-logging-and-monitoring/logging/prose_test_1", prose_test_1);
   TestSuite_AddLive (suite, "/command-logging-and-monitoring/logging/prose_test_2", prose_test_2);
   TestSuite_AddLive (suite, "/command-logging-and-monitoring/logging/prose_test_3", prose_test_3);
}
