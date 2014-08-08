#include "vfd-priv.h"
#include "container.h"
#include "trace_atomic.h"
#include "text.h"
#include "lock.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

/*--------------------------------------------------------------------------
 * FDBucket
 *  sparse array implementation
 */
#define TRACEDB_FDLEN ( 1 << 6 )

typedef struct tracedb_FDSlotBucket tracedb_FDSlotBucket;
struct tracedb_FDSlotBucket
{
    tracedb_BTNode n;

    /* the range of fds goes from start to start + dim */
    tracedb_fd_t start;

    /* the number of valid fds in array */
    unsigned int count;

    /* fd array */
    void *obj [ TRACEDB_FDLEN ];
};

static
int tracedb_FDSlotBucketFind ( const void *item, const tracedb_BTNode *n )
{
    return * ( const tracedb_fd_t* ) item - ( ( const tracedb_FDSlotBucket* ) n ) -> start;
}

static
int tracedb_FDSlotBucketCmp ( const tracedb_BTNode *item, const tracedb_BTNode *n )
{
    return ( ( const tracedb_FDSlotBucket* ) item ) -> start -
        ( ( const tracedb_FDSlotBucket* ) n ) -> start;
}

typedef struct tracedb_FDFirstFreeData tracedb_FDFirstFreeData;
struct tracedb_FDFirstFreeData
{
    tracedb_FDSlotBucket *b;
    tracedb_fd_t fd;
};

static
bool tracedb_FDFirstFree ( tracedb_BTNode *n, void *data )
{
    tracedb_fd_t fd;
    tracedb_FDFirstFreeData *pb = data;

    /* a hole in the array at the current fd start */
    if ( ( ( const tracedb_FDSlotBucket* ) n ) -> start != pb -> fd )
        return true;

    /* skip over entire block space */
    if ( ( ( const tracedb_FDSlotBucket* ) n ) -> count == TRACEDB_FDLEN )
    {
        pb -> fd += TRACEDB_FDLEN;
        return false;
    }

    /* have an array block with space - find first free */
    for ( fd = 0; fd < TRACEDB_FDLEN; ++ fd )
    {
        if ( ( ( const tracedb_FDSlotBucket* ) n ) -> obj [ fd ] == NULL )
            break;
    }

    assert ( fd < TRACEDB_FDLEN );
    pb -> fd += fd;
    pb -> b = ( tracedb_FDSlotBucket* ) n;

    return true;
}

static
int tracedb_FDSlotNew ( tracedb_BTree *tbl, tracedb_fd_t *fd, void *item )
{
    tracedb_FDSlotBucket *b;
    tracedb_FDFirstFreeData pb;

    if ( item == NULL )
        return EINVAL;

    /* find a free fd */
    pb . b = NULL;
    pb . fd = 0;
    tracedb_BTreeDoUntil ( tbl, false, tracedb_FDFirstFree, & pb );

    /* if there is no bucket, create it */
    b = pb . b;
    if ( pb . b == NULL )
    {
        b = malloc ( sizeof * b );
        if ( b == NULL )
            return errno;

        assert ( ( pb . fd & ( TRACEDB_FDLEN - 1 ) ) == 0 );
        b -> start = pb . fd;
        b -> count = 0;
        memset ( b -> obj, 0, sizeof b -> obj );
        tracedb_BTreeInsert ( tbl, & b -> n, tracedb_FDSlotBucketCmp );
    }

    /* insert the object */
    b -> obj [ pb . fd - b -> start ] = item;
    ++ b -> count;

    /* return the fd */
    * fd = pb . fd;
    return 0;
}

static
int tracedb_FDSlotInvalidate ( tracedb_BTree *tbl, tracedb_fd_t fd,
    tracedb_FDSlotBucket **bp, void **prior )
{
    tracedb_fd_t id = fd & - TRACEDB_FDLEN;

    tracedb_FDSlotBucket *b = ( tracedb_FDSlotBucket* )
        tracedb_BTreeFind ( tbl, & id, tracedb_FDSlotBucketFind );

    if ( ( * bp = b ) == NULL )
        return EBADF;

    /* the following code rejects NULL and -1 */
    switch ( ( size_t ) ( * prior = b -> obj [ fd - id ] ) + 1 )
    {
    case 0:
    case 1:
        return EBADF;
    }

    b -> obj [ fd - id ] = ( void* ) ~ ( size_t ) 0;
    return 0;
}

