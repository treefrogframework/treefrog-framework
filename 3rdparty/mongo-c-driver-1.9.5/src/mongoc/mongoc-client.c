/*
 * Copyright 2013 MongoDB, Inc.
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


#include <bson.h>
#include "mongoc-config.h"
#ifdef MONGOC_HAVE_DNSAPI
/* for DnsQuery_UTF8 */
#include <Windows.h>
#include <WinDNS.h>
#include <ws2tcpip.h>
#else
#if defined(MONGOC_HAVE_RES_NSEARCH) || defined(MONGOC_HAVE_RES_SEARCH)
#include <netdb.h>
#include <netinet/tcp.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <bson-string.h>

#endif
#endif

#include "mongoc-cursor-array-private.h"
#include "mongoc-client-private.h"
#include "mongoc-collection-private.h"
#include "mongoc-counters-private.h"
#include "mongoc-database-private.h"
#include "mongoc-gridfs-private.h"
#include "mongoc-error.h"
#include "mongoc-log.h"
#include "mongoc-queue-private.h"
#include "mongoc-socket.h"
#include "mongoc-stream-buffered.h"
#include "mongoc-stream-socket.h"
#include "mongoc-thread-private.h"
#include "mongoc-trace-private.h"
#include "mongoc-uri-private.h"
#include "mongoc-util-private.h"
#include "mongoc-set-private.h"
#include "mongoc-log.h"
#include "mongoc-write-concern-private.h"
#include "mongoc-read-concern-private.h"
#include "mongoc-host-list-private.h"
#include "mongoc-read-prefs-private.h"
#include "mongoc-client-session-private.h"

#ifdef MONGOC_ENABLE_SSL
#include "mongoc-stream-tls.h"
#include "mongoc-ssl-private.h"
#include "mongoc-cmd-private.h"

#endif


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "client"


static void
_mongoc_client_op_killcursors (mongoc_cluster_t *cluster,
                               mongoc_server_stream_t *server_stream,
                               int64_t cursor_id,
                               int64_t operation_id,
                               const char *db,
                               const char *collection);

static void
_mongoc_client_killcursors_command (mongoc_cluster_t *cluster,
                                    mongoc_server_stream_t *server_stream,
                                    int64_t cursor_id,
                                    const char *db,
                                    const char *collection,
                                    mongoc_client_session_t *cs);

#define DNS_ERROR(_msg, ...)                               \
   do {                                                    \
      bson_set_error (error,                               \
                      MONGOC_ERROR_STREAM,                 \
                      MONGOC_ERROR_STREAM_NAME_RESOLUTION, \
                      _msg,                                \
                      __VA_ARGS__);                        \
      GOTO (done);                                         \
   } while (0)


#ifdef MONGOC_HAVE_DNSAPI

typedef bool (*mongoc_rr_callback_t) (const char *service,
                                      PDNS_RECORD pdns,
                                      mongoc_uri_t *uri,
                                      bson_error_t *error);

static bool
srv_callback (const char *service,
              PDNS_RECORD pdns,
              mongoc_uri_t *uri,
              bson_error_t *error)
{
   return mongoc_uri_append_host (
      uri, pdns->Data.SRV.pNameTarget, pdns->Data.SRV.wPort, error);
}

