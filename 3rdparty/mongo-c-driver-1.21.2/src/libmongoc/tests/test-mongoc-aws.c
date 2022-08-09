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

#include "TestSuite.h"
#include "test-libmongoc.h"
#include "mongoc/mongoc-cluster-aws-private.h"

void
test_obtain_credentials (void *unused)
{
   mongoc_uri_t *uri;
   _mongoc_aws_credentials_t creds;
   bool ret;
   bson_error_t error;

   /* A username specified with a password is parsed correctly. */
   uri = mongoc_uri_new ("mongodb://"
                         "access_key_id:secret_access_key@localhost/?"
                         "authMechanism=MONGODB-AWS");
   ret = _mongoc_aws_credentials_obtain (uri, &creds, &error);
   ASSERT_OR_PRINT (ret, error);
   ASSERT_CMPSTR (creds.access_key_id, "access_key_id");
   ASSERT_CMPSTR (creds.secret_access_key, "secret_access_key");
   BSON_ASSERT (creds.session_token == NULL);
   _mongoc_aws_credentials_cleanup (&creds);
   mongoc_uri_destroy (uri);

   /* A username specified with no password is an error. */
   uri = mongoc_uri_new (
      "mongodb://access_key_id:@localhost/?authMechanism=MONGODB-AWS");
   ret = _mongoc_aws_credentials_obtain (uri, &creds, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (
      error,
      MONGOC_ERROR_CLIENT,
      MONGOC_ERROR_CLIENT_AUTHENTICATE,
      "ACCESS_KEY_ID is set, but SECRET_ACCESS_KEY is missing");
   _mongoc_aws_credentials_cleanup (&creds);
   mongoc_uri_destroy (uri);

   /* Password not set at all (not empty string) */
   uri = mongoc_uri_new (
      "mongodb://access_key_id@localhost/?authMechanism=MONGODB-AWS");
   ret = _mongoc_aws_credentials_obtain (uri, &creds, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (
      error,
      MONGOC_ERROR_CLIENT,
      MONGOC_ERROR_CLIENT_AUTHENTICATE,
      "ACCESS_KEY_ID is set, but SECRET_ACCESS_KEY is missing");
   _mongoc_aws_credentials_cleanup (&creds);
   mongoc_uri_destroy (uri);

   /* A session token may be set through the AWS_SESSION_TOKEN auth mechanism
    * property */
   uri = mongoc_uri_new ("mongodb://"
                         "access_key_id:secret_access_key@localhost/?"
                         "authMechanism=MONGODB-AWS&authMechanismProperties="
                         "AWS_SESSION_TOKEN:token");
   ret = _mongoc_aws_credentials_obtain (uri, &creds, &error);
   ASSERT_OR_PRINT (ret, error);
   ASSERT_CMPSTR (creds.access_key_id, "access_key_id");
   ASSERT_CMPSTR (creds.secret_access_key, "secret_access_key");
   ASSERT_CMPSTR (creds.session_token, "token");
   _mongoc_aws_credentials_cleanup (&creds);
   mongoc_uri_destroy (uri);

   /* A session token in the URI with no username/password is an error. */
   uri = mongoc_uri_new ("mongodb://localhost/"
                         "?authMechanism=MONGODB-AWS&authMechanismProperties="
                         "AWS_SESSION_TOKEN:token");
   ret = _mongoc_aws_credentials_obtain (uri, &creds, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_AUTHENTICATE,
                          "AWS_SESSION_TOKEN is set, but ACCESS_KEY_ID and "
                          "SECRET_ACCESS_KEY are missing");
   _mongoc_aws_credentials_cleanup (&creds);
   mongoc_uri_destroy (uri);
}

void
test_obtain_credentials_from_env (void *unused)
{
   mongoc_uri_t *uri;
   _mongoc_aws_credentials_t creds;
   bool ret;
   bson_error_t error;

   /* "clear" environment variables by setting them to the empty string. */
   test_framework_setenv ("AWS_ACCESS_KEY_ID", "");
   test_framework_setenv ("AWS_SECRET_ACCESS_KEY", "");
   test_framework_setenv ("AWS_SESSION_TOKEN", "");

   /* Environment variables are used if username/password is not set. */
   test_framework_setenv ("AWS_ACCESS_KEY_ID", "access_key_id");
   test_framework_setenv ("AWS_SECRET_ACCESS_KEY", "secret_access_key");
   uri = mongoc_uri_new ("mongodb://localhost/?authMechanism=MONGODB-AWS");
   ret = _mongoc_aws_credentials_obtain (uri, &creds, &error);
   ASSERT_OR_PRINT (ret, error);
   ASSERT_CMPSTR (creds.access_key_id, "access_key_id");
   ASSERT_CMPSTR (creds.secret_access_key, "secret_access_key");
   BSON_ASSERT (creds.session_token == NULL);
   _mongoc_aws_credentials_cleanup (&creds);
   mongoc_uri_destroy (uri);

   /* Omitting one of the required environment variables is an error. */
   test_framework_setenv ("AWS_ACCESS_KEY_ID", "access_key_id");
   test_framework_setenv ("AWS_SECRET_ACCESS_KEY", "");
   uri = mongoc_uri_new ("mongodb://localhost/?authMechanism=MONGODB-AWS");
   ret = _mongoc_aws_credentials_obtain (uri, &creds, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (
      error,
      MONGOC_ERROR_CLIENT,
      MONGOC_ERROR_CLIENT_AUTHENTICATE,
      "ACCESS_KEY_ID is set, but SECRET_ACCESS_KEY is missing");
   _mongoc_aws_credentials_cleanup (&creds);
   mongoc_uri_destroy (uri);

   /* Omitting one of the required environment variables is an error. */
   test_framework_setenv ("AWS_ACCESS_KEY_ID", "");
   test_framework_setenv ("AWS_SECRET_ACCESS_KEY", "secret_access_key");
   uri = mongoc_uri_new ("mongodb://localhost/?authMechanism=MONGODB-AWS");
   ret = _mongoc_aws_credentials_obtain (uri, &creds, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (
      error,
      MONGOC_ERROR_CLIENT,
      MONGOC_ERROR_CLIENT_AUTHENTICATE,
      "SECRET_ACCESS_KEY is set, but ACCESS_KEY_ID is missing");
   _mongoc_aws_credentials_cleanup (&creds);
   mongoc_uri_destroy (uri);

   /* Only specifying the token is an error. */
   test_framework_setenv ("AWS_ACCESS_KEY_ID", "");
   test_framework_setenv ("AWS_SECRET_ACCESS_KEY", "");
   test_framework_setenv ("AWS_SESSION_TOKEN", "token");
   uri = mongoc_uri_new ("mongodb://localhost/?authMechanism=MONGODB-AWS");
   ret = _mongoc_aws_credentials_obtain (uri, &creds, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_AUTHENTICATE,
                          "AWS_SESSION_TOKEN is set, but ACCESS_KEY_ID and "
                          "SECRET_ACCESS_KEY are missing");
   _mongoc_aws_credentials_cleanup (&creds);
   mongoc_uri_destroy (uri);

   /* But a session token in the environment is picked up. */
   test_framework_setenv ("AWS_ACCESS_KEY_ID", "access_key_id");
   test_framework_setenv ("AWS_SECRET_ACCESS_KEY", "secret_access_key");
   test_framework_setenv ("AWS_SESSION_TOKEN", "token");
   uri = mongoc_uri_new ("mongodb://localhost/?authMechanism=MONGODB-AWS");
   ret = _mongoc_aws_credentials_obtain (uri, &creds, &error);
   ASSERT_OR_PRINT (ret, error);
   ASSERT_CMPSTR (creds.access_key_id, "access_key_id");
   ASSERT_CMPSTR (creds.secret_access_key, "secret_access_key");
   ASSERT_CMPSTR (creds.session_token, "token");
   _mongoc_aws_credentials_cleanup (&creds);
   mongoc_uri_destroy (uri);

   /* "clear" environment variables by setting them to the empty string. */
   test_framework_setenv ("AWS_ACCESS_KEY_ID", "");
   test_framework_setenv ("AWS_SECRET_ACCESS_KEY", "");
   test_framework_setenv ("AWS_SESSION_TOKEN", "");
}

static void
test_derive_region (void *unused)
{
   bson_error_t error;
   char *region;
   bool ret;
   char *large;

#define WITH_LEN(s) s, strlen (s)

   ret = _mongoc_validate_and_derive_region (
      WITH_LEN ("abc..def"), &region, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_AUTHENTICATE,
                          "Invalid STS host: empty part");
   bson_free (region);

   ret = _mongoc_validate_and_derive_region (WITH_LEN ("."), &region, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_AUTHENTICATE,
                          "Invalid STS host: empty part");
   bson_free (region);

   ret = _mongoc_validate_and_derive_region (WITH_LEN ("..."), &region, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_AUTHENTICATE,
                          "Invalid STS host: empty part");
   bson_free (region);

   ret =
      _mongoc_validate_and_derive_region (WITH_LEN ("first."), &region, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_AUTHENTICATE,
                          "Invalid STS host: empty part");
   bson_free (region);

   ret = _mongoc_validate_and_derive_region (
      WITH_LEN ("sts.amazonaws.com"), &region, &error);
   BSON_ASSERT (ret);
   ASSERT_CMPSTR ("us-east-1", region);
   bson_free (region);

   ret = _mongoc_validate_and_derive_region (
      WITH_LEN ("first.second"), &region, &error);
   BSON_ASSERT (ret);
   ASSERT_CMPSTR ("second", region);
   bson_free (region);

   ret =
      _mongoc_validate_and_derive_region (WITH_LEN ("first"), &region, &error);
   BSON_ASSERT (ret);
   ASSERT_CMPSTR ("us-east-1", region);
   bson_free (region);

   ret = _mongoc_validate_and_derive_region (WITH_LEN (""), &region, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_AUTHENTICATE,
                          "Invalid STS host: empty");
   bson_free (region);

   large = bson_malloc0 (257);
   memset (large, 'a', 256);

   ret = _mongoc_validate_and_derive_region (
      large, strlen (large), &region, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_AUTHENTICATE,
                          "Invalid STS host: too large");
   bson_free (region);
   bson_free (large);

#undef WITH_LEN
}

void
test_aws_install (TestSuite *suite)
{
   TestSuite_AddFull (suite,
                      "/aws/obtain_credentials",
                      test_obtain_credentials,
                      NULL /* dtor */,
                      NULL /* ctx */,
                      test_framework_skip_if_no_aws);
   TestSuite_AddFull (suite,
                      "/aws/obtain_credentials_from_env",
                      test_obtain_credentials_from_env,
                      NULL /* dtor */,
                      NULL /* ctx */,
                      test_framework_skip_if_no_aws,
                      test_framework_skip_if_no_setenv);
   TestSuite_AddFull (suite,
                      "/aws/derive_region",
                      test_derive_region,
                      NULL /* dtor */,
                      NULL /* ctx */,
                      test_framework_skip_if_no_aws);
}
