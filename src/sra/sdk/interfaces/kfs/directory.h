/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#ifndef _h_kfs_directory_
#define _h_kfs_directory_

#ifndef _h_kfs_extern_
#include <kfs/extern.h>
#endif

#ifndef _h_klib_defs_
#include <klib/defs.h>
#endif

#ifndef _h_kfs_defs_
#include <kfs/defs.h>
#endif

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * forwards
 */
struct KFile;
struct KNamelist;


/*--------------------------------------------------------------------------
 * KDirectory
 *  a container node within file system
 */
typedef struct KDirectory KDirectory;

/* AddRef
 * Release
 *  ignores NULL references
 */
KFS_EXTERN rc_t CC KDirectoryAddRef ( const KDirectory *self );
KFS_EXTERN rc_t CC KDirectoryRelease ( const KDirectory *self );

/* List
 *  create a directory listing
 *
 *  "list" [ OUT ] - return parameter for list object
 *
 *  "f" [ IN, NULL OKAY ] and "data" [ IN, OPAQUE ] - optional
 *  filter function to execute on each path. receives a base directory
 *  and relative path for each entry. if "f" returns true, the name will
 *  be added to the list.
 *
 *  "path" [ IN, NULL OKAY ] - optional parameter for target
 *  directory. if NULL or empty, interpreted to mean "."
 *
 *  function makes a flat list, does not step into sub-dirs!
 *
 */
