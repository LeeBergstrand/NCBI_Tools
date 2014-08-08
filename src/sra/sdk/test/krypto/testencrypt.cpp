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
 * Unit tests for KEncryptFile
 */
#include <cstring>
#include <ktst/unit_test.hpp>
#include <krypto/key.h>
#include <krypto/encfile.h>
#include <krypto/encfile-priv.h>
#include <krypto/reencfile.h>
#include <kfs/impl.h>
#include <klib/out.h>
#include <kapp/args.h>
#include <kfg/config.h>

using namespace std;

TEST_SUITE(EncryptFileTestSuite);

TEST_CASE(KEncryptFileRead)
{
    OUTMSG (("\nKEncryptFileRead\n"));

//    OUTMSG (("Create base environmnet\n"));

    const char pw_a [] = "first pw";
    KKey key_a;

    REQUIRE_RC (KKeyInitUpdate (&key_a, kkeyAES128, pw_a, strlen (pw_a)));

    KDirectory * cwd;

    REQUIRE_RC(KDirectoryNativeDir (&cwd));

    const char file_a [] = "temp/file-a";
    KFile * rawfile_a;

    REQUIRE_RC (KDirectoryCreateFile (cwd, &rawfile_a, false, 0660, kcmParents|kcmInit, file_a));

    const char file_b [] = "temp/file-b";
    KFile * rawfile_b;

    REQUIRE_RC (KDirectoryCreateFile (cwd, &rawfile_b, false, 0660, kcmParents|kcmInit, file_b));

    KFile * encfile_b;

    REQUIRE_RC (KEncFileMakeWrite (&encfile_b, rawfile_b, &key_a));


    char oline [] = "0123456789\n";
    size_t linelen = strlen (oline);

    const uint64_t num_lines = 3280;

    for (uint64_t ii = 0; ii < num_lines; ++ii)
    {
        size_t num_writ;

        REQUIRE_RC (KFileWriteAll (rawfile_a, ii * linelen, oline, linelen, &num_writ));
        REQUIRE (num_writ == linelen);

        REQUIRE_RC (KFileWriteAll (encfile_b, ii * linelen, oline, linelen, &num_writ));
        REQUIRE (num_writ == linelen);
    }

    REQUIRE_RC (KFileRelease (encfile_b));
    REQUIRE_RC (KFileRelease (rawfile_b));
    REQUIRE_RC (KFileRelease (rawfile_a));

    const KFile * ra;

    REQUIRE_RC (KDirectoryOpenFileRead (cwd, &ra, file_a));

    const KFile * rb; /* CONTROL */

    REQUIRE_RC (KDirectoryOpenFileRead (cwd, &rb, file_b));

    const KFile * ea;

    REQUIRE_RC (KEncryptFileMakeRead (&ea, ra, &key_a));

    const KFile * eb;

    REQUIRE_RC (KReencFileMakeRead (&eb, rb, &key_a, &key_a));


    const size_t read_bytes = 32850;

    uint64_t z_a, z_b, z_c;

    REQUIRE_RC (KFileSize (ea, &z_a));
    REQUIRE_RC (KFileSize (rb, &z_b));
    REQUIRE_RC (KFileSize (eb, &z_c));

//     KOutMsg ("%lu %lu\n", z_a, z_b);

    REQUIRE (z_a == z_b);
    REQUIRE (z_c == z_b);

    for (uint64_t pos = 0; ; pos += read_bytes)
//    for (uint64_t pos = 9000; ; pos += read_bytes)
    {
        char buff_a [read_bytes];
        char buff_b [read_bytes];
        char buff_c [read_bytes];
        size_t read_a, read_b, read_c;

//        KOutMsg ("%lu\n", pos);
        if ((pos == 1102160) || (pos == 1069320))
            OUTMSG (("Breakpoint\n"));

        REQUIRE_RC (KFileReadAll (rb, pos, buff_b, sizeof buff_b, &read_b));
        REQUIRE_RC (KFileReadAll (ea, pos, buff_a, sizeof buff_a, &read_a));
        REQUIRE_RC (KFileReadAll (eb, pos, buff_c, sizeof buff_c, &read_c));

        REQUIRE (read_a == read_b);
        REQUIRE (read_c == read_b);

        if (read_a == 0)
            break;

//         int errs = 0;
//         bool failed;
//         if (memcmp (buff_a, buff_b, read_a) == 0)
//             OUTMSG (("pos: %lu a:c matched\n", pos));
//         else
//         {
//             OUTMSG (("pos: %lu a:b NO MATCH\n", pos));
//             for (uint64_t jj = 0; jj < read_a; ++jj)
//             {
//                 failed = ((0xFF) & buff_a[jj]) != ((0xFF) & buff_b[jj]);               
//                 if (failed)
//                 {
//                     KOutMsg ("pos: %lu a:b %2.2x : %2.2x %s\n", pos + jj, ((0xFF) & buff_a[jj]), ((0xFF) & buff_b[jj]),
//                              "FAIL");
//                     if (++errs > 10)
//                         break;
//                 }
//             }
//         }
//         if (memcmp (buff_c, buff_b, read_c) == 0)
//             OUTMSG (("pos: %lu a:b matched\n", pos));
//         else
//         {       
//             OUTMSG (("pos: %lu a:b NO MATCH\n", pos));
//             errs = 0;
//             for (uint64_t jj = 0; jj < read_c; ++jj)
//             {
//                 failed = ((0xFF) & buff_c[jj]) != ((0xFF) & buff_b[jj]);               
//                 if (failed)
//                 {
//                     KOutMsg ("pos: %lu c:b %2.2x : %2.2x %s\n", pos + jj, ((0xFF) & buff_c[jj]), ((0xFF) & buff_b[jj]),
//                              "FAIL");
//                     if (++errs > 10)
//                         break;
//                 }
//             }
//         }
//         KOutMsg ("a vs. b\n");
        if (pos + read_a <= z_a - 8)
            REQUIRE (memcmp (buff_a, buff_b, read_a) == 0);
        else
            REQUIRE (memcmp (buff_a, buff_b, read_a - 8) == 0);

//         KOutMsg ("c vs. b\n");
        if (pos + read_a <= z_a - 8)
            REQUIRE (memcmp (buff_c, buff_b, read_c) == 0);
        else
            REQUIRE (memcmp (buff_c, buff_b, read_c - 8) == 0);
    }

    REQUIRE_RC (KFileRelease (ea));
    REQUIRE_RC (KFileRelease (eb));
    REQUIRE_RC (KFileRelease (ra));
    REQUIRE_RC (KFileRelease (rb));


//    REQUIRE_RC (KDirectoryRemove (cwd, true, file_a));
//    REQUIRE_RC (KDirectoryRemove (cwd, true, file_b));
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
    rc_t rc=EncryptFileTestSuite(argc, argv);
    return rc;
}

}
