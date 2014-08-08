#ifndef _h_fdio_
#define _h_fdio_

#ifndef _h_itypes_
#include "itypes.h"
#endif

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * forwards
 */
typedef struct tracedb_DirList tracedb_DirList;


/*--------------------------------------------------------------------------
 * FSOpenPerm
 *  values to pass to open and create functions
 */
enum tracedb_FSOpenPerm
{
    tracedb_permRead = 1,
    tracedb_permWrite = 2,
    tracedb_permReadWrite = tracedb_permRead | tracedb_permWrite,

    tracedb_permAppend = 4,
    tracedb_permTrunc = 8,
    tracedb_permExcl = 16,
    tracedb_permParents = 32
};


/*--------------------------------------------------------------------------
 * SeekOrigin
 *  values to pass to seek functions
 */
enum tracedb_SeekOrigin
{
    tracedb_seekSet,
    tracedb_seekCur,
    tracedb_seekEnd
};


/*--------------------------------------------------------------------------
 * fs_t
 *  file system - index to a registered file system
 *
 *  the special value of -1 means "invalid"
 *
 *  value 0 is the std-C/Unix file system, defined as "stdfs"
 *
 *  non-zero values represent alternate file systems
 */
typedef int32_t tracedb_fs_t;

/* stdfs
 *  the standard file system
 */
#define tracedb_stdfs 0


/*--------------------------------------------------------------------------
 * fd_t
 *  file descriptor - index to a privately held object
 *
 *  the special value of -1 means "invalid"
 *
 *  values 0..some system and configuration dependent value are
 *  defined as belonging to the "stdfs"
 *
 *  values with the high bit set ( and thus appear negative )
 *  are our own range, i.e. -2 and below are valid numbers.
 */
typedef int32_t tracedb_fd_t;


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

/* FSOpen
 *  finds a registered, named file system
 *
 *  "fs" [ OUT ] - return parameter for file system
 *
 *  "name" [ IN, NULL OKAY ] - NUL terminated string
 *  if NULL or the special value "stdfs", the standard
 *  fs descriptor will be returned.
 *
 *  return values:
 */
int tracedb_FSOpen ( tracedb_fs_t *fs, const char *name );

/* FSCopy
 *  create a copy of a file system descriptor
 *
 *  "copy" [ OUT ] - return parameter for copy
 *
 *  return values:
 */
int tracedb_FSCopy ( tracedb_fs_t self, tracedb_fs_t *copy );

/* FSWhack
 *  closes an fs_t returned by FSOpen
 *
 *  return values:
 */
int tracedb_FSWhack ( tracedb_fs_t self );

/* FSExists
 *  determines if file or directory exists
 *
 *  "path" [ IN ] - NUL terminated path denoting any
 *  named object within the file system.
 *
 *  return values:
 *    true if exists
 *    false if object does not exist or any error occurs
 */
bool tracedb_FSExists ( tracedb_fs_t self, const char *path );

/* FSIsDir
 *  determines if path represents a file or directory
 *
 *  "path" [ IN ] - NUL terminated path denoting any
 *  named object within the file system
 *
 *  return values:
 *    true if object exists and is a directory
 *    false if the object cannot be shown to be a directory
 */
bool tracedb_FSIsDir ( tracedb_fs_t self, const char *path );

/* FSResolve
 *  gives canonical path
 *
 *  if the file system "fs" supports the concept of a working directory,
 *  the path will be interpreted relative to the working directory unless
 *  it begins with '/'.
 *
 *  "path" [ IN ] - NUL terminated path to resolve
 *
 *  "resolved" [ OUT ] and "max_size" [ IN ] - return parameter for
 *  resolved object path
 *
 *  returns values:
 */
int tracedb_FSResolve ( tracedb_fs_t self, const char *path,
    char *resolved, size_t max_size );

/* FSPrintPath
 * FSVPrintPath
 *  perform [v]snprintf function into buffer as a path string
 *
 *  "path" [ OUT ] and "bsize" [ IN ] - describe the output path buffer
 *
 *  "psize" [ OUT, NULL OKAY ] - the resultant size of path in bytes,
 *  excluding terminating NULL byte.
 *
 *  "fmt" [ IN ] and "args" [ IN ] - standard format arguments
 *
 *  returns values:
 *    ENAMETOOLONG if buffer size is too small or exceeds fs path max
 */
int tracedb_FSPrintPath ( tracedb_fs_t self, char *path,
    size_t bsize, size_t *psize, const char *fmt, ... );
int tracedb_FSVPrintPath ( tracedb_fs_t self, char *path,
    size_t bsize, size_t *psize, const char *fmt, va_list args );

