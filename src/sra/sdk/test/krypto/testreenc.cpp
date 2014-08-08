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
 * Unit tests for KReencFile
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

#include <common/test_assert.h>

using namespace std;

TEST_SUITE(ReencFileTestSuite);

TEST_CASE(KReencFileRead)
{
    OUTMSG (("\nKReencFileRead\n"));

//    OUTMSG (("Create base environmnet\n"));

    KDirectory * cwd;

    REQUIRE_RC(KDirectoryNativeDir (&cwd));

    const char file_a [] = "file-a";
    const char file_b [] = "file-b";

    const char password_a [] = "first password";
    const char password_b [] = "second password";

    KKey key_a;
    KKey key_b;

    /* create two encrypted files from the same "source" */

    REQUIRE_RC(KKeyInitUpdate (&key_a, kkeyAES128, password_a, strlen (password_a)));
    REQUIRE_RC(KKeyInitUpdate (&key_b, kkeyAES128, password_b, strlen (password_b)));

    char obuff [127-32];

    for (size_t ix = 0; ix < sizeof obuff; ++ix)
        obuff[ix] = (char)(' ' + (char)ix);

//    OUTMSG (("Create two files\n"));

    KFile * wraw_a;
    KFile * wraw_b;

    REQUIRE_RC(KDirectoryCreateFile (cwd, &wraw_a, false, 0660, kcmInit, file_a));
    REQUIRE_RC(KDirectoryCreateFile (cwd, &wraw_b, false, 0660, kcmInit, file_b));

    KFile * wenc_a;
    KFile * wenc_b;

    REQUIRE_RC(KEncFileMakeWrite (&wenc_a, wraw_a, &key_a));
    REQUIRE_RC(KEncFileMakeWrite (&wenc_b, wraw_b, &key_b));


    for (unsigned int ix = 0; ix < 1024 * 1024; ++ix)
    {
        size_t num_writ;

//        OUTMSG (("Writing '%u' at '%lu'\n", ix, ix * sizeof obuff));

        REQUIRE_RC(KFileWriteAll (wenc_a, ix * sizeof obuff, obuff, sizeof obuff, &num_writ));
        REQUIRE (num_writ == sizeof obuff);

        REQUIRE_RC(KFileWriteAll (wenc_b, ix * sizeof obuff, obuff, sizeof obuff, &num_writ));
        REQUIRE (num_writ == sizeof obuff);
    }

    REQUIRE_RC(KFileRelease (wenc_a));
    REQUIRE_RC(KFileRelease (wenc_b));

    REQUIRE_RC(KFileRelease (wraw_a));
    REQUIRE_RC(KFileRelease (wraw_b));
    
    uint64_t file_size_a, z_b;

//   OUTMSG (("Validate the sizes of the files\n"));

    REQUIRE_RC(KDirectoryFileSize (cwd, &file_size_a, file_a));
    REQUIRE_RC(KDirectoryFileSize (cwd, &z_b, file_b));

//    OUTMSG (("File sizes file_a '%,lu' file_b '%,lu'\n", file_size_a, z_b));

    REQUIRE(file_size_a == z_b);

    uint64_t block_count;

    block_count = (file_size_a - 32) / 32832;

//    OUTMSG (("Open the files for raw read\n"));

    const KFile * rraw_a;
    const KFile * rraw_b;

    REQUIRE_RC(KDirectoryOpenFileRead (cwd, &rraw_a, file_a));
    REQUIRE_RC(KDirectoryOpenFileRead (cwd, &rraw_b, file_b));

//    OUTSTR (("Create the re-encryptor\n"));

    const KFile * renc_a;

    REQUIRE_RC(KReencFileMakeRead (&renc_a, rraw_a, &key_a, &key_b));

    char abuff [60000];
    char bbuff [60000];

    uint64_t pos;
    size_t az;
    size_t bz;

    pos = 0;

//    OUTMSG (("Straight through read and compare\n"));

    static const char zbuff[8] = {0,0,0,0,0,0,0,0};

    for (;;)
    {
        REQUIRE_RC(KFileReadAll (renc_a, pos, abuff, sizeof abuff, &az));

//             KOutStr ( "renc:");
//             for (int iii = 0; iii < 100; ++iii)
//                 KOutMsg (" %2.2x",(uint8_t)abuff[iii]);  
//             KOutStr ("\n");

//             REQUIRE(az == bz);

        REQUIRE_RC(KFileReadAll (rraw_b, pos, bbuff, sizeof bbuff, &bz));

//             KOutStr ( "rraw:");
//             for (int iii = 0; iii < 100; ++iii)
//                 KOutMsg (" %2.2x",(uint8_t)bbuff[iii]);  
//             KOutStr ("\n");
                     
        if (az == 0)
            break;

        if (az < sizeof abuff)
        {
//                 KOutMsg ("a: ");
//                 for (unsigned iii = az - 24; iii < az; ++iii)
//                     KOutMsg ("%2.2x ",(uint8_t)abuff[iii]);
//                 KOutMsg ("\n");
//                 KOutMsg ("b: ");
//                 for (unsigned iii = az - 24; iii < az; ++iii)
//                     KOutMsg ("%2.2x ",(uint8_t)bbuff[iii]);
//                 KOutMsg ("\n\n");
            REQUIRE(memcmp (abuff, bbuff, az-8) == 0);
            REQUIRE(memcmp (abuff+az-8, zbuff, 8) == 0);
        }
        else
        {
            if (memcmp (abuff, bbuff, az) != 0)
                for (unsigned iii = 0; iii < az; ++ iii)
                    if (abuff[iii] != bbuff[iii])
                        OUTMSG ((">>pos %lu %2.2x : %2.2x\n", pos + iii,
                                 (0xFF & abuff[iii]), (0xFF & bbuff[iii])));
            REQUIRE(memcmp (abuff, bbuff, az) == 0);
        }
        pos += az;
    }

    REQUIRE_RC (KFileRelease (renc_a));
    REQUIRE_RC (KFileRelease (rraw_b));
    REQUIRE_RC (KFileRelease (rraw_a));

//    OUTMSG (("Random access read and compare\n"));

    char abuff2 [100000];
    char bbuff2 [100000];

    const uint64_t skip = 5;

    for (uint64_t jx = 0; jx < skip; ++jx)
    {
//        OUTMSG (("Testing with skips start '%lu'\n",jx));

        REQUIRE_RC(KDirectoryOpenFileRead (cwd, &rraw_a, file_a));
        REQUIRE_RC(KDirectoryOpenFileRead (cwd, &rraw_b, file_b));

        REQUIRE_RC(KReencFileMakeRead (&renc_a, rraw_a, &key_a, &key_b));

        for (uint64_t ix = jx; ; ix += skip)
        {
            pos = ix * sizeof abuff2;

//            OUTMSG (("Read '%zu' at '%lu'\n", sizeof abuff2, pos));
            REQUIRE_RC(KFileReadAll (renc_a, pos, abuff2, sizeof abuff2, &az));

#ifdef _WIN32
            if (az == 0)
            {
                REQUIRE_GE(pos, z_b);
                break;
            }
#endif

            REQUIRE_RC(KFileReadAll (rraw_b, pos, bbuff2, sizeof bbuff2, &bz));

//            OUTMSG (("Read  '%zu' and '%zu'\n", az, bz));

            REQUIRE(az == bz);

            if (az == 0)
                break;

//            OUTMSG (( "az '%zu' bz '%zu'\n",az,bz));

//                 if (az < sizeof abuff2)
//                 {
//                     KOutMsg ("a: ");
//                     for (unsigned iii = az - 24; iii < az; ++iii)
//                         KOutMsg ("%2.2x ",(uint8_t)abuff2[iii]);
//                     KOutMsg ("\n\n");
//                     KOutMsg ("b: ");
//                     for (unsigned iii = az - 24; iii < az; ++iii)
//                         KOutMsg ("%2.2x ",(uint8_t)bbuff2[iii]);
//                     KOutMsg ("\n\n");
//                 }

            if (az == sizeof (abuff2))
                REQUIRE (memcmp (abuff2, bbuff2, az) == 0);
            else
            {
                REQUIRE (memcmp (abuff2, bbuff2, az - 8) == 0);
                REQUIRE (memcmp (abuff2+az-8, zbuff, 8) == 0);
            }
        }

        REQUIRE_RC (KFileRelease (renc_a));
        REQUIRE_RC (KFileRelease (rraw_b));
        REQUIRE_RC (KFileRelease (rraw_a));

    }

//         KOutStr ("Release KReencFileRead\n");

    REQUIRE_RC (KDirectoryRemove (cwd, true, file_a));
    REQUIRE_RC (KDirectoryRemove (cwd, true, file_b));
}

