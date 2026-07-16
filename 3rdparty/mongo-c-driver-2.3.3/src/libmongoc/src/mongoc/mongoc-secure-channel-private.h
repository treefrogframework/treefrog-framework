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

#include <mongoc/mongoc-prelude.h>

#ifndef MONGOC_SECURE_CHANNEL_PRIVATE_H
#define MONGOC_SECURE_CHANNEL_PRIVATE_H

#include <mongoc/mongoc-stream-tls-secure-channel-private.h>

#include <mongoc/mongoc-ssl.h>
#include <mongoc/mongoc-stream-tls.h>

#include <bson/bson.h>

#define SECURITY_WIN32
#include <schannel.h>
#include <schnlsp.h>
#include <security.h>

BSON_BEGIN_DECLS

bool
mongoc_secure_channel_setup_ca(const mongoc_ssl_opt_t *opt);

bool
mongoc_secure_channel_setup_crl(const mongoc_ssl_opt_t *opt);

// mongoc_secure_channel_load_crl is used in tests.
PCCRL_CONTEXT
mongoc_secure_channel_load_crl(const char *crl_file);

ssize_t
mongoc_secure_channel_read(mongoc_stream_tls_t *tls, void *data, size_t data_length);

ssize_t
mongoc_secure_channel_write(mongoc_stream_tls_t *tls, const void *data, size_t data_length);

PCCERT_CONTEXT
mongoc_secure_channel_setup_certificate(const mongoc_ssl_opt_t *opt);


/* it may require 16k + some overhead to hold one decryptable block of data - do
 * what cURL does, add 1k */
#define MONGOC_SCHANNEL_BUFFER_INIT_SIZE (17 * 1024)

void
_mongoc_secure_channel_init_sec_buffer(SecBuffer *buffer,
                                       unsigned long buf_type,
                                       void *buf_data_ptr,
                                       unsigned long buf_byte_size);

void
_mongoc_secure_channel_init_sec_buffer_desc(SecBufferDesc *desc, SecBuffer *buffer_array, unsigned long buffer_count);

void
mongoc_secure_channel_realloc_buf(size_t *size, uint8_t **buf, size_t new_size);

bool
mongoc_secure_channel_handshake_step_1(mongoc_stream_tls_t *tls, char *hostname, bson_error_t *error);
bool
mongoc_secure_channel_handshake_step_2(mongoc_stream_tls_t *tls, char *hostname, bson_error_t *error);
bool
mongoc_secure_channel_handshake_step_3(mongoc_stream_tls_t *tls, char *hostname, bson_error_t *error);

BSON_END_DECLS


#endif /* MONGOC_SECURE_CHANNEL_PRIVATE_H */
