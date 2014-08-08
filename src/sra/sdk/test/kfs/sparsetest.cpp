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
* Unit tests for KRamFile
*/
#include <cstring>
#include <ktst/unit_test.hpp>
#include <kfs/ramfile.h>
#include <kfs/impl.h>
#include <klib/out.h>
#include <kapp/args.h>

#include <tchar.h>
#include <Windows.h>


/* CHEATING! */
#include "../../libs/kfs/win/sysfile-priv.h"

#include <common/test_assert.h>

using namespace std;

TEST_SUITE(SparseTestSuite);

TEST_CASE(SparseCreate)
{
    KOutMsg ("SparseCreate\n");

    TCHAR volname [MAX_PATH+1];
    TCHAR voltype [MAX_PATH+1];
    DWORD volflags;

    GetVolumeInformation (_T("C:\\"), volname, sizeof volname, NULL, NULL, &volflags, voltype, sizeof voltype);

    cout << "\n==========\n" << volname << "\n" << voltype << "\n" << hex << volflags << "\n"
         << ( volflags & FILE_SUPPORTS_SPARSE_FILES) << "\n";

    // test is dead if we can't create a sparse file at all
    REQUIRE ((volflags & FILE_SUPPORTS_SPARSE_FILES) == FILE_SUPPORTS_SPARSE_FILES);

    KDirectory * cwd;

    REQUIRE_RC(KDirectoryNativeDir (&cwd));

    TCHAR win_file_name [MAX_PATH + 18];
    GetTempPath(MAX_PATH, win_file_name);
    _tcscat(win_file_name, _T("\\sparse-test-file"));

    char file_name [MAX_PATH + 18] = "/";
    for (size_t i = 0, j = 1;  i < MAX_PATH + 18;  ++i) {
        switch (win_file_name[i]) {
        case _T(':'):
            break;
        case _T('\\'):
            file_name[j++] = '/';
            break;
        default:
            file_name[j++] = (char) win_file_name[i];
            break;
        }
        if (win_file_name[i] == _T('\0')) {
            file_name[j] = '\0';
            break;
        }
    }

    KFile * wfile;

    KOutMsg ("Ignore the next error if there is one\n");
    KDirectoryRemove (cwd, true, file_name);
    KOutMsg ("done ignoring\n");

    REQUIRE_RC (KDirectoryCreateFile (cwd, &wfile, false, 0660, kcmInit, file_name));

    volatile KSysFile * swfile = (volatile KSysFile*)wfile;

    KOutMsg ("As KSysFile %d %d should both be false \n", swfile->failed_set_sparse, swfile->is_sparse);

    uint64_t empty_size;
    REQUIRE_RC (KFileSize (wfile, &empty_size));
    REQUIRE (empty_size == 0);
    const uint64_t size_for_set = 2 * 1024 * 1024;
    REQUIRE_RC (KFileSetSize (wfile, size_for_set));

    KOutMsg ("\nAs KSysFile %d %d should be false and true\n", swfile->failed_set_sparse, swfile->is_sparse);

    BY_HANDLE_FILE_INFORMATION info;

    GetFileInformationByHandle (swfile->handle, &info);

    KOutMsg ("is sparse if not 0 '%u'\n", info.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE);

 
    uint64_t set_size;

    REQUIRE_RC (KFileSize (wfile, &set_size));

    KOutMsg ("set_size %lu size_for_set %lu\n", set_size, size_for_set);

    REQUIRE (set_size == size_for_set);

    uint64_t dir_size;

    REQUIRE_RC (KDirectoryFileSize (cwd, &dir_size, file_name));


    DWORD lo_comp_size,hi_comp_size;
    uint64_t comp_size;


    lo_comp_size = GetCompressedFileSize (win_file_name, &hi_comp_size);
    comp_size = lo_comp_size + (((uint64_t)hi_comp_size) << 32);
    if (lo_comp_size == 4294967295)
    {
        DWORD le = GetLastError();
        KOutMsg ("error: %u\n", (uint32_t)le);
    }
    else
    {
        KOutMsg ("comp_size in parts %u %u\n", lo_comp_size, hi_comp_size);
        KOutMsg ("comp %lu phys %lu\n", comp_size, dir_size);
    }

    uint64_t pos;
    char b [1024];
    char z [1024];

    memset (b, '\377', sizeof b);
    memset (z, '\0', sizeof z);

    REQUIRE_RC (KFileRelease (wfile));

    for (int64_t pos = size_for_set - sizeof b; pos >= 0; pos -= sizeof b)
    {
        size_t writ;

//        KOutMsg ("pos = %lu\n", pos);

        REQUIRE_RC (KDirectoryOpenFileWrite (cwd, &wfile, true, file_name));
        REQUIRE_RC (KFileWriteAll (wfile, pos, b, sizeof b, &writ));
        swfile = (volatile KSysFile*)wfile;
    GetFileInformationByHandle (swfile->handle, &info);

    KOutMsg ("is sparse if not 0 '%u'\n", info.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE);

        REQUIRE (writ == sizeof b);

        REQUIRE_RC (KDirectoryFileSize (cwd, &dir_size, file_name));

        lo_comp_size = GetCompressedFileSize (win_file_name, &hi_comp_size);
        comp_size = lo_comp_size + (((uint64_t)hi_comp_size) << 32);
         KOutMsg ("comp_size in parts %u %u\n", lo_comp_size, hi_comp_size);
         KOutMsg ("As KSysFile %d %d should be false and true\n", swfile->failed_set_sparse, swfile->is_sparse);
         KOutMsg ("comp %lu phys %lu\n", comp_size, dir_size);
        

        REQUIRE_RC (KFileRelease (wfile));
        const KFile * rfile;

        REQUIRE_RC (KDirectoryOpenFileRead (cwd, &rfile, file_name));
//        KOutMsg ("Comparing zero range\n");
        for (uint64_t ii = 0; ii < pos; ii += sizeof b)
        {
            char bb [sizeof b];
            size_t read;

//            KOutMsg ("zero ii = %lu\n", ii);

            REQUIRE_RC (KFileReadAll (rfile, ii, bb, sizeof bb, &read));
            REQUIRE (read == sizeof bb);
            REQUIRE (memcmp (z, bb, sizeof bb) == 0);
        }
//        KOutMsg ("Comparing non zero range\n");
        for (uint64_t ii = pos; ii < size_for_set; ii += sizeof b)
        {
            char bb [sizeof b];
            size_t read;

//            KOutMsg ("set  ii = %lu\n",ii);

            REQUIRE_RC (KFileReadAll (rfile, ii, bb, sizeof bb, &read));
            REQUIRE (read == sizeof bb);
            REQUIRE (memcmp (b, bb, sizeof bb) == 0);
        }
        REQUIRE_RC (KFileRelease (rfile));

    }


    REQUIRE_RC (KDirectoryRemove (cwd, true, file_name));
}
    

