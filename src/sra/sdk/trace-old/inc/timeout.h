#ifndef _h_timeout_
#define _h_timeout_

#ifndef _h_itypes_
#include "itypes.h"
#endif

#include <time.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * timeout_t
 *  for our purposes we need a timespec
 *  but due to the expense of calling into the kernel
 *  to prepare one, we make it lazily preparable...
 */
typedef struct tracedb_timeout_t tracedb_timeout_t;
struct tracedb_timeout_t
{
    struct timespec ts;
    unsigned int mS;
    unsigned int prepared;
};

typedef tracedb_timeout_t *tracedb_timeoutref_t;
#define TRACEDB_TIMEOUTREF( tm ) & ( tm )
#define TRACEDB_TIMEOUTDEREF( tm ) * ( tm )

/* TimeoutPrepare
 *  ensures that a timeout is prepared with an absolute value
 */
void tracedb_timeout_prepare ( tracedb_timeout_t *tm );
#define tracedb_TimeoutPrepare( tm ) \
    ( ( tm ) . prepared == 0 ? tracedb_timeout_prepare ( & ( tm ) ) : ( void ) 0 )

/* TimeoutInit
 *  initialize a timeout value from wait value
 */
#define tracedb_TimeoutInit( tm, millis, prepare ) \
    ( ( tm ) -> mS = ( millis ), \
      ( prepare ) ? tracedb_timeout_prepare ( tm ) : ( void ) ( ( tm ) -> prepared = 0 ) )

/* TimeoutWait
 *  periodically executes a poll function
 *  until condition is satisfied or timeout
 *  returns true if condition was satisfied
 */
bool tracedb_timeout_wait ( tracedb_timeoutref_t tm, unsigned int mS,
    bool ( * poll ) ( void *data ), void *data );
#define tracedb_TimeoutWait( tm, mS, poll, data ) \
    tracedb_timeout_wait ( TRACEDB_TIMEOUTREF ( tm ), mS, poll, data )

/* TimeoutIncrement
 *  increments a timeout value by a number of mS
 */
void tracedb_timeout_increment ( tracedb_timeout_t *tm, unsigned int mS );
#define tracedb_TimeoutIncrement( tm, mS ) \
    tracedb_timeout_increment ( tm, mS )

/* TimeoutCompare
 *  returns a tri-state value for time comparisons
 */
#define tracedb_TimeoutCompare( ta, tb ) \
    ( ( ta ) . ts . tv_sec == ( tb ) . ts . tv_sec ? \
      ( ( ta ) . ts . tv_nsec < ( tb ) . ts . tv_nsec ? -1 : \
        ( ta ) . ts . tv_nsec > ( tb ) . ts . tv_nsec ) : \
      ( ta ) . ts . tv_sec - ( tb ) . ts . tv_sec )

/* TimeoutWhack
 *  does nothing here
 */
#define tracedb_TimeoutWhack( tm ) \
    ( ( void ) 0 )


/*--------------------------------------------------------------------------
 * NanoSeconds
 *  a 64 bit unsigned integer
 *
 *  there is a nice little function called nanosleep, which is
 *  an attempt at introducing the proper API, even if system
 *  support is missing.
 *
 *  many systems have a minimum sleep time of around 10mS, so
 *  specifying a 5nS sleep can really throw the timing of the
 *  application off.
 *
 *  create a NanoSeconds object and then call it to sleep with
 *  real nanosecond specifications. the system specific code will
 *  not actually sleep until the accumulated time is in line with
 *  the system's minimum.
 */
typedef uint64_t tracedb_NanoSeconds;
typedef tracedb_NanoSeconds *tracedb_NanoSecondsRef;
#define TRACEDB_NANOSECONDSREF( ns ) ( ns )
#define TRACEDB_NANOSECONDSDEREF( ns ) ( ns )

/* NanoSecondsInit
 *  just zeros out the spec
 */
#define tracedb_NanoSecondsInit( ns ) \
    * ( ns ) = 0

/* NanoSecondsSleep
 *  supports illusion of fine grained sleeping
 */
void tracedb_NanoSecondsSleep ( tracedb_NanoSeconds *cumulative, uint64_t nS );

/* NanoSecondsWhack
 */
#define tracedb_NanoSecondsWhack( ns ) \
    ( ( void ) 0 )


#ifdef __cplusplus
}
#endif

#endif /* _h_timeout_ */
