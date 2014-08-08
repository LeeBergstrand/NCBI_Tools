#include "tracedb-priv.h"
#include "text.h"
#include "fdio.h"
#include "mmap.h"
#include "mmpool.h"
#include "trace_atomic32.h"
#include "logging.h"
#include "fmtdef.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#define CATCH_EMFILE_ERR 1

/*--------------------------------------------------------------------------
 * TraceDBPath
 *  represents server/database by name
 */
typedef struct TraceDBPath TraceDBPath;
struct TraceDBPath
{
    /* kept in a b-tree */
    tracedb_BTNode n;

    /* id range within database */
    ti_t ti_start, ti_stop;

    /* path size in bytes */
    unsigned int len;

    /* path hash value */
    unsigned int hash;

    /* null-terminated path */
    char path [ 4 ];
};

typedef struct TraceDBPathData TraceDBPathData;
struct TraceDBPathData
{
    const char *path;
    unsigned int len;
    unsigned int hash;
};

static
void TraceDBPathWhack ( tracedb_BTNode *n, void *ignore )
{
    free ( n );
}

static
int TraceDBPathFind ( const void *left, const tracedb_BTNode *right )
{
#define a ( ( const TraceDBPathData* ) left )
#define b ( ( const TraceDBPath* ) right )

    if ( a -> hash != b -> hash )
    {
        if ( a -> hash < b -> hash )
            return -1;
        return 1;
    }

    if ( a -> len != b -> len )
    {
        return ( int ) a -> len - ( int ) b -> len;
    }

    return memcmp ( a -> path, b -> path, a -> len );

#undef a
#undef b
}

static
int TraceDBPathSort ( const tracedb_BTNode *left,
    const tracedb_BTNode *right )
{
#define a ( ( const TraceDBPath* ) left )
#define b ( ( const TraceDBPath* ) right )

    if ( a -> hash != b -> hash )
    {
        if ( a -> hash < b -> hash )
            return -1;
        return 1;
    }

    if ( a -> len != b -> len )
    {
        return ( int ) a -> len - ( int ) b -> len;
    }

    return memcmp ( a -> path, b -> path, a -> len );

#undef a
#undef b
}


/*--------------------------------------------------------------------------
 * TraceDBFile
 *  a file within a database directory
 */
typedef struct TraceDBFile TraceDBFile;
struct TraceDBFile
{
    /* kept in a cache */
    tracedb_BTCNode lru;

    /* belongs to a database */
    TraceDBPath *db;

    /* has an open file */
    tracedb_File f;

    /* is reference counted */
    tracedb_atomic32_t refcount;

    /* has a 1-based file id where id 0
       is used to mean the index file */
    unsigned int fileno;
};

static
void TraceDBFileWhack ( tracedb_BTCNode *n, void *data )
{
    TraceDBFile *b = ( TraceDBFile* ) n;
    TraceDB *db = data;
    tracedb_FileWhack ( b -> f );
    tracedb_SemaphoreSignal ( db -> dsem );
    free ( b );
}

#define TraceDBFileRelease( file, db ) \
    ( tracedb_atomic32_dec_and_test ( & ( file ) -> refcount ) ? \
      TraceDBFileWhack ( & ( file ) -> lru, db ) : ( void ) 0 )

static
int TraceDBFileMake ( TraceDBFile **bp, TraceDBPath *loc, unsigned int fileno )
{
    int status;
    TraceDBFile *b = malloc ( sizeof * b );
    if ( b == NULL )
        status = errno;
    else
    {
        size_t len;
        char fname [ PATH_MAX ];

        b -> db = loc;
        b -> fileno = fileno;

        if ( fileno == 0 )
        {
            /* fileno 0 is used for index file */
#ifdef TRACE_IDX
            status = tracedb_string_printf ( fname, sizeof fname, & len, "%s/" TRACE_IDX, loc -> path );
#else
            status = tracedb_string_printf ( fname, sizeof fname, & len, "%s/idx%d", loc -> path, TRACEDB_VERSION );
#endif
            if ( status != 0 )
            {
                if ( status == ENOBUFS )
                    return ENAMETOOLONG;
                return status;
            }
        }
        else
        {
            char *name;
            unsigned int i;

            /* will only be using base 32, but the full alphabet looks nice */
            const char num [] = "0123456789abcdefghijklmnopqrstuvwxyz";

            /* copy path to db, account for fileno size */
            status = tracedb_string_printf ( fname, sizeof fname - sizeof "00000/0000000" - 1,
                                     & len, "%s/", loc -> path );
            if ( status != 0 )
            {
                if ( status == ENOBUFS )
                    return ENAMETOOLONG;
                return status;
            }

            /* now work on the last portion of the string */
            name = & fname [ len ];

            /* generate last two digits for file */
            for ( -- fileno, i = 12; i >= 11; fileno >>= 5, -- i )
                name [ i ] = num [ fileno & 0x1F ];

            /* generate first five digits for both dir and file */
            for ( ; i >= 6; fileno >>= 5, -- i )
                name [ i ] = name [ i - 6 ] = num [ fileno & 0x1F ];

            /* finish off the string to a true path */
            name [ 5 ] = '/';
            name [ 13 ] = 0;
        }

        /* open the indicated file */
        status = tracedb_FileOpen ( & b -> f, fname, tracedb_permRead );
        if ( status == 0 )
        {
            tracedb_atomic32_set ( & b -> refcount, 1 );
            * bp = b;
            return 0;
        }

        free ( b );
    }
    return status;
}

