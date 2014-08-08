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
#include <kfs/buffile.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <common/test_assert.h>


#define SUBDIR "test-buffile"
#define MAXWRITE 8192
#define MAXREAD 8192
#if 1
#define FSIZE 3*256*1024*1024
#else
#define FSIZE 5L*1024*1024*1024
#endif

#define TEST_FWRITE 0

static
uint8_t g_in_buffer [MAXREAD];

/* cases [ an "x" indicates that the corresponding code has been exercised ]
 *    0. empty
 *    1. small read 100% within buffer
 *    2. small read ( single buffer ) overlapping buffer on left
 *    3. small read overlapping buffer on right
 *    4. large read ( multiple buffers ) overlapping buffer on right
 *    5. large read overlapping buffer on left
 *    6. small read with no intersection
 *    7. large read with buffer 100% within data
 *    8. large read with no intersection
 *    9. read beyond eof
 */
static
rc_t read_file ( const KFile *file, uint64_t pos, uint64_t eof, size_t to_read, size_t *num_read )
{
    rc_t rc = KFileRead ( file, pos, g_in_buffer, to_read, num_read );
    if ( rc == 0 && * num_read != to_read )
    {
        if ( pos + * num_read != eof )
            rc = RC ( rcExe, rcFile, rcReading, rcTransfer, rcIncomplete );
    }
    return rc;
}

static
rc_t random_read_from_file ( const KFile * file, size_t bsize )
{
    size_t num_read;
    size_t to_read, max_read;
    uint64_t total_read, size;

    rc_t rc = KFileSize ( file, & size );
    if ( rc != 0 )
        return rc;

    to_read = bsize;
    max_read = bsize + bsize + ( bsize >> 1 );
    if ( to_read > MAXREAD )
        to_read = MAXREAD;
    if ( max_read > MAXREAD )
        to_read = MAXREAD;

    /* read small windows around a position */
    for ( total_read = 0; rc == 0 && total_read < size; total_read += bsize )
    {
        /* case 0: empty
           or case 6: small read with no intersection */
        rc = read_file ( file, total_read, size, to_read / 2, & num_read );

        /* case 1: small read 100% within buffer */
        if ( rc == 0 )
            rc = read_file ( file, total_read + to_read / 4, size, to_read / 2, & num_read );

        /* case 2: small read ( single buffer ) overlapping buffer on left */
        if ( rc == 0 )
            rc = read_file ( file, total_read + to_read / 2 + 2, size, to_read / 2, & num_read );

        /* case 3: small read overlapping buffer on right */
        if ( rc == 0 && total_read >= bsize )
            rc = read_file ( file, total_read + to_read / 2 + 2, size, to_read / 2, & num_read );

        /* case 4: large read ( multiple buffers ) overlapping buffer on right */
        if ( rc == 0 && total_read >= max_read )
            rc = read_file ( file, total_read - max_read + 2, size, max_read, & num_read );

        /* case 6: small read with no intersection */
        if ( rc == 0 )
            rc = read_file ( file, total_read + to_read / 4, size, to_read / 2, & num_read );

        /* case 5: large read overlapping buffer on left */
        if ( rc == 0 && total_read >= 2 )
            rc = read_file ( file, total_read + to_read / 2 + 2, size, max_read / 2, & num_read );

        /* case 7: large read with buffer 100% within data */
        if ( rc == 0 && total_read >= 2 )
        {
            rc = read_file ( file, total_read, size, 1, & num_read );
            if ( rc == 0 )
                rc = read_file ( file, total_read - 2, size, max_read, & num_read );
        }

        /* case 8: large read with no intersection
           or case 9: read beyond eof */
        if ( rc == 0 )
            rc = read_file ( file, total_read + bsize * 4 + 2, size, max_read, & num_read );
    }

    return 0;
}


static
rc_t test_random_unbuffered_read ( const KDirectory *dir, size_t bsize )
{
    rc_t rc;
    const KFile * fin;

    KOutMsg ( "KBufFile random read test - unbuffered file..." );

    rc = KDirectoryOpenFileRead (dir, &fin, "unbuffered");
    if ( rc != 0 )
        LOGERR (klogFatal, rc, "\nFailed to open unbuffered file");
    else
    {
        rc = random_read_from_file (fin, bsize);
        if ( rc == 0 )
            rc = random_read_from_file (fin, bsize);
        if ( rc != 0 )
            LOGERR (klogFatal, rc, "\nFailed to read unbuffered input");
        else
            KOutMsg ( " complete.\n" );

        KFileRelease (fin);
    }

    return rc;
}