/* FSCreateDir
 *  cause a directory to be created with the specified mode
 *
 *  "path" [ IN ] - NUL terminated path to directory
 *
 *  "mode" [ IN ] - standard Unix directory mode used when creating
 *  new directories. NB - will not change mode of existing directories.
 *
 *  "parents" [ IN ] - boolean value for requesting automatic
 *  creation of parent directories to complete the path
 *
 *  return values:
 *    EEXIST if the directory already exists
 *    ENOTDIR if the path or one of its parent components
 *      is not actually a directory
 *    ENOENT if a parent component of the path does not
 *      exist or is a dangling symbolic link
 */
int tracedb_FSCreateDir ( tracedb_fs_t self,
    const char *path, mode_t mode, bool parents );

/* FSListDir
 *  get a directory listing
 *
 *  requires include of directory.h
 *
 *  "dl" [ OUT ] - return parameter for directory list
 *
 *  "path" [ IN ] - NUL terminated path to directory
 *
 *  return values:
 */
int tracedb_FSListDir ( tracedb_fs_t self,
    tracedb_DirList *dl, const char *path );

/* FSRemoveDir
 *  delete a directory from the file system
 *
 *  "path" [ IN ] - NUL terminated path to directory
 *
 *  "force" [ IN ] - boolean value which, when true, will
 *  cause directory contents to be recursively removed before
 *  the directory itself is removed. when false, the directory
 *  must be empty.
 *
 *  return values:
 */
int tracedb_FSRemoveDir ( tracedb_fs_t self,
    const char *path, bool force );

/* FSRemove
 *  removes an object from the file system
 *
 *  "path" [ IN ] - NUL terminated path to object within
 *  the file system
 */
int tracedb_FSRemove ( tracedb_fs_t self, const char *path );

/* FSRename
 *  renames or moves a file from one location to another
 *
 *  "from" [ IN ] - NUL terminated path to existing object
 *  within the file system
 *
 *  "to" [ IN ] - NUL terminated path to which "from" is to
 *  be renamed.
 *
 *  return values:
 */
int tracedb_FSRename ( tracedb_fs_t self,
    const char *from, const char *to );

/* FSFileSize
 *  returns file size in "eof"
 *
 *  "path" [ IN ] - NUL terminated path to file or file alias
 *
 *  "eof" [ OUT ] - return parameter for file size
 *
 *  return values:
 */
int tracedb_FSFileSize ( tracedb_fs_t self,
    const char *path, uint64_t *eof );

/* FSSetFileSize
 *  sets end of file by path
 *
 *  "path" [ IN ] - NUL terminated path to file or file alias
 *
 *  "eof" [ IN ] - desired file size. NB - can be used to extend
 *  or truncate file.
 *
 *  return values:
 */
int tracedb_FSSetFileSize ( tracedb_fs_t self,
    const char *path, uint64_t eof );

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
int tracedb_FSChangeMode ( tracedb_fs_t self, const char *path,
    mode_t mode, mode_t mask, bool recurse );

/* FSOpenFile
 * FSOpenFixed
 *  open a named file or device with supplied permission
 *
 *  "fd" [ OUT ] - return parameter for newly opened file -OR-
 *  "fd" [ IN ] - fixed file descriptor id for newly opened file
 *
 *  "path" [ IN ] - NUL terminated path to file or file alias
 *
 *  "perm" [ IN ] - one or more FSOpenPerm bits
 *
 *  return values:
 */
int tracedb_FSOpenFile ( tracedb_fs_t self, tracedb_fd_t *fd,
    const char *path, int perm );
int tracedb_FSOpenFixed ( tracedb_fs_t self, tracedb_fd_t fd,
    const char *path, int perm );

/* FSCreateFile
 * FSCreateFixed
 *  creates the file if it does not exist
 *  opens the named file with supplied permissions
 *
 *  "fd" [ OUT ] - return parameter for newly opened file -OR-
 *  "fd" [ IN ] - fixed file descriptor id for newly opened file
 *
 *  "path" [ IN ] - NUL terminated path to file or file alias
 *
 *  "perm" [ IN ] - one or more FSOpenPerm bits
 *
 *  "mode" [ IN ] - a standard Unix file mode, and is used only to
 *  create a file, i.e. an existing file does not have its
 *  access mode changed.
 *
 *  "FSCreateFixed" creates on a specific fd
 */
int tracedb_FSCreateFile ( tracedb_fs_t self, tracedb_fd_t *fd,
    const char *path, int perm, mode_t mode );
int tracedb_FSCreateFixed ( tracedb_fs_t self, tracedb_fd_t fd,
    const char *path, int perm, mode_t mode );


