#include "vfd-priv.h"
#include "directory.h"

#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*--------------------------------------------------------------------------
 * stdfs forwards
 */
static bool tracedb_StdFSExists ( tracedb_fs_t fs, const char *path );
static bool tracedb_StdFSIsDir ( tracedb_fs_t fs, const char *path );
static int tracedb_StdFSResolve ( tracedb_fs_t fs, const char *path, char *resolved, size_t max_size );
static int tracedb_StdFSVPrintPath ( tracedb_fs_t fs, char *path, size_t bsize, size_t *psize, const char *fmt, va_list args );
static int tracedb_StdFSCreateDir ( tracedb_fs_t fs, const char *path, mode_t mode, bool parents );
static int tracedb_StdFSListDir ( tracedb_fs_t fs, tracedb_DirList *dl, const char *path );
static int tracedb_StdFSRemoveDir ( tracedb_fs_t fs, const char *path, bool force );
static int tracedb_StdFSRemove ( tracedb_fs_t fs, const char *path );
static int tracedb_StdFSRename ( tracedb_fs_t fs, const char *from, const char *to );
static int tracedb_StdFSFileSize ( tracedb_fs_t fs, const char *path, uint64_t *eof );
static int tracedb_StdFSSetFileSize ( tracedb_fs_t fs, const char *path, uint64_t eof );
static int tracedb_StdFSChangeMode ( tracedb_fs_t fs, const char *path, mode_t mode, mode_t mask, bool recurse );
static int tracedb_StdFSOpenFile ( tracedb_fs_t fs, tracedb_fd_t *fd, const char *path, int perm );
static int tracedb_StdFSCreateFile ( tracedb_fs_t fs, tracedb_fd_t *fd, const char *path, int perm, mode_t mode );
static int tracedb_StdFDCopy ( tracedb_fd_t src, tracedb_fd_t *dst );
static int tracedb_StdFDAssign ( tracedb_fd_t src, tracedb_fd_t dst );
static int tracedb_StdFDRenumber ( tracedb_fd_t src, tracedb_fd_t dst );
static int tracedb_StdFDRead ( tracedb_fd_t fd, void *buffer, size_t max_bytes, size_t *num_read );
static int tracedb_StdFDWrite ( tracedb_fd_t fd, const void *buffer, size_t bytes, size_t *num_writ );
static int tracedb_StdFDWhack ( tracedb_fd_t fd );
static int tracedb_StdFDFileLockRgn ( tracedb_fd_t fd, bool exclusive, uint64_t bytes, uint64_t pos );
static int tracedb_StdFDFileUnlockRgn ( tracedb_fd_t fd, uint64_t bytes, uint64_t pos );
static int tracedb_StdFDFileSize ( tracedb_fd_t fd, uint64_t *size );
static int tracedb_StdFDFileSetSize ( tracedb_fd_t fd, uint64_t size );
static int tracedb_StdFDFilePosition ( tracedb_fd_t fd, uint64_t *pos );
static int tracedb_StdFDFileReposition ( tracedb_fd_t fd, uint64_t pos );
static int tracedb_StdFDFileSeek ( tracedb_fd_t fd, int64_t offset, int origin, uint64_t *pos );
static int tracedb_StdFDFileRead ( tracedb_fd_t fd, uint64_t pos, void *buffer, size_t max_bytes, size_t *num_read );
static int tracedb_StdFDFileWrite ( tracedb_fd_t fd, uint64_t pos, const void *buffer, size_t bytes, size_t *num_writ );
static int tracedb_StdDirListWhack ( tracedb_DirList *dl );


/*--------------------------------------------------------------------------
 * FS
 *  path operations on file system
 *
 *  by definition all paths are simply
 *  null-terminated 8-bit character strings
 *
 *  the strings themselves are either ASCII or UTF-8
 *  or can be opaque to the code
 */

/* FSExists
 *  determines if file or directory exists
 */