static
rc_t test_random_buffered_read ( const KDirectory *dir, size_t bsize )
{
    rc_t rc;
    const KFile * fin;

    KOutMsg ( "KBufFile random read test - buffered file..." );

    rc = KDirectoryOpenFileRead (dir, &fin, "unbuffered");
    if ( rc != 0 )
        LOGERR (klogFatal, rc, "\nFailed to open unbuffered file");
    else
    {
        const KFile * bufin;
        rc = KBufFileMakeRead ( & bufin, fin, bsize );
        if ( rc != 0 )
            LOGERR (klogFatal, rc, "\nFailed to buffer input file");
        else
        {
            rc = random_read_from_file (bufin, bsize);
            if ( rc == 0 )
                rc = random_read_from_file (bufin, bsize);
            if ( rc != 0 )
                LOGERR (klogFatal, rc, "\nFailed to read buffered input");
            else
                KOutMsg ( " complete.\n" );

            KFileRelease ( bufin );
        }


        KFileRelease (fin);
    }

    return rc;
}

static
rc_t random_test_read ( const KDirectory *dir )
{
    time_t t0 = time ( NULL );
    rc_t rc = test_random_unbuffered_read ( dir, 256 );
    if ( rc == 0 )
    {
        time_t t1 = time ( NULL );
        rc = test_random_buffered_read ( dir, 256 );
        if ( rc == 0 )
        {
            time_t t2 = time ( NULL );
            KOutMsg ("KBufFile random read test done:\n"
                     "         unbuffered time %lu\n"
                     "  256-byte buffered time %lu\n"
                     , t1-t0
                     , t2-t1
                );
        }
    }
    return rc;
}

static
rc_t read_from_file ( const KFile * file)
{
    uint64_t total_read, size;
    size_t num_read, to_read;

    rc_t rc = KFileSize ( file, & size );
    if ( rc != 0 )
        return rc;

    for ( to_read = 0, total_read = 0; total_read < size; total_read += num_read )
    {
        if (++ to_read >= MAXREAD)
            to_read = 1;

        rc = KFileRead (file, total_read, g_in_buffer, to_read, &num_read);
        if ( rc != 0 )
            return rc;
        if ( num_read == 0 )
            break;
    }

    return 0;
}


static
rc_t test_unbuffered_read ( const KDirectory *dir )
{
    rc_t rc;
    const KFile * fin;

    KOutMsg ( "KBufFile sequential read test - unbuffered file..." );

    rc = KDirectoryOpenFileRead (dir, &fin, "unbuffered");
    if ( rc != 0 )
        LOGERR (klogFatal, rc, "\nFailed to open unbuffered file");
    else
    {
        rc = read_from_file (fin);
        if ( rc == 0 )
            rc = read_from_file (fin);
        if ( rc != 0 )
            LOGERR (klogFatal, rc, "\nFailed to read unbuffered input");
        else
            KOutMsg ( " complete.\n" );

        KFileRelease (fin);
    }

    return rc;
}

/* cases [ an "x" indicates that the corresponding code has been exercised ]
 *  x 0. empty
 *  x 1. small read 100% within buffer
 *  x 2. small read ( single buffer ) overlapping buffer on left
 *  x 3. large read ( multiple buffers ) overlapping buffer on left
 *    4. small read overlapping buffer on right
 *    5. large read overlapping buffer on right
 *    6. small read with no intersection
 *    7. large read with no intersection
 *    8. large read with buffer 100% within data
 */
static
rc_t test_buffered_read ( const KDirectory *dir, size_t bsize )
{
    rc_t rc;
    const KFile * fin;

    KOutMsg ( "KBufFile sequential read test - %u byte buffered file...", ( unsigned ) bsize );

    rc = KDirectoryOpenFileRead (dir, &fin, "unbuffered");
    if ( rc != 0 )
        LOGERR (klogFatal, rc, "Failed to open buffered file");
    else
    {
        const KFile * bufin;
        rc = KBufFileMakeRead ( & bufin, fin, bsize );
        if ( rc != 0 )
            LOGERR (klogFatal, rc, "\nFailed to buffer input file");
        else
        {
            rc = read_from_file (bufin);
            if ( rc == 0 )
                rc = read_from_file (bufin);
            if ( rc != 0 )
                LOGERR (klogFatal, rc, "\nFailed to read buffered input");
            else
                KOutMsg ( " complete.\n" );

            KFileRelease ( bufin );
        }

        KFileRelease (fin);
    }

    return rc;
}