#define tracedb_FDSlotReset( fd, b, prior ) \
    ( ( b ) -> obj [ ( fd ) & ( TRACEDB_FDLEN - 1 ) ] = ( prior ) )

static
void tracedb_FDSlotWhack ( tracedb_BTree *tbl, tracedb_fd_t fd,
        tracedb_FDSlotBucket *b )
{
    tracedb_fd_t id = fd & - TRACEDB_FDLEN;

    b -> obj [ fd - id ] = NULL;
    if ( -- b -> count == 0 )
    {
        tracedb_BTreeUnlink ( tbl, & b -> n );
        free ( b );
    }
}

static
int tracedb_FDSlotSet ( tracedb_BTree *tbl, tracedb_fd_t fd,
    void *item, void **prior )
{
    tracedb_fd_t id;
    tracedb_FDSlotBucket *b;

    if ( item == NULL )
        return EINVAL;

    id = fd & - TRACEDB_FDLEN;
    b = ( tracedb_FDSlotBucket* ) tracedb_BTreeFind ( tbl, & id,
                                    tracedb_FDSlotBucketFind );

    if ( b == NULL )
    {
        b = malloc ( sizeof * b );
        if ( b == NULL )
            return errno;

        b -> start = id;
        b -> count = 0;
        memset ( b -> obj, 0, sizeof b -> obj );
        tracedb_BTreeInsert ( tbl, & b -> n, tracedb_FDSlotBucketCmp );
    }

    if ( ( * prior = b -> obj [ fd - id ] ) == NULL )
        ++ b -> count;

    b -> obj [ fd - id ] = item;
    return 0;
}

static
int tracedb_FDSlotGet ( tracedb_BTree *tbl, tracedb_fd_t fd, void **item )
{
    tracedb_fd_t id = fd & - TRACEDB_FDLEN;

    tracedb_FDSlotBucket *b = ( tracedb_FDSlotBucket* )
        tracedb_BTreeFind ( tbl, & id, tracedb_FDSlotBucketFind );

    if ( b == NULL )
        return EBADF;

    /* reject NULL and -1 */
    switch ( ( size_t ) ( * item = b -> obj [ fd - id ] ) + 1 )
    {
    case 0:
    case 1:
        return EBADF;
    }

    return 0;
}


/*--------------------------------------------------------------------------
 * FNSBucket
 *  holds onto an FS or NS object
 */
typedef struct tracedb_FNSBucket tracedb_FNSBucket;
struct tracedb_FNSBucket
{
    tracedb_BTNode n;

    void *obj;
    const tracedb_String *name;

    /* LSB is name reference
       remaining are slot references */
    tracedb_atomic32_t refcount;
};

#define tracedb_FNSBucketInit( self, object, ext_name, refs ) \
    ( ( self ) -> obj = ( object ), \
      ( self ) -> name = ( ext_name ), \
      tracedb_atomic32_set ( & ( self ) -> refcount, ( refs ) ) )

#define tracedb_FNSBucketAddRef( self ) \
    tracedb_atomic32_add ( & ( self ) -> refcount, 2 )

#define tracedb_FNSBucketRelease( self ) \
    tracedb_atomic32_add_and_read ( & ( self ) -> refcount, -2 )

#define tracedb_FNSBucketAddNameRef( self ) \
    ( assert ( ( tracedb_atomic32_read ( & ( self ) -> refcount ) & 1 ) == 0 ), \
      tracedb_atomic32_inc ( & ( self ) -> refcount ) )

#define tracedb_FNSBucketNameRelease( self ) \
    ( assert ( ( tracedb_atomic32_read ( & ( self ) -> refcount ) & 1 ) != 0 ), \
      tracedb_atomic32_dec_and_test ( & ( self ) -> refcount ) )

#define tracedb_FNSBucketAssertName( self ) \
    ( assert ( ( self ) -> name != NULL ), \
      assert ( ( tracedb_atomic32_read ( & ( self ) -> refcount ) & 1 ) != 0 ) )

static
int tracedb_FNSBucketFind ( const void *item, const tracedb_BTNode *n )
{
    return tracedb_StringCompare (
        ( const tracedb_String* ) item,
        ( ( const tracedb_FNSBucket* ) n ) -> name
        );
}