bool tracedb_FSExists ( tracedb_fs_t fs, const char *path )
{
    int status;

    tracedb_FS *obj;
    bool exists;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSExists ( fs, path );

    if ( path == NULL || path [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status != 0 )
        exists = false;
    else
    {
        exists = ( * obj -> vt -> exists ) ( obj, path );
        tracedb_FNSRelease ( obj );
    }
    return exists;
}

/* FSIsDir
 *  determines if path represents a file or directory
 */
bool tracedb_FSIsDir ( tracedb_fs_t fs, const char *path )
{
    int status;

    tracedb_FS *obj;
    bool isDir;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSIsDir ( fs, path );

    if ( path == NULL || path [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status != 0 )
        isDir = false;
    else
    {
        isDir = ( * obj -> vt -> isDir ) ( obj, path );
        tracedb_FNSRelease ( obj );
    }
    return isDir;
}

/* FSResolve
 *  gives canonical path
 *
 *  returns status code
 */
int tracedb_FSResolve ( tracedb_fs_t fs,
    const char *path, char *resolved, size_t max_size )
{
    int status;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSResolve ( fs, path, resolved, max_size );

    if ( path == NULL || resolved == NULL || path [ 0 ] == 0 || max_size == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> vt -> resolve ) ( obj, path, resolved, max_size );
        tracedb_FNSRelease ( obj );
    }
    return status;
}

/* FSVPrintPath
 *  perform snprintf function into buffer as a path string
 *
 *  "path" [ OUT ] and "bsize" [ IN ] - describe the output path buffer
 *
 *  "psize" [ OUT, NULL OKAY ] - the resultant size of path in bytes
 *  upon success, excluding terminating NULL byte.
 *
 *  "fmt" [ IN ] and "args" [ IN ] - standard format arguments
 *
 *  returns values:
 *    ENAMETOOLONG if buffer size is too small or exceeds fs path max
 */
int tracedb_FSVPrintPath ( tracedb_fs_t fs,
    char *path, size_t bsize, size_t *psize,
    const char *fmt, va_list args )
{
    int status;
    size_t psizebuff;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSVPrintPath ( fs, path, bsize, psize, fmt, args );

    if ( psize == NULL )
        psize = & psizebuff;

    if ( ( path == NULL && bsize != 0 ) || fmt == NULL || args == NULL )
    {
        * psize = 0;
        return EINVAL;
    }

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> vt -> vprintPath ) ( obj, path, bsize, psize, fmt, args );
        tracedb_FNSRelease ( obj );
    }
    return status;
}

/* FSCreateDir
 *  cause a directory to be created with the specified mode
 *
 *  "mode" is a standard Unix directory mode
 *
 *  if "parents" is true, then any missing parent directories
 *  in the path are created as well.
 *
 *  NB - if the directory already exists, its mode will not be changed.
 *
 *  returns EEXIST if the directory already exists
 *
 *  returns ENOTDIR if the path or one of its parent components
 *  is not actually a directory
 *
 *  returns ENOENT if a parent component of the path does not
 *  exist or is a dangling symbolic link
 */
int tracedb_FSCreateDir ( tracedb_fs_t fs,
    const char *path, mode_t mode, bool parents )
{
    int status;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSCreateDir ( fs, path, mode, parents );

    if ( path == NULL || path [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> vt -> createDir ) ( obj, path, mode, parents );
        tracedb_FNSRelease ( obj );
    }
    return status;
}

/* FSListDir
 *  get a directory listing
 *
 *  requires include of directory.h
 */
int tracedb_FSListDir ( tracedb_fs_t fs, tracedb_DirList *dl,
    const char *path )
{
    int status;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSListDir ( fs, dl, path );

    if ( dl == NULL || path == NULL || path [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> vt -> listDir ) ( obj, dl, path );
        tracedb_FNSRelease ( obj );
    }
    return status;
}

/* FSRemoveDir
 *  delete a directory from the file system
 *  if "force" is false, the directory must be empty
 *  otherwise, a recursive delete will be performed.
 */
int tracedb_FSRemoveDir ( tracedb_fs_t fs,
    const char *path, bool force )
{
    int status;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSRemoveDir ( fs, path, force );

    if ( path == NULL || path [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> vt -> removeDir ) ( obj, path, force );
        tracedb_FNSRelease ( obj );
    }
    return status;
}