static
rc_t sequential_test_read ( const KDirectory *dir )
{
    time_t t0 = time ( NULL );
    rc_t rc = test_unbuffered_read ( dir );
    if ( rc == 0 )
    {
        time_t t1 = time ( NULL );
        rc = test_buffered_read ( dir, 256 );
        if ( rc == 0 )
        {
            time_t t2 = time ( NULL );
            rc = test_buffered_read ( dir, 256 * 1024 );
            if ( rc == 0 )
            {
                time_t t3 = time ( NULL );
                KOutMsg ("KBufFile sequential read test done:\n"
                         "         unbuffered time %lu\n"
                         "  256-byte buffered time %lu\n"
                         " 256K-byte buffered time %lu\n"
                         , t1-t0
                         , t2-t1
                         , t3-t2
                    );
            }
        }
    }
    return rc;
}

static
rc_t test_read ( const KDirectory *dir )
{
    rc_t rc = sequential_test_read ( dir );
    if ( rc == 0 )
        rc = random_test_read ( dir );
    return rc;
}

static
rc_t create_file (KDirectory * dir, KFile ** file, const char * path, ... )
{
    rc_t rc;
    va_list args;

    va_start ( args, path );
    rc = KDirectoryVCreateFile (dir, file, false, 0777, kcmInit, path, args);
    va_end ( args );

    return rc;
}

static
uint8_t Gbuffer [MAXWRITE];

static
void init_buffer (void)
{
    int ix;
    int max = 'Z' - 'A';

    for (ix = 0; ix < MAXWRITE; ++ix)
    {
        Gbuffer[ix] = 'A' + ix % max;
    }
}

static
rc_t write_to_file (KFile * file, uint64_t size, time_t *fgtime)
{
    uint64_t total_out;
    size_t num_writ, to_write;

    time_t t0 = time ( NULL );

    for ( to_write = 0, total_out = 0; total_out < size; total_out += num_writ )
    {
        rc_t rc;

        if (++ to_write >= MAXWRITE)
            to_write = 1;

        rc = KFileWrite (file, total_out, Gbuffer, to_write, &num_writ);
        if (rc)
            return rc;
    }

    * fgtime = time ( NULL ) - t0;

    return 0;
}

#if TEST_FWRITE
static
rc_t write_to_FILE (FILE * file, uint64_t size, time_t *fgtime)
{
    uint64_t total_out;
    size_t num_writ, to_write;

    time_t t0 = time ( NULL );

    for ( to_write = 0, total_out = 0; total_out < size; total_out += num_writ )
    {
        if (++ to_write >= MAXWRITE)
            to_write = 1;

        num_writ = fwrite(Gbuffer, 1, to_write, file);
        if ( num_writ != to_write )
            return RC ( rcExe, rcFile, rcWriting, rcTransfer, rcIncomplete );
    }

    * fgtime = time ( NULL ) - t0;

    return 0;
}
#endif

static
rc_t test_unbuffered_write ( KDirectory *dir, uint64_t size, time_t *fgtime )
{
    rc_t rc;
    KFile * fout;

    KOutMsg ( "KBufFile sequential write test - unbuffered file..." );

    rc = create_file (dir, &fout, "unbuffered");
    if ( rc != 0 )
        LOGERR (klogFatal, rc, "\nFailed to create unbuffered file");
    else
    {
        time_t t0, t1;
        rc = write_to_file (fout, size, & t0);
        if ( rc == 0 )
            rc = write_to_file (fout, size, & t1);
        if ( rc != 0 )
            LOGERR (klogFatal, rc, "\nFailed to write unbuffered output");
        else
        {
            KOutMsg ( " complete.\n" );
            * fgtime = t0 + t1;
        }

        KFileRelease (fout);
    }

    return rc;
}

/* cases [ an "x" indicates that the corresponding code has been exercised ]
 *  x 0. empty
 *  x 1. small write 100% within buffer
 *  x 2. small write ( single buffer ) overlapping buffer on left
 *    3. large write ( multiple buffers ) overlapping buffer on left
 *    4. small write overlapping buffer on right
 *    5. large write overlapping buffer on right
 *    6. small write with no intersection
 *  x 7. large write with no intersection
 *    8. large write with buffer 100% within data
 */
