#ifndef _h_fdnet_
#define _h_fdnet_

#ifndef _h_itypes_
#include "itypes.h"
#endif

#ifndef _h_fdio_
#include "fdio.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * NSProto
 *  a networking protocol identifier
 */
enum tracedb_NSProto
{
    tracedb_ipICMP,  /* Internet Control Message Protocol */
    tracedb_upIPC,   /* Inter-Process Communication       */
    tracedb_ipUDP,   /* User Datagram Protocol            */
    tracedb_ipTCP    /* Transmission Control Protocol     */
};


/*--------------------------------------------------------------------------
 * ns_t
 *  network system - index to a registered network
 *
 *  the special value of -1 means "invalid"
 *
 *  value 0 represents native networking, defined as "stdns"
 *
 *  non-zero values represent alternate network systems
 */
typedef int tracedb_ns_t;

/* stdns
 *  the native network system
 */
#define tracedb_stdns 0


/*--------------------------------------------------------------------------
 * nsaddr_t
 *  an IPC, IPv4 or IPv6 network address
 *  when "vers" == 0, the block represents an IPC address
 *  otherwise, it selects the IP version
 */
typedef struct tracedb_nsaddr_t tracedb_nsaddr_t;
struct tracedb_nsaddr_t
{
    uint32_t vers;
    union
    {
        char path [ 108 ];
        uint8_t ipv4 [ 4 ];
        uint16_t ipv6 [ 8 ];
    } u;
};

/*--------------------------------------------------------------------------
 * NS
 *  operations on network system
 *
 *  by definition all paths are simply
 *  null-terminated 8-bit character strings
 *
 *  the strings themselves are either ASCII or UTF-8
 *  or can be opaque to the code
 */

/* NSOpen
 *  finds a registered, named network system
 *
 *  "name" [ IN, NULL OKAY ] - an optional null-terminated
 *  string giving the name of the network system to be opened.
 *  either NULL or the special value "stdns" will be
 *  taken to mean the standard network system.
 *
 *  return values:
 */
int tracedb_NSOpen ( tracedb_ns_t *ns, const char *name );

/* NSCopy
 *  create a copy of a network system descriptor
 *
 *  "copy" [ OUT ] - return parameter for copy
 *
 *  return values:
 */
int tracedb_NSCopy ( tracedb_ns_t self, tracedb_ns_t *copy );

/* NSWhack
 *  closes an ns_t
 *
 *  return values:
 */
int tracedb_NSWhack ( tracedb_ns_t self );

/* NSResolve
 *  resolves texual address to a binary address
 *
 *  "name" [ IN ] - a null-terminated textual address
 *  may be either a symbolic ( DNS ) name or a numeric
 *  address. in the latter case IPv4 and IPv6 patterns
 *  are supported.
 *
 *  "addr" [ OUT ] - return parameter for binary address
 *
 *  return values:
 */
int tracedb_NSResolve ( tracedb_ns_t self, const char *name,
    tracedb_nsaddr_t *addr );

/* NSLookup
 *  performs a reverse DNS lookup of ip address
 *
 *  "addr" [ IN ] - an initialized binary address
 *
 *  "name" [ OUT ] and "max_size" [ IN ] - return parameter
 *  for name buffer
 *
 *  return values:
 */
int tracedb_NSLookup ( tracedb_ns_t self,
    const tracedb_nsaddr_t *addr, char *name, size_t max_size );

/* NSPortNumber
 *  resolves a port number for a named or numeric service
 *
 *  "svc" [ IN ] - a null-terminated service name, e.g. "ftp"
 *
 *  "proto" [ IN ] - an NSProto value
 *
 *  "port" [ OUT ] - return parameter for port number
 *
 *  return values:
 */
int tracedb_NSPortNumber ( tracedb_ns_t self,
    const char *svc, int proto, int *port );

/* NSPortName
 *  performs a reverse service name lookup by port number and protocol
 *
 *  "port" [ IN ] - an integer port number
 *
 *  "proto" [ IN ] - an NSProto value
 *
 *  "svc" [ OUT ] and "max_size" [ IN ] - return parameter for
 *  port name buffer
 *
 *  return values:
 */
