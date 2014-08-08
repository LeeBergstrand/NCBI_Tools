#define FORCE_MALLOC_MMAP 1

#if FORCE_MALLOC_MMAP
#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#endif

#include "mmap.h"
#include "file.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>


#if FORCE_MALLOC_MMAP
static
void *malloc_mmap ( void *start, size_t length,
    int prot, int flags, int fd, off_t offset )
{
    ssize_t num_read;
    void *addr = valloc ( length );
    if ( addr == NULL )
        return MAP_FAILED;

    num_read = pread ( fd, addr, length, offset );
    if ( num_read < 0 )
    {
        free ( addr );
        return MAP_FAILED;
    }

    if ( ( size_t ) num_read < length )
        memset ( & ( ( char* ) addr ) [ num_read ], 0, length - num_read );

    return addr;
}

static
int malloc_munmap ( void *start, size_t length )
{
    if ( start == NULL || start == MAP_FAILED )
        errno = EINVAL;
    else
        free ( start );
    return errno != 0 ? -1 : 0;
}
#endif

/*--------------------------------------------------------------------------
 * MemRegion
 *  abstract region of memory
 */

/* MemRegionSubrgn
 *  creates a sub-region of an existing one
 *  note that the sub-region is always a non-allocated copy
 *  and is dependent upon the lifetime of its source
 *
 *  returns "sub" if "pos" was valid
 *  or NULL otherwise
 *
 *  "size" may be 0 to indicate infinite length
 *  or may extend beyond end of source region.
 */
tracedb_MemRegion *tracedb_MemRegionSubrgn ( const tracedb_MemRegion *mr,
    tracedb_MemRegion *sub, size_t size, uint64_t pos )
{
    uint64_t end;
    size_t bytes;

    if ( mr == NULL || sub == NULL )
        return NULL;

    if ( pos < mr -> pos )
        return NULL;
    sub -> addr = ( char* ) mr -> addr + ( pos - mr -> pos );
    sub -> pos = pos;

    end = mr -> pos + mr -> size;
    if ( pos >= end )
        return NULL;

    bytes = ( size_t ) ( end - pos );
    if ( pos + bytes != end )
        return NULL;

    if ( size != 0 && bytes > size )
        bytes = size;
    sub -> size = bytes;

    return sub;
}


/*--------------------------------------------------------------------------
 * MMRegion
 *  memory mapped region of a file
 */

/* MMRegionCreateRWMap
 *  internals of mapping a read/write file
 */
static
int tracedb_MMRegionCreateRWMap ( tracedb_MMRegion *mmr,
    uint64_t eof, size_t len, uint64_t pos )
{
    int status = 0;

    /* before anything else, the mapped region must be
       aligned on page boundaries */
    size_t bytes;
    uint64_t left, right, range;
    uint64_t pg_mask = sysconf ( _SC_PAGE_SIZE ) - 1;

    if ( len == 0 )
        ++ len;

    left = pos & ~ pg_mask;
    right = ( pos + len + pg_mask ) & ~ pg_mask;
    range = right - left;
    bytes = ( size_t ) range;

    /* check bytes for huge range */
    if ( ( uint64_t ) bytes != range )
        return ENOMEM;

    /* extend the underlying file if needed */
    if ( eof < right )
        status = tracedb_FileSetSize ( mmr -> f, right );

    /* map the region */
    if ( status == 0 )
    {
        mmr -> mr . addr = mmap ( NULL, bytes,
            PROT_READ | PROT_WRITE, MAP_SHARED, mmr -> f, left );
        if ( mmr -> mr . addr == MAP_FAILED )
        {
            status = errno;
            if ( status == EINVAL && ( range >> 30 ) != 0 )
                status = ENOMEM;
            if ( eof < right )
                tracedb_FileSetSize ( mmr -> f, eof );
        }
        else
        {
            /* complete the structure */
            mmr -> mr . size = bytes;
            mmr -> mr . pos = left;
        }
    }

    return status;
}

/* MMRegionCreateROMap
 *  internals of mapping a read-only file
 */
static
int tracedb_MMRegionCreateROMap ( tracedb_MMRegion *mmr,
    uint64_t eof, size_t len, uint64_t pos )
{
    int status = 0;
    uint64_t pg_mask = sysconf ( _SC_PAGE_SIZE ) - 1;

    /* zero-length files are not mapped at all */
    if ( eof == 0 || len == 0 )
    {
        mmr -> mr . addr = "zero length region";
        mmr -> mr . size = 0;
        mmr -> mr . pos = pos;
    }
    else
    {
        /* the mapped region must be aligned on page boundaries */
        size_t bytes;
        uint64_t left, right, range;

        left = pos & ~ pg_mask;
        right = ( pos + len + pg_mask ) & ~ pg_mask;
        
        /* crop the right edge to eof */
        if ( eof < right )
            right = eof;

        range = right - left;
        bytes = ( size_t ) range;

        /* check bytes for huge range */
        if ( ( uint64_t ) bytes != range )
            return ENOMEM;
        
        /* map the region */
        mmr -> mr . addr = mmap ( NULL, bytes,
            PROT_READ, MAP_SHARED, mmr -> f, left );
        if ( mmr -> mr . addr == MAP_FAILED )
        {
            status = errno;
            if ( status == EINVAL && ( range >> 30 ) != 0 )
                status = ENOMEM;
        }
        else
        {
            /* complete the structure */
            mmr -> mr . size = bytes;
            mmr -> mr . pos = left;
        }
    }

    return status;
}