/* FSRemove
 *  removes an object from the file system
 */
int tracedb_FSRemove ( tracedb_fs_t fs, const char *path )
{
    int status;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSRemove ( fs, path );

    if ( path == NULL || path [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> vt -> remove ) ( obj, path );
        tracedb_FNSRelease ( obj );
    }
    return status;
}

/* FSRename
 *  renames or moves a file from one location to another
 */
int tracedb_FSRename ( tracedb_fs_t fs,
    const char *from, const char *to )
{
    int status;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSRename ( fs, from, to );

    if ( from == NULL || to == NULL || from [ 0 ] == 0 || to [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> vt -> rename ) ( obj, from, to );
        tracedb_FNSRelease ( obj );
    }
    return status;
}

/* FSFileSize
 *  returns file size in "eof"
 */
int tracedb_FSFileSize ( tracedb_fs_t fs,
    const char *path, uint64_t *eof )
{
    int status;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSFileSize ( fs, path, eof );

    if ( path == NULL || eof == NULL || path [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> vt -> fileSize ) ( obj, path, eof );
        tracedb_FNSRelease ( obj );
    }
    return status;
}

/* FSSetFileSize
 *  sets end of file by path
 */
int tracedb_FSSetFileSize ( tracedb_fs_t fs,
    const char *path, uint64_t eof )
{
    int status;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSSetFileSize ( fs, path, eof );

    if ( path == NULL || path [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> vt -> setFileSize ) ( obj, path, eof );
        tracedb_FNSRelease ( obj );
    }
    return status;
}


/* FSChangeMode
 *  attempts to change access mod of path
 *
 *  "path" [ IN ] - NUL terminated path to entry
 *
 *  "mode" [ IN ] - a standard Unix file mode containing
 *  new bit values
 *
 *  "mask" [ IN ] - a standard Unix file mode containing
 *  a 1 for every bit to affect. e.g. "mode"=>0, "mask"=>0222
 *  would remove write access. a mask of ZERO is equivalent
 *  to 07777, i.e. all bits.
 *
 *  "recurse" [ IN ] - in the case of a directory,
 *  apply changes recursively if value is true.
 */
int tracedb_FSChangeMode ( tracedb_fs_t fs, const char *path,
    mode_t mode, mode_t mask, bool recurse )
{
    int status;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSChangeMode ( fs, path, mode, mask, recurse );

    if ( path == NULL || path [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> vt -> changeMode ) ( obj, path, mode, mask, recurse );
        tracedb_FNSRelease ( obj );
    }
    return status;
}

/* FSOpenFile
 *  open a named file or device with supplied permission
 *
 *  "perm" is one or more FSOpenPerm bits or'd together
 *
 *  "FSOpenFixed" opens on a specific fd
 */