int tracedb_NSPortName ( tracedb_ns_t self,
    int port, int proto, char *svc, size_t max_size );

/* NSResolveEndpoint
 *  resolves endpoint = [ <host> ':' ] <portnumber>
 *
 *  "endpoint" [ IN ] - a null-terminated string minimally specifying
 *  a port number and optionally combined with a host spec.
 *
 *  "proto" [ IN ] - an NSProto value
 *
 *  "addr" [ OUT ] - resolved host address, or
 *  IPv4 INADDR_ANY ( all zeros ) if the host spec is missing
 *
 *  "port" [ OUT ] - resolve port number
 *
 *  return values:
 */
int tracedb_NSResolveEndpoint ( tracedb_ns_t self,
    const char *endpoint, int proto,
    tracedb_nsaddr_t *addr, int *port );

/* NSMakeSock
 *  create an IPC or IP socket for a given IP version and NS protocol
 *
 *  "sock" [ OUT ] - return parameter for newly created socket
 *
 *  "vers" [ IN ] - for IP protocols, a supported version
 *  number ( 4 or 6 ). for IPC, must be zero.
 *
 *  "proto" [ IN ] - an NSProto value
 *
 *  return values:
 */
int tracedb_NSMakeSock ( tracedb_ns_t self, tracedb_fd_t *sock,
    int vers, int proto );


/*--------------------------------------------------------------------------
 * FDSock
 *  an fd supporting socket operations
 */

/* FDSockBind
 *  give the socket a local ip address
 *
 *  "addr" [ IN ] and "port" [ IN ] - the desired local
 *  endpoint of the socket.

 *  "reuse" [ IN ] - indicates whether the local endpoint
 *  should be made available for immediate reuse after this
 *  socket is no longer bound to it.
 *
 *  return values:
 */
int tracedb_FDSockBind ( tracedb_fd_t self,
    const tracedb_nsaddr_t *addr, int port, bool reuse );

/* FDSockConnect
 *  define or initiate a peer connection
 *
 *  for "connection-oriented" ( stream ) protocols ( tcp ),
 *  open and negotiate a connection with peer.
 *
 *  for "connection-less" ( dgram ) protocols ( udp, icmp ),
 *  set default recipient address and single accept address.
 *
 *  "addr" [ IN ] and "port" [ IN ] - peer endpoint
 *
 *  return values:
 */
int tracedb_FDSockConnect ( tracedb_fd_t self,
    const tracedb_nsaddr_t *addr, int port );

/* FDSockLocal
 *  returns local endpoint
 *
 *  "addr" [ OUT ] and "port" [ OUT ] - return parameters
 *  for local endpoint
 *
 *  return values:
 */
int tracedb_FDSockLocal ( tracedb_fd_t self,
    tracedb_nsaddr_t *addr, int *port );

/* FDSockRemote
 *  returns remote endpoint
 *
 *  "addr" [ OUT ] and "port" [ OUT ] - return parameters
 *  for remote endpoint
 *
 *  return values:
 */
int tracedb_FDSockRemote ( tracedb_fd_t self,
    tracedb_nsaddr_t *addr, int *port );

/* FDSockSetInputTimeout
 *  sets a timeout value for input operations
 *
 *  "mS" [ IN ] - a quantity indicating input operation
 *  timeout behavior:
 *    a negative value means infinite wait
 *    a value of zero means no blocking
 *    a positive value indicates a timeout in milliseconds
 *
 *  return values:
 */
int tracedb_FDSockSetInputTimeout ( tracedb_fd_t self, int mS );

/* FDSockSetOutputTimeout
 *  sets a timeout value for output operations
 *
 *  "mS" [ IN ] - a quantity indicating output operation
 *  timeout behavior:
 *    a negative value means infinite wait
 *    a value of zero means no blocking
 *    a positive value indicates a timeout in milliseconds
 *
 *  return values:
 */
int tracedb_FDSockSetOutputTimeout ( tracedb_fd_t self, int mS );

