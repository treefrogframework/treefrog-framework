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


#ifndef MONGOC_TRACE_PRIVATE_H
#define MONGOC_TRACE_PRIVATE_H


#include <mongoc/mongoc-log-private.h>

#include <mongoc/mongoc-config.h>
#include <mongoc/mongoc-log.h>

#include <bson/bson.h>

#include <mlib/config.h>

#include <ctype.h>


BSON_BEGIN_DECLS

// `gLogTrace` determines if tracing is enabled at runtime.
extern bool gLogTrace;

#define TRACE(msg, ...)                                         \
   if (1) {                                                     \
      mlib_diagnostic_push ();                                  \
      mlib_disable_constant_conditional_expression_warnings (); \
      if (MONGOC_TRACE_ENABLED && gLogTrace) {                  \
         mongoc_log (MONGOC_LOG_LEVEL_TRACE,                    \
                     MONGOC_LOG_DOMAIN,                         \
                     "TRACE: %s():%d " msg,                     \
                     BSON_FUNC,                                 \
                     (int) (__LINE__),                          \
                     __VA_ARGS__);                              \
      }                                                         \
      mlib_diagnostic_pop ();                                   \
   } else                                                       \
      ((void) 0)

#define ENTRY                                                                                                   \
   if (1) {                                                                                                     \
      mlib_diagnostic_push ();                                                                                  \
      mlib_disable_constant_conditional_expression_warnings ();                                                 \
      if (MONGOC_TRACE_ENABLED && gLogTrace) {                                                                  \
         mongoc_log (MONGOC_LOG_LEVEL_TRACE, MONGOC_LOG_DOMAIN, "ENTRY: %s():%d", BSON_FUNC, (int) (__LINE__)); \
      }                                                                                                         \
      mlib_diagnostic_pop ();                                                                                   \
   } else                                                                                                       \
      ((void) 0)

#define EXIT                                                                                                    \
   do {                                                                                                         \
      mlib_diagnostic_push ();                                                                                  \
      mlib_disable_constant_conditional_expression_warnings ();                                                 \
      if (MONGOC_TRACE_ENABLED && gLogTrace) {                                                                  \
         mongoc_log (MONGOC_LOG_LEVEL_TRACE, MONGOC_LOG_DOMAIN, " EXIT: %s():%d", BSON_FUNC, (int) (__LINE__)); \
      }                                                                                                         \
      mlib_diagnostic_pop ();                                                                                   \
      return;                                                                                                   \
   } while (0) // do-while instead of if-else to avoid false-positive -Wreturn-type warnings with GCC 11.

#define RETURN(ret)                                                                                             \
   do {                                                                                                         \
      mlib_diagnostic_push ();                                                                                  \
      mlib_disable_constant_conditional_expression_warnings ();                                                 \
      if (MONGOC_TRACE_ENABLED && gLogTrace) {                                                                  \
         mongoc_log (MONGOC_LOG_LEVEL_TRACE, MONGOC_LOG_DOMAIN, " EXIT: %s():%d", BSON_FUNC, (int) (__LINE__)); \
      }                                                                                                         \
      mlib_diagnostic_pop ();                                                                                   \
      return ret;                                                                                               \
   } while (0) // do-while instead of if-else to avoid false-positive -Wreturn-type warnings with GCC 11.

#define GOTO(label)                                                                                               \
   if (1) {                                                                                                       \
      mlib_diagnostic_push ();                                                                                    \
      mlib_disable_constant_conditional_expression_warnings ();                                                   \
      if (MONGOC_TRACE_ENABLED && gLogTrace) {                                                                    \
         mongoc_log (                                                                                             \
            MONGOC_LOG_LEVEL_TRACE, MONGOC_LOG_DOMAIN, " GOTO: %s():%d %s", BSON_FUNC, (int) (__LINE__), #label); \
      }                                                                                                           \
      mlib_diagnostic_pop ();                                                                                     \
      goto label;                                                                                                 \
   } else                                                                                                         \
      ((void) 0)

#define DUMP_BSON(_bson)                                               \
   if (1) {                                                            \
      mlib_diagnostic_push ();                                         \
      mlib_disable_constant_conditional_expression_warnings ();        \
      if (MONGOC_TRACE_ENABLED && gLogTrace) {                         \
         char *_bson_str;                                              \
         if (_bson) {                                                  \
            _bson_str = bson_as_canonical_extended_json (_bson, NULL); \
         } else {                                                      \
            _bson_str = bson_strdup ("<NULL>");                        \
         }                                                             \
         mongoc_log (MONGOC_LOG_LEVEL_TRACE,                           \
                     MONGOC_LOG_DOMAIN,                                \
                     "TRACE: %s():%d %s = %s",                         \
                     BSON_FUNC,                                        \
                     (int) (__LINE__),                                 \
                     #_bson,                                           \
                     _bson_str);                                       \
         bson_free (_bson_str);                                        \
      }                                                                \
      mlib_diagnostic_pop ();                                          \
   } else                                                              \
      ((void) 0)

#define DUMP_IOVEC(_n, _iov, _iovcnt)                               \
   if (1) {                                                         \
      mlib_diagnostic_push ();                                      \
      mlib_disable_constant_conditional_expression_warnings ();     \
      if (MONGOC_TRACE_ENABLED && gLogTrace) {                      \
         mongoc_log (MONGOC_LOG_LEVEL_TRACE,                        \
                     MONGOC_LOG_DOMAIN,                             \
                     "TRACE: %s():%d %s = %p [%d]",                 \
                     BSON_FUNC,                                     \
                     (int) (__LINE__),                              \
                     #_n,                                           \
                     (void *) _iov,                                 \
                     (int) _iovcnt);                                \
         mongoc_log_trace_iovec (MONGOC_LOG_DOMAIN, _iov, _iovcnt); \
      }                                                             \
      mlib_diagnostic_pop ();                                       \
   } else                                                           \
      ((void) 0)


BSON_END_DECLS


#endif /* MONGOC_TRACE_PRIVATE_H */