static
int tracedb_FNSBucketCmp ( const tracedb_BTNode *item, const tracedb_BTNode *n )
{
    return tracedb_StringCompare (
        ( ( const tracedb_FNSBucket* ) item ) -> name,
        ( ( const tracedb_FNSBucket* ) n ) -> name
        );
}


/*--------------------------------------------------------------------------
 * sFSNameTable - a table of FS objects accessible by name
 * sFSTable     - a table of FS objects accessible by fs_t
 * sNSNameTable - a table of NS objects accessible by name
 * sNSTable     - a table of NS objects accessible by ns_t
 * sFNSInited   - whether the locks have been initialized
 */
static tracedb_BTree tracedb_sFSNameTable;
static tracedb_RWLock tracedb_sFSNameLock;

static tracedb_BTree tracedb_sFSTable;
static tracedb_RWLock tracedb_sFSLock;

static tracedb_BTree tracedb_sNSNameTable;
static tracedb_RWLock tracedb_sNSNameLock;

static tracedb_BTree tracedb_sNSTable;
static tracedb_RWLock tracedb_sNSLock;

static bool tracedb_sFNSInited;


/*--------------------------------------------------------------------------
 * sFDTable     - a table of FD objects accessible by fd_t
 */
static tracedb_BTree tracedb_sFDTable;
static tracedb_RWLock tracedb_sFDLock;


/*--------------------------------------------------------------------------
 * FNS
 *  combined FS/NS operations
 */


/* FNSInit
 *  initialize the locks
 */
static
int tracedb_FNSInit ( void )
{
    assert ( ! tracedb_sFNSInited );

    /* initialize the named file system table */
    tracedb_BTreeInit ( & tracedb_sFSNameTable );
    tracedb_RWLockInit ( & tracedb_sFSNameLock );

    /* initialize the fs table */
    tracedb_BTreeInit ( & tracedb_sFSTable );
    tracedb_RWLockInit ( & tracedb_sFSLock );

    /* initialize the named network system table */
    tracedb_BTreeInit ( & tracedb_sNSNameTable );
    tracedb_RWLockInit ( & tracedb_sNSNameLock );

    /* initialize the ns table */
    tracedb_BTreeInit ( & tracedb_sNSTable );
    tracedb_RWLockInit ( & tracedb_sNSLock );

    /* initialize the fd table */
    tracedb_BTreeInit ( & tracedb_sFDTable );
    tracedb_RWLockInit ( & tracedb_sFDLock );

    tracedb_sFNSInited = true;
    return 0;
}

int tracedb_VFSInit ( void )
{
    if ( tracedb_sFNSInited )
        return 0;
    return tracedb_FNSInit ();
}

int tracedb_VNSInit ( void )
{
    if ( tracedb_sFNSInited )
        return 0;
    return tracedb_FNSInit ();
}

/* FNSOpenAnon
 *  creates an id for provided object
 *  used to pass an anonymously created id
 *
 *  when returned id is closed, the whack
 *  method of obj will be invoked.
 */
static
int tracedb_FNSOpenAnon ( void *self, tracedb_fd_t *fd,
    tracedb_BTree *tbl, tracedb_RWLockRef lock )
{
    int status = EINVAL;
    if ( fd != NULL )
    {
        if ( self != NULL )
        {
            if ( ! tracedb_sFNSInited )
                status = EFAULT;
            else
            {
                tracedb_FNSBucket *b = malloc ( sizeof * b );
                if ( b == NULL )
                    status = errno;
                else
                {
                    /* object is opened, but not registered */
                    tracedb_FNSBucketInit ( b, self, NULL, 2 );

                    /* create a slot */
                    status = tracedb_RWLockWriteLock ( TRACEDB_RWLOCKDEREF ( lock ) );
                    if ( status == 0 )
                    {
                        status = tracedb_FDSlotNew ( tbl, fd, b );
            
                        tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( lock ) );
            
                        if ( status == 0 )
                        {
                            tracedb_FNSAddRef ( ( tracedb_FS* ) self );
                            assert ( tracedb_atomic_read ( & ( ( tracedb_FS* ) self ) -> refcount ) > 0 );
                            * fd += 1;
                            return status;
                        }
                    }


                    free ( b );
                }
            }
        }

        * fd = -1;
    }

    return status;
}

int tracedb_FSOpenAnon ( tracedb_FS *self, tracedb_fs_t *fs )
{
    return tracedb_FNSOpenAnon ( self, fs, & tracedb_sFSTable,
                                TRACEDB_RWLOCKREF ( tracedb_sFSLock ) );
}

