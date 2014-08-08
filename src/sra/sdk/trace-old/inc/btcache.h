#ifndef _h_btcache_
#define _h_btcache_

#ifndef _h_lock_
#include "lock.h"
#endif

#ifndef _h_container_
#include "container.h"
#endif


#ifndef TRACEDB_CACHE_USE_RWLOCK
#define TRACEDB_CACHE_USE_RWLOCK 1
#endif


#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * BTCNode
 *  a node that exists in a b-tree as well as an LRU cache
 */
typedef struct tracedb_BTCNode tracedb_BTCNode;
struct tracedb_BTCNode
{
    tracedb_BTNode bt;
    tracedb_DLNode dl;
};


/*--------------------------------------------------------------------------
 * BTCache
 *  a btree with LRU cache
 */
typedef struct tracedb_BTCache tracedb_BTCache;
struct tracedb_BTCache
{
    tracedb_BTree bt;
    tracedb_DLList lru;
#if TRACEDB_CACHE_USE_RWLOCK
    tracedb_RWTmLock bt_lock;
#endif
    tracedb_TmMutex lru_lock;
    volatile uint32_t count;
};

/* BTCacheInit
 *  initialize an empty cache
 */
int tracedb_BTCacheInit ( tracedb_BTCache *cache );

#if TRACEDB_CACHE_USE_RWLOCK

/* BTCacheReadLock
 *  acquires read lock on cache
 */
#define tracedb_BTCacheReadLock( cache ) \
    tracedb_RWTmLockReadLock ( ( cache ) -> bt_lock )

/* BTCacheWriteLock
 *  acquires write lock on cache
 *  may fail, so test return result
 */
#define tracedb_BTCacheWriteLock( cache ) \
    tracedb_RWTmLockWriteLock ( ( cache ) -> bt_lock )

/* BTCacheTryReadLock
 *  try to acquire a read lock on cache
 */
#define tracedb_BTCacheTryReadLock( cache ) \
    tracedb_RWTmLockTryReadLock ( ( cache ) -> bt_lock )

/* BTCacheTryWriteLock
 *  try to acquire a write lock on cache
 */
#define tracedb_BTCacheTryWriteLock( cache ) \
    tracedb_RWTmLockTryWriteLock ( ( cache ) -> bt_lock )

/* BTCacheTimedReadLock
 *  try to acquire a read lock on cache before the timeout
 *  "tm" is a timeout_t initialized by TimeoutInit
 */
#define tracedb_BTCacheTimedReadLock( cache, tm ) \
    tracedb_RWTmLockTimedReadLock ( ( cache ) -> bt_lock, tm )

/* BTCacheTimedWriteLock
 *  try to acquire a write lock on cache before the timeout
 *  "tm" is a timeout_t initialized by TimeoutInit
 */
#define tracedb_BTCacheTimedWriteLock( cache, tm ) \
    tracedb_RWTmLockTimedWriteLock ( ( cache ) -> bt_lock, tm )

/* BTCacheChangeToWriteLock
 *  given an existing read lock, swap to write lock
 */
#define tracedb_BTCacheChangeToWriteLock( cache ) \
    ( tracedb_RWTmLockUnlock ( ( cache ) -> bt_lock ), \
      tracedb_RWTmLockWriteLock ( ( cache ) -> bt_lock ) )

/* BTCacheTryChangeToWriteLock
 *  given an existing read lock, try to swap to write lock
 *  always releases read lock
 */
#define tracedb_BTCacheTryChangeToWriteLock( cache ) \
    ( tracedb_RWTmLockUnlock ( ( cache ) -> bt_lock ), \
      tracedb_RWTmLockTryWriteLock ( ( cache ) -> bt_lock ) )

/* BTCacheChangeToTimedWriteLock
 *  given an existing read lock, swap to write lock
 */
#define tracedb_BTCacheChangeToTimedWriteLock( cache, tm ) \
    ( tracedb_RWTmLockUnlock ( ( cache ) -> bt_lock ), \
      tracedb_RWTmLockTimedWriteLock ( ( cache ) -> bt_lock, tm ) )

/* BTCacheUnlock
 *  releases lock on cache
 */
#define tracedb_BTCacheUnlock( cache ) \
    tracedb_RWTmLockUnlock ( ( cache ) -> bt_lock )

#else /* CACHE_USE_RWLOCK */

/* BTCacheReadLock
 *  acquires lock on cache
 */
