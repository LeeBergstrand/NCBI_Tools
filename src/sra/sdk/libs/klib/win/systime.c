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

#include <klib/extern.h>
#include <klib/time.h>

#include <WINDOWS.H>
#include <os-native.h>


/*--------------------------------------------------------------------------
 * KTime_t
 *  64 bit time_t
 */

#if _ARCH_BITS == 32
#define UNIX_EPOCH_IN_WIN       116444736000000000ULL
#else
#define UNIX_EPOCH_IN_WIN       116444736000000000UL
#endif
#define UNIX_TIME_UNITS_IN_WIN  10000000


/* KTime2FILETIME
 *  convert from Unix to Windows
 */
static
const FILETIME *KTime2FILETIME ( KTime_t ts, FILETIME *ft )
{
    uint64_t win_time = ( ts * UNIX_TIME_UNITS_IN_WIN ) + UNIX_EPOCH_IN_WIN;
    ft -> dwLowDateTime = ( DWORD ) win_time;
    ft -> dwHighDateTime = win_time >> 32;
    return ft;
}

/* FILETIME2KTime
 */
static
KTime_t FILETIME2KTime ( const FILETIME *ft )
{
    uint64_t win_time = ft -> dwLowDateTime + ( ( int64_t ) ft -> dwHighDateTime << 32 );
    return ( KTime_t ) ( win_time - UNIX_EPOCH_IN_WIN ) / UNIX_TIME_UNITS_IN_WIN;
}


/* Stamp
 *  current timestamp
 */
LIB_EXPORT KTime_t CC KTimeStamp ( void )
{
    FILETIME ft;
    GetSystemTimeAsFileTime ( & ft );
    return FILETIME2KTime ( & ft );
}


/*--------------------------------------------------------------------------
 * SYSTEMTIME
 */

static
int SYSTEMTIME_compare ( const SYSTEMTIME *a, const SYSTEMTIME *b )
{
    int diff = a -> wMonth - b -> wMonth;
    if ( diff == 0 )
    {
        diff = a -> wDay - b -> wDay;
        if ( diff == 0 )
        {
            diff = a -> wHour - b -> wHour;
            if ( diff == 0 )
            {
                diff = a -> wMinute - b -> wMinute;
                if ( diff == 0 )
                    diff = a -> wSecond - b -> wSecond;
            }
        }
    }
    return diff;
}

static
void SYSTEMTIME_from_half_baked_SYSTEMTIME ( const SYSTEMTIME *half_baked, SYSTEMTIME *proper, WORD year )
{
    int i;
    FILETIME ft;

    * proper = * half_baked;

    /* fix some stuff */
    proper -> wYear = year;
    proper -> wMonth = half_baked -> wMonth;
    proper -> wDayOfWeek = 0; /* ignored */
    proper -> wDay = 1;
    proper -> wHour = half_baked -> wHour;
    proper -> wMinute = 0;
    proper -> wSecond = 0;
    proper -> wMilliseconds = 0;

    /* convert it to FILETIME and back, just to get the proper day of week
       if there's a better way to do it, go ahead.
       by now, my lunch is too difficult to keep down... */
    SystemTimeToFileTime ( proper, & ft );
    FileTimeToSystemTime ( & ft, proper );

    /* now, move ahead to the day of week */
    proper -> wDay += half_baked -> wDayOfWeek - proper -> wDayOfWeek;
    if ( half_baked -> wDayOfWeek < proper -> wDayOfWeek )
        proper -> wDay += 7;
    proper -> wDayOfWeek = half_baked -> wDayOfWeek;

    /* now find the occurrence of the weekday */
    if ( half_baked -> wDay > 1 )
        proper -> wDay += ( half_baked -> wDay - 1 ) * 7;
}

/*--------------------------------------------------------------------------
 * KTime
 *  simple time structure
 */


/* Make
 *  make KTime from struct tm
 */
static
void KTimeMake ( KTime *kt, const SYSTEMTIME *st )
{
    kt -> year = st -> wYear;
    kt -> month = st -> wMonth - 1;
    kt -> day = st -> wDay - 1;
    kt -> weekday = st -> wDayOfWeek;
    kt -> hour = ( uint8_t ) st -> wHour;
    kt -> minute = ( uint8_t ) st -> wMinute;
    kt -> second = ( uint8_t ) st -> wSecond;
}