int tracedb_NSOpenAnon ( tracedb_NS *self, tracedb_ns_t *ns )
{
    return tracedb_FNSOpenAnon ( self, ns, & tracedb_sNSTable,
                                TRACEDB_RWLOCKREF ( tracedb_sNSLock ) );
}

/* FNSRegister
 *  gives an external name to an anonymous entry
 */
static
int tracedb_FNSRegister ( tracedb_fs_t self, void **ticket, const char *name,
    tracedb_BTree *ntbl, tracedb_RWLockRef nlock, 
    tracedb_BTree *dtbl, tracedb_RWLockRef dlock )
{
    int status = EINVAL;

    if ( ticket != NULL )
    {
        * ticket = NULL;

        if ( self > 0 && name != NULL && name [ 0 ] != 0 )
        {
            if ( self == 0 )
                status = EEXIST;
            else if ( ! tracedb_sFNSInited )
                status = EFAULT;
            else
            {
                /* lock slot table against modification
                   and use as a critical section */
                status = tracedb_RWLockWriteLock ( TRACEDB_RWLOCKDEREF ( dlock ) );
                if ( status == 0 )
                {
                    /* look up the bucket */
                    tracedb_FNSBucket *b;
                    status = tracedb_FDSlotGet ( dtbl, self - 1, ( void** ) & b );
                    if ( status == 0 )
                    {
                        /* can't name it twice */
                        if ( b -> name != NULL )
                            status = EBUSY;
                        else
                        {
                            /* now we need to modify the name table */
                            status = tracedb_RWLockWriteLock ( TRACEDB_RWLOCKDEREF ( nlock ) );
                            if ( status == 0 )
                            {
                                /* give it a name */
                                tracedb_String str;
                                tracedb_StringInitCtracedb_String ( & str, name );
                                status = tracedb_StringCopy ( & b -> name, & str );
                                if ( status == 0 )
                                {
                                    /* insert into name table */
                                    tracedb_BTNode *exist;
                                    status = tracedb_BTreeInsertUnique ( ntbl, & b -> n, & exist, tracedb_FNSBucketCmp );
                                    if ( status != 0 )
                                    {
                                        /* drop name */
                                        tracedb_StringWhack ( ( tracedb_String* ) b -> name );
                                        b -> name = NULL;
                                    }
                                    else
                                    {
                                        /* success */
                                        * ticket = b;
                                        tracedb_FNSBucketAddNameRef ( b );
                                    }
                                }

                                /* release the name table lock */
                                tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( nlock ) );
                            }
                        }
                    }

                    /* release the slot table lock */
                    tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( dlock ) );
                }
            }
        }
    }

    return status;
}

int tracedb_FSRegister ( tracedb_fs_t self, void **ticket, const char *name )
{
    return tracedb_FNSRegister ( self, ticket, name,
        & tracedb_sFSNameTable, TRACEDB_RWLOCKREF ( tracedb_sFSNameLock ),
        & tracedb_sFSTable, TRACEDB_RWLOCKREF ( tracedb_sFSLock ) );
}

int tracedb_NSRegister ( tracedb_ns_t self, void **ticket, const char *name )
{
    return tracedb_FNSRegister ( self, ticket, name,
        & tracedb_sNSNameTable, TRACEDB_RWLOCKREF ( tracedb_sNSNameLock ),
        & tracedb_sNSTable, TRACEDB_RWLOCKREF ( tracedb_sNSLock ) );
}

/* FNSUnregister
 *  removes an FS or NS object from the registry
 *
 *  the incoming bucket may have a refcount >= 0:
 *
 *  a) when > 0, the system is open, but is subject to being
 *     whacked during the execution of this function. if so,
 *     the bucket+system will not be touched because the
 *     bucket has a name.
 *
 *  b) when == 0, the system is closed and can only be
 *     referenced by name, i.e. via the name table, and we
 *     lock that for update during the function which protects
 *     the object from any access.
 */
