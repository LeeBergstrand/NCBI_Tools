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

#include <klib/log.h>
#include <klib/out.h>
#include <klib/status.h>
#include <klib/rc.h>

#include <kfs/file.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>


static
rc_t run (void )
{
    const KFile * f;
    rc_t rc;

    rc = KFileMakeStdIn (&f);
    if (rc == 0)
    {
        char buff [100];
        uint64_t pos;
        size_t num_read;

        for (pos = 0, rc = 0; rc == 0; pos += num_read)
        {
            rc = KFileRead (f, pos, buff, sizeof buff, &num_read);
            if (rc == 0)
            {
                KOutMsg ("'%.*s'\n", num_read, buff);
            }
            else if ((GetRCObject(rc) == rcTimeout) &&
                     (GetRCState(rc) == rcDone))
            {
                KOutMsg ("Timeout\n");
                rc = 0;
            }
        }
    }
    return rc;
}


rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        "  %s [OPTIONS]\n"
        "\n"
        "Summary:\n"
        "  test sysfile timeout approacn\n",
        progname);
}

const char UsageDefaultName[] = "test-sysfile-timeout";
rc_t CC Usage (const Args * args)
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


ver_t CC KAppVersion (void)
{
    return 0;
}

rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;
    rc_t rc;

    rc = ArgsMakeAndHandle (&args, argc, argv, 0);
    if (rc == 0)
        rc = run ();

    if (rc)
        LOGERR (klogErr, rc, "Exiting status");
    else
        STSMSG (0, ("Exiting status (%R)\n", rc));
    return rc;
}
