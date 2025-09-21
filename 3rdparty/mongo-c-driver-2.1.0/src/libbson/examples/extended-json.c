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

// extended-json.c shows examples of producing Extended JSON.

#include <bson/bson.h>

#include <stdio.h>

int
main (void)
{
   {
      // bson_as_canonical_extended_json ... begin
      bson_t *b = BCON_NEW ("foo", BCON_INT32 (123));
      char *str = bson_as_canonical_extended_json (b, NULL);
      printf ("Canonical Extended JSON: %s\n", str);
      // Prints:
      // Canonical Extended JSON: { "foo" : { "$numberInt" : "123" } }
      bson_free (str);
      bson_destroy (b);
      // bson_as_canonical_extended_json ... end
   }
   {
      // bson_as_relaxed_extended_json ... begin
      bson_t *b = BCON_NEW ("foo", BCON_INT32 (123));
      char *str = bson_as_relaxed_extended_json (b, NULL);
      printf ("Relaxed Extended JSON: %s\n", str);
      // Prints:
      // Relaxed Extended JSON: { "foo" : 123 }
      bson_free (str);
      bson_destroy (b);
      // bson_as_relaxed_extended_json ... end
   }
   {
      // bson_as_legacy_extended_json ... begin
      bson_t *b = BCON_NEW ("foo", BCON_INT32 (123));
      char *str = bson_as_legacy_extended_json (b, NULL);
      printf ("Legacy Extended JSON: %s\n", str);
      // Prints:
      // Legacy Extended JSON: { "foo" : 123 }
      bson_free (str);
      bson_destroy (b);
      // bson_as_legacy_extended_json ... end
   }
   {
      // bson_array_as_canonical_extended_json ... begin
      bson_t *b = BCON_NEW ("0", BCON_INT32 (1), "1", BCON_UTF8 ("bar"));
      // The document for an array is a normal BSON document with integer values for the keys, starting with 0 and
      // continuing sequentially.
      char *str = bson_array_as_canonical_extended_json (b, NULL);
      printf ("Canonical Extended JSON array: %s\n", str);
      // Prints:
      // Canonical Extended JSON array: [ { "$numberInt" : "1" }, "bar" ]
      bson_free (str);
      bson_destroy (b);
      // bson_array_as_canonical_extended_json ... end
   }
   {
      // bson_array_as_relaxed_extended_json ... begin
      bson_t *b = BCON_NEW ("0", BCON_INT32 (1), "1", BCON_UTF8 ("bar"));
      // The document for an array is a normal BSON document with integer values for the keys, starting with 0 and
      // continuing sequentially.
      char *str = bson_array_as_relaxed_extended_json (b, NULL);
      printf ("Relaxed Extended JSON array: %s\n", str);
      // Prints:
      // Relaxed Extended JSON array: [ 1, "bar" ]
      bson_free (str);
      bson_destroy (b);
      // bson_array_as_relaxed_extended_json ... end
   }
   {
      // bson_array_as_legacy_extended_json ... begin
      bson_t *b = BCON_NEW ("0", BCON_INT32 (1), "1", BCON_UTF8 ("bar"));
      // The document for an array is a normal BSON document with integer values for the keys, starting with 0 and
      // continuing sequentially.
      char *str = bson_array_as_legacy_extended_json (b, NULL);
      printf ("Legacy Extended JSON array: %s\n", str);
      // Prints:
      // Legacy Extended JSON array: [ 1, "bar" ]
      bson_free (str);
      bson_destroy (b);
      // bson_array_as_legacy_extended_json ... end
   }
   {
      // bson_as_json_with_opts ... begin
      bson_t *b = BCON_NEW ("foo", BCON_INT32 (123));
      bson_json_opts_t *opts = bson_json_opts_new (BSON_JSON_MODE_CANONICAL, BSON_MAX_LEN_UNLIMITED);
      char *str = bson_as_json_with_opts (b, NULL, opts);
      printf ("Canonical Extended JSON: %s\n", str);
      // Prints:
      // Canonical Extended JSON: { "foo" : { "$numberInt" : "123" } }
      bson_free (str);
      bson_json_opts_destroy (opts);
      bson_destroy (b);
      // bson_as_json_with_opts ... end
   }
}
