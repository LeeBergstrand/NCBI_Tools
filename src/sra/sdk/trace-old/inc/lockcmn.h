#ifndef _h_lockcmn_
#define _h_lockcmn_

#ifndef _h_lock_
#include "lock.h"
#endif

#ifndef _h_timeout_
#include "timeout.h"
#endif

#if ! TRACEDB_HAVE_TMLOCK || ! TRACEDB_HAVE_RWLOCK

#ifndef _h_cond_
#include "cond.h"
#endif

#ifndef _h_trace_atomic32_
#include "trace_atomic32.h"
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif



/*--------------------------------------------------------------------------
 * Mutex
 *  a straight mutex lock without timeout
 */

/* MutexInit
 *  initialize a mutex with default attributes
 */
#define tracedb_MutexInit( mutex ) \
    tracedb_mutex_init ( mutex )

/* MutexLock
 *  acquire a lock
 *  may fail, so test return result
 */
#define tracedb_MutexLock( mutex ) \
    tracedb_mutex_lock ( TRACEDB_MUTEXREF ( mutex ) )

/* MutexTryLock
 *  try to acquire a lock
 *  returns true if obtained
 */
#define tracedb_MutexTryLock( mutex ) \
    tracedb_mutex_trylock ( TRACEDB_MUTEXREF ( mutex ) )

/* MutexUnlock
 *  release the lock
 */
#define tracedb_MutexUnlock( mutex ) \
    tracedb_mutex_unlock ( TRACEDB_MUTEXREF ( mutex ) )

/* MutexWhack
 *  tear down a mutex
 */
#define tracedb_MutexWhack( mutex ) \
    tracedb_mutex_destroy ( TRACEDB_MUTEXREF ( mutex ) )


/*--------------------------------------------------------------------------
 * TmMutex
 *  a mutex lock with timeout
 */
#if ! TRACEDB_HAVE_TMLOCK

typedef struct tracedb_TmMutex tracedb_TmMutex;
struct tracedb_TmMutex
{
    tracedb_Condition cond;
    atomic32_t lock;
    int waiters;
};

typedef tracedb_TmMutex *tracedb_TmMutexRef;
#define TRACEDB_TMMUTEXREF( mutex ) & ( mutex )
#define TRACEDB_TMMUTEXDEREF( mutex ) * ( mutex )

int tracedb_tm_mutex_init ( tracedb_TmMutex *mutex );
int tracedb_tm_mutex_lock ( tracedb_TmMutexRef mutex );
int tracedb_tm_mutex_trylock ( tracedb_TmMutexRef mutex );
int tracedb_tm_mutex_timedlock ( tracedb_TmMutexRef mutex, timeoutref_t tm );
int tracedb_tm_mutex_unlock ( tracedb_TmMutexRef mutex );
int tracedb_tm_mutex_destroy ( tracedb_TmMutexRef mutex );

#endif

/* TmMutexInit
 *  initialize a mutex with default attributes
 */
#define tracedb_TmMutexInit( mutex ) \
    tracedb_tm_mutex_init ( mutex )

/* TmMutexLock
 *  acquire a lock
 *  may fail, so test return result
 */
#define tracedb_TmMutexLock( mutex ) \
    tracedb_tm_mutex_lock ( TRACEDB_MUTEXREF ( mutex ) )

/* TmMutexTryLock
 *  try to acquire a lock
 *  returns true if obtained
 */
#define tracedb_TmMutexTryLock( mutex ) \
    tracedb_tm_mutex_trylock ( TRACEDB_MUTEXREF ( mutex ) )

/* TmMutexTimedLock
 *  try to acquire a lock within a given timeout
 *  returns EBUSY if could not be acquired within given time
 */
#define tracedb_TmMutexTimedLock( mutex, tm ) \
    tracedb_tm_mutex_timedlock ( TRACEDB_MUTEXREF ( mutex ), TRACEDB_TIMEOUTREF ( tm ) )

