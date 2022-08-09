#include "mongoc-ts-pool-private.h"
#include "common-thread-private.h"

#include "bson/bson.h"

#if defined(_MSC_VER)
typedef double _max_align_type;
#elif defined(__APPLE__)
typedef long double _max_align_type;
#else
/* Based on the GCC max_align_t definition */
typedef struct _max_align_type {
   long long maxalign_1 __attribute__ ((__aligned__ (__alignof__(long long))));
   long double maxalign_2
      __attribute__ ((__aligned__ (__alignof__(long double))));
} _max_align_type;
#endif

/**
 * Toggle this to enable/disable checks that all items are returned to the pool
 * before the pool is destroyed
 */
enum { AUDIT_POOL_ENABLED = 0 };

typedef struct pool_node {
   struct pool_node *next;
   mongoc_ts_pool *owner_pool;

   _max_align_type data[];
} pool_node;

struct mongoc_ts_pool {
   mongoc_ts_pool_params params;
   pool_node *head;
   /* Number of elements in the pool */
   int32_t size;
   bson_mutex_t mtx;
   /* Number of elements that the pool has given to users.
    * If AUDIT_POOL_ENABLED is zero, this member is unused */
   int32_t outstanding_items;
};

/**
 * @brief Check whether we should drop the given node from the pool
 */
static bool
_should_prune (const pool_node *node)
{
   mongoc_ts_pool *pool = node->owner_pool;
   return pool->params.prune_predicate &&
          pool->params.prune_predicate (node->data, pool->params.userdata);
}

/**
 * @brief Create a new pool node and contained element.
 *
 * @return pool_node* A pointer to a constructed element, or NULL if the
 * constructor sets `error`
 */
static pool_node *
_new_item (mongoc_ts_pool *pool, bson_error_t *error)
{
   pool_node *node =
      bson_malloc0 (sizeof (pool_node) + pool->params.element_size);
   node->owner_pool = pool;
   if (pool->params.constructor) {
      /* To construct, we need to know if that constructor fails */
      bson_error_t my_error;
      if (!error) {
         /* Caller doesn't care about the error, but we care in case the
          * constructor might fail */
         error = &my_error;
      }
      /* Clear the error */
      error->code = 0;
      error->domain = 0;
      error->message[0] = 0;
      /* Construct the object */
      pool->params.constructor (node->data, pool->params.userdata, error);
      if (error->code != 0) {
         /* Constructor reported an error. Deallocate and drop the node. */
         bson_free (node);
         node = NULL;
      }
   }
   if (node && AUDIT_POOL_ENABLED) {
      bson_atomic_int32_fetch_add (
         &pool->outstanding_items, 1, bson_memory_order_relaxed);
   }
   return node;
}

/**
 * @brief Destroy the given node and the element that it contains
 */
static void
_delete_item (pool_node *node)
{
   mongoc_ts_pool *pool = node->owner_pool;
   if (pool->params.destructor) {
      pool->params.destructor (node->data, pool->params.userdata);
   }
   bson_free (node);
}

/**
 * @brief Try to take a node from the pool. Returns `NULL` if the pool is empty.
 */
static pool_node *
_try_get (mongoc_ts_pool *pool)
{
   pool_node *node;
   bson_mutex_lock (&pool->mtx);
   node = pool->head;
   if (node) {
      pool->head = node->next;
   }
   bson_mutex_unlock (&pool->mtx);
   if (node) {
      bson_atomic_int32_fetch_sub (&pool->size, 1, bson_memory_order_relaxed);
      if (AUDIT_POOL_ENABLED) {
         bson_atomic_int32_fetch_add (
            &pool->outstanding_items, 1, bson_memory_order_relaxed);
      }
   }
   return node;
}

mongoc_ts_pool *
mongoc_ts_pool_new (mongoc_ts_pool_params params)
{
   mongoc_ts_pool *r = bson_malloc0 (sizeof (mongoc_ts_pool));
   r->params = params;
   r->head = NULL;
   r->size = 0;
   if (AUDIT_POOL_ENABLED) {
      r->outstanding_items = 0;
   }
   bson_mutex_init (&r->mtx);
   return r;
}