/* Local
 *  populate "kt" from "ts" in local time zone
 */
LIB_EXPORT const KTime* CC KTimeLocal ( KTime *kt, KTime_t ts )
{
    if ( kt != NULL )
    {
        DWORD tz_id;
        FILETIME ft;
        SYSTEMTIME gst, lst;
        TIME_ZONE_INFORMATION tz;

        /* generate windows time in 100nS units */
        KTime2FILETIME ( ts, & ft );

        /* generate a system time - almost what we need,
           except it's GMT and has no associated time zone */
        FileTimeToSystemTime ( & ft, & gst );

        /* assume we're NOT in DST */
        kt -> dst = false;

        /* get local timezone information */
        tz_id = GetTimeZoneInformation ( & tz );
        switch ( tz_id )
        {
        case TIME_ZONE_ID_STANDARD:
        case TIME_ZONE_ID_DAYLIGHT:

            /* convert GMT time to local time with tz info */
            SystemTimeToTzSpecificLocalTime ( & tz, & gst, & lst );
            KTimeMake ( kt, & lst );

            /* our gentle brothers and sisters in Redmond never
               cease to amaze... it's very nice - handy, even -
               to know that the system is "currently" operating
               in one mode or another, but that tells us nothing
               about the timestamp we're trying to interpret.

               to discover whether the timestamp we're converting
               is within daylight savings time, we can compare against
               the two railpost SYSTEMTIME entries, but then there's
               no telling whether we're in or out since the calendar
               is circular! aside from having to perform a multi-part
               comparison, we'll come to different conclusions depending
               upon whether the hemisphere is northern or southern!

               to disambiguate, we can use tz_id to detect hemisphere,
               and then know what's going on. Wow.

               also, it's not clear to the author whether the returned
               structures in tz will be proper or hacked, since the
               MSDN descriptions only describe how to hack them for
               input, but not how they will look on output. */

            if ( tz . StandardDate . wMonth == 0 || tz . DaylightDate . wMonth == 0 )
                kt -> tzoff = - ( int16_t ) tz . Bias;
            else
            {
                bool south = tz_id == TIME_ZONE_ID_DAYLIGHT;

                SYSTEMTIME cst, dst, std;
                GetSystemTime ( & cst );

                /* fill out proper structures, since those in tz are bad... */
                SYSTEMTIME_from_half_baked_SYSTEMTIME ( & tz . DaylightDate, & dst, cst . wYear );
                SYSTEMTIME_from_half_baked_SYSTEMTIME ( & tz . StandardDate, & std, cst . wYear );

                /* perform northern test for DST */
                if ( SYSTEMTIME_compare ( & lst, & dst ) >= 0 && SYSTEMTIME_compare ( & lst, & std ) < 0 )
                    kt -> dst = true;

                /* test to see which hemisphere */
                south ^= ( SYSTEMTIME_compare ( & cst, & dst ) >= 0 && SYSTEMTIME_compare ( & cst, & std ) < 0 );

                /* correct for southern hemisphere */
                kt -> dst ^= south;

                /* set the timezone offset */
                kt -> tzoff = - ( int16_t ) ( tz . Bias +
                    kt -> dst ? tz . DaylightBias : tz . StandardBias );
            }
            break;

        default:

            /* failed - use GMT instead */
            KTimeMake ( kt, & gst );
            kt -> tzoff = 0;
        }
    }
    return kt;
}


/* Global
 *  populate "kt" from "ts" in GMT
 */
LIB_EXPORT const KTime* CC KTimeGlobal ( KTime *kt, KTime_t ts )
{
    if ( kt != NULL )
    {
        FILETIME ft;
        SYSTEMTIME gst;

        /* generate windows time in 100nS units */
        KTime2FILETIME ( ts, & ft );

        /* generate a system time */
        FileTimeToSystemTime ( & ft, & gst );

	/* fill out GMT time structure */
        KTimeMake ( kt, & gst );
        kt -> tzoff = 0;
        kt -> dst = false;
    }
    return kt;
}