/* TmMutexUnlock
 *  release the lock
 */
#define tracedb_TmMutexUnlock( mutex ) \
    tracedb_tm_mutex_unlock ( TRACEDB_MUTEXREF ( mutex ) )

/* TmMutexWhack
 *  tear down a mutex
 */
#define tracedb_TmMutexWhack( mutex ) \
    tracedb_tm_mutex_destroy ( TRACEDB_MUTEXREF ( mutex ) )



/*--------------------------------------------------------------------------
 * RWLock
 *  always externally defined
 */
#if ! TRACEDB_HAVE_RWLOCK

typedef struct tracedb_RWTmLock tracedb_RWLock;
typedef tracedb_RWLock *tracedb_RWLockRef;
#define TRACEDB_RWLOCKREF( rwlock ) & ( rwlock )
#define TRACEDB_RWLOCKDEREF( rwlock ) * ( rwlock )

#define tracedb_rwlock_init( rwlock ) tracedb_tm_rwlock_init ( rwlock )
#define tracedb_rwlock_rdlock( rwlock ) tracedb_tm_rwlock_rdlock ( rwlock )
#define tracedb_rwlock_wrlock( rwlock ) tracedb_tm_rwlock_wrlock ( rwlock )
#define tracedb_rwlock_tryrdlock( rwlock ) tracedb_tm_rwlock_tryrdlock ( rwlock )
#define tracedb_rwlock_trywrlock( rwlock ) tracedb_tm_rwlock_trywrlock ( rwlock )
#define tracedb_rwlock_unlock( rwlock ) tracedb_tm_rwlock_unlock ( rwlock )
#define tracedb_rwlock_destroy( rwlock ) tracedb_tm_rwlock_destroy ( rwlock )

#endif

/* RWLockInit
 *  initialize a rwlock with default attributes
 */
#define tracedb_RWLockInit( rwlock ) \
    tracedb_rwlock_init ( rwlock )

/* RWLockReadLock
 *  acquire a read lock
 *  may fail, so test return result
 */
#define tracedb_RWLockReadLock( rwlock ) \
    tracedb_rwlock_rdlock ( TRACEDB_RWLOCKREF ( rwlock ) )

/* RWLockWriteLock
 *  acquire a write lock
 *  may fail, so test return result
 */
#define tracedb_RWLockWriteLock( rwlock ) \
    tracedb_rwlock_wrlock ( TRACEDB_RWLOCKREF ( rwlock ) )

/* RWLockTryReadLock
 *  try to acquire a read lock
 *  returns true if obtained
 */
#define tracedb_RWLockTryReadLock( rwlock ) \
    tracedb_rwlock_tryrdlock ( TRACEDB_RWLOCKREF ( rwlock ) )

/* RWLockTryWriteLock
 *  try to acquire a write lock
 *  returns true if obtained
 */
#define tracedb_RWLockTryWriteLock( rwlock ) \
    tracedb_rwlock_trywrlock ( TRACEDB_RWLOCKREF ( rwlock ) )

/* RWLockUnlock
 *  release the lock
 */
#define tracedb_RWLockUnlock( rwlock ) \
    tracedb_rwlock_unlock ( TRACEDB_RWLOCKREF ( rwlock ) )

/* RWLockWhack
 *  tear down a rwlock
 */
#define tracedb_RWLockWhack( rwlock ) \
    tracedb_rwlock_destroy ( TRACEDB_RWLOCKREF ( rwlock ) );


/*--------------------------------------------------------------------------
 * RWTmLock
 *  always externally defined
 */
#if ! TRACEDB_HAVE_RWLOCK || ! TRACEDB_HAVE_TMLOCK

typedef struct tracedb_RWTmLock tracedb_RWTmLock;
typedef tracedb_RWTmLock *tracedb_RWTmLockRef;
#define TRACEDB_RWTMLOCKREF( rwlock ) & ( rwlock )
#define TRACEDB_RWTMLOCKDEREF( rwlock ) * ( rwlock )