TEST_CASE(KReencFileWriteDoubleHeader)
{
    KOutMsg ("\nKReencFileWriteDoubleHeader\n");

    KDirectory * cwd;

    REQUIRE_RC(KDirectoryNativeDir (&cwd));

    const char file_a [] = "file-a";
    const char file_b [] = "file-b";

    const char password_a [] = "first password";
    const char password_b [] = "second password";

    KKey key_a;
    KKey key_b;

    /* create two encrypted files from the same "source" */

    REQUIRE_RC(KKeyInitUpdate (&key_a, kkeyAES128, password_a, strlen (password_a)));
    REQUIRE_RC(KKeyInitUpdate (&key_b, kkeyAES128, password_b, strlen (password_b)));

    KFile * wraw_a;
    KFile * wraw_b;

//    KOutMsg ("Create two files\n");

    REQUIRE_RC(KDirectoryCreateFile (cwd, &wraw_a, false, 0660, kcmInit, file_a));
    REQUIRE_RC(KDirectoryCreateFile (cwd, &wraw_b, false, 0660, kcmInit, file_b));

    KFile * wenc_a;
    KFile * wenc_b;

    REQUIRE_RC(KEncFileMakeWrite (&wenc_a, wraw_a, &key_a));
    REQUIRE_RC(KEncFileMakeWrite (&wenc_b, wraw_b, &key_b));

    char obuff [127-32];

    for (size_t ix = 0; ix < sizeof obuff; ++ix)
        obuff[ix] = (char)(' ' + (char)ix);

// error only showed up with small files
//    for (unsigned int ix = 0; ix < 1024 * 1024; ++ix)
    for (unsigned int ix = 0; ix < 1; ++ix)
    {
        size_t num_writ;

//            KOutMsg ("Writing '%u' at '%lu'\n", ix, ix * sizeof obuff);

        REQUIRE_RC(KFileWriteAll (wenc_a, ix * sizeof obuff, obuff, sizeof obuff, &num_writ));
        REQUIRE (num_writ == sizeof obuff);

        REQUIRE_RC(KFileWriteAll (wenc_b, ix * sizeof obuff, obuff, sizeof obuff, &num_writ));
            REQUIRE (num_writ == sizeof obuff);
    }


    REQUIRE_RC(KFileRelease (wenc_a));
    REQUIRE_RC(KFileRelease (wenc_b));

    REQUIRE_RC(KFileRelease (wraw_a));
    REQUIRE_RC(KFileRelease (wraw_b));

    uint64_t file_size_a, file_size_b;

    REQUIRE_RC(KDirectoryFileSize (cwd, &file_size_a, file_a));
    REQUIRE_RC(KDirectoryFileSize (cwd, &file_size_b, file_b));

//    KOutMsg ("File sizes file_a '%,lu' file_b '%,lu'\n", file_size_a, file_size_b);

    REQUIRE (file_size_a == file_size_b);

    uint64_t block_count;

    block_count = (file_size_a - 32) / 32832;

    const KFile * rraw_a;
    const KFile * rraw_b;

    REQUIRE_RC(KDirectoryOpenFileRead (cwd, &rraw_a, file_a));
    REQUIRE_RC(KDirectoryOpenFileRead (cwd, &rraw_b, file_b));

    const KFile * renc_a;

//    KOutStr ("Make KReencFileRead\n");

    REQUIRE_RC(KReencFileMakeRead (&renc_a, rraw_a, &key_a, &key_b));

    char abuff [60000];
    char bbuff [60000];
    char cbuff [60000];

    size_t az;
    size_t bz;
    size_t cz;


    KOutStr ("Read from position 0 twice\n");

    REQUIRE_RC(KFileReadAll (rraw_b, 0, bbuff, sizeof bbuff, &bz));
    REQUIRE_RC(KFileReadAll (renc_a, 0, abuff, sizeof abuff, &az));
    REQUIRE_RC(KFileReadAll (renc_a, 0, cbuff, sizeof cbuff, &cz));

//    KOutMsg ("a %zu b %zu c %zu\n", az, bz, cz);

    REQUIRE (az == bz);
    REQUIRE (cz == bz);

    // kludgomatic ignoring of the rc_checksum 8 bytes
    if (az < sizeof abuff)
    {
        az -= 8;
        REQUIRE (memcmp (abuff, cbuff, az) == 0);
        REQUIRE (memcmp (abuff, bbuff, az) == 0);
    }
    else
    {
        REQUIRE (memcmp (abuff, cbuff, az) == 0);
        REQUIRE (memcmp (abuff, bbuff, az) == 0);
    }

    REQUIRE_RC (KFileRelease (renc_a));
    REQUIRE_RC (KFileRelease (rraw_b));
    REQUIRE_RC (KFileRelease (rraw_a));

    REQUIRE_RC (KDirectoryRemove (cwd, true, file_a));
    REQUIRE_RC (KDirectoryRemove (cwd, true, file_b));
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
    rc_t rc=ReencFileTestSuite(argc, argv);
    return rc;
}

}