static
int tracedb_FNSUnregister ( tracedb_FNSBucket *b,
        tracedb_BTree *tbl, tracedb_RWLockRef lock )
{
    int status;

    if ( b == NULL )
        return EINVAL;

    if ( ! tracedb_sFNSInited )
        return ENOENT;

    /* early assert that bucket is, in fact, named */
    tracedb_FNSBucketAssertName ( b );

    /* lock table for update */
    status = tracedb_RWLockWriteLock ( TRACEDB_RWLOCKDEREF ( lock ) );
    if ( status == 0 )
    {
        const tracedb_String *name;

        /* test bucket for inclusion in this tree */
        tracedb_BTNode *par = & b -> n;
        while ( par != tbl -> root )
        {
            par = tracedb_BTNodeParent ( par );
            if ( par == NULL )
            {
                tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( lock ) );
                return ENOENT;
            }
        }

        /* make it anonymous */
        name = b -> name;
        tracedb_BTreeUnlink ( tbl, & b -> n );
        b -> name = NULL;

        /* unlock table */
        tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( lock ) );

        /* test whether system is closed */
        if ( tracedb_FNSBucketNameRelease ( b ) )
        {
            tracedb_FNS *obj = b -> obj;

            /* drop reference to system */
            status = tracedb_FNSRelease ( & obj -> fs );
            if ( status != 0 )
            {
                /* system refused - reacquire lock */
                int err = tracedb_RWLockWriteLock ( TRACEDB_RWLOCKDEREF ( lock ) );
                if ( err != 0 )
                {
                    /* catastrohpic error */
                    exit ( err );
                }

                /* restore references */
                assert ( tracedb_atomic_read ( & obj -> fs . refcount ) == 0 );
                tracedb_FNSAddRef ( & obj -> fs );
                tracedb_FNSBucketAddNameRef ( b );

                /* restore name and insert into table */
                b -> name = name;
                tracedb_BTreeInsert ( tbl, & b -> n,
                                    tracedb_FNSBucketCmp );

                /* unlock and abort */
                tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( lock ) );
                return status;
            }

            /* whack bucket */
            free ( b );
        }

        /* whack name */
        tracedb_StringWhack ( ( tracedb_String* ) name );
    }

    return status;
}

int tracedb_FSUnregister ( void *ticket )
{
    return tracedb_FNSUnregister ( ticket, & tracedb_sFSNameTable, TRACEDB_RWLOCKREF ( tracedb_sFSNameLock ) );
}

int tracedb_NSUnregister ( void *ticket )
{
    return tracedb_FNSUnregister ( ticket, & tracedb_sNSNameTable, TRACEDB_RWLOCKREF ( tracedb_sNSNameLock ) );
}

/* FNSOpen
 *  finds a registered, named system
 *
 *  if "name" is NULL or the special value "stdfns",
 *  the standard descriptor will be returned.
 */
static
int tracedb_FNSOpen ( tracedb_fd_t *fd, const char *name,
    const char *stdfns, tracedb_BTree *ntbl, tracedb_RWLockRef nlock,
    tracedb_BTree *dtbl, tracedb_RWLockRef dlock )
{
    int status;

    if ( fd == NULL )
        return EINVAL;

    if ( name == NULL || strcmp ( name, stdfns ) == 0 )
    {
        * fd = 0;
        return 0;
    }

    * fd = -1;

    if ( name [ 0 ] == 0 )
        return EINVAL;

    if ( ! tracedb_sFNSInited )
        return ENOENT;

    status = tracedb_RWLockReadLock ( TRACEDB_RWLOCKDEREF ( nlock ) );
    if ( status == 0 )
    {
        tracedb_String str;
        tracedb_FNSBucket *b;

        tracedb_StringInitCtracedb_String ( & str, name );
        b = ( tracedb_FNSBucket* ) tracedb_BTreeFind ( ntbl, & str,
                            tracedb_FNSBucketFind );
        if ( b == NULL )
            status = ENOENT;
        else
        {
            status = tracedb_RWLockWriteLock ( TRACEDB_RWLOCKDEREF ( dlock ) );
            if ( status == 0 )
            {
                status = tracedb_FDSlotNew ( dtbl, fd, b );
                if ( status == 0 )
                {
                    tracedb_FNSBucketAddRef ( b );
                    * fd += 1;
                }

                tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( dlock ) );
            }
        }

        tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( nlock ) );
    }

    return status;
}

int tracedb_FSOpen ( tracedb_fs_t *fs, const char *name )
{
    return tracedb_FNSOpen ( ( tracedb_fd_t* ) fs, name, "stdfs",
        & tracedb_sFSNameTable, TRACEDB_RWLOCKREF ( tracedb_sFSNameLock ),
        & tracedb_sFSTable, TRACEDB_RWLOCKREF ( tracedb_sFSLock ) );
}