int tracedb_FSOpenFile ( tracedb_fs_t fs, tracedb_fd_t *fd,
    const char *path, int perm )
{
    int status;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSOpenFile ( fs, fd, path, perm );

    if ( fd == NULL || path == NULL || path [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        tracedb_FDFile *f;

        if ( obj -> vt -> openFD != NULL )
        {
            status = ( * obj -> vt -> openFD ) ( obj, fd, path, perm );
            tracedb_FNSRelease ( obj );
            return status;
        }

        status = ( * obj -> vt -> openFile ) ( obj, & f, path, perm );
        tracedb_FNSRelease ( obj );

        if ( status == 0 )
        {
            status = tracedb_FNSNewFD ( fd, ( tracedb_FD* ) f );
            if ( status == 0 )
                return 0;

            assert ( f != NULL );
            if ( tracedb_atomic_dec_and_test ( & f -> refcount ) )
                ( * f -> md -> whack ) ( f );
        }
    }

    * fd = -1;
    return status;
}

/* FSCreateFile
 *  creates the file if it does not exist
 *  opens the named file with supplied permissions
 *
 *  "mode" is a standard Unix file mode, and is used only to
 *  create a file, i.e. an existing file does not have its
 *  access mode changed.
 *
 *  "FSCreateFixed" creates on a specific fd
 */
int tracedb_FSCreateFile ( tracedb_fs_t fs, tracedb_fd_t *fd,
    const char *path, int perm, mode_t mode )
{
    int status;

    tracedb_FS *obj;

    if ( fs == tracedb_stdfs )
        return tracedb_StdFSCreateFile ( fs, fd, path, perm, mode );

    if ( fd == NULL || path == NULL || path [ 0 ] == 0 )
        return EINVAL;

    status = tracedb_FSFind ( fs, & obj );
    if ( status == 0 )
    {
        tracedb_FDFile *f;

        if ( obj -> vt -> createFD != NULL )
        {
            status = ( * obj -> vt -> createFD ) ( obj, fd, path, perm, mode );
            tracedb_FNSRelease ( obj );
            return status;
        }

        status = ( * obj -> vt -> createFile ) ( obj, & f, path, perm, mode );
        tracedb_FNSRelease ( obj );

        if ( status == 0 )
        {
            status = tracedb_FNSNewFD ( fd, ( tracedb_FD* ) f );
            if ( status == 0 )
               return 0;

            assert ( f != NULL );
            if ( tracedb_atomic_dec_and_test ( & f -> refcount ) )
                ( * f -> md -> whack ) ( f );
        }
    }

    * fd = -1;
    return status;
}


/*--------------------------------------------------------------------------
 * DirList
 */

/* DirListWhack
 *  destroy dirlist
 */
int tracedb_DirListWhack ( tracedb_DirList *dl )
{
    int status;

    tracedb_FS *obj;

    if ( dl == NULL )
        return 0;

    if ( dl -> fs == tracedb_stdfs )
        return tracedb_StdDirListWhack ( dl );

    status = tracedb_FSFind ( dl -> fs, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> vt -> listWhack ) ( obj, dl );
        tracedb_FNSRelease ( obj );
    }
    return status;
}


/*--------------------------------------------------------------------------
 * FD
 *  base class of all fd operations
 *  supports streaming, copying, renumbering and closing
 */

/* FDCopy
 *  create a copy of a file descriptor
 *  returns new fd in "dst"
 */
int tracedb_FDCopy ( tracedb_fd_t src, tracedb_fd_t *dst )
{
    if ( src >= 0 )
        return tracedb_StdFDCopy ( src, dst );

    if ( dst == NULL )
        return EINVAL;

    * dst = -1;

    return tracedb_FNSDupFD ( src, dst );
}

/* FDAssign
 *  assigns a copy of a file descriptor to a specific fd
 *  will close the target if initially open
 */
int tracedb_FDAssign ( tracedb_fd_t src, tracedb_fd_t dst )
{
    if ( src >= 0 )
        return tracedb_StdFDAssign ( src, dst );

    if ( src == dst )
        return EBUSY;

    return tracedb_FNSAssignFD ( src, dst );
}

/* FDRenumber
 *  reassigns an open fd to a new number
 *
 *  internally performs an FDAssign to dst and a FDWhack on src
 *  but only if dst != src, and the dst number belongs to
 *  the src fd space.
 */
int tracedb_FDRenumber ( tracedb_fd_t src, tracedb_fd_t dst )
{
    if ( src >= 0 )
        return tracedb_StdFDRenumber ( src, dst );

    if ( src == dst )
        return 0;

    return tracedb_FNSRenumberFD ( src, dst );
}

/* FDRead
 *  read some number of bytes from the stream
 *  returns the number of bytes read in "num_read"
 *
 *  "num_read" may be less than "max_bytes", including 0
 *  which indicates an end-of-stream condition
 *
 *  any non-zero error code means that NO bytes were read
 *  due to an error condition, such as a closed stream or
 *  an interrupt.
 */
