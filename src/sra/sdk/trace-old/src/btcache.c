#include "btcache.h"

#include <assert.h>
#include <errno.h>


/*--------------------------------------------------------------------------
 * BTCache
 *  a btree with LRU cache
 */

/* BTCacheInit
 *  initialize an empty cache
 */
int tracedb_BTCacheInit ( tracedb_BTCache *cache )
{
    int status;

    if ( cache == NULL )
        return EINVAL;

    tracedb_BTreeInit ( & cache -> bt );
    tracedb_DLListInit ( & cache -> lru );
    cache -> count = 0;

    status = tracedb_MutexInit ( & cache -> lru_lock );
#if TRACEDB_CACHE_USE_RWLOCK
    if ( status == 0 )
    {
        status = tracedb_RWLockInit ( & cache -> bt_lock );
        if ( status != 0 )
            tracedb_MutexWhack ( cache -> lru_lock );
    }
#endif
    return status;
}

/* BTCacheFind
 *  finds an object in the tree and updates MRU
 *
 *  assumes cache is read-locked
 */
tracedb_BTCNode *tracedb_BTCacheFind (
    tracedb_BTCache *cache, const void *item,
    int ( *cmp ) ( const void *item, const tracedb_BTCNode *n ) )
{
    tracedb_BTCNode *node;

    /* validate cache pointer */
    if ( cache == NULL )
        return NULL;

    /* try to find the item */
    node = ( tracedb_BTCNode* ) tracedb_BTreeFind ( & cache -> bt, item,
        ( int ( * ) ( const void*, const tracedb_BTNode* ) ) cmp );
    if ( node != NULL && cache -> lru . head != & node -> dl )
    {
#if TRACEDB_CACHE_USE_RWLOCK
        /* don't modify LRU list with only a read lock */
        int status = tracedb_MutexLock ( cache -> lru_lock );
        if ( status == 0 )
        {
#endif
            /* unhook from present location in cache */
            tracedb_DLListUnlink ( & cache -> lru, & node -> dl );

            /* make most recently used */
            tracedb_DLListPushHead ( & cache -> lru, & node -> dl );

#if TRACEDB_CACHE_USE_RWLOCK
            /* release lock */
            tracedb_MutexUnlock ( cache -> lru_lock );
        }
#endif
    }

    return node;
}

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
bool tracedb_BTCacheInsert (
    tracedb_BTCache *cache, tracedb_BTCNode *node,
    int ( *sort ) ( const tracedb_BTCNode *item, const tracedb_BTCNode *n ) )
{
    /* validate pointers */
    if ( cache == NULL || node == NULL )
        return false;

    /* insert item into b-tree */
    if ( tracedb_BTreeInsertUnique ( & cache -> bt, & node -> bt, NULL,
        ( int ( * ) ( const tracedb_BTNode*, const tracedb_BTNode* ) ) sort ) != 0 )
        return false;

    /* make most recently used */
    tracedb_DLListPushHead ( & cache -> lru, & node -> dl );
    return true;
}

/* BTCacheUnlink
 *  removes a node from cache and updates the count
 *  assumes the cache is write locked
 */
void tracedb_BTCacheUnlink ( tracedb_BTCache *cache, tracedb_BTCNode *node )
{
    if ( cache != NULL && node != NULL )
    {
        tracedb_BTreeUnlink ( & cache -> bt, & node -> bt );
        tracedb_DLListUnlink ( & cache -> lru, & node -> dl );
        assert ( cache -> count > 0 );
        -- cache -> count;
    }
}

/* BTCacheDrop
 *  drops oldest node from the cache and updates count
 *  returns item dropped
 */
tracedb_BTCNode *tracedb_BTCacheDrop ( tracedb_BTCache *cache )
{
    tracedb_DLNode *dl;
    tracedb_BTCNode *node;

    if ( cache == NULL )
        return NULL;

    /* look for anything in the list */
    dl = tracedb_DLListPopTail ( & cache -> lru );
    if ( dl == NULL )
        return NULL;

    /* discover beginning of node object */
    node = ( tracedb_BTCNode* ) ( ( char* ) dl - ( char* ) & ( ( tracedb_BTCNode* ) NULL ) -> dl );

    /* drop from the b-tree and decrement count */
    tracedb_BTreeUnlink ( & cache -> bt, & node -> bt );
    assert ( cache -> count > 0 );
    -- cache -> count;

    return node;
}

/* BTCacheForEach
 *  executes a function on each element
 */
void tracedb_BTCacheForEach ( const tracedb_BTCache *cache, bool reverse,
    void ( * f ) ( tracedb_BTCNode *n, void *data ), void *data )
{
    if ( cache != NULL )
    {
        tracedb_BTreeForEach ( & cache -> bt, reverse,
            ( void ( * ) ( tracedb_BTNode*, void* ) ) f, data );
    }
}

/* BTCacheForEachLRU
 *  executes a function on each element in LRU list
 */
void tracedb_BTCacheForEachLRU ( const tracedb_BTCache *cache, bool reverse,
    void ( * f ) ( tracedb_BTCNode *n, void *data ), void *data )
{
    if ( cache != NULL && f != NULL )
    {
        tracedb_DLNode *dl;
        tracedb_BTCNode *node;

        if ( reverse )
        {
            dl = tracedb_DLListTail ( & cache -> lru );
            while ( dl != NULL )
            {
                node = ( tracedb_BTCNode* )
                    ( ( char* ) dl - ( char* ) & ( ( tracedb_BTCNode* ) NULL ) -> dl );

                ( * f ) ( node, data );

                dl = tracedb_DLNodePrev ( dl );
            }
        }
        else
        {
            dl = tracedb_DLListHead ( & cache -> lru );
            while ( dl != NULL )
            {
                node = ( tracedb_BTCNode* )
                    ( ( char* ) dl - ( char* ) & ( ( tracedb_BTCNode* ) NULL ) -> dl );

                ( * f ) ( node, data );

                dl = tracedb_DLNodeNext ( dl );
            }
        }
    }
}

/* BTCacheDoUntil
 *  executes a function on each element
 *  until the function returns true
 */
void tracedb_BTCacheDoUntil ( const tracedb_BTCache *cache, bool reverse,
    bool ( * f ) ( tracedb_BTCNode *n, void *data ), void *data )
{
    if ( cache != NULL )
    {
        tracedb_BTreeDoUntil ( & cache -> bt, reverse,
            ( bool ( * ) ( tracedb_BTNode*, void* ) ) f, data );
    }
}

/* BTCacheWhack
 *  removes nodes from tree and
 *  executes user provided destructor
 *  destroys locks
 */
void tracedb_BTCacheWhack ( tracedb_BTCache *cache,
    void ( * whack ) ( tracedb_BTCNode *n, void *data ), void *data )
{
    if ( cache != NULL )
    {
        tracedb_DLListInit ( & cache -> lru );
        tracedb_BTreeWhack ( & cache -> bt, 
            ( void ( * ) ( tracedb_BTNode*, void* ) ) whack, data );
        cache -> count = 0;
#if TRACEDB_CACHE_USE_RWLOCK
        tracedb_RWLockWhack ( cache -> bt_lock );
#endif
        tracedb_MutexWhack ( cache -> lru_lock );
    }
}
