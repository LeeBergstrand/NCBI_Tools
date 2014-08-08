#include "thread.h"

#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#define TRACEDB_RETURN_DATA 1

/*--------------------------------------------------------------------------
 * Thread
 */

/* ThreadRun
 */
typedef struct tracedb_ThreadRunData tracedb_ThreadRunData;
struct tracedb_ThreadRunData
{
    int ( * f ) ( void *data );
    void *data;
#if TRACEDB_RETURN_DATA
    int status;
#endif
};

static
void *tracedb_ThreadRun ( void *data )
{
    tracedb_ThreadRunData *pb = data;
#if TRACEDB_RETURN_DATA
    pb -> status = ( * pb -> f ) ( pb -> data );
    return data;
#else
    int status = ( * pb -> f ) ( pb -> data );
    free ( pb );
    return ( void* ) status;
#endif
}

/* ThreadCreate
 *  create a new thread with an entry function and data
 *  here essentially as a wrapper to isolate callback function
 */
int tracedb_ThreadCreate ( tracedb_Thread *t,
    int ( * f ) ( void *data ), void *data )
{
    int status;
    tracedb_ThreadRunData *pb = malloc ( sizeof * pb );
    if ( pb == NULL )
        return errno;
    pb -> f = f;
    pb -> data = data;
    status = pthread_create ( t, NULL, tracedb_ThreadRun, pb );
    if ( status != 0 )
        free ( pb );
    return status;
}

/* ThreadJoin
 *  suspend current thread until joined thread exits
 *  gets return value in retval
 */
int tracedb_ThreadJoin ( tracedb_ThreadRef t, int *retval )
{
    void *trtn;
    int status = pthread_join ( t, & trtn );
#if TRACEDB_RETURN_DATA
    if ( trtn == NULL )
    {
        if ( retval != NULL )
            * retval = EINVAL;
    }
    else
    {
        if ( retval != NULL )
            * retval = ( ( tracedb_ThreadRunData* ) trtn ) -> status;
        free ( trtn );
    }
#else
    if ( retval != NULL )
        * retval = ( int ) trtn;
#endif
    return status;
}