int tracedb_FDRead ( tracedb_fd_t fd,
    void *buffer, size_t max_bytes, size_t *num_read )
{
    int status;

    tracedb_FD *obj;

    if ( fd >= 0 )
        return tracedb_StdFDRead ( fd, buffer, max_bytes, num_read );

    if ( max_bytes == 0 )
    {
        if ( num_read != NULL )
            * num_read = 0;
        return 0;
    }

    if ( buffer == NULL )
        return EINVAL;

    status = tracedb_FDFind ( fd, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> md -> vt1 -> read ) ( obj, buffer, max_bytes, num_read );
        tracedb_FDRelease ( obj );
    }
    return status;
}

/* FDWrite
 *  write a specified number of bytes to the stream
 *  returns the number of bytes written in "num_writ"
 *
 *  "num_writ" may be less than "len", but still > 0
 *  in cases where buffering or blocksize is not transparent.
 *  it may also be the case for end-of-media or broken-pipe
 *  errors where a partial write was successful but there are
 *  still data remaining.
 */
int tracedb_FDWrite ( tracedb_fd_t fd,
    const void *buffer, size_t bytes, size_t *num_writ )
{
    int status;

    tracedb_FD *obj;

    if ( fd >= 0 )
        return tracedb_StdFDWrite ( fd, buffer, bytes, num_writ );

    if ( bytes == 0 )
    {
        if ( num_writ != NULL )
            * num_writ = 0;
        return 0;
    }

    if ( buffer == NULL )
        return EINVAL;

    status = tracedb_FDFind ( fd, & obj );
    if ( status == 0 )
    {
        status = ( * obj -> md -> vt1 -> write ) ( obj, buffer, bytes, num_writ );
        tracedb_FDRelease ( obj );
    }
    return status;
}

/* FDWhack
 *  closes an fd if open
 *  ignores unopen fds
 */
int tracedb_FDWhack ( tracedb_fd_t fd )
{
    int status;

    if ( fd >= 0 )
        return tracedb_StdFDWhack ( fd );

    status = tracedb_FNSWhackFD ( fd );

    if ( status == EBADF )
        return 0;
    return status;
}


/*--------------------------------------------------------------------------
 * FDFile
 *  an fd supporting file operations
 */

/* FDFileFind
 *  find the FD
 *  ensure it's a member of FDFile class
 */
static
int tracedb_FDFileFind ( tracedb_fd_t fd, tracedb_FDFile **f )
{
    int status = tracedb_FDFind ( fd, ( tracedb_FD** ) f );
    if ( status == 0 )
    {
        if ( ( tracedb_FDVariant ( * f ) & tracedb_vfdLvl2 ) != tracedb_vfdFile )
        {
            tracedb_FDRelease ( * f );
            status = EBADF;
        }
    }
    return status;
}

/* FDFileLockRgn
 * FDFileUnlockRgn
 *  directly lock/unlock a region within an open file
 *
 *  returns 0 on success, non-zero if the file cannot be
 *  immediately locked for whatever reason.
 *
 *  NB - FDFileLockRgn implements Unix behavior, which is akin
 *  to a "try-lock". there is no blocking mode for this function.
 *
 *  if "exclusive" is true, no other locks will be allowed.
 *
 *  if "exclusive" is false, then by definition the lock type
 *  is for read and will allow multiple reader locks.
 *
 *  "len" is the length of the region to lock/unlock, with the
 *  special value "0" interpreted as meaning from the starting
 *  position to the end of file.
 *
 *  "pos" is the starting position of the region to be locked.
 *
 *  each successful Unlock will drop either the single exclusive
 *  lock or one of possibly multiple reader locks.
 */
int tracedb_FDFileLockRgn ( tracedb_fd_t fd,
    bool exclusive, uint64_t bytes, uint64_t pos )
{
    int status;
    tracedb_FDFile *f;

    if ( fd >= 0 )
        return tracedb_StdFDFileLockRgn ( fd, exclusive, bytes, pos );

    status = tracedb_FDFileFind ( fd, & f );
    if ( status == 0 )
    {
        status = ( * f -> md -> vt2 -> lockRgn ) ( f, exclusive, bytes, pos );
        tracedb_FDRelease ( f );
    }
    return status;
}

