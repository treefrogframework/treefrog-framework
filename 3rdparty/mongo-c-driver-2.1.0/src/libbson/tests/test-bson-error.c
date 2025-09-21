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

#include <bson/bson.h>

#include <mlib/test.h>

#include <TestSuite.h>


static void
test_bson_error_basic (void)
{
   bson_error_t error;

   bson_set_error (&error, 123, 456, "%s:%d", "localhost", 27017);
   ASSERT_CMPSTR (error.message, "localhost:27017");
   ASSERT_CMPUINT32 (error.domain, ==, 123u);
   ASSERT_CMPUINT32 (error.code, ==, 456u);
   ASSERT_CMPUINT (error.reserved, ==, 1u); // BSON_ERROR_CATEGORY
}

static void
test_bson_error_clear (void)
{
   bson_error_t err;
   err.code = 42;
   err.domain = 1729;
   bson_error_clear (&err);
   mlib_check (err.code, eq, 0);
   mlib_check (err.domain, eq, 0);

   // Valid no-op:
   bson_error_clear (NULL);
}

static void
test_bson_error_reset (void)
{
   bson_error_t err;
   bson_error_t *eptr = &err;
   err.code = 42;
   bson_error_reset (eptr);
   mlib_check (eptr, ptr_eq, &err);
   mlib_check (err.code, eq, 0);

   eptr = NULL;
   bson_error_reset (eptr);
   mlib_check (eptr != NULL, because, "bson_error_reset sets null pointers to non-null");
   mlib_check (eptr->code, eq, 0);
}

static void
test_bson_strerror_r (void)
{
   FILE *f = fopen ("file-that-does-not-exist", "r");
   ASSERT (!f);
   char errmsg_buf[BSON_ERROR_BUFFER_SIZE];
   char *errmsg = bson_strerror_r (errno, errmsg_buf, sizeof errmsg_buf);
   // Check a message is returned. Do not check platform-dependent contents:
   ASSERT (errmsg);
   const char *unknown_msg = "Unknown error";
   if (strstr (errmsg, unknown_msg)) {
      test_error ("Expected error message to contain platform-dependent content, not: '%s'", errmsg);
   }
}

void
test_bson_error_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/bson/error/basic", test_bson_error_basic);
   TestSuite_Add (suite, "/bson/error/clear", test_bson_error_clear);
   TestSuite_Add (suite, "/bson/error/reset", test_bson_error_reset);
   TestSuite_Add (suite, "/bson/strerror_r", test_bson_strerror_r);
}
