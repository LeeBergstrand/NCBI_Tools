#ifndef _h_semaphore_
#define _h_semaphore_

#ifndef _h_cond_
#include "cond.h"
#endif

#ifndef _h_trace_atomic32_
#include "trace_atomic32.h"
#endif

#ifndef _h_timeout_
#include "timeout.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * Semaphore
 *  a counting semaphore
 *  useful for managing queues, files, etc.
 */
typedef struct tracedb_Semaphore tracedb_Semaphore;
struct tracedb_Semaphore
{
    tracedb_Condition cond;
    tracedb_atomic32_t count;
};

typedef tracedb_Semaphore *tracedb_SemaphoreRef;
#define TRACEDB_SEMAPHOREREF( sem ) \
    & ( sem )
#define TRACEDB_SEMAPHOREDEREF( sem ) \
    * ( sem )

/* SemaphoreInit
 *  initialize a semaphore
 */
#define tracedb_SemaphoreInit( sem, cnt ) \
    ( tracedb_atomic32_set ( & ( sem ) -> count, ( long ) ( cnt ) ), \
      tracedb_ConditionInit ( & ( sem ) -> cond ) )

/* SemaphoreSignal
 *  increments count and wakes a single blocked thread
 */
int tracedb_semaphore_signal ( tracedb_SemaphoreRef sem );
#define tracedb_SemaphoreSignal( sem ) \
    ( tracedb_atomic32_add_if_ge ( & ( sem ) . count, 1, 0 ) ? \
      0 : tracedb_semaphore_signal ( TRACEDB_SEMAPHOREREF ( sem ) ) )

/* SemaphoreWait
 *  decrements the count and waits for >= 0
 */
int tracedb_semaphore_wait ( tracedb_SemaphoreRef sem );
#define tracedb_SemaphoreWait( sem ) \
    ( tracedb_atomic32_add_if_gt ( & ( sem ) . count, -1, 0 ) ? \
      0 : tracedb_semaphore_wait ( TRACEDB_SEMAPHOREREF ( sem ) ) )

/* SemaphoreTry
 *  attempts to decrement the count
 *  returns 0 on success, and EBUSY otherwise
 */
#define tracedb_SemaphoreTry( sem ) \
    ( tracedb_atomic32_add_if_gt ( & ( sem ) . count, -1, 0 ) ? \
      0 : EBUSY )

/* SemaphoreTimedWait
 *  attempt to decrement the count to produce >= 0
 *  and waits for up to provided timeout to do so
 *  "tm" is a timeout_t prepared by TimeoutInit
 */
int tracedb_semaphore_timedwait ( tracedb_SemaphoreRef sem,
    tracedb_timeoutref_t tm );
#define tracedb_SemaphoreTimedWait( sem, tm ) \
    ( tracedb_atomic32_add_if_gt ( & ( sem ) . count, -1, 0 ) ? \
      0 : tracedb_semaphore_timedwait ( TRACEDB_SEMAPHOREREF ( sem ), TRACEDB_TIMEOUTREF ( tm ) ) )

/* SemaphoreWhack
 *  tear down a semaphore
 */
#define tracedb_SemaphoreWhack( sem ) \
    tracedb_ConditionWhack ( ( sem ) . cond )


/*--------------------------------------------------------------------------
 * RSemaphore
 *  a resource metering semaphore
 *  good for controlling access to memory, etc.
 *
 *  "waiting" is really a bool, but declared as an int
 *  for better struct alignment
 */
typedef struct tracedb_RSemaphore tracedb_RSemaphore;
struct tracedb_RSemaphore
{
    uint64_t count;
    tracedb_Condition cond;
    int32_t waiting;
    int32_t align1;
};

typedef tracedb_RSemaphore *tracedb_RSemaphoreRef;
#define TRACEDB_RSEMAPHOREREF( sem ) \
    & ( sem )
#define TRACEDB_RSEMAPHOREDEREF( sem ) \
    * ( sem )

/* RSemaphoreInit
 *  initialize a resource semaphore
 */
#define tracedb_RSemaphoreInit( sem, cnt ) \
    ( ( sem ) -> count = ( cnt ), \
      ( sem ) -> waiting = false, \
      tracedb_ConditionInit ( & ( sem ) -> cond ) )

/* RSemaphoreSignalAvail
 *  makes provided count available
 *  wakes all blocked threads
 */
int tracedb_rsemaphore_signal_avail ( tracedb_RSemaphoreRef sem, uint64_t avail );
#define tracedb_RSemaphoreSignalAvail( sem, avail ) \
    tracedb_rsemaphore_signal_avail ( TRACEDB_RSEMAPHOREREF( sem ), avail )

/* RSemaphoreWaitAvail
 *  waits until the specified count becomes available
 *
 *  "held" indicates resources already held by calling thread
 *  such that a potential deadlock can be avoided
 */
int tracedb_rsemaphore_wait_avail ( tracedb_RSemaphoreRef sem, uint64_t avail, uint64_t held );
#define tracedb_RSemaphoreWaitAvail( sem, avail, held ) \
    tracedb_rsemaphore_wait_avail ( TRACEDB_RSEMAPHOREREF( sem ), avail, held )

/* RSemaphoreTimedWaitAvail
 *  wait up to provided timeout for the specified
 *  count to become available.
 *  "tm" is a timeout_t prepared by TimeoutInit
 */
int tracedb_rsemaphore_timedwait_avail ( tracedb_RSemaphoreRef sem,
    uint64_t avail, uint64_t held, tracedb_timeoutref_t tm );
#define tracedb_RSemaphoreTimedWaitAvail( sem, avail, held, tm ) \
    tracedb_rsemaphore_timedwait_avail ( TRACEDB_RSEMAPHOREREF( sem ), \
        avail, held, TRACEDB_TIMEOUTREF ( tm ) )

/* RSemaphoreWhack
 *  tear down a resource semaphore
 */
#define tracedb_RSemaphoreWhack( sem ) \
    tracedb_ConditionWhack ( ( sem ) . cond )


#ifdef __cplusplus
}
#endif

#endif /* _h_semaphore_ */