/* MMRegionOpen
 *  a single-step open of an entire named file
 *
 *  note that opening an empty file will NOT create a map
 *  but will not return an error: it will create a zero-length
 *  region and this must be respected.
 */
int tracedb_MMRegionOpen ( tracedb_MMRegion *mmr,
    const char *path, int perm )
{
    int status;

    if ( mmr == NULL )
        return EINVAL;

    status = tracedb_FileOpen ( & mmr -> f, path, perm | tracedb_permRead );
    if ( status == 0 )
    {
        uint64_t eof;
        status = tracedb_FileSize ( mmr -> f, & eof );
        if ( status == 0 )
        {
            if ( ( perm & tracedb_permWrite ) != 0 )
                status = tracedb_MMRegionCreateRWMap ( mmr, eof,
                    ( size_t ) eof, 0 );
            else
                status = tracedb_MMRegionCreateROMap ( mmr, eof,
                    ( size_t ) eof, 0 );
            if ( status == 0 )
                return 0;
        }
        tracedb_FileWhack ( mmr -> f );
    }
    return status;
}

/* MMRegionCreate
 *  a single-step create of an entire named file
 *
 *  permReadWrite will always be forced whether specified or not
 */
int tracedb_MMRegionCreate ( tracedb_MMRegion *mmr,
    const char *path, int perm, mode_t mode )
{
    int status;

    if ( mmr == NULL )
        return EINVAL;

    status = tracedb_FileCreate ( & mmr -> f, path,
                        perm | tracedb_permReadWrite, mode );
    if ( status == 0 )
    {
        uint64_t eof;
        status = tracedb_FileSize ( mmr -> f, & eof );
        if ( status == 0 )
        {
            status = tracedb_MMRegionCreateRWMap ( mmr, eof,
                ( size_t ) eof, 0 );
            if ( status == 0 )
                return 0;
        }
        tracedb_FileWhack ( mmr -> f );
    }
    return status;
}

/* MMRegionRWMap
 *  map a region of a file for read/write
 */
int tracedb_MMRegionRWMap ( tracedb_MMRegion *mmr,
    tracedb_File f, size_t len, uint64_t pos )
{
    uint64_t eof;
    int status;

    if ( mmr == NULL )
        return EINVAL;

    status = tracedb_FileSize ( f, & eof );
    if ( status == 0 )
    {
        mmr -> f = f;
        status = tracedb_MMRegionCreateRWMap ( mmr, eof, len, pos );
    }

    return status;
}

/* MMRegionROMap
 *  map a region of a file for read only
 */
int tracedb_MMRegionROMap ( tracedb_MMRegion *mmr, tracedb_File f,
    size_t len, uint64_t pos )
{
    uint64_t eof;
    int status;

    if ( mmr == NULL )
        return EINVAL;

    if ( len == 0 )
    {
        mmr -> f = f;
        return tracedb_MMRegionCreateROMap ( mmr, 0, 0, pos );
    }

    status = tracedb_FileSize ( f, & eof );
    if ( status == 0 )
    {
        mmr -> f = f;
        status = tracedb_MMRegionCreateROMap ( mmr, eof, len, pos );
    }

    return status;
}

/* MMRegionRWRemap
 *  remap an existing region with appropriate access
 */