typedef struct TraceDBFileData TraceDBFileData;
struct TraceDBFileData
{
    TraceDBPath *db;
    unsigned int fileno;
};

static
int TraceDBFileFind ( const void *left, const tracedb_BTCNode *right )
{
#define a ( ( const TraceDBFileData* ) left )
#define b ( ( const TraceDBFile* ) right )

    if ( a -> fileno != b -> fileno )
    {
        if ( a -> fileno < b -> fileno )
            return -1;
        return 1;
    }
    return ( int ) ( a -> db - b -> db );

#undef a
#undef b
}

static
int TraceDBFileSort ( const tracedb_BTCNode *left, 
                      const tracedb_BTCNode *right )
{
#define a ( ( const TraceDBFile* ) left )
#define b ( ( const TraceDBFile* ) right )

    if ( a -> fileno != b -> fileno )
    {
        if ( a -> fileno < b -> fileno )
            return -1;
        return 1;
    }
    return ( int ) ( a -> db - b -> db );

#undef a
#undef b
}


/*--------------------------------------------------------------------------
 * TraceDBIdx
 *  a cached index on a limited number of ids
 */
typedef struct TraceDBIdx TraceDBIdx;
struct TraceDBIdx
{
    /* kept in a cache */
    tracedb_BTCNode lru;

    /* belongs to a database */
    TraceDBPath *db;

    /* represents this range */
    ti_t ti_start, ti_stop;

    /* validity of the block */
    int valid;

    /* is reference counted */
    tracedb_atomic32_t refcount;

    /* here are the indices */
    TraceIdx idx [ TRACEDB_IDXSIZE ];
};

static
void TraceDBIdxWhack ( tracedb_BTCNode *n, void *data )
{
    TraceDBIdx *b = ( TraceDBIdx* ) n;
    TraceDB *db = data;
    tracedb_SemaphoreSignal ( db -> isem );
    free ( b );
}

#define TraceDBIdxRelease( idx, db ) \
    ( tracedb_atomic32_dec_and_test ( & ( idx ) -> refcount ) ? \
      TraceDBIdxWhack ( & ( idx ) -> lru, db ) : ( void ) 0 )

typedef struct TraceDBIdxData TraceDBIdxData;
struct TraceDBIdxData
{
    ti_t id;
};

static
int TraceDBIdxFind ( const void *left, const tracedb_BTCNode *right )
{
#define a ( ( const TraceDBIdxData* ) left )
#define b ( ( const TraceDBIdx* ) right )

    /* the left side is always valid
       and valid is always > invalid */
    if ( ! b -> valid )
        return 1;
    if ( a -> id < b -> ti_start )
        return -1;
    return ( int ) ( a -> id > b -> ti_stop );

#undef a
#undef b
}

static
int TraceDBIdxSort ( const tracedb_BTCNode *left,
                     const tracedb_BTCNode *right )
{
#define a ( ( const TraceDBIdx* ) left )
#define b ( ( const TraceDBIdx* ) right )

    /* valid is > invalid */
    if ( a -> valid != b -> valid )
        return a -> valid - b -> valid;
    if ( a -> ti_start < b -> ti_start )
        return -1;
    if ( a -> ti_stop > b -> ti_stop || a -> ti_start > b -> ti_start )
        return 1;
    if ( a -> ti_stop < b -> ti_stop )
        return -1;
    return ( int ) ( a -> db - b -> db );

#undef a
#undef b
}


/*--------------------------------------------------------------------------
 * TraceDB
 */

/* TraceDBInit
 *  initializes db resources with limits
 *
 *  "hard_file_limit" sets the maximum number of open files
 *  under this db.
 *
 *  "hard_vm_limit" sets the maximum virtual address space consumption
 *  under this db
 *
 *  "db" is initialized with a default timeout for multi-threaded
 *  applications. in single-threaded apps, the timeout should be
 *  reset to 0 ( NB: setting a negative timeout should only be done
 *  while debugging ).
 */
int TraceDBInit ( TraceDB *db, unsigned int hard_file_limit, size_t hard_vm_limit )
{
    int status;

    if ( db == NULL )
        return EINVAL;

    /* interpret limit values of zero as default */
    if ( hard_file_limit == 0 )
        hard_file_limit = 350;
    if ( hard_vm_limit == 0 )
        hard_vm_limit = 1200 * 1024 * 1024;

    /* refuse to allow a terribly low limit */
    else if ( hard_vm_limit < sizeof ( TraceDBIdx ) )
        hard_vm_limit = sizeof ( TraceDBIdx );

    /* assign hard limits to soft-limits */
    db -> imax = ( unsigned int ) ( hard_vm_limit / sizeof ( TraceDBIdx ) );
    db -> dmax = hard_file_limit;

    /* initialize members that can fail */
    status = tracedb_BTCacheInit ( & db -> icache );
    if ( status == 0 )
    {
        status = tracedb_BTCacheInit ( & db -> dcache );
        if ( status == 0 )
        {
            status = tracedb_SemaphoreInit ( & db -> isem, db -> imax );
            if ( status == 0 )
            {
                status = tracedb_SemaphoreInit ( & db -> dsem, db -> dmax );
                if ( status == 0 )
                {
                    status = tracedb_SemaphoreInit ( & db -> gsem, 1 );
                    if ( status == 0 )
                    {
                        status = tracedb_ConditionInit ( & db -> gcond );
                        if ( status == 0 )
                        {
                            /* everything from here is fail-safe */
                            tracedb_BTreeInit ( & db -> paths );
                            db -> ilim = db -> imax;
                            db -> dlim = db -> dmax;
                            db -> timeout = 3 * 60 * 1000;
                            db -> verbose = 0;
                            db -> quitting = false;
                            
                            /* success */
                            return 0;
                        }

                        tracedb_SemaphoreWhack ( db -> gsem );
                    }

                    tracedb_SemaphoreWhack ( db -> dsem );
                }

                tracedb_SemaphoreWhack ( db -> isem );
            }

            tracedb_BTCacheWhack ( & db -> dcache, TraceDBFileWhack, db );
        }

        tracedb_BTCacheWhack ( & db -> icache, TraceDBIdxWhack, db );
    }

    return status;
}

