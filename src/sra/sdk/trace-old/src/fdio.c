#include "fdio.h"
#include "directory.h"

#ifndef __USE_UNIX98
#define __USE_UNIX98 1
#endif
#include <unistd.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>

#ifndef TRACEDB_FDLNK
#define TRACEDB_FDLNK /* extern */
#endif


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

#if ! TRACEDB_OMIT_FSOPEN
/* FSOpen
 *  finds a registered, named file system
 *
 *  if "name" is NULL or the special value "stdfs",
 *  the standard fs descriptor will be returned.
 */
TRACEDB_FDLNK
int tracedb_FSOpen ( tracedb_fs_t *fsp, const char *name )
{
    if ( fsp == NULL )
        return EINVAL;

    if ( name != NULL && memcmp ( name, "stdfs", sizeof "stdfs" ) )
        return ENOENT;

    * fsp = tracedb_stdfs;
    return 0;
}
#endif

#if ! TRACEDB_OMIT_FSCOPY
/* FSCopy
 *  create a copy of a file system descriptor
 *
 *  "copy" [ OUT ] - return parameter for copy
 *
 *  return values:
 */
TRACEDB_FDLNK
int tracedb_FSCopy ( tracedb_fs_t self, tracedb_fs_t *copy )
{
    if ( self != tracedb_stdfs )
        return EBADF;

    if ( copy == NULL )
        return EINVAL;

    * copy = self;
    return 0;
}
#endif

#if ! TRACEDB_OMIT_FSWHACK
/* FSWhack
 *  closes an fs_t returned by FSOpen
 */
TRACEDB_FDLNK
int tracedb_FSWhack ( tracedb_fs_t fs )
{
    if ( fs != tracedb_stdfs )
        return EBADF;

    return 0;
}
#endif

/* FSExists
 *  determines if file or directory exists
 */
TRACEDB_FDLNK
bool tracedb_FSExists ( tracedb_fs_t fs, const char *path )
{
    struct stat st;

    if ( fs != tracedb_stdfs )
        return EBADF;

    return ( bool ) ( stat ( path, & st ) == 0 );
}

/* FSIsDir
 *  determines if path represents a file or directory
 */
TRACEDB_FDLNK
bool tracedb_FSIsDir ( tracedb_fs_t fs, const char *path )
{
    struct stat st;

    if ( fs != tracedb_stdfs )
        return EBADF;

    return  ( bool ) ( stat ( path, & st ) >= 0 && S_ISDIR ( st . st_mode ) );
}

/* FSResolve
 *  gives canonical path
 *
 *  returns status code
 */