/* FDSockRead
 *  socket specific read routine
 *  ( by definition, "FDRead" must also work )
 *
 *  "buffer" [ OUT ] and "max_bytes" [ IN ] - return parameter
 *  for retrieved data
 *
 *  "num_read" [ OUT ] - return parameter for the number of
 *  bytes of data returned in "buffer"
 *
 *  "no_sigpipe" [ IN ] - when true, indicates that the
 *  implementation should attempt to douse SIGPIPE if supported.
 *  N.B. - not all Unices support this.
 *
 *  return values:
 */
int tracedb_FDSockRead ( tracedb_fd_t self,
    void *buffer, size_t max_bytes, size_t *num_read, bool no_sigpipe );

/* FDSockReadAny
 *  behaves as FDSockRead
 *  handles interrupts via callback
 *  ( by definition, "FDReadAny" must also work )
 *
 *  "buffer" [ OUT ] and "max_bytes" [ IN ] - return parameter
 *  for retrieved data
 *
 *  "num_read" [ OUT ] - return parameter for the number of
 *  bytes of data returned in "buffer"
 *
 *  "no_sigpipe" [ IN ] - when true, indicates that the
 *  implementation should attempt to douse SIGPIPE if supported.
 *  N.B. - not all Unices support this.
 *
 *  "handle_intr" [ IN ] and "data" [ IN ] - callback function
 *  and optional data parameter invoked from within loop upon EINTR.
 *
 *  return values:
 */
int tracedb_FDSockReadAny ( tracedb_fd_t self,
    void *buffer, size_t max_bytes, size_t *num_read,
    bool no_sigpipe, int ( * handle_intr ) ( void *data ), void *data );

/* FDSockWrite
 *  socket specific write routine
 *  ( by definition, "FDWrite" must also work )
 *
 *  "buffer" [ IN ] and "bytes" [ IN ] - buffer of data to
 *  be written
 *
 *  "num_writ" [ OUT ] - return parameter for the
 *  number of bytes actually written
 *
 *  "no_sigpipe" [ IN ] - when true, indicates that the
 *  implementation should attempt to douse SIGPIPE if supported.
 *  N.B. - not all Unices support this.
 *
 *  return values:
 */
int tracedb_FDSockWrite ( tracedb_fd_t self,
    const void *buffer, size_t bytes, size_t *num_writ, bool no_sigpipe );

/* FDSockWriteAll
 *  invokes FDSockWrite until all bytes are written
 *  or non-recoverable error condition is encountered.
 *  ( by definition, "FDWriteAll" must also work )
 *
 *  "buffer" [ IN ] and "bytes" [ IN ] - buffer of data to
 *  be written
 *
 *  "num_writ" [ OUT ] - return parameter for the
 *  number of bytes actually written
 *
 *  "no_sigpipe" [ IN ] - when true, indicates that the
 *  implementation should attempt to douse SIGPIPE if supported.
 *  N.B. - not all Unices support this.
 *
 *  "handle_intr" [ IN ] and "data" [ IN ] - callback function
 *  and optional data parameter invoked from within loop upon EINTR.
 *
 *  return values:
 */
int tracedb_FDSockWriteAll ( tracedb_fd_t self,
    const void *buffer, size_t bytes, size_t *num_writ,
    bool no_sigpipe, int ( * handle_intr ) ( void *data ), void *data );

/* FDSockWhack
 *  close a socket fd if open
 *  ignores unopen socket fds
 *  ( by definition, "FDWhack" must also work )
 *
 *  closing a socket, even though it is a synchronous call, can
 *  technically be an asynchronous operation, e.g. the TCP protocol
 *  carries on some level of conversation over time in response to
 *  a close.
 *
 *  "input" [ IN ] - if true, the input side of the socket will
 *  be closed to further input.
 *
 *  "output" [ IN ] - if true, the output side of the socket will
 *  be closed to further input.
 *
 *  return values:
 */
int tracedb_FDSockWhack ( tracedb_fd_t self, bool input, bool output );


/*--------------------------------------------------------------------------
 * FDDatagramSock
 *  an fd supporting datagram socket operations
 */