#if 0

TEST_CASE(SparseWrite_simple_write)
{
    KOutMsg ("%s SparseWrite_simple_write\n",__func__);

    char input [32 * 1024];

    for (size_t ix = 0; ix < sizeof input;  ++ ix)
        input [ix] = (char)(' ' + (char)(ix & 0x3F));

    char buffer[4 * 1024 + 1]; // extra for NUL for display purposes

    KFile * wfile;


    for (size_t pos = 0; pos < sizeof buffer /* including the +1 */; pos += 100) // skipping a bunch since its working
//    for (size_t pos = 0; pos < sizeof buffer /* including the +1 */; ++pos)
    {
        for (size_t inc = 1; inc < sizeof (input); inc += 50) // faster since its working
//        for (size_t inc = 1; inc < sizeof (input); ++inc)
        {
            for (size_t bsize = 0; bsize <= sizeof input + 1; bsize += inc)
            {
                size_t num_writ;

                memset (buffer, 0, sizeof buffer);

                REQUIRE_RC(SparseMakeWrite (&wfile, buffer, sizeof buffer - 1));
                REQUIRE_RC(KFileWrite(wfile, pos, input, bsize, &num_writ));
                REQUIRE(((bsize<sizeof(input))?bsize:sizeof(input))==num_writ);
                REQUIRE(memcmp(buffer,input,num_writ) == 0);
                REQUIRE_RC(KFileRelease(wfile));
            }
        }
    }

}


