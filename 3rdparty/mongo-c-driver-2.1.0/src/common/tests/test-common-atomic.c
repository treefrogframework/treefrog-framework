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


#include <common-atomic-private.h>

#include <bson/bson.h>

#include <TestSuite.h>

#define ATOMIC(Kind, Operation) BSON_CONCAT4 (mcommon_atomic_, Kind, _, Operation)


#define TEST_KIND_WITH_MEMORDER(Kind, TypeName, MemOrder, Assert)              \
   do {                                                                        \
      int i;                                                                   \
      TypeName got;                                                            \
      TypeName value = 0;                                                      \
      got = ATOMIC (Kind, fetch) (&value, MemOrder);                           \
      Assert (got, ==, 0);                                                     \
      got = ATOMIC (Kind, fetch_add) (&value, 42, MemOrder);                   \
      Assert (got, ==, 0);                                                     \
      Assert (value, ==, 42);                                                  \
      got = ATOMIC (Kind, fetch_sub) (&value, 7, MemOrder);                    \
      Assert (got, ==, 42);                                                    \
      Assert (value, ==, 35);                                                  \
      got = ATOMIC (Kind, exchange) (&value, 77, MemOrder);                    \
      Assert (got, ==, 35);                                                    \
      Assert (value, ==, 77);                                                  \
      /* Compare-exchange fail: */                                             \
      got = ATOMIC (Kind, compare_exchange_strong) (&value, 4, 9, MemOrder);   \
      Assert (got, ==, 77);                                                    \
      Assert (value, ==, 77);                                                  \
      /* Compare-exchange succeed: */                                          \
      got = ATOMIC (Kind, compare_exchange_strong) (&value, 77, 9, MemOrder);  \
      Assert (got, ==, 77);                                                    \
      Assert (value, ==, 9);                                                   \
      /* Compare-exchange fail: */                                             \
      got = ATOMIC (Kind, compare_exchange_weak) (&value, 8, 12, MemOrder);    \
      Assert (got, ==, 9);                                                     \
      Assert (value, ==, 9);                                                   \
      /* Compare-exchange-weak succeed: */                                     \
      /* 'weak' may fail spuriously, so it must *eventually* succeed */        \
      for (i = 0; i < 10000 && value != 53; ++i) {                             \
         got = ATOMIC (Kind, compare_exchange_weak) (&value, 9, 53, MemOrder); \
         Assert (got, ==, 9);                                                  \
      }                                                                        \
      /* Check that it evenutally succeeded */                                 \
      Assert (value, ==, 53);                                                  \
   } while (0)

#define TEST_INTEGER_KIND(Kind, TypeName, Assert)                                     \
   do {                                                                               \
      TEST_KIND_WITH_MEMORDER (Kind, TypeName, mcommon_memory_order_relaxed, Assert); \
      TEST_KIND_WITH_MEMORDER (Kind, TypeName, mcommon_memory_order_acq_rel, Assert); \
      TEST_KIND_WITH_MEMORDER (Kind, TypeName, mcommon_memory_order_acquire, Assert); \
      TEST_KIND_WITH_MEMORDER (Kind, TypeName, mcommon_memory_order_release, Assert); \
      TEST_KIND_WITH_MEMORDER (Kind, TypeName, mcommon_memory_order_consume, Assert); \
      TEST_KIND_WITH_MEMORDER (Kind, TypeName, mcommon_memory_order_seq_cst, Assert); \
   } while (0)


static void
test_integers (void)
{
   TEST_INTEGER_KIND (int64, int64_t, ASSERT_CMPINT64);
   TEST_INTEGER_KIND (int32, int32_t, ASSERT_CMPINT32);
   TEST_INTEGER_KIND (int16, int16_t, ASSERT_CMPINT);
   TEST_INTEGER_KIND (int8, int8_t, ASSERT_CMPINT);
   TEST_INTEGER_KIND (int, int, ASSERT_CMPINT);
}


static void
test_pointers (void)
{
   int u = 12;
   int v = 9;
   int w = 91;
   int *ptr = &v;
   int *other;
   int *prev;
   other = mcommon_atomic_ptr_fetch ((void *) &ptr, mcommon_memory_order_relaxed);
   ASSERT_CMPVOID (other, ==, ptr);
   prev = mcommon_atomic_ptr_exchange ((void *) &other, &u, mcommon_memory_order_relaxed);
   ASSERT_CMPVOID (prev, ==, &v);
   ASSERT_CMPVOID (other, ==, &u);
   prev = mcommon_atomic_ptr_compare_exchange_strong ((void *) &other, &v, &w, mcommon_memory_order_relaxed);
   ASSERT_CMPVOID (prev, ==, &u);
   ASSERT_CMPVOID (other, ==, &u);
   prev = mcommon_atomic_ptr_compare_exchange_strong ((void *) &other, &u, &w, mcommon_memory_order_relaxed);
   ASSERT_CMPVOID (prev, ==, &u);
   ASSERT_CMPVOID (other, ==, &w);
}


static void
test_thread_fence (void)
{
   mcommon_atomic_thread_fence ();
}

static void
test_thrd_yield (void)
{
   mcommon_thrd_yield ();
}

void
test_mcommon_atomic_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/mcommon/atomic/integers", test_integers);
   TestSuite_Add (suite, "/mcommon/atomic/pointers", test_pointers);
   TestSuite_Add (suite, "/mcommon/atomic/thread_fence", test_thread_fence);
   TestSuite_Add (suite, "/mcommon/atomic/thread_yield", test_thrd_yield);
}
