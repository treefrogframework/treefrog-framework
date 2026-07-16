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

#include "./util.h"

#include <mlib/cmp.h>
#include <mlib/loop.h>

#include <TestSuite.h>
#include <test-conveniences.h>

static int
cmp_key(const void *a, const void *b)
{
   return strcmp(*(const char **)a, *(const char **)b);
}

bson_t *
bson_copy_and_sort(const bson_t *in)
{
   bson_t *out = bson_new();
   bson_iter_t iter;
   const char **keys;
   int nkeys = bson_count_keys(in);
   int i = 0;

   keys = bson_malloc0(sizeof(const char *) * nkeys);
   BSON_FOREACH(in, iter)
   {
      keys[i] = bson_iter_key(&iter);
      i++;
   }

   qsort((void *)keys, nkeys, sizeof(const char *), cmp_key);
   for (i = 0; i < nkeys; i++) {
      BSON_ASSERT(bson_iter_init_find(&iter, in, keys[i]));
      BSON_APPEND_VALUE(out, keys[i], bson_iter_value(&iter));
   }
   bson_free((void *)keys);
   return out;
}

typedef struct {
   const char *str;
   bson_type_t type;
} mcommon_string_and_type_t;

/* List of aliases: https://www.mongodb.com/docs/manual/reference/bson-types/ */
mcommon_string_and_type_t bson_type_map[] = {
   {"double", BSON_TYPE_DOUBLE},   {"string", BSON_TYPE_UTF8},
   {"object", BSON_TYPE_DOCUMENT}, {"array", BSON_TYPE_ARRAY},
   {"binData", BSON_TYPE_BINARY},  {"undefined", BSON_TYPE_UNDEFINED},
   {"objectId", BSON_TYPE_OID},    {"bool", BSON_TYPE_BOOL},
   {"date", BSON_TYPE_DATE_TIME},  {"null", BSON_TYPE_NULL},
   {"regex", BSON_TYPE_REGEX},     {"dbPointer", BSON_TYPE_DBPOINTER},
   {"javascript", BSON_TYPE_CODE}, {"javascriptWithScope", BSON_TYPE_CODEWSCOPE},
   {"int", BSON_TYPE_INT32},       {"timestamp", BSON_TYPE_TIMESTAMP},
   {"long", BSON_TYPE_INT64},      {"decimal", BSON_TYPE_DECIMAL128},
   {"minKey", BSON_TYPE_MINKEY},   {"maxKey", BSON_TYPE_MAXKEY},
   {"eod", BSON_TYPE_EOD},
};

bson_type_t
bson_type_from_string(const char *in)
{
   mlib_foreach_arr (mcommon_string_and_type_t, it, bson_type_map) {
      if (0 == strcmp(in, it->str)) {
         return it->type;
      }
   }
   test_error("unrecognized type string: %s\n", in);
   return BSON_TYPE_EOD;
}

const char *
bson_type_to_string(bson_type_t btype)
{
   mlib_foreach_arr (mcommon_string_and_type_t, it, bson_type_map) {
      if (btype == it->type) {
         return it->str;
      }
   }
   test_error("unrecognized type: %d\n", (int)btype);
   return "invalid";
}

static void
test_copy_and_sort(void)
{
   bson_t *in = tmp_bson("{'b': 1, 'a': 1, 'd': 1, 'c': 1}");
   bson_t *expect = tmp_bson("{'a': 1, 'b': 1, 'c': 1, 'd': 1}");
   bson_t *out = bson_copy_and_sort(in);
   if (!bson_equal(expect, out)) {
      test_error("expected: %s, got: %s", tmp_json(expect), tmp_json(out));
   }
   bson_destroy(out);
}

void
test_bson_util_install(TestSuite *suite)
{
   TestSuite_Add(suite, "/unified/selftest/util/copy_and_sort", test_copy_and_sort);
}

/* CMAP (CDRIVER-3525) isn't planned for implementation in this driver. Many tests that would otherwise require CMAP
 * are used in partial form by skipping individual checks that involve unsupported events. */
bool
is_unsupported_event_type(const char *event_type)
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
      if (0 == bson_strcasecmp(event_type, *iter)) {
         return true;
      }
   }
   return false;
}

int64_t
usecs_since_epoch(void)
{
   struct timeval tv;
   BSON_ASSERT(bson_gettimeofday(&tv) == 0);

   BSON_ASSERT(mlib_in_range(int64_t, tv.tv_sec));
   BSON_ASSERT(mlib_in_range(int64_t, tv.tv_usec));

   const int64_t secs = (int64_t)tv.tv_sec;
   const int64_t usecs = (int64_t)tv.tv_usec;

   const int64_t factor = 1000000;

   BSON_ASSERT(INT64_MAX / factor >= secs);
   BSON_ASSERT(INT64_MAX - (factor * secs) >= usecs);

   return secs * factor + usecs;
}

const char *
mongoc_strcasestr(const char *haystack, const char *needle)
{
   BSON_ASSERT_PARAM(haystack);
   BSON_ASSERT_PARAM(needle);

   if (!*needle) {
      return haystack;
   }

   const char *h = haystack;
   while (*h) {
      const char *h_start = h;
      const char *n = needle;

      while (*n && *h && tolower(*h) == tolower(*n)) {
         h++;
         n++;
      }

      if (!*n) {
         return h_start; // Match found
      }

      h = h_start + 1; // Move to the next starting position in haystack
   }

   return NULL; // No match found
}
