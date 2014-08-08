#ifndef _h_mmpool_
#define _h_mmpool_

#ifndef _h_mmap_
#include "mmap.h"
#endif

#ifndef _h_semaphore_
#include "semaphore.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * MMRegion
 *  metered memory mapped region of a file
 */

/* MMRegionOpenMetered
 *  acquires memory space from semaphore, and then from system
 */
int tracedb_mmregion_openmetered ( tracedb_MMRegion *mmr, const char *fname, int perm,
    tracedb_RSemaphoreRef sem, tracedb_timeoutref_t tm );
#define tracedb_MMRegionOpenMetered( mmr, fname, perm, sem, tm ) \
    mmregion_openmetered ( mmr, fname, perm, TRACEDB_RSEMAPHOREREF ( sem ), \
    TRACEDB_TIMEOUTREF ( tm ) )

/* MMRegionMapMetered
 *  attempts to map a region within given time period
 *
 *  "map_func" is either MMRegionRWMap or MMRegionROMap
 */
int tracedb_mmregion_mapmetered ( tracedb_MMRegion *mmr, tracedb_File f, size_t len, uint64_t pos,
    tracedb_RSemaphoreRef sem, tracedb_timeoutref_t tm,
    int ( * map_func ) ( tracedb_MMRegion *mmr, tracedb_File f, size_t len, uint64_t pos ) );
#define tracedb_MMRegionMapMetered( mmr, f, len, pos, sem, tm, map_func ) \
    mmregion_mapmetered ( mmr, f, len, pos, TRACEDB_RSEMAPHOREREF ( sem ), \
    TRACEDB_TIMEOUTREF ( tm ), map_func )

/* MMRegionUnmapMetered
 *  unmaps the region and returns space to pool
 */
int tracedb_mmregion_unmapmetered ( tracedb_MMRegion *mmr, tracedb_RSemaphoreRef sem );
#define tracedb_MMRegionUnmapMetered( mmr, sem ) \
    mmregion_unmapmetered ( mmr, TRACEDB_RSEMAPHOREREF ( sem ) )

/* MMRegionWhackMetered
 *  whacks the region and returns space to pool
 */
int tracedb_mmregion_whackmetered ( tracedb_MMRegion *mmr, tracedb_RSemaphoreRef sem );
#define tracedb_MMRegionWhackMetered( mmr, sem ) \
    mmregion_whackmetered ( mmr, TRACEDB_RSEMAPHOREREF ( sem ) )

#ifdef __cplusplus
}
#endif


#endif /* _h_mmpool_ */
