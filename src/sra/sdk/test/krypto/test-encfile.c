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
#include <krypto/encfile-priv.h>
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

static
rc_t readBuffer (const KFile * f, uint64_t pos, void * buffer, size_t bsize, size_t *pnum_read)
{
    uint8_t * bbuffer = buffer;
    size_t tot_read;
    size_t num_read;
    rc_t rc = 0;

    for (tot_read = 0; tot_read < bsize ; tot_read += num_read)
    {
        rc = KFileRead (f, pos + tot_read, bbuffer + tot_read, bsize - tot_read, &num_read);
        if (rc)
            return rc;
        if (num_read == 0)
            break;
    }
    *pnum_read = tot_read;
    return rc;
}



/*
 * tools like valgrin will hate this function.  stuff doesn't get released on failure paths.
 */
static
rc_t run()
{
    KFile * encrypted = NULL;
    KFile * encryptor = NULL;
    KDirectory * dir;
    uint8_t buff [26 + 26 + 10];
    uint8_t obuff [26 + 26 + 10];
    uint8_t ibuff [26 + 26 + 10];
    uint32_t ix, diff_count;
    size_t num_writ;
    rc_t rc;
    uint64_t pos;

    do
    {

        rc = KDirectoryNativeDir (&dir);
        if (rc)
        {
            LOGERR (klogErr, rc, "error opening pwd");
            break;
        }

        rc = KDirectoryCreateFile (dir, &encrypted, false, 0777, kcmInit,
                                   TEST_FILE_NAME);
        if (rc)
        {
            PLOGERR (klogErr,
                     (klogErr, rc, "error creating $F", "F=%s", TEST_FILE_NAME));
            break;
        }

        STSMSG (1, ("starting write test"));
        rc = KEncFileMakeWrite (&encryptor, encrypted, &key);
        if (rc)
        {
            LOGERR (klogErr, rc, "error creating encryptor object");
            break;
        }

        for (ix = 0; ix < 26; ++ix)
        {
            obuff[ix] = 3 ^ (buff[ix] = 'A' + ix);
            obuff[ix+26] = 3 ^ (buff[ix+26] = 'a' + ix);
        }
        for (ix = 0; ix < 10; ++ix)
        {
            obuff[ix+26+26] = 3 ^ (buff[ix+26+26] = '0' + ix);
        }

        for (ix = 0; ix < 10000; ++ix)
        {
            rc = KFileWrite (encryptor, ix * sizeof (buff), buff,
                             sizeof (buff), &num_writ);
            if (rc)
            {
                LOGERR (klogErr, rc, "error writing to encryptor");
                break;
            }
            if (num_writ != sizeof (buff))
            {
                rc = -1;
                LOGERR (klogErr, rc, "incomplete write");
                break;
            }
        }

        if (rc) break;

        KFileRelease (encryptor);
        KFileRelease (encrypted);

        STSMSG (1, ("starting validate test"));
        rc = KDirectoryOpenFileRead (dir, (const KFile**)&encrypted, TEST_FILE_NAME);
        if (rc)
        {
            LOGERR (klogErr, rc, "error opening new encrypted file for read");
            break;
        }

        rc = KEncFileValidate (encrypted);
        if (rc)
        {
            LOGERR (klogErr, rc, "error validating encrypted file");
            break;
        }

        KFileRelease (encrypted);

        STSMSG (1, ("starting read test"));
        rc = KDirectoryOpenFileRead (dir, (const KFile **)&encrypted, TEST_FILE_NAME);
        if (rc)
        {
            LOGERR (klogErr, rc, "error opening new encrypted file for read");
            break;
        }

        rc = KEncFileMakeRead ((const KFile**)&encryptor, encrypted, &key);
        if (rc)
        {
            LOGERR (klogErr, rc, "error making decryptor");
            break;
        }
         
        for (ix = 0; ix < 10000; ++ix)
        {
            rc = readBuffer (encryptor, ix * sizeof (buff), ibuff,
                             sizeof (ibuff), &num_writ);
            if (rc)
            {
                LOGERR (klogErr, rc, "error reading from decryptor");
                break;
            }

            if (num_writ != sizeof (ibuff))
            {
                rc = -1;
                LOGERR (klogErr, rc, "incomplete write");
                break;
            }

            if (memcmp (buff, ibuff, sizeof (buff)) != 0)
            {
                rc = -1;
                PLOGMSG (klogErr,
                         (klogErr, "error decrypting $(I) $(B) v. $(BB)",
                          "I=%u,B=%s,BB=%s", ix, buff, ibuff));
                break;
            }
        }

        if (rc) break;

        KFileRelease (encryptor);
        KFileRelease (encrypted);

#if 0
        STSMSG (1, ("starting update test"));

        rc = KDirectoryOpenFileWrite (dir, &encrypted, true, TEST_FILE_NAME);
        if (rc)
        {
            LOGERR (klogErr, rc, "unable to open encrypted file for update");
            break;
        }

        rc = KEncFileMakeUpdate (&encryptor, encrypted, &key);
        if (rc)
            break;

        for (ix = 0; ix < sizeof (ibuff); ++ix)
            ibuff[ix] ^= 5; /* just a fairly randomish change */

        /* position is around the middle of the first "page" */

        pos = sizeof (obuff) * ((16*1024)/sizeof (obuff));

        STSMSG (2, ("first update write at %lu", pos));

        rc = KFileWrite (encryptor, pos, obuff,
                         sizeof (obuff), &num_writ);
        if (rc)
        {
            LOGERR (klogErr, rc, "failed first update write");
            break;
        }

        /* posistion is where the buffer crosses the boundary between pages */
        pos = sizeof (obuff) * ((32*1024)/sizeof (obuff));

        STSMSG (2, ("second update write at %lu", pos));

        rc = KFileWrite (encryptor, pos, obuff,
                         sizeof (obuff), &num_writ);

        if (rc)
        {
            LOGERR (klogErr, rc, "failed second update write");
            break;
        }

        /* posistion is between the previous writes */

        pos = sizeof (obuff) * ((24*1024)/sizeof (obuff));

        STSMSG (2, ("third update write at %lu", pos));

        rc = KFileWrite (encryptor, pos, obuff,
                         sizeof (obuff), &num_writ);

        if (rc)
        {
            LOGERR (klogErr, rc, "failed third update write");
            break;
        }

        KFileRelease (encryptor);
        KFileRelease (encrypted);

        rc = KDirectoryOpenFileRead (dir, (const KFile**)&encrypted, TEST_FILE_NAME);
        if (rc)
        {
            LOGERR (klogErr, rc, "error opening new encrypted file for read");
            break;
        }

        rc = KEncFileValidate (encrypted);
        if (rc)
        {
            LOGERR (klogErr, rc, "error validating encrypted file");
            break;
        }

        KFileRelease (encrypted);

        rc = KDirectoryOpenFileRead (dir, (const KFile**)&encrypted, TEST_FILE_NAME);
        if (rc)
        {
            LOGERR (klogErr, rc, "error opening new encrypted file for read");
            break;
        }

        rc = KEncFileMakeRead ((const KFile**)&encryptor, encrypted, &key);
        if (rc)
        {
            LOGERR (klogErr, rc, "error making decryptor");
            break;
        }
         
        for (ix = 0; ix < 10000; ++ix)
        {
            rc = readBuffer (encryptor, ix * sizeof (buff), ibuff,
                             sizeof (ibuff), &num_writ);
            if (rc)
            {
                LOGERR (klogErr, rc, "error reading from decryptor");
                break;
            }

            if (num_writ != sizeof (ibuff))
            {
                rc = -1;
                LOGERR (klogErr, rc, "incomplete write");
                break;
            }

            diff_count = 0;
            if (memcmp (buff, ibuff, sizeof (buff)) != 0)
            {
                if (memcmp (obuff, ibuff, sizeof (obuff)) == 0)
                {
                    ++diff_count;
                }
                else
                {
                    rc = -1;
                    PLOGMSG (klogErr,
                             (klogErr, "error decrypting $(I) $(B) v. $(BB)",
                              "I=%u,B=%s,BB=%s", ix, buff, ibuff));
                    break;
                }
            }
        }

        if (diff_count != 3)
        {
            rc = -1;
            PLOGERR (klogErr, 
                     (klogErr, rc, "updates failed got $(D) changes not 3",
                      "D=%u", diff_count));
            break;
        }

        if (rc) break;

        KFileRelease (encryptor);
        KFileRelease (encrypted);
#endif
        KDirectoryRelease (dir);
    } while (0);
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
        "  Test the KCSPRng type.\n",
        progname);
}

const char UsageDefaultName [] = "test-encfile";
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

