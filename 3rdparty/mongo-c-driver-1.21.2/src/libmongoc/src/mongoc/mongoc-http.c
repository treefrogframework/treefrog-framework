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

#include "mongoc-http-private.h"

#include "mongoc-client-private.h"
#include "mongoc-host-list-private.h"
#include "mongoc-stream-tls.h"
#include "mongoc-stream-private.h"
#include "mongoc-buffer-private.h"

void
_mongoc_http_request_init (mongoc_http_request_t *request)
{
   memset (request, 0, sizeof (*request));
}

void
_mongoc_http_response_init (mongoc_http_response_t *response)
{
   memset (response, 0, sizeof (*response));
}

void
_mongoc_http_response_cleanup (mongoc_http_response_t *response)
{
   if (!response) {
      return;
   }
   bson_free (response->headers);
   bson_free (response->body);
}

bool
_mongoc_http_send (mongoc_http_request_t *req,
                   int timeout_ms,
                   bool use_tls,
                   mongoc_ssl_opt_t *ssl_opts,
                   mongoc_http_response_t *res,
                   bson_error_t *error)
{
   mongoc_stream_t *stream = NULL;
   mongoc_host_list_t host_list;
   bool ret = false;
   mongoc_iovec_t iovec;
   ssize_t bytes_read;
   char *path = NULL;
   bson_string_t *http_request = NULL;
   mongoc_buffer_t http_response_buf;
   char *http_response_str;
   char *ptr;
   const char *header_delimiter = "\r\n\r\n";

   memset (res, 0, sizeof (*res));
   _mongoc_buffer_init (&http_response_buf, NULL, 0, NULL, NULL);

   if (!_mongoc_host_list_from_hostport_with_err (
          &host_list, req->host, (uint16_t) req->port, error)) {
      goto fail;
   }

   stream = mongoc_client_connect_tcp (timeout_ms, &host_list, error);
   if (!stream) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Failed to connect to: %s",
                      req->host);
      goto fail;
   }

#ifndef MONGOC_ENABLE_SSL
   if (use_tls) {
      bson_set_error (
         error,
         MONGOC_ERROR_STREAM,
         MONGOC_ERROR_STREAM_SOCKET,
         "Failed to connect to %s: libmongoc not built with TLS support",
         req->host);
      goto fail;
   }
#else
   if (use_tls) {
      mongoc_stream_t *tls_stream;

      BSON_ASSERT (ssl_opts);
      tls_stream = mongoc_stream_tls_new_with_hostname (
         stream, req->host, ssl_opts, true);
      if (!tls_stream) {
         bson_set_error (error,
                         MONGOC_ERROR_STREAM,
                         MONGOC_ERROR_STREAM_SOCKET,
                         "Failed create TLS stream to: %s",
                         req->host);
         goto fail;
      }

      stream = tls_stream;
      if (!mongoc_stream_tls_handshake_block (
             stream, req->host, timeout_ms, error)) {
         goto fail;
      }
   }
#endif

   if (!req->path) {
      path = bson_strdup ("/");
   } else if (req->path[0] != '/') {
      path = bson_strdup_printf ("/%s", req->path);
   } else {
      path = bson_strdup (req->path);
   }

   http_request = bson_string_new ("");
   bson_string_append_printf (
      http_request, "%s %s HTTP/1.0\r\n", req->method, path);
   /* Always add Host header. */
   bson_string_append_printf (http_request, "Host: %s\r\n", req->host);
   /* Always add Connection: close header to ensure server closes connection. */
   bson_string_append_printf (http_request, "Connection: close\r\n");
   /* Add Content-Length if body included. */
   if (req->body_len) {
      bson_string_append_printf (
         http_request, "Content-Length: %d\r\n", req->body_len);
   }
   if (req->extra_headers) {
      bson_string_append (http_request, req->extra_headers);
   }
   bson_string_append (http_request, "\r\n");

   iovec.iov_base = http_request->str;
   iovec.iov_len = http_request->len;

   if (!_mongoc_stream_writev_full (stream, &iovec, 1, timeout_ms, error)) {
      goto fail;
   }

   if (req->body) {
      iovec.iov_base = (void *) req->body;
      iovec.iov_len = req->body_len;
      if (!_mongoc_stream_writev_full (stream, &iovec, 1, timeout_ms, error)) {
         goto fail;
      }
   }

   /* Read until connection close. */
   do {
      bytes_read = _mongoc_buffer_try_append_from_stream (
         &http_response_buf, stream, 512, timeout_ms);
   } while (bytes_read > 0 || mongoc_stream_should_retry (stream));

   if (bytes_read < 0 && mongoc_stream_timed_out (stream)) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Timeout reading from stream");
      goto fail;
   }

   if (http_response_buf.len == 0) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "No response received");
      goto fail;
   }

   http_response_str = (char *) http_response_buf.data;

   /* Find the end of the headers. */
   ptr = strstr (http_response_str, header_delimiter);
   if (NULL == ptr) {
      bson_set_error (
         error,
         MONGOC_ERROR_STREAM,
         MONGOC_ERROR_STREAM_SOCKET,
         "Error occurred reading response: end of headers not found");
      goto fail;
   }

   res->headers_len = ptr - http_response_str;
   res->headers = bson_strndup (http_response_str, res->headers_len);
   res->body_len =
      http_response_buf.len - res->headers_len - strlen (header_delimiter);
   /* Add a NULL character in case caller assumes NULL terminated. */
   res->body = bson_malloc0 (res->body_len + 1);
   memcpy (res->body, ptr + strlen (header_delimiter), res->body_len);
   ret = true;

fail:
   mongoc_stream_destroy (stream);
   if (http_request) {
      bson_string_free (http_request, true);
   }
   _mongoc_buffer_destroy (&http_response_buf);
   bson_free (path);
   return ret;
}
