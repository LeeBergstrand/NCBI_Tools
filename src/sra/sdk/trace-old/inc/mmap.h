#ifndef _h_mmap_
#define _h_mmap_

#ifndef _h_file_
#include "file.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * MemRegion
 *  abstract region of memory
 *
 *  this region is meant to be a window upon
 *  a memory mapped file or other object within
 *  virtual address space.
 *
 *  it is the preferred object to pass to functions requiring
 *  access to a range of memory but not to manipulate the file map.
 *
 *  the given address corresponds to the starting position of the region.
 */
typedef struct tracedb_MemRegion tracedb_MemRegion;
struct tracedb_MemRegion
{
    void *addr;
    size_t size;
    uint64_t pos;
};

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
    tracedb_MemRegion *sub, size_t size, uint64_t pos );


/*--------------------------------------------------------------------------
 * MMRegion
 *  memory mapped region of a file
 */
typedef struct tracedb_MMRegion tracedb_MMRegion;
struct tracedb_MMRegion
{
    tracedb_MemRegion mr;
    tracedb_File f;
};


/* MMRegionOpen
 *  a single-step open of an entire named file
 *
 *  note that opening an empty file will NOT create a map
 *  but will not return an error: it will create a zero-length
 *  region and this must be respected.
 *
 *  permRead will always be forced whether specified or not
 */
int tracedb_MMRegionOpen ( tracedb_MMRegion *mmr, const char *path,
    int perm );

/* MMRegionCreate
 *  a single-step create of an entire named file
 *
 *  permReadWrite will always be forced whether specified or not
 */
int tracedb_MMRegionCreate ( tracedb_MMRegion *mmr, const char *path,
    int perm, mode_t mode );

/* MMRegionRWMap
 *  maps a region of a file for read/write
 */
int tracedb_MMRegionRWMap ( tracedb_MMRegion *mmr, tracedb_File f,
    size_t size, uint64_t pos );

/* MMRegionROMap
 *  maps a region of a file for read only
 */
int tracedb_MMRegionROMap ( tracedb_MMRegion *mmr, tracedb_File f,
    size_t size, uint64_t pos );

/* MMRegionRWRemap
 * MMRegionRORemap
 *  remap an existing region with appropriate access
 */
int tracedb_MMRegionRWRemap ( tracedb_MMRegion *mmr, size_t size, uint64_t pos );
int tracedb_MMRegionRORemap ( tracedb_MMRegion *mmr, size_t size, uint64_t pos );

/* MMRegionUnmap
 *  tears down the memory map but leaves file open
 */
int tracedb_MMRegionUnmap ( tracedb_MMRegion *mmr );

/* MMRegionWhack
 *  tears down map and whacks file
 */
int tracedb_MMRegionWhack ( tracedb_MMRegion *mmr );

/* MMRegionPageRound
 *  returns the number of bytes that would be used by
 *  mapping the specified bytes at the given file offset.
 *
 *  while technically not an MMRegion function, it
 *  belongs with the module.
 */
size_t tracedb_MMRegionPageRound ( size_t size, uint64_t pos );

#ifdef __cplusplus
}
#endif


#endif /* _h_mmap_ */
