/*==============================================================================

  tracedb.h

    a flat-file storage for arbitrarily sized records

    the API reflects the needs of executing within the TraceOS
    environment, where the path to server/database for any
    range of ids is known and kept external to this library.

    the user will start by initializing a TraceDB with two
    hard limits: the file limit, and the VM limit. one controls
    how many files should be left open and the other controls
    how much virtual address space ( RAM ) should be consumed.

    hard limits are used as quotas to prevent, of course, potential
    depletion of resources and starvation of some procedures, but
    more importantly they enable blocking resource allocation with
    timeouts so that a request on one thread does not fail when
    another has temporarily used all allowed resources, but instead
    can wait until that thread completes and releases them.

    soft limits establish cache trigger points, that are designed
    to wake up a background gc thread when exceeded. the main purpose
    of the gc thread is to perform cache flushing and resource
    release so that cycles are not spent on doing so in the foreground.
    soft limits are initially set to hard limits, but may be reset
    after the fact. a soft limit of 0 disables caching, while a
    limit >= the corresponding hard limit disables background flushing.
 */


#ifndef _h_tracedb_
#define _h_tracedb_

#ifndef _h_cond_
#include "cond.h"
#endif

#ifndef _h_btcache_
#include "btcache.h"
#endif

#ifndef _h_semaphore_
#include "semaphore.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * ti_t
 *  a trace id
 */
#ifndef ti_t_defined
typedef uint64_t ti_t;
#define ti_t_defined
#endif


/*--------------------------------------------------------------------------
 * TraceDB
 *  access blobs by their id
 *
 *  the blobs are organized into one or more directories,
 *  each with its own index.
 *
 *  it is possible to store a large quantity of blobs within a single
 *  directory structure, but the index file should not grow too large,
 *  especially on 32 bit architectures, because the current implementation
 *  maps an entire index file into memory.
 *
 *  to enable verbose execution tracing printfs, set "verbose" to
 *  some level > 0. the higher the number, the more verbose the output.
 *  output is generated to stdout.
 */
typedef struct TraceDB TraceDB;
struct TraceDB
{
    /* caches */
    tracedb_BTCache icache, dcache;

    /* semaphores */
    tracedb_Semaphore isem, dsem, gsem;

    /* locations */
    tracedb_BTree paths;

    /* condition for activating GC thread */
    tracedb_Condition gcond;

    /* cache limits */
    unsigned int ilim, dlim;
    unsigned int imax, dmax;

    /* timeout for opening files */
    int timeout;

    /* execution verbosity */
    int verbose;

    /* communication with gc */
    volatile bool quitting;
};

/* TraceDBInit
 *  initializes db resources with limits
 *
 *  "hard_file_limit" sets the maximum number of open files
 *  under this db.
 *
 *  "hard_vm_limit" sets the maximum virtual address space consumption
 *  under this db
 *
 *  "db" is initialized with a default timeout for multi-threaded
 *  applications. in single-threaded apps, the timeout should be
 *  reset to 0 ( NB: setting a negative timeout should only be done
 *  while debugging ).
 */
int TraceDBInit ( TraceDB *db, unsigned int hard_file_limit, size_t hard_vm_limit );

/* TraceDBGet/SetSoftFileLimit
 *  accessors for the data file cache limits
 *
 *  setting the new file limit lower than the number of cached files
 *  will trigger a bg thread gc pass but will not flush the cache
 *  immediately.
 */
int TraceDBGetSoftFileLimit ( const TraceDB *db, unsigned int *limit );
int TraceDBSetSoftFileLimit ( TraceDB *db, unsigned int limit );

/* TraceDBGet/SetSoftMemLimit
 *  accessors for the vm cache limits
 *
 *  setting the new vm limit lower than the total of cached indices
 *  will trigger a bg thread gc pass but will not flush the cache
 *  immediately.
 */
int TraceDBGetSoftMemLimit ( const TraceDB *db, size_t *vm_limit );
int TraceDBSetSoftMemLimit ( TraceDB *db, size_t vm_limit );

/* TraceDBRunGCTasks
 *  runs garbage collection tasks every "mS" milliseconds
 *  until the process exits or TraceDBWhack is called on the db
 *
 *  NB - this is a blocking function and must be run on its own thread
 */
int TraceDBRunGCTasks ( TraceDB *db, int mS );

/* TraceDBRead
 *  reads from a trace blob
 *
 *  reads a maximum of "max_len" bytes into "buffer",
 *  starting at "offset" into blob, returns the number
 *  of bytes read in "num_read" and the bytes remaining
 *  to be read in "remaining", i.e.
 *
 *    remaining = blob_size - offset - num_read;
 */
int TraceDBRead ( TraceDB *db, const char *path, ti_t id,
    void *buffer, size_t max_len, size_t offset,
    size_t *num_read, size_t *remaining );

/* TraceDBWhack
 *  releases all resources
 *  sets object into an unusable state
 *
 *  will wait on a GC thread if running, and can return
 *  an error if the thread does not exit within the supplied
 *  timeout.
 */
int TraceDBWhack ( TraceDB *db, int gc_timeout );


#ifdef __cplusplus
}
#endif

#endif /* _h_tracedb_ */
