#ifndef _h_vfd_vfdio_
#define _h_vfd_vfdio_

#ifndef _h_timeout_
#include "timeout.h"
#endif

#ifndef _h_fdio_
#include "fdio.h"
#endif

#ifndef _h_fdset_
#include "fdset.h"
#endif

#ifndef _h_trace_atomic_
#include "trace_atomic.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * virtual extensions to fdio interface
 */

/* VFSInit - IDEMPOTENT
 *  initialize virtual file system manager
 *  must be invoked before any other vfdio operations
 */
int tracedb_VFSInit ( void );

/* FSRegister
 *  register a file system handler
 *  attach a reference to object
 *
 *  "ticket" [ OUT ] - return parameter for ticket
 *  used to unregister file system later
 *
 *  "name" [ IN ] - null terminated file system name
 *
 *  return values:
 *    EEXIST - a file system is already registered under given name
 */
int tracedb_FSRegister ( tracedb_fs_t self, void **ticket, const char *name );

/* FSUnregister
 *  removes an FS object from the registry,
 *  as identified by a ticket returned from FSRegister
 */
int tracedb_FSUnregister ( void *ticket );


/*--------------------------------------------------------------------------
 * implementation details - forwards
 */
typedef struct tracedb_FS tracedb_FS;
typedef struct tracedb_FD tracedb_FD;
typedef struct tracedb_FDFile tracedb_FDFile;


/*--------------------------------------------------------------------------
 * FD
 *  file descriptor based stream
 */
typedef struct tracedb_vtFD tracedb_vtFD;
struct tracedb_vtFD
{
    /* streaming read & write operations */
    int ( * read ) ( tracedb_FD *self, void *buffer, size_t max_bytes, size_t *num_read );
    int ( * write ) ( tracedb_FD *self, const void *buffer, size_t bytes, size_t *num_writ );
};

typedef struct tracedb_mdFD tracedb_mdFD;
struct tracedb_mdFD
{
    /* a privately defined constant, identifying
       more or less the "class" of fd. it is typed
       as a "size_t" to ensure proper pointer alignment */
    size_t variant;

    /* if classname metadata is to be generated,
       here is a const classname string */
#if VFD_CLASSNAMES
    const char *name;
#endif

    /* invoked when the last reference of an FD has
       been released and the object can be whacked */
    int ( * whack ) ( tracedb_FD *self );

    /* the FD-level vtbl */
    const tracedb_vtFD *vt1;

};

struct tracedb_FD
{
    /* class metadata */
    const tracedb_mdFD *md;

    /* all FD objects are reference counted */
    tracedb_atomic_t refcount;
};

/* FDInit
 *  initialize parent portion of an FD implementation
 *
 *  "meta" [ IN ] - implementation specific const mdFD*
 *
 *  "refs" [ IN ] - initial refcount of object, normally 1
 *  given that FD objects are created on demand and minimally
 *  are referenced by the object's file descriptor.
 */
#define tracedb_FDInit( self, meta, refs ) \
    ( void ) ( ( self ) -> md = ( meta ), \
               tracedb_atomic_set ( & ( self ) -> refcount, ( refs ) ) )

/* FDAddRef
 *  increase reference count
 */
#define tracedb_FDAddRef( self ) \
    tracedb_atomic_inc ( & ( self ) -> refcount )

/* FDDropRef
 *  release a reference to FD
 */
#define tracedb_FDDropRef( self ) \
    tracedb_atomic_dec_and_test ( & ( self ) -> refcount )

/* FDRelease
 *  release a reference to FD
 */
#define tracedb_FDRelease( self ) \
    ( ( tracedb_atomic_dec_and_test ( & ( self ) -> refcount ) ) ? \
      ( * ( self ) -> md -> whack ) ( self ) : 0 )


/*--------------------------------------------------------------------------
 * FDFile
 *  virtual table for a FDFile file descriptor
 */
typedef struct tracedb_vtFDFile tracedb_vtFDFile;
struct tracedb_vtFDFile
{
    /* standard operations on files described in fdio.h */
    int ( * lockRgn ) ( tracedb_FDFile *self, bool exclusive, uint64_t bytes, uint64_t pos );
    int ( * unlockRgn ) ( tracedb_FDFile *self, uint64_t bytes, uint64_t pos );
    int ( * size ) ( const tracedb_FDFile *self, uint64_t *size );
    int ( * setSize ) ( tracedb_FDFile *self, uint64_t size );
    int ( * position ) ( const tracedb_FDFile *self, uint64_t *pos );
    int ( * reposition ) ( tracedb_FDFile *self, uint64_t pos );
    int ( * seek ) ( tracedb_FDFile *self, int64_t offset, int origin, uint64_t *pos );
    int ( * pread ) ( tracedb_FDFile *self, uint64_t pos, void *buffer, size_t max_bytes, size_t *num_read );
    int ( * pwrite ) ( tracedb_FDFile *self, uint64_t pos, const void *buffer, size_t bytes, size_t *num_writ );
};

typedef struct tracedb_mdFDFile tracedb_mdFDFile;
struct tracedb_mdFDFile
{
    /* a privately defined constant, identifying
       more or less the "class" of fd. it is typed
       as a "size_t" to ensure proper pointer alignment */
    size_t variant;

