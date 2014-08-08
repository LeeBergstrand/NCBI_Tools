#include "timeout.h"

#include <errno.h>
#include <assert.h>

/*--------------------------------------------------------------------------
 * timeout_t
 */

/* TimeoutPrepare
 * TimeoutInit
 *  initialize a timeout value from wait value
 */
void tracedb_timeout_prepare ( tracedb_timeout_t *tm )
{
    struct timeval tv;
    struct timezone tz;
    int64_t abs_micros;

    /* current time in seconds and uS */
    gettimeofday ( & tv, & tz );
    
    /* convert to uS */
    abs_micros = tv . tv_sec;
    abs_micros = abs_micros * 1000 * 1000 + tv . tv_usec;
    
    /* add wait period for future timeout */
    abs_micros += ( uint64_t ) tm -> mS * 1000;
    
    /* convert to seconds and nS */
    tm -> ts . tv_sec = ( time_t ) ( abs_micros / 1000000 );
    tm -> ts . tv_nsec = ( uint32_t ) ( ( abs_micros % 1000000 ) * 1000 );
    tm -> prepared = true;
}

/* TimeoutWait
 *  periodically executes a poll function
 *  until condition is satisfied or timeout
 *  returns true if condition was satisfied
 */
bool tracedb_timeout_wait ( tracedb_timeoutref_t tm, unsigned int mS,
    bool ( * poll ) ( void *data ), void *data )
{
    struct timezone tz;
    struct timeval tv, to;
    struct timespec period;

    /* poll immediately */
    if ( ( * poll ) ( data ) )
        return true;

    /* check absolute time for timeout */
    gettimeofday ( & tv, & tz );

    /* prepare the timeout if needed */
    if ( ! tm -> prepared )
    {
        /* convert to uS */
        int64_t abs_micros = tv . tv_sec;
        abs_micros = abs_micros * 1000 * 1000 + tv . tv_usec;
    
        /* add wait period for future timeout */
        abs_micros += ( uint64_t ) tm -> mS * 1000;
    
        /* convert to seconds and nS */
        tm -> ts . tv_sec = ( time_t ) ( abs_micros / 1000000 );
        tm -> ts . tv_nsec = ( uint32_t ) ( ( abs_micros % 1000000 ) * 1000 );
        tm -> prepared = true;
    }

    /* build a nanosecond sleep value */
    if ( mS <= 10 )
    {
        period . tv_sec = 0;
        period . tv_nsec = 10 * 1000 * 1000;
    }
    else
    {
        period . tv_sec = mS / 1000;
        period . tv_nsec = ( mS % 1000 ) * 1000 * 1000;
    }

    /* build a timeval from timeout */
    to . tv_sec = tm -> ts . tv_sec;
    to . tv_usec = ( tm -> ts . tv_nsec + 999 ) / 1000;
    if ( to . tv_usec >= 1000000 )
    {
        ++ to . tv_sec;
        to . tv_usec -= 1000000;
    }
    
    /* poll until exit condition */
    while ( tv . tv_sec <= to . tv_sec )
    {	
        /* check absolute time for timeout */
        if ( tv . tv_sec == to . tv_sec )
        {
            if ( tv . tv_usec >= to . tv_usec )
                break;
        }
	
        /* wait for the poll period */
        nanosleep ( & period, 0 );
	
        /* poll again */
        if ( ( * poll ) ( data ) )
            return true;

        /* get new time of day */
        gettimeofday ( & tv, & tz );
    }
    return false;
}

/* TimeoutIncrement
 *  increments a timeout value by a number of mS
 */
void tracedb_timeout_increment ( tracedb_timeout_t *tm, unsigned int mS )
{
    if ( mS > 0 && tm != NULL )
    {
        /* convert to nS */
        int64_t abs_nanos = ( int64_t ) tm -> ts . tv_sec * 1000 * 1000 * 1000;
        abs_nanos += tm -> ts . tv_nsec;
        
        /* add wait period for future timeout */
        abs_nanos += ( int64_t ) mS * 1000 * 1000;
        
        /* convert to seconds and nS */
        tm -> ts . tv_sec = ( time_t ) ( abs_nanos / ( 1000 * 1000 * 1000 ) );
        tm -> ts . tv_nsec = ( uint32_t ) ( abs_nanos % ( 1000 * 1000 * 1000 ) );
    }
}



/*--------------------------------------------------------------------------
 * NanoSeconds
 */

/* NanoSecondsSleep
 *  supports illusion of fine grained sleeping
 */
void tracedb_NanoSecondsSleep ( tracedb_NanoSeconds *cumulative, uint64_t nS )
{
    if ( cumulative != NULL && ( * cumulative += nS ) >= 10000000 )
    {
        struct timespec ts;
        ts . tv_sec = * cumulative / 1000000000;
        ts . tv_nsec = * cumulative % 1000000000;

        * cumulative = 0;

        nanosleep ( & ts, 0 );
    }
}
