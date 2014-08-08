#ifndef _h_vfd_vfdnet_
#define _h_vfd_vfdnet_

#ifndef _h_fdnet_
#include "fdnet.h"
#endif

#ifndef _h_vfdio_
#include "vfdio.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * virtual extensions to fdnet interface
 */

/* VNSInit - IDEMPOTENT
 *  initialize virtual network system manager
 *  must be invoked before any other vfdnet operations
 */
int tracedb_VNSInit ( void );

/* NSRegister
 *  register a network system handler
 *  attach a reference to object
 *  return a registration ticket in "ticket"
 *
 *  "ticket" [ OUT ] - return parameter for ticket
 *  used later to unregister network system
 *
 *  "name" [ IN ] - null terminated network system name
 *
 *  return values:
 *    EEXIST - a network system is already registered under given name
 */
int tracedb_NSRegister ( tracedb_ns_t self, void **ticket, const char *name );

/* NSUnregister
 *  removes an NS object from the registry,
 *  as identified by a ticket returned from NSRegister
 */
int tracedb_NSUnregister ( void *ticket );


/*--------------------------------------------------------------------------
 * implementation details - forwards
 */
typedef struct tracedb_NS tracedb_NS;
typedef struct tracedb_FDSock tracedb_FDSock;
typedef struct tracedb_FDStreamSock tracedb_FDStreamSock;
typedef struct tracedb_FDDatagramSock tracedb_FDDatagramSock;


/*--------------------------------------------------------------------------
 * FDSock
 *  fd based socket
 */
typedef struct tracedb_vtFDSock tracedb_vtFDSock;
struct tracedb_vtFDSock
{
    /* handler for FDSockWhack - not the main object whacker */
    int ( * whack ) ( tracedb_FDSock *self, bool input, bool output );

    /* standard operations on sock from fdnet.h */
    int ( * bind ) ( tracedb_FDSock *self, const tracedb_nsaddr_t *addr, int port, bool reuse );
    int ( * connect ) ( tracedb_FDSock *self, const tracedb_nsaddr_t *addr, int port );
    int ( * local ) ( const tracedb_FDSock *self, tracedb_nsaddr_t *addr, int *port );
    int ( * remote ) ( const tracedb_FDSock *self, tracedb_nsaddr_t *addr, int *port );
    int ( * setInputTimeout ) ( tracedb_FDSock *self, int mS );
    int ( * setOutputTimeout ) ( tracedb_FDSock *self, int mS );
    int ( * recv ) ( tracedb_FDSock *self, void *buffer, size_t max_bytes, size_t *num_read, bool no_sigpipe );
    int ( * send ) ( tracedb_FDSock *self, const void *buffer, size_t len, size_t *num_writ, bool no_sigpipe );
};

typedef struct tracedb_mdFDSock tracedb_mdFDSock;
struct tracedb_mdFDSock
{
    /* a privately defined constant, identifying
       more or less the "class" of fd. it is typed
       as a "size_t" to ensure proper pointer alignment */
    size_t variant;

    /* if classname metadata is to be generated,
       here is a const classname string */
#if VFD_CLASSNAMES
    const char *name;
#endif

    /* invoked when the last reference of an FD has
       been released and the object can be whacked */
    int ( * whack ) ( tracedb_FDSock *self );

    /* the FD-level vtbl */
    const tracedb_vtFD *vt1;

    /* FDSock-level vtbl */
    const tracedb_vtFDSock *vt2;
};

struct tracedb_FDSock
{
    /* class metadata */
    const tracedb_mdFDSock *md;

    /* all FD objects are reference counted */
    tracedb_atomic_t refcount;
};

/* FDSockInit
 *  initialize parent portion of an FDSock implementation
 *
 *  "meta" [ IN ] - implementation specific const mdFDSock*
 *
 *  "refs" [ IN ] - initial refcount of object, normally 1
 *  given that FD objects are created on demand and minimally
 *  are referenced by the object's file descriptor.
 */
#define tracedb_FDSockInit( self, meta, refs ) \
    ( void ) ( ( self ) -> md = ( meta ), \
               tracedb_atomic_set ( & ( self ) -> refcount, ( refs ) ) )


/*--------------------------------------------------------------------------
 * FDDatagramSock
 */
typedef struct tracedb_vtFDDatagramSock tracedb_vtFDDatagramSock;
struct tracedb_vtFDDatagramSock
{
    int ( * recvfrom ) ( tracedb_FDDatagramSock *self, void *buffer, size_t max_bytes,
        size_t *num_read, tracedb_nsaddr_t *addr, int *port, bool no_sigpipe );
    int ( * sendto ) ( tracedb_FDDatagramSock *self, const void *buffer, size_t len,
        size_t *num_writ, const tracedb_nsaddr_t *addr, int port, bool no_sigpipe );
};

typedef struct tracedb_mdFDDatagramSock tracedb_mdFDDatagramSock;
struct tracedb_mdFDDatagramSock
{
    /* a privately defined constant, identifying
       more or less the "class" of fd. it is typed
       as a "size_t" to ensure proper pointer alignment */
    size_t variant;

