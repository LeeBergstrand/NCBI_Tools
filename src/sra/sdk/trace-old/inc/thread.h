#ifndef _h_thread_
#define _h_thread_

#ifndef _h_posthread_
#include "posthread.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * Thread
 */
typedef pthread_t tracedb_Thread;
typedef tracedb_Thread tracedb_ThreadRef;
#define TRACEDB_THREADREF( t ) ( t )
#define TRACEDB_THREADDEREF( t ) ( t )

/* ThreadCreate
 *  create a new thread with an entry function and data
 */
int tracedb_ThreadCreate ( tracedb_Thread *t,
        int ( * f ) ( void *data ), void *data );

/* ThreadCurrent
 *  return an id for the current thread
 */
#define tracedb_ThreadCurrent() \
    pthread_self ()

/* ThreadExit
 *  exit for a single thread
 */
#define tracedb_ThreadExit( retval ) \
    pthread_exit ( retval )

/* ThreadJoin
 *  suspend current thread until joined thread exits
 *  gets return value in retval
 */
int tracedb_ThreadJoin ( tracedb_ThreadRef t, int *retval );

/* ThreadDetach
 *  allow indicated thread to run independently of group
 */
#define tracedb_ThreadDetach( t ) \
    pthread_detach ( t )

/* ThreadCancel
 *  send a cancellation message to thread
 */
#define tracedb_ThreadCancel( t ) \
    pthread_cancel ( t )

/* ThreadWhack
 *  noop in POSIX
 */
#define tracedb_ThreadWhack( t ) \
    ( ( void ) 0 )


#ifdef __cplusplus
}
#endif

#endif /* _h_thread_ */