#define tracedb_BTCacheReadLock( cache ) \
    tracedb_TmMutexLock ( ( cache ) -> lru_lock )

/* BTCacheWriteLock
 *  acquires lock on cache
 *  may fail, so test return result
 */
#define tracedb_BTCacheWriteLock( cache ) \
    tracedb_TmMutexLock ( ( cache ) -> lru_lock )

/* BTCacheTryReadLock
 *  try to acquire a lock on cache
 */
#define tracedb_BTCacheTryReadLock( cache ) \
    tracedb_TmMutexTryLock ( ( cache ) -> lru_lock )

/* BTCacheTryWriteLock
 *  try to acquire a lock on cache
 */
#define tracedb_BTCacheTryWriteLock( cache ) \
    tracedb_TmMutexTryLock ( ( cache ) -> lru_lock )

/* BTCacheTimedReadLock
 *  try to acquire a lock on cache before the timeout
 *  "tm" is a timeout_t initialized by TimeoutInit
 */
#define tracedb_BTCacheTimedReadLock( cache, tm ) \
    tracedb_TmMutexTimedLock ( ( cache ) -> lru_lock, tm )

/* BTCacheTimedWriteLock
 *  try to acquire a write lock on cache before the timeout
 *  "tm" is a timeout_t initialized by TimeoutInit
 */
#define tracedb_BTCacheTimedWriteLock( cache, tm ) \
    tracedb_TmMutexTimedLock ( ( cache ) -> lru_lock, tm )

/* BTCacheChangeToWriteLock
 *  a noop
 */
#define tracedb_BTCacheChangeToWriteLock( cache ) \
    0

/* BTCacheTryChangeToWriteLock
 *  a noop
 */
#define tracedb_BTCacheTryChangeToWriteLock( cache ) \
    0

/* BTCacheChangeToTimedWriteLock
 *  a noop
 */
#define tracedb_BTCacheChangeToTimedWriteLock( cache, tm ) \
    0

/* BTCacheUnlock
 *  releases lock on cache
 */
#define tracedb_BTCacheUnlock( cache ) \
    tracedb_TmMutexUnlock ( ( cache ) -> lru_lock )

#endif /* CACHE_USE_RWLOCK */

/* BTCacheFind
 *  finds an object in the tree and updates MRU
 *
 *  assumes cache is read-locked
 */
tracedb_BTCNode *tracedb_BTCacheFind ( tracedb_BTCache *cache,
    const void *item,
    int ( *cmp ) ( const void *item, const tracedb_BTCNode *n ) );

/* BTCacheInsert
 *  sticks an object into the b-tree and LRU cache
 *  according to an external sort function, but does
 *  NOT update the count on success for implementation
 *  atomicity when coordinating multiple caches.
 *
 *  assumes the cache is write locked
 *
 *  returns true if insert succeeded, at which time
 *  the caller should adjust the count manually, since
 *  we're called within a write lock.
 */
bool tracedb_BTCacheInsert ( tracedb_BTCache *cache,
    tracedb_BTCNode *node,
    int ( *sort ) ( const tracedb_BTCNode *item, const tracedb_BTCNode *n ) );

/* BTCacheUnlink
 *  removes a node from cache and updates the count
 *  assumes the cache is write locked
 */
void tracedb_BTCacheUnlink ( tracedb_BTCache *cache, tracedb_BTCNode *node );

/* BTCacheDrop
 *  drops oldest node from the cache and updates count
 *  returns item dropped
 */
tracedb_BTCNode *tracedb_BTCacheDrop ( tracedb_BTCache *cache );

/* BTCacheForEach
 *  executes a function on each element
 */
void tracedb_BTCacheForEach ( const tracedb_BTCache *cache, bool reverse,
    void ( * f ) ( tracedb_BTCNode *n, void *data ), void *data );

/* BTCacheDoUntil
 *  executes a function on each element
 *  until the function returns true
 */
void tracedb_BTCacheDoUntil ( const tracedb_BTCache *cache, bool reverse,
    bool ( * f ) ( tracedb_BTCNode *n, void *data ), void *data );

/* BTCacheWhack
 *  removes nodes from tree and
 *  executes user provided destructor
 *  destroys locks
 */
void tracedb_BTCacheWhack ( tracedb_BTCache *cache,
    void ( * whack ) ( tracedb_BTCNode *n, void *data ), void *data );


#ifdef __cplusplus
}
#endif

#endif /* _h_btcache_ */