/*--------------------------------------------------------------------------
 * FD
 *  base class of all fd operations
 *  supports streaming, copying, renumbering and closing
 */

/* FDCopy
 *  create a copy of a file descriptor
 *  returns new fd in "dst"
 */
int tracedb_FDCopy ( tracedb_fd_t src, tracedb_fd_t *dst );

/* FDAssign
 *  assigns a copy of a file descriptor to a specific fd
 *  will close the target if initially open
 */
int tracedb_FDAssign ( tracedb_fd_t src, tracedb_fd_t dst );

/* FDRenumber
 *  reassigns an open fd to a new number
 *
 *  internally performs an FDAssign to dst and a FDWhack on src
 *  but only if dst != src, and the dst number belongs to
 *  the src fd space.
 */
int tracedb_FDRenumber ( tracedb_fd_t src, tracedb_fd_t dst );

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
int tracedb_FDRead ( tracedb_fd_t self,
    void *buffer, size_t max_bytes, size_t *num_read );

/* FDReadAny
 *  behaves just like FDRead, except that interrupts
 *  are handled by an externally provided callback
 *
 *  "buffer" [ OUT ] and "max_bytes" [ IN ] - output parameter for
 *  data to be read
 *
 *  "num_read" [ OUT ] - return parameter for number of valid bytes
 *  in "buffer" as a result of the read.
 *
 *  "handle_intr" [ IN, NULL OKAY ] and "data" [ IN, NULL OKAY ] -
 *  optional callback to be invoked if FDRead returns EINTR
 *
 *  return values:
 *    EINTR - may only be returned by callback
 *    all other values returned by FDRead
 */
int tracedb_FDReadAny ( tracedb_fd_t self,
    void *buffer, size_t max_bytes, size_t *num_read,
    int ( * handle_intr ) ( void * data ), void *data );

/* FDWrite
 *  write a specified number of bytes to the stream
 *  returns the number of bytes written in "num_writ"
 *
 *  "num_writ" may be less than "bytes", but still > 0
 *  in cases where buffering or blocksize is not transparent.
 *  it may also be the case for end-of-media or broken-pipe
 *  errors where a partial write was successful but there are
 *  still data remaining.
 */
int tracedb_FDWrite ( tracedb_fd_t fd,
    const void *buffer, size_t bytes, size_t *num_writ );

/* FDWriteAll
 *  invokes FDWrite until all bytes are written
 *  or a true error condition is encountered.
 *
 *  interrupts ( EINTR ) are handled by either invoking
 *  an optional user-supplied callback function or else
 *  ignoring and retrying.
 *
 *  the callback function "handle_intr" may be NULL, or
 *  it will be a function that returns 0 as a continue condition
 *  and non-zero to abort the write. the value supplied for "data"
 *  is passed to the callback as context.
 */
int tracedb_FDWriteAll ( tracedb_fd_t fd,
    const void *buffer, size_t bytes, size_t *num_writ,
    int ( * handle_intr ) ( void *data ), void *data );

/* FDWhack
 *  closes an fd if open
 *  ignores unopen fds
 */
int tracedb_FDWhack ( tracedb_fd_t fd );


/*--------------------------------------------------------------------------
 * FDPipe
 *  a pair of fds for pipe operations
 *  note that pipes are inherently local,
 *  and supplied by the stdfs.
 */

/* FDPipeMake
 *  create a pipe with arbitrary file descriptors
 *  the resultant fds will be used for exclusive read or write
 */
int tracedb_FDPipeMake ( tracedb_fd_t *read, tracedb_fd_t *write );

/* FDPipeAssign
 *  create a pipe with fixed file descriptors
 *  if a supplied descriptor is already open, it will be closed
 *  before being reassigned to the pipe
 *
 *  both descriptors must belong to the same space
 */
int tracedb_FDPipeAssign ( tracedb_fd_t read, tracedb_fd_t write );

/* FDPipeWhack
 *  whack both ends
 *  has difficulty when one succeeds and the other doesn't...
 */
int tracedb_FDPipeWhack ( tracedb_fd_t read, tracedb_fd_t write );


/*--------------------------------------------------------------------------
 * FDFile
 *  an fd supporting file operations
 */

/* FDFileLock
 * FDFileUnlock
 *  lock/unlock an open file
 *
 *  returns 0 on success, non-zero if the file cannot be
 *  immediately locked for whatever reason.
 *
 *  NB - FDFileLock implements Unix behavior, which is akin to
 *  a "try-lock". there is no blocking mode for this function.
 *
 *  if "exclusive" is true, no other locks will be allowed.
 *
 *  if "exclusive" is false, then by definition the lock type
 *  is for read and will allow multiple reader locks.
 *
 *  each successful Unlock will drop either the single exclusive
 *  lock or one of possibly multiple reader locks.
 *
 *  "fd" is an fd_t
 *  "exclusive" is of type bool
 */
