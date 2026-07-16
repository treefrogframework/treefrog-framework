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


#include <mongoc/mongoc-set-private.h>

#include <bson/bson.h>

#include <mlib/cmp.h>

mongoc_set_t *
mongoc_set_new(size_t nitems, mongoc_set_item_dtor dtor, void *dtor_ctx)
{
   mongoc_set_t *set = (mongoc_set_t *)bson_malloc(sizeof(*set));

   set->items_allocated = BSON_MAX(nitems, 1);
   set->items = BSON_ARRAY_ALLOC(set->items_allocated, mongoc_set_item_t);
   set->items_len = 0;

   set->dtor = dtor;
   set->dtor_ctx = dtor_ctx;

   return set;
}

static int
mongoc_set_id_cmp(const void *a_, const void *b_)
{
   mongoc_set_item_t *a = (mongoc_set_item_t *)a_;
   mongoc_set_item_t *b = (mongoc_set_item_t *)b_;

   if (a->id == b->id) {
      return 0;
   }

   return a->id < b->id ? -1 : 1;
}

void
mongoc_set_add(mongoc_set_t *set, uint32_t id, void *item)
{
   if (set->items_len >= set->items_allocated) {
      set->items_allocated *= 2;
      set->items = (mongoc_set_item_t *)bson_realloc(set->items, sizeof(*set->items) * set->items_allocated);
   }

   set->items[set->items_len].id = id;
   set->items[set->items_len].item = item;

   set->items_len++;

   if (set->items_len > 1 && set->items[set->items_len - 2].id > id) {
      qsort(set->items, set->items_len, sizeof(*set->items), mongoc_set_id_cmp);
   }
}

void
mongoc_set_rm(mongoc_set_t *set, uint32_t id)
{
   const mongoc_set_item_t key = {.id = id};

   mongoc_set_item_t *const ptr =
      (mongoc_set_item_t *)bsearch(&key, set->items, set->items_len, sizeof(key), mongoc_set_id_cmp);

   if (ptr) {
      if (set->dtor) {
         set->dtor(ptr->item, set->dtor_ctx);
      }

      const size_t index = (size_t)(ptr - set->items);

      if (index != set->items_len - 1u) {
         memmove(set->items + index, set->items + index + 1u, (set->items_len - (index + 1u)) * sizeof(key));
      }

      set->items_len--;
   }
}

void *
mongoc_set_get(mongoc_set_t *set, uint32_t id)
{
   mongoc_set_item_t *ptr;
   mongoc_set_item_t key;

   key.id = id;

   ptr = (mongoc_set_item_t *)bsearch(&key, set->items, set->items_len, sizeof(key), mongoc_set_id_cmp);

   return ptr ? ptr->item : NULL;
}

void *
mongoc_set_get_item(mongoc_set_t *set, size_t idx)
{
   BSON_ASSERT(set);
   BSON_ASSERT(idx < set->items_len);

   return set->items[idx].item;
}


void *
mongoc_set_get_item_and_id(mongoc_set_t *set, size_t idx, uint32_t *id /* OUT */)
{
   BSON_ASSERT(set);
   BSON_ASSERT(id);
   BSON_ASSERT(idx < set->items_len);

   *id = set->items[idx].id;

   return set->items[idx].item;
}


void
mongoc_set_destroy(mongoc_set_t *set)
{
   if (set->dtor) {
      for (size_t i = 0u; i < set->items_len; i++) {
         set->dtor(set->items[i].item, set->dtor_ctx);
      }
   }

   bson_free(set->items);
   bson_free(set);
}


typedef struct {
   mongoc_set_for_each_cb_t cb;
   void *ctx;
} _mongoc_set_for_each_helper_t;

static bool
_mongoc_set_for_each_helper(uint32_t id, void *item, void *ctx)
{
   _mongoc_set_for_each_helper_t *helper = ctx;

   BSON_UNUSED(id);

   return helper->cb(item, helper->ctx);
}

void
mongoc_set_for_each(mongoc_set_t *set, mongoc_set_for_each_cb_t cb, void *ctx)
{
   _mongoc_set_for_each_helper_t helper = {
      .cb = cb,
      .ctx = ctx,
   };

   mongoc_set_for_each_with_id(set, _mongoc_set_for_each_helper, &helper);
}


typedef struct {
   mongoc_set_for_each_const_cb_t cb;
   void *ctx;
} _mongoc_set_for_each_const_helper_t;

static bool
_mongoc_set_for_each_const_helper(uint32_t id, const void *item, void *ctx)
{
   _mongoc_set_for_each_const_helper_t *helper = ctx;

   BSON_UNUSED(id);

   return helper->cb(item, helper->ctx);
}

void
mongoc_set_for_each_const(const mongoc_set_t *set, mongoc_set_for_each_const_cb_t cb, void *ctx)
{
   _mongoc_set_for_each_const_helper_t helper = {
      .cb = cb,
      .ctx = ctx,
   };

   mongoc_set_for_each_with_id_const(set, _mongoc_set_for_each_const_helper, &helper);
}

void
mongoc_set_for_each_with_id(mongoc_set_t *set, mongoc_set_for_each_with_id_cb_t cb, void *ctx)
{
   BSON_ASSERT_PARAM(set);
   BSON_ASSERT_PARAM(cb);
   BSON_ASSERT(ctx || true);

   BSON_ASSERT(mlib_in_range(uint32_t, set->items_len));
   const uint32_t items_len = (uint32_t)set->items_len;

   /* prevent undefined behavior of memcpy(NULL) */
   if (items_len == 0) {
      return;
   }

   mongoc_set_item_t *const old_set = BSON_ARRAY_ALLOC(items_len, mongoc_set_item_t);
   memcpy(old_set, set->items, sizeof(*old_set) * items_len);

   for (uint32_t i = 0u; i < items_len; i++) {
      void *const item = old_set[i].item;

      if (!cb(i, item, ctx)) {
         break;
      }
   }

   bson_free(old_set);
}

void
mongoc_set_for_each_with_id_const(const mongoc_set_t *set, mongoc_set_for_each_with_id_const_cb_t cb, void *ctx)
{
   BSON_ASSERT_PARAM(set);
   BSON_ASSERT_PARAM(cb);
   BSON_ASSERT(ctx || true);

   BSON_ASSERT(mlib_in_range(uint32_t, set->items_len));
   const uint32_t items_len = (uint32_t)set->items_len;

   /* prevent undefined behavior of memcpy(NULL) */
   if (items_len == 0) {
      return;
   }

   mongoc_set_item_t *const old_set = BSON_ARRAY_ALLOC(items_len, mongoc_set_item_t);
   memcpy(old_set, set->items, sizeof(*old_set) * items_len);

   for (uint32_t i = 0u; i < items_len; i++) {
      const void *const item = old_set[i].item;

      if (!cb(i, item, ctx)) {
         break;
      }
   }

   bson_free(old_set);
}


void *
mongoc_set_find_item(mongoc_set_t *set, mongoc_set_for_each_cb_t cb, void *ctx)
{
   for (size_t i = 0u; i < set->items_len; i++) {
      mongoc_set_item_t *const item = set->items + i;

      if (cb(item->item, ctx)) {
         return item;
      }
   }

   return NULL;
}


uint32_t
mongoc_set_find_id(const mongoc_set_t *set, mongoc_set_for_each_const_cb_t cb, void *ctx)
{
   for (size_t i = 0u; i < set->items_len; i++) {
      const mongoc_set_item_t *const item = set->items + i;

      if (cb(item->item, ctx)) {
         return item->id;
      }
   }

   return 0;
}
