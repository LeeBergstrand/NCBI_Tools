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

#include <vfs/manager.h>
#include <vfs/path.h>
#include <kfs/file.h>

#include <common/test_assert.h>

static
rc_t run (void)
{
    VFSManager * manager;
    rc_t rc = 0, orc = 0;

    STSMSG (1, ("Make VFSManager"));
    rc = VFSManagerMake (&manager);
    STSMSG (2, ("rc %R", orc, orc));
    if (rc == 0)
    {
#if 1
        static const char name[] = "test-kfs-manager-data-file";
#else
        static const char name[] = 
            "ncbi-kfs:test-kfs-manager-data-file?enc&pwfile=password";
#endif
        VPath * path;

        STSMSG (1, ("Make test VPath file '%s'",name));
        rc = VPathMake (&path, name);
        STSMSG (2, ("rc %R", orc, orc));
        if (rc == 0)
        {
            KFile * file;

            STSMSG (1, ("Open File for write using manager and path"));
            rc = VFSManagerCreateFile (manager, &file, false, 0666, kcmCreate,
                                       path);
            STSMSG (2, ("rc %R", rc, rc));
            if (rc == 0)
            {
                char buff[4096];
                size_t ix;
                size_t num_writ;
                uint64_t tot_writ = 0;

                for (ix = 0; ix < sizeof buff; ++ix)
                    buff[ix] = 'A' + (ix%26);

                STSMSG (1, ("writing to file"));
                for (ix = 0; ix < 32; ++ix)
                {
                    rc = KFileWrite (file, tot_writ, buff, sizeof buff, &num_writ);
                    if (rc == 0)
                        tot_writ += num_writ;
                };
                                     





                STSMSG (1, ("Release file - it should whack"));
                orc = KFileRelease (file);
                STSMSG (2, ("rc %R", orc, orc));
                if (rc == 0) rc = orc;


#if 1
                STSMSG (1, ("Remove file"));
                orc = VFSManagerRemove (manager, true, path);
                STSMSG (2, ("rc %R", orc, orc));
                if (rc == 0) rc = orc;
#endif
            }
            STSMSG (1, ("Release VPath - it should Whack"));
            orc = VPathRelease (path);
            STSMSG (2, ("rc %R", orc, orc));
            if (rc == 0) rc = orc;
        }
        STSMSG (1, ("Release VFSManager - it should Whack"));
        orc = VFSManagerRelease (manager);
        STSMSG (2, ("rc %R", orc, orc));
        if (rc == 0) rc = orc;
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
        "  Test the VFSManager class.\n",
        progname);
}


const char UsageDefaultName[] = "test-kfsmanager";

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