KFS_EXTERN rc_t CC KDirectoryList ( const KDirectory *self, struct KNamelist **list,
    bool ( CC * f ) ( const KDirectory *dir, const char *name, void *data ),
    void *data, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVList ( const KDirectory *self, struct KNamelist **list,
    bool ( CC * f ) ( const KDirectory *dir, const char *name, void *data ),
    void *data, const char *path, va_list args );

/* Visit
 *  visit each path under designated directory,
 *  recursively if so indicated
 *
 *  "recurse" [ IN ] - if true, recursively visit sub-directories
 *
 *  "f" [ IN ] and "data" [ IN, OPAQUE ] - function to execute
 *  on each path. receives a base directory and relative path
 *  for each entry. if "f" returns true, the iteration will
 *  terminate and that value will be returned. NB - "dir" will not
 *  be the same as "self".
 *
 *  "path" [ IN, NULL OKAY ] - optional NUL terminated string
 *   in directory-native character set. if NULL or empty, interpreted to mean "."
 *
 * VisitFull hits all files types that including those are normally hidden
 */
KFS_EXTERN rc_t CC KDirectoryVisit ( const KDirectory *self, bool recurse,
    rc_t ( CC * f ) ( const KDirectory *dir, uint32_t type, const char *name, void *data ),
    void *data, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVVisit ( const KDirectory *self, bool recurse,
    rc_t ( CC * f ) ( const KDirectory *dir, uint32_t type, const char *name, void *data ),
    void *data, const char *path, va_list args );

/* VisitUpdate
 *  like Visit except that the directory passed back to "f"
 *  is available for update operations
 */
KFS_EXTERN rc_t CC KDirectoryVisitUpdate ( KDirectory *self, bool recurse,
    rc_t ( CC * f ) ( KDirectory *dir, uint32_t type, const char *name, void *data ),
    void *data, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVVisitUpdate ( KDirectory *self, bool recurse,
    rc_t ( CC * f ) ( KDirectory *dir, uint32_t type, const char *name, void *data ),
    void *data, const char *path, va_list args );

/* PathType
 *  returns a KPathType
 *
 *  "path" [ IN ] - NUL terminated string in directory-native character set
 */
KFS_EXTERN uint32_t CC KDirectoryPathType ( const KDirectory *self,
    const char *path, ... );
KFS_EXTERN uint32_t CC KDirectoryVPathType ( const KDirectory *self,
    const char *path, va_list args );

/* ResolvePath
 *  resolves path to an absolute or directory-relative path
 *
 *  "absolute" [ IN ] - if true, always give a path starting
 *  with '/'. NB - if the directory is chroot'd, the absolute path
 *  will still be relative to directory root.
 *
 *  "resolved" [ OUT ] and "rsize" [ IN ] - buffer for
 *  NUL terminated result path in directory-native character set
 *  the resolved path will be directory relative
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target path. NB - need not exist.
 */
KFS_EXTERN rc_t CC KDirectoryResolvePath ( const KDirectory *self, bool absolute,
    char *resolved, size_t rsize, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVResolvePath ( const KDirectory *self, bool absolute,
    char *resolved, size_t rsize, const char *path, va_list args );

/* ResolveAlias
 *  resolves an alias path to its immediate target
 *  NB - the resolved path may be yet another alias
 *
 *  "absolute" [ IN ] - if true, always give a path starting
 *  with '/'. NB - if the directory is chroot'd, the absolute path
 *  will still be relative to directory root.
 *
 *  "resolved" [ OUT ] and "rsize" [ IN ] - buffer for
 *  NUL terminated result path in directory-native character set
 *  the resolved path will be directory relative
 *
 *  "alias" [ IN ] - NUL terminated string in directory-native
 *  character set denoting an object presumed to be an alias.
 */
KFS_EXTERN rc_t CC KDirectoryResolveAlias ( const KDirectory *self, bool absolute,
    char *resolved, size_t rsize, const char *alias, ... );
KFS_EXTERN rc_t CC KDirectoryVResolveAlias ( const KDirectory *self, bool absolute,
    char *resolved, size_t rsize, const char *alias, va_list args );

/* Rename
 *  rename an object accessible from directory, replacing
 *  any existing target object of the same type
 *
 *  "from" [ IN ] - NUL terminated string in directory-native
 *  character set denoting existing object
 *
 *  "to" [ IN ] - NUL terminated string in directory-native
 *  character set denoting existing object
 *
 *  "force" [ IN ] - not false means try to do more if it fails internally
 */
KFS_EXTERN rc_t CC KDirectoryRename ( KDirectory *self, bool force, const char *from, const char *to );

/* Remove
 *  remove an accessible object from its directory
 *
 *  "force" [ IN ] - if true and target is a directory,
 *  remove recursively
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target object
 */
KFS_EXTERN rc_t CC KDirectoryRemove ( KDirectory *self, bool force,
    const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVRemove ( KDirectory *self, bool force,
    const char *path, va_list args );

/* ClearDir
 *  remove all directory contents
 *
 *  "force" [ IN ] - if true and directory entry is a
 *  sub-directory, remove recursively
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target directory
 */
KFS_EXTERN rc_t CC KDirectoryClearDir ( KDirectory *self, bool force,
    const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVClearDir ( KDirectory *self, bool force,
    const char *path, va_list args );

/* Access
 *  get access to object
 *
 *  "access" [ OUT ] - return parameter for Unix access mode
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target object
 */
KFS_EXTERN rc_t CC KDirectoryAccess ( const KDirectory *self,
    uint32_t *access, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVAccess ( const KDirectory *self,
    uint32_t *access, const char *path, va_list args );

/* SetAccess
 *  set access to object a la Unix "chmod"
 *
 *  "recurse" [ IN ] - if non zero and "path" is a directory,
 *  apply changes recursively.
 *
 *  "access" [ IN ] and "mask" [ IN ] - definition of change
 *  where "access" contains new bit values and "mask defines
 *  which bits should be changed.
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target object
 */
KFS_EXTERN rc_t CC KDirectorySetAccess ( KDirectory *self, bool recurse,
    uint32_t access, uint32_t mask, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVSetAccess ( KDirectory *self, bool recurse,
    uint32_t access, uint32_t mask, const char *path, va_list args );

/* Date
 *  get date/time to object
 *
 *  "date" [ OUT ] - return parameter
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target object
 */
KFS_EXTERN rc_t CC KDirectoryDate ( const KDirectory *self,
    KTime_t *date, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVDate ( const KDirectory *self,
    KTime_t *date, const char *path, va_list args );

/* SetDate
 *  set date to object
 *
 *  "recurse" [ IN ] - if non zero and "path" is a directory,
 *  apply changes recursively.
 *
 *  "date" [ IN ] - new time and date for the object
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target object
 */
KFS_EXTERN rc_t CC KDirectorySetDate ( KDirectory *self, bool recurse,
    KTime_t date, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVSetDate ( KDirectory *self, bool recurse,
    KTime_t date, const char *path, va_list args );

/* CreateAlias
 *  creates a path alias according to create mode
 *  such that "alias" => "targ"
 *
 *  "access" [ IN ] - standard Unix directory access mode
 *  used when "mode" has kcmParents set and alias path does
 *  not exist.
 *
 *  "mode" [ IN ] - a creation mode ( see explanation above ).
 *
 *  "targ" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target object, i.e. the object which
 *  is designated by symlink "alias". THE PATH IS GIVEN RELATIVE
 *  TO DIRECTORY ( "self" ), NOT SYMLINK ( "alias" )!
 *
 *  "alias" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target alias, i.e. the symlink that
 *  designates a target "targ".
 */
KFS_EXTERN rc_t CC KDirectoryCreateAlias ( KDirectory *self,
    uint32_t access, KCreateMode mode,
    const char *targ, const char *alias );

/* OpenFileRead
 *  opens an existing file with read-only access
 *
 *  "f" [ OUT ] - return parameter for newly opened file
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target file
 */
KFS_EXTERN rc_t CC KDirectoryOpenFileRead ( const KDirectory *self,
    struct KFile const **f, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVOpenFileRead ( const KDirectory *self,
    struct KFile const **f, const char *path, va_list args );

/* OpenFileWrite
 *  opens an existing file with write access
 *
 *  "f" [ OUT ] - return parameter for newly opened file
 *
 *  "update" [ IN ] - if true, open in read/write mode
 *  otherwise, open in write-only mode
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target file
 */
KFS_EXTERN rc_t CC KDirectoryOpenFileWrite ( KDirectory *self,
    struct KFile **f, bool update, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVOpenFileWrite ( KDirectory *self,
    struct KFile **f, bool update, const char *path, va_list args );

/* CreateFile
 *  opens a file with write access
 *
 *  "f" [ OUT ] - return parameter for newly opened file
 *
 *  "update" [ IN ] - if true, open in read/write mode
 *  otherwise, open in write-only mode
 *
 *  "access" [ IN ] - standard Unix access mode, e.g. 0664
 *
 *  "mode" [ IN ] - a creation mode ( see explanation above ).
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target file
 */
KFS_EXTERN rc_t CC KDirectoryCreateFile ( KDirectory *self, struct KFile **f,
    bool update, uint32_t access, KCreateMode mode, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVCreateFile ( KDirectory *self, struct KFile **f,
    bool update, uint32_t access, KCreateMode mode, const char *path, va_list args );

/* FileSize
 *  returns size in bytes of target file
 *
 *  "size" [ OUT ] - return parameter for file size
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target file
 */
KFS_EXTERN rc_t CC KDirectoryFileSize ( const KDirectory *self,
    uint64_t *size, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVFileSize ( const KDirectory *self,
    uint64_t *size, const char *path, va_list args );

/* FilePhysicalSize
 *  returns physical allocated size in bytes of target file.  It might
 * or might not differ form FileSize
 *
 *  "size" [ OUT ] - return parameter for file size
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target file
 */
KFS_EXTERN rc_t CC KDirectoryFilePhysicalSize ( const KDirectory *self,
    uint64_t *size, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVFilePhysicalSize ( const KDirectory *self,
    uint64_t *size, const char *path, va_list args );

/* SetFileSize
 *  sets size in bytes of target file
 *
 *  "size" [ IN ] - new file size
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target file
 */
KFS_EXTERN rc_t CC KDirectorySetFileSize ( KDirectory *self,
    uint64_t size, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVSetFileSize ( KDirectory *self,
    uint64_t size, const char *path, va_list args );

/* FileLocator
 *  returns a 64-bit key pretinent only to the particular file
 *  system device holding tha file.
 *
 *  It can be used as a form of sort key except that it is not 
 *  guaranteed to be unique.
 *
 *  "locator" [ OUT ] - return parameter for file locator
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target file
 */
KFS_EXTERN rc_t CC KDirectoryFileLocator ( const KDirectory *self,
    uint64_t *locator, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVFileLocator ( const KDirectory *self,
    uint64_t *locator, const char *path, va_list args );

/* FileContiguous
 *  returns size in bytes of target file
 *
 *  "size" [ OUT ] - return parameter for file size
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target file
 */
KFS_EXTERN rc_t CC KDirectoryFileContiguous ( const KDirectory *self,
    bool *contiguous, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVFileContiguous ( const KDirectory *self,
    bool *contiguous, const char *path, va_list args );

/* OpenDirRead
 * OpenDirUpdate
 *  opens a sub-directory
 *
 *  "chroot" [ IN ] - if true, the new directory becomes
 *  chroot'd and will interpret paths beginning with '/'
 *  relative to itself.
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target directory
 */
KFS_EXTERN rc_t CC KDirectoryOpenDirRead ( const KDirectory *self,
    const KDirectory **sub, bool chroot, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVOpenDirRead ( const KDirectory *self,
    const KDirectory **sub, bool chroot, const char *path, va_list args );

KFS_EXTERN rc_t CC KDirectoryOpenDirUpdate ( KDirectory *self,
    KDirectory **sub, bool chroot, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVOpenDirUpdate ( KDirectory *self,
    KDirectory **sub, bool chroot, const char *path, va_list args );

/* CreateDir
 *  create a sub-directory
 *
 *  "access" [ IN ] - standard Unix directory mode, e.g.0775
 *
 *  "mode" [ IN ] - a creation mode ( see explanation in defs.h ).
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target directory
 */
KFS_EXTERN rc_t CC KDirectoryCreateDir ( KDirectory *self,
    uint32_t access, KCreateMode mode, const char *path, ... );
KFS_EXTERN rc_t CC KDirectoryVCreateDir ( KDirectory *self,
    uint32_t access, KCreateMode mode, const char *path, va_list args );


/* CopyPath
 *  copies a file
 *
 *  "src_path" [ IN ] - path to source-file
 *
 *  "dst_path" [ IN ] - file to create
 *
 *  "path" [ IN ] - NUL terminated string in directory-native
 *  character set denoting target directory
 */
KFS_EXTERN rc_t CC KDirectoryCopyPath ( const KDirectory *src_dir,
    KDirectory *dst_dir, const char *src_path, const char * dst_path );


/* CopyPaths
 *  copies files, optional recursive in sub-dirs...
 *
 *  "recursive" [ IN ] - handle subdir's recursivly
 *
 *  "src" [ IN ] - what directory to copy
 *
 *  "dst" [ IN ] - into what directory to copy
 *      ( will be created if it does not already exist )
 *
 */
LIB_EXPORT rc_t CC KDirectoryCopyPaths( const KDirectory * src_dir,
    KDirectory *dst_dir, bool recursive, const char *src, const char *dst );


/* Copy
 *  detects if src is a file or a directory
 *
 *  "recursive" [ IN ] - handle subdir's recursivly 
 *            ( if srs is a directory )
 *
 *  "src" [ IN ] - what file/directory to copy
 *
 *  "dst" [ IN ] - into what file/directory to copy
 *      ( will be created if it does not already exist )
 *
 */
LIB_EXPORT rc_t CC KDirectoryCopy( const KDirectory * src_dir,
    KDirectory *dst_dir, bool recursive, const char *src, const char *dst );


/* NativeDir
 *  returns a native file-system directory node reference
 *  the directory root will be "/" and set to the native
 *  idea of current working directory
 *
 *  NB - the returned reference will be non-const, allowing
 *  modification operations to be attempted. these operations
 *  may still fail if the underlying FS disallows them.
 *
 *  "dir" [ OUT ] - return parameter for native directory
 */
KFS_EXTERN rc_t CC KDirectoryNativeDir ( KDirectory **dir );


#ifdef __cplusplus
}
#endif

#endif /* _h_kfs_directory_ */