/* TraceDBGet/SetSoftFileLimit
 *  accessors for the data file cache limits
 *
 *  setting the new file limit lower than the number of cached files
 *  will trigger a bg thread gc pass but will not flush the cache
 *  immediately.
 */
int TraceDBGetSoftFileLimit ( const TraceDB *db, unsigned int *limit )
{
    if ( db == NULL || limit == NULL )
        return EINVAL;

    * limit = db -> dlim;
    return 0;
}

int TraceDBSetSoftFileLimit ( TraceDB *db, unsigned int limit )
{
    int status;

    if ( db == NULL )
        return EINVAL;

    status = tracedb_BTCacheTryWriteLock ( & db -> dcache );
    if ( status == EBUSY )
    {
        tracedb_timeout_t tm;
        tracedb_TimeoutInit ( & tm, db -> timeout, true );
        status = tracedb_BTCacheTimedWriteLock ( & db -> dcache, tm );
        tracedb_TimeoutWhack ( tm );
    }
    if ( status == 0 )
    {
        db -> dlim = limit;

        if ( db -> dcache . count > limit )
        {
            status = tracedb_ConditionLock ( db -> gcond );
            if ( status == 0 )
            {
                status = tracedb_ConditionSignal ( db -> gcond );
                tracedb_ConditionUnlock ( db -> gcond );
            }
        }

        tracedb_BTCacheUnlock ( & db -> dcache );
    }
    return status;
}

/* TraceDBGet/SetSoftMemLimit
 *  accessors for the vm cache limits
 *
 *  setting the new vm limit lower than the total of cached indices
 *  will trigger a bg thread gc pass but will not flush the cache
 *  immediately.
 */
int TraceDBGetSoftMemLimit ( const TraceDB *db, size_t *vm_limit )
{
    if ( db == NULL || vm_limit == NULL )
        return EINVAL;

    * vm_limit = db -> ilim * sizeof ( TraceDBIdx );
    return 0;
}

int TraceDBSetSoftMemLimit ( TraceDB *db, size_t vm_limit )
{
    int status;
    unsigned int ilim;

    if ( db == NULL )
        return EINVAL;

    ilim = ( unsigned int ) ( vm_limit / sizeof ( TraceDBIdx ) );

    status = tracedb_BTCacheTryWriteLock ( & db -> icache );
    if ( status == EBUSY )
    {
        tracedb_timeout_t tm;
        tracedb_TimeoutInit ( & tm, db -> timeout, true );
        status = tracedb_BTCacheTimedWriteLock ( & db -> icache, tm );
        tracedb_TimeoutWhack ( tm );
    }
    if ( status == 0 )
    {
        db -> ilim = ilim;

        if ( db -> icache . count > ilim )
        {
            status = tracedb_ConditionLock ( db -> gcond );
            if ( status == 0 )
            {
                status = tracedb_ConditionSignal ( db -> gcond );
                tracedb_ConditionUnlock ( db -> gcond );
            }
        }
        
        tracedb_BTCacheUnlock ( & db -> icache );
    }

    return status;
}


/* TraceDBWhack
 *  releases all resources
 */
int TraceDBWhack ( TraceDB *db, int gc_timeout )
{
    if ( db != NULL )
    {
        int status;
        tracedb_timeout_t tm;

        db -> quitting = true;

        /* if there's a GC thread running, wait for its exit */
        tracedb_TimeoutInit ( & tm, gc_timeout, false );
        status = tracedb_SemaphoreTimedWait ( db -> gsem, tm );
        tracedb_TimeoutWhack ( tm );
        if ( status != 0 )
            return status;

        /* shut down caches */
        tracedb_BTCacheWhack ( & db -> dcache, TraceDBFileWhack, db );
        tracedb_BTCacheWhack ( & db -> icache, TraceDBIdxWhack, db );

        /* forget paths */
        tracedb_BTreeWhack ( & db -> paths, TraceDBPathWhack, NULL );

        /* whack synchronization primitives */
        tracedb_SemaphoreWhack ( db -> isem );
        tracedb_SemaphoreWhack ( db -> dsem );
        tracedb_SemaphoreWhack ( db -> gsem );

        tracedb_ConditionWhack ( db -> gcond );
    }
    return 0;
}

/* TraceDBRunGCTasks
 *  runs garbage collection tasks every "mS" milliseconds
 *  until the process exists or TraceDBWhack is called on the db
 *
 *  NB - this is a blocking function and must be run on its own thread
 */
