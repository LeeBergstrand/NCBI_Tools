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
#include <klib/rc.h>
#include <klib/status.h>

#include <kfs/directory.h>
#include <kfs/file.h>

#include <kxml/xml.h>

#include <string.h>

#include <common/test_assert.h>

/*
 * not intended to be pretty.
 * not intended to be slick.
 * not intended to make tools like valgrind happy.
 */
#define BASE "test-xfs"
#define XML BASE "/test-xfs.tar.gz.xml"

#define INNER_FILE1 "test-xfs.tar.gz/test-xfs.tar/tar/kar.sra/a.bz2/a"
#define INNER_FILE2 "test-xfs.tar.gz/test-xfs.tar/tar/kar.sra/b.bz2/b"
#define INNER_ID   "7"

static
rc_t run()
{
    KDirectory * pwd;
    const KDirectory * xfs;
    const KFile * xml;
    rc_t rc;

    STSMSG (1, ("Open file system\n"));
    rc = KDirectoryNativeDir (&pwd);
    if (rc)
        LOGERR (klogErr, rc, "Failed to open filesystem");
    else
    {
        STSMSG (1, ("Open file %s\n", XML));
        rc = KDirectoryOpenFileRead (pwd, &xml, XML);
        if (rc)
            LOGERR (klogErr, rc, "failed to open xml file");
        else
        {
            STSMSG (1, ("Open chrooted as XML TOC %s\n", BASE));
            rc = KDirectoryOpenXTocDirRead (pwd, &xfs, true, xml, BASE);
            if (rc)
                LOGERR (klogErr, rc, "Failed to open xfs directory");
            else
            {
                const KFile * afile;

                STSMSG (1, ("Open file %s\n", INNER_FILE1));
                rc = KDirectoryOpenFileRead (xfs, &afile, INNER_FILE1);
                if (rc)
                    LOGERR (klogErr, rc, "failed to open inner most file");
                else
                {
                    char b [512];
                    size_t z;

                    memset (b, 0, sizeof b);

                    rc = KFileRead (afile, 0, b, sizeof (b), &z);
                    if (rc == 0)
                        KOutMsg ("++++++++++\n%zu bytes\n-----\n%s\n-----\n", z, b);
                    else
                        LOGERR (klogErr, rc, "Failed to read");
                    STSMSG (2, ("Closing file %s\n", INNER_FILE1));

                    KFileRelease (afile);
                }
                if (rc == 0)
                {
                    STSMSG (1, ("Open file %s\n", INNER_FILE2));
                    rc = KDirectoryOpenFileRead (xfs, &afile, INNER_FILE2);
                    if (rc)
                        LOGERR (klogErr, rc, "failed to open inner most file by id");
                    else
                    {
                        char b [512];
                        size_t z;

                        memset (b, 0, sizeof b);

                        rc = KFileRead (afile, 0, b, sizeof (b), &z);
                        if (rc == 0)
                            KOutMsg ("++++++++++\n%zu bytes\n-----\n%s\n-----\n", z, b);
                        else
                            LOGERR (klogErr, rc, "Failed to read");
                        KFileRelease (afile);
                    }
                }

                KDirectoryRelease (xfs);
            }
            if (rc == 0)
            {
                STSMSG (1, ("Open as XML TOC %s\n", BASE));
                rc = KDirectoryOpenXTocDirRead (pwd, &xfs, false, xml, BASE);
                if (rc)
                    LOGERR (klogErr, rc, "Failed to open xfs directory");
                else
                {
                    const KFile * afile;
                    STSMSG (1, ("Open file %s\n", INNER_FILE1));
                    rc = KDirectoryOpenFileRead (xfs, &afile, INNER_FILE1);
                    if (rc)
                        LOGERR (klogErr, rc, "failed to open inner most file");
                    else
                    {
                        char b [512];
                        size_t z;

                        memset (b, 0, sizeof b);

                        rc = KFileRead (afile, 0, b, sizeof (b), &z);
                        if (rc == 0)
                            KOutMsg ("++++++++++\n%zu bytes\n-----\n%s\n-----\n", z, b);
                        else
                            LOGERR (klogErr, rc, "Failed to read");
                        STSMSG (2, ("Closing file %s\n", INNER_FILE1));

                        KFileRelease (afile);
                    }
                    if (rc == 0)
                    {
                        STSMSG (1, ("Open file %s\n", INNER_FILE2));
                        rc = KDirectoryOpenFileRead (xfs, &afile, INNER_FILE2);
                        if (rc)
                            LOGERR (klogErr, rc, "failed to open inner most file by id");
                        else
                        {
                            char b [512];
                            size_t z;

                            memset (b, 0, sizeof b);

                            rc = KFileRead (afile, 0, b, sizeof (b), &z);
                            if (rc == 0)
                                KOutMsg ("++++++++++\n%zu bytes\n-----\n%s\n-----\n", z, b);
                            else
                                LOGERR (klogErr, rc, "Failed to read");
                            KFileRelease (afile);
                        }
                    }

                    KDirectoryRelease (xfs);
                }
            }
            KFileRelease (xml);
        }
        KDirectoryRelease (pwd);
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
        "  Test the KXTocDir type.\n",
        progname);
}


const char UsageDefaultName[] = "test-xfs";

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
    {

        rc = run();
        if (rc)
            LOGERR (klogErr, rc, "Exiting failure");
        else
            STSMSG (0, ("Exiting okay\n"));
    }

    if (rc)
        LOGERR (klogErr, rc, "Exiting status");
    else
        STSMSG (0, ("Existing status (%R)\n", rc));
    return rc;
}