    /* if classname metadata is to be generated,
       here is a const classname string */
#if VFD_CLASSNAMES
    const char *name;
#endif

    /* invoked when the last reference of an FD has
       been released and the object can be whacked */
    int ( * whack ) ( tracedb_FDDatagramSock *self );

    /* the FD-level vtbl */
    const tracedb_vtFD *vt1;

    /* FDSock-level vtbl */
    const tracedb_vtFDSock *vt2;

    /* FDDatagramSock-level vtbl */
    const tracedb_vtFDDatagramSock *vt3;
};

struct tracedb_FDDatagramSock
{
    const tracedb_mdFDDatagramSock *md;
    tracedb_atomic_t refcount;
};

/* FDDatagramSockInit
 *  initialize parent portion of an FDDatagramSock implementation
 *
 *  "meta" [ IN ] - implementation specific const mdFDDatagramSock*
 *
 *  "refs" [ IN ] - initial refcount of object, normally 1
 *  given that FD objects are created on demand and minimally
 *  are referenced by the object's file descriptor.
 */
#define tracedb_FDDatagramSockInit( self, meta, refs ) \
    ( void ) ( ( self ) -> md = ( meta ), \
               tracedb_atomic_set ( & ( self ) -> refcount, ( refs ) ) )


/*--------------------------------------------------------------------------
 * FDStreamSock
 */
typedef struct tracedb_vtFDStreamSock tracedb_vtFDStreamSock;
struct tracedb_vtFDStreamSock
{
    int ( * listen ) ( tracedb_FDStreamSock *self, unsigned int qlen );
    int ( * accept ) ( tracedb_FDStreamSock *self, tracedb_FDSock **sock,
        tracedb_nsaddr_t *addr, int *port );
    int ( * setDelay ) ( tracedb_FDStreamSock *self, bool enable );
};

typedef struct tracedb_mdFDStreamSock tracedb_mdFDStreamSock;
struct  tracedb_mdFDStreamSock
{
    /* a privately defined constant, identifying
       more or less the "class" of fd. it is typed
       as a "size_t" to ensure proper pointer alignment */
    size_t variant;

    /* if classname metadata is to be generated,
       here is a const classname string */
#if VFD_CLASSNAMES
    const char *name;
#endif

    /* invoked when the last reference of an FD has
       been released and the object can be whacked */
    int ( * whack ) ( tracedb_FDStreamSock *self );

    /* the FD-level vtbl */
    const tracedb_vtFD *vt1;

    /* FDSock-level vtbl */
    const tracedb_vtFDSock *vt2;

    /* FDStreamSock-level vtbl */
    const tracedb_vtFDStreamSock *vt3;
};

struct tracedb_FDStreamSock
{
    const tracedb_mdFDStreamSock *md;
    tracedb_atomic_t refcount;
};

/* FDStreamSockInit
 *  initialize parent portion of an FDStreamSock implementation
 *
 *  "meta" [ IN ] - implementation specific const mdFDStreamSock*
 *
 *  "refs" [ IN ] - initial refcount of object, normally 1
 *  given that FD objects are created on demand and minimally
 *  are referenced by the object's file descriptor.
 */
#define tracedb_FDStreamSockInit( self, meta, refs ) \
    ( void ) ( ( self ) -> md = ( meta ), \
               tracedb_atomic_set ( & ( self ) -> refcount, ( refs ) ) )


/*--------------------------------------------------------------------------
 * NS
 *  network system object
 */
typedef struct tracedb_vtNS tracedb_vtNS;
struct tracedb_vtNS
{
    /* invoked when the last reference of an NS has
       been released and the object can be whacked */
    int ( * whack ) ( tracedb_NS *self );

    /* standard operations on ns described in fdnet.h */
    int ( * resolve ) ( tracedb_NS *self, const char *name, tracedb_nsaddr_t *addr );
    int ( * lookup ) ( tracedb_NS *self, const tracedb_nsaddr_t *addr, char *name, size_t max_size );
    int ( * portNumber ) ( tracedb_NS *self, const char *svc, int proto, int *port );
    int ( * portName ) ( tracedb_NS *self, int port, int proto, char *svc, size_t max_size );

    /* create/return a reference to FDSock
       that gets whacked when sock is closed */
    int ( * makeSock ) ( tracedb_NS *self, tracedb_FDSock **sock, int vers, int proto );

    /* select function on several fds */
    int ( * select ) ( tracedb_NS *self, tracedb_FDSet *read_set, tracedb_FDSet *write_set, tracedb_FDSet *except_set, tracedb_timeoutref_t tm );
};

struct tracedb_NS
{
    const tracedb_vtNS *vt;
    tracedb_atomic_t refcount;
};

/* NSInit
 *  initialize the parent portion of a network system implementation
 *
 *  "vtbl" [ IN ] - an implementation-specific const vtNS*
 */
#define tracedb_NSInit( self, vtbl ) \
    ( void ) ( ( self ) -> vt = ( vtbl ), \
               tracedb_atomic_set ( & ( self ) -> refcount, 0 ) )


#ifdef __cplusplus
}
#endif

#endif /* _h_vfd_vfdnet_ */