static
bool WhackInvalidIndices ( tracedb_BTNode *n, void *data )
{
#define idx ( ( TraceDBIdx* ) n )
    TraceDB *db = data;

    if ( idx -> valid != 0 )
        return true;

    TRACEDB_VERBOSE ( 2, db, ( "whacking index block %" TRACEDB_LU64
        " through %" TRACEDB_LU64 " with path '%s'\n",
        idx -> ti_start, idx -> ti_stop, idx -> db -> path ) );

    tracedb_BTCacheUnlink ( & db -> icache, & idx -> lru );
    TraceDBIdxRelease ( idx, db );
    return false;

#undef idx
}

static
bool WhackInvalidPaths ( tracedb_BTNode *n, void *data )
{
#define loc ( ( TraceDBPath* ) n )
    TraceDB *db = data;

    if ( loc -> hash != 0 )
        return true;

    TRACEDB_VERBOSE ( 2, db, ( "whacking invalid path '%s'\n", loc -> path ) );

    tracedb_BTreeUnlink ( & db -> paths, n );
    free ( n );
    return false;

#undef loc
}

int TraceDBRunGCTasks ( TraceDB *db, int mS )
{
    int status;

    if ( db == NULL )
        return EINVAL;

    status = tracedb_SemaphoreTry ( db -> gsem );
    if ( status == 0 )
    {
        /* acquire mutex outside of the loop */
        status = tracedb_ConditionLock ( db -> gcond );
        if ( status == 0 )
        {
            /* we're in the loop */
            while ( ! db -> quitting )
            {
                tracedb_timeout_t tm;

                /* sleep on the condition */
                tracedb_TimeoutInit ( & tm, mS, true );
                status = tracedb_ConditionTimedWait ( db -> gcond, tm );
                tracedb_TimeoutWhack ( tm );

                /* detect error condition */
                if ( status != 0 && status != ETIMEDOUT )
                    break;

                /* free up memory */
                status = tracedb_BTCacheTryWriteLock ( & db -> icache );
                if ( status != 0 )
                {
                    tracedb_TimeoutInit ( & tm, db -> timeout, true );
                    status = tracedb_BTCacheTimedWriteLock ( & db -> icache, tm );
                    tracedb_TimeoutWhack ( tm );
                }
                if ( status == 0 )
                {
                    TRACEDB_VERBOSE ( 5, db, ( "GC task - inside write lock on icache\n" ) );

                    /* whack any invalid indices
                       use "do until" because indices are
                       sorted according to valid state, and
                       an invalid index will be left-most.
                       so run until a valid one is found */
                    tracedb_BTreeDoUntil ( & db -> icache . bt, false,
                        WhackInvalidIndices, db );

                    /* whack any invalid paths
                       use "do until" because paths are
                       sorted according to hash code, and
                       an invalid path will be left-most.
                       so run until a valid one is found */
                    tracedb_BTreeDoUntil ( & db -> paths, false,
                        WhackInvalidPaths, db );

                    /* drop excess indices from cache */
                    while ( db -> icache . count > db -> ilim )
                    {
                        TraceDBIdx *idx = ( TraceDBIdx* )
                            tracedb_BTCacheDrop ( & db -> icache );
                        assert ( idx != NULL );
                        TraceDBIdxRelease ( idx, db );
                    }

                    TRACEDB_VERBOSE ( 5, db, ( "GC task - releasing write lock on icache\n" ) );
                    tracedb_BTCacheUnlock ( & db -> icache );
                }

                /* free up file descriptors */
                status = tracedb_BTCacheTryWriteLock ( & db -> dcache );
                if ( status != 0 )
                {
                    tracedb_TimeoutInit ( & tm, db -> timeout, true );
                    status = tracedb_BTCacheTimedWriteLock ( & db -> dcache, tm );
                    tracedb_TimeoutWhack ( tm );
                }
                if ( status == 0 )
                {
                    TRACEDB_VERBOSE ( 5, db, ( "GC task - inside write lock on dcache\n" ) );

                    /* drop excess files from cache */
                    while ( db -> dcache . count > db -> dlim )
                    {
                        TraceDBFile *b = ( TraceDBFile* )
                            tracedb_BTCacheDrop ( & db -> dcache );
                        assert ( b != NULL );
                        TraceDBFileRelease ( b, db );
                    }

                    TRACEDB_VERBOSE ( 5, db, ( "GC task - releasing write lock on dcache\n" ) );
                    tracedb_BTCacheUnlock ( & db -> dcache );
                }
            }

            /* release condition mutex */
            tracedb_ConditionUnlock ( db -> gcond );
        }

        /* signal our exit */
        tracedb_SemaphoreSignal ( db -> gsem );
    }

    return status;
}


/* TraceDBGetFile
 *  looks up a file from dcache
 */
