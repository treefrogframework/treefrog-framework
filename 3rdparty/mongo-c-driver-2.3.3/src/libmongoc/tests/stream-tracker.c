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

#include <stream-tracker.h>

#include <common-thread-private.h>
#include <mongoc/mongoc-client-pool-private.h> // _mongoc_client_pool_set_stream_initiator
#include <mongoc/mongoc-client-private.h>      // mongoc_client_default_stream_initiator
#include <mongoc/mongoc-host-list-private.h>

#include <TestSuite.h>         // ASSERT_OR_PRINT
#include <test-conveniences.h> // tmp_bson
#include <test-libmongoc.h>    // test_framework_*

typedef struct {
   mongoc_host_list_t host;
   int count_active;
   int count_total;
} stream_tracker_entry;

struct stream_tracker_t {
#define STREAM_TRACKER_MAX_ENTRIES 10 // Arbitrary
   stream_tracker_entry entries[STREAM_TRACKER_MAX_ENTRIES];
   bson_mutex_t lock;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
};

stream_tracker_t *
stream_tracker_new(void)
{
   stream_tracker_t *st = bson_malloc0(sizeof(stream_tracker_t));
   bson_mutex_init(&st->lock);
   return st;
}

static mongoc_stream_t *
stream_tracker_initiator(const mongoc_uri_t *uri, const mongoc_host_list_t *host, void *user_data, bson_error_t *error);

void
stream_tracker_track_client(stream_tracker_t *st, mongoc_client_t *client)
{
   BSON_ASSERT_PARAM(st);
   BSON_ASSERT_PARAM(client);

   // Can only track one pool or single-threaded client:
   BSON_ASSERT(!st->pool);
   BSON_ASSERT(!st->client);

   st->client = client;
   mongoc_client_set_stream_initiator(client, stream_tracker_initiator, st);
}

void
stream_tracker_track_pool(stream_tracker_t *st, mongoc_client_pool_t *pool)
{
   BSON_ASSERT_PARAM(st);
   BSON_ASSERT_PARAM(pool);

   // Can only track one pool or single-threaded client:
   BSON_ASSERT(!st->pool);
   BSON_ASSERT(!st->client);

   st->pool = pool;
   _mongoc_client_pool_set_stream_initiator(pool, stream_tracker_initiator, st);
}

int
stream_tracker_count_active(stream_tracker_t *st, const char *host_)
{
   BSON_ASSERT_PARAM(st);
   BSON_ASSERT_PARAM(host_);

   bson_error_t error;
   mongoc_host_list_t host;
   ASSERT_OR_PRINT(_mongoc_host_list_from_string_with_err(&host, host_, &error), error);

   int count = 0;

   // Find matching entry (if present):
   {
      bson_mutex_lock(&st->lock);
      for (size_t i = 0; i < STREAM_TRACKER_MAX_ENTRIES; i++) {
         if (_mongoc_host_list_compare_one(&st->entries[i].host, &host)) {
            count = st->entries[i].count_active;
            break;
         }
      }
      bson_mutex_unlock(&st->lock);
   }

   return count;
}

int
stream_tracker_count_total(stream_tracker_t *st, const char *host_)
{
   BSON_ASSERT_PARAM(st);
   BSON_ASSERT_PARAM(host_);

   bson_error_t error;
   mongoc_host_list_t host;
   ASSERT_OR_PRINT(_mongoc_host_list_from_string_with_err(&host, host_, &error), error);

   int count = 0;

   // Find matching entry (if present):
   {
      bson_mutex_lock(&st->lock);
      for (size_t i = 0; i < STREAM_TRACKER_MAX_ENTRIES; i++) {
         if (_mongoc_host_list_compare_one(&st->entries[i].host, &host)) {
            count = st->entries[i].count_total;
            break;
         }
      }
      bson_mutex_unlock(&st->lock);
   }

   return count;
}

static void
stream_tracker_increment(stream_tracker_t *st, const mongoc_host_list_t *host)
{
   BSON_ASSERT_PARAM(st);
   BSON_ASSERT_PARAM(host);

   bson_mutex_lock(&st->lock);
   // Find (or create) matching entry.
   for (size_t i = 0; i < STREAM_TRACKER_MAX_ENTRIES; i++) {
      if (0 == strlen(st->entries[i].host.host_and_port)) {
         // No matching entry. Create one.
         st->entries[i].host = *host;
         st->entries[i].count_active = 1;
         st->entries[i].count_total = 1;
         bson_mutex_unlock(&st->lock);
         return;
      }
      if (_mongoc_host_list_compare_one(&st->entries[i].host, host)) {
         st->entries[i].count_active++;
         st->entries[i].count_total++;
         bson_mutex_unlock(&st->lock);
         return;
      }
   }

   test_error("No room to add %s. Increase STREAM_TRACKER_MAX_ENTRIES.", host->host_and_port);
}

