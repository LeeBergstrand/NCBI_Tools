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
#include <klib/out.h>
#include <klib/text.h>
#include <klib/rc.h>
#include <sysalloc.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <common/test_assert.h>

const char UsageDefaultName[] = "test-error";
rc_t CC UsageSummary ( const char * progname )
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        "  %s\n"
        "\n", progname );
}


rc_t CC Usage ( const Args * args )
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


/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
ver_t CC KAppVersion ( void )
{
    return 0x1000000;
}


rc_t CC KMain ( int argc, char *argv [] )
{
    rc_t rc = 0;
    Args * args;

    rc = ArgsMakeAndHandle (&args, argc, argv, 0);
    if (rc == 0)
    {
        int i;

        KOutMsg ( "Tests output of error codes:\n" );
        for ( i = 0; i < 1000; ++i )
        {
            KOutMsg ( "%d: %!\n", i, i );
        }
}
    return rc;
}
