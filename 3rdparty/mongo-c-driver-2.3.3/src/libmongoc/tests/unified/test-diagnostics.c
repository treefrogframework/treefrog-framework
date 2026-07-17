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

#include "./test-diagnostics.h"

#include <common-string-private.h>
#include <common-thread-private.h>

#include <mongoc/utlist.h>

#include <TestSuite.h>

#include <signal.h>

typedef struct _msg_t {
   char *string;
   struct _msg_t *next;
} diagnostics_msg_t;

typedef struct {
   diagnostics_msg_t *test_info;
   diagnostics_msg_t *error_info;
   bson_mutex_t mutex;
} test_diagnostics_t;

static test_diagnostics_t diagnostics;

static char *
test_diagnostics_error_string(bson_error_t *error)
{
   diagnostics_msg_t *msg_iter = NULL;
   test_diagnostics_t *td = &diagnostics;

   mcommon_string_append_t str;
   mcommon_string_new_as_append(&str);

   /* Give a large header / footer to make the error easily grep-able */
   mcommon_string_append(&str,
                         "****************************** BEGIN_MONGOC_ERROR "
                         "******************************\n");

   bson_mutex_lock(&td->mutex);
   if (td->test_info) {
      mcommon_string_append(&str, "test info:\n");
   }

   LL_FOREACH(td->test_info, msg_iter)
   {
      mcommon_string_append(&str, msg_iter->string);
      mcommon_string_append(&str, "\n");
   }

   mcommon_string_append(&str, "\n");

   if (td->error_info) {
      mcommon_string_append(&str, "error context:\n");
   }

   LL_FOREACH(td->error_info, msg_iter)
   {
      mcommon_string_append(&str, msg_iter->string);
      mcommon_string_append(&str, "\n\n");
   }

   bson_mutex_unlock(&td->mutex);

   if (error && error->code != 0) {
      mcommon_string_append_printf(&str, "error: %s\n", error->message);
   }

   mcommon_string_append(&str,
                         "******************************* END_MONGOC_ERROR "
                         "*******************************\n");

   return mcommon_string_from_append_destroy_with_steal(&str);
}

static void
handle_abort(int signo)
{
   BSON_UNUSED(signo);
   MONGOC_ERROR("abort handler entered");
   char *const msg = test_diagnostics_error_string(NULL);
   fprintf(stderr, "%s", msg);
   bson_free(msg);
}

void
test_diagnostics_init(void)
{
   test_diagnostics_t *td = &diagnostics;
   memset(td, 0, sizeof(test_diagnostics_t));
   bson_mutex_init(&td->mutex);
   signal(SIGABRT, handle_abort);
}

void
test_diagnostics_reset(void)
{
   test_diagnostics_t *td = &diagnostics;
   diagnostics_msg_t *iter, *iter_tmp;

   LL_FOREACH_SAFE(td->test_info, iter, iter_tmp)
   {
      bson_free(iter->string);
      bson_free(iter);
   }

   LL_FOREACH_SAFE(td->error_info, iter, iter_tmp)
   {
      bson_free(iter->string);
      bson_free(iter);
   }

   td->test_info = NULL;
   td->error_info = NULL;
}

void
test_diagnostics_cleanup(void)
{
   test_diagnostics_t *td = &diagnostics;

   test_diagnostics_reset();
   bson_mutex_destroy(&td->mutex);
   signal(SIGABRT, SIG_DFL);
}

void
_test_diagnostics_add(bool fail, const char *fmt, ...)
{
   test_diagnostics_t *td = &diagnostics;
   va_list args;
   diagnostics_msg_t *msg = NULL;
   char *msg_string;

   va_start(args, fmt);
   msg_string = bson_strdupv_printf(fmt, args);
   va_end(args);

   msg = bson_malloc0(sizeof(diagnostics_msg_t));
   msg->string = msg_string;

   bson_mutex_lock(&td->mutex);
   if (fail) {
      LL_PREPEND(td->error_info, msg);
   } else {
      LL_APPEND(td->test_info, msg);
   }
   bson_mutex_unlock(&td->mutex);
   MONGOC_DEBUG("%s", msg_string);
}

void
test_diagnostics_abort(bson_error_t *error)
{
   signal(SIGABRT, SIG_DFL);
   test_error("%s", test_diagnostics_error_string(error));
}
