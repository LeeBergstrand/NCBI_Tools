#include "lock.h"
#include "timeout.h"

#include <errno.h>
#include <assert.h>


/*--------------------------------------------------------------------------
 * TmMutex
 *  a mutex lock with timeout
 */
#if ! TRACEDB_HAVE_TMLOCK
#error "Linux is expected to support timed pthread locking"
#endif


/*--------------------------------------------------------------------------
 * RWTmLock
 */
#if ! TRACEDB_HAVE_RWLOCK

int tracedb_tm_rwlock_init ( tracedb_RWTmLock *rwlock )
{
    int status;

    if ( rwlock == NULL )
        return EINVAL;

    status = pthread_mutex_init ( & rwlock -> mutex, NULL );
    if ( status == 0 )
    {
        status = pthread_cond_init ( & rwlock -> rcond, NULL );
        if ( status == 0 )
        {
            status = pthread_cond_init ( & rwlock -> wcond, NULL );
            if ( status == 0 )
            {
                rwlock -> refcount = rwlock -> rwait = rwlock -> wwait = 0;
                return 0;
            }

            pthread_cond_destroy ( & rwlock -> rcond );
        }

        pthread_mutex_destroy ( & rwlock -> mutex );
    }

    return status;
}

int tracedb_tm_rwlock_rdlock ( tracedb_RWTmLockRef rwlock )
{
    int status;

    if ( rwlock == NULL )
        return EINVAL;
    
    status = pthread_mutex_lock ( & rwlock -> mutex );
    if ( status == 0 )
    {
        while ( rwlock -> refcount < 0 || rwlock -> wwait > 0 )
        {
            ++ rwlock -> rwait;
            status = pthread_cond_wait ( & rwlock -> rcond, & rwlock -> mutex );
            -- rwlock -> rwait;
            if ( status != 0 )
                break;
        }
	
        if ( status == 0 )
            ++ rwlock -> refcount;

        pthread_mutex_unlock ( & rwlock -> mutex );
    }
    return status;
}

int tracedb_tm_rwlock_wrlock ( tracedb_RWTmLockRef rwlock )
{
    int status;

    if ( rwlock == NULL )
        return EINVAL;

    status = pthread_mutex_lock ( & rwlock -> mutex );
    if ( status == 0 )
    {
        /* wait while there are readers or writers */
        while ( rwlock -> refcount )
        {
            ++ rwlock -> wwait;
            status = pthread_cond_wait ( & rwlock -> wcond, & rwlock -> mutex );
            -- rwlock -> wwait;
            if ( status != 0 )
                break;
        }
	
        /* indicate single writer */
        if ( status == 0 )
            rwlock -> refcount = -1;
	
        pthread_mutex_unlock ( & rwlock -> mutex );
    }
    
    return status;    
}

int tracedb_tm_rwlock_tryrdlock ( tracedb_RWTmLockRef rwlock )
{
    int status;

    if ( rwlock == NULL )
        return EINVAL;
    
    status = pthread_mutex_trylock ( & rwlock -> mutex );
    if ( status == 0 )
    {
        if ( rwlock -> refcount < 0 || rwlock -> wwait > 0 )
            status = EBUSY;
        else
            ++ rwlock -> refcount;

        pthread_mutex_unlock ( & rwlock -> mutex );
    }
    return status;
}

int tracedb_tm_rwlock_trywrlock ( tracedb_RWTmLockRef rwlock )
{
    int status;

    if ( rwlock == NULL )
        return EINVAL;

    status = pthread_mutex_trylock ( & rwlock -> mutex );
    if ( status == 0 )
    {
        /* fail if there are readers or writers */
        if ( rwlock -> refcount )
            status = EBUSY;
	
        /* indicate single writer */
        else
            rwlock -> refcount = -1;
	
        pthread_mutex_unlock ( & rwlock -> mutex );
    }
    
    return status;    
}

int tracedb_tm_rwlock_timedrdlock ( tracedb_RWTmLockRef rwlock, timeoutref_t tm )
{
    int status;

    if ( rwlock == NULL || tm == NULL )
        return EINVAL;

    TimeoutPrepare ( TIMEOUTDEREF ( tm ) );
    
    status = pthread_mutex_timedlock ( & rwlock -> mutex, & tm -> ts );
    if ( status == 0 )
    {
        while ( rwlock -> refcount < 0 || rwlock -> wwait > 0 )
        {
            ++ rwlock -> rwait;
            status = pthread_cond_timedwait ( & rwlock -> rcond,
                & rwlock -> mutex, & tm -> ts );
            -- rwlock -> rwait;
            if ( status != 0 )
            {
                if ( status == ETIMEDOUT )
                    status = EBUSY;
                break;
            }
        }
	
        if ( status == 0 )
            ++ rwlock -> refcount;
	
        pthread_mutex_unlock ( & rwlock -> mutex );
    }
    return status;
}

int tracedb_tm_rwlock_timedwrlock ( tracedb_RWTmLockRef rwlock, timeoutref_t tm )
{
    int status;

    if ( rwlock == NULL || tm == NULL )
        return EINVAL;

    TimeoutPrepare ( TIMEOUTDEREF ( tm ) );
    
    status = pthread_mutex_timedlock ( & rwlock -> mutex, & tm -> ts );
    if ( status == 0 )
    {
        while ( rwlock -> refcount )
        {
            ++ rwlock -> wwait;
            status = pthread_cond_timedwait ( & rwlock -> wcond,
                & rwlock -> mutex, & tm -> ts );
            -- rwlock -> wwait;
            if ( status != 0 )
            {
                if ( status == ETIMEDOUT )
                    status = EBUSY;
                break;
            }
        }
	
        if ( status == 0 )
            rwlock -> refcount = -1;
	
        pthread_mutex_unlock ( & rwlock -> mutex );
    }

    return status;
}

int tracedb_tm_rwlock_unlock ( tracedb_RWTmLockRef rwlock )
{
    int status;

    if ( rwlock == NULL )
        return EINVAL;

    status = pthread_mutex_lock ( & rwlock -> mutex );
    if ( status == 0 )
    {
        if ( rwlock -> refcount > 0 )
            -- rwlock -> refcount;
        else if ( rwlock -> refcount < 0 )
            rwlock -> refcount = 0;
        else
        {
            pthread_mutex_unlock ( & rwlock -> mutex );
            return EINVAL;
        }

        if ( rwlock -> wwait )
        {
            if ( ! rwlock -> refcount )
                status = pthread_cond_signal ( & rwlock -> wcond );
        }
        else if ( rwlock -> rwait )
        {
            status = pthread_cond_broadcast ( & rwlock -> rcond );
        }

        pthread_mutex_unlock ( & rwlock -> mutex );
    }
    return status;
}

int tracedb_tm_rwlock_destroy ( tracedb_RWTmLockRef rwlock )
{
    int status;

    if ( rwlock == NULL )
        return EINVAL;

    if ( rwlock -> refcount || rwlock -> rwait || rwlock -> wwait )
        return EBUSY;

    status = pthread_mutex_destroy ( & rwlock -> mutex );
    if ( status == 0 )
    {
        pthread_cond_destroy ( & rwlock -> rcond );
        pthread_cond_destroy ( & rwlock -> wcond );
    }

    return status;
}

#endif