int tracedb_NSOpen ( tracedb_ns_t *ns, const char *name )
{
    return tracedb_FNSOpen ( ( tracedb_fd_t* ) ns, name, "stdns",
        & tracedb_sNSNameTable, TRACEDB_RWLOCKREF ( tracedb_sNSNameLock ),
        & tracedb_sNSTable, TRACEDB_RWLOCKREF ( tracedb_sNSLock ) );
}

/* FNSCopy
 *  create a copy of system descriptor
 *
 *  "copy" [ OUT ] - return parameter for copy
 *
 *  return values:
 */
static
int tracedb_FNSCopy ( tracedb_fd_t self, tracedb_fd_t *copy,
    tracedb_BTree *tbl, tracedb_RWLockRef lock )
{
    int status;

    if ( copy == NULL )
        return EINVAL;

    if ( self == 0 )
    {
        * copy = 0;
        return 0;
    }

    if ( ! tracedb_sFNSInited || self < 0 )
        return EBADF;

    status = tracedb_RWLockWriteLock ( TRACEDB_RWLOCKDEREF ( lock ) );
    if ( status == 0 )
    {
        tracedb_FNSBucket *b;
        status = tracedb_FDSlotGet ( tbl, self - 1, ( void** ) & b );
        if ( status == 0 )
        {
            tracedb_fd_t idx;
            status = tracedb_FDSlotNew ( tbl, & idx, b );
            if ( status == 0 )
            {
                * copy = idx + 1;
                tracedb_FNSBucketAddRef ( b );
            }
        }

        tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( lock ) );
    }
    return status;
}

int tracedb_FSCopy ( tracedb_fs_t self, tracedb_fs_t *copy )
{
    return tracedb_FNSCopy ( ( tracedb_fd_t ) self,
        ( tracedb_fd_t* ) copy,
        & tracedb_sFSTable, TRACEDB_RWLOCKREF ( tracedb_sFSLock ) );
}

int tracedb_NSCopy ( tracedb_ns_t self, tracedb_ns_t *copy )
{
    return tracedb_FNSCopy ( ( tracedb_fd_t ) self,
        ( tracedb_fd_t* ) copy,
        & tracedb_sNSTable, TRACEDB_RWLOCKREF ( tracedb_sNSLock ) );
}

/* FNSWhack
 *  closes an fd_t returned by FNSOpen or FNSOpenAnon
 */
static
int tracedb_FNSWhack ( tracedb_fd_t fd,
        tracedb_BTree *tbl, tracedb_RWLockRef lock )
{
    int status;

    if ( fd == 0 )
        return 0;

    if ( ! tracedb_sFNSInited || fd <= 0 )
        return EBADF;

    /* convert fd to zero-based */
    fd -= 1;

    status = tracedb_RWLockWriteLock ( TRACEDB_RWLOCKDEREF ( lock ) );
    if ( status == 0 )
    {
        tracedb_FNSBucket *b;
        tracedb_FDSlotBucket *sb;

        /* invalidate fd and get bucket */
        status = tracedb_FDSlotInvalidate ( tbl, fd, & sb, ( void** ) & b );
        if ( status == 0 )
        {
            tracedb_FNS *obj = b -> obj;

            /* discard open reference to bucket */
            if ( tracedb_FNSBucketRelease ( b ) == 0 )
            {
                /* bucket should be whacked
                   discard its reference to system */
                if ( tracedb_FNSDropRef ( & obj -> fs ) )
                {
                    int err;

                    /* must release lock to avoid potential deadlock */
                    tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( lock ) );

                    /* ask object to close, which can be refused */
                    status = ( * obj -> fs . vt -> whack ) ( & obj -> fs );

                    /* reclaim lock */
                    err = tracedb_RWLockWriteLock ( TRACEDB_RWLOCKDEREF ( lock ) );
                    if ( err != 0 )
                    {
                        /* catastrophic error */
                        exit ( err );
                    }

                    /* if object refused to close */
                    if ( status != 0 )
                    {
                        /* restore references */
                        assert ( tracedb_atomic_read ( & obj -> fs . refcount ) == 0 );
                        tracedb_FNSAddRef ( & obj -> fs );
                        tracedb_FNSBucketAddRef ( b );

                        /* restore slot */
                        tracedb_FDSlotReset ( fd, sb, b );

                        /* abandon lock and abort */
                        tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( lock ) );
                        return status;
                    }
                }

                /* whack bucket */
                assert ( b -> name == NULL );
                free ( b );
            }

            /* whack slot */
            tracedb_FDSlotWhack ( tbl, fd, sb );
        }

        tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( lock ) );
    }

    return status;
}

