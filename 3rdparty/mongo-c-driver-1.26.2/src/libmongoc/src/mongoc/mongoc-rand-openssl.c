/*
 * Copyright 2016 MongoDB, Inc.
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

#include "mongoc-config.h"

#ifdef MONGOC_ENABLE_CRYPTO_LIBCRYPTO

#include "mongoc-rand.h"
#include "mongoc-rand-private.h"

#include "mongoc.h"

#include <openssl/opensslv.h>
#include <openssl/rand.h>

int
_mongoc_rand_bytes (uint8_t *buf, int num)
{
#if OPENSSL_VERSION_NUMBER < 0x10101000L
   /* Versions of OpenSSL before 1.1.1 can potentially produce the same random
    * sequences in processes with the same PID. Rather than attempt to detect
    * PID changes (useful for parent/child forking but not if PIDs wrap), mix
    * the current time into the generator's state.
    * See also: https://wiki.openssl.org/index.php/Random_fork-safety */
   struct timeval tv;

   bson_gettimeofday (&tv);
   RAND_add (&tv, sizeof(tv), 0.0);
#endif

   return RAND_bytes (buf, num);
}

void
mongoc_rand_seed (const void *buf, int num)
{
   RAND_seed (buf, num);
}

void
mongoc_rand_add (const void *buf, int num, double entropy)
{
   RAND_add (buf, num, entropy);
}

int
mongoc_rand_status (void)
{
   return RAND_status ();
}

#endif