int tracedb_FDFileUnlockRgn ( tracedb_fd_t fd,
    uint64_t bytes, uint64_t pos )
{
    int status;
    tracedb_FDFile *f;

    if ( fd >= 0 )
        return tracedb_StdFDFileUnlockRgn ( fd, bytes, pos );

    status = tracedb_FDFileFind ( fd, & f );
    if ( status == 0 )
    {
        status = ( * f -> md -> vt2 -> unlockRgn ) ( f, bytes, pos );
        tracedb_FDRelease ( f );
    }
    return status;
}

/* FDFileSize
 *  returns file size in bytes
 */
int tracedb_FDFileSize ( tracedb_fd_t fd, uint64_t *size )
{
    int status;
    tracedb_FDFile *f;

    if ( fd >= 0 )
        return tracedb_StdFDFileSize ( fd, size );

    if ( size == NULL )
        return EINVAL;

    status = tracedb_FDFileFind ( fd, & f );
    if ( status == 0 )
    {
        status = ( * f -> md -> vt2 -> size ) ( f, size );
        tracedb_FDRelease ( f );
    }
    return status;
}

/* FDFileSetSize
 *  sets the logical file size
 */
int tracedb_FDFileSetSize ( tracedb_fd_t fd, uint64_t size )
{
    int status;
    tracedb_FDFile *f;

    if ( fd >= 0 )
        return tracedb_StdFDFileSetSize ( fd, size );

    status = tracedb_FDFileFind ( fd, & f );
    if ( status == 0 )
    {
        status = ( * f -> md -> vt2 -> setSize ) ( f, size );
        tracedb_FDRelease ( f );
    }
    return status;
}

/* FDFilePosition
 *  reports stream pointer
 */
int tracedb_FDFilePosition ( tracedb_fd_t fd, uint64_t *pos )
{
    int status;
    tracedb_FDFile *f;

    if ( fd >= 0 )
        return tracedb_StdFDFilePosition ( fd, pos );

    if ( pos == NULL )
        return EINVAL;

    status = tracedb_FDFileFind ( fd, & f );
    if ( status == 0 )
    {
        status = ( * f -> md -> vt2 -> position ) ( f, pos );
        tracedb_FDRelease ( f );
    }
    return status;
}

/* FDFileReposition
 *  attempts to reposition stream pointer to absolute offset
 */
int tracedb_FDFileReposition ( tracedb_fd_t fd, uint64_t pos )
{
    int status;
    tracedb_FDFile *f;

    if ( fd >= 0 )
        return tracedb_StdFDFileReposition ( fd, pos );

    status = tracedb_FDFileFind ( fd, & f );
    if ( status == 0 )
    {
        status = ( * f -> md -> vt2 -> reposition ) ( f, pos );
        tracedb_FDRelease ( f );
    }
    return status;
}

/* FDFileSeek
 *  here for reasons of nostalgia
 *
 *  "offset" is a signed quantity, presumably
 *
 *  "origin" is akin to "whence" in old Unix-speak, and
 *  is one of the values of SeekOrigin
 *
 *  the resultant position is returned in "pos"
 */
int tracedb_FDFileSeek ( tracedb_fd_t fd,
    int64_t offset, int origin, uint64_t *pos )
{
    int status;
    tracedb_FDFile *f;

    if ( fd >= 0 )
        return tracedb_StdFDFileSeek ( fd, offset, origin, pos );

    if ( pos == NULL || origin < 0 || origin > tracedb_seekEnd )
        return EINVAL;

    status = tracedb_FDFileFind ( fd, & f );
    if ( status == 0 )
    {
        status = ( * f -> md -> vt2 -> seek ) ( f, offset, origin, pos );
        tracedb_FDRelease ( f );
    }
    return status;
}

/* FDFileRead
 *  read from a file descriptor from a specified offset
 *  may not alter the position of the stream pointer
 *  otherwise has the semantics of FDRead
 *
 *  to read from the stream pointer, use FDRead
 */
