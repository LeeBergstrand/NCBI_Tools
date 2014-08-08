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

#include <kapp/main.h>
#include <kapp/args.h>
#include <klib/time.h>
#include <klib/log.h>
#include <klib/out.h>
#include <klib/rc.h>

#include <common/test_assert.h>


static
rc_t run ( void )
{
    KTime tm;
    KTime_t ts;

    KOutMsg ( "getting current time stamp..." );
    ts = KTimeStamp ();
    KOutMsg ( " done\n" );

    KOutMsg ( "issuing simple UTC time-of-day output as '%%T':\n  " );
    KOutMsg ( "%T", KTimeGlobal ( & tm, ts ) );
    KOutMsg ( " ( done )\n" );

    KOutMsg ( "issuing simple local time-of-day output as '%%T':\n  " );
    KOutMsg ( "%T", KTimeLocal ( & tm, ts ) );
    KOutMsg ( " ( done )\n" );

    KOutMsg ( "issuing local date output as '%%hT':\n  " );
    KOutMsg ( "%hT", & tm );
    KOutMsg ( " ( done )\n" );

    KOutMsg ( "issuing long local datetime output as '%%lT':\n  " );
    KOutMsg ( "%lT", & tm );
    KOutMsg ( " ( done )\n" );

    return 0;
}


/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
ver_t CC KAppVersion ( void )
{
    return 0;
}


/* Usage
 *  This function is called when the command line argument
 *  handling sees -? -h or --help
 */
rc_t CC UsageSummary ( const char *progname )
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        "  %s [Options]\n"
        "\n"
        "Summary:\n"
        "  Simple test of time conversion and printing.\n"
        , progname );
}

const char UsageDefaultName[] = "time-test";

rc_t CC Usage ( const Args *args )
{
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
    rc_t rc;

    if (args == NULL)
        rc = RC (rcApp, rcArgv, rcAccessing, rcSelf, rcNull);
    else
        rc = ArgsProgram (args, &fullpath, &progname);

    UsageSummary (progname);

    KOutMsg ("Options:\n");

    HelpOptionsStandard();

    HelpVersion (fullpath, KAppVersion());

    return rc;
}

    
/* KMain
 */
rc_t CC KMain ( int argc, char *argv [] )
{
    Args *args;
    rc_t rc = ArgsMakeAndHandle ( & args, argc, argv, 0 );
    if ( rc == 0 )
    {
        rc = run ();        
        ArgsWhack ( args );
    }

    return rc;
}
