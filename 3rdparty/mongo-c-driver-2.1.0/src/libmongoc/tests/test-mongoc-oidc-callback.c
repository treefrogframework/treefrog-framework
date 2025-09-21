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

#include <mongoc/mongoc-oidc-callback-private.h>

//

#include <TestSuite.h>
#include <test-conveniences.h>

static mongoc_oidc_credential_t *
_test_oidc_callback_fn_cb (mongoc_oidc_callback_params_t *params)
{
   BSON_UNUSED (params);
   test_error ("should not be invoked");
}

static void
test_oidc_callback_new (void)
{
   // Invalid arguments.
   {
      ASSERT (!mongoc_oidc_callback_new (NULL));
   }

   mongoc_oidc_callback_t *const callback = mongoc_oidc_callback_new (&_test_oidc_callback_fn_cb);
   ASSERT (mongoc_oidc_callback_get_fn (callback) == &_test_oidc_callback_fn_cb);

   // Initial values.
   {
      ASSERT (!mongoc_oidc_callback_get_user_data (callback));
   }

   // Normal values.
   {
      int user_data = 0;

      mongoc_oidc_callback_set_user_data (callback, &user_data);

      ASSERT (mongoc_oidc_callback_get_user_data (callback) == &user_data);
   }

   // "Reset" values.
   {
      mongoc_oidc_callback_set_user_data (callback, NULL);

      ASSERT (!mongoc_oidc_callback_get_user_data (callback));
   }

   mongoc_oidc_callback_destroy (callback);
}

static void
test_oidc_callback_params (void)
{
   mongoc_oidc_callback_params_t *const params = mongoc_oidc_callback_params_new ();

   // Initial values.
   ASSERT (!mongoc_oidc_callback_params_get_timeout (params));
   ASSERT_CMPSTR (mongoc_oidc_callback_params_get_username (params), NULL);
   ASSERT_CMPINT32 (mongoc_oidc_callback_params_get_version (params), ==, MONGOC_PRIVATE_OIDC_CALLBACK_API_VERSION);
   ASSERT (!mongoc_oidc_callback_params_get_cancelled_with_timeout (params));

   // Input parameters.
   {
      // Normal values.
      {
         mongoc_oidc_callback_params_set_timeout (params, 123);
         {
            char username[] = "username";
            mongoc_oidc_callback_params_set_username (params, username);
            username[0] = '\0'; // Ensure a copy was made.
         }
         mongoc_oidc_callback_params_set_version (params, 123);

         const int64_t *timeout = mongoc_oidc_callback_params_get_timeout (params);
         ASSERT (timeout);
         ASSERT_CMPINT64 (*timeout, ==, 123);
         ASSERT_CMPSTR (mongoc_oidc_callback_params_get_username (params), "username");
         ASSERT_CMPINT32 (mongoc_oidc_callback_params_get_version (params), ==, 123);
      }

      // "Reset" values.
      {
         mongoc_oidc_callback_params_set_username (params, NULL);
         mongoc_oidc_callback_params_unset_timeout (params);
         mongoc_oidc_callback_params_set_version (params, MONGOC_PRIVATE_OIDC_CALLBACK_API_VERSION);

         ASSERT_CMPSTR (mongoc_oidc_callback_params_get_username (params), NULL);
         ASSERT (!mongoc_oidc_callback_params_get_timeout (params));
         ASSERT_CMPINT32 (
            mongoc_oidc_callback_params_get_version (params), ==, MONGOC_PRIVATE_OIDC_CALLBACK_API_VERSION);
      }
   }

   // Out parameters.
   {
      // Normal values.
      {
         mongoc_oidc_callback_params_cancel_with_timeout (params);

         ASSERT (mongoc_oidc_callback_params_get_cancelled_with_timeout (params));
      }

      // "Reset" values.
      {
         mongoc_oidc_callback_params_set_cancelled_with_timeout (params, false);

         ASSERT (!mongoc_oidc_callback_params_get_cancelled_with_timeout (params));
      }
   }

   // Owning resources.
   {
      mongoc_oidc_callback_params_set_username (params, "must be freed");
   }

   mongoc_oidc_callback_params_destroy (params);
}

static void
test_oidc_credential (void)
{
   // Normal.
   {
      char token[] = "token";
      mongoc_oidc_credential_t *const cred = mongoc_oidc_credential_new (token);
      token[0] = '\0'; // Ensure a copy was made.

      ASSERT_CMPSTR (mongoc_oidc_credential_get_access_token (cred), "token");
      ASSERT (!mongoc_oidc_credential_get_expires_in (cred));
      mongoc_oidc_credential_destroy (cred);
   }

   // Normal with expires_in.
   {
      char token[] = "token";
      mongoc_oidc_credential_t *const cred = mongoc_oidc_credential_new_with_expires_in (token, 123);
      token[0] = '\0'; // Ensure a copy was made.

      ASSERT_CMPSTR (mongoc_oidc_credential_get_access_token (cred), "token");
      const int64_t *const expires_in = mongoc_oidc_credential_get_expires_in (cred);
      ASSERT (expires_in);
      ASSERT_CMPINT64 (*expires_in, ==, 123);
      mongoc_oidc_credential_destroy (cred);
   }

   // expires_in == 0 is a valid argument.
   {
      mongoc_oidc_credential_t *const cred = mongoc_oidc_credential_new_with_expires_in ("token", 0);
      ASSERT_CMPSTR (mongoc_oidc_credential_get_access_token (cred), "token");
      const int64_t *const expires_in = mongoc_oidc_credential_get_expires_in (cred);
      ASSERT (expires_in);
      ASSERT_CMPINT64 (*expires_in, ==, 0);
      mongoc_oidc_credential_destroy (cred);
   }

   // Invalid arguments.
   {
      ASSERT (!mongoc_oidc_credential_new (NULL));
      ASSERT (!mongoc_oidc_credential_new_with_expires_in (NULL, 123));
      ASSERT (!mongoc_oidc_credential_new_with_expires_in ("token", -1));
   }
}

void
test_mongoc_oidc_callback_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/oidc/callback/new", test_oidc_callback_new);
   TestSuite_Add (suite, "/oidc/callback/params", test_oidc_callback_params);
   TestSuite_Add (suite, "/oidc/callback/credential", test_oidc_credential);
}