static void
stream_tracker_decrement(stream_tracker_t *st, const mongoc_host_list_t *host)
{
   BSON_ASSERT_PARAM(st);
   BSON_ASSERT_PARAM(host);

   bson_mutex_lock(&st->lock);
   // Find matching entry.
   for (size_t i = 0; i < STREAM_TRACKER_MAX_ENTRIES; i++) {
      if (0 == strlen(st->entries[i].host.host_and_port)) {
         test_error("Unexpected: no matching entry for %s", st->entries[i].host.host_and_port);
      }
      if (_mongoc_host_list_compare_one(&st->entries[i].host, host)) {
         ASSERT(st->entries[i].count_active > 0);
         st->entries[i].count_active--;
         bson_mutex_unlock(&st->lock);
         return;
      }
   }

   test_error("Unexpected. No matching entry to decrement!");
}

void
stream_tracker_destroy(stream_tracker_t *st)
{
   if (!st) {
      return;
   }
   bson_mutex_destroy(&st->lock);
   bson_free(st);
}

// tracked_stream_t wraps a mongoc_stream_t and updates a linked stream_tracker.
#define MONGOC_STREAM_TRACKED 8
typedef struct {
   mongoc_stream_t vtable;
   mongoc_stream_t *wrapped;
   mongoc_host_list_t host;
   stream_tracker_t *st;
} tracked_stream_t;

static int
tracked_stream_close(mongoc_stream_t *stream)
{
   BSON_ASSERT_PARAM(stream);
   return mongoc_stream_close(((tracked_stream_t *)stream)->wrapped);
}


static void
tracked_stream_destroy(mongoc_stream_t *stream)
{
   BSON_ASSERT_PARAM(stream);
   tracked_stream_t *ts = (tracked_stream_t *)stream;
   stream_tracker_decrement(ts->st, &ts->host);
   mongoc_stream_destroy(ts->wrapped);
   bson_free(ts);
}


static void
tracked_stream_failed(mongoc_stream_t *stream)
{
   BSON_ASSERT_PARAM(stream);
   tracked_stream_t *ts = (tracked_stream_t *)stream;
   stream_tracker_decrement(ts->st, &ts->host);
   mongoc_stream_failed(ts->wrapped);
   bson_free(ts);
}


static int
tracked_stream_setsockopt(mongoc_stream_t *stream, int level, int optname, void *optval, mongoc_socklen_t optlen)
{
   BSON_ASSERT_PARAM(stream);
   return mongoc_stream_setsockopt(((tracked_stream_t *)stream)->wrapped, level, optname, optval, optlen);
}


static int
tracked_stream_flush(mongoc_stream_t *stream)
{
   BSON_ASSERT_PARAM(stream);
   return mongoc_stream_flush(((tracked_stream_t *)stream)->wrapped);
}


static ssize_t
tracked_stream_readv(
   mongoc_stream_t *stream, mongoc_iovec_t *iov, size_t iovcnt, size_t min_bytes, int32_t timeout_msec)
{
   BSON_ASSERT_PARAM(stream);
   return mongoc_stream_readv(((tracked_stream_t *)stream)->wrapped, iov, iovcnt, min_bytes, timeout_msec);
}


static ssize_t
tracked_stream_writev(mongoc_stream_t *stream, mongoc_iovec_t *iov, size_t iovcnt, int32_t timeout_msec)
{
   BSON_ASSERT_PARAM(stream);
   return mongoc_stream_writev(((tracked_stream_t *)stream)->wrapped, iov, iovcnt, timeout_msec);
}


static bool
tracked_stream_check_closed(mongoc_stream_t *stream)
{
   BSON_ASSERT_PARAM(stream);
   return mongoc_stream_check_closed(((tracked_stream_t *)stream)->wrapped);
}


static bool
tracked_stream_timed_out(mongoc_stream_t *stream)
{
   BSON_ASSERT_PARAM(stream);
   return mongoc_stream_timed_out(((tracked_stream_t *)stream)->wrapped);
}


static bool
tracked_stream_should_retry(mongoc_stream_t *stream)
{
   BSON_ASSERT_PARAM(stream);
   return mongoc_stream_should_retry(((tracked_stream_t *)stream)->wrapped);
}


static mongoc_stream_t *
tracked_stream_get_base_stream(mongoc_stream_t *stream)
{
   BSON_ASSERT_PARAM(stream);
   mongoc_stream_t *wrapped = ((tracked_stream_t *)stream)->wrapped;

   if (wrapped->get_base_stream) {
      return wrapped->get_base_stream(wrapped);
   }

   return wrapped;
}