int tracedb_FDFileRead ( tracedb_fd_t fd,
    uint64_t pos, void *buffer, size_t max_bytes, size_t *num_read )
{
    int status;
    tracedb_FDFile *f;

    if ( fd >= 0 )
        return tracedb_StdFDFileRead ( fd, pos, buffer, max_bytes, num_read );

    if ( max_bytes == 0 )
    {
        if ( num_read != NULL )
            * num_read = 0;
        return 0;
    }

    if ( buffer == NULL )
        return EINVAL;

    status = tracedb_FDFileFind ( fd, & f );
    if ( status == 0 )
    {
        status = ( * f -> md -> vt2 -> pread ) ( f, pos, buffer, max_bytes, num_read );
        tracedb_FDRelease ( f );
    }
    return status;
}

/* FDFileWrite
 *  write to a file descriptor at a specified offset
 *  may not alter the position of the stream pointer
 *  otherwise has the semantics of FDWrite
 *
 *  to write at the stream pointer, use FDWrite
 */
int tracedb_FDFileWrite ( tracedb_fd_t fd,
    uint64_t pos, const void *buffer, size_t bytes, size_t *num_writ )
{
    int status;
    tracedb_FDFile *f;

    if ( fd >= 0 )
        return tracedb_StdFDFileWrite ( fd, pos, buffer, bytes, num_writ );

    if ( bytes == 0 )
    {
        if ( num_writ != NULL )
            * num_writ = 0;
        return 0;
    }

    if ( buffer == NULL )
        return EINVAL;

    status = tracedb_FDFileFind ( fd, & f );
    if ( status == 0 )
    {
        status = ( * f -> md -> vt2 -> pwrite ) ( f, pos, buffer, bytes, num_writ );
        tracedb_FDRelease ( f );
    }
    return status;
}

/*--------------------------------------------------------------------------
 * rename standard fs functions
 */
#define tracedb_FSOpen           tracedb_StdFSOpen
#define tracedb_FSCopy           tracedb_StdFSCopy
#define tracedb_FSWhack          tracedb_StdFSWhack
#define tracedb_FSExists         tracedb_StdFSExists
#define tracedb_FSIsDir          tracedb_StdFSIsDir
#define tracedb_FSResolve        tracedb_StdFSResolve
#define tracedb_FSVPrintPath     tracedb_StdFSVPrintPath
#define tracedb_FSCreateDir      tracedb_StdFSCreateDir
#define tracedb_FSListDir        tracedb_StdFSListDir
#define tracedb_FSRemoveDir      tracedb_StdFSRemoveDir
#define tracedb_FSRemove         tracedb_StdFSRemove
#define tracedb_FSRename         tracedb_StdFSRename
#define tracedb_FSFileSize       tracedb_StdFSFileSize
#define tracedb_FSSetFileSize    tracedb_StdFSSetFileSize
#define tracedb_FSChangeMode     tracedb_StdFSChangeMode
#define tracedb_FSOpenFile       tracedb_StdFSOpenFile
#define tracedb_FSCreateFile     tracedb_StdFSCreateFile
#define tracedb_FDCopy           tracedb_StdFDCopy
#define tracedb_FDAssign         tracedb_StdFDAssign
#define tracedb_FDRenumber       tracedb_StdFDRenumber
#define tracedb_FDRead           tracedb_StdFDRead
#define tracedb_FDWrite          tracedb_StdFDWrite
#define tracedb_FDWhack          tracedb_StdFDWhack
#define tracedb_FDFileLockRgn    tracedb_StdFDFileLockRgn
#define tracedb_FDFileUnlockRgn  tracedb_StdFDFileUnlockRgn
#define tracedb_FDFileSize       tracedb_StdFDFileSize
#define tracedb_FDFileSetSize    tracedb_StdFDFileSetSize
#define tracedb_FDFilePosition   tracedb_StdFDFilePosition
#define tracedb_FDFileReposition tracedb_StdFDFileReposition
#define tracedb_FDFileSeek       tracedb_StdFDFileSeek
#define tracedb_FDFileRead       tracedb_StdFDFileRead
#define tracedb_FDFileWrite      tracedb_StdFDFileWrite
#define tracedb_DirListWhack     tracedb_StdDirListWhack

#undef TRACEDB_FDLNK
#define TRACEDB_FDLNK static

