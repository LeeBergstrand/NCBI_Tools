/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#include <sysalloc.h>
#include <kproc/timeout.h>
#include <os-native.h>
#include <klib/log.h>
#include <klib/rc.h>


/*--------------------------------------------------------------------------
 * timeout_t
 *  a structure for communicating a timeout
 *  which under Unix converts to an absolute time once prepared
 */


/* Init
 *  initialize a timeout in milliseconds
 */
rc_t TimeoutInit ( timeout_t *tm, uint32_t msec )
{
    if ( tm == NULL )
        return RC ( rcPS, rcTimeout, rcConstructing, rcSelf, rcNull );

    tm -> mS = msec;
    tm -> prepared = false;

    return 0;
}

/* Prepare
 *  ensures that a timeout is prepared with an absolute value
*/
rc_t TimeoutPrepare ( timeout_t *self )
{
    if ( self == NULL )
        return RC ( rcPS, rcTimeout, rcUpdating, rcSelf, rcNull );

    if ( ! self -> prepared )
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
        abs_micros += ( uint64_t ) self -> mS * 1000;
    
        /* convert to seconds and nS */
        self -> ts . tv_sec = ( time_t ) ( abs_micros / 1000000 );
        self -> ts . tv_nsec = ( uint32_t ) ( ( abs_micros % 1000000 ) * 1000 );
        self -> prepared = true;
    }

    return 0;
}
