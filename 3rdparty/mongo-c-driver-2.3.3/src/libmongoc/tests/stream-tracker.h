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

#ifndef STREAM_TRACKER_H
#define STREAM_TRACKER_H

#include <mongoc/mongoc-client-pool.h>
#include <mongoc/mongoc-client.h>

#include <mlib/timer.h>

#include <stdint.h>

// stream_tracker_t is a test utility to count streams created to servers.
typedef struct stream_tracker_t stream_tracker_t;

stream_tracker_t *
stream_tracker_new(void);

// stream_tracker_track_client tracks streams in a single-threaded client.
void
stream_tracker_track_client(stream_tracker_t *st, mongoc_client_t *client);

// stream_tracker_track_pool tracks streams in a pool. Call before calling mongoc_client_pool_pop.
void
stream_tracker_track_pool(stream_tracker_t *st, mongoc_client_pool_t *pool);

// stream_tracker_count_active returns a count of active streams.
int
stream_tracker_count_active(stream_tracker_t *st, const char *host);

// stream_tracker_count_total returns a cumulative count of streams.
int
stream_tracker_count_total(stream_tracker_t *st, const char *host);

void
stream_tracker_destroy(stream_tracker_t *st);

#define stream_tracker_assert_active_count(st, host, expect)      \
   if (1) {                                                       \
      int _got = stream_tracker_count_active(st, host);           \
      if (_got != expect) {                                       \
         test_error("Got unexpected active stream count to %s:\n" \
                    "  Expected %d, got %d",                      \
                    host,                                         \
                    expect,                                       \
                    _got);                                        \
      }                                                           \
   } else                                                         \
      ((void)0)

#define stream_tracker_assert_total_count(st, host, expect)      \
   if (1) {                                                      \
      int _got = stream_tracker_count_total(st, host);           \
      if (_got != expect) {                                      \
         test_error("Got unexpected total stream count to %s:\n" \
                    "  Expected %d, got %d",                     \
                    host,                                        \
                    expect,                                      \
                    _got);                                       \
      }                                                          \
   } else                                                        \
      ((void)0)

#define stream_tracker_assert_eventual_active_count(st, host, expect)                \
   if (1) {                                                                          \
      mlib_timer _timer = mlib_expires_after(5, s);                                  \
      while (true) {                                                                 \
         int _got = stream_tracker_count_active(st, host);                           \
         if (_got == expect) {                                                       \
            break;                                                                   \
         }                                                                           \
         if (mlib_timer_is_expired(_timer)) {                                        \
            test_error("Timed out waiting for expected active stream count to %s:\n" \
                       "  Expected %d, got %d",                                      \
                       host,                                                         \
                       expect,                                                       \
                       _got);                                                        \
         }                                                                           \
         mlib_sleep_for(100, ms);                                                    \
      }                                                                              \
   } else                                                                            \
      ((void)0)

#endif // STREAM_TRACKER_H
