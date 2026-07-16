/*
 * Copyright 2009-present MongoDB, Inc.
 * Licensed under the Apache License, Version 2.0 (the "License");
 *
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

#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-host-list-private.h>

#include <inttypes.h> // PRIu16
/* strcasecmp on windows */
#include <mongoc/mongoc-util-private.h>

#include <mongoc/utlist.h>

#include <mlib/cmp.h>

static mongoc_host_list_t *
_mongoc_host_list_find_host_and_port(mongoc_host_list_t *hosts, const char *host_and_port)
{
   mongoc_host_list_t *iter;
   LL_FOREACH(hosts, iter)
   {
      BSON_ASSERT(iter);

      if (strcasecmp(iter->host_and_port, host_and_port) == 0) {
         return iter;
      }
   }

   return NULL;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_host_list_upsert --
 *
 *       If new_host is not already in list, copy and add it to the end of list.
 *       If *list == NULL, then it will be set to a new host.
 *
 * Returns:
 *       Nothing.
 *
 *--------------------------------------------------------------------------
 */
void
_mongoc_host_list_upsert(mongoc_host_list_t **list, const mongoc_host_list_t *new_host)
{
   mongoc_host_list_t *link = NULL;
   mongoc_host_list_t *next_link = NULL;

   BSON_ASSERT(list);
   if (!new_host) {
      return;
   }

   link = _mongoc_host_list_find_host_and_port(*list, new_host->host_and_port);

   if (!link) {
      link = bson_malloc0(sizeof(mongoc_host_list_t));
      LL_APPEND(*list, link);
   } else {
      /* Make sure linking is preserved when copying data into final. */
      next_link = link->next;
   }

   memcpy(link, new_host, sizeof(mongoc_host_list_t));
   link->next = next_link;
}

/* Duplicates a host list.
 */
mongoc_host_list_t *
_mongoc_host_list_copy_all(const mongoc_host_list_t *src)
{
   mongoc_host_list_t *tail = NULL;
   const mongoc_host_list_t *src_iter;
   mongoc_host_list_t *head = NULL;

   LL_FOREACH(src, src_iter)
   {
      tail = bson_malloc0(sizeof(mongoc_host_list_t));
      memcpy(tail, src_iter, sizeof(mongoc_host_list_t));

      LL_PREPEND(head, tail);
   }

   return head;
}

size_t
_mongoc_host_list_length(const mongoc_host_list_t *list)
{
   const mongoc_host_list_t *tmp;
   size_t counter = 0u;

   tmp = list;
   while (tmp) {
      tmp = tmp->next;
      counter++;
   }

   return counter;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_host_list_compare_one --
 *
 *       Check two hosts have the same domain (case-insensitive), port,
 *       and address family.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */
bool
_mongoc_host_list_compare_one(const mongoc_host_list_t *host_a, const mongoc_host_list_t *host_b)
{
   return (0 == strcasecmp(host_a->host_and_port, host_b->host_and_port) && host_a->family == host_b->family);
}

bool
_mongoc_host_list_contains_one(mongoc_host_list_t *host_list, mongoc_host_list_t *host)
{
   return NULL != _mongoc_host_list_find_host_and_port(host_list, host->host_and_port);
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_host_list_destroy_all --
 *
 *       Destroy whole linked list of hosts.
 *
 *--------------------------------------------------------------------------
 */
void
_mongoc_host_list_destroy_all(mongoc_host_list_t *host)
{
   mongoc_host_list_t *tmp;

   while (host) {
      tmp = host->next;
      bson_free(host);
      host = tmp;
   }
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_host_list_from_string --
 *
 *       Populate a mongoc_host_list_t from a fully qualified address
 *
 *--------------------------------------------------------------------------
 */
bool
_mongoc_host_list_from_string(mongoc_host_list_t *link_, const char *address)
{
   bson_error_t error = {0};
   bool r = _mongoc_host_list_from_string_with_err(link_, address, &error);
   if (!r) {
      MONGOC_ERROR("Could not parse address %s: %s", address, error.message);
      return false;
   }
   return true;
}

static inline bool
_parse_host_ipv6(mongoc_host_list_t *link, mstr_view addr, bson_error_t *error)
{
   bson_error_reset(error);
   _mongoc_set_error(error, 0, 0, "Invalid IPv6 literal address '%.*s'", MSTR_FMT(addr));
   // Find the opening bracket (must be the first char)
   const size_t open_square_pos = mstr_find(addr, mstr_cstring("["), 0, 1);
   if (open_square_pos != 0) {
      _mongoc_set_error(error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "%s: Must start with a bracket '['",
                        error->message);
      return false;
   }
   // Find the closing bracket
   const size_t close_square_pos = mstr_find(addr, mstr_cstring("]"));
   if (close_square_pos == SIZE_MAX) {
      // Closing bracket is missing
      _mongoc_set_error(error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "%s: Mising closing bracket ']'",
                        error->message);
      return false;
   }
   // Find the port delimiter, if present. It must be the next character
   const size_t port_delim_pos = mstr_find(addr, mstr_cstring(":"), close_square_pos + 1, 1);

   if (port_delim_pos == SIZE_MAX) {
      // There is no port specifier, or it is misplaced, so the closing bracket
      // should be the final character:
      if (close_square_pos != addr.len - 1) {
         _mongoc_set_error(error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_COMMAND_INVALID_ARG,
                           "%s: Invalid trailing content following closing bracket ']'",
                           error->message);
         return false;
      }
   }

   uint16_t port = MONGOC_DEFAULT_PORT;
   if (port_delim_pos != SIZE_MAX) {
      bson_error_t err2;
      const mstr_view port_str = mstr_substr(addr, port_delim_pos + 1);
      if (!_mongoc_parse_port(port_str, &port, &err2)) {
         _mongoc_set_error(error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_COMMAND_INVALID_ARG,
                           "%s: Invalid port '%.*s': %s",
                           error->message,
                           MSTR_FMT(port_str),
                           err2.message);
         return false;
      }
   }

   return _mongoc_host_list_from_hostport_with_err(
      link, mstr_slice(addr, open_square_pos + 1, close_square_pos), port, error);
}

static inline bool
_parse_host(mongoc_host_list_t *link, mstr_view spec, bson_error_t *error)
{
   if (mstr_contains(spec, mstr_cstring("]"))) {
      // There is a "]" bracket, so this is probably an IPv6 literal, which is
      // more strict
      return _parse_host_ipv6(link, spec, error);
   }
   // Parsing anything else is simpler.
   uint16_t port = MONGOC_DEFAULT_PORT;
   // Try to split around the port delimiter:
   mstr_view hostname, port_str;
   if (mstr_split_around(spec, mstr_cstring(":"), &hostname, &port_str)) {
      // We have a ":" delimiter. Try to parse it as a port number:
      bson_error_t e2;
      if (!_mongoc_parse_port(port_str, &port, &e2)) {
         // Invalid port number
         _mongoc_set_error(error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_COMMAND_INVALID_ARG,
                           "Invalid host specifier '%.*s': Invalid port string '%.*s': %s",
                           MSTR_FMT(spec),
                           MSTR_FMT(port_str),
                           e2.message);
         return false;
      }
   }

   return _mongoc_host_list_from_hostport_with_err(link, hostname, port, error);
}

bool
_mongoc_host_list_from_string_with_err(mongoc_host_list_t *link_, const char *address, bson_error_t *error)
{
   return _parse_host(link_, mstr_cstring(address), error);
}

bool
_mongoc_host_list_from_hostport_with_err(mongoc_host_list_t *link_, mstr_view host, uint16_t port, bson_error_t *error)
{
   BSON_ASSERT(link_);
   *link_ = (mongoc_host_list_t){
      .next = NULL,
      .port = port,
   };

   if (host.len == 0) {
      _mongoc_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_NAME_RESOLUTION, "Empty hostname in URI");
      return false;
   }

   if (host.len > BSON_HOST_NAME_MAX) {
      _mongoc_set_error(error,
                        MONGOC_ERROR_STREAM,
                        MONGOC_ERROR_STREAM_NAME_RESOLUTION,
                        "Hostname provided in URI is too long, max is %d chars",
                        BSON_HOST_NAME_MAX);
      return false;
   }

   bson_strncpy(link_->host, host.data, host.len + 1);

   /* like "fe80::1" or "::1" */
   if (mstr_contains(host, mstr_cstring(":"))) {
      link_->family = AF_INET6;

      // Check that IPv6 literal is two less than the max to account for `[` and
      // `]` added below.
      if (host.len > BSON_HOST_NAME_MAX - 2) {
         _mongoc_set_error(error,
                           MONGOC_ERROR_STREAM,
                           MONGOC_ERROR_STREAM_NAME_RESOLUTION,
                           "IPv6 literal provided in URI is too long, max is %d chars",
                           BSON_HOST_NAME_MAX - 2);
         return false;
      }

      mongoc_lowercase(link_->host, link_->host);
      int req =
         bson_snprintf(link_->host_and_port, sizeof link_->host_and_port, "[%s]:%" PRIu16, link_->host, link_->port);
      BSON_ASSERT(mlib_in_range(size_t, req));
      // Use `<`, not `<=` to account for NULL byte.
      BSON_ASSERT((size_t)req < sizeof link_->host_and_port);
   } else if (mstr_contains(host, mstr_cstring("/")) && mstr_contains(host, mstr_cstring(".sock"))) {
      link_->family = AF_UNIX;
      bson_strncpy(link_->host_and_port, link_->host, host.len + 1);
   } else {
      /* This is either an IPv4 or hostname. */
      link_->family = AF_UNSPEC;

      mongoc_lowercase(link_->host, link_->host);
      int req =
         bson_snprintf(link_->host_and_port, sizeof link_->host_and_port, "%s:%" PRIu16, link_->host, link_->port);
      BSON_ASSERT(mlib_in_range(size_t, req));
      // Use `<`, not `<=` to account for NULL byte.
      BSON_ASSERT((size_t)req < sizeof link_->host_and_port);
   }

   return true;
}

void
_mongoc_host_list_remove_host(mongoc_host_list_t **hosts, const char *host, uint16_t port)
{
   mongoc_host_list_t *current;
   mongoc_host_list_t *prev = NULL;

   for (current = *hosts; current; prev = current, current = current->next) {
      if ((current->port == port) && (strcmp(current->host, host) == 0)) {
         /* Node found, unlink. */
         if (prev) {
            prev->next = current->next;
         } else {
            /* No previous, unlinking at head. */
            *hosts = current->next;
         }
         bson_free(current);
         break;
      }
   }
}
