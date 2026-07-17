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

#include <mongoc/mongoc-http-private.h>

#include <mongoc/mongoc.h>

#include <mlib/duration.h>
#include <mlib/test.h>
#include <mlib/timer.h>

#include <TestSuite.h>
#include <test-libmongoc.h>

void
test_mongoc_http_get(void *unused)
{
   mongoc_http_request_t req;
   mongoc_http_response_t res;
   bool r;
   bson_error_t error = {0};

   BSON_UNUSED(unused);

   _mongoc_http_request_init(&req);
   _mongoc_http_response_init(&res);

   /* Basic GET request */
   req.method = "GET";
   req.host = "localhost";
   req.path = "get";
   req.port = 18000;
   r = _mongoc_http_send(&req, mlib_expires_after(mlib_duration(10, s)), false, NULL, &res, &error);
   ASSERT_OR_PRINT(r, error);

   ASSERT_WITH_MSG(res.status == 200,
                   "unexpected status code %d\n"
                   "RESPONSE BODY BEGIN\n"
                   "%s"
                   "RESPONSE BODY END\n",
                   res.status,
                   res.body_len > 0 ? res.body : "");
   ASSERT_CMPINT(res.body_len, >, 0);
   _mongoc_http_response_cleanup(&res);
}

void
test_mongoc_http_post(void *unused)
{
   mongoc_http_request_t req;
   mongoc_http_response_t res;
   bool r;
   bson_error_t error = {0};

   BSON_UNUSED(unused);

   _mongoc_http_request_init(&req);
   _mongoc_http_response_init(&res);

   /* Basic POST request with a body. */
   req.method = "POST";
   req.host = "localhost";
   req.path = "post";
   req.port = 18000;
   r = _mongoc_http_send(&req, mlib_expires_after(mlib_duration(10, s)), false, NULL, &res, &error);
   ASSERT_OR_PRINT(r, error);

   ASSERT_WITH_MSG(res.status == 200,
                   "unexpected status code %d\n"
                   "RESPONSE BODY BEGIN\n"
                   "%s"
                   "RESPONSE BODY END\n",
                   res.status,
                   res.body_len > 0 ? res.body : "");
   ASSERT_CMPINT(res.body_len, >, 0);
   _mongoc_http_response_cleanup(&res);
}

static void
_init_valid_req(mongoc_http_request_t *req)
{
   _mongoc_http_request_init(req);
   req->method = "GET";
   req->host = "localhost";
   req->path = "/foo";
   req->port = 80;
}

static void
test_mongoc_http_validate(void)
{
   mongoc_http_request_t req;
   bson_error_t error;

   /* baseline: a well-formed request passes */
   _init_valid_req(&req);
   mlib_check(_mongoc_http_request_validate(&req, &error));

   /* method: empty */
   _init_valid_req(&req);
   req.method = "";
   mlib_check(!_mongoc_http_request_validate(&req, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_STATE, "method");

   /* method: contains CR */
   _init_valid_req(&req);
   req.method = "GET\r";
   mlib_check(!_mongoc_http_request_validate(&req, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_STATE, "method");

   /* method: contains LF */
   _init_valid_req(&req);
   req.method = "GET\n";
   mlib_check(!_mongoc_http_request_validate(&req, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_STATE, "method");

   /* method: contains space (would split the request line) */
   _init_valid_req(&req);
   req.method = "GE T";
   mlib_check(!_mongoc_http_request_validate(&req, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_STATE, "method");

   /* host: empty */
   _init_valid_req(&req);
   req.host = "";
   mlib_check(!_mongoc_http_request_validate(&req, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_STATE, "host");

   /* host: CRLF injection */
   _init_valid_req(&req);
   req.host = "evil.com\r\nInjected: header";
   mlib_check(!_mongoc_http_request_validate(&req, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_STATE, "host");

   /* path: CRLF injection */
   _init_valid_req(&req);
   req.path = "/legit\r\nInjected: header";
   mlib_check(!_mongoc_http_request_validate(&req, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_STATE, "path");

   /* path: bare LF injection */
   _init_valid_req(&req);
   req.path = "/legit\nInjected";
   mlib_check(!_mongoc_http_request_validate(&req, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_STATE, "path");

   /* path: space splits the request line (GET /le git HTTP/1.0) */
   _init_valid_req(&req);
   req.path = "/le git";
   mlib_check(!_mongoc_http_request_validate(&req, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_STATE, "path");

   /* path: NULL is allowed (defaults to /) */
   _init_valid_req(&req);
   req.path = NULL;
   mlib_check(_mongoc_http_request_validate(&req, &error));

   /* extra_headers: well-formed headers pass */
   _init_valid_req(&req);
   req.extra_headers = "X-Foo: bar\r\nX-Baz: qux\r\n";
   mlib_check(_mongoc_http_request_validate(&req, &error));

   /* extra_headers: blank line terminates headers, allowing body injection */
   _init_valid_req(&req);
   req.extra_headers = "X-Foo: bar\r\n\r\nINJECTED BODY";
   mlib_check(!_mongoc_http_request_validate(&req, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_STATE, "extra_headers");

   /* extra_headers: leading CRLF combines with the preceding header's terminator to end headers early */
   _init_valid_req(&req);
   req.extra_headers = "\r\nX-Injected: header";
   mlib_check(!_mongoc_http_request_validate(&req, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_STATE, "extra_headers");

   /* extra_headers: NULL is allowed */
   _init_valid_req(&req);
   req.extra_headers = NULL;
   mlib_check(_mongoc_http_request_validate(&req, &error));
}

void
test_http_install(TestSuite *suite)
{
   TestSuite_Add(suite, "/http/validate", test_mongoc_http_validate);
   TestSuite_AddFull(suite,
                     "/http/get [uses:simple-http-server-18000]",
                     test_mongoc_http_get,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_offline);

   TestSuite_AddFull(suite,
                     "/http/post [uses:simple-http-server-18000]",
                     test_mongoc_http_post,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_offline);
}
