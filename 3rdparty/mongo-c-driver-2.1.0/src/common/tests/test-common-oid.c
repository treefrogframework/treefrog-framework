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

#include <common-oid-private.h>

#include <TestSuite.h>

static void
test_mcommon_oid_zero (void)
{
   bson_oid_t oid;
   bson_oid_init_from_string (&oid, "000000000000000000000000");
   BSON_ASSERT (true == bson_oid_equal (&oid, &kZeroObjectId));
   BSON_ASSERT (true == mcommon_oid_is_zero (&oid));
   bson_oid_init_from_string (&oid, "010000000000000000000000");
   BSON_ASSERT (false == mcommon_oid_is_zero (&oid));
   bson_oid_init_from_string (&oid, "000000000000000000000001");
   BSON_ASSERT (false == mcommon_oid_is_zero (&oid));
   bson_oid_init_from_string (&oid, "ffffffffffffffffffffffff");
   BSON_ASSERT (false == mcommon_oid_is_zero (&oid));
   mcommon_oid_set_zero (&oid);
   BSON_ASSERT (true == mcommon_oid_is_zero (&oid));
}

void
test_mcommon_oid_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/mcommon/oid/zero", test_mcommon_oid_zero);
}
