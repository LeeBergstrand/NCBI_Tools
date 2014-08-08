#ifndef _h_cond_
#define _h_cond_

#ifndef _h_posthread_
#include "posthread.h"
#endif

#ifndef _h_timeout_
#include "timeout.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * Condition
 *  an open condition object that uses the POSIX model
 *
 *  under this model, the Condition object must be locked
 *  before any other access occurs.
 *
 *  externally stored state is then accessed to either check
 *  for a condition or modify the condition.
 *
 *  for threads waiting on a condition that has not yet been
 *  satisfied, the wait ( either timed or infinite ) occurs
 *  within the lock. under the POSIX model, that lock is released
 *  while the waiting thread sleeps, and re-acquired upon awaking.
 *
 *  it is not required that a waiter thread actually sleep on the
 *  condition if external state indicates the condition is already
 *  satisfied.
 *
 *  a signal thread must also acquire the lock before signalling
 *  the condition. it signals the condition after updating external
 *  state, protected by the lock. to awaken a single waiting thread,
 *  use "ConditionSignal" or to awaken all threads, "ConditionBroadcast"
 *
 *  it is of course imperitive to unlock the Condition when finished.
 */
typedef struct tracedb_Condition tracedb_Condition;
struct tracedb_Condition
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};

typedef struct tracedb_Condition *tracedb_ConditionRef;
#define TRACEDB_CONDITIONREF( cnd ) \
    & ( cnd )
#define TRACEDB_CONDITIONDEREF( cnd ) \
    * ( cnd )

/* ConditionInit
 *  "cnd" is an object pointer
 */
int tracedb_ConditionInit ( tracedb_Condition *cnd );

/* ConditionLock
 *  acquire a lock around condition access
 *  "cnd" is an object reference
 */
#define tracedb_ConditionLock( cnd ) \
    pthread_mutex_lock ( & ( cnd ) . mutex )

/* ConditionSignal
 *  wake up the next waiting thread
 *  "cnd" is an object reference
 */
#define tracedb_ConditionSignal( cnd ) \
    pthread_cond_signal ( & ( cnd ) . cond )

/* ConditionBroadcast
 *  wake up all waiting threads
 *  "cnd" is an object reference
 */
#define tracedb_ConditionBroadcast( cnd ) \
    pthread_cond_broadcast ( & ( cnd ) . cond )

/* ConditionUnlock
 *  release the lock around condition
 *  "cnd" is an object reference
 */
#define tracedb_ConditionUnlock( cnd ) \
    pthread_mutex_unlock ( & ( cnd ) . mutex )

/* ConditionWait
 *  wait forever on condition
 *  "cnd" is an object reference
 */
#define tracedb_ConditionWait( cnd ) \
    pthread_cond_wait ( & ( cnd ) . cond, & ( cnd ) . mutex )

/* ConditionTimedWait
 *  wait for condition until satisfied or
 *  timeout value has passed.
 *
 *  "cnd" is an object reference
 *  "tm" is a "timeout_t" prepared by TimeoutInit.
 */
#define tracedb_ConditionTimedWait( cnd, tm ) \
    ( tracedb_TimeoutPrepare ( tm ), \
      pthread_cond_timedwait ( & ( cnd ) . cond, & ( cnd ) . mutex, & ( tm ) . ts ) )

/* ConditionWhack
 *  whack the condition object
 *  "cnd" is an object reference
 */
#define tracedb_ConditionWhack( cnd ) \
    ( pthread_mutex_destroy ( & ( cnd ) . mutex ), \
      pthread_cond_destroy ( & ( cnd ) . cond ) )

#ifdef __cplusplus
}
#endif

#endif /* _h_cond_ */