static
int TraceDBGetFile ( TraceDB *db, TraceDBPath *loc,
    unsigned int fileno, TraceDBFile **bp )
{
    int status;
    tracedb_timeout_t tm;

    status = tracedb_BTCacheTryReadLock ( & db -> dcache );
    if ( status == EBUSY )
    {
        tracedb_TimeoutInit ( & tm, db -> timeout, true );
        status = tracedb_BTCacheTimedReadLock ( & db -> dcache, tm );
        tracedb_TimeoutWhack ( tm );
    }
    if ( status == 0 )
    {
        TraceDBFile *b;
        TraceDBFileData file_data;

#if TRACEDB_CACHE_USE_RWLOCK
        TRACEDB_VERBOSE ( 5, db, ( "inside read lock on dcache\n" ) );
#else
        TRACEDB_VERBOSE ( 5, db, ( "inside lock on dcache\n" ) );
#endif
        file_data . db = loc;
        file_data . fileno = fileno;

        b = ( TraceDBFile* )
            tracedb_BTCacheFind ( & db -> dcache, & file_data, TraceDBFileFind );
        if ( b == NULL )
        {
#if TRACEDB_CACHE_USE_RWLOCK
            TRACEDB_VERBOSE ( 5, db, ( "releasing read lock on dcache\n" ) );

            status = tracedb_BTCacheTryChangeToWriteLock ( & db -> dcache );
            if ( status == EBUSY )
            {
                tracedb_TimeoutInit ( & tm, db -> timeout, true );
                status = tracedb_BTCacheTimedWriteLock ( & db -> dcache, tm );
                tracedb_TimeoutWhack ( tm );
            }

            if ( status != 0 )
            {
                TRACEDB_VERBOSE ( 0, db, ( "failed to acquire write lock on dcache: error %d\n", status ) );
                return status;
            }

            TRACEDB_VERBOSE ( 5, db, ( "inside write lock on dcache\n" ) );

            b = ( TraceDBFile* )
                tracedb_BTCacheFind ( & db -> dcache, & file_data, TraceDBFileFind );
            if ( b != NULL )
            {
                tracedb_atomic32_inc (  & b -> refcount );
                * bp = b;
            }
            else
#endif
#if CATCH_EMFILE_ERR
            while ( 1 )
#endif
            {
                /* flush cache if we're at or above max */
                if ( db -> dmax == 0 ) while ( db -> dcache . count > 0 )
                {
                    /* this case is here for robustness, but
                       should never execute because with a dmax
                       of 0, files should never be cached */
                    b = ( TraceDBFile* ) tracedb_BTCacheDrop ( & db -> dcache );
                    assert ( b != NULL );
                    TraceDBFileRelease ( b, db );
                }
                else while ( db -> dcache . count >= db -> dmax )
                {
                    /* bring the cache contents down to 1 under limit
                       because we're about to open a new file and
                       insert it into the cache, reaching limit again */
                    b = ( TraceDBFile* ) tracedb_BTCacheDrop ( & db -> dcache );
                    assert ( b != NULL );
                    TraceDBFileRelease ( b, db );
                }

                /* request a count from semaphore */
                tracedb_TimeoutInit ( & tm, db -> timeout, false );
                status = tracedb_SemaphoreTimedWait ( db -> dsem, tm );
                tracedb_TimeoutWhack ( tm );

                /* go ahead if we got it */
                if ( status == 0 )
                {
                    status = TraceDBFileMake ( bp, loc, fileno );
                    if ( status != 0 )
                    {
                        /* replace the semaphore's count */
                        tracedb_SemaphoreSignal ( db -> dsem );
#if CATCH_EMFILE_ERR
                        if ( status == EMFILE && db -> dcache . count > 0 )
                        {
                            db -> dmax = db -> dcache . count;
                            continue;
                        }
#endif
                    }
                    else
                    {
                        b = * bp;

                        /* if caching is enabled */
                        if ( db -> dlim > 0 )
                        {
                            /* insert into the cache */
                            if ( tracedb_BTCacheInsert ( & db -> dcache, & b -> lru, TraceDBFileSort ) )
                            {
                                tracedb_atomic32_set ( & b -> refcount, 2 );
                                if ( ++ db -> dcache . count > db -> dlim )
                                {
                                    if ( tracedb_ConditionLock ( db -> gcond ) == 0 )
                                    {
                                        tracedb_ConditionSignal ( db -> gcond );
                                        tracedb_ConditionUnlock ( db -> gcond );
                                    }
                                }
                            }
                        }
                    }
                }
#if CATCH_EMFILE_ERR
                break;
#endif
            }
        }
        else
        {
            tracedb_atomic32_inc ( & b -> refcount );
            * bp = b;
        }

        TRACEDB_VERBOSE ( 5, db, ( "releasing lock on dcache\n" ) );
        tracedb_BTCacheUnlock ( & db -> dcache );

    }
    return status;
}

/* TraceDBGetLoc
 *  looks up an existing db location by path data
 *  creates a new location as required
 *  returns the location and open index file
 *
 *  NB - called within a write lock on icache
 */