static
rc_t test_buffered_write ( KDirectory *dir, uint64_t fsize, size_t bsize, time_t *fgtime )
{
    rc_t rc;
    KFile * fout;

    KOutMsg ( "KBufFile sequential write test - %u byte buffered file...", ( unsigned ) bsize );

    rc = create_file (dir, &fout, "buffered-%u", ( unsigned ) bsize );
    if ( rc != 0 )
        LOGERR (klogFatal, rc, "Failed to create buffered file");
    else
    {
        KFile * bufout;
        rc = KBufFileMakeWrite ( & bufout, fout, false, bsize );
        if ( rc != 0 )
            LOGERR (klogFatal, rc, "\nFailed to buffer output file");
        else
        {
            time_t t0, t1;
            rc = write_to_file (bufout, fsize, & t0);
            if ( rc == 0 )
                rc = write_to_file (bufout, fsize, & t1);
            if ( rc != 0 )
                LOGERR (klogFatal, rc, "\nFailed to write buffered output");
            else
            {
                KOutMsg ( " complete.\n" );
                * fgtime = t0 + t1;
            }

            KFileRelease ( bufout );
        }

        KFileRelease (fout);
    }

    return rc;
}

#if TEST_FWRITE
static
rc_t test_buffered_fwrite ( KDirectory *dir, uint64_t fsize, time_t *fgtime )
{
    rc_t rc;
    FILE * fout;
    char path [ 4096 ];

    KOutMsg ( "FILE write test - buffered file..." );

    rc = KDirectoryResolvePath ( dir, true, path, sizeof path, "buffered-FILE" );
    if ( rc == 0 )
    {
        fout = fopen ( path, "w" );
        if ( fout == NULL )
            rc = RC ( rcExe, rcFile, rcOpening, rcNoObj, rcUnknown );
    }
    if ( rc != 0 )
        LOGERR (klogFatal, rc, "Failed to create buffered file");
    else
    {
        time_t t0, t1;
        rc = write_to_FILE (fout, fsize, & t0);
        if ( rc == 0 )
        {
            fseek ( fout, 0, SEEK_SET );
            rc = write_to_FILE (fout, fsize, & t1);
            if ( rc != 0 )
                LOGERR (klogFatal, rc, "\nFailed to write buffered output");
            else
            {
                KOutMsg ( " complete.\n" );
                * fgtime = t0 + t1;
            }
        }

        fclose (fout);
    }

    return rc;
}
#endif

static
rc_t sequential_test_write_compare ( const KFile *std, const KFile *buff )
{
    rc_t rc;
    size_t num_read;
    uint64_t std_eof, buff_eof, total;

    size_t buff_size = sizeof g_in_buffer / 2;
    uint8_t * std_buff = g_in_buffer;
    uint8_t * buff_buff = std_buff + buff_size;

    /* get the logical file sizes */
    rc = KFileSize ( std, & std_eof );
    if ( rc == 0 )
        rc = KFileSize ( buff, & buff_eof );
    if ( rc != 0 )
    {
        LOGERR ( klogFatal, rc, "failed to obtain file sizes for comparison" );
        return rc;
    }

    if ( std_eof != buff_eof )
    {
        LOGMSG ( klogFatal, "file sizes differ for comparison" );
        if ( std_eof > buff_eof )
            std_eof = buff_eof;
        rc = RC ( rcExe, rcFile, rcValidating, rcRange, rcIncorrect );
    }

    /* run a comparison */
    for ( total = 0; total < std_eof; total += num_read )
    {
        rc_t rc2;

        size_t to_read = buff_size;
        if ( total + to_read > std_eof )
            to_read = ( size_t ) ( std_eof - total );

        rc2 = KFileRead ( std, total, std_buff, to_read, & num_read );
        if ( rc2 == 0 && num_read != to_read )
            rc2 = RC ( rcExe, rcFile, rcReading, rcTransfer, rcIncomplete );
        if ( rc2 != 0 )
        {
            LOGERR ( klogFatal, rc2, "failed to read std file for comparison" );
            return rc2;
        }

        rc2 = KFileRead ( buff, total, buff_buff, to_read, & num_read );
        if ( rc2 == 0 && num_read != to_read )
            rc2 = RC ( rcExe, rcFile, rcReading, rcTransfer, rcIncomplete );
        if ( rc2 != 0 )
        {
            LOGERR ( klogFatal, rc2, "failed to read buff file for comparison" );
            return rc2;
        }

        if ( memcmp ( std_buff, buff_buff, num_read ) != 0 )
        {
            rc2 = RC ( rcExe, rcFile, rcValidating, rcData, rcCorrupt );
            LOGERR ( klogFatal, rc2, "failed to read buff file for comparison" );
            return rc2;
        }
    }

    return rc;
}

static
rc_t sequential_test_write_accuracy_buff ( const KDirectory *dir, const KFile *std, const char *path )
{
    const KFile * buff;
    rc_t rc = KDirectoryOpenFileRead ( dir, & buff, path );
    if ( rc != 0 )
        LOGERR ( klogFatal, rc, "failed to open buffered file for comparison" );
    else
    {
        rc = sequential_test_write_compare ( std, buff );
        KFileRelease ( buff );
    }
    return rc;
}