TEST_CASE(KramFileWrite_append_write)
{
    KOutMsg ("%s KramFileWrite_append_write\n",__func__);
    char input [4 * 1024];

    for (size_t ix = 0; ix < sizeof input;  ++ ix)
        input [ix] = (char)(' ' + (char)(ix & 0x3F));

    char buffer[4 * 1024 + 1]; // extra for NUL for display purposes

    KFile * wfile;

    for (size_t pos = 0; pos < sizeof buffer - 1; pos += 200)
//    for (size_t pos = 0; pos < sizeof buffer - 1; pos ++)
    {
        for (size_t bsize = 1; bsize < sizeof buffer - 1; bsize += 200)
//        for (size_t bsize = 1; bsize < sizeof buffer - 1; ++bsize)
        {
            uint64_t tot_writ = 0;
            size_t num_writ;

            REQUIRE_RC(SparseMakeWrite (&wfile, buffer, sizeof buffer - 1));
            memset (buffer, 0, sizeof buffer);

            for (size_t off = 0; tot_writ + bsize < sizeof buffer - 1; off += num_writ)
            {
                REQUIRE_RC (KFileWrite(wfile, tot_writ, input + off, bsize, &num_writ));
                REQUIRE(num_writ == bsize);
                tot_writ += num_writ;

                REQUIRE(memcmp(input,buffer,tot_writ) == 0);
            }

            REQUIRE_RC(KFileRelease(wfile));            
        }
    }
}

TEST_CASE(SparseWrite_oversized_writes)
{
    KOutMsg ("%s SparseWrite_oversized_writes\n",__func__);
    char input [16 * 1024];

    for (size_t ix = 0; ix < sizeof input;  ++ ix)
        input [ix] = (char)(' ' + (char)(ix & 0x3F));

    char buffer[4 * 1024 + 1];

    KFile * wfile;

//    for (size_t max = 1; max <= sizeof buffer - 1; ++max)
    for (size_t max = 1; max <= sizeof buffer - 1; max += 200)
    {
        memset (buffer, 0, sizeof buffer);
        REQUIRE_RC(SparseMakeWrite (&wfile, buffer, max));

//        for (size_t bsize = (max+1)/2; bsize < sizeof input; ++bsize)
        for (size_t bsize = (max+1)/2; bsize < sizeof input; bsize+= 150)
        {
            size_t num_writ;

            REQUIRE_RC (KFileWrite(wfile, 0, input, bsize, &num_writ));
            if (bsize > max)
                REQUIRE(num_writ == max);
            else
                REQUIRE(num_writ == bsize);

            REQUIRE(0 == memcmp(buffer, input, num_writ));
            REQUIRE(0 != memcmp(buffer, input, num_writ+1));
        }
    }
}


TEST_CASE(SparseWrite_shift_right)
{
    KOutMsg ("%s SparseWrite_shift_right\n",__func__);
    char input [16 * 1024];

    for (size_t ix = 0; ix < sizeof input;  ++ ix)
        input [ix] = (char)(' ' + (char)(ix & 0x3F));

    char buffer[4 * 1024];

    uint64_t tot_writ;
    size_t num_writ;

    KFile * wfile;

    for (size_t bsize = 1; bsize < sizeof buffer; ++bsize)
    {
//         for (size_t off = sizeof buffer / 2

        REQUIRE_RC(SparseMakeWrite (&wfile, buffer, sizeof buffer - 1));

        REQUIRE_RC(KFileWrite(wfile, 0, input, bsize, &num_writ));
        REQUIRE(bsize = num_writ);


    }
    
#if 0

//    for (size_t bsize = 1; bsize < 2 * sizeof buffer; ++bsize)
    for (bsize = 1; bsize < 2 * sizeof buffer; ++bsize)
    {
        REQUIRE(num_writ == sizeof buffer / 2);
        tot_writ = num_writ;

        KOutMsg ("buffer (%.*s)\n", num_writ, buffer);

        for (uint64_t pos = bsize; pos < sizeof input - sizeof buffer; pos += bsize)
        {
            KOutMsg ("KFileWrite (wfile, %u, (%.*s), %u, &num_writ)\n", pos, bsize, input + pos - 1, bsize);

            REQUIRE_RC (KFileWrite(wfile, pos, input + pos - 1, bsize, &num_writ));
            REQUIRE((num_writ == bsize) || (num_writ == sizeof buffer - 1));
            tot_writ += num_writ;

            KOutMsg("pos %u bsize %u num_writ %u\n",pos,bsize,num_writ);
            KOutMsg("buffer (%.*s)\n",tot_writ,buffer);


//            KOutMsg("memcmp((%.*s),(%.*s),%u)\n",
//            REQUIRE(memcmp(buffer, input + bsize - (sizeof buffer - 1), sizeof buffer - 1) == 0);
        }
    }
#endif
}
#endif

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
    rc_t rc=SparseTestSuite(argc, argv);
    return rc;
}

}
