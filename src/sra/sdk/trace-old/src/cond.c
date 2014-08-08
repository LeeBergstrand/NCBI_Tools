#include "cond.h"

#include <errno.h>
#include <assert.h>

/*--------------------------------------------------------------------------
 * Condition
 *  an open condition object that uses the POSIX model
 */

/* ConditionInit
 */
int tracedb_ConditionInit ( tracedb_Condition *cnd )
{
    int status;

    if ( cnd == NULL )
        return EINVAL;

    status = pthread_cond_init ( & cnd -> cond, NULL );
    if ( status == 0 )
    {
        status = pthread_mutex_init ( & cnd -> mutex, NULL );
        if ( status != 0 )
            pthread_cond_destroy ( & cnd -> cond );
    }
    return status;
}
