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

#include "bson/bson-compat.h"

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bson/bson-atomic.h"
#include "bson/bson-clock.h"
#include "bson/bson-context.h"
#include "bson/bson-context-private.h"
#include "bson/bson-memory.h"
#include "common-thread-private.h"

#ifdef BSON_HAVE_SYSCALL_TID
#include <sys/syscall.h>
#endif


#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif


/*
 * Globals.
 */
static bson_context_t gContextDefault;

static BSON_INLINE uint16_t
_bson_getpid (void)
{
   uint16_t pid;
#ifdef BSON_OS_WIN32
   DWORD real_pid;

   real_pid = GetCurrentProcessId ();
   pid = (real_pid & 0xFFFF) ^ ((real_pid >> 16) & 0xFFFF);
#else
   pid = getpid ();
#endif

   return pid;
}

/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_set_oid_seq32 --
 *
 *       32-bit sequence generator, non-thread-safe version.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_set_oid_seq32 (bson_context_t *context, /* IN */
                             bson_oid_t *oid)         /* OUT */
{
   uint32_t seq = context->seq32++;

   seq = BSON_UINT32_TO_BE (seq);
   memcpy (&oid->bytes[9], ((uint8_t *) &seq) + 1, 3);
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_set_oid_seq32_threadsafe --
 *
 *       Thread-safe version of 32-bit sequence generator.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_set_oid_seq32_threadsafe (bson_context_t *context, /* IN */
                                        bson_oid_t *oid)         /* OUT */
{
   int32_t seq = bson_atomic_int_add (&context->seq32, 1);

   seq = BSON_UINT32_TO_BE (seq);
   memcpy (&oid->bytes[9], ((uint8_t *) &seq) + 1, 3);
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_set_oid_seq64 --
 *
 *       64-bit oid sequence generator, non-thread-safe version.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_set_oid_seq64 (bson_context_t *context, /* IN */
                             bson_oid_t *oid)         /* OUT */
{
   uint64_t seq;

   BSON_ASSERT (context);
   BSON_ASSERT (oid);

   seq = BSON_UINT64_TO_BE (context->seq64++);
   memcpy (&oid->bytes[4], &seq, sizeof (seq));
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_set_oid_seq64_threadsafe --
 *
 *       Thread-safe 64-bit sequence generator.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_set_oid_seq64_threadsafe (bson_context_t *context, /* IN */
                                        bson_oid_t *oid)         /* OUT */
{
   int64_t seq = bson_atomic_int64_add (&context->seq64, 1);

   seq = BSON_UINT64_TO_BE (seq);
   memcpy (&oid->bytes[4], &seq, sizeof (seq));
}


static void
_bson_context_init_random (bson_context_t *context, bool init_sequence);

/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_set_oid_rand --
 *
 *       Sets the process specific five byte random sequence in an oid.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */
void
_bson_context_set_oid_rand (bson_context_t *context, bson_oid_t *oid)
{
   BSON_ASSERT (context);
   BSON_ASSERT (oid);

   if (context->flags & BSON_CONTEXT_DISABLE_PID_CACHE) {
      uint16_t pid = _bson_getpid ();

      if (pid != context->pid) {
         context->pid = pid;
         /* randomize the random bytes, not the sequence. */
         _bson_context_init_random (context, false);
      }
   }
   memcpy (&oid->bytes[4], &context->rand, sizeof (context->rand));
}

/*
 *--------------------------------------------------------------------------
 *
 * _get_rand --
 *
 *       Gets a random four byte integer. Callers that will use the "rand"
 *       function must call "srand" prior.
 *
 * Returns:
 *       A random int32_t.
 *
 *--------------------------------------------------------------------------
 */
static int32_t
_get_rand (unsigned int *pseed)
{
   int32_t result = 0;
#ifndef BSON_HAVE_RAND_R
   /* ms's runtime is multithreaded by default, so no rand_r */
   /* no rand_r on android either */
   result = rand ();
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || \
   defined(__OpenBSD__)
   arc4random_buf (&result, sizeof (result));
#else
   result = rand_r (pseed);
#endif
   return result;
}


/*
 * --------------------------------------------------------------------------
 *
 * _bson_context_get_hostname
 *
 *       Gets the hostname of the machine, logs a warning on failure. "out"
 *       must be an array of HOST_NAME_MAX bytes.
 *
 * --------------------------------------------------------------------------
 */
static void
_bson_context_get_hostname (char *out)
{
   if (gethostname (out, HOST_NAME_MAX) != 0) {
      if (errno == ENAMETOOLONG) {
         fprintf (stderr,
                  "hostname exceeds %d characters, truncating.",
                  HOST_NAME_MAX);
      } else {
         fprintf (stderr, "unable to get hostname: %d", errno);
      }
   }
   out[HOST_NAME_MAX - 1] = '\0';
}


static void
_bson_context_init_random (bson_context_t *context, bool init_sequence)
{
   int64_t rand_bytes;
   struct timeval tv;
   unsigned int seed = 0;
   char hostname[HOST_NAME_MAX];
   char *ptr;
   int hostname_chars_left;

   /*
    * The seed consists of the following xor'd together:
    * - current time in seconds
    * - current time in milliseconds
    * - current pid
    * - current hostname
    */
   bson_gettimeofday (&tv);
   seed ^= (unsigned int) tv.tv_sec;
   seed ^= (unsigned int) tv.tv_usec;
   seed ^= (unsigned int) context->pid;

   context->gethostname (hostname);
   hostname_chars_left = strlen (hostname);
   ptr = hostname;
   while (hostname_chars_left) {
      uint32_t hostname_chunk = 0;
      uint32_t to_copy = hostname_chars_left > 4 ? 4 : hostname_chars_left;

      memcpy (&hostname_chunk, ptr, to_copy);
      seed ^= (unsigned int) hostname_chunk;
      hostname_chars_left -= to_copy;
      ptr += to_copy;
   }

#ifndef BSON_HAVE_RAND_R
   srand (seed);
#endif

   /* Generate a seed for the random starting position of our increment
    * bytes and the five byte random number. */
   if (init_sequence) {
      /* We mask off the last nibble so that the last digit of the OID will
       * start at zero. Just to be nice. */
      context->seq32 = _get_rand (&seed) & 0x007FFFF0;
   }

   rand_bytes = _get_rand (&seed);
   rand_bytes <<= 32;
   rand_bytes |= _get_rand (&seed);

   /* Copy five random bytes, endianness does not matter. */
   memcpy (&context->rand, (char *) &rand_bytes, sizeof (context->rand));
}

static void
_bson_context_init (bson_context_t *context, bson_context_flags_t flags)
{
   context->flags = (int) flags;
   context->oid_set_seq32 = _bson_context_set_oid_seq32;
   context->oid_set_seq64 = _bson_context_set_oid_seq64;
   context->gethostname = _bson_context_get_hostname;

   if ((flags & BSON_CONTEXT_THREAD_SAFE)) {
      context->oid_set_seq32 = _bson_context_set_oid_seq32_threadsafe;
      context->oid_set_seq64 = _bson_context_set_oid_seq64_threadsafe;
   }

   context->pid = _bson_getpid ();
   _bson_context_init_random (context, true);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_context_new --
 *
 *       Initializes a new context with the flags specified.
 *
 *       In most cases, you want to call this with @flags set to
 *       BSON_CONTEXT_NONE.
 *
 *       If you are running on Linux, %BSON_CONTEXT_USE_TASK_ID can result
 *       in a healthy speedup for multi-threaded scenarios.
 *
 *       If you absolutely must have a single context for your application
 *       and use more than one thread, then %BSON_CONTEXT_THREAD_SAFE should
 *       be bitwise-or'd with your flags. This requires synchronization
 *       between threads.
 *
 *       If you expect your hostname to change often, you may consider
 *       specifying %BSON_CONTEXT_DISABLE_HOST_CACHE so that gethostname()
 *       is called for every OID generated. This is much slower.
 *
 *       If you expect your pid to change without notice, such as from an
 *       unexpected call to fork(), then specify
 *       %BSON_CONTEXT_DISABLE_PID_CACHE.
 *
 * Returns:
 *       A newly allocated bson_context_t that should be freed with
 *       bson_context_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bson_context_t *
bson_context_new (bson_context_flags_t flags)
{
   bson_context_t *context;

   context = bson_malloc0 (sizeof *context);
   _bson_context_init (context, flags);

   return context;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_context_destroy --
 *
 *       Cleans up a bson_context_t and releases any associated resources.
 *       This should be called when you are done using @context.
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
bson_context_destroy (bson_context_t *context) /* IN */
{
   bson_free (context);
}


static BSON_ONCE_FUN (_bson_context_init_default)
{
   _bson_context_init (
      &gContextDefault,
      (BSON_CONTEXT_THREAD_SAFE | BSON_CONTEXT_DISABLE_PID_CACHE));
   BSON_ONCE_RETURN;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_context_get_default --
 *
 *       Fetches the default, thread-safe implementation of #bson_context_t.
 *       If you need faster generation, it is recommended you create your
 *       own #bson_context_t with bson_context_new().
 *
 * Returns:
 *       A shared instance to the default #bson_context_t. This should not
 *       be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bson_context_t *
bson_context_get_default (void)
{
   static bson_once_t once = BSON_ONCE_INIT;

   bson_once (&once, _bson_context_init_default);

   return &gContextDefault;
}