static bool
txt_callback (const char *service,
              PDNS_RECORD pdns,
              mongoc_uri_t *uri,
              bson_error_t *error)
{
   DWORD i;
   bson_string_t *txt;
   bool r;

   txt = bson_string_new (NULL);

   for (i = 0; i < pdns->Data.TXT.dwStringCount; i++) {
      bson_string_append (txt, pdns->Data.TXT.pStringArray[i]);
   }

   r = mongoc_uri_parse_options (uri, txt->str, true /* from_dns */, error);
   bson_string_free (txt, true);

   return r;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_get_rr_dnsapi --
 *
 *       Fetch SRV or TXT resource records using the Windows DNS API and
 *       update @uri.
 *
 * Returns:
 *       Success or failure.
 *
 *       For an SRV lookup, returns false if there is any error.
 *
 *       For TXT lookup, ignores any error fetching the resource record, but
 *       returns false if the resource record is found and there is an error
 *       reading its contents as URI options.
 *
 * Side effects:
 *       @error is set if there is a failure.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_get_rr_dnsapi (const char *service,
                       mongoc_rr_type_t rr_type,
                       mongoc_uri_t *uri,
                       bson_error_t *error)
{
   const char *rr_type_name;
   WORD nst;
   mongoc_rr_callback_t callback;
   PDNS_RECORD pdns = NULL;
   DNS_STATUS res;
   LPVOID lpMsgBuf = NULL;
   bool dns_success;
   bool callback_success = true;
   int i;

   ENTRY;

   if (rr_type == MONGOC_RR_SRV) {
      /* return true only if DNS succeeds */
      dns_success = false;
      rr_type_name = "SRV";
      nst = DNS_TYPE_SRV;
      callback = srv_callback;
   } else {
      /* return true whether or not DNS succeeds */
      dns_success = true;
      rr_type_name = "TXT";
      nst = DNS_TYPE_TEXT;
      callback = txt_callback;
   }

   res = DnsQuery_UTF8 (service,
                        nst,
                        DNS_QUERY_BYPASS_CACHE,
                        NULL /* IP Address */,
                        &pdns,
                        0 /* reserved */);

   if (res) {
      DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

      if (FormatMessage (flags,
                         0,
                         res,
                         MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                         (LPTSTR) &lpMsgBuf,
                         0,
                         0)) {
         DNS_ERROR ("Failed to look up %s record \"%s\": %s",
                    rr_type_name,
                    service,
                    (char *) lpMsgBuf);
      }

      DNS_ERROR ("Failed to look up %s record \"%s\": Unknown error",
                 rr_type_name,
                 service);
   }

   if (!pdns) {
      DNS_ERROR ("No %s records for \"%s\"", rr_type_name, service);
   }

   dns_success = true;
   i = 0;

   do {
      if (i > 0 && rr_type == MONGOC_RR_TXT) {
         /* Initial DNS Seedlist Discovery Spec: a client "MUST raise an error
          * when multiple TXT records are encountered". */
         callback_success = false;
         DNS_ERROR ("Multiple TXT records for \"%s\"", service);
      }

      if (!callback (service, pdns, uri, error)) {
         callback_success = false;
         GOTO (done);
      }
      pdns = pdns->pNext;
      i++;
   } while (pdns);

done:
   if (pdns) {
      DnsRecordListFree (pdns, DnsFreeRecordList);
   }

   if (lpMsgBuf) {
      LocalFree (lpMsgBuf);
   }

   RETURN (dns_success && callback_success);
}

#elif (defined(MONGOC_HAVE_RES_NSEARCH) || defined(MONGOC_HAVE_RES_SEARCH))

typedef bool (*mongoc_rr_callback_t) (const char *service,
                                      ns_msg *ns_answer,
                                      ns_rr *rr,
                                      mongoc_uri_t *uri,
                                      bson_error_t *error);

static bool
srv_callback (const char *service,
              ns_msg *ns_answer,
              ns_rr *rr,
              mongoc_uri_t *uri,
              bson_error_t *error)
{
   const uint8_t *data;
   char name[1024];
   uint16_t port;
   int size;
   bool ret = false;

   data = ns_rr_rdata (*rr);
   port = ntohs (*(short *) (data + 4));
   size = dn_expand (ns_msg_base (*ns_answer),
                     ns_msg_end (*ns_answer),
                     data + 6,
                     name,
                     sizeof (name));

   if (size < 1) {
      DNS_ERROR ("Invalid record in SRV answer for \"%s\": \"%s\"",
                 service,
                 strerror (h_errno));
   }

   ret = mongoc_uri_append_host (uri, name, port, error);

done:
   return ret;
}

static bool
txt_callback (const char *service,
              ns_msg *ns_answer,
              ns_rr *rr,
              mongoc_uri_t *uri,
              bson_error_t *error)
{
   char s[256];
   const uint8_t *data;
   bson_string_t *txt;
   uint16_t pos, total;
   uint8_t len;
   bool r = false;

   total = (uint16_t) ns_rr_rdlen (*rr);
   if (total < 1 || total > 255) {
      DNS_ERROR ("Invalid TXT record size %hu for \"%s\"", total, service);
   }

   /* a TXT record has one or more strings, each up to 255 chars, each is
    * prefixed by its length as 1 byte. thus endianness doesn't matter. */
   txt = bson_string_new (NULL);
   pos = 0;
   data = ns_rr_rdata (*rr);

   while (pos < total) {
      memcpy (&len, data + pos, sizeof (uint8_t));
      pos++;
      bson_strncpy (s, (const char *) (data + pos), (size_t) len + 1);
      bson_string_append (txt, s);
      pos += len;
   }

   r = mongoc_uri_parse_options (uri, txt->str, true /* from_dns */, error);
   bson_string_free (txt, true);

done:
   return r;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_get_rr_search --
 *
 *       Fetch SRV or TXT resource records using libresolv and update @uri.
 *
 * Returns:
 *       Success or failure.
 *
 *       For an SRV lookup, returns false if there is any error.
 *
 *       For TXT lookup, ignores any error fetching the resource record, but
 *       returns false if the resource record is found and there is an error
 *       reading its contents as URI options.
 *
 * Side effects:
 *       @error is set if there is a failure.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_get_rr_search (const char *service,
                       mongoc_rr_type_t rr_type,
                       mongoc_uri_t *uri,
                       bson_error_t *error)
{
#ifdef MONGOC_HAVE_RES_NSEARCH
   struct __res_state state = {0};
#endif
   int size;
   unsigned char search_buf[1024];
   ns_msg ns_answer;
   int n;
   int i;
   const char *rr_type_name;
   ns_type nst;
   mongoc_rr_callback_t callback;
   ns_rr resource_record;
   bool dns_success;
   bool callback_success = true;

   ENTRY;

   if (rr_type == MONGOC_RR_SRV) {
      /* return true only if DNS succeeds */
      dns_success = false;
      rr_type_name = "SRV";
      nst = ns_t_srv;
      callback = srv_callback;
   } else {
      /* return true whether or not DNS succeeds */
      dns_success = true;
      rr_type_name = "TXT";
      nst = ns_t_txt;
      callback = txt_callback;
   }

#ifdef MONGOC_HAVE_RES_NSEARCH
   /* thread-safe */
   res_ninit (&state);
   size = res_nsearch (
      &state, service, ns_c_in, nst, search_buf, sizeof (search_buf));
#elif defined(MONGOC_HAVE_RES_SEARCH)
   size = res_search (service, ns_c_in, nst, search_buf, sizeof (search_buf));
#endif

   if (size < 0) {
      DNS_ERROR ("Failed to look up %s record \"%s\": %s",
                 rr_type_name,
                 service,
                 strerror (h_errno));
   }

   if (ns_initparse (search_buf, size, &ns_answer)) {
      DNS_ERROR ("Invalid %s answer for \"%s\"", rr_type_name, service);
   }

   n = ns_msg_count (ns_answer, ns_s_an);
   if (!n) {
      DNS_ERROR ("No %s records for \"%s\"", rr_type_name, service);
   }

   for (i = 0; i < n; i++) {
      if (i > 0 && rr_type == MONGOC_RR_TXT) {
         /* Initial DNS Seedlist Discovery Spec: a client "MUST raise an error
          * when multiple TXT records are encountered". */
         callback_success = false;
         DNS_ERROR ("Multiple TXT records for \"%s\"", service);
      }

      if (ns_parserr (&ns_answer, ns_s_an, i, &resource_record)) {
         DNS_ERROR ("Invalid record %d of %s answer for \"%s\": \"%s\"",
                    i,
                    rr_type_name,
                    service,
                    strerror (h_errno));
      }

      if (!callback (service, &ns_answer, &resource_record, uri, error)) {
         callback_success = false;
         GOTO (done);
      }
   }

   dns_success = true;

done:

#ifdef MONGOC_HAVE_RES_NDESTROY
   /* defined on BSD/Darwin, and only if MONGOC_HAVE_RES_NSEARCH is defined */
   res_ndestroy (&state);
#elif defined(MONGOC_HAVE_RES_NCLOSE)
   /* defined on Linux, and only if MONGOC_HAVE_RES_NSEARCH is defined */
   res_nclose (&state);
#endif
   RETURN (dns_success && callback_success);
}
#endif

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_client_get_rr --
 *
 *       Fetch an SRV or TXT resource record and update @uri. See RFCs 1464
 *       and 2782, and MongoDB's Initial DNS Seedlist Discovery Spec.
 *
 * Returns:
 *       Success or failure.
 *
 * Side effects:
 *       @error is set if there is a failure.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_client_get_rr (const char *service,
                       mongoc_rr_type_t rr_type,
                       mongoc_uri_t *uri,
                       bson_error_t *error)
{
#ifdef MONGOC_HAVE_DNSAPI
   return _mongoc_get_rr_dnsapi (service, rr_type, uri, error);
#elif (defined(MONGOC_HAVE_RES_NSEARCH) || defined(MONGOC_HAVE_RES_SEARCH))
   return _mongoc_get_rr_search (service, rr_type, uri, error);
#else
   bson_set_error (error,
                   MONGOC_ERROR_STREAM,
                   MONGOC_ERROR_STREAM_NAME_RESOLUTION,
                   "libresolv unavailable, cannot use mongodb+srv URI");
   return false;
#endif
}

#undef DNS_ERROR

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_connect_tcp --
 *
 *       Connect to a host using a TCP socket.
 *
 *       This will be performed synchronously and return a mongoc_stream_t
 *       that can be used to connect with the remote host.
 *
 * Returns:
 *       A newly allocated mongoc_stream_t if successful; otherwise
 *       NULL and @error is set.
 *
 * Side effects:
 *       @error is set if return value is NULL.
 *
 *--------------------------------------------------------------------------
 */

static mongoc_stream_t *
mongoc_client_connect_tcp (const mongoc_uri_t *uri,
                           const mongoc_host_list_t *host,
                           bson_error_t *error)
{
   mongoc_socket_t *sock = NULL;
   struct addrinfo hints;
   struct addrinfo *result, *rp;
   int32_t connecttimeoutms;
   int64_t expire_at;
   char portstr[8];
   int s;

   ENTRY;

   BSON_ASSERT (uri);
   BSON_ASSERT (host);

   connecttimeoutms = mongoc_uri_get_option_as_int32 (
      uri, MONGOC_URI_CONNECTTIMEOUTMS, MONGOC_DEFAULT_CONNECTTIMEOUTMS);

   BSON_ASSERT (connecttimeoutms);

   bson_snprintf (portstr, sizeof portstr, "%hu", host->port);

   memset (&hints, 0, sizeof hints);
   hints.ai_family = host->family;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = 0;
   hints.ai_protocol = 0;

   s = getaddrinfo (host->host, portstr, &hints, &result);

   if (s != 0) {
      mongoc_counter_dns_failure_inc ();
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_NAME_RESOLUTION,
                      "Failed to resolve %s",
                      host->host);
      RETURN (NULL);
   }

   mongoc_counter_dns_success_inc ();

   for (rp = result; rp; rp = rp->ai_next) {
      /*
       * Create a new non-blocking socket.
       */
      if (!(sock = mongoc_socket_new (
               rp->ai_family, rp->ai_socktype, rp->ai_protocol))) {
         continue;
      }

      /*
       * Try to connect to the peer.
       */
      expire_at = bson_get_monotonic_time () + (connecttimeoutms * 1000L);
      if (0 !=
          mongoc_socket_connect (
             sock, rp->ai_addr, (mongoc_socklen_t) rp->ai_addrlen, expire_at)) {
         char *errmsg;
         char errmsg_buf[BSON_ERROR_BUFFER_SIZE];
         char ip[255];

         mongoc_socket_inet_ntop (rp, ip, sizeof ip);
         errmsg = bson_strerror_r (
            mongoc_socket_errno (sock), errmsg_buf, sizeof errmsg_buf);
         MONGOC_WARNING ("Failed to connect to: %s:%d, error: %d, %s\n",
                         ip,
                         host->port,
                         mongoc_socket_errno (sock),
                         errmsg);
         mongoc_socket_destroy (sock);
         sock = NULL;
         continue;
      }

      break;
   }

   if (!sock) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_CONNECT,
                      "Failed to connect to target host: %s",
                      host->host_and_port);
      freeaddrinfo (result);
      RETURN (NULL);
   }

   freeaddrinfo (result);

   return mongoc_stream_socket_new (sock);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_connect_unix --
 *
 *       Connect to a MongoDB server using a UNIX domain socket.
 *
 * Returns:
 *       A newly allocated mongoc_stream_t if successful; otherwise
 *       NULL and @error is set.
 *
 * Side effects:
 *       @error is set if return value is NULL.
 *
 *--------------------------------------------------------------------------
 */