int tracedb_tm_rwlock_init ( tracedb_RWTmLock *rwlock );
int tracedb_tm_rwlock_rdlock ( tracedb_RWTmLockRef rwlock );
int tracedb_tm_rwlock_wrlock ( tracedb_RWTmLockRef rwlock );
int tracedb_tm_rwlock_tryrdlock ( tracedb_RWTmLockRef rwlock );
int tracedb_tm_rwlock_trywrlock ( tracedb_RWTmLockRef rwlock );
int tracedb_tm_rwlock_timedrdlock ( tracedb_RWTmLockRef rwlock, timeoutref_t tm );
int tracedb_tm_rwlock_timedwrlock ( tracedb_RWTmLockRef rwlock, timeoutref_t tm );
int tracedb_tm_rwlock_unlock ( tracedb_RWTmLockRef rwlock );
int tracedb_tm_rwlock_destroy ( tracedb_RWTmLockRef rwlock );

#endif

/* RWTmLockInit
 *  initialize a rwlock with default attributes
 */
#define tracedb_RWTmLockInit( rwlock ) \
    tracedb_tm_rwlock_init ( rwlock )

/* RWTmLockReadLock
 *  acquire a read lock
 *  may fail, so test return result
 */
#define tracedb_RWTmLockReadLock( rwlock ) \
    tracedb_tm_rwlock_rdlock ( TRACEDB_RWLOCKREF ( rwlock ) )

/* RWTmLockWriteLock
 *  acquire a write lock
 *  may fail, so test return result
 */
#define tracedb_RWTmLockWriteLock( rwlock ) \
    tracedb_tm_rwlock_wrlock ( TRACEDB_RWLOCKREF ( rwlock ) )

/* RWTmLockTryReadLock
 *  try to acquire a read lock
 *  returns true if obtained
 */
#define tracedb_RWTmLockTryReadLock( rwlock ) \
    tracedb_tm_rwlock_tryrdlock ( TRACEDB_RWLOCKREF ( rwlock ) )

/* RWTmLockTryWriteLock
 *  try to acquire a write lock
 *  returns true if obtained
 */
#define tracedb_RWTmLockTryWriteLock( rwlock ) \
    tracedb_tm_rwlock_trywrlock ( TRACEDB_RWLOCKREF ( rwlock ) )

/* RWTmLockTimedReadLock
 *  try to acquire a read lock before the timeout
 *  returns true if obtained
 *  "tm" is a timeout_t initialized by TimeoutInit
 */
#define tracedb_RWTmLockTimedReadLock( rwlock, tm ) \
    tracedb_tm_rwlock_timedrdlock ( TRACEDB_RWLOCKREF ( rwlock ), TRACEDB_TIMEOUTREF ( tm ) )

/* RWTmLockTimedWriteLock
 *  try to acquire a write lock before the timeout
 *  returns true if obtained
 *  "tm" is a timeout_t initialized by TimeoutInit
 */
#define tracedb_RWTmLockTimedWriteLock( rwlock, tm ) \
    tracedb_tm_rwlock_timedwrlock ( TRACEDB_RWLOCKREF ( rwlock ), TRACEDB_TIMEOUTREF ( tm ) )

/* RWTmLockUnlock
 *  release the lock
 */
#define tracedb_RWTmLockUnlock( rwlock ) \
    tracedb_tm_rwlock_unlock ( TRACEDB_RWLOCKREF ( rwlock ) )

/* RWTmLockWhack
 *  tear down a rwlock
 */
#define tracedb_RWTmLockWhack( rwlock ) \
    tracedb_tm_rwlock_destroy ( TRACEDB_RWLOCKREF ( rwlock ) );


#ifdef __cplusplus
}
#endif

#endif /* _h_lockcmn_ */