static
rc_t sequential_test_write_accuracy ( const KDirectory *dir )
{
    const KFile * std;
    rc_t rc = KDirectoryOpenFileRead ( dir, & std, "unbuffered" );
    if ( rc != 0 )
        LOGERR ( klogFatal, rc, "failed to open unbuffered file for comparison" );
    else
    {
        KOutMsg ( "testing sequential write accuracy of 256 byte buffer..." );
        rc = sequential_test_write_accuracy_buff ( dir, std, "buffered-256" );
        if ( rc != 0 )
            LOGERR ( klogFatal, rc, " failed\n");
        else
        {
            KOutMsg ( " complete\n" );
            KOutMsg ( "testing sequential write accuracy of 256K buffer..." );
            rc = sequential_test_write_accuracy_buff ( dir, std, "buffered-262144" );
            if ( rc != 0 )
                LOGERR ( klogFatal, rc, " failed\n");
            else
                KOutMsg ( " complete\n" );
        }
        KFileRelease ( std );
    }
    return rc;
}

static
rc_t sequential_test_write ( KDirectory *dir, uint64_t fsize )
{
    time_t fg0, t0 = time ( NULL );
    rc_t rc = test_unbuffered_write ( dir, fsize, & fg0 );
    if ( rc == 0 )
    {
        time_t fg1, t1 = time ( NULL );
        rc = test_buffered_write ( dir, fsize, 256, & fg1 );
        if ( rc == 0 )
        {
            time_t fg2, t2 = time ( NULL );
            rc = test_buffered_write ( dir, fsize, 256 * 1024, & fg2 );
            if ( rc == 0 )
            {
                time_t t3 = time ( NULL );
                KOutMsg ("KBufFile sequential write test done:\n"
                         "         unbuffered time %lu ( %lu in foreground )\n"
                         "  256-byte buffered time %lu ( %lu in foreground )\n"
                         " 256K-byte buffered time %lu ( %lu in foreground )\n"
                         , t1-t0, fg0
                         , t2-t1, fg1
                         , t3-t2, fg2
                    );

                rc = sequential_test_write_accuracy ( dir );
            }
        }
    }
    return rc;
}

#if TEST_FWRITE
static
rc_t test_fwrite ( KDirectory *dir, uint64_t fsize )
{
    time_t fg0, t0 = time ( NULL );
    rc_t rc = test_buffered_fwrite ( dir, fsize, & fg0 );
    if ( rc == 0 )
    {
        time_t t1 = time ( NULL );
        KOutMsg ("FILE write test done:\n"
                 " buffered time %lu ( %lu in foreground )\n"
                 , t1-t0, fg0
            );
    }
    return rc;
}
#endif

static
rc_t test_write ( KDirectory *dir, uint64_t fsize )
{
    return sequential_test_write ( dir, fsize );
}

static
rc_t run ( uint64_t fsize )
{
    KDirectory *wd;
    rc_t rc = KDirectoryNativeDir ( & wd );
    if ( rc != 0 )
        LOGERR(klogFatal, rc, "Failed to open file system %R");
    else
    {
        rc = KDirectoryCreateDir ( wd, 0777, kcmInit, SUBDIR );
        if ( rc != 0 )
            LOGERR (klogFatal, rc, "Failed to created/init test directory");
        else
        {
            KDirectory *dir;
            rc = KDirectoryOpenDirUpdate ( wd, & dir, false, SUBDIR );
            if ( rc != 0 )
                LOGERR (klogFatal, rc, "Failed to open test directory");
            else
            {
                rc = test_write ( dir, fsize );

                if ( rc == 0 )
                    rc = test_read ( dir );
#if TEST_FWRITE
                if ( rc == 0 )
                {
                    KDirectoryClearDir ( dir, true, "." );
                    rc = test_fwrite ( dir, fsize );
                }
#endif
                KDirectoryRelease ( dir );

                KDirectoryRemove ( wd, true, SUBDIR );
            }
        }

        KDirectoryRelease ( wd );
    }

    return rc;
}


const char UsageDefaultName[] = "test-buffile.c";
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
    rc_t rc = ArgsMakeAndHandle (&args, argc, argv, 0);
    if (rc == 0)
    {
        uint64_t fsize = FSIZE;

        init_buffer();

        rc = run ( fsize );
    }

    if (rc)
        LOGERR (klogErr, rc, "Exiting status");
    else
        STSMSG (0, ("Exiting status (%R)\n", rc));
    return rc;
}