    /* if classname metadata is to be generated,
       here is a const classname string */
#if VFD_CLASSNAMES
    const char *name;
#endif

    /* invoked when the last reference of an FD has
       been released and the object can be whacked */
    int ( * whack ) ( tracedb_FDFile *self );

    /* the FD-level vtbl */
    const tracedb_vtFD *vt1;

    /* the FDFile-level vtbl */
    const tracedb_vtFDFile *vt2;
};

struct tracedb_FDFile
{
    /* see comments in FD */
    const tracedb_mdFDFile *md;
    tracedb_atomic_t refcount;
};

/* FDFileInit
 *  initialize parent portion of an FDFile implementation
 *
 *  "meta" [ IN ] - implementation specific const mdFDFile*
 *
 *  "refs" [ IN ] - initial refcount of object, normally 1
 *  given that FD objects are created on demand and minimally
 *  are referenced by the object's file descriptor.
 */
#define tracedb_FDFileInit( self, meta, refs ) \
    ( void ) ( ( self ) -> md = ( meta ), \
               tracedb_atomic_set ( & ( self ) -> refcount, ( refs ) ) )


/*--------------------------------------------------------------------------
 * FS
 *  virtual table for a file system object
 */
typedef struct tracedb_vtFS tracedb_vtFS;
struct tracedb_vtFS
{
    /* invoked when the last reference of an FS has
       been released and the object can be whacked */
    int ( * whack ) ( tracedb_FS *self );

    /* standard operations on fs described in fdio.h */
    bool ( * exists ) ( const tracedb_FS *self, const char *path );
    bool ( * isDir ) ( const tracedb_FS *self, const char *path );
    int ( * resolve ) ( const tracedb_FS *self, const char *path, char *resolved, size_t max_size );
    int ( * vprintPath ) ( const tracedb_FS *self, char *path, size_t bsize, size_t *psize, const char *fmt, va_list args );
    int ( * createDir ) ( tracedb_FS *self, const char *path, mode_t mode, bool parents );

    /* create/return a reference to DirList
       that gets whacked by corresponding method */
    int ( * listDir ) ( const tracedb_FS *self, tracedb_DirList *dl, const char *path );
    int ( * listWhack ) ( const tracedb_FS *self, tracedb_DirList *dl );

    int ( * removeDir ) ( tracedb_FS *self, const char *path, bool force );
    int ( * remove ) ( tracedb_FS *self, const char *path );
    int ( * rename ) ( tracedb_FS *self, const char *from, const char *to );
    int ( * fileSize ) ( const tracedb_FS *self, const char *path, uint64_t *size );
    int ( * setFileSize ) ( tracedb_FS *self, const char *path, uint64_t size );
    int ( * changeMode ) ( tracedb_FS *self, const char *path, mode_t mode, mode_t mask, bool recurse );

    /* return a full fd, ready to go.
       if NULL, implementation invokes FDFile based interface,
       but may be overridden if operation can be chained */
    int ( * openFD ) ( tracedb_FS *self, tracedb_fd_t *fd, const char *path, int perm );
    int ( * createFD ) ( tracedb_FS *self, tracedb_fd_t *f, const char *path, int perm, mode_t mode );

    /* create/return a reference to FDFile
       that gets whacked when file is closed */
    int ( * openFile ) ( tracedb_FS *self, tracedb_FDFile **f, const char *path, int perm );
    int ( * createFile ) ( tracedb_FS *self, tracedb_FDFile **f, const char *path, int perm, mode_t mode );

    /* select function on several fds */
    int ( * select ) ( tracedb_FS *self, tracedb_FDSet *read_set, tracedb_FDSet *write_set, tracedb_FDSet *except_set, tracedb_timeoutref_t tm );
};

struct tracedb_FS
{
    /* base vTable */
    const tracedb_vtFS *vt;

    /* all FS objects are reference counted */
    tracedb_atomic_t refcount;
};

/* FSInit
 *  initialize the parent portion of a file system implementation
 *
 *  "vtbl" [ IN ] - an implementation-specific const vtFS*
 */
#define tracedb_FSInit( self, vtbl ) \
    ( void ) ( ( self ) -> vt = ( vtbl ), \
               tracedb_atomic_set ( & ( self ) -> refcount, 0 ) )

/* FNSAddRef
 *  adds a reference to system
 */
#define tracedb_FNSAddRef( self ) \
    tracedb_atomic_inc ( & ( self ) -> refcount )

/* FNSDropRef
 *  release a reference to system
 */
#define tracedb_FNSDropRef( self ) \
    tracedb_atomic_dec_and_test ( & ( self ) -> refcount )

/* FNSRelease
 *  release a reference to system
 *  and whack object when gone
 */
#define tracedb_FNSRelease( self ) \
    ( tracedb_atomic_dec_and_test ( & ( self ) -> refcount ) ?  \
      ( * ( self ) -> vt -> whack ) ( self ) : 0 )


#ifdef __cplusplus
}
#endif

#endif /* _h_vfd_vfdio_ */