static mongoc_stream_t *
mongoc_client_connect_unix (const mongoc_uri_t *uri,
                            const mongoc_host_list_t *host,
                            bson_error_t *error)
{
#ifdef _WIN32
   ENTRY;
   bson_set_error (error,
                   MONGOC_ERROR_STREAM,
                   MONGOC_ERROR_STREAM_CONNECT,
                   "UNIX domain sockets not supported on win32.");
   RETURN (NULL);
#else
   struct sockaddr_un saddr;
   mongoc_socket_t *sock;
   mongoc_stream_t *ret = NULL;

   ENTRY;

   BSON_ASSERT (uri);
   BSON_ASSERT (host);

   memset (&saddr, 0, sizeof saddr);
   saddr.sun_family = AF_UNIX;
   bson_snprintf (saddr.sun_path, sizeof saddr.sun_path - 1, "%s", host->host);

   sock = mongoc_socket_new (AF_UNIX, SOCK_STREAM, 0);

   if (sock == NULL) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Failed to create socket.");
      RETURN (NULL);
   }

   if (-1 == mongoc_socket_connect (
                sock, (struct sockaddr *) &saddr, sizeof saddr, -1)) {
      mongoc_socket_destroy (sock);
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_CONNECT,
                      "Failed to connect to UNIX domain socket.");
      RETURN (NULL);
   }

   ret = mongoc_stream_socket_new (sock);

   RETURN (ret);
#endif
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_default_stream_initiator --
 *
 *       A mongoc_stream_initiator_t that will handle the various type
 *       of supported sockets by MongoDB including TCP and UNIX.
 *
 *       Language binding authors may want to implement an alternate
 *       version of this method to use their native stream format.
 *
 * Returns:
 *       A mongoc_stream_t if successful; otherwise NULL and @error is set.
 *
 * Side effects:
 *       @error is set if return value is NULL.
 *
 *--------------------------------------------------------------------------
 */

mongoc_stream_t *
mongoc_client_default_stream_initiator (const mongoc_uri_t *uri,
                                        const mongoc_host_list_t *host,
                                        void *user_data,
                                        bson_error_t *error)
{
   mongoc_stream_t *base_stream = NULL;
#ifdef MONGOC_ENABLE_SSL
   mongoc_client_t *client = (mongoc_client_t *) user_data;
   const char *mechanism;
   int32_t connecttimeoutms;
#endif

   BSON_ASSERT (uri);
   BSON_ASSERT (host);

#ifndef MONGOC_ENABLE_SSL
   if (mongoc_uri_get_ssl (uri)) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_NO_ACCEPTABLE_PEER,
                      "SSL is not enabled in this build of mongo-c-driver.");
      return NULL;
   }
#endif


   switch (host->family) {
   case AF_UNSPEC:
#if defined(AF_INET6)
   case AF_INET6:
#endif
   case AF_INET:
      base_stream = mongoc_client_connect_tcp (uri, host, error);
      break;
   case AF_UNIX:
      base_stream = mongoc_client_connect_unix (uri, host, error);
      break;
   default:
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_INVALID_TYPE,
                      "Invalid address family: 0x%02x",
                      host->family);
      break;
   }

#ifdef MONGOC_ENABLE_SSL
   if (base_stream) {
      mechanism = mongoc_uri_get_auth_mechanism (uri);

      if (client->use_ssl ||
          (mechanism && (0 == strcmp (mechanism, "MONGODB-X509")))) {
         mongoc_stream_t *original = base_stream;

         base_stream = mongoc_stream_tls_new_with_hostname (
            base_stream, host->host, &client->ssl_opts, true);

         if (!base_stream) {
            mongoc_stream_destroy (original);
            bson_set_error (error,
                            MONGOC_ERROR_STREAM,
                            MONGOC_ERROR_STREAM_SOCKET,
                            "Failed initialize TLS state.");
            return NULL;
         }

         connecttimeoutms = mongoc_uri_get_option_as_int32 (
            uri, MONGOC_URI_CONNECTTIMEOUTMS, MONGOC_DEFAULT_CONNECTTIMEOUTMS);

         if (!mongoc_stream_tls_handshake_block (
                base_stream, host->host, connecttimeoutms, error)) {
            mongoc_stream_destroy (base_stream);
            return NULL;
         }
      }
   }
#endif

   return base_stream ? mongoc_stream_buffered_new (base_stream, 1024) : NULL;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_client_create_stream --
 *
 *       INTERNAL API
 *
 *       This function is used by the mongoc_cluster_t to initiate a
 *       new stream. This is done because cluster is private API and
 *       those using mongoc_client_t may need to override this process.
 *
 *       This function calls the default initiator for new streams.
 *
 * Returns:
 *       A newly allocated mongoc_stream_t if successful; otherwise
 *       NULL and @error is set.
 *
 * Side effects:
 *       @error is set if return value is NULL.
 *
 *--------------------------------------------------------------------------
 */

