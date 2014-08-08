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
#include <kfs/bzip.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <common/test_assert.h>

#if 0
#define BUFFSIZE (130 * 1024) /* should be larger than the internal buffer for KBZipFile */
#else
#define BUFFSIZE (130)
#endif
static
char outbuff [BUFFSIZE];

static
bool outbuff_initialized = false;

static
rc_t initbuff ()
{
    if (outbuff_initialized == false)
        memset (outbuff, 'A', sizeof outbuff);
    outbuff_initialized = true;
    return 0;
}


static
rc_t write (KFile * f)
{
    uint64_t tot_writ;
    size_t num_writ;
    rc_t rc;

    initbuff();

    for (tot_writ = 0; tot_writ < sizeof outbuff; tot_writ += num_writ)
    {
        rc = KFileWrite (f, tot_writ, outbuff, sizeof outbuff - tot_writ, &num_writ);
        if (rc)
            break;
    }
    return rc;
}


static const char file_one [] = "file_one.bz2";
static const char file_two [] = "file_two.bz2";
static const char file_three [] = "file_three.bz2";
static const char file_cat [] = "file_cat.bz2";

static
rc_t create_file (KDirectory * d, const char * path)
{
    KFile * f;
    rc_t rc;

    rc = KDirectoryCreateFile (d, &f, false, 0666, kcmCreate, path);
    if (rc == 0)
    {
        KFile * z;

        rc = KFileMakeBzip2ForWrite (&z, f);
        if (rc == 0)
        {
            rc = write (z);

            KFileRelease (z);
        }

        KFileRelease (f);
    }
    return rc;
}


rc_t copy (KFile * w, const KFile * r, uint64_t * wposin)
{
    uint8_t b [16 * 1024];
    uint64_t wpos;
    uint64_t rpos;
    size_t tot_writ;
    size_t num_writ;
    size_t num_read;
    rc_t rc = 0;

    for ((wpos = *wposin),(rpos = 0); rc == 0; )
    {
        rc = KFileRead (r, rpos, b, sizeof b, &num_read);
        if ((rc != 0) || (num_read == 0))
            break;

        for (tot_writ = 0; tot_writ < num_read; tot_writ += num_writ)
        {
            rc = KFileWrite (w, wpos, b + tot_writ, num_read - tot_writ, &num_writ);
            if (rc) break;
            wpos += num_writ;
        }

        rpos += num_read;
    }

    *wposin = wpos;

    return rc;
}


rc_t append (KDirectory * d, KFile * w, uint64_t *wpos, const char * path)
{
    rc_t rc;
    const KFile * r;

    rc = KDirectoryOpenFileRead (d, &r, file_one);
    if (rc == 0)
    {
        rc = copy (w, r, wpos);
        if (rc == 0)
        {
            rc = KFileRelease (r);
        }
    }
    return rc;
}


rc_t readbuff (const KFile * f, char * buff, size_t buffsize, uint64_t pos, size_t * pnum_read)
{
    rc_t rc;
    size_t tot_read;
    size_t num_read;


    for (tot_read = 0; tot_read < buffsize; tot_read += num_read)
    {
        STSMSG (2, ("reading at %lu", pos));

        rc = KFileRead (f, pos + tot_read, buff + tot_read,
                        buffsize - tot_read, &num_read);
        if ((rc != 0) || (num_read == 0))
            break;

        STSMSG (2, ("read %lu", num_read));


    }
    *pnum_read = tot_read;
    return rc;
}


rc_t validate (const KFile * f, uint64_t sz)
{
    uint64_t pos;
    size_t num_read = 0;
    char buff [111+1];  /* deliberatly not even */
    int  cmp;
    rc_t rc;

    for (pos = 0; pos < sz; pos += num_read)
    {

        STSMSG (3, ("readbuff %lu", pos));
        rc = readbuff (f, buff, sizeof buff - 1, pos, &num_read);
        buff[num_read] = '\0';
        if (rc)
            break;

        if (num_read == 0)
        {
            rc = RC (rcExe, rcFile, rcReading, rcFile, rcTooShort);
            break;
        }

        if ((cmp = memcmp (buff, outbuff, num_read)) == 0)
        {
            STSMSG (1, ("compared OK %zu\n", num_read));
        }
        else
        {
            STSMSG (1, ("Failed to compare at %lu len %zu got %d", pos, num_read, cmp));
        }
        STSMSG (1, ("'%s'", buff));
    }


    if (pos == sz)
        STSMSG (1, ("read whole file\n"));
    else
        STSMSG (1, ("Failed to read whole file %lu out of %lu\n", pos, sz));

    STSMSG (1, ("exit validate %R", rc));
    return rc;
}

static
rc_t run (void )
{
    KDirectory *wd;
    rc_t rc, orc;

    rc = KDirectoryNativeDir ( & wd );
    if ( rc != 0 )
        LOGERR(klogFatal, rc, "Failed to open file system %R");
    else
    {
        do
        {
            KFile * w;
            const KFile * r;
            const KFile * zr;
            uint64_t wpos = 0;

            STSMSG (1,("creating file one"));
            rc = create_file (wd, file_one);
            if (rc) break;

            STSMSG (1,("creating file two"));
            rc = create_file (wd, file_two);
            if (rc) break;

            STSMSG (1,("creating file three"));
            rc = create_file (wd, file_three);
            if (rc) break;

            STSMSG (1,("creating concatenated file"));
            rc = KDirectoryCreateFile (wd, &w, false, 0666, kcmCreate, file_cat);
            if (rc) break;

            do
            {
                orc = append (wd, w, &wpos, file_one);
                if (orc) break;

                orc = append (wd, w, &wpos, file_two);
                if (orc) break;

                orc = append (wd, w, &wpos, file_three);
            } while (0);

            rc = KFileRelease (w);
            if (orc || rc) break;

            STSMSG (1,("reading concatenated file"));
            rc = KDirectoryOpenFileRead (wd, &r, file_cat);
            if (rc == 0)
            {
                rc = KFileMakeBzip2ForRead (&zr, r);
                if (rc == 0)
                {
                    STSMSG (1, ("validating"));
                    orc = validate (zr, 3*BUFFSIZE);
                
                    KFileRelease (zr);
                }
                KFileRelease (r);
            }

        } while (0);
#if 1
        KDirectoryRemove (wd, true, file_one);
        KDirectoryRemove (wd, true, file_two);
        KDirectoryRemove (wd, true, file_three);
        KDirectoryRemove (wd, true, file_cat);
#endif
        KDirectoryRelease (wd);
    }
    return orc ? orc : rc;
}


rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        "  %s [OPTIONS]\n"
        "\n"
        "Summary:\n"
        "  Test the KBuffile type.  Must be run in a directory with\n"
        "  write permissionas test files will be created in a directory\n"
        "  called test-buffile (it will also be created.\n",
        progname);
}

const char UsageDefaultName[] = "test-bzip-concat";

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
