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

#ifndef MONGOC_STREAM_TLS_SECURE_CHANNEL_PRIVATE_H
#define MONGOC_STREAM_TLS_SECURE_CHANNEL_PRIVATE_H

#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL
#include <mongoc/mongoc-shared-private.h>

#include <bson/bson.h>

/* Its mandatory to indicate to Windows who is compiling the code */
#define SECURITY_WIN32
#include <schannel.h>
#include <security.h>


BSON_BEGIN_DECLS


/* enum for the nonblocking SSL connection state machine */
typedef enum {
   ssl_connect_1,
   ssl_connect_2,
   ssl_connect_2_reading,
   ssl_connect_2_writing,
   ssl_connect_3,
   ssl_connect_done
} ssl_connect_state;

/* Structs to store Schannel handles */
typedef struct {
   CredHandle cred_handle;
   TimeStamp time_stamp;
} mongoc_secure_channel_cred_handle;

// `mongoc_secure_channel_cred` may be shared on multiple connections.
typedef struct _mongoc_secure_channel_cred {
   PCCERT_CONTEXT cert; /* Owning. Optional client cert. */
   SCHANNEL_CRED cred;  // TODO: switch to SCH_CREDENTIALS to support TLS v1.3
} mongoc_secure_channel_cred;

typedef struct {
   CtxtHandle ctxt_handle;
   TimeStamp time_stamp;
} mongoc_secure_channel_ctxt;

/**
 * mongoc_stream_tls_secure_channel_t:
 *
 * Private storage for Secure Channel Streams
 */
typedef struct {
   ssl_connect_state connecting_state;
   mongoc_shared_ptr cred_ptr; // Manages a mongoc_secure_channel_cred.
   mongoc_secure_channel_cred_handle *cred_handle;
   mongoc_secure_channel_ctxt *ctxt;
   SecPkgContext_StreamSizes stream_sizes;
   size_t encdata_length, decdata_length;
   size_t encdata_offset, decdata_offset;
   unsigned char *encdata_buffer, *decdata_buffer;
   unsigned long req_flags, ret_flags;
   int recv_unrecoverable_err;  /* _mongoc_stream_tls_secure_channel_read had an
                                   unrecoverable err */
   bool recv_sspi_close_notify; /* true if connection closed by close_notify */
   bool recv_connection_closed; /* true if connection closed, regardless how */
} mongoc_stream_tls_secure_channel_t;

struct _mongoc_ssl_opt_t; // Forward declare. Defined in mongoc-ssl.h.
struct _mongoc_stream_t;  // Forward declare. Defined in mongoc-stream.h.

mongoc_secure_channel_cred *
mongoc_secure_channel_cred_new (const struct _mongoc_ssl_opt_t *opt);

// mongoc_secure_channel_cred_deleter is useful as a deleter for mongoc_shared_t.
void
mongoc_secure_channel_cred_deleter (void *cred_void);

struct _mongoc_stream_t *
mongoc_stream_tls_secure_channel_new_with_creds (struct _mongoc_stream_t *base_stream,
                                                 const struct _mongoc_ssl_opt_t *opt,
                                                 mongoc_shared_ptr cred_ptr /* optional */);

BSON_END_DECLS

#endif /* MONGOC_ENABLE_SSL_SECURE_CHANNEL */
#endif /* MONGOC_STREAM_TLS_SECURE_CHANNEL_PRIVATE_H */