mongoc_stream_t *
_mongoc_client_create_stream (mongoc_client_t *client,
                              const mongoc_host_list_t *host,
                              bson_error_t *error)
{
   BSON_ASSERT (client);
   BSON_ASSERT (host);

   return client->initiator (client->uri, host, client->initiator_data, error);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_client_recv --
 *
 *       Receives a RPC from a remote MongoDB cluster node.
 *
 * Returns:
 *       true if successful; otherwise false and @error is set.
 *
 * Side effects:
 *       @error is set if return value is false.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_client_recv (mongoc_client_t *client,
                     mongoc_rpc_t *rpc,
                     mongoc_buffer_t *buffer,
                     mongoc_server_stream_t *server_stream,
                     bson_error_t *error)
{
   BSON_ASSERT (client);
   BSON_ASSERT (rpc);
   BSON_ASSERT (buffer);
   BSON_ASSERT (server_stream);

   if (!mongoc_cluster_try_recv (
          &client->cluster, rpc, buffer, server_stream, error)) {
      mongoc_topology_invalidate_server (
         client->topology, server_stream->sd->id, error);
      return false;
   }
   return true;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_new --
 *
 *       Create a new mongoc_client_t using the URI provided.
 *
 *       @uri should be a MongoDB URI string such as "mongodb://localhost/"
 *       More information on the format can be found at
 *       http://docs.mongodb.org/manual/reference/connection-string/
 *
 * Returns:
 *       A newly allocated mongoc_client_t or NULL if @uri_string is
 *       invalid.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */
mongoc_client_t *
mongoc_client_new (const char *uri_string)
{
   mongoc_topology_t *topology;
   mongoc_client_t *client;
   mongoc_uri_t *uri;


   if (!uri_string) {
      uri_string = "mongodb://127.0.0.1/";
   }

   if (!(uri = mongoc_uri_new (uri_string))) {
      return NULL;
   }

   topology = mongoc_topology_new (uri, true);

   client = _mongoc_client_new_from_uri (topology);
   if (!client) {
      mongoc_topology_destroy (topology);
   }
   mongoc_uri_destroy (uri);

   return client;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_set_ssl_opts
 *
 *       set ssl opts for a client
 *
 * Returns:
 *       Nothing
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

#ifdef MONGOC_ENABLE_SSL
void
mongoc_client_set_ssl_opts (mongoc_client_t *client,
                            const mongoc_ssl_opt_t *opts)
{
   BSON_ASSERT (client);
   BSON_ASSERT (opts);

   _mongoc_ssl_opts_cleanup (&client->ssl_opts);

   client->use_ssl = true;
   _mongoc_ssl_opts_copy_to (opts, &client->ssl_opts);

   if (client->topology->single_threaded) {
      mongoc_topology_scanner_set_ssl_opts (client->topology->scanner,
                                            &client->ssl_opts);
   }
}
#endif


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_new_from_uri --
 *
 *       Create a new mongoc_client_t for a mongoc_uri_t.
 *
 * Returns:
 *       A newly allocated mongoc_client_t.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_client_t *
mongoc_client_new_from_uri (const mongoc_uri_t *uri)
{
   mongoc_topology_t *topology;

   topology = mongoc_topology_new (uri, true);

   /* topology->uri may be different from uri: if this is a mongodb+srv:// URI
    * then mongoc_topology_new has fetched SRV and TXT records and updated its
    * uri from them.
    */
   return _mongoc_client_new_from_uri (topology);
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_client_new_from_uri --
 *
 *       Create a new mongoc_client_t for a given topology object.
 *
 * Returns:
 *       A newly allocated mongoc_client_t.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_client_t *
_mongoc_client_new_from_uri (mongoc_topology_t *topology)
{
   mongoc_client_t *client;
   const mongoc_read_prefs_t *read_prefs;
   const mongoc_read_concern_t *read_concern;
   const mongoc_write_concern_t *write_concern;
   const char *appname;

   BSON_ASSERT (topology);

#ifndef MONGOC_ENABLE_SSL
   if (mongoc_uri_get_ssl (topology->uri)) {
      MONGOC_ERROR ("Can't create SSL client, SSL not enabled in this build.");
      return NULL;
   }
#endif

   client = (mongoc_client_t *) bson_malloc0 (sizeof *client);
   client->uri = mongoc_uri_copy (topology->uri);
   client->initiator = mongoc_client_default_stream_initiator;
   client->initiator_data = client;
   client->topology = topology;
   client->error_api_version = MONGOC_ERROR_API_VERSION_LEGACY;
   client->error_api_set = false;
   client->client_sessions = mongoc_set_new (8, NULL, NULL);
   client->csid_rand_seed = (unsigned int) bson_get_monotonic_time ();

   write_concern = mongoc_uri_get_write_concern (client->uri);
   client->write_concern = mongoc_write_concern_copy (write_concern);

   read_concern = mongoc_uri_get_read_concern (client->uri);
   client->read_concern = mongoc_read_concern_copy (read_concern);

   read_prefs = mongoc_uri_get_read_prefs_t (client->uri);
   client->read_prefs = mongoc_read_prefs_copy (read_prefs);

   appname =
      mongoc_uri_get_option_as_utf8 (client->uri, MONGOC_URI_APPNAME, NULL);
   if (appname && client->topology->single_threaded) {
      /* the appname should have already been validated */
      BSON_ASSERT (mongoc_client_set_appname (client, appname));
   }

   mongoc_cluster_init (&client->cluster, client->uri, client);

#ifdef MONGOC_ENABLE_SSL
   client->use_ssl = false;
   if (mongoc_uri_get_ssl (client->uri)) {
      mongoc_ssl_opt_t ssl_opt = {0};

      _mongoc_ssl_opts_from_uri (&ssl_opt, client->uri);
      /* sets use_ssl = true */
      mongoc_client_set_ssl_opts (client, &ssl_opt);
   }
#endif

   mongoc_counter_clients_active_inc ();

   return client;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_destroy --
 *
 *       Destroys a mongoc_client_t and cleans up all resources associated
 *       with the client instance.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @client is destroyed.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_client_destroy (mongoc_client_t *client)
{
   if (client) {
      if (client->topology->single_threaded) {
         _mongoc_client_end_sessions (client);
         mongoc_topology_destroy (client->topology);
      }

      mongoc_write_concern_destroy (client->write_concern);
      mongoc_read_concern_destroy (client->read_concern);
      mongoc_read_prefs_destroy (client->read_prefs);
      mongoc_cluster_destroy (&client->cluster);
      mongoc_uri_destroy (client->uri);
      mongoc_set_destroy (client->client_sessions);

#ifdef MONGOC_ENABLE_SSL
      _mongoc_ssl_opts_cleanup (&client->ssl_opts);
#endif

      bson_free (client);

      mongoc_counter_clients_active_dec ();
      mongoc_counter_clients_disposed_inc ();
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_get_uri --
 *
 *       Fetch the URI used for @client.
 *
 * Returns:
 *       A mongoc_uri_t that should not be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

const mongoc_uri_t *
mongoc_client_get_uri (const mongoc_client_t *client)
{
   BSON_ASSERT (client);

   return client->uri;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_start_session --
 *
 *       Creates a structure to communicate in a session over @client.
 *
 *       This structure should be freed when the caller is done with it
 *       using mongoc_client_session_destroy().
 *
 * Returns:
 *       A newly allocated mongoc_client_session_t.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_client_session_t *
mongoc_client_start_session (mongoc_client_t *client,
                             const mongoc_session_opt_t *opts,
                             bson_error_t *error)
{
   mongoc_server_session_t *ss;
   mongoc_client_session_t *cs;
   uint32_t csid;

   ENTRY;

   ss = _mongoc_client_pop_server_session (client, error);
   if (!ss) {
      RETURN (NULL);
   }

   /* get a random internal id for the session, retrying on collision */
   do {
      csid = (uint32_t) _mongoc_rand_simple (&client->csid_rand_seed);
   } while (mongoc_set_get (client->client_sessions, csid));

   cs = _mongoc_client_session_new (client, ss, opts, csid);

   /* remember session so if we see its client_session_id in a command, we can
    * find its lsid and clusterTime */
   mongoc_set_add (client->client_sessions, csid, cs);

   RETURN (cs);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_get_database --
 *
 *       Fetches a newly allocated database structure to communicate with
 *       a database over @client.
 *
 *       @database should be a db name such as "test".
 *
 *       This structure should be freed when the caller is done with it
 *       using mongoc_database_destroy().
 *
 * Returns:
 *       A newly allocated mongoc_database_t.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_database_t *
mongoc_client_get_database (mongoc_client_t *client, const char *name)
{
   BSON_ASSERT (client);
   BSON_ASSERT (name);

   return _mongoc_database_new (client,
                                name,
                                client->read_prefs,
                                client->read_concern,
                                client->write_concern);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_get_default_database --
 *
 *       Get the database named in the MongoDB connection URI, or NULL
 *       if none was specified in the URI.
 *
 *       This structure should be freed when the caller is done with it
 *       using mongoc_database_destroy().
 *
 * Returns:
 *       A newly allocated mongoc_database_t or NULL.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_database_t *
mongoc_client_get_default_database (mongoc_client_t *client)
{
   const char *db;

   BSON_ASSERT (client);
   db = mongoc_uri_get_database (client->uri);

   if (db) {
      return mongoc_client_get_database (client, db);
   }

   return NULL;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_get_collection --
 *
 *       This function returns a newly allocated collection structure.
 *
 *       @db should be the name of the database, such as "test".
 *       @collection should be the name of the collection such as "test".
 *
 *       The above would result in the namespace "test.test".
 *
 *       You should free this structure when you are done with it using
 *       mongoc_collection_destroy().
 *
 * Returns:
 *       A newly allocated mongoc_collection_t that should be freed with
 *       mongoc_collection_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_collection_t *
mongoc_client_get_collection (mongoc_client_t *client,
                              const char *db,
                              const char *collection)
{
   BSON_ASSERT (client);
   BSON_ASSERT (db);
   BSON_ASSERT (collection);

   return _mongoc_collection_new (client,
                                  db,
                                  collection,
                                  client->read_prefs,
                                  client->read_concern,
                                  client->write_concern);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_get_gridfs --
 *
 *       This function returns a newly allocated collection structure.
 *
 *       @db should be the name of the database, such as "test".
 *
 *       @prefix optional prefix for GridFS collection names, or NULL. Default
 *       is "fs", thus the default collection names for GridFS are "fs.files"
 *       and "fs.chunks".
 *
 * Returns:
 *       A newly allocated mongoc_gridfs_t that should be freed with
 *       mongoc_gridfs_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_gridfs_t *
mongoc_client_get_gridfs (mongoc_client_t *client,
                          const char *db,
                          const char *prefix,
                          bson_error_t *error)
{
   BSON_ASSERT (client);
   BSON_ASSERT (db);

   if (!prefix) {
      prefix = "fs";
   }

   return _mongoc_gridfs_new (client, db, prefix, error);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_get_write_concern --
 *
 *       Fetches the default write concern for @client.
 *
 * Returns:
 *       A mongoc_write_concern_t that should not be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

const mongoc_write_concern_t *
mongoc_client_get_write_concern (const mongoc_client_t *client)
{
   BSON_ASSERT (client);

   return client->write_concern;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_set_write_concern --
 *
 *       Sets the default write concern for @client.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_client_set_write_concern (mongoc_client_t *client,
                                 const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT (client);

   if (write_concern != client->write_concern) {
      if (client->write_concern) {
         mongoc_write_concern_destroy (client->write_concern);
      }
      client->write_concern = write_concern
                                 ? mongoc_write_concern_copy (write_concern)
                                 : mongoc_write_concern_new ();
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_get_read_concern --
 *
 *       Fetches the default read concern for @client.
 *
 * Returns:
 *       A mongoc_read_concern_t that should not be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

const mongoc_read_concern_t *
mongoc_client_get_read_concern (const mongoc_client_t *client)
{
   BSON_ASSERT (client);

   return client->read_concern;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_set_read_concern --
 *
 *       Sets the default read concern for @client.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_client_set_read_concern (mongoc_client_t *client,
                                const mongoc_read_concern_t *read_concern)
{
   BSON_ASSERT (client);

   if (read_concern != client->read_concern) {
      if (client->read_concern) {
         mongoc_read_concern_destroy (client->read_concern);
      }
      client->read_concern = read_concern
                                ? mongoc_read_concern_copy (read_concern)
                                : mongoc_read_concern_new ();
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_get_read_prefs --
 *
 *       Fetch the default read preferences for @client.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

const mongoc_read_prefs_t *
mongoc_client_get_read_prefs (const mongoc_client_t *client)
{
   BSON_ASSERT (client);

   return client->read_prefs;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_set_read_prefs --
 *
 *       Set the default read preferences for @client.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_client_set_read_prefs (mongoc_client_t *client,
                              const mongoc_read_prefs_t *read_prefs)
{
   BSON_ASSERT (client);

   if (read_prefs != client->read_prefs) {
      if (client->read_prefs) {
         mongoc_read_prefs_destroy (client->read_prefs);
      }
      client->read_prefs = read_prefs
                              ? mongoc_read_prefs_copy (read_prefs)
                              : mongoc_read_prefs_new (MONGOC_READ_PRIMARY);
   }
}

mongoc_cursor_t *
mongoc_client_command (mongoc_client_t *client,
                       const char *db_name,
                       mongoc_query_flags_t flags,
                       uint32_t skip,
                       uint32_t limit,
                       uint32_t batch_size,
                       const bson_t *query,
                       const bson_t *fields,
                       const mongoc_read_prefs_t *read_prefs)
{
   char ns[MONGOC_NAMESPACE_MAX];
   mongoc_cursor_t *cursor;

   BSON_ASSERT (client);
   BSON_ASSERT (db_name);
   BSON_ASSERT (query);

   /*
    * Allow a caller to provide a fully qualified namespace
    */
   if (NULL == strstr (db_name, "$cmd")) {
      bson_snprintf (ns, sizeof ns, "%s.$cmd", db_name);
      db_name = ns;
   }

   /* flags, skip, limit, batch_size, fields are unused */
   cursor = _mongoc_cursor_new_with_opts (
      client, db_name, false /* is_find */, query, NULL, read_prefs, NULL);

   return cursor;
}


static bool
_mongoc_client_retryable_write_command_with_stream (
   mongoc_client_t *client,
   mongoc_cmd_parts_t *parts,
   mongoc_server_stream_t *server_stream,
   bson_t *reply,
   bson_error_t *error)
{
   mongoc_server_stream_t *retry_server_stream = NULL;
   bson_iter_t txn_number_iter;
   bool is_retryable = true;
   bool ret;

   ENTRY;

   BSON_ASSERT (parts->is_retryable_write);

   /* increment the transaction number for the first attempt of each retryable
    * write command */
   BSON_ASSERT (bson_iter_init_find (
      &txn_number_iter, parts->assembled.command, "txnNumber"));
   bson_iter_overwrite_int64 (
      &txn_number_iter, ++parts->assembled.session->server_session->txn_number);

retry:
   ret = mongoc_cluster_run_command_monitored (
      &client->cluster, &parts->assembled, reply, error);

   /* If a retryable error is encountered and the write is retryable, select
    * a new writable stream and retry. If server selection fails or the selected
    * server does not support retryable writes, fall through and allow the
    * original error to be reported. */
   if (!ret && is_retryable &&
       (error->domain == MONGOC_ERROR_STREAM ||
        mongoc_cluster_is_not_master_error (error))) {
      bson_error_t ignored_error;

      /* each write command may be retried at most once */
      is_retryable = false;

      if (retry_server_stream) {
         mongoc_server_stream_cleanup (retry_server_stream);
      }

      retry_server_stream =
         mongoc_cluster_stream_for_writes (&client->cluster, &ignored_error);

      if (retry_server_stream && retry_server_stream->sd->max_wire_version >=
                                    WIRE_VERSION_RETRY_WRITES) {
         parts->assembled.server_stream = retry_server_stream;
         GOTO (retry);
      }
   }

   if (retry_server_stream) {
      mongoc_server_stream_cleanup (retry_server_stream);
   }

   RETURN (ret);
}


static bool
_mongoc_client_command_with_stream (mongoc_client_t *client,
                                    mongoc_cmd_parts_t *parts,
                                    mongoc_server_stream_t *server_stream,
                                    bson_t *reply,
                                    bson_error_t *error)
{
   ENTRY;

   parts->assembled.operation_id = ++client->cluster.operation_id;
   if (!mongoc_cmd_parts_assemble (parts, server_stream, error)) {
      _mongoc_bson_init_if_set (reply);
      return false;
   };

   if (parts->is_retryable_write) {
      RETURN (_mongoc_client_retryable_write_command_with_stream (
         client, parts, server_stream, reply, error));
   }

   RETURN (mongoc_cluster_run_command_monitored (
      &client->cluster, &parts->assembled, reply, error));
}


bool
mongoc_client_command_simple (mongoc_client_t *client,
                              const char *db_name,
                              const bson_t *command,
                              const mongoc_read_prefs_t *read_prefs,
                              bson_t *reply,
                              bson_error_t *error)
{
   mongoc_cluster_t *cluster;
   mongoc_server_stream_t *server_stream = NULL;
   mongoc_cmd_parts_t parts;
   bool ret;

   ENTRY;

   BSON_ASSERT (client);
   BSON_ASSERT (db_name);
   BSON_ASSERT (command);

   if (!_mongoc_read_prefs_validate (read_prefs, error)) {
      RETURN (false);
   }

   cluster = &client->cluster;
   mongoc_cmd_parts_init (&parts, client, db_name, MONGOC_QUERY_NONE, command);
   parts.read_prefs = read_prefs;

   /* Server Selection Spec: "The generic command method has a default read
    * preference of mode 'primary'. The generic command method MUST ignore any
    * default read preference from client, database or collection
    * configuration. The generic command method SHOULD allow an optional read
    * preference argument."
    */
   server_stream = mongoc_cluster_stream_for_reads (cluster, read_prefs, error);

   if (server_stream) {
      ret = _mongoc_client_command_with_stream (
         client, &parts, server_stream, reply, error);
   } else {
      if (reply) {
         bson_init (reply);
      }

      ret = false;
   }

   mongoc_cmd_parts_cleanup (&parts);
   mongoc_server_stream_cleanup (server_stream);

   RETURN (ret);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_client_command_with_opts --
 *
 *       Execute a command on the server. If mode is MONGOC_CMD_READ or
 *       MONGOC_CMD_RW, then read concern is applied from @opts, or else from
 *       @default_rc, and read preferences are applied from @default_prefs.
 *       If mode is MONGOC_CMD_WRITE or MONGOC_CMD_RW, then write concern is
 *       applied from @opts if present, or else from @default_wc.
 *
 *       If mode is MONGOC_CMD_RAW, then read concern and write concern are
 *       applied from @opts only. Read preferences are applied from
 *       @read_prefs.
 *
 *       The mongoc_client_t's read preference, read concern, and write concern
 *       are *NOT* applied.
 *
 * Returns:
 *       Success or failure.
 *       A write concern timeout or write concern error is considered a failure.
 *
 * Side effects:
 *       @reply is always initialized.
 *       @error is filled out if the command fails.
 *
 *--------------------------------------------------------------------------
 */
bool
_mongoc_client_command_with_opts (mongoc_client_t *client,
                                  const char *db_name,
                                  const bson_t *command,
                                  mongoc_command_mode_t mode,
                                  const bson_t *opts,
                                  mongoc_query_flags_t flags,
                                  const mongoc_read_prefs_t *default_prefs,
                                  mongoc_read_concern_t *default_rc,
                                  mongoc_write_concern_t *default_wc,
                                  bson_t *reply,
                                  bson_error_t *error)
{
   mongoc_cmd_parts_t parts;
   const char *command_name;
   mongoc_server_stream_t *server_stream = NULL;
   mongoc_cluster_t *cluster;
   bson_t reply_local;
   bson_t *reply_ptr;
   uint32_t server_id;
   bool ret = false;

   ENTRY;

   BSON_ASSERT (client);
   BSON_ASSERT (db_name);
   BSON_ASSERT (command);

   mongoc_cmd_parts_init (&parts, client, db_name, flags, command);
   parts.is_read_command = (mode & MONGOC_CMD_READ);
   parts.is_write_command = (mode & MONGOC_CMD_WRITE);

   command_name = _mongoc_get_command_name (command);

   if (!command_name) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "Empty command document");
      GOTO (err);
   }

   reply_ptr = reply ? reply : &reply_local;

   if (mode == MONGOC_CMD_READ || mode == MONGOC_CMD_RAW) {
      /* NULL read pref is ok */
      if (!_mongoc_read_prefs_validate (default_prefs, error)) {
         GOTO (err);
      }

      parts.read_prefs = default_prefs;
   } else {
      /* this is a command that writes */
      default_prefs = NULL;
   }

   cluster = &client->cluster;
   if (!_mongoc_get_server_id_from_opts (opts,
                                         MONGOC_ERROR_COMMAND,
                                         MONGOC_ERROR_COMMAND_INVALID_ARG,
                                         &server_id,
                                         error)) {
      GOTO (err);
   }

   if (server_id) {
      /* "serverId" passed in opts */
      server_stream = mongoc_cluster_stream_for_server (
         cluster, server_id, true /* reconnect ok */, error);

      if (server_stream && server_stream->sd->type != MONGOC_SERVER_MONGOS) {
         parts.user_query_flags |= MONGOC_QUERY_SLAVE_OK;
      }
   } else if (parts.is_write_command) {
      server_stream = mongoc_cluster_stream_for_writes (cluster, error);
   } else {
      server_stream =
         mongoc_cluster_stream_for_reads (cluster, default_prefs, error);
   }

   if (server_stream) {
      int32_t wire_version = server_stream->sd->max_wire_version;
      bson_iter_t iter;

      if (opts && bson_iter_init (&iter, opts)) {
         if (!mongoc_cmd_parts_append_opts (
                &parts, &iter, wire_version, error)) {
            GOTO (err);
         }
      }

      /* use default write concern unless it's in opts */
      if ((mode & MONGOC_CMD_WRITE) &&
          !mongoc_write_concern_is_default (default_wc) &&
          (!opts || !bson_has_field (opts, "writeConcern"))) {
         bool is_fam = !strcasecmp (command_name, "findandmodify");

         if ((is_fam && wire_version >= WIRE_VERSION_FAM_WRITE_CONCERN) ||
             (!is_fam && wire_version >= WIRE_VERSION_CMD_WRITE_CONCERN)) {
            bson_append_document (&parts.extra,
                                  "writeConcern",
                                  12,
                                  _mongoc_write_concern_get_bson (default_wc));
         }
      }

      /* use read prefs and read concern for read commands, unless in opts */
      if ((mode & MONGOC_CMD_READ) &&
          wire_version >= WIRE_VERSION_READ_CONCERN &&
          !mongoc_read_concern_is_default (default_rc) &&
          (!opts || !bson_has_field (opts, "readConcern"))) {
         bson_append_document (&parts.extra,
                               "readConcern",
                               11,
                               _mongoc_read_concern_get_bson (default_rc));
      }

      ret = _mongoc_client_command_with_stream (
         client, &parts, server_stream, reply_ptr, error);

      if (ret && (mode & MONGOC_CMD_WRITE)) {
         ret = !_mongoc_parse_wc_err (reply_ptr, error);
      }
      if (reply_ptr == &reply_local) {
         bson_destroy (reply_ptr);
      }
      GOTO (done);
   }

err:
   if (reply) {
      bson_init (reply);
   }

done:

   if (server_stream) {
      mongoc_server_stream_cleanup (server_stream);
   }

   mongoc_cmd_parts_cleanup (&parts);

   RETURN (ret);
}


bool
mongoc_client_read_command_with_opts (mongoc_client_t *client,
                                      const char *db_name,
                                      const bson_t *command,
                                      const mongoc_read_prefs_t *read_prefs,
                                      const bson_t *opts,
                                      bson_t *reply,
                                      bson_error_t *error)
{
   return _mongoc_client_command_with_opts (
      client,
      db_name,
      command,
      MONGOC_CMD_READ,
      opts,
      MONGOC_QUERY_NONE,
      COALESCE (read_prefs, client->read_prefs),
      client->read_concern,
      client->write_concern,
      reply,
      error);
}


bool
mongoc_client_write_command_with_opts (mongoc_client_t *client,
                                       const char *db_name,
                                       const bson_t *command,
                                       const bson_t *opts,
                                       bson_t *reply,
                                       bson_error_t *error)
{
   return _mongoc_client_command_with_opts (client,
                                            db_name,
                                            command,
                                            MONGOC_CMD_WRITE,
                                            opts,
                                            MONGOC_QUERY_NONE,
                                            client->read_prefs,
                                            client->read_concern,
                                            client->write_concern,
                                            reply,
                                            error);
}


bool
mongoc_client_read_write_command_with_opts (
   mongoc_client_t *client,
   const char *db_name,
   const bson_t *command,
   const mongoc_read_prefs_t *read_prefs /* IGNORED */,
   const bson_t *opts,
   bson_t *reply,
   bson_error_t *error)
{
   return _mongoc_client_command_with_opts (
      client,
      db_name,
      command,
      MONGOC_CMD_RW,
      opts,
      MONGOC_QUERY_NONE,
      COALESCE (read_prefs, client->read_prefs),
      client->read_concern,
      client->write_concern,
      reply,
      error);
}


bool
mongoc_client_command_with_opts (mongoc_client_t *client,
                                 const char *db_name,
                                 const bson_t *command,
                                 const mongoc_read_prefs_t *read_prefs,
                                 const bson_t *opts,
                                 bson_t *reply,
                                 bson_error_t *error)
{
   return _mongoc_client_command_with_opts (client,
                                            db_name,
                                            command,
                                            MONGOC_CMD_RAW,
                                            opts,
                                            MONGOC_QUERY_NONE,
                                            read_prefs,
                                            client->read_concern,
                                            client->write_concern,
                                            reply,
                                            error);
}


bool
mongoc_client_command_simple_with_server_id (
   mongoc_client_t *client,
   const char *db_name,
   const bson_t *command,
   const mongoc_read_prefs_t *read_prefs,
   uint32_t server_id,
   bson_t *reply,
   bson_error_t *error)
{
   mongoc_server_stream_t *server_stream;
   mongoc_cmd_parts_t parts;
   bool ret;

   ENTRY;

   BSON_ASSERT (client);
   BSON_ASSERT (db_name);
   BSON_ASSERT (command);

   if (!_mongoc_read_prefs_validate (read_prefs, error)) {
      RETURN (false);
   }

   server_stream = mongoc_cluster_stream_for_server (
      &client->cluster, server_id, true /* reconnect ok */, error);

   if (server_stream) {
      mongoc_cmd_parts_init (
         &parts, client, db_name, MONGOC_QUERY_NONE, command);
      parts.read_prefs = read_prefs;

      ret = _mongoc_client_command_with_stream (
         client, &parts, server_stream, reply, error);

      mongoc_cmd_parts_cleanup (&parts);
      mongoc_server_stream_cleanup (server_stream);
      RETURN (ret);
   } else {
      if (reply) {
         bson_init (reply);
      }

      RETURN (false);
   }
}


static void
_mongoc_client_prepare_killcursors_command (int64_t cursor_id,
                                            const char *collection,
                                            bson_t *command)
{
   bson_t child;

   bson_append_utf8 (command, "killCursors", 11, collection, -1);
   bson_append_array_begin (command, "cursors", 7, &child);
   bson_append_int64 (&child, "0", 1, cursor_id);
   bson_append_array_end (command, &child);
}


void
_mongoc_client_kill_cursor (mongoc_client_t *client,
                            uint32_t server_id,
                            int64_t cursor_id,
                            int64_t operation_id,
                            const char *db,
                            const char *collection,
                            mongoc_client_session_t *cs)
{
   mongoc_server_stream_t *server_stream;

   ENTRY;

   BSON_ASSERT (client);
   BSON_ASSERT (cursor_id);

   /* don't attempt reconnect if server unavailable, and ignore errors */
   server_stream = mongoc_cluster_stream_for_server (
      &client->cluster, server_id, false /* reconnect_ok */, NULL /* error */);

   if (!server_stream) {
      return;
   }

   if (db && collection &&
       server_stream->sd->max_wire_version >= WIRE_VERSION_KILLCURSORS_CMD) {
      _mongoc_client_killcursors_command (
         &client->cluster, server_stream, cursor_id, db, collection, cs);
   } else {
      _mongoc_client_op_killcursors (&client->cluster,
                                     server_stream,
                                     cursor_id,
                                     operation_id,
                                     db,
                                     collection);
   }

   mongoc_server_stream_cleanup (server_stream);

   EXIT;
}


static void
_mongoc_client_monitor_op_killcursors (mongoc_cluster_t *cluster,
                                       mongoc_server_stream_t *server_stream,
                                       int64_t cursor_id,
                                       int64_t operation_id,
                                       const char *db,
                                       const char *collection)
{
   bson_t doc;
   mongoc_client_t *client;
   mongoc_apm_command_started_t event;

   ENTRY;

   client = cluster->client;

   if (!client->apm_callbacks.started) {
      return;
   }

   bson_init (&doc);
   _mongoc_client_prepare_killcursors_command (cursor_id, collection, &doc);
   mongoc_apm_command_started_init (&event,
                                    &doc,
                                    db,
                                    "killCursors",
                                    cluster->request_id,
                                    operation_id,
                                    &server_stream->sd->host,
                                    server_stream->sd->id,
                                    client->apm_context);

   client->apm_callbacks.started (&event);
   mongoc_apm_command_started_cleanup (&event);
   bson_destroy (&doc);

   EXIT;
}


static void
_mongoc_client_monitor_op_killcursors_succeeded (
   mongoc_cluster_t *cluster,
   int64_t duration,
   mongoc_server_stream_t *server_stream,
   int64_t cursor_id,
   int64_t operation_id)
{
   mongoc_client_t *client;
   bson_t doc;
   bson_t cursors_unknown;
   mongoc_apm_command_succeeded_t event;

   ENTRY;

   client = cluster->client;

   if (!client->apm_callbacks.succeeded) {
      EXIT;
   }

   /* fake server reply to killCursors command: {ok: 1, cursorsUnknown: [42]} */
   bson_init (&doc);
   bson_append_int32 (&doc, "ok", 2, 1);
   bson_append_array_begin (&doc, "cursorsUnknown", 14, &cursors_unknown);
   bson_append_int64 (&cursors_unknown, "0", 1, cursor_id);
   bson_append_array_end (&doc, &cursors_unknown);

   mongoc_apm_command_succeeded_init (&event,
                                      duration,
                                      &doc,
                                      "killCursors",
                                      cluster->request_id,
                                      operation_id,
                                      &server_stream->sd->host,
                                      server_stream->sd->id,
                                      client->apm_context);

   client->apm_callbacks.succeeded (&event);

   mongoc_apm_command_succeeded_cleanup (&event);
   bson_destroy (&doc);
}


static void
_mongoc_client_monitor_op_killcursors_failed (
   mongoc_cluster_t *cluster,
   int64_t duration,
   mongoc_server_stream_t *server_stream,
   const bson_error_t *error,
   int64_t operation_id)
{
   mongoc_client_t *client;
   mongoc_apm_command_failed_t event;

   ENTRY;

   client = cluster->client;

   if (!client->apm_callbacks.failed) {
      EXIT;
   }

   mongoc_apm_command_failed_init (&event,
                                   duration,
                                   "killCursors",
                                   error,
                                   cluster->request_id,
                                   operation_id,
                                   &server_stream->sd->host,
                                   server_stream->sd->id,
                                   client->apm_context);

   client->apm_callbacks.failed (&event);

   mongoc_apm_command_failed_cleanup (&event);
}


static void
_mongoc_client_op_killcursors (mongoc_cluster_t *cluster,
                               mongoc_server_stream_t *server_stream,
                               int64_t cursor_id,
                               int64_t operation_id,
                               const char *db,
                               const char *collection)
{
   int64_t started;
   mongoc_rpc_t rpc = {{0}};
   bson_error_t error;
   bool has_ns;
   bool r;

   /* called by old mongoc_client_kill_cursor without db/collection? */
   has_ns = (db && collection);
   started = bson_get_monotonic_time ();

   ++cluster->request_id;

   rpc.header.msg_len = 0;
   rpc.header.request_id = cluster->request_id;
   rpc.header.response_to = 0;
   rpc.header.opcode = MONGOC_OPCODE_KILL_CURSORS;
   rpc.kill_cursors.zero = 0;
   rpc.kill_cursors.cursors = &cursor_id;
   rpc.kill_cursors.n_cursors = 1;

   if (has_ns) {
      _mongoc_client_monitor_op_killcursors (
         cluster, server_stream, cursor_id, operation_id, db, collection);
   }

   r = mongoc_cluster_legacy_rpc_sendv_to_server (
      cluster, &rpc, server_stream, &error);

   if (has_ns) {
      if (r) {
         _mongoc_client_monitor_op_killcursors_succeeded (
            cluster,
            bson_get_monotonic_time () - started,
            server_stream,
            cursor_id,
            operation_id);
      } else {
         _mongoc_client_monitor_op_killcursors_failed (
            cluster,
            bson_get_monotonic_time () - started,
            server_stream,
            &error,
            operation_id);
      }
   }
}


static void
_mongoc_client_killcursors_command (mongoc_cluster_t *cluster,
                                    mongoc_server_stream_t *server_stream,
                                    int64_t cursor_id,
                                    const char *db,
                                    const char *collection,
                                    mongoc_client_session_t *cs)
{
   bson_t command = BSON_INITIALIZER;
   mongoc_cmd_parts_t parts;

   ENTRY;

   _mongoc_client_prepare_killcursors_command (cursor_id, collection, &command);
   mongoc_cmd_parts_init (
      &parts, cluster->client, db, MONGOC_QUERY_SLAVE_OK, &command);
   parts.assembled.operation_id = ++cluster->operation_id;
   mongoc_cmd_parts_set_session (&parts, cs);

   if (mongoc_cmd_parts_assemble (&parts, server_stream, NULL)) {
      /* Find, getMore And killCursors Commands Spec: "The result from the
       * killCursors command MAY be safely ignored."
       */
      mongoc_cluster_run_command_monitored (
         cluster, &parts.assembled, NULL, NULL);
   }

   mongoc_cmd_parts_cleanup (&parts);
   bson_destroy (&command);

   EXIT;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_kill_cursor --
 *
 *       Destroy a cursor on the server.
 *
 *       NOTE: this is only reliable when connected to a single mongod or
 *       mongos. If connected to a replica set, the driver attempts to
 *       kill the cursor on the primary. If connected to multiple mongoses
 *       the kill-cursors message is sent to a *random* mongos.
 *
 *       If no primary, mongos, or standalone server is known, return
 *       without attempting to reconnect.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_client_kill_cursor (mongoc_client_t *client, int64_t cursor_id)
{
   mongoc_topology_t *topology;
   mongoc_server_description_t *selected_server;
   mongoc_read_prefs_t *read_prefs;
   bson_error_t error;
   uint32_t server_id = 0;

   topology = client->topology;
   read_prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   mongoc_mutex_lock (&topology->mutex);
   if (!mongoc_topology_compatible (&topology->description, NULL, &error)) {
      MONGOC_ERROR ("Could not kill cursor: %s", error.message);
      mongoc_mutex_unlock (&topology->mutex);
      mongoc_read_prefs_destroy (read_prefs);
      return;
   }

   /* see if there's a known writable server - do no I/O or retries */
   selected_server =
      mongoc_topology_description_select (&topology->description,
                                          MONGOC_SS_WRITE,
                                          read_prefs,
                                          topology->local_threshold_msec);

   if (selected_server) {
      server_id = selected_server->id;
   }

   mongoc_mutex_unlock (&topology->mutex);

   if (server_id) {
      _mongoc_client_kill_cursor (client,
                                  server_id,
                                  cursor_id,
                                  0 /* operation_id */,
                                  NULL /* db */,
                                  NULL /* collection */,
                                  NULL /* session */);
   } else {
      MONGOC_INFO ("No server available for mongoc_client_kill_cursor");
   }

   mongoc_read_prefs_destroy (read_prefs);
}


char **
mongoc_client_get_database_names (mongoc_client_t *client, bson_error_t *error)
{
   return mongoc_client_get_database_names_with_opts (client, NULL, error);
}


char **
mongoc_client_get_database_names_with_opts (mongoc_client_t *client,
                                            const bson_t *opts,
                                            bson_error_t *error)
{
   bson_iter_t iter;
   const char *name;
   char **ret = NULL;
   int i = 0;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_t cmd = BSON_INITIALIZER;

   BSON_ASSERT (client);
   BSON_APPEND_INT32 (&cmd, "listDatabases", 1);
   BSON_APPEND_BOOL (&cmd, "nameOnly", true);

   /* ignore client read prefs */
   cursor = _mongoc_cursor_new_with_opts (
      client, "admin", false /* is_find */, NULL, opts, NULL, NULL);

   _mongoc_cursor_array_init (cursor, &cmd, "databases");
   bson_destroy (&cmd);

   while (mongoc_cursor_next (cursor, &doc)) {
      if (bson_iter_init (&iter, doc) && bson_iter_find (&iter, "name") &&
          BSON_ITER_HOLDS_UTF8 (&iter) &&
          (name = bson_iter_utf8 (&iter, NULL))) {
         ret = (char **) bson_realloc (ret, sizeof (char *) * (i + 2));
         ret[i] = bson_strdup (name);
         ret[++i] = NULL;
      }
   }

   if (!ret && !mongoc_cursor_error (cursor, error)) {
      ret = (char **) bson_malloc0 (sizeof (void *));
   }

   mongoc_cursor_destroy (cursor);

   return ret;
}


mongoc_cursor_t *
mongoc_client_find_databases (mongoc_client_t *client, bson_error_t *error)
{
   /* existing bug in this deprecated API: error pointer is unused */
   return mongoc_client_find_databases_with_opts (client, NULL);
}


mongoc_cursor_t *
mongoc_client_find_databases_with_opts (mongoc_client_t *client,
                                        const bson_t *opts)
{
   bson_t cmd = BSON_INITIALIZER;
   mongoc_cursor_t *cursor;

   BSON_ASSERT (client);

   BSON_APPEND_INT32 (&cmd, "listDatabases", 1);

   /* ignore client read prefs */
   cursor = _mongoc_cursor_new_with_opts (
      client, "admin", false /* is_find */, NULL, opts, NULL, NULL);

   _mongoc_cursor_array_init (cursor, &cmd, "databases");

   bson_destroy (&cmd);

   return cursor;
}


int32_t
mongoc_client_get_max_message_size (mongoc_client_t *client) /* IN */
{
   BSON_ASSERT (client);

   return mongoc_cluster_get_max_msg_size (&client->cluster);
}


int32_t
mongoc_client_get_max_bson_size (mongoc_client_t *client) /* IN */
{
   BSON_ASSERT (client);

   return mongoc_cluster_get_max_bson_obj_size (&client->cluster);
}


bool
mongoc_client_get_server_status (mongoc_client_t *client,         /* IN */
                                 mongoc_read_prefs_t *read_prefs, /* IN */
                                 bson_t *reply,                   /* OUT */
                                 bson_error_t *error)             /* OUT */
{
   bson_t cmd = BSON_INITIALIZER;
   bool ret = false;

   BSON_ASSERT (client);

   BSON_APPEND_INT32 (&cmd, "serverStatus", 1);
   ret = mongoc_client_command_simple (
      client, "admin", &cmd, read_prefs, reply, error);
   bson_destroy (&cmd);

   return ret;
}


void
mongoc_client_set_stream_initiator (mongoc_client_t *client,
                                    mongoc_stream_initiator_t initiator,
                                    void *user_data)
{
   BSON_ASSERT (client);

   if (!initiator) {
      initiator = mongoc_client_default_stream_initiator;
      user_data = client;
   } else {
      MONGOC_DEBUG ("Using custom stream initiator.");
   }

   client->initiator = initiator;
   client->initiator_data = user_data;

   if (client->topology->single_threaded) {
      mongoc_topology_scanner_set_stream_initiator (
         client->topology->scanner, initiator, user_data);
   }
}


bool
_mongoc_client_set_apm_callbacks_private (mongoc_client_t *client,
                                          mongoc_apm_callbacks_t *callbacks,
                                          void *context)
{
   if (callbacks) {
      memcpy (
         &client->apm_callbacks, callbacks, sizeof (mongoc_apm_callbacks_t));
   } else {
      memset (&client->apm_callbacks, 0, sizeof (mongoc_apm_callbacks_t));
   }

   client->apm_context = context;
   mongoc_topology_set_apm_callbacks (client->topology, callbacks, context);

   return true;
}


bool
mongoc_client_set_apm_callbacks (mongoc_client_t *client,
                                 mongoc_apm_callbacks_t *callbacks,
                                 void *context)
{
   if (!client->topology->single_threaded) {
      MONGOC_ERROR ("Cannot set callbacks on a pooled client, use "
                    "mongoc_client_pool_set_apm_callbacks");
      return false;
   }

   return _mongoc_client_set_apm_callbacks_private (client, callbacks, context);
}


mongoc_server_description_t *
mongoc_client_get_server_description (mongoc_client_t *client,
                                      uint32_t server_id)
{
   /* the error info isn't useful */
   return mongoc_topology_server_by_id (client->topology, server_id, NULL);
}


mongoc_server_description_t **
mongoc_client_get_server_descriptions (const mongoc_client_t *client,
                                       size_t *n /* OUT */)
{
   mongoc_topology_t *topology;
   mongoc_server_description_t **sds;

   BSON_ASSERT (client);
   BSON_ASSERT (n);

   topology = client->topology;

   /* in case the client is pooled */
   mongoc_mutex_lock (&topology->mutex);

   sds = mongoc_topology_description_get_servers (&topology->description, n);

   mongoc_mutex_unlock (&topology->mutex);

   return sds;
}


void
mongoc_server_descriptions_destroy_all (mongoc_server_description_t **sds,
                                        size_t n)
{
   size_t i;

   for (i = 0; i < n; ++i) {
      mongoc_server_description_destroy (sds[i]);
   }

   bson_free (sds);
}


mongoc_server_description_t *
mongoc_client_select_server (mongoc_client_t *client,
                             bool for_writes,
                             const mongoc_read_prefs_t *prefs,
                             bson_error_t *error)
{
   mongoc_ss_optype_t optype = for_writes ? MONGOC_SS_WRITE : MONGOC_SS_READ;
   mongoc_server_description_t *sd;

   if (for_writes && prefs) {
      bson_set_error (error,
                      MONGOC_ERROR_SERVER_SELECTION,
                      MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                      "Cannot use read preferences with for_writes = true");
      return NULL;
   }

   if (!_mongoc_read_prefs_validate (prefs, error)) {
      return NULL;
   }

   sd = mongoc_topology_select (client->topology, optype, prefs, error);
   if (!sd) {
      return NULL;
   }

   if (mongoc_cluster_check_interval (&client->cluster, sd->id)) {
      /* check not required, or it succeeded */
      return sd;
   }

   /* check failed, retry once */
   mongoc_server_description_destroy (sd);
   sd = mongoc_topology_select (client->topology, optype, prefs, error);
   if (sd) {
      return sd;
   }

   return NULL;
}

bool
mongoc_client_set_error_api (mongoc_client_t *client, int32_t version)
{
   if (!client->topology->single_threaded) {
      MONGOC_ERROR ("Cannot set Error API Version on a pooled client, use "
                    "mongoc_client_pool_set_error_api");
      return false;
   }

   if (version != MONGOC_ERROR_API_VERSION_LEGACY &&
       version != MONGOC_ERROR_API_VERSION_2) {
      MONGOC_ERROR ("Unsupported Error API Version: %" PRId32, version);
      return false;
   }

   if (client->error_api_set) {
      MONGOC_ERROR ("Can only set Error API Version once");
      return false;
   }

   client->error_api_version = version;
   client->error_api_set = true;

   return true;
}

bool
mongoc_client_set_appname (mongoc_client_t *client, const char *appname)
{
   if (!client->topology->single_threaded) {
      MONGOC_ERROR ("Cannot call set_appname on a client from a pool");
      return false;
   }

   return _mongoc_topology_set_appname (client->topology, appname);
}

mongoc_server_session_t *
_mongoc_client_pop_server_session (mongoc_client_t *client, bson_error_t *error)
{
   return _mongoc_topology_pop_server_session (client->topology, error);
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_client_lookup_session --
 *
 *       Retrieve a mongoc_client_session_t associated with @client_session_id.
 *       Use this to find the "lsid" and "$clusterTime" to send in the server
 *       command.
 *
 * Returns:
 *       True on success, false on error and @error is set.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */
bool
_mongoc_client_lookup_session (const mongoc_client_t *client,
                               uint32_t client_session_id,
                               mongoc_client_session_t **cs /* OUT */,
                               bson_error_t *error /* OUT */)
{
   ENTRY;

   *cs = mongoc_set_get (client->client_sessions, client_session_id);

   if (*cs) {
      RETURN (true);
   }

   bson_set_error (error,
                   MONGOC_ERROR_COMMAND,
                   MONGOC_ERROR_COMMAND_INVALID_ARG,
                   "Invalid sessionId");

   RETURN (false);
}

void
_mongoc_client_unregister_session (mongoc_client_t *client,
                                   mongoc_client_session_t *session)
{
   mongoc_set_rm (client->client_sessions, session->client_session_id);
}

void
_mongoc_client_push_server_session (mongoc_client_t *client,
                                    mongoc_server_session_t *server_session)
{
   _mongoc_topology_push_server_session (client->topology, server_session);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_client_end_sessions --
 *
 *       End all server sessions in the topology's server session pool.
 *       Don't block long: if server selection or connecting fails, quit.
 *
 *       The server session pool becomes invalid, but it's *not* cleared.
 *       Destroy the topology after this without using any sessions.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_client_end_sessions (mongoc_client_t *client)
{
   mongoc_topology_t *t = client->topology;
   mongoc_read_prefs_t *prefs;
   bson_error_t error;
   uint32_t server_id;
   bson_t cmd = BSON_INITIALIZER;
   mongoc_server_stream_t *stream;
   mongoc_cmd_parts_t parts;
   mongoc_cluster_t *cluster = &client->cluster;
   bool r;

   if (t->session_pool) {
      prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY_PREFERRED);
      server_id =
         mongoc_topology_select_server_id (t, MONGOC_SS_READ, prefs, &error);

      mongoc_read_prefs_destroy (prefs);
      if (!server_id) {
         MONGOC_WARNING ("Couldn't send \"endSessions\": %s", error.message);
         return;
      }

      stream = mongoc_cluster_stream_for_server (
         cluster, server_id, false /* reconnect_ok */, &error);

      if (!stream) {
         MONGOC_WARNING ("Couldn't send \"endSessions\": %s", error.message);
         return;
      }

      _mongoc_topology_end_sessions_cmd (t, &cmd);
      mongoc_cmd_parts_init (
         &parts, client, "admin", MONGOC_QUERY_SLAVE_OK, &cmd);
      parts.assembled.operation_id = ++cluster->operation_id;
      parts.prohibit_lsid = true;

      r = mongoc_cmd_parts_assemble (&parts, stream, &error);
      if (!r) {
         MONGOC_WARNING ("Couldn't construct \"endSessions\" command: %s",
                         error.message);
      } else {
         r = mongoc_cluster_run_command_monitored (
            cluster, &parts.assembled, NULL, &error);

         if (!r) {
            MONGOC_WARNING ("Couldn't send \"endSessions\": %s", error.message);
         }
      }

      bson_destroy (&cmd);
      mongoc_cmd_parts_cleanup (&parts);
      mongoc_server_stream_cleanup (stream);
   }
}