static
int TraceDBGetLoc ( TraceDB *db, const TraceDBPathData *pd,
    TraceDBPath **locp, TraceDBFile **bp )
{
    int status;

    /* find location */
    TraceDBPath *loc = ( TraceDBPath* )
        tracedb_BTreeFind ( & db -> paths, pd, TraceDBPathFind );

    /* if found, then get the file */
    if ( loc != NULL )
        return TraceDBGetFile ( db, * locp = loc, 0, bp );

    /* create one */
    loc = malloc ( sizeof * loc - sizeof loc -> path +
                   pd -> len + 1 );
    if ( loc == NULL )
        return errno;

    loc -> len = pd -> len;
    loc -> hash = pd -> hash;
    strcpy ( loc -> path, pd -> path );

    /* open the index file */
    status = TraceDBGetFile ( db, * locp = loc, 0, bp );
    if ( status == 0 )
    {
        tracedb_IdxHeader hdr;
        size_t num_read;

        /* read the index file header */
        TraceDBFile *b = * bp;
        status = tracedb_FileRead ( b -> f, 0, & hdr, sizeof hdr, & num_read );
        if ( status == 0 )
        {
            if ( num_read != sizeof hdr )
            {
                TRACEDB_VERBOSE ( 0, db, ( "ignoring corrupt index file of '%s' with size %" TRACEDB_LUSZ "\n", loc -> path, num_read ) );
                status = ENOENT;
            }
            else if ( hdr . endian != tracedb_eByteOrderTag )
            {
                if ( hdr . endian == tracedb_eByteOrderReverse )
                    TRACEDB_VERBOSE ( 0, db, ( "ignoring index file of '%s' due to reverse byte ordering\n", loc -> path ) );
                else
                    TRACEDB_VERBOSE ( 0, db, ( "ignoring corrupt index file of '%s' due to unrecognized byte ordering\n", loc -> path ) );
                status = ENOENT;
            }
            else if ( hdr . version != tracedb_eCurrentVersion )
            {
                TRACEDB_VERBOSE ( 0, db, ( "ignoring index file of '%s' due to unsupported version: %u\n", loc -> path, hdr . version ) );
                status = ENOENT;
            }
            else
            {
                loc -> ti_start = hdr . ti_start;
                loc -> ti_stop = hdr . ti_stop;

                /* insert into the tree */
                tracedb_BTreeInsert ( & db -> paths, & loc -> n, TraceDBPathSort );

                /* got it */
                return 0;
            }
        }

        /* drop the file */
        TraceDBFileRelease ( b, db );
    }

    /* drop the index */
    free ( loc );

    return status;
}

/* TraceDBGetIdx
 *  looks up an existing index from cache by id
 *  ensures that the path is still the same if found
 *  creates a new index as required
 */
