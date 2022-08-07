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

#include "mongoc.h"
#include "mongoc-ssl.h"

#include "mongoc-prelude.h"

#ifndef MONGOC_HTTP_PRIVATE_H
#define MONGOC_HTTP_PRIVATE_H

typedef struct {
   const char *host;
   int port;
   const char *method;
   const char *path;
   const char *extra_headers;
   const char *body;
   int body_len;
} mongoc_http_request_t;

typedef struct {
   int status;
   char *headers;
   int headers_len;
   char *body;
   int body_len;
} mongoc_http_response_t;

void
_mongoc_http_request_init (mongoc_http_request_t *request);

void
_mongoc_http_response_init (mongoc_http_response_t *response);

void
_mongoc_http_response_cleanup (mongoc_http_response_t *response);

/*
 * Send an HTTP request and get a response.
 * On success, returns true.
 * On failure, returns false and sets error.
 * If use_tls is true, then ssl_opts must be set.
 * Caller must call _mongoc_http_response_cleanup on res.
 */
bool
_mongoc_http_send (mongoc_http_request_t *req,
                   int timeout_ms,
                   bool use_tls,
                   mongoc_ssl_opt_t *ssl_opts,
                   mongoc_http_response_t *res,
                   bson_error_t *error);

#endif /* MONGOC_HTTP_PRIVATE */
