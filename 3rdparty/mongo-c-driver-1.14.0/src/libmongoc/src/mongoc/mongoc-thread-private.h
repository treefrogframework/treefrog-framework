/*
 * Copyright 2013-present MongoDB Inc.
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

#include "mongoc/mongoc-prelude.h"

#ifndef MONGOC_THREAD_PRIVATE_H
#define MONGOC_THREAD_PRIVATE_H

#include <bson/bson.h>

#include "common-thread-private.h"
#include "mongoc/mongoc-config.h"
#include "mongoc/mongoc-log.h"

#if defined(BSON_OS_UNIX)
#define mongoc_cond_t pthread_cond_t
#define mongoc_cond_broadcast pthread_cond_broadcast
#define mongoc_cond_init(_n) pthread_cond_init ((_n), NULL)
#define mongoc_cond_wait pthread_cond_wait
#define mongoc_cond_signal pthread_cond_signal
static BSON_INLINE int
mongoc_cond_timedwait (pthread_cond_t *cond,
                       pthread_mutex_t *mutex,
                       int64_t timeout_msec)
{
   struct timespec to;
   struct timeval tv;
   int64_t msec;

   bson_gettimeofday (&tv);

   msec = ((int64_t) tv.tv_sec * 1000) + (tv.tv_usec / 1000) + timeout_msec;

   to.tv_sec = msec / 1000;
   to.tv_nsec = (msec % 1000) * 1000 * 1000;

   return pthread_cond_timedwait (cond, mutex, &to);
}
#define mongoc_cond_destroy pthread_cond_destroy
#else
#define mongoc_cond_t CONDITION_VARIABLE
#define mongoc_cond_init InitializeConditionVariable
#define mongoc_cond_wait(_c, _m) mongoc_cond_timedwait ((_c), (_m), INFINITE)
static BSON_INLINE int
mongoc_cond_timedwait (mongoc_cond_t *cond,
                       bson_mutex_t *mutex,
                       int64_t timeout_msec)
{
   int r;

   if (SleepConditionVariableCS (cond, mutex, (DWORD) timeout_msec)) {
      return 0;
   } else {
      r = GetLastError ();

      if (r == WAIT_TIMEOUT || r == ERROR_TIMEOUT) {
         return WSAETIMEDOUT;
      } else {
         return EINVAL;
      }
   }
}
#define mongoc_cond_signal WakeConditionVariable
#define mongoc_cond_broadcast WakeAllConditionVariable
static BSON_INLINE int
mongoc_cond_destroy (mongoc_cond_t *_ignored)
{
   return 0;
}
#endif


#endif /* MONGOC_THREAD_PRIVATE_H */