int tracedb_MMRegionRWRemap ( tracedb_MMRegion *mmr,
    size_t size, uint64_t pos )
{
    int status;

    size_t bytes;
    uint64_t left, right, range, eof;
    uint64_t pg_mask = sysconf ( _SC_PAGE_SIZE ) - 1;

    if ( size == 0 )
        return tracedb_MMRegionUnmap ( mmr );

    if ( mmr == NULL )
        return EINVAL;

    /* before anything else, the mapped region must be
       aligned on page boundaries */
    left = pos & ~ pg_mask;
    right = ( pos + size + pg_mask ) & ~ pg_mask;
    range = right - left;
    bytes = ( size_t ) range;

    /* check bytes for huge range */
    if ( ( uint64_t ) bytes != range )
        return ENOMEM;

    /* extend the underlying file if needed */
    status = tracedb_FileSize ( mmr -> f, & eof );
    if ( status == 0 && eof < right )
        status = tracedb_FileSetSize ( mmr -> f, right );

    /* remap the region */
    if ( status == 0 )
    {
        void *addr;

        /* if the starting positions have changed,
           need to create a new map */
        if ( pos != mmr -> mr . pos )
        {
            if ( mmr -> mr . size > 0 )
            {
                if ( munmap ( mmr -> mr . addr, mmr -> mr . size ) != 0 )
                    return errno;
                mmr -> mr . addr = MAP_FAILED;
                mmr -> mr . size = 0;
            }

            addr = mmap ( NULL, bytes,
                PROT_READ | PROT_WRITE, MAP_SHARED, mmr -> f, left );
        }

        /* if just resizing the same guy */
        else
        {
            if ( bytes == mmr -> mr . size )
                return 0;

            addr = mremap ( mmr -> mr . addr,
                mmr -> mr . size, bytes, MREMAP_MAYMOVE );
        }

        if ( addr == MAP_FAILED )
        {
            status = errno;
            if ( status == EINVAL && ( range >> 30 ) != 0 )
                status = ENOMEM;
            if ( eof < right )
                tracedb_FileSetSize ( mmr -> f, eof );
        }
        else
        {
            /* complete the structure */
            mmr -> mr . addr = addr;
            mmr -> mr . size = bytes;
            mmr -> mr . pos = left;
        }
    }

    return status;
}

/* MMRegionRORemap
 *  remap an existing region with appropriate access
 */
int tracedb_MMRegionRORemap ( tracedb_MMRegion *mmr,
    size_t size, uint64_t pos )
{
    int status;

    void *addr;
    size_t bytes;
    uint64_t left, right, range, eof;
    uint64_t pg_mask = sysconf ( _SC_PAGE_SIZE ) - 1;

    if ( size == 0 )
        return tracedb_MMRegionUnmap ( mmr );

    if ( mmr == NULL )
        return EINVAL;

    /* determine end of file */
    status = tracedb_FileSize ( mmr -> f, & eof );
    if ( status != 0 )
        return status;

    /* the mapped region must be aligned on page boundaries */
    left = pos & ~ pg_mask;
    right = ( pos + size + pg_mask ) & ~ pg_mask;
        
    /* crop the right edge to eof */
    if ( eof < right )
        right = eof;

    range = right - left;
    bytes = ( size_t ) range;
    
    /* check bytes for huge range */
    if ( ( uint64_t ) bytes != range )
        return ENOMEM;
        
    /* if the starting positions have changed,
       need to create a new map */
    if ( pos != mmr -> mr . pos )
    {
        if ( mmr -> mr . size > 0 )
        {
            if ( munmap ( mmr -> mr . addr, mmr -> mr . size ) != 0 )
                return errno;
            mmr -> mr . addr = MAP_FAILED;
            mmr -> mr . size = 0;
        }
        
        addr = mmap ( NULL, bytes,
            PROT_READ, MAP_SHARED, mmr -> f, left );
    }

    /* if just resizing the same guy */
    else
    {
        if ( bytes == mmr -> mr . size )
            return 0;
        
        addr = mremap ( mmr -> mr . addr,
            mmr -> mr . size, bytes, MREMAP_MAYMOVE );
    }

    if ( addr == MAP_FAILED )
    {
        status = errno;
        if ( status == EINVAL && ( range >> 30 ) != 0 )
            status = ENOMEM;
    }
    else
    {
        /* complete the structure */
        mmr -> mr . addr = addr;
        mmr -> mr . size = bytes;
        mmr -> mr . pos = left;
    }

    return status;
}

/* MMRegionUnmap
 */
int tracedb_MMRegionUnmap ( tracedb_MMRegion *mmr )
{
    if ( mmr == NULL )
        return EINVAL;
    if ( mmr -> mr . size > 0 )
    {
        if ( munmap ( mmr -> mr . addr, mmr -> mr . size ) != 0 )
            return errno;

        mmr -> mr . size = 0;
        mmr -> mr . pos = 0;
    }

    mmr -> mr . addr = MAP_FAILED;
    return 0;
}

/* MMRegionWhack
 */
int tracedb_MMRegionWhack ( tracedb_MMRegion *mmr )
{
    int status = tracedb_MMRegionUnmap ( mmr );
    if ( status == 0 )
    {
        status = tracedb_FileWhack ( mmr -> f );
        if ( status == 0 )
            mmr -> f = -1;
    }
    return status;
}

/* MMRegionPageRound
 *  private function to return the number of bytes actually used by map
 */
size_t tracedb_MMRegionPageRound ( size_t len, uint64_t pos )
{
    uint64_t pg_mask = sysconf ( _SC_PAGE_SIZE ) - 1;
    uint64_t left = pos & ~ pg_mask;
    uint64_t right = ( pos + len + pg_mask ) & ~ pg_mask;
    uint64_t range = right - left;
    size_t bytes = ( size_t ) range;

    /* check bytes for huge range */
    if ( ( uint64_t ) bytes != range )
        return ( size_t ) -1;

    return bytes;
}