TRACEDB_FDLNK
int tracedb_FSResolve ( tracedb_fs_t fs,
    const char *path, char *resolved, size_t max_size )
{
    if ( fs != tracedb_stdfs )
        return EBADF;

    if ( max_size < PATH_MAX )
    {
        char *p;
        int status;

        if ( resolved == NULL || max_size == 0 )
            return EINVAL;

        p = malloc ( PATH_MAX );
        if ( p == NULL )
            status = errno;
        else
        {
            if ( ! realpath ( path, p ) )
                status = errno;
            else
            {
                size_t len = strlen ( p );
                if ( len >= max_size )
                    status = ENAMETOOLONG;
                else
                {
                    strcpy ( resolved, p );
                    status = 0;
                }
            }
            free ( p );
        }
        return status;
    }

    if ( ! realpath ( path, resolved ) )
        return errno;

    return 0;
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
TRACEDB_FDLNK
int tracedb_FSVPrintPath ( tracedb_fs_t self,
    char *path, size_t bsize, size_t *psize,
    const char *fmt, va_list args )
{
    int writ;
    size_t psizebuff;

    if ( psize == NULL )
        psize = & psizebuff;

    if ( self != tracedb_stdfs )
    {
        * psize = 0;
        return EBADF;
    }

    if ( ( path == NULL && bsize != 0 ) || fmt == NULL || args == NULL )
    {
        * psize = 0;
        return EINVAL;
    }

    if ( bsize > PATH_MAX )
        bsize = PATH_MAX;

    writ = vsnprintf ( path, bsize, fmt, args );
    if ( writ < 0 || ( size_t ) writ >= bsize )
        return ENAMETOOLONG;

    * psize = writ;
    return 0;
}

/* DirectoryCreateParents
 */
static
int tracedb_DirectoryCreateParents ( const char *path, mode_t mode )
{
    char fname [ PATH_MAX ];
    int i, size = snprintf ( fname, sizeof fname, "%s", path );

    /* detect name too long */
    if ( size < 0 || ( size_t ) size >= sizeof fname )
        return ENAMETOOLONG;

    /* strip off last element */
    for ( i = size; i > 0 && fname [ -- i ] != '/'; )
        ( ( void ) 0 );

    /* require a slash other than root */
    if ( i == 0 )
        return ENOENT;

    /* set the stop index for scanning the string
       look for the earliest known good directory */
    for ( size = 0; i > 0; )
    {
        fname [ i ] = 0;
        if ( tracedb_FSExists ( tracedb_stdfs, fname ) )
        {
            fname [ i ] = '/';
            break;
        }
        for ( ; i > 0 && fname [ -- i ] != '/'; )
            ( ( void ) 0 );

        /* "i" is now a potential stop index.
           if it's the penultimate slash within,
           then it's where we stop */
        if ( size == 0 )
            size = i;
    }

    /* massage directory modes based upon expressed mode */
    for ( mode |= ( ( mode & 0444 ) >> 2 ) | ( ( mode & 0222 ) >> 1 );
          i <= size; )
    {
        int status = mkdir ( fname, mode );
        if ( status != 0 )
            return errno;
        for ( ; ++ i <= size; )
        {
            if ( fname [ i ] == 0 )
            {
                fname [ i ] = '/';
                break;
            }
        }
    }

    return 0;
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
TRACEDB_FDLNK
int tracedb_FSCreateDir ( tracedb_fs_t fs,
    const char *path, mode_t mode, bool parents )
{
    int status;

    if ( fs != tracedb_stdfs )
        return EBADF;

    status = mkdir ( path, mode );
    if ( status < 0 )
    {
        /* if not creating parents, all errors are errors */
        if ( ! parents )
            return errno;

        /* if path already exists, ensure it's a directory */
        if ( errno == EEXIST )
        {
            if ( tracedb_FSIsDir ( fs, path ) )
                return 0;
            return EEXIST;
        }

        /* any error other than the target directory doesn't exist */
        if ( errno != ENOENT )
            return errno;

        /* create parent directories */
        status = tracedb_DirectoryCreateParents ( path, mode );
        if ( status == 0 )
        {
            status = mkdir ( path, mode );
            if ( status < 0 )
                status = errno;
        }
    }
    return status;
}

/* FSListDir
 *  get a directory listing
 *
 *  requires include of directory.h
 */
typedef struct tracedb_DirListData tracedb_DirListData;
struct tracedb_DirListData
{
    struct dirent **namelist;
    int count;
    int ignore;
};

static
int tracedb_drop_dots ( const struct dirent *e )
{
    const char *entry = e -> d_name;
    if ( entry [ 0 ] == '.' )
    {
        switch ( entry [ 1 ] )
        {
        case 0:
            return 0;
        case '.':
            if ( entry [ 2 ] == 0 )
                return 0;
            break;
        }
    }
    return 1;
}

TRACEDB_FDLNK
int tracedb_FSListDir ( tracedb_fs_t fs, tracedb_DirList *dl, const char *path )
{
    int i, count;
    tracedb_DirListData *data;
    struct dirent **namelist;

    if ( fs != tracedb_stdfs )
        return EBADF;

    if ( dl == NULL )
        return EINVAL;

    if ( ! tracedb_FSIsDir ( fs, path ) )
        return ENOTDIR;

    namelist = NULL;
    count = scandir ( path, & namelist, tracedb_drop_dots, alphasort );
    if ( count < 0 )
        return errno;

    dl -> fs = fs;
    dl -> idx = 0;
    dl -> len = 0;
    dl -> last = 0;

    data = malloc ( sizeof * data + count * sizeof dl -> entries [ 0 ] );
    if ( data == NULL )
    {
        int status = errno;

        for ( i = 0; i < count; ++ i )
            free ( namelist [ i ] );
        free ( namelist );

        return status;
    }

    data -> namelist = namelist;
    data -> count = count;

    dl -> entries = ( const char** ) ( data + 1 );
    dl -> data = data;

    for ( i = 0; i < count; ++i )
        dl -> entries [ i ] = namelist [ i ] -> d_name;

    if ( i > 0 )
    {
        dl -> len = i;
        dl -> last = i - 1;
    }
    
    return 0;
}

/* ForceRemoveDir
 */
static
int tracedb_ForceRemoveDir ( char *path )
{
    struct dirent **namelist;
    int i, len, status, num_entries;

    num_entries = scandir ( path, & namelist, tracedb_drop_dots, alphasort );
    if ( num_entries < 0 )
        return errno;
    
    len = strlen ( path );
    path [ len ++ ] = '/';

    for ( i = status = 0; i < num_entries; ++ i )
    {
        /* remove the entry */
        if ( status == 0 )
        {
            strcpy ( path + len, namelist [ i ] -> d_name );
            status = tracedb_FSRemove ( tracedb_stdfs, path );
            if ( status == ENOTEMPTY )
                status = tracedb_ForceRemoveDir ( path );
        }
	
        free ( namelist [ i ] );
    }

    free ( namelist );

    path [ len - 1 ] = 0;

    if ( status == 0 )
    {
        if ( rmdir ( path ) != 0 )
            status = errno;
    }
    
    return status;
}

/* FSRemoveDir
 *  delete a directory from the file system
 *  if "force" is false, the directory must be empty
 *  otherwise, a recursive delete will be performed.
 */
TRACEDB_FDLNK
int tracedb_FSRemoveDir ( tracedb_fs_t fs,
    const char *path, bool force )
{
    char buffer [ PATH_MAX ];

    if ( fs != tracedb_stdfs )
        return EBADF;

    if ( rmdir ( path ) == 0 )
        return 0;

    if ( ! force || errno != ENOTEMPTY )
        return errno;

    strncpy ( buffer, path, sizeof buffer );
    buffer [ sizeof buffer - 1 ] = 0;

    return tracedb_ForceRemoveDir ( buffer );
}

/* FSRemove
 *  removes an object from the file system
 */
TRACEDB_FDLNK
int tracedb_FSRemove ( tracedb_fs_t fs, const char *path )
{
    if ( fs != tracedb_stdfs )
        return EBADF;

    if ( unlink ( path ) != 0 )
    {
        if ( errno == EISDIR )
            return tracedb_FSRemoveDir ( fs, path, false );

        if ( errno != ENOENT )
            return errno;
    }
    return 0;
}

/* FSRename
 *  renames or moves a file from one location to another
 */
TRACEDB_FDLNK
int tracedb_FSRename ( tracedb_fs_t fs,
    const char *from, const char *to )
{
    if ( fs != tracedb_stdfs )
        return EBADF;

    if ( rename ( from, to ) != 0 )
        return errno;

    return 0;
}

/* FSFileSize
 *  returns file size in "eof"
 */
TRACEDB_FDLNK
int tracedb_FSFileSize ( tracedb_fs_t fs,
    const char *path, uint64_t *eof )
{
    struct stat st;

    if ( fs != tracedb_stdfs )
        return EBADF;

    if ( eof == 0 )
        return EINVAL;

    if ( stat ( path, & st ) != 0 )
        return errno;

    if ( S_ISDIR ( st . st_mode ) )
        return EISDIR;

    *eof = st . st_size;
    return 0;
}

/* FSSetFileSize
 *  sets end of file by path
 */
TRACEDB_FDLNK
int tracedb_FSSetFileSize ( tracedb_fs_t fs,
    const char *path, uint64_t eof )
{
    if ( fs != tracedb_stdfs )
        return EBADF;

    if ( truncate ( path, eof ) != 0 )
        return errno;

    return 0;
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
static
int tracedb_FSChangeEntryMode ( const char *path,
    mode_t mode, mode_t mask, mode_t st_mode )
{
    mode &= mask;
    mode |= st_mode & ~ mask;

    if ( chmod ( path, mode & 07777 ) != 0 )
        return errno;
    return 0;
}

static
int tracedb_FSChangeDirMode ( char *path,
    mode_t mode, mode_t mask, mode_t st_mode )
{
    bool eperm = false;
    struct dirent **namelist;
    int i, len, status, num_entries;

    mode_t enable = mode & mask;
    if ( enable != 0 )
    {
        status = tracedb_FSChangeEntryMode ( path, mode, enable, st_mode );
        if ( status != 0 )
            return status;
    }

    num_entries = scandir ( path, & namelist, tracedb_drop_dots, alphasort );
    if ( num_entries < 0 )
        return errno;
    
    len = strlen ( path );
    path [ len ++ ] = '/';

    for ( i = status = 0; i < num_entries; ++ i )
    {
        if ( status == 0 )
        {
            struct stat st;
            strcpy ( path + len, namelist [ i ] -> d_name );
            if ( stat ( path, & st ) != 0 )
                status = errno;
            else
            {
                if ( S_ISDIR ( st . st_mode ) )
                    status = tracedb_FSChangeDirMode ( path, mode, mask, st . st_mode );
                else
                    status = tracedb_FSChangeEntryMode ( path, mode, mask, st . st_mode );

                if ( status == EPERM )
                {
                    eperm = true;
                    status = 0;
                }
            }
        }
	
        free ( namelist [ i ] );
    }

    free ( namelist );

    path [ len - 1 ] = 0;

    if ( status == 0 )
    {
        mode_t disable = ~ mode & mask;
        if ( disable != 0 )
            status = tracedb_FSChangeEntryMode ( path, mode, disable, st_mode );
    }

    if ( eperm && status == 0 )
        status = EPERM;

    return status;
}

TRACEDB_FDLNK
int tracedb_FSChangeMode ( tracedb_fs_t fs, const char *path,
    mode_t mode, mode_t mask, bool recurse )
{
    struct stat st;
    char buffer [ PATH_MAX ];

    if ( fs != tracedb_stdfs )
        return EBADF;

    if ( stat ( path, & st ) != 0 )
        return errno;

    if ( mask == 0 )
        mask = 07777;

    if ( ! recurse || ! S_ISDIR ( st . st_mode ) )
        return tracedb_FSChangeEntryMode ( path, mode, mask, st . st_mode );
    
    strncpy ( buffer, path, sizeof buffer );
    buffer [ sizeof buffer - 1 ] = 0;

    return tracedb_FSChangeDirMode ( buffer, mode, mask, st . st_mode );
}

/* MapOpenPerm
 *  converts an external define to an OS specific value
 */
static
int tracedb_MapOpenPerm ( int perm )
{
    static int mode [] =
    {
        O_RDONLY,
        O_RDONLY,
        O_WRONLY,
        O_RDWR
    };
    
    int os_perm = mode [ perm & 3 ];
    if ( perm & tracedb_permAppend )
        os_perm |= O_APPEND;
    if ( perm & tracedb_permTrunc )
        os_perm |= O_TRUNC;
    if ( perm & tracedb_permExcl )
        os_perm |= O_EXCL;
    return os_perm;
}

/* FSOpenFile
 *  open a named file or device with supplied permission
 *
 *  "perm" is one or more FSOpenPerm bits or'd together
 *
 *  "FSOpenFixed" opens on a specific fd
 */
TRACEDB_FDLNK
int tracedb_FSOpenFile ( tracedb_fs_t fs, tracedb_fd_t *fd,
    const char *path, int perm )
{
    if ( fs != tracedb_stdfs )
        return EBADF;

    if ( fd == NULL )
        return EINVAL;

    * fd = open ( path, tracedb_MapOpenPerm ( perm ) );

    if ( * fd < 0 )
        return errno;

    return 0;
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
TRACEDB_FDLNK
int tracedb_FSCreateFile ( tracedb_fs_t fs, tracedb_fd_t *fd,
    const char *path, int perm, mode_t mode )
{
    if ( fs != tracedb_stdfs )
        return EBADF;

    if ( fd == NULL )
        return EINVAL;

    * fd = open ( path, tracedb_MapOpenPerm ( perm ) | O_CREAT, mode );
    if ( * fd < 0 )
    {
        int status;

        if ( ( perm & tracedb_permParents ) == 0 || errno != ENOENT )
            return errno;

        status = tracedb_DirectoryCreateParents ( path, mode );

        if ( status != 0 )
            return status;

        * fd = open ( path, tracedb_MapOpenPerm ( perm ) | O_CREAT, mode );

        if ( * fd < 0 )
            return errno;
    }
    return 0;
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
TRACEDB_FDLNK
int tracedb_FDCopy ( tracedb_fd_t src, tracedb_fd_t *dst )
{
    if ( dst == NULL )
        return EINVAL;

    * dst = dup ( src );

    if ( * dst < 0 )
        return errno;

    return 0;
}

/* FDAssign
 *  assigns a copy of a file descriptor to a specific fd
 *  will close the target if initially open
 */
TRACEDB_FDLNK
int tracedb_FDAssign ( tracedb_fd_t src, tracedb_fd_t dst )
{
    if ( dst == src )
        return EBUSY;

    /* dup2 will close the dst file,
       but NFS presents some problems.
       close it explicitly first */
    close ( dst );

    /* create a copy */
    if ( dup2 ( src, dst ) < 0 )
        return errno;

    return 0;
}

/* FDRenumber
 *  reassigns an open fd to a new number
 *
 *  internally performs an FDAssign to dst and a FDWhack on src
 *  but only if dst != src, and the dst number belongs to
 *  the src fd space.
 */
TRACEDB_FDLNK
int tracedb_FDRenumber ( tracedb_fd_t src, tracedb_fd_t dst )
{
    if ( dst != src )
    {
        /* dup2 will close the dst file,
           but NFS presents some problems.
           close it explicitly first */
        close ( dst );

        /* create a copy */
        if ( dup2 ( src, dst ) < 0 )
            return errno;

        /* douse the original */
        close ( src );
    }
    return 0;
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
TRACEDB_FDLNK
int tracedb_FDRead ( tracedb_fd_t fd,
    void *buffer, size_t max_bytes, size_t *num_read )
{
    ssize_t count;

    if ( max_bytes == 0 )
    {
        if ( num_read != NULL )
            * num_read = 0;
        return 0;
    }

    if ( buffer == NULL )
        return EINVAL;

    count = read ( fd, buffer, max_bytes );

    if ( count < 0 )
        return errno;

    if ( num_read != NULL )
        * num_read = count;

    return 0;
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
TRACEDB_FDLNK
int tracedb_FDWrite ( tracedb_fd_t fd,
    const void *buffer, size_t bytes, size_t *num_writ )
{
    ssize_t count;

    if ( bytes == 0 )
    {
        if ( num_writ != NULL )
            * num_writ = 0;
        return 0;
    }

    if ( buffer == NULL )
        return EINVAL;

    count = write ( fd, buffer, bytes );

    if ( count < 0 )
        return errno;

    if ( num_writ != NULL )
        * num_writ = count;

    return 0;
}

/* FDWhack
 *  closes an fd if open
 *  ignores unopen fds
 */
TRACEDB_FDLNK
int tracedb_FDWhack ( tracedb_fd_t fd )
{
    if ( fd != -1 && close ( fd ) != 0 )
    {
        if ( errno != EBADF || fd < 0 )
            return errno;
    }
    return 0;
}


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
int tracedb_FDPipeMake ( tracedb_fd_t *read, tracedb_fd_t *write )
{
    tracedb_fd_t fds [ 2 ];

    if ( read == NULL || write == NULL )
        return EINVAL;

    if ( pipe ( fds ) != 0 )
        return errno;

    * read = fds [ 0 ];
    * write = fds [ 1 ];

    return 0;
}

/* FDPipeAssign
 *  create a pipe with fixed file descriptors
 *  if a supplied descriptor is already open, it will be closed
 *  before being reassigned to the pipe
 *
 *  both descriptors must belong to the same space
 */
int tracedb_FDPipeAssign ( tracedb_fd_t read, tracedb_fd_t write )
{
    int status;
    tracedb_fd_t fds [ 2 ];

    if ( read == write )
        return EINVAL;

    if ( read < 0 || write < 0 )
        return EBADF;

    status = pipe ( fds );
    if ( status != 0 )
        return errno;

    /* if either side is already correct, just renumber the other side */
    if ( fds [ 0 ] == read )
    {
        status = tracedb_FDRenumber ( fds [ 1 ], write );
        if ( status == 0 )
            return 0;
    }
    else if ( fds [ 1 ] == write )
    {
        status = tracedb_FDRenumber ( fds [ 0 ], read );
        if ( status == 0 )
            return 0;
    }
    else
    {
        tracedb_fd_t copy;

        /* if either side conflicts, duplicate it */
        if ( fds [ 0 ] == write )
        {
            status = tracedb_FDCopy ( fds [ 0 ], & copy );
            if ( status == 0 )
                fds [ 0 ] = copy;
        }
        if ( fds [ 1 ] == read )
        {
            status = tracedb_FDCopy ( fds [ 1 ], & copy );
            if ( status == 0 )
                fds [ 1 ] = copy;
        }
        if ( status == 0 )
        {
            /* neither fd is correct and both should be renumbered */
            assert ( fds [ 0 ] != read && fds [ 1 ] != write );
            assert ( fds [ 0 ] != write && fds [ 1 ] != read );
            status = tracedb_FDRenumber ( fds [ 0 ], read );
            if ( status == 0 )
            {
                fds [ 0 ] = read;
                status = tracedb_FDRenumber ( fds [ 1 ], read );
                if ( status == 0 )
                    return 0;
            }
        }
    }

    /* failed */
    close ( fds [ 0 ] );
    close ( fds [ 1 ] );

    return status;
}

/* FDPipeWhack
 *  whack both ends
 *  has difficulty when one succeeds and the other doesn't...
 */
int tracedb_FDPipeWhack ( tracedb_fd_t read, tracedb_fd_t write )
{
    int status = tracedb_FDWhack ( read );
    if ( status == 0 )
        status = tracedb_FDWhack ( write );
    return status;
}


/*--------------------------------------------------------------------------
 * FDFile
 *  an fd supporting file operations
 */

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
TRACEDB_FDLNK
int tracedb_FDFileLockRgn ( tracedb_fd_t fd,
    bool exclusive, uint64_t bytes, uint64_t pos )
{
    struct flock lock;
    
    lock . l_type = exclusive ? F_WRLCK : F_RDLCK;
    lock . l_whence = SEEK_SET;
    lock . l_start = pos;
    lock . l_len = bytes;
    lock . l_pid = 0;
    
    if ( fcntl ( fd, F_SETLK, & lock ) != 0 )
        return errno;

    return 0;
}

TRACEDB_FDLNK
int tracedb_FDFileUnlockRgn ( tracedb_fd_t fd,
    uint64_t bytes, uint64_t pos )
{
    struct flock lock;
    
    lock . l_type = F_UNLCK;
    lock . l_whence = SEEK_SET;
    lock . l_start = pos;
    lock . l_len = bytes;
    lock . l_pid = 0;
    
    if ( fcntl ( fd, F_SETLK, & lock ) != 0 )
        return errno;

    return 0;
}

/* FDFileSize
 *  returns file size in bytes
 */
TRACEDB_FDLNK
int tracedb_FDFileSize ( tracedb_fd_t fd, uint64_t *size )
{
    struct stat st;

    if ( size == NULL )
        return EINVAL;

    if ( fstat ( fd, & st ) != 0 )
        return errno;

    if ( S_ISDIR ( st . st_mode ) )
        return EISDIR;

    * size = st . st_size;

    return 0;
}

/* FDFileSetSize
 *  sets the logical file size
 */
TRACEDB_FDLNK
int tracedb_FDFileSetSize ( tracedb_fd_t fd, uint64_t size )
{
    if ( ftruncate ( fd, size ) != 0 )
        return errno;

    return 0;
}

/* FDFilePosition
 *  reports stream pointer
 */
TRACEDB_FDLNK
int tracedb_FDFilePosition ( tracedb_fd_t fd, uint64_t *pos )
{
    if ( pos == NULL )
        return EINVAL;

    * pos = lseek ( fd, 0, SEEK_CUR );

    if ( * pos == ( uint64_t ) -1 )
        return errno;

    return 0;
}

/* FDFileReposition
 *  attempts to reposition stream pointer to absolute offset
 */
TRACEDB_FDLNK
int tracedb_FDFileReposition ( tracedb_fd_t fd, uint64_t pos )
{
    uint64_t after = lseek ( fd, pos, SEEK_SET );

    if ( after == ( uint64_t ) -1 )
        return errno;

    return 0;
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
TRACEDB_FDLNK
int tracedb_FDFileSeek ( tracedb_fd_t fd,
    int64_t offset, int origin, uint64_t *pos )
{
    static int whence [] =
    {
        SEEK_SET,
        SEEK_CUR,
        SEEK_END
    };

    if ( pos == NULL || origin < 0 || origin > tracedb_seekEnd )
        return EINVAL;

    * pos = lseek ( fd, offset, whence [ origin ] );

    if ( * pos == ( uint64_t ) -1 )
        return errno;

    return 0;
}

/* FDFileRead
 *  read from a file descriptor from a specified offset
 *  may not alter the position of the stream pointer
 *  otherwise has the semantics of FDRead
 *
 *  to read from the stream pointer, use FDRead
 */
TRACEDB_FDLNK
int tracedb_FDFileRead ( tracedb_fd_t fd,
    uint64_t pos, void *buffer, size_t max_bytes, size_t *num_read )
{
    ssize_t count;

    if ( num_read == NULL )
        return EINVAL;

    count = pread ( fd, buffer, max_bytes, pos );

    if ( count < 0 )
        return errno;

    * num_read = count;

    return 0;
}

/* FDFileWrite
 *  write to a file descriptor at a specified offset
 *  may not alter the position of the stream pointer
 *  otherwise has the semantics of FDWrite
 *
 *  to write at the stream pointer, use FDWrite
 */
TRACEDB_FDLNK
int tracedb_FDFileWrite ( tracedb_fd_t fd,
    uint64_t pos, const void *buffer, size_t bytes, size_t *num_writ )
{
    ssize_t count;

    if ( num_writ == NULL )
        return EINVAL;

    count = pwrite ( fd, buffer, bytes, pos );

    if ( count < 0 )
        return errno;

    * num_writ = count;

    return 0;
}

/*--------------------------------------------------------------------------
 * DirList
 */

/* DirListWhack
 *  destroy dirlist
 */
TRACEDB_FDLNK
int tracedb_DirListWhack ( tracedb_DirList *dl )
{
    if ( dl != NULL )
    {
        tracedb_DirListData *data;

        if ( dl -> fs != tracedb_stdfs )
            return EBADF;

        data = dl -> data;
        if ( data != NULL )
        {
            int i;

            struct dirent **namelist = data -> namelist;
            for ( i = 0; i < data -> count; ++ i )
                free ( namelist [ i ] );
            free ( namelist );

            free ( data );
        }
    }

    return 0;
}
