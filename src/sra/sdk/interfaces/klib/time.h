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

#ifndef _h_klib_time_
#define _h_klib_time_

#ifndef _h_klib_extern_
#include <klib/extern.h>
#endif

#ifndef _h_klib_defs_
#include <klib/defs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * KTime_t
 *  64 bit time_t
 */


/* Stamp
 *  current timestamp
 */
KLIB_EXTERN KTime_t CC KTimeStamp ( void );


/*--------------------------------------------------------------------------
 * KTime
 *  simple time structure
 */
typedef struct KTime KTime;
struct KTime
{
    uint32_t year;        /* full year                                */
    uint16_t month;       /* 0 .. 11                                  */
    uint16_t day;         /* 0 .. 30                                  */
    uint16_t weekday;     /* 0 .. 6 : 0 is Sunday                     */
    int16_t tzoff;        /* -719 .. +719 minutes from Prime Meridian */
    uint8_t hour;         /* 0 .. 23                                  */
    uint8_t minute;       /* 0 .. 59                                  */
    uint8_t second;       /* 0 .. 60 :  60 used for leap seconds      */
    bool dst;             /* true if Daylight Savings Time in effect  */
};


/* Local
 *  populate "kt" from "ts" in local time zone
 */
KLIB_EXTERN const KTime* CC KTimeLocal ( KTime *kt, KTime_t ts );


/* Global
 *  populate "kt" from "ts" in GMT
 */
KLIB_EXTERN const KTime* CC KTimeGlobal ( KTime *kt, KTime_t ts );


#ifdef __cplusplus
}
#endif

#endif /*  _h_klib_time_ */