static mongoc_stream_t *
tracked_stream_new(mongoc_stream_t *stream, stream_tracker_t *st, const mongoc_host_list_t *host)
{
   BSON_ASSERT_PARAM(stream);
   BSON_ASSERT_PARAM(st);
   BSON_ASSERT_PARAM(host);

   tracked_stream_t *ts = (tracked_stream_t *)bson_malloc0(sizeof(tracked_stream_t));

   // Set vtable to wrapper functions:
   ts->vtable.type = MONGOC_STREAM_TRACKED;
   ts->vtable.close = tracked_stream_close;
   ts->vtable.destroy = tracked_stream_destroy;
   ts->vtable.failed = tracked_stream_failed;
   ts->vtable.flush = tracked_stream_flush;
   ts->vtable.readv = tracked_stream_readv;
   ts->vtable.writev = tracked_stream_writev;
   ts->vtable.setsockopt = tracked_stream_setsockopt;
   ts->vtable.check_closed = tracked_stream_check_closed;
   ts->vtable.timed_out = tracked_stream_timed_out;
   ts->vtable.should_retry = tracked_stream_should_retry;
   ts->vtable.get_base_stream = tracked_stream_get_base_stream;

   // Wrap base stream:
   ts->wrapped = stream;

   // Set data for tracking:
   ts->st = st;
   ts->host = *host;

   // Record a new stream created to host:
   stream_tracker_increment(ts->st, &ts->host);

   return (mongoc_stream_t *)ts;
}

mongoc_stream_t *
stream_tracker_initiator(const mongoc_uri_t *uri, const mongoc_host_list_t *host, void *user_data, bson_error_t *error)
{
   BSON_ASSERT_PARAM(uri);
   BSON_ASSERT_PARAM(host);
   BSON_ASSERT_PARAM(user_data);
   BSON_ASSERT_PARAM(error);

   stream_tracker_t *st = (stream_tracker_t *)user_data;

   // mongoc_client_default_stream_initiator expects a client context. If tracking a pool, pop a temporary client:
   mongoc_client_t *client = (st->pool) ? mongoc_client_pool_pop(st->pool) : st->client;
   ASSERT(client);

   mongoc_stream_t *base_stream = mongoc_client_default_stream_initiator(uri, host, client, error);
   ASSERT_OR_PRINT(base_stream, (*error));

   if (st->pool) {
      mongoc_client_pool_push(st->pool, client);
   }
   return tracked_stream_new(base_stream, st, host);
}

static void
test_stream_tracker(void)
{
   // Get first host+port from test environment. Example: "localhost:27017" or "[::1]:27017"
   char *first_host_and_port = test_framework_get_host_and_port();

   // Test single-threaded client:
   {
      stream_tracker_t *st = stream_tracker_new();
      mongoc_client_t *client = test_framework_new_default_client();
      stream_tracker_track_client(st, client);

      // Expect initial count is 0:
      stream_tracker_assert_active_count(st, first_host_and_port, 0);

      // Do operation requiring a stream. Target first host:
      bson_error_t error;
      ASSERT_OR_PRINT(mongoc_client_command_simple_with_server_id(
                         client, "admin", tmp_bson("{'ping': 1}"), NULL, 1 /* server ID */, NULL, &error),
                      error);

      // Expect active and total count incremented:
      stream_tracker_assert_active_count(st, first_host_and_port, 1);
      stream_tracker_assert_total_count(st, first_host_and_port, 1);

      // Destroy stream:
      mongoc_client_destroy(client);

      // Expect active count decremented:
      stream_tracker_assert_active_count(st, first_host_and_port, 0);
      // Expect total count unchanged:
      stream_tracker_assert_total_count(st, first_host_and_port, 1);

      stream_tracker_destroy(st);
   }

   // Test client-pool:
   {
      stream_tracker_t *st = stream_tracker_new();
      mongoc_client_pool_t *pool = test_framework_new_default_client_pool();
      stream_tracker_track_pool(st, pool);

      // Expect initial count is 0:
      stream_tracker_assert_active_count(st, first_host_and_port, 0);

      // Pop a client, triggering background connections to be created:
      mongoc_client_t *client = mongoc_client_pool_pop(pool);

      // Server 4.4 added support for streaming monitoring and has 2 monitoring connections.
      int monitor_count = test_framework_get_server_version() >= test_framework_str_to_version("4.4") ? 2 : 1;
      stream_tracker_assert_eventual_active_count(st, first_host_and_port, monitor_count);

      // Do operation requiring a stream. Target first host:
      bson_error_t error;
      ASSERT_OR_PRINT(mongoc_client_command_simple_with_server_id(
                         client, "admin", tmp_bson("{'ping': 1}"), NULL, 1 /* server ID */, NULL, &error),
                      error);

      // Expect active and total count incremented:
      stream_tracker_assert_active_count(st, first_host_and_port, monitor_count + 1);
      stream_tracker_assert_total_count(st, first_host_and_port, monitor_count + 1);

      // Destroy pool.
      mongoc_client_pool_push(pool, client);
      mongoc_client_pool_destroy(pool);

      // Expect active count decremented:
      stream_tracker_assert_active_count(st, first_host_and_port, 0);
      // Expect total count unchanged:
      stream_tracker_assert_total_count(st, first_host_and_port, monitor_count + 1);

      stream_tracker_destroy(st);
   }

   bson_free(first_host_and_port);
}

void
test_stream_tracker_install(TestSuite *suite)
{
   TestSuite_AddLive(suite, "/stream_tracker/selftest", test_stream_tracker);
}
