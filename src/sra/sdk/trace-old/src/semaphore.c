#include "semaphore.h"

#include <errno.h>
#include <assert.h>

/*--------------------------------------------------------------------------
 * Semaphore
 *  counting semaphore
 */


/* SemaphoreSignal
 *  increments count and wakes a single blocked thread
 */
int tracedb_semaphore_signal ( tracedb_SemaphoreRef sem )
{
    int status;

    if ( sem == NULL )
        return EINVAL;

    status = tracedb_ConditionLock ( sem -> cond );
    if ( status == 0 )
    {
        /* favor the Intel version of atomic32_add...,
           since Apple is going that way, too */
        if ( tracedb_atomic32_add_and_read ( & sem -> count, 1 ) <= 0 )
            status = tracedb_ConditionSignal ( sem -> cond );

        tracedb_ConditionUnlock ( sem -> cond );
    }
    return status;
}

/* SemaphoreWait
 *  decrements the count and waits for >= 0
 */
int tracedb_semaphore_wait ( tracedb_SemaphoreRef sem )
{
    int status;

    if ( sem == NULL )
        return EINVAL;

    status = tracedb_ConditionLock ( sem -> cond );
    if ( status == 0 )
    {
        if ( tracedb_atomic32_add_and_read ( & sem -> count, -1 ) < 0 )
            status = tracedb_ConditionWait ( sem -> cond );
        tracedb_ConditionUnlock ( sem -> cond );
    }
    return status;
}

/* SemaphoreTimedWait
 *  attempt to decrement the count to produce >= 0
 *  and waits for up to provided timeout to do so
 *  "tm" is a timeout_t prepared by TimeoutInit
 */
int tracedb_semaphore_timedwait ( tracedb_SemaphoreRef sem,
    tracedb_timeoutref_t tm )
{
    int status;

    if ( sem == NULL )
        return EINVAL;

    status = tracedb_ConditionLock ( sem -> cond );
    if ( status == 0 )
    {
        if ( tracedb_atomic32_add_and_read ( & sem -> count, -1 ) < 0 )
        {
            status = tracedb_ConditionTimedWait ( sem -> cond,
                TRACEDB_TIMEOUTDEREF ( tm ) );
            if ( status != 0 )
                tracedb_atomic32_inc ( & sem -> count );
        }
        tracedb_ConditionUnlock ( sem -> cond );
    }
    return status;
}


/*--------------------------------------------------------------------------
 * RSemaphore
 *  resource metering semaphore
 */

/* RSemaphoreSignalAvail
 *  makes provided count available
 */
int tracedb_rsemaphore_signal_avail ( tracedb_RSemaphore *sem,
    uint64_t avail )
{
    int status;

    if ( sem == NULL )
        return EINVAL;

    status = tracedb_ConditionLock ( sem -> cond );
    if ( status == 0 )
    {
        sem -> count += avail;

        if ( sem -> waiting )
        {
            sem -> waiting = false;
            status = tracedb_ConditionBroadcast ( sem -> cond );
        }

        tracedb_ConditionUnlock ( sem -> cond );
    }
    return status;
}

/* RSemaphoreWaitAvail
 *  waits until the specified count becomes available
 */
int tracedb_rsemaphore_wait_avail ( tracedb_RSemaphore *sem,
    uint64_t avail, uint64_t held )
{
    int status;

    if ( sem == NULL )
        return EINVAL;

    status = tracedb_ConditionLock ( sem -> cond );
    if ( status == 0 )
    {
        while ( sem -> count < avail )
        {
            sem -> waiting = true;
            status = tracedb_ConditionWait ( sem -> cond );
        }

        sem -> count -= avail;

        tracedb_ConditionUnlock ( sem -> cond );
    }
    return status;
}

/* RSemaphoreTimedWaitAvail
 *  wait up to provided timeout for the specified
 *  count to become available.
 *  "tm" is a timeout_t prepared by TimeoutInit
 */
int tracedb_rsemaphore_timedwait_avail ( tracedb_RSemaphore *sem,
    uint64_t avail, uint64_t held, tracedb_timeoutref_t tm )
{
    int status;

    if ( sem == NULL )
        return EINVAL;

    status = tracedb_ConditionLock ( sem -> cond );
    if ( status == 0 )
    {
        while ( sem -> count < avail )
        {
            sem -> waiting = true;
            status = tracedb_ConditionTimedWait ( sem -> cond,
                                    TRACEDB_TIMEOUTDEREF ( tm ) );
            if ( status != 0 )
                break;
        }

        if ( status == 0 )
            sem -> count -= avail;

        tracedb_ConditionUnlock ( sem -> cond );
    }
    return status;
}
