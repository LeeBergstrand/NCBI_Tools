#ifndef _h_queue_
#define _h_queue_

#ifndef _h_semaphore_
#include "semaphore.h"
#endif

#ifndef _h_timeout_
#include "timeout.h"
#endif

#ifndef _h_container_
#include "container.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * Queue
 *  a simple thread-safe queue structure supporting push/pop operation
 */
typedef struct tracedb_Queue tracedb_Queue;
struct tracedb_Queue
{
    tracedb_Semaphore rc;
    tracedb_Semaphore wc;

    unsigned int capacity;
    unsigned int bmask;
    unsigned int imask;
    volatile unsigned int read;
    volatile unsigned int write;
    volatile void **buffer;
    volatile bool sealed;
};

/* QueueInit
 * initialize a queue object
 */
int tracedb_QueueInit ( tracedb_Queue *q, unsigned int capacity );

/* QueuePush
 *  add an object to the queue
 *  "tm" is a timeout_t prepared by TimeoutInit
 */
int tracedb_queue_push ( tracedb_Queue *q, const void *elem,
    tracedb_timeoutref_t tm );
#define tracedb_QueuePush( q, elem, tm ) \
    tracedb_queue_push ( q, elem, TIMEOUTREF ( tm ) )

/* QueuePop
 *  pop an object from queue
 *  "tm" is a timeout_t prepared by TimeoutInit
 */
int tracedb_queue_pop ( tracedb_Queue *q, void **elem,
    tracedb_timeoutref_t tm );
#define tracedb_QueuePop( q, elem, tm ) \
    tracedb_queue_pop ( q, elem, TIMEOUTREF ( tm ) )

/* QueueSealed
 *  ask if the queue has been closed off
 *  meaning there will be no further push operations
 */
#define tracedb_QueueSealed( q ) \
    ( ( const tracedb_Queue* ) ( q ) ) -> sealed

/* QueueSeal
 *  indicate that the queue has been closed off
 *  meaning there will be no further push operations
 */
#define tracedb_QueueSeal( q ) \
    ( void ) ( ( q ) -> sealed = true )

/* QueueWhack
 *  removes all elements from queue,
 *  executes a user provided destructor,
 *  and whacks buffer space
 */
void tracedb_QueueWhack ( tracedb_Queue *q,
    void ( * whack ) ( void *elem, void *data ), void *data );

#ifdef __cplusplus
}
#endif

#endif /* _h_queue_ */
