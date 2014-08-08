#ifndef _h_fdset_
#define _h_fdset_

#ifndef _h_fdio_
#include "fdio.h"
#endif

#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * FDSet
 */
typedef struct tracedb_FDSet tracedb_FDSet;
struct tracedb_FDSet
{
    unsigned int count;
    tracedb_fd_t fds [ __FD_SETSIZE ];
};

/* FDSetInit
 *  produces an initially empty set
 */
void tracedb_FDSetInit ( tracedb_FDSet *set );
#define tracedb_FDSetInit( set ) \
    ( ( set ) -> count = 0 )

/* FDSetWhack
 *  whacks whatever needs whacking
 */
#define tracedb_FDSetWhack( set ) \
    ( ( void ) 0 )

/* FDSetAdd
 *  adds a file descriptor to set
 */
int tracedb_FDSetAdd ( tracedb_FDSet *set, tracedb_fd_t fd );
#define tracedb_FDSetAdd( set, fd ) \
    ( ( ( set ) -> count >= ( sizeof ( set ) -> fds / sizeof ( set ) -> fds [ 0 ] ) ) ? \
      ENOBUFS : ( ( set ) -> fds [ ( set ) -> count ++ ] = ( fd ), 0 ) )

/* FDSetCopy
 *  copies data
 */
int tracedb_FDSetCopy ( tracedb_FDSet *to, const tracedb_FDSet *from );

/* FDSetMerge
 *  adds an entire set to another
 */
int tracedb_FDSetMerge ( tracedb_FDSet *to, const tracedb_FDSet *from );

/* FDSetHas
 *  tests set for fd inclusion
 */
bool tracedb_FDSetHas ( const tracedb_FDSet *set, tracedb_fd_t fd );

/* FDSetForEach
 *  executes supplied function on each included fd
 */
void tracedb_FDSetForEach ( const tracedb_FDSet *set,
    void ( * f ) ( tracedb_fd_t fd, void *data ), void *data );

/* FDSetDoUntil
 *  executes supplied function on each included fd
 *  or the function returns true
 */
void tracedb_FDSetDoUntil ( const tracedb_FDSet *set,
    bool ( * f ) ( tracedb_fd_t fd, void *data ), void *data );

/* FDSetToNative
 *  creates a native fd_set from FDSet
 *
 *  "native" [ OUT ] is the native fd_set to be initialized
 *
 *  "max_fd" [ IN/OUT ] upon input is the previously recorded maximum fd
 *  which should have been set initially to -1. upon return, it will be
 *  reset to any higher numbered fd from the input set.
 *
 *  returns EINVAL if any argument is invalid or any
 *  source fd is negative ( not from stdfs ) or beyond
 *  maximum value allowed in fd_set ( __FD_SETSIZE ).
 *
 * NB - only valid for fds from stdfs
 */
int tracedb_FDSetToNative ( const tracedb_FDSet *set, fd_set *native,
    tracedb_fd_t *max_fd );

/* FDSetFromNative
 *  creates an FDSet from a native fd_set
 *
 *  "native" [ IN ] is the initialized native fd_set
 *
 *  "max_fd" [ IN ] is the highest possible fd value in "native"
 *  this is a conservatively high value, in that it may exceed the
 *  actual maximum.
 *
 *  returns EINVAL if any set is invalid
 */
int tracedb_FDSetFromNative ( tracedb_FDSet *set, const fd_set *native,
    tracedb_fd_t max_fd );


#ifdef __cplusplus
}
#endif

#endif /* _h_fdset_ */