int tracedb_FSWhack ( tracedb_fs_t fs )
{
    return tracedb_FNSWhack ( ( tracedb_fd_t ) fs, & tracedb_sFSTable, TRACEDB_RWLOCKREF ( tracedb_sFSLock ) );
}

int tracedb_NSWhack ( tracedb_ns_t ns )
{
    return tracedb_FNSWhack ( ( tracedb_fd_t ) ns, & tracedb_sNSTable, TRACEDB_RWLOCKREF ( tracedb_sNSLock ) );
}

/* FNSFind
 */
static
int tracedb_FNSFind ( tracedb_fd_t fd, void **obj, tracedb_BTree *tbl,
    tracedb_RWLockRef lock )
{
    int status;

    if ( ! tracedb_sFNSInited || fd <= 0 )
        return EBADF;

    status = tracedb_RWLockReadLock ( TRACEDB_RWLOCKDEREF ( lock ) );
    if ( status == 0 )
    {
        tracedb_FNSBucket *b;
        status = tracedb_FDSlotGet ( tbl, fd - 1, ( void** ) & b );
        if ( status == 0 )
        {
            assert ( tracedb_atomic_read ( & ( ( tracedb_FS* ) b -> obj ) -> refcount ) > 0 );
            tracedb_FNSAddRef ( ( tracedb_FS* ) b -> obj );
            * obj = b -> obj;
        }

        tracedb_RWLockUnlock ( TRACEDB_RWLOCKDEREF ( lock ) );
    }
    return status;
}

int tracedb_FSFind ( tracedb_fs_t fs, tracedb_FS **obj )
{
    return tracedb_FNSFind ( ( tracedb_fd_t ) fs, ( void** ) obj,
        & tracedb_sFSTable, TRACEDB_RWLOCKREF ( tracedb_sFSLock ) );
}

int tracedb_NSFind ( tracedb_ns_t ns, tracedb_NS **obj )
{
    return tracedb_FNSFind ( ( tracedb_fd_t ) ns, ( void** ) obj,
        & tracedb_sNSTable, TRACEDB_RWLOCKREF ( tracedb_sNSLock ) );
}


/*--------------------------------------------------------------------------
 * FD
 *  base class of all fd operations
 *  supports streaming, copying, renumbering and closing
 */

/* FDFind
 */
int tracedb_FDFind ( tracedb_fd_t fd, tracedb_FD **obj )
{
    int status;

    if ( ! tracedb_sFNSInited || fd > -2 )
        return EBADF;

    status = tracedb_RWLockReadLock ( tracedb_sFDLock );
    if ( status == 0 )
    {
        status = tracedb_FDSlotGet ( & tracedb_sFDTable, -2 - fd, ( void** ) obj );
        if ( status == 0 )
            tracedb_FDAddRef ( * obj );
        tracedb_RWLockUnlock ( tracedb_sFDLock );
    }
    return status;
}

/* FNSNewFD
 */
int tracedb_FNSNewFD ( tracedb_fd_t *fd, tracedb_FD *obj )
{
    int status = tracedb_RWLockWriteLock ( tracedb_sFDLock );
    if ( status == 0 )
    {
        tracedb_fd_t idx;
        status = tracedb_FDSlotNew ( & tracedb_sFDTable, & idx, obj );
        tracedb_RWLockUnlock ( tracedb_sFDLock );

        if ( status == 0 )
        {
            * fd = -2 - idx;
            return 0;
        }
    }
    return status;
}

/* FNSDupFD
 *  creates a duplicate file descriptor
 *  attaches a new reference to src object
 */
int tracedb_FNSDupFD ( tracedb_fd_t src, tracedb_fd_t *dst )
{
    int status;

    if ( ! tracedb_sFNSInited || src > -2 )
        return EBADF;

    status = tracedb_RWLockWriteLock ( tracedb_sFDLock );
    if ( status == 0 )
    {
        tracedb_FD *obj;
        status = tracedb_FDSlotGet ( & tracedb_sFDTable, -2 - src,
                                    ( void** ) & obj );
        if ( status == 0 )
        {
            tracedb_fd_t idx;
            status = tracedb_FDSlotNew ( & tracedb_sFDTable, & idx, obj );
            if ( status == 0 )
            {
                tracedb_FDAddRef ( obj );
                * dst = -2 - idx;
            }
        }
        tracedb_RWLockUnlock ( tracedb_sFDLock );
    }
    return status;
}

