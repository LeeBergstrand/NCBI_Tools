#ifndef _h_file_
#define _h_file_

#ifndef _h_fdio_
#include "fdio.h"
#endif

#ifndef _h_timeout_
#include "timeout.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define TRACEDB_FILE_IS_FD 1

/*--------------------------------------------------------------------------
 * File
 *  Unix native File is file descriptor
 */
typedef tracedb_fd_t tracedb_File;

/* FileRef
 *  allows flexibility in the definition of File as
 *  either a struct or some sort of handle, as it is today
 */
typedef tracedb_File tracedb_FileRef;
#define TRACEDB_FILEREF( f ) \
    ( f )
#define TRACEDB_FILEDEREF( f ) \
    ( f )


/* FileOpen
 *  opens an existing file
 *
 *  "f" is a File*
 *  "path" is a const char*
 *  "perm" is an int OpenPerm
 */
#define tracedb_FileOpen( f, path, perm ) \
    tracedb_FSOpenFile ( tracedb_stdfs, f, path, perm )

/* FileCreate
 *  creates or opens a file, based upon permissions
 *
 *  "f" is a File*
 *  "path" is a const char*
 *  "perm" is an int IOPerm
 *  "mode" is a mode_t Unix mode spec
 */
#define tracedb_FileCreate( f, path, perm, mode ) \
    tracedb_FSCreateFile ( tracedb_stdfs, f, path, perm, mode )

/* FileLock
 * FileLockRgn
 *  acquire a shared or exclusive lock on file,
 *  blocking until acquired or interrupted.
 *
 *  exclusive locks allow only a single owner,
 *  while shared locks allow multiple owners.
 */
#define tracedb_FileLock( f, exclusive ) \
    tracedb_FileLockRgn ( f, exclusive, 0, 0 )
int tracedb_FileLockRgn ( tracedb_File f, bool exclusive,
                        uint64_t bytes, uint64_t pos );

/* FileTryLock
 * FileTryLockRgn
 *  acquire a shared or exclusive lock on file,
 *  without blocking, returning
 */
#define tracedb_FileTryLock( f, exclusive ) \
    tracedb_FDFileLock ( f, exclusive )
#define tracedb_FileTryLockRgn( f, exclusive, bytes, pos ) \
    tracedb_FDFileLockRgn ( f, exclusive, bytes, pos )

/* FileTimedLock
 * FileTimedLockRgn
 *  acquire a reader or writer lock on file,
 *  waiting up to the specified timeout
 *
 *  see FileLock[Region] for all other behavior and parameters
 */
int file_timed_lock_rgn ( tracedb_File f, bool exclusive,
    uint64_t bytes, uint64_t pos, tracedb_timeoutref_t tm );

#define tracedb_FileTimedLock( f, exclusive, tm ) \
    file_timed_lock_rgn ( f, exclusive, 0, 0, TIMEOUTREF( tm ) )
#define tracedb_FileTimedLockRgn( f, exclusive, bytes, pos, tm ) \
    file_timed_lock_rgn ( f, exclusive, bytes, pos, TIMEOUTREF ( tm ) )

/* FileUnlock
 * FileUnlockRgn
 *  release a lock on the file as obtained by FileLock[Region]
 *
 *  releasing a read lock may still leave the file locked for read,
 *  depending upon the number of concurrent readers.
 *
 *  releasing a write lock will always unlock the file.
 *
 *  "f" is a File
 *  "bytes" is a size_t of the length of the locked region
 *  "pos" is an uint64_t specifying the start of the lock region
 */
#define tracedb_FileUnlock( f ) \
    tracedb_FDFileUnlock ( f )
#define tracedb_FileUnlockRgn( f, bytes, pos ) \
    tracedb_FDFileUnlockRgn ( f, bytes, pos )

/* FileSize
 *  access the file size and return it in "eof"
 *
 *  "f" is a File
 *  "eof" is an uint64_t*
 */
#define tracedb_FileSize( f, eof ) \
    tracedb_FDFileSize ( f, eof )

/* FileSetSize
 *  set the file end marker
 *
 *  "f" is a File
 *  "eof" is an uint64_t
 */
#define tracedb_FileSetSize( f, eof ) \
    tracedb_FDFileSetSize ( f, eof )

/* FilePosition
 *  reports position of File stream pointer
 *
 *  "f" is a File
 *  "pos" is an uint64_t*
 */
#define tracedb_FilePosition( f, pos ) \
    tracedb_FDFilePosition ( f, pos )

/* FileReposition
 *  sets the stream pointer of a File
 *
 *  "f" is a File
 *  "pos" is an uint64_t
 */
#define tracedb_FileReposition( f, pos ) \
    tracedb_FDFileReposition ( f, pos )

/* FileRead
 * FileReadPtr
 *  see "FDRead" and "FDFileRead" for documentation
 *
 *  "f" is a File
 *  "pos" is an uint64_t
 *  "buffer" is a void* where data are returned
 *  "max_bytes" is a size_t giving the usable space in "buffer"
 *  "num_read" is a size_t* where the number of valid bytes
 *     in "buffer" will be returned.
 */
#define tracedb_FileRead( f, pos, buffer, max_bytes, num_read ) \
    tracedb_FDFileRead ( f, pos, buffer, max_bytes, num_read )
#define tracedb_FileReadPtr( f, buffer, max_bytes, num_read ) \
    tracedb_FDRead ( f, buffer, max_bytes, num_read )

/* FileWrite
 * FileWritePtr
 *  see "FDWrite" and "FDFileWrite" for documentation
 *
 *  "f" is a File
 *  "pos" is an uint64_t
 *  "buffer" is a const void* of data to be written
 *  "bytes" is a size_t giving the valid bytes in "buffer"
 *  "num_writ" is a size_t* where the number of bytes
 *     successfully written will be returned.
 */
#define tracedb_FileWrite( f, pos, buffer, bytes, num_writ ) \
    tracedb_FDFileWrite ( f, pos, buffer, bytes, num_writ )
#define tracedb_FileWritePtr( f, buffer, bytes, num_writ ) \
    tracedb_FDWrite ( f, buffer, bytes, num_writ )

/* FileWriteAll
 * FileWriteAllPtr
 *  see "FDWriteAll" and "FDFileWriteAll" for documentation
 *
 *  "f" is a File
 *  "pos" is an uint64_t
 *  "buffer" is a const void* of data to be written
 *  "bytes" is a size_t giving the valid bytes in "buffer"
 *  "num_writ" is a size_t* where the number of bytes
 *     successfully written will be returned.
 *  "handle_intr" and "data" are for interrupt callback
 */
#define tracedb_FileWriteAll( f, pos, buffer, bytes, num_writ, handle_intr, data ) \
    tracedb_FDFileWriteAll ( f, pos, buffer, bytes, num_writ, handle_intr, data )
#define tracedb_FileWriteAllPtr( f, buffer, bytes, num_writ, handle_intr, data ) \
    tracedb_FDWriteAll ( f, buffer, bytes, num_writ, handle_intr, data )

/* FileWhack
 *  close the File
 */
#define tracedb_FileWhack( f ) \
    tracedb_FDFileWhack ( f )

#ifdef __cplusplus
}
#endif


#endif /* _h_file_ */
