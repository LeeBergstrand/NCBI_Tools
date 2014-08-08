#ifndef _h_vfd_priv_
#define _h_vfd_priv_

#ifndef _h_vfd_vfdio_
#include "vfd/vfdio.h"
#endif

#ifndef _h_vfd_vfdnet_
#include "vfd/vfdnet.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * VFDBits
 *  for detecting fd classes
 */
enum tracedb_VFDBits
{
    /* level 1 is the base FD stream class */
    tracedb_vfdStream = 0,

    /* level 2 are the basic FD subclasses */
    tracedb_vfdFile = 1,
    tracedb_vfdPipe = 2,
    tracedb_vfdSock = 3,
    tracedb_vfdLvl2 = 15,

    /* level 3 are variants upon level 2 */
    tracedb_vfdDgramSock = ( 1 << 4 ),
    tracedb_vfdStreamSock = ( 2 << 4 ),
    tracedb_vfdLvl3 = ( 15 << 4 )
};


/*--------------------------------------------------------------------------
 * FD
 */

/* FDFind
 *  looks up an object based on fd
 *  returns a new reference to object
 *
 *  legal fd value range is < -1
 */
int tracedb_FDFind ( tracedb_fd_t fd, tracedb_FD **obj );

/* FDVariant
 *  gives the type of FD
 */
#define tracedb_FDVariant( obj ) \
    ( int ) ( ( obj ) -> md -> variant )


/*--------------------------------------------------------------------------
 * FNS
 *  file and network system slot operations
 */
typedef union tracedb_FNS tracedb_FNS;
union tracedb_FNS
{
    tracedb_FS fs;
    tracedb_NS ns;
};

/* FNSOpenAnon
 *  allows an implementation to create a descriptor for object
 *  may be subsequently given an external name with FNSRegister
 */
int tracedb_FSOpenAnon ( tracedb_FS *self, tracedb_fs_t *fs );
int tracedb_NSOpenAnon ( tracedb_NS *self, tracedb_ns_t *ns );

/* FNSFind
 *  looks up an FNS object
 *  returns a new reference to object
 *
 *  valid range for fs/ns is > 0
 */
int tracedb_FSFind ( tracedb_fs_t fs, tracedb_FS **obj );
int tracedb_NSFind ( tracedb_ns_t ns, tracedb_NS **obj );

/* FNSNewFD
 *  create a new slot for object
 *  NB - does not touch object refcount
 */
int tracedb_FNSNewFD ( tracedb_fd_t *fd, tracedb_FD *obj );

/* FNSDupFD
 *  duplicate a slot to any available id
 *  attach a reference to its object
 */
int tracedb_FNSDupFD ( tracedb_fd_t src, tracedb_fd_t *dst );

/* FNSAssignFD
 *  duplicate a slot to a specific id
 *  attach new reference to object
 */
int tracedb_FNSAssignFD ( tracedb_fd_t src, tracedb_fd_t dst );

/* FNSRenumberFD
 *  reassign object from one slot to another
 */
int tracedb_FNSRenumberFD ( tracedb_fd_t src, tracedb_fd_t dst );

/* FNSWhackFD
 *  whacks slot
 *  releases object
 */
int tracedb_FNSWhackFD ( tracedb_fd_t fd );

#ifdef __cplusplus
}
#endif

#endif /* _h_vfd_priv_ */
