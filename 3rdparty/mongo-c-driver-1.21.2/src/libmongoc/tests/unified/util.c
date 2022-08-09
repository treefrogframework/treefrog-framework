/*
 * Copyright 2020-present MongoDB, Inc.
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

#include "util.h"

#include "test-conveniences.h"
#include "TestSuite.h"

uint8_t *
hex_to_bin (const char *hex, uint32_t *len)
{
   int i;
   int hex_len;
   uint8_t *out;

   hex_len = strlen (hex);
   if (hex_len % 2 != 0) {
      return NULL;
   }

   *len = hex_len / 2;
   out = bson_malloc0 (*len);

   for (i = 0; i < hex_len; i += 2) {
      uint32_t hex_char;

      if (1 != sscanf (hex + i, "%2x", &hex_char)) {
         bson_free (out);
         return NULL;
      }
      out[i / 2] = (uint8_t) hex_char;
   }
   return out;
}

char *
bin_to_hex (const uint8_t *bin, uint32_t len)
{
   char *out = bson_malloc0 (2 * len + 1);
   uint32_t i;

   for (i = 0; i < len; i++) {
      bson_snprintf (out + (2 * i), 2, "%02x", bin[i]);
   }
   return out;
}

static int
cmp_key (const void *a, const void *b)
{
   return strcmp (*(const char **) a, *(const char **) b);
}

bson_t *
bson_copy_and_sort (const bson_t *in)
{
   bson_t *out = bson_new ();
   bson_iter_t iter;
   const char **keys;
   int nkeys = bson_count_keys (in);
   int i = 0;

   keys = bson_malloc0 (sizeof (const char *) * nkeys);
   BSON_FOREACH (in, iter)
   {
      keys[i] = bson_iter_key (&iter);
      i++;
   }

   qsort ((void *) keys, nkeys, sizeof (const char *), cmp_key);
   for (i = 0; i < nkeys; i++) {
      BSON_ASSERT (bson_iter_init_find (&iter, in, keys[i]));
      BSON_APPEND_VALUE (out, keys[i], bson_iter_value (&iter));
   }
   bson_free ((void *) keys);
   return out;
}

typedef struct {
   const char *str;
   bson_type_t type;
} bson_string_and_type_t;

/* List of aliases: https://docs.mongodb.com/manual/reference/bson-types/ */
bson_string_and_type_t bson_type_map[] = {
   {"double", BSON_TYPE_DOUBLE},
   {"string", BSON_TYPE_UTF8},
   {"object", BSON_TYPE_DOCUMENT},
   {"array", BSON_TYPE_ARRAY},
   {"binData", BSON_TYPE_BINARY},
   {"undefined", BSON_TYPE_UNDEFINED},
   {"objectId", BSON_TYPE_OID},
   {"bool", BSON_TYPE_BOOL},
   {"date", BSON_TYPE_DATE_TIME},
   {"null", BSON_TYPE_NULL},
   {"regex", BSON_TYPE_REGEX},
   {"dbPointer", BSON_TYPE_DBPOINTER},
   {"javascript", BSON_TYPE_CODE},
   {"javascriptWithScope", BSON_TYPE_CODEWSCOPE},
   {"int", BSON_TYPE_INT32},
   {"timestamp", BSON_TYPE_TIMESTAMP},
   {"long", BSON_TYPE_INT64},
   {"decimal", BSON_TYPE_DECIMAL128},
   {"minKey", BSON_TYPE_MINKEY},
   {"maxKey", BSON_TYPE_MAXKEY},
   {"eod", BSON_TYPE_EOD},
};

bson_type_t
bson_type_from_string (const char *in)
{
   int i;
   for (i = 0; i < sizeof (bson_type_map) / sizeof (bson_type_map[0]); i++) {
      if (0 == strcmp (in, bson_type_map[i].str)) {
         return bson_type_map[i].type;
      }
   }
   test_error ("unrecognized type string: %s\n", in);
   return BSON_TYPE_EOD;
}

const char *
bson_type_to_string (bson_type_t btype)
{
   int i;
   for (i = 0; i < sizeof (bson_type_map) / sizeof (bson_type_map[0]); i++) {
      if (btype == bson_type_map[i].type) {
         return bson_type_map[i].str;
      }
   }
   test_error ("unrecognized type: %d\n", (int) btype);
   return "invalid";
}

static void
test_copy_and_sort (void)
{
   bson_t *in = tmp_bson ("{'b': 1, 'a': 1, 'd': 1, 'c': 1}");
   bson_t *expect = tmp_bson ("{'a': 1, 'b': 1, 'c': 1, 'd': 1}");
   bson_t *out = bson_copy_and_sort (in);
   if (!bson_equal (expect, out)) {
      test_error ("expected: %s, got: %s", tmp_json (expect), tmp_json (out));
   }
   bson_destroy (out);
}

void
test_bson_util_install (TestSuite *suite)
{
   TestSuite_Add (
      suite, "/unified/selftest/util/copy_and_sort", test_copy_and_sort);
}

/* TODO (CDRIVER-3525) add test support for CMAP events once the C driver
 * supports CMAP. */
bool
is_unsupported_event_type (const char *event_type)
{
   char *unsupported_event_types[] = {"poolCreatedEvent",
                                      "poolReadyEvent",
                                      "poolClearedEvent",
                                      "poolClosedEvent",
                                      "connectionCreatedEvent",
                                      "connectionReadyEvent",
                                      "connectionClosedEvent",
                                      "connectionCheckOutStartedEvent",
                                      "connectionCheckOutFailedEvent",
                                      "connectionCheckedOutEvent",
                                      "connectionCheckedInEvent",
                                      NULL};

   char **iter;

   for (iter = unsupported_event_types; *iter != NULL; iter++) {
      if (0 == strcmp (event_type, *iter)) {
         return true;
      }
   }
   return false;
}