/* FNSAssignFD
 *  assigns the object from src file descriptor
 *  to the dst file descriptor
 *  attaches a new reference to src object
 *  releases reference to dst object if it existed
 */
int tracedb_FNSAssignFD ( tracedb_fd_t src, tracedb_fd_t dst )
{
    int status;

    if ( ! tracedb_sFNSInited || src > -2 || dst > -2 )
        return EBADF;

    status = tracedb_RWLockWriteLock ( tracedb_sFDLock );
    if ( status == 0 )
    {
        tracedb_FD *obj, *prior;
        status = tracedb_FDSlotGet ( & tracedb_sFDTable, -2 - src,
                                    ( void** ) & obj );
        if ( status == 0 )
        {
            status = tracedb_FDSlotSet ( & tracedb_sFDTable, -2 - dst,
                                    obj, ( void** ) & prior );
            if ( status == 0 )
                tracedb_FDAddRef ( obj );
        }
        tracedb_RWLockUnlock ( tracedb_sFDLock );

        if ( status == 0 && prior != NULL )
            status = tracedb_FDRelease ( prior );
    }
    return status;
}

/* FNSRenumberFD
 *  moves object from src fd to dst fd
 *  releases any prior dst object
 *  closes src fd
 */
int tracedb_FNSRenumberFD ( tracedb_fd_t src, tracedb_fd_t dst )
{
    int status;

    if ( ! tracedb_sFNSInited || src > -2 || dst > -2 )
        return EBADF;

    status = tracedb_RWLockWriteLock ( tracedb_sFDLock );
    if ( status == 0 )
    {
        tracedb_FD *obj, *prior;
        tracedb_FDSlotBucket *sb;
        status = tracedb_FDSlotInvalidate ( & tracedb_sFDTable, -2 - src,
                                        & sb, ( void** ) & obj );
        if ( status == 0 )
        {
            tracedb_FDSlotWhack ( & tracedb_sFDTable, -2 - src, sb );
            status = tracedb_FDSlotSet ( & tracedb_sFDTable, -2 - dst,
                                        obj, ( void** ) & prior );
        }

        tracedb_RWLockUnlock ( tracedb_sFDLock );

        if ( status == 0 && prior != NULL )
            status = tracedb_FDRelease ( prior );
    }
    return status;
}

/* FNSWhackFD
 *  releases object
 *  closes slot
 */
int tracedb_FNSWhackFD ( tracedb_fd_t fd )
{
    int status;

    if ( ! tracedb_sFNSInited || fd > -2 )
        return EBADF;

    /* convert into unsigned zero-based space */
    fd = -2 - fd;

    status = tracedb_RWLockWriteLock ( tracedb_sFDLock );
    if ( status == 0 )
    {
        tracedb_FD *obj;
        tracedb_FDSlotBucket *sb;

        /* invalidate fd and get object */
        status = tracedb_FDSlotInvalidate ( & tracedb_sFDTable, fd,
                                    & sb, ( void** ) & obj );
        if ( status == 0 )
        {
            /* in the easy case only the slot gets whacked */
            if ( ! tracedb_FDDropRef ( obj ) )
                tracedb_FDSlotWhack ( & tracedb_sFDTable, fd, sb );
            else
            {
                int err;

                /* in the hard case, release the lock
                   to avoid potential deadlock */
                tracedb_RWLockUnlock ( tracedb_sFDLock );

                /* ask the object to whack itself */
                status = ( * obj -> md -> whack ) ( obj );

                /* reacquire the lock */
                err = tracedb_RWLockWriteLock ( tracedb_sFDLock );
                if ( err != 0 )
                {
                    exit ( err );
                }

                /* if object refused, restore state */
                if ( status != 0 )
                {
                    tracedb_FDAddRef ( obj );
                    tracedb_FDSlotReset ( fd, sb, obj );
                }

                /* otherwise, whack the slot */
                else
                {
                    tracedb_FDSlotWhack ( & tracedb_sFDTable, fd, sb );
                }
            }
        }

        tracedb_RWLockUnlock ( tracedb_sFDLock );
    }
    return status;
}
