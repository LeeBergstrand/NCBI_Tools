#ifndef _h_lock_
#define _h_lock_

#ifndef _h_posthread_
#include "posthread.h"
#endif

#ifndef _h_lockcmn_
#include "lockcmn.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * Mutex
 *  a straight mutex lock without timeout
 */
typedef pthread_mutex_t tracedb_Mutex;
typedef tracedb_Mutex *tracedb_MutexRef;
#define TRACEDB_MUTEXREF( mutex ) & ( mutex )
#define TRACEDB_MUTEXDEREF( mutex ) * ( mutex )

#define tracedb_mutex_init( mutex ) \
    pthread_mutex_init ( mutex, NULL )
#define tracedb_mutex_lock( mutex ) \
    pthread_mutex_lock ( mutex )
#define tracedb_mutex_trylock( mutex ) \
    pthread_mutex_trylock ( mutex )
#define tracedb_mutex_unlock( mutex ) \
    pthread_mutex_unlock ( mutex )
#define tracedb_mutex_destroy( mutex ) \
    pthread_mutex_destroy ( mutex )


/*--------------------------------------------------------------------------
 * TmMutex
 *  a mutex lock with timeout
 */
#if TRACEDB_HAVE_TMLOCK

typedef pthread_mutex_t tracedb_TmMutex;
typedef tracedb_TmMutex *tracedb_TmMutexRef;
#define TRACEDB_TMMUTEXREF( mutex ) & ( mutex )
#define TRACEDB_TMMUTEXDEREF( mutex ) * ( mutex )

#define tracedb_tm_mutex_init( mutex ) \
    pthread_mutex_init ( mutex, NULL )
#define tracedb_tm_mutex_lock( mutex ) \
    pthread_mutex_lock ( mutex )
#define tracedb_tm_mutex_trylock( mutex ) \
    pthread_mutex_trylock ( mutex )
#define tracedb_tm_mutex_timedlock( mutex, tm ) \
    ( tracedb_TimeoutPrepare ( TRACEDB_TIMEOUTDEREF ( tm ) ), \
      pthread_mutex_timedlock ( mutex, & ( tm ) -> ts ) )
#define tracedb_tm_mutex_unlock( mutex ) \
    pthread_mutex_unlock ( mutex )
#define tracedb_tm_mutex_destroy( mutex ) \
    pthread_mutex_destroy ( mutex )

#endif


/*--------------------------------------------------------------------------
 * RWLock
 *  use pthread type if defined
 */
#if TRACEDB_HAVE_RWLOCK

typedef pthread_rwlock_t tracedb_RWLock;
typedef tracedb_RWLock *tracedb_RWLockRef;
#define TRACEDB_RWLOCKREF( rwlock ) & ( rwlock )
#define TRACEDB_RWLOCKDEREF( rwlock ) * ( rwlock )

#define tracedb_rwlock_init( rwlock ) \
     pthread_rwlock_init ( rwlock, NULL )
#define tracedb_rwlock_rdlock( rwlock ) \
    pthread_rwlock_rdlock ( rwlock )
#define tracedb_rwlock_wrlock( rwlock ) \
    pthread_rwlock_wrlock ( rwlock )
#define tracedb_rwlock_tryrdlock( rwlock ) \
    pthread_rwlock_tryrdlock ( rwlock )
#define tracedb_rwlock_trywrlock( rwlock ) \
    pthread_rwlock_trywrlock ( rwlock )
#define tracedb_rwlock_unlock( rwlock ) \
    pthread_rwlock_unlock ( rwlock )
#define tracedb_rwlock_destroy( rwlock ) \
    pthread_rwlock_destroy ( rwlock )

#endif


/*--------------------------------------------------------------------------
 * RWTmLock
 *  use pthread type only if defined and timed
 */
#if TRACEDB_HAVE_RWLOCK && TRACEDB_HAVE_TMLOCK

typedef pthread_rwlock_t tracedb_RWTmLock;
typedef tracedb_RWTmLock *tracedb_RWTmLockRef;
#define TRACEDB_RWTMLOCKREF( rwlock ) & ( rwlock )
#define TRACEDB_RWTMLOCKDEREF( rwlock ) * ( rwlock )

#define tracedb_tm_rwlock_init( rwlock ) \
     pthread_rwlock_init ( rwlock, NULL )
#define tracedb_tm_rwlock_rdlock( rwlock ) \
    pthread_rwlock_rdlock ( rwlock )
#define tracedb_tm_rwlock_wrlock( rwlock ) \
    pthread_rwlock_wrlock ( rwlock )
#define tracedb_tm_rwlock_tryrdlock( rwlock ) \
    pthread_rwlock_tryrdlock ( rwlock )
#define tracedb_tm_rwlock_trywrlock( rwlock ) \
    pthread_rwlock_trywrlock ( rwlock )
#define tracedb_tm_rwlock_timedrdlock( rwlock, tm ) \
    ( tracedb_TimeoutPrepare ( TRACEDB_TIMEOUTDEREF ( tm ) ), \
      pthread_rwlock_timedrdlock ( rwlock, & ( tm ) -> ts ) )
#define tracedb_tm_rwlock_timedwrlock( rwlock, tm ) \
    ( tracedb_TimeoutPrepare ( TRACEDB_TIMEOUTDEREF ( tm ) ), \
      pthread_rwlock_timedwrlock ( rwlock, & ( tm ) -> ts ) )
#define tracedb_tm_rwlock_unlock( rwlock ) \
    pthread_rwlock_unlock ( rwlock )
#define tracedb_tm_rwlock_destroy( rwlock ) \
    pthread_rwlock_destroy ( rwlock )

#else

struct tracedb_RWTmLock
{
    pthread_mutex_t mutex;
    pthread_cond_t rcond;
    pthread_cond_t wcond;
    int refcount;
    int rwait;
    int wwait;
};

#endif


#ifdef __cplusplus
}
#endif

#endif /* _h_lock_ */