void
mongoc_ts_pool_free (mongoc_ts_pool *pool)
{
   if (AUDIT_POOL_ENABLED) {
      BSON_ASSERT (
         pool->outstanding_items == 0 &&
         "Pool was destroyed while there are still items checked out");
   }
   mongoc_ts_pool_clear (pool);
   bson_mutex_destroy (&pool->mtx);
   bson_free (pool);
}

void
mongoc_ts_pool_clear (mongoc_ts_pool *pool)
{
   pool_node *node;
   {
      bson_mutex_lock (&pool->mtx);
      node = pool->head;
      pool->head = NULL;
      pool->size = 0;
      bson_mutex_unlock (&pool->mtx);
   }
   while (node) {
      pool_node *n = node;
      node = n->next;
      _delete_item (n);
   }
}

void *
mongoc_ts_pool_get_existing (mongoc_ts_pool *pool)
{
   pool_node *node;
retry:
   node = _try_get (pool);
   if (node && _should_prune (node)) {
      /* This node should be pruned now. Drop it and try again. */
      mongoc_ts_pool_drop (node->data);
      goto retry;
   }
   return node ? node->data : NULL;
}

void *
mongoc_ts_pool_get (mongoc_ts_pool *pool, bson_error_t *error)
{
   pool_node *node;
retry:
   node = _try_get (pool);
   if (node && _should_prune (node)) {
      /* This node should be pruned now. Drop it and try again. */
      mongoc_ts_pool_drop (node->data);
      goto retry;
   }
   if (node == NULL) {
      /* We need a new item */
      node = _new_item (pool, error);
   }
   if (node == NULL) {
      /* No item in pool, and we couldn't create one either */
      return NULL;
   }
   return node->data;
}

void
mongoc_ts_pool_return (void *item)
{
   pool_node *node = (void *) ((uint8_t *) (item) -offsetof (pool_node, data));
   if (_should_prune (node)) {
      mongoc_ts_pool_drop (item);
   } else {
      mongoc_ts_pool *pool = node->owner_pool;
      bson_mutex_lock (&pool->mtx);
      node->next = pool->head;
      pool->head = node;
      bson_mutex_unlock (&pool->mtx);
      bson_atomic_int32_fetch_add (
         &node->owner_pool->size, 1, bson_memory_order_relaxed);
      if (AUDIT_POOL_ENABLED) {
         bson_atomic_int32_fetch_sub (
            &node->owner_pool->outstanding_items, 1, bson_memory_order_relaxed);
      }
   }
}

void
mongoc_ts_pool_drop (void *item)
{
   pool_node *node = (void *) ((uint8_t *) (item) -offsetof (pool_node, data));
   if (AUDIT_POOL_ENABLED) {
      bson_atomic_int32_fetch_sub (
         &node->owner_pool->outstanding_items, 1, bson_memory_order_relaxed);
   }
   _delete_item (node);
}

int
mongoc_ts_pool_is_empty (const mongoc_ts_pool *pool)
{
   return mongoc_ts_pool_size (pool) == 0;
}

size_t
mongoc_ts_pool_size (const mongoc_ts_pool *pool)
{
   return bson_atomic_int32_fetch (&pool->size, bson_memory_order_relaxed);
}

void
mongoc_ts_pool_visit_each (mongoc_ts_pool *pool,
                           void *visit_userdata,
                           int (*visit) (void *item,
                                         void *pool_userdata,
                                         void *visit_userdata))
{
   /* Pointer to the pointer that must be updated in case of an item pruning */
   pool_node **node_ptrptr;
   /* The node we are looking at */
   pool_node *node;
   bson_mutex_lock (&pool->mtx);
   node_ptrptr = &pool->head;
   node = pool->head;
   while (node) {
      const bool should_remove =
         visit (node->data, pool->params.userdata, visit_userdata);
      pool_node *const next_node = node->next;
      if (!should_remove) {
         node_ptrptr = &node->next;
         node = next_node;
         continue;
      }
      /* Retarget the previous pointer to the next node in line */
      *node_ptrptr = node->next;
      _delete_item (node);
      pool->size--;
      /* Leave node_ptrptr pointing to the previous pointer, because we may
       * need to erase another item */
      node = next_node;
   }
   bson_mutex_unlock (&pool->mtx);
}