static
int TraceDBGetIdx ( TraceDB *db, const char *path, ti_t id, TraceDBIdx **idxp )
{
    int status;
    tracedb_timeout_t tm;

    TraceDBPathData pd;

    /* turn the path into path data */
    pd . path = path;
    pd . len = strlen ( path );
    pd . hash = tracedb_string_hash ( path, pd . len );

    /* find index */
    status = tracedb_BTCacheTryReadLock ( & db -> icache );
    if ( status == EBUSY )
    {
        tracedb_TimeoutInit ( & tm, db -> timeout, true );

        status = tracedb_BTCacheTimedReadLock ( & db -> icache, tm );

        tracedb_TimeoutWhack ( tm );
    }
    if ( status == 0 )
    {
        TraceDBIdx *idx, *inv;
        TraceDBPath *loc = NULL;
        TraceDBIdxData idx_data;

#if TRACEDB_CACHE_USE_RWLOCK
        TRACEDB_VERBOSE ( 5, db, ( "inside read lock on icache\n" ) );
#else
        TRACEDB_VERBOSE ( 5, db, ( "inside lock on icache\n" ) );
#endif

        /* look up the index by id */
        idx_data . id = id;
        idx = ( TraceDBIdx* )
            tracedb_BTCacheFind ( & db -> icache, & idx_data, TraceDBIdxFind );

        /* if found, check that the path hasn't changed */
        if ( idx != NULL )
        {
            loc = idx -> db;
            assert ( loc != NULL );

            if ( loc -> len != pd . len ||
                 loc -> hash != pd . hash ||
                 memcmp ( loc -> path, path, pd . len ) != 0 )
            {
                /* we've been sent a new path */
                inv = idx;
                idx = NULL;
            }
        }

        /* if not found or changed, get a new index */
        if ( idx == NULL )
        {
#if TRACEDB_CACHE_USE_RWLOCK
            /* switch to write lock */
            TRACEDB_VERBOSE ( 5, db, ( "releasing read lock on icache\n" ) );

            status = tracedb_BTCacheTryChangeToWriteLock ( & db -> icache );
            if ( status == EBUSY )
            {
                tracedb_TimeoutInit ( & tm, db -> timeout, true );
                status = tracedb_BTCacheTimedWriteLock ( & db -> icache, tm );
                tracedb_TimeoutWhack ( tm );
            }

            if ( status != 0 )
            {
                TRACEDB_VERBOSE ( 0, db, ( "failed to acquire write lock on icache: error %d\n", status ) );
                return status;
            }

            TRACEDB_VERBOSE ( 5, db, ( "inside write lock on icache\n" ) );

            /* try again for index */
            idx = ( TraceDBIdx* )
                tracedb_BTCacheFind ( & db -> icache, & idx_data, TraceDBIdxFind );

            /* if it had been found before but not now,
               forget any previously held loc that would
               otherwise get invalidated below */
            if ( idx == NULL )
                loc = NULL;

            /* if it had been found before and is still
               the same thing, we'll get the same result */
            else if ( idx == inv )
                idx = NULL;

            /* it has changed, so check it out */
            else
            {
                loc = idx -> db;
                assert ( loc != NULL );

                if ( loc -> len != pd . len ||
                     loc -> hash != pd . hash ||
                     memcmp ( loc -> path, path, pd . len ) != 0 )
                {
                    /* we've been sent a new path */
                    inv = idx;
                    idx = NULL;
                }
                else
                {
                    tracedb_atomic32_inc ( & idx -> refcount );
                    * idxp = idx;
                }
            }

            if ( idx == NULL )
#endif
            {
                TraceDBFile *b;
                TraceDBPath *locp;

                /* invalidate location path if needed */
                if ( loc != NULL )
                {
                    TRACEDB_VERBOSE ( 2, db, ( "invalidating path '%s' - new path '%s'\n",
                                       loc -> path, path ) );

                    /* the original index is invalid */
                    tracedb_BTCacheUnlink ( & db -> icache, & inv -> lru );
                    inv -> valid = 0;
                    if ( ! tracedb_BTCacheInsert ( & db -> icache, & inv -> lru, TraceDBIdxSort ) )
                        TraceDBIdxRelease ( inv, db );
                    else
                        ++ db -> icache . count;

                    /* the original location is invalid */
                    tracedb_BTreeUnlink ( & db -> paths, & loc -> n );
                    loc -> hash = 0;
                    if ( tracedb_BTreeInsert ( & db -> paths, & loc -> n, TraceDBPathSort ) != 0 )
                        free ( loc );
                }

                /* locate the path object */
                status = TraceDBGetLoc ( db, & pd, & locp, & b );
                if ( status == 0 )
                {
                    /* keep close to register */
                    loc = locp;

                    /* ensure valid id */
                    if ( id < loc -> ti_start || id > loc -> ti_stop )
                    {
                        /* there is no record with this id in this db */
                        status = ENOENT;
                    }
                    else
                    {
                        ti_t ti_start, ti_stop;

                        /* normalize id within index */
                        id -= loc -> ti_start;

                        /* block range */
                        ti_start = id - ( id & TRACEDB_IDXMASK ) + loc -> ti_start;
                        ti_stop = ti_start + ( TRACEDB_IDXSIZE - 1 );

                        /* restore id in case it's used later on */
                        id += loc -> ti_start;

                        /* limit to last index in db */
                        if ( ti_stop > loc -> ti_stop )
                            ti_stop = loc -> ti_stop;

                        /* flush cache if at or above max */
                        if ( db -> imax == 0 ) while ( db -> icache . count > 0 )
                        {
                            /* this code is here for robustness, but should
                               never execute since an imax of 0 indicates no
                               index should be cached. */
                            idx = ( TraceDBIdx* ) tracedb_BTCacheDrop ( & db -> icache );
                            assert ( idx != NULL );
                            TraceDBIdxRelease ( idx, db );
                        }
                        else while ( db -> icache . count >= db -> imax )
                        {
                            /* bring count down to 1 below max so that
                               the subsequent insert does not exceed max */
                            idx = ( TraceDBIdx* ) tracedb_BTCacheDrop ( & db -> icache );
                            assert ( idx != NULL );
                            TraceDBIdxRelease ( idx, db );
                        }

                        /* wait on isem */
                        tracedb_TimeoutInit ( & tm, db -> timeout, false );
                        status = tracedb_SemaphoreTimedWait ( db -> isem, tm );
                        tracedb_TimeoutWhack ( tm );

                        /* if we got it */
                        if ( status == 0 )
                        {
                            size_t idx_bytes = sizeof idx -> idx [ 0 ] *
                                ( unsigned int ) ( ti_stop - ti_start + 1 );

                            /* create a new index */
                            idx = malloc ( sizeof * idx - sizeof idx -> idx + idx_bytes );
                            if ( idx == NULL )
                            {
                                status = errno;
                                tracedb_SemaphoreSignal ( db -> isem );
                            }
                            else
                            {
                                size_t num_read;
                                
                                /* offset into index file */
                                uint64_t pos = sizeof ( tracedb_IdxHeader ) +
                                    ( ti_start - loc -> ti_start ) *
                                    sizeof idx -> idx [ 0 ];

                                /* read indices */
                                status = tracedb_FileRead ( b -> f, pos, idx -> idx, idx_bytes, & num_read );
                                if ( status != 0 )
                                {
                                    free ( idx );
                                    tracedb_SemaphoreSignal ( db -> isem );
                                    TRACEDB_VERBOSE ( 0, db, ( "failed to read file '%s/idx%d' at "
                                                       "offset %" TRACEDB_LU64 " with size %" TRACEDB_LUSZ "\n",
                                                       loc -> path, TRACEDB_VERSION, pos, idx_bytes ) );
                                }
                                else if ( num_read != idx_bytes )
                                {
                                    status = -1;
                                    free ( idx );
                                    tracedb_SemaphoreSignal ( db -> isem );
                                    TRACEDB_VERBOSE ( 0, db, ( "short read from file '%s/idx%d' at "
                                                       "offset %" TRACEDB_LU64 " with size %" TRACEDB_LUSZ
                                                       " and num read %" TRACEDB_LUSZ "\n",
                                                       loc -> path, TRACEDB_VERSION,
                                                       pos, idx_bytes, num_read ) );
                                }
                                else
                                {
                                    TRACEDB_VERBOSE ( 2, db, ( "read %" TRACEDB_LUSZ " bytes from file '%s/idx%d'\n",
                                                       num_read, loc -> path, TRACEDB_VERSION ) );

                                    /* have a valid index block */
                                    idx -> db = loc;
                                    idx -> ti_start = ti_start;
                                    idx -> ti_stop = ti_stop;
                                    idx -> valid = 1;
                                    tracedb_atomic32_set ( & idx -> refcount, 1 );
                                    * idxp = idx;
                                    
                                    /* check for cache enable */
                                    if ( db -> ilim > 0 )
                                    {
                                        if ( tracedb_BTCacheInsert ( & db -> icache, & idx -> lru, TraceDBIdxSort ) )
                                        {
                                            tracedb_atomic32_set ( & idx -> refcount, 2 );
                                            if ( ++ db -> icache . count > db -> ilim )
                                            {
                                                if ( tracedb_ConditionLock ( db -> gcond ) == 0 )
                                                {
                                                    tracedb_ConditionSignal ( db -> gcond );
                                                    tracedb_ConditionUnlock ( db -> gcond );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    TraceDBFileRelease ( b, db );
                }
            }
        }

        /* have a valid index */
        else
        {
            tracedb_atomic32_inc ( & idx -> refcount );
            * idxp = idx;
        }

        TRACEDB_VERBOSE ( 5, db, ( "releasing lock on icache\n" ) );
        tracedb_BTCacheUnlock ( & db -> icache );
    }

    return status;
}

/* TraceDBRead
 *  reads from a trace blob
 *
 *  reads a maximum of "max_len" bytes into "buffer",
 *  starting at "offset" into blob, returns the number
 *  of bytes read in "num_read" and the bytes remaining
 *  to be read in "remaining", i.e.
 *
 *    remaining = blob_size - offset - num_read;
 */
int TraceDBRead ( TraceDB *db, const char *path, ti_t id,
    void *buffer, size_t max_len, size_t offset,
    size_t *num_read, size_t *remaining )
{
    int status;
    TraceDBIdx *idxp;

    if ( db == NULL || db -> quitting )
        return EINVAL;

    if ( max_len != 0 && buffer == NULL )
        return EINVAL;

    status = TraceDBGetIdx ( db, path, id, & idxp );
    if ( status == 0 )
    {
        TraceDBIdx *idx = idxp;
        TraceIdx ii = idx -> idx [ id - idx -> ti_start ];

        /* get the item offset and size */
        uint64_t off = TraceIdxGetOID ( ii );
        size_t size = TraceIdxGetSize ( ii );

        /* get the database location */
        TraceDBPath *loc = idx -> db;

        /* by now we're done with the index */
        TraceDBIdxRelease ( idx, db );

        /* an offset of 0 is not found */
        if ( off == 0 )
            status = ENOENT;

        /* check for end of blob */
        else if ( offset >= size )
        {
            if ( num_read != NULL )
                * num_read = 0;
            if ( remaining != NULL )
                * remaining = 0;
        }

        /* attempt to read some blob */
        else
        {
            TraceDBFile *b;

            status = TraceDBGetFile ( db, loc, TRACEDB_FILENO ( off ) + 1, & b );
            if ( status == 0 )
            {
                size_t to_read, dummy;

                /* ensure "num_read" is not NULL */
                if ( num_read == NULL )
                    num_read = & dummy;

                /* remaining bytes to read */
                size -= offset;

                /* calculate offset into file */
                off = TRACEDB_FILEOFF ( off ) + offset;

                /* the number of bytes that should be read this time */
                to_read = size;
                if ( to_read > max_len )
                    to_read = max_len;

                /* read into buffer */
                status = tracedb_FileRead ( b -> f, off, buffer, to_read, num_read );

                /* done with file */
                TraceDBFileRelease ( b, db );

                /* cleanup after read */
                if ( status == 0 && * num_read != to_read )
                    status = -1;
                else if ( remaining != NULL )
                    * remaining = size - to_read;
            }
        }
    }

    return status;
}


/* TraceDBReadHeaderInfo
 *  reads header information from a trace blob
 */
int TraceDBReadHeaderInfo ( TraceDB *db, const char *path, ti_t id,
    TraceDBHeaderInfo *info )
{
    int status;
    TraceDBIdx *idxp;

    if ( db == NULL || info == NULL || db -> quitting )
        return EINVAL;

    status = TraceDBGetIdx ( db, path, id, & idxp );
    if ( status == 0 )
    {
        TraceDBIdx *idx = idxp;
        TraceIdx ii = idx -> idx [ id - idx -> ti_start ];

        /* get the database location */
        TraceDBPath *loc = idx -> db;

        /* get the item offset and size */
        info -> idx . oid = TraceIdxGetOID ( ii );
        info -> idx . size = TraceIdxGetSize ( ii );

        /* by now we're done with the index */
        TraceDBIdxRelease ( idx, db );

        /* an offset of 0 has no blob */
        if ( info -> idx . oid == 0 )
        {
            info -> hdr . id = 0;
            info -> hdr . size = 0;
            info -> hdr . crc = 0;
        }

        /* attempt to read some blob */
        else
        {
            TraceDBFile *b;

            status = TraceDBGetFile ( db, loc, TRACEDB_FILENO ( info -> idx . oid ) + 1, & b );
            if ( status == 0 )
            {
                TraceHdr hdr;
                size_t num_read;

                /* calculate offset to hdr */
                uint64_t off = TRACEDB_FILEOFF ( info -> idx . oid ) - sizeof hdr;

                /* read into hdr */
                status = tracedb_FileRead ( b -> f, off, & hdr, sizeof hdr, & num_read );

                /* done with file */
                TraceDBFileRelease ( b, db );

                /* cleanup after read */
                if ( status == 0 && num_read != sizeof hdr )
                    status = -1;
                else
                {
                    info -> hdr . id = TraceHdrGetID ( hdr );
                    info -> hdr . size = TraceHdrGetSize ( hdr );
                    info -> hdr . crc = TraceHdrGetCRC ( hdr );
                }
            }
        }
    }

    return status;
}
