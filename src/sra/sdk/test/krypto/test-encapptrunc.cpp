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

/**
* Unit tests for KEnvV2File
*/
#include <cstring>
#include <ktst/unit_test.hpp>
#include <krypto/key.h>
#include <krypto/encfile.h>
#include <krypto/encfile-priv.h>
#include <krypto/reencfile.h>
#include <kfs/impl.h>
#include <kfs/buffile.h>
#include <klib/out.h>
#include <kapp/args.h>
#include <kfg/config.h>

#include <common/test_assert.h>

using namespace std;

TEST_SUITE(EncAppTrunc);


TEST_CASE(EncApptrunc)
{
    OUTMSG (("\nEncAppTrunc\n"));

// ======================================================================
// a common prelude to many of these tests

    KDirectory * cwd;

    REQUIRE_RC(KDirectoryNativeDir (&cwd));

    const char name [] = "temp.encapptrunc.test";
    const char password [] = "just some password";

    KKey key;

    /* create two encrypted files from the same "source" */

    REQUIRE_RC(KKeyInitUpdate (&key, kkeyAES128, password, strlen (password)));

    KFile * raw;
    KFile * enc;

    REQUIRE_RC(KDirectoryCreateFile (cwd, &raw, true, 0660, kcmInit, name));
    REQUIRE_RC(KEncFileMakeUpdate_v2 (&enc, raw, &key));

    size_t   bsize;
    uint64_t fsize;

    REQUIRE_RC (KFileSize (enc, &fsize));

    OUTMSG (("fsize %lu\n", fsize));
    REQUIRE (fsize == 0);

    REQUIRE_RC (KFileWriteAll (enc, 0, "123", 3, &bsize));
    REQUIRE (bsize == 3);

    REQUIRE_RC (KFileSize (enc, &fsize));
    OUTMSG (("fsize %lu\n", fsize));
    REQUIRE (fsize == 3);

    REQUIRE_RC (KFileRelease (enc));
    REQUIRE_RC (KFileRelease (raw));

    REQUIRE_RC (KDirectoryFileSize (cwd, &fsize, name));
    OUTMSG (("fsize %lu\n", fsize));
    REQUIRE (fsize == 16 + 32768 + 64 + 16);

    REQUIRE_RC (KDirectoryOpenFileWrite (cwd, &raw, true, name));
    REQUIRE_RC(KEncFileMakeUpdate_v2 (&enc, raw, &key));

    REQUIRE_RC (KFileSize (enc, &fsize));
    OUTMSG (("fsize %lu\n", fsize));
    REQUIRE (fsize == 3);

    REQUIRE_RC (KFileWrite (enc, 3, "456", 3, &bsize));
    REQUIRE (bsize == 3);

    REQUIRE_RC (KFileSize (enc, &fsize));
    OUTMSG (("fsize %lu\n", fsize));
    REQUIRE (fsize == 6);

    REQUIRE_RC (KFileRelease (enc));
    REQUIRE_RC (KFileRelease (raw));

    REQUIRE_RC (KDirectoryFileSize (cwd, &fsize, name));
    REQUIRE (fsize == 16 + 32768 + 64 + 16);

    REQUIRE_RC (KDirectoryOpenFileWrite (cwd, &raw, true, name));
    REQUIRE_RC(KEncFileMakeUpdate_v2 (&enc, raw, &key));

    REQUIRE_RC (KFileSize (enc, &fsize));
    OUTMSG (("fsize %lu\n", fsize));
    REQUIRE (fsize == 6);

    char buffer [128];

    REQUIRE_RC (KFileRead (enc, 0, buffer, sizeof buffer, &bsize));
    REQUIRE (bsize == 6);

    for (unsigned ux = 0; ux < bsize; ++ux)
        OUTMSG (("%2.2x", buffer[ux]));
               

    OUTMSG ((" '%.6s'\n",buffer));
    REQUIRE (memcmp (buffer, "123456", 6) == 0);

    REQUIRE_RC (KFileRelease (enc));
    REQUIRE_RC (KFileRelease (raw));

    REQUIRE_RC (KDirectoryRelease (cwd));
}



//////////////////////////////////////////// Main

extern "C"
{

ver_t CC KAppVersion ( void )
{
    return 0x1000000;
}

rc_t CC UsageSummary (const char * prog_name)
{
    return 0;
}

rc_t CC Usage ( const Args * args)
{
    return 0;
}

const char UsageDefaultName[] = "test-kfg";

rc_t CC KMain ( int argc, char *argv [] )
{
    KConfigDisableUserSettings();
    rc_t rc = EncAppTrunc (argc, argv);
    return rc;
}

}
