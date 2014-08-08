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

#include <kfs/directory.h>
#include <kfs/file.h>
#include <kfs/wgaencrypt.h>

#include <kxml/xml.h>

#include <string.h>
#include <assert.h>

/*
 * not intended to be pretty.
 * not intended to be slick.
 * not intended to make tools like valgrind happy.
 */
#define TEST      "wga_enc/test.txt.gz.test_enc"
#define ENCRYPT   "wga_enc/test.txt.gz.ncbi_enc"
#define UNENCRYPT "wga_enc/test.txt.gz"
#define WGA_KEY   (uint8_t*)"q"

static
rc_t run()
{
    KDirectory * pwd;
    rc_t rc;

    STSMSG (1, ("Open file system"));
    rc = KDirectoryNativeDir (&pwd);
    if (rc)
        LOGERR (klogErr, rc, "Failed to open filesystem");
    else
    {
        /* encrypt the file */
        {
            const KFile * sourcefile;   /* open source first to make sure we are in the right place */

            rc = KDirectoryOpenFileRead (pwd, &sourcefile, UNENCRYPT);
            if (rc)
                LOGERR (klogErr, rc, "Failed to open source file");
            else
            {
                KFile * encfile;

                rc = KDirectoryCreateFile (pwd, &encfile, true, 0664, kcmInit, TEST);
                if (rc)
                    LOGERR (klogErr, rc, "Failed to open encrypted output");
                else
                {
                    KFile * writefile;

                    rc = KFileMakeWGAEncWrite (&writefile, encfile, WGA_KEY, 1, 4096, fer_encAES);
                    if (rc)
                        LOGERR (klogErr, rc, "Failed to open encrypter");
                    else
                    {
                        uint64_t pos = 0;
                        size_t num_read;
                        uint64_t buffer [64*1024];

                        do
                        {
                            rc = KFileRead (sourcefile, pos, buffer, sizeof buffer, &num_read);
                            if (rc)
                                LOGERR (klogErr, rc, "Failed to read");

                            else if (num_read)
                            {
                                size_t tot_writ;
                                size_t num_writ;

                                for (tot_writ = 0; tot_writ < num_read; tot_writ += num_writ)
                                {
                                    rc = KFileWrite (writefile, pos + tot_writ, buffer + tot_writ,
                                                     num_read - tot_writ, &num_writ);
                                    if (rc)
                                    {
                                        LOGERR (klogErr, rc, "Failed to write");
                                        break;
                                    }
                                    else if (num_writ == 0)
                                    {
                                        rc = -1;
                                        break;
                                    }
                                }
                                pos += num_read;
                            }
                        } while ((rc == 0) && (num_read > 0));

                        KFileRelease (writefile);
                    }
                    KFileRelease (encfile);
                }
                KFileRelease (sourcefile);
            }
        }
        if (rc == 0)
        {
            const KFile * unencrypt;

            STSMSG (1, ("Open unencryptd file %s\n", UNENCRYPT));
            rc = KDirectoryOpenFileRead (pwd, &unencrypt, UNENCRYPT);
            if (rc)
                LOGERR (klogErr, rc, "failed to open unencryptd file");
            else
            {
                const KFile * encrypt;

                STSMSG (1, ("Open encryptd file %s\n", ENCRYPT));
                rc = KDirectoryOpenFileRead (pwd, &encrypt, ENCRYPT);
                if (rc)
                    LOGERR (klogErr, rc, "Failed to open encryptd file");
                else
                {
                    const KFile * decrypt;

                    STSMSG (1, ("Open decrypt file\n"));
                    rc = KFileMakeWGAEncRead (&decrypt, encrypt, WGA_KEY, 1);
                    if (rc)
                        LOGERR (klogErr, rc, "Failed to open decrypter");

                    else
                    {
                        size_t tot_readu;
                        size_t tot_readd;
                        size_t num_read;
                        uint8_t decoded [64 * 1024 * 24];
                        uint8_t unencoded [64 * 1024 * 24];
                        int comp;

                        memset (decoded, 0, sizeof decoded);
                        memset (unencoded, 0, sizeof unencoded);

                        for (tot_readu = 0; rc == 0 && tot_readu < sizeof unencoded; tot_readu += num_read)
                        {
                            STSMSG (5, ("Read unencrypted '%u' @ %lu\n", sizeof unencoded - tot_readu, tot_readu));
                            rc = KFileRead (unencrypt, tot_readu, unencoded + tot_readu,
                                            sizeof unencoded - tot_readu, &num_read);
                            if (num_read == 0)
                                break;

                        }
                        if (rc == 0)
                        {
                            for (tot_readd = 0; rc == 0 && tot_readd < sizeof decoded; tot_readd += num_read)
                            {

                                STSMSG (5, ("Read decrypted '%u' @ %lu\n", sizeof decoded - tot_readd, tot_readd));
                                rc = KFileRead (decrypt, tot_readd, decoded + tot_readd,
                                                sizeof decoded - tot_readd, &num_read);
                                if (num_read == 0)
                                    break;
                            }

                            comp = memcmp(decoded,unencoded, sizeof decoded);

                            STSMSG (1, ("Read u '%zu' d '%zu' cmp '%d'", tot_readu, tot_readd, comp));

                        
                            if (comp != 0)
                            {
                                rc = RC (rcExe, rcNoTarg, rcValidating, rcFile, rcInconsistent);
                                LOGERR (klogErr, rc, "Unencryptfailed");


                                {
                                    size_t ix;
                                    size_t limit;
                                    size_t matched = 0;
                                    size_t mismatched = 0;

                                    limit = tot_readu;
                                    if (limit < tot_readd)
                                        limit = tot_readd;

                                    for (ix = 0; ix < limit; ++ix)
                                    {
                                        if (decoded[ix] != unencoded[ix])
                                        {
                                            ++mismatched;
                                            STSMSG (2, ("%5.5zu: D %c %2.2X U %c %2.2X\n", 
                                                        ix,
                                                        decoded[ix]?decoded[ix]:'?',decoded[ix],
                                                        unencoded[ix]?unencoded[ix]:'?',unencoded[ix]));
                                        }
                                        else
                                            ++matched;
                                    }
                                    STSMSG (2, ("matched %zu mismatched %zu",matched,mismatched));
                                }
                            }
                        
                        }
                    }
                    KFileRelease (decrypt);
                }
                KFileRelease (encrypt);
            }
            KFileRelease (unencrypt);
        }
        if (rc == 0)
        {
            const KFile * unencrypt;

            STSMSG (1, ("Open unencryptd file %s\n", UNENCRYPT));
            rc = KDirectoryOpenFileRead (pwd, &unencrypt, UNENCRYPT);
            if (rc)
                LOGERR (klogErr, rc, "failed to open unencryptd file");
            else
            {
                const KFile * encrypt;

                STSMSG (1, ("Open encryptd file %s\n", TEST));
                rc = KDirectoryOpenFileRead (pwd, &encrypt, TEST);
                if (rc)
                    LOGERR (klogErr, rc, "Failed to open encryptd file");
                else
                {
                    const KFile * decrypt;

                    STSMSG (1, ("Open decrypt file\n"));
                    rc = KFileMakeWGAEncRead (&decrypt, encrypt, WGA_KEY, 1);
                    if (rc)
                        LOGERR (klogErr, rc, "Failed to open decrypter");

                    else
                    {
                        size_t tot_readu;
                        size_t tot_readd;
                        size_t num_read;
                        uint8_t decoded [64 * 1024 * 24];
                        uint8_t unencoded [64 * 1024 * 24];
                        int comp;

                        memset (decoded, 0, sizeof decoded);
                        memset (unencoded, 0, sizeof unencoded);

                        for (tot_readu = 0; rc == 0 && tot_readu < sizeof unencoded; tot_readu += num_read)
                        {
                            STSMSG (5, ("Read unencrypted '%u' @ %lu\n", sizeof unencoded - tot_readu, tot_readu));
                            rc = KFileRead (unencrypt, tot_readu, unencoded + tot_readu,
                                            sizeof unencoded - tot_readu, &num_read);
                            if (num_read == 0)
                                break;

                        }
                        if (rc == 0)
                        {
                            for (tot_readd = 0; rc == 0 && tot_readd < sizeof decoded; tot_readd += num_read)
                            {

                                STSMSG (5, ("Read decrypted '%u' @ %lu\n", sizeof decoded - tot_readd, tot_readd));
                                rc = KFileRead (decrypt, tot_readd, decoded + tot_readd,
                                                sizeof decoded - tot_readd, &num_read);
                                if (num_read == 0)
                                    break;
                            }

                            comp = memcmp(decoded,unencoded, sizeof decoded);

                            STSMSG (1, ("Read u '%zu' d '%zu' cmp '%d'", tot_readu, tot_readd, comp));

                        
                            if (comp != 0)
                            {
                                rc = RC (rcExe, rcNoTarg, rcValidating, rcFile, rcInconsistent);
                                LOGERR (klogErr, rc, "Unencryptfailed");


                                {
                                    size_t ix;
                                    size_t limit;
                                    size_t matched = 0;
                                    size_t mismatched = 0;

                                    limit = tot_readu;
                                    if (limit < tot_readd)
                                        limit = tot_readd;

                                    for (ix = 0; ix < limit; ++ix)
                                    {
                                        if (decoded[ix] != unencoded[ix])
                                        {
                                            ++mismatched;
                                            STSMSG (2, ("%5.5zu: D %c %2.2X U %c %2.2X\n", 
                                                        ix,
                                                        decoded[ix]?decoded[ix]:'?',decoded[ix],
                                                        unencoded[ix]?unencoded[ix]:'?',unencoded[ix]));
                                        }
                                        else
                                            ++matched;
                                    }
                                    STSMSG (2, ("matched %zu mismatched %zu",matched,mismatched));
                                }
                            }
                        
                        }
                    }
                    KFileRelease (decrypt);
                }
                KFileRelease (encrypt);
            }
            KFileRelease (unencrypt);
        }
    }
    KDirectoryRelease (pwd);
    return rc;
}

static
void summary (const char * progname)
{
    OUTMSG (("\n"
             "Usage:\n"
             "  %s [OPTIONS]\n"
             "\n"
             "Summary:\n"
             "  Test the KXTocDir type.\n",
             progname));
}
static
const char def_name[] = "test-xfs";
rc_t CC Usage (const Args * args)
{
    const char * progname = def_name;
    const char * fullpath = def_name;
    rc_t rc = 0;

    rc = ArgsProgram (args, &fullpath, &progname);
    if (rc == 0)
    {
        assert (args);
        summary (def_name);
        HelpOptionsStandard ();
    }
    return rc;
}

MINIUSAGE(def_name)


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
