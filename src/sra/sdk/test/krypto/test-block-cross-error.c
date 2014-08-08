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
#include <krypto/encfile.h>
#include <krypto/key.h>
#include <kapp/main.h>
#include <kapp/args.h>

#include <klib/log.h>
#include <klib/out.h>
#include <klib/status.h>
#include <klib/rc.h>

#include <kfs/directory.h>
#include <kfs/file.h>

#include <string.h>

#include <common/test_assert.h>

/*
 * not intended to be pretty.
 * not intended to be slick.
 * not intended to make tools like valgrind happy.
 *
 * This does not REALLY test the quality of the RNG; it's just
 * a development test and a "did I break it" test.
 *
 * It just spews lines of random bytes.  These could be gathered
 * and analyzed with another tool such as the one from NIST.  Perhaps
 * a bit of change to the output format might be needed for that.
 */

#define TEST_FILE_NAME  "test-encfile.nenc"

KKey key = { kkeyAES128, "1234567890123456" };

/*
 * tools like valgrind will hate this function.  stuff doesn't get released on failure paths.
 */
static
rc_t run()
{
    KFile * encrypted = NULL;
    KFile * encryptor = NULL;
    KDirectory * dir;
    uint8_t buff [] = "0123456789";
    size_t num_writ;
    rc_t rc;
    int ix;
    uint64_t pos;

    rc = KDirectoryNativeDir (&dir);
    if (rc)
        LOGERR (klogErr, rc, "error opening pwd");

    else
    {
        rc = KDirectoryCreateFile (dir, &encrypted, false, 0777, kcmInit,
                                   TEST_FILE_NAME);
        if (rc)
            PLOGERR (klogErr,
                     (klogErr, rc, "error creating $F", "F=%s", TEST_FILE_NAME));

        else
        {

            rc = KEncFileMakeWrite (&encryptor, encrypted, &key);
            if (rc)
                LOGERR (klogErr, rc, "error creating encryptor object");

            else
            {
                STSMSG (1, ("starting write test"));

                for (ix = 0; ix < 10000; ++ix)
                {
                    rc = KFileWriteAll (encryptor, ix * (sizeof (buff) - 1), buff,
                                        sizeof (buff) - 1 /* no NUL */, &num_writ);
                    if (rc)
                    {
                        LOGERR (klogErr, rc, "error writing");
                        break;
                    }
                }
                KFileRelease (encryptor);
            }
            KFileRelease (encrypted);
        }
        KDirectoryRelease (dir);
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
        "  \n",
        progname);
}

const char UsageDefaultName [] = "test-block-cross-error";
rc_t CC Usage (const Args * args)
{
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
    rc_t rc;

    if (args == NULL)
        rc = RC (rcApp, rcArgv, rcAccessing, rcSelf, rcNull);
    else
        rc = ArgsProgram (args, &fullpath, &progname);
    if (rc)
        progname = fullpath = UsageDefaultName;

    UsageSummary (progname);

    KOutMsg ("Options:\n");

    HelpOptionsStandard ();

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
        STSMSG (0, ("Exiting status (%R)\n", rc));
    return rc;
}