#define tracedb_FDFileLock( fd, exclusive ) \
    tracedb_FDFileLockRgn ( fd, exclusive, 0, 0 )
#define tracedb_FDFileUnlock( fd ) \
    tracedb_FDFileUnlockRgn ( fd, 0, 0 )

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
 *  "bytes" is the size of the region to lock/unlock, with the
 *  special value "0" interpreted as meaning from the starting
 *  position to the end of file.
 *
 *  "pos" is the starting position of the region to be locked.
 *
 *  each successful Unlock will drop either the single exclusive
 *  lock or one of possibly multiple reader locks.
 */
int tracedb_FDFileLockRgn ( tracedb_fd_t fd,
    bool exclusive, uint64_t bytes, uint64_t pos );
int tracedb_FDFileUnlockRgn ( tracedb_fd_t fd,
    uint64_t bytes, uint64_t pos );

/* FDFileSize
 *  returns file size in bytes
 */
int tracedb_FDFileSize ( tracedb_fd_t fd, uint64_t *size );

/* FDFileSetSize
 *  sets the logical file size
 */
int tracedb_FDFileSetSize ( tracedb_fd_t fd, uint64_t size );

/* FDFilePosition
 *  reports stream pointer
 */
int tracedb_FDFilePosition ( tracedb_fd_t fd, uint64_t *pos );

/* FDFileReposition
 *  attempts to reposition stream pointer to absolute offset
 */
int tracedb_FDFileReposition ( tracedb_fd_t fd, uint64_t pos );

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
    int64_t offset, int origin, uint64_t *pos );

/* FDFileRead
 *  read from a file descriptor from a specified offset
 *  may not alter the position of the stream pointer
 *  otherwise has the semantics of FDRead
 *
 *  to read from the stream pointer, use FDRead
 */
int tracedb_FDFileRead ( tracedb_fd_t fd,
    uint64_t pos, void *buffer, size_t max_bytes, size_t *num_read );

/* FDFileReadAny
 *  behaves just like FDFileRead, except that interrupts
 *  are handled by an externally provided callback
 *
 *  "pos" [ IN ] - specific data pointer for read
 *
 *  "buffer" [ OUT ] and "max_bytes" [ IN ] - output parameter for
 *  data to be read
 *
 *  "num_read" [ OUT ] - return parameter for number of valid bytes
 *  in "buffer" as a result of the read.
 *
 *  "handle_intr" [ IN, NULL OKAY ] and "data" [ IN, NULL OKAY ] -
 *  optional callback to be invoked if FDFileRead returns EINTR
 *
 *  return values:
 *    EINTR - may only be returned by callback
 *    all other values returned by FDFileRead
 */
int tracedb_FDFileReadAny ( tracedb_fd_t self,
    uint64_t pos, void *buffer, size_t max_bytes,
    size_t *num_read, int ( * handle_intr ) ( void * data ), void *data );

/* FDFileWrite
 *  write to a file descriptor at a specified offset
 *  may not alter the position of the stream pointer
 *  otherwise has the semantics of FDWrite
 *
 *  to write at the stream pointer, use FDWrite
 */
int tracedb_FDFileWrite ( tracedb_fd_t fd,
    uint64_t pos, const void *buffer, size_t bytes, size_t *num_writ );

/* FDFileWriteAll
 *  invokes FDFileWrite until all bytes are written
 *  or a true error condition is encountered.
 *
 *  interrupts ( EINTR ) are handled by either invoking
 *  an optional user-supplied callback function or else
 *  ignoring and retrying.
 *
 *  the callback function "handle_intr" may be NULL, or
 *  it will be a function that returns 0 as a continue condition
 *  and non-zero to abort the write. the value supplied for "data"
 *  is passed to the callback as context.
 *
 *  to write at the stream pointer, use FDWriteAll
 */
int tracedb_FDFileWriteAll ( tracedb_fd_t fd,
    uint64_t pos, const void *buffer, size_t bytes,
    size_t *num_writ, int ( * handle_intr ) ( void *data ), void *data );

/* FDFileWhack
 *  close a file fd if open
 *  ignores unopen file fds
 */
#define tracedb_FDFileWhack( fd ) \
    tracedb_FDWhack ( fd )


#ifdef __cplusplus
}
#endif

#endif /* _h_fdio_ */
