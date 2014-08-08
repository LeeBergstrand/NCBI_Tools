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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <klib/rc.h>
#include <vfs/path.h>
#include <kfs/file.h>
#include <kfs/kfs-priv.h>
#include <klib/log.h>
#include <klib/status.h>
#include <klib/out.h>
#include <kapp/main.h>

static
rc_t run ()
{
    const KFile * in;
    rc_t rc = 0;

    do {
        uint64_t in_pos = 0;
        size_t num_read;
        char buffer [8 * 1024];

        rc = KFileMakeStdIn (&in);
        if (rc) break;

        for ( ;; )
        {
            VPath * path;

            rc = KOutMsg ("Enter a system specific path: ");
            if (rc) break;

            memset (buffer, 0, sizeof buffer);

            rc = KFileRead (in, in_pos, buffer, sizeof buffer, &num_read);

            if (rc)
            {
                LOGERR (klogFatal, rc, "error with KFileRead");
                break;
            }

            if (num_read == 0) break;

            in_pos += num_read;
            
            if (buffer[num_read-1] == '\n')
                buffer[num_read-1] = '\0';
            if (buffer[num_read-2] == '\r')
                buffer[num_read-2] = '\0';

            rc = VPathMakeSysPath (&path, buffer);
            if (rc)
            {
                LOGERR (klogErr, rc, "error with MakeSysPath");
                break;
            }

            memset (buffer, 0, sizeof buffer);

            rc = VPathReadPath (path, buffer, sizeof buffer, &num_read);
            if (rc)
            {
                LOGERR (klogErr, rc, "error wth ReadPath");
                break;
            }

            rc = KOutMsg ("VPath path is '%s' size '%zu'\n\n", buffer, num_read);
            if (rc)
            {
                LOGERR (klogErr, rc, "error with KOuMsg");
                break;
            }
        }
    } while (0);
    return rc;
}


rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        "  %s [OPTIONS] path strings\n"
        "\n"
        "Summary:\n"
        "  Display the KFS view of a system specific path strings.\n"
        "\n", progname);
}

const char UsageDefaultName[] = "test-kpath-read-path";

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

rc_t CC KMain (int argc, char * argv[])
{
    Args * args;
    rc_t rc;

    rc = ArgsMakeAndHandle (&args, argc, argv, 1, NULL, 0);
    if (rc)
        LOGERR (klogFatal, rc, "Could not parse command line");
    else
    {
        rc = run();
        ArgsWhack (args);
    }
    return rc;
}