/* FDDatagramSockRead
 *  read from a connected or unconnected socket
 *
 *  reading from an unconnected socket is akin to an accept and read
 *  from a connected, stream-oriented socket.
 *
 *  "buffer" [ OUT ] and "max_bytes" [ IN ] - return parameter
 *  for retrieved data
 *
 *  "num_read" [ OUT ] - the number of bytes returned in "buffer"
 *
 *  "addr" [ OUT ] and "port" [ OUT ] - return parameter for
 *  the endpoint of the remote socket
 *
 *  "no_sigpipe" [ IN ] - when true, indicates that the
 *  implementation should attempt to douse SIGPIPE if supported.
 *  N.B. - not all Unices support this.
 *
 *  return values:
 */
int tracedb_FDDatagramSockRead ( tracedb_fd_t self,
    void *buffer, size_t max_bytes,
    size_t *num_read, tracedb_nsaddr_t *addr, int *port, bool no_sigpipe );

/* FDDatagramSockWrite
 *  write to a specific endpoint
 *
 *  "buffer" [ IN ] and "bytes" [ IN ] - data to be written
 *
 *  "num_writ" [ OUT ] - the number of bytes actually written
 *
 *  "addr" [ IN ] and "port" [ IN ] - endpoint of remote socket
 *  to receive datagram
 *
 *  "no_sigpipe" [ IN ] - when true, indicates that the
 *  implementation should attempt to douse SIGPIPE if supported.
 *  N.B. - not all Unices support this.
 *
 *  return values:
 */
int tracedb_FDDatagramSockWrite ( tracedb_fd_t self,
    const void *buffer, size_t bytes, size_t *num_writ,
    const tracedb_nsaddr_t *addr, int port, bool no_sigpipe );

/* FDDatagramSockWhack
 *  whack a datagram socket
 *  ( by definition, "FDSockWhack" and "FDWhack" must also work )
 *
 *  "input" [ IN ] - if true, the input side of the socket will
 *  be closed to further input.
 *
 *  "output" [ IN ] - if true, the output side of the socket will
 *  be closed to further input.
 *
 *  return values:
 */
#define tracedb_FDDatagramSockWhack( self, input, output ) \
    tracedb_FDSockWhack ( self, input, output )


/*--------------------------------------------------------------------------
 * FDStreamSock
 *  an fd supporting stream socket operations
 */

/* FDStreamSockListen
 *  instruct socket layer to begin queuing connection requests
 *
 *  "qlen" [ IN ] - indicates how many requests to queue before rejecting.
 *  N.B. some implementations have been known to use this
 *  parameter as a guideline rather than a hard parameter.
 *
 *  return values:
 */
int tracedb_FDStreamSockListen ( tracedb_fd_t self, unsigned int qlen );

/* FDStreamSockAccept
 *  return the next connection request from queue
 *
 *  "sock" [ OUT ] - return parameter for newly accepted connection
 *  socket upon success or -1 upon error.
 *
 *  "addr" [ OUT, NULL OKAY ] and "port" [ OUT, NULL OKAY ] - optional
 *  return parameters for remote socket endpoint
 *
 *  return values:
 */
int tracedb_FDStreamSockAccept ( tracedb_fd_t self, tracedb_fd_t *sock,
    tracedb_nsaddr_t *addr, int *port );

/* FDStreamSockSetDelay
 *  enable or disable the Nagel algorithm
 *
 *  "enable" [ IN ] -  Boolean switch value
 *
 *  return values:
 */
int tracedb_FDStreamSockSetDelay ( tracedb_fd_t self, bool enable );

/* FDStreamSockWhack
 *  whack a stream socket
 *  ( by definition, "FDSockWhack" and "FDWhack" must also work )
 *
 *  "input" [ IN ] - if true, the input side of the socket will
 *  be closed to further input.
 *
 *  "output" [ IN ] - if true, the output side of the socket will
 *  be closed to further input.
 *
 *  return values:
 */
#define tracedb_FDStreamSockWhack( self, input, output ) \
    tracedb_FDSockWhack ( self, input, output )


#ifdef __cplusplus
}
#endif

#endif /* _h_fdnet_ */
