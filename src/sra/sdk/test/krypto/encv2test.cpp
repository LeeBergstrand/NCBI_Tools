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

TEST_SUITE(EncV2FileTestSuite);

#if 1
TEST_CASE(KEncFileMakeEmpty)
{
    OUTMSG (("\nKEncFileMakeEmpty\n"));

    KDirectory * cwd;

    REQUIRE_RC(KDirectoryNativeDir (&cwd));

//    const char wv1 [] = "temp.encv2.wv1";
    const char wv2 [] = "temp.encv2.wv2";
    const char uv2 [] = "temp.encv2.uv2";

//    KFile * sys_wv1;
    KFile * sys_wv2;
    KFile * sys_uv2;

//    REQUIRE_RC(KDirectoryCreateFile (cwd, &sys_wv1, false, 0660, kcmInit, wv1));
    REQUIRE_RC(KDirectoryCreateFile (cwd, &sys_wv2, false, 0660, kcmInit, wv2));
    REQUIRE_RC(KDirectoryCreateFile (cwd, &sys_uv2, true,  0660, kcmInit, uv2));


    const char password [] = "just some password";

    KKey key;

    REQUIRE_RC(KKeyInitUpdate (&key, kkeyAES128, password, strlen (password)));

//    KFile * enc_wv1;
    KFile * enc_wv2;
    KFile * enc_uv2;

//    REQUIRE_RC(KEncFileMakeWrite_v1  (&enc_wv1, sys_wv1, &key));
    REQUIRE_RC(KEncFileMakeWrite_v2  (&enc_wv2, sys_wv2, &key));
    REQUIRE_RC(KEncFileMakeUpdate_v2 (&enc_uv2, sys_uv2, &key));

//    uint64_t size_wv1;
    uint64_t size_wv2;
    uint64_t size_uv2;

//    REQUIRE_RC(KFileSize (enc_wv1, &size_wv1));
    REQUIRE_RC(KFileSize (enc_wv2, &size_wv2));
    REQUIRE_RC(KFileSize (enc_uv2, &size_uv2));

//    REQUIRE (size_wv1 == 0);
    REQUIRE (size_wv2 == 0);
    REQUIRE (size_uv2 == 0);

//    REQUIRE_RC(KFileRelease (enc_wv1));
    REQUIRE_RC(KFileRelease (enc_wv2));
    REQUIRE_RC(KFileRelease (enc_uv2));

//    REQUIRE_RC(KFileRelease (sys_wv1));
    REQUIRE_RC(KFileRelease (sys_wv2));
    REQUIRE_RC(KFileRelease (sys_uv2));

//    REQUIRE_RC (KDirectoryFileSize (cwd, &size_wv1, wv1));
    REQUIRE_RC (KDirectoryFileSize (cwd, &size_wv2, wv2));
    REQUIRE_RC (KDirectoryFileSize (cwd, &size_uv2, uv2));

//    REQUIRE (size_wv1 == 32);
    REQUIRE (size_wv2 == 32);
    REQUIRE (size_uv2 == 32);

//    REQUIRE_RC (KDirectoryRemove (cwd, true, wv1));
    REQUIRE_RC (KDirectoryRemove (cwd, true, wv2));
    REQUIRE_RC (KDirectoryRemove (cwd, true, uv2));

    REQUIRE_RC (KDirectoryRelease (cwd));
}
#endif

#if 1
TEST_CASE(KEncV2FileWrite)
{
    OUTMSG (("\nKEncV2FileLinearReadWrite\n"));

    KDirectory * cwd;

    REQUIRE_RC(KDirectoryNativeDir (&cwd));

    const char write_v1 [] = "temp.encv2test-a";
    const char write_v2 [] = "temp.encv2test-b";
    const char update_v2 [] = "temp.encv2test-c";

    const char password [] = "just some password";

    KKey key;

    /* create two encrypted files from the same "source" */

    REQUIRE_RC(KKeyInitUpdate (&key, kkeyAES128, password, strlen (password)));

    char obuff [127-32];

    for (size_t ix = 0; ix < sizeof obuff; ++ix)
        obuff[ix] = (char)(' ' + (char)ix);

    const unsigned int repeat_count = 1000;

    uint64_t sys_file_size;
    uint64_t dec_file_size;

// comparing the creation of files using the old v1 write, new v2 write and v2 update
// creating new files that should be identical

    {
        OUTMSG (("write three files\n"));

        KFile * sys_write_v1;
        KFile * sys_write_v2;
        KFile * sys_update_v2;

//    KOutMsg ("Create the three files\n");

        REQUIRE_RC(KDirectoryCreateFile (cwd, &sys_write_v1, false, 0660, kcmInit, write_v1));
        REQUIRE_RC(KDirectoryCreateFile (cwd, &sys_write_v2, false, 0660, kcmInit, write_v2));
        REQUIRE_RC(KDirectoryCreateFile (cwd, &sys_update_v2, true,  0660, kcmInit, update_v2));

        KFile * enc_write_v1;
        KFile * enc_write_v2;
        KFile * enc_update_v2;

        REQUIRE_RC(KEncFileMakeWrite_v1  (&enc_write_v1, sys_write_v1, &key));
        REQUIRE_RC(KEncFileMakeWrite_v2  (&enc_write_v2, sys_write_v2, &key));
        REQUIRE_RC(KEncFileMakeUpdate_v2 (&enc_update_v2, sys_update_v2, &key));

        dec_file_size = 0;

        for (unsigned int ix = 0; ix < repeat_count; ++ix)
        {
            size_t num_writ;

            REQUIRE_RC(KFileWriteAll (enc_write_v1, ix * sizeof obuff, obuff, sizeof obuff, &num_writ));
            REQUIRE (num_writ == sizeof obuff);

            REQUIRE_RC(KFileWriteAll (enc_write_v2, ix * sizeof obuff, obuff, sizeof obuff, &num_writ));
            REQUIRE (num_writ == sizeof obuff);

            REQUIRE_RC(KFileWriteAll (enc_update_v2, ix * sizeof obuff, obuff, sizeof obuff, &num_writ));
            REQUIRE (num_writ == sizeof obuff);

            dec_file_size += num_writ;

        }

        REQUIRE_RC(KFileRelease (enc_write_v1));
        REQUIRE_RC(KFileRelease (enc_write_v2));
        REQUIRE_RC(KFileRelease (enc_update_v2));

        REQUIRE_RC(KFileRelease (sys_write_v1));
        REQUIRE_RC(KFileRelease (sys_write_v2));
        REQUIRE_RC(KFileRelease (sys_update_v2));
    }
    {
        OUTMSG (("compare three file sizes\n"));

        uint64_t file_size_write_v1, file_size_write_v2, file_size_update_v2;

        REQUIRE_RC(KDirectoryFileSize (cwd, &file_size_write_v1, write_v1));
        REQUIRE_RC(KDirectoryFileSize (cwd, &file_size_write_v2, write_v2));
        REQUIRE_RC(KDirectoryFileSize (cwd, &file_size_update_v2, update_v2));

        REQUIRE(file_size_write_v1 == file_size_write_v2);
        REQUIRE(file_size_write_v1 == file_size_update_v2);

        sys_file_size = file_size_write_v1;
    }

    {
        OUTMSG (("compare three files sys\n"));

        const KFile * sys_read_v1;
        const KFile * sys_read_v2;
        KFile       * sys_update_v2;

        REQUIRE_RC(KDirectoryOpenFileRead (cwd, &sys_read_v1, write_v1));
        REQUIRE_RC(KDirectoryOpenFileRead (cwd, &sys_read_v2, write_v2));
        REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_update_v2, true, update_v2));

        size_t num_read_v1, num_read_v2, num_update_v2;

        const KFile * dec_read_v1;
        const KFile * dec_read_v2;
        KFile       * dec_update_v2;

        {
            uint64_t size;

            REQUIRE_RC (KFileSize (sys_read_v1, &size));
            REQUIRE (size == sys_file_size);

            REQUIRE_RC (KFileSize (sys_read_v2, &size));
            REQUIRE (size == sys_file_size);

            REQUIRE_RC (KFileSize (sys_update_v2, &size));
            REQUIRE (size == sys_file_size);
        }
        {
            const size_t read_buffer_size = 4000;

            char buffer_read_v1 [read_buffer_size];
            char buffer_read_v2 [read_buffer_size];
            char buffer_update_v2 [read_buffer_size];

            for (uint64_t pos = 0; ; pos += num_read_v1)
            {
                REQUIRE_RC (KFileReadAll (sys_read_v1, pos,buffer_read_v1,
                                          sizeof buffer_read_v1, &num_read_v1));
                REQUIRE_RC (KFileReadAll (sys_read_v2, pos, buffer_read_v2,
                                          sizeof buffer_read_v2, &num_read_v2));
                REQUIRE_RC (KFileReadAll (sys_update_v2, pos, buffer_update_v2,
                                          sizeof buffer_update_v2,
                                          &num_update_v2));

                REQUIRE (num_read_v1 == num_read_v2);
                REQUIRE (num_read_v1 == num_update_v2);

                if (num_read_v1 == 0)
                    break;

                // the header might differ and use version number 1 or 2 
                // at the time this was written
                if (pos == 0)
                {
                    size_t offset = sizeof (KEncFileSig) + sizeof (Endian_t);

                    REQUIRE (memcmp (buffer_read_v1, buffer_read_v2, offset) == 0);
                    REQUIRE (memcmp (buffer_read_v1, buffer_update_v2, offset) == 0);

#define GET_VERSION(buf) *reinterpret_cast<KEncFileVersion*>(buf + offset)
                    REQUIRE ((GET_VERSION(buffer_read_v1) == 1) || (GET_VERSION(buffer_read_v1) == 2));
                    REQUIRE ((GET_VERSION(buffer_read_v2) == 2));
                    REQUIRE ((GET_VERSION(buffer_update_v2) == 2));
#undef GET_VERSION

                    offset += sizeof (KEncFileVersion);

                    REQUIRE (memcmp (buffer_read_v1+offset, buffer_read_v2+offset, num_read_v1-offset) == 0);
                    REQUIRE (memcmp (buffer_read_v1+offset, buffer_update_v2+offset, num_read_v1-offset) == 0);
                }
                else
                {
                    // this test will assume that all three versions create a file
                    // with a valid cnum_update_v2 checksum at the end
                    REQUIRE (memcmp (buffer_read_v1, buffer_read_v2, num_read_v1) == 0);
                    REQUIRE ((memcmp (buffer_read_v1, buffer_update_v2, num_read_v1) == 0) ||
                             ((num_read_v1<read_buffer_size )&&(memcmp (buffer_read_v1, buffer_update_v2, num_read_v1-8) == 0)));
                }
            }

            REQUIRE_RC (KFileRelease (sys_read_v1));
            REQUIRE_RC (KFileRelease (sys_read_v2));
            REQUIRE_RC(KFileRelease (sys_update_v2));

            OUTMSG (("compare three files decrypted linearly\n"));

            REQUIRE_RC(KDirectoryOpenFileRead  (cwd, &sys_read_v1, write_v1));
            REQUIRE_RC(KDirectoryOpenFileRead  (cwd, &sys_read_v2, write_v2));
            REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_update_v2, true, update_v2));

            REQUIRE_RC(KEncFileMakeRead_v1   (&dec_read_v1, sys_read_v1, &key));
            REQUIRE_RC(KEncFileMakeRead_v2   (&dec_read_v2, sys_read_v2, &key));
            REQUIRE_RC(KEncFileMakeUpdate_v2 (&dec_update_v2, sys_update_v2, &key));

            for (uint64_t pos = 0; ; pos += num_read_v1)
            {
                REQUIRE_RC (KFileReadAll (dec_read_v1, pos, buffer_read_v1, sizeof buffer_read_v1, &num_read_v1));
                REQUIRE_RC (KFileReadAll (dec_read_v2, pos, buffer_read_v2, sizeof buffer_read_v2, &num_read_v2));
                REQUIRE_RC (KFileReadAll (dec_update_v2, pos, buffer_update_v2, sizeof buffer_update_v2, &num_update_v2));

                REQUIRE (num_read_v1 == num_read_v2);
                REQUIRE (num_read_v1 == num_update_v2);

                if (num_read_v1 == 0)
                    break;

                REQUIRE (memcmp (buffer_read_v1, buffer_read_v2, num_read_v2) == 0);
                REQUIRE (memcmp (buffer_read_v2, buffer_update_v2, num_read_v2) == 0);
            }

            REQUIRE_RC(KFileRelease (dec_read_v1));
            REQUIRE_RC(KFileRelease (dec_read_v2));
            REQUIRE_RC(KFileRelease (dec_update_v2));

            REQUIRE_RC(KFileRelease (sys_read_v1));
            REQUIRE_RC(KFileRelease (sys_read_v2));
            REQUIRE_RC(KFileRelease (sys_update_v2));
        }
        {
            const size_t dec_buffer_size = 10000;

            char buffer_read_v1 [dec_buffer_size];
            char buffer_read_v2 [dec_buffer_size];
            char buffer_update_v2 [dec_buffer_size];

            OUTMSG (("compare three files decrypted skip around\n"));

            REQUIRE_RC(KDirectoryOpenFileRead  (cwd, &sys_read_v1, write_v1));
            REQUIRE_RC(KDirectoryOpenFileRead  (cwd, &sys_read_v2, write_v2));
            REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_update_v2, true, update_v2));

//             OUTMSG (("%s\n",write_v1));
            REQUIRE_RC(KEncFileMakeRead_v1   (&dec_read_v1, sys_read_v1, &key));
            OUTMSG (("%s\n",write_v2));
            REQUIRE_RC(KEncFileMakeRead_v2   (&dec_read_v2, sys_read_v2, &key));
            OUTMSG (("%s\n",update_v2));
            REQUIRE_RC(KEncFileMakeUpdate_v2 (&dec_update_v2, sys_update_v2, &key));

            {
                uint64_t size;

// operation not supported
//                 REQUIRE_RC (KFileSize (dec_read_v1, &size));
//                 REQUIRE (size == sys_file_size);

                REQUIRE_RC (KFileSize (dec_read_v2, &size));
//                 OUTMSG (("size %lu sys_file_size %lu %lu\n", size, sys_file_size, ((((size+32767)/32768)*(32768+64))+32)));
                REQUIRE (((((size+32767)/32768)*(32768+64))+32) == sys_file_size);

                REQUIRE_RC (KFileSize (dec_update_v2, &size));
//                 OUTMSG (("size %lu sys_file_size %lu %lu\n", size, sys_file_size, ((((size+32767)/32768)*(32768+64))+32)));
                REQUIRE (((((size+32767)/32768)*(32768+64))+32) == sys_file_size);
            }


            for (uint64_t pos = dec_file_size; pos > dec_buffer_size; pos -= dec_buffer_size)
            {   
                REQUIRE_RC (KFileReadAll (dec_read_v1, pos,
                                          buffer_read_v1,
                                          sizeof buffer_read_v1,
                                          &num_read_v1));
                REQUIRE_RC (KFileReadAll (dec_read_v2, pos,
                                          buffer_read_v2,
                                          sizeof buffer_read_v2,
                                          &num_read_v2));
                REQUIRE_RC (KFileReadAll (dec_update_v2, pos,
                                          buffer_update_v2,
                                          sizeof buffer_update_v2,
                                          &num_update_v2));

                REQUIRE (num_read_v1 == num_read_v2);
                REQUIRE (num_read_v1 == num_update_v2);

                REQUIRE (memcmp (buffer_read_v1, buffer_read_v2, num_read_v2) == 0);
                REQUIRE (memcmp (buffer_read_v2, buffer_update_v2, num_read_v2) == 0);
            }
            REQUIRE_RC (KFileReadAll (dec_read_v1, 0, buffer_read_v1,
                                      sizeof buffer_read_v1, &num_read_v1));
            REQUIRE_RC (KFileReadAll (dec_read_v2, 0, buffer_read_v2,
                                      sizeof buffer_read_v2, &num_read_v2));
            REQUIRE_RC (KFileReadAll (dec_update_v2, 0, buffer_update_v2,
                                      sizeof buffer_update_v2, &num_update_v2));

            REQUIRE (num_read_v1 == num_read_v2);
            REQUIRE (num_read_v1 == num_update_v2);

            REQUIRE (memcmp (buffer_read_v1, buffer_read_v2, num_read_v2) == 0);
            REQUIRE (memcmp (buffer_read_v2, buffer_update_v2, num_read_v2) == 0);

            REQUIRE_RC (KFileRelease (dec_read_v1));
            REQUIRE_RC (KFileRelease (dec_read_v2));
            REQUIRE_RC (KFileRelease (dec_update_v2));

            REQUIRE_RC (KFileRelease (sys_read_v1));
            REQUIRE_RC (KFileRelease (sys_read_v2));
            REQUIRE_RC (KFileRelease (sys_update_v2));

        }
    }

    REQUIRE_RC (KDirectoryRemove (cwd, true, write_v1));
    REQUIRE_RC (KDirectoryRemove (cwd, true, write_v2));
    REQUIRE_RC (KDirectoryRemove (cwd, true, update_v2));

    REQUIRE_RC (KDirectoryRelease (cwd));
}
#endif

class AutoBuffer
{
public:
    AutoBuffer(size_t sz) : buf(new char[sz]), size(sz) { }
    ~AutoBuffer() { delete[] buf; }
    operator char *(void) { return buf; }
    size_t GetSize(void) const { return size; }

private:
    char * buf;
    size_t size;
};

TEST_CASE (KEncFile_with_KBufFile)
{
    OUTMSG (("\nKEncFile_with_KBufFile\n"));

    KDirectory * cwd;

    REQUIRE_RC(KDirectoryNativeDir (&cwd));

    const char password [] = "just some password";

    KKey key;

    /* create two encrypted files from the same "sounum_update_v2e" */

    REQUIRE_RC(KKeyInitUpdate (&key, kkeyAES128, password, strlen (password)));

    KOutStr ("Testing FileSize and Update Mode of Encfile Append\n");

    const char name_encrypt[] = "temp.encrypt";
    const char name_buffered[] = "temp.buffered";

    KFile * sys_encrypt;
    KFile * sys_buffered;

    KFile * enc_encrypt;
    KFile * enc_buffered;

    KFile * buf_buffered;

    const size_t buffer_size = 32768;
    const size_t buffered_buffer_size = 4 * buffer_size;
    uint64_t size_encrypt, size_buffered;
    size_t num_encrypt, num_buffered;

#define SIZE_TO_ENC_SIZE(ZZZ) ((((ZZZ+buffer_size-1)/buffer_size)*(buffer_size+64))+32)

    const size_t stuff_size = ((size_t)'~' - ((size_t)'!' - 1));
    char stuff[stuff_size];

    for (size_t ii = 0; ii < stuff_size; ++ii)
        stuff[ii] = ' ' + ii;

    // Create an empty file
    const uint64_t empty_file_size = 0;
    const uint64_t enc_empty_file_size = SIZE_TO_ENC_SIZE(empty_file_size);
    // write at the beginning
    const uint64_t beginning_file_position = 0;
    const uint64_t beginning_file_size = beginning_file_position + stuff_size;
    const uint64_t enc_beginning_file_size = SIZE_TO_ENC_SIZE(beginning_file_size);
    // write overlapping the first two blocks
    const uint64_t overlap_file_position = buffer_size - (stuff_size/2);
    const uint64_t overlap_file_size = overlap_file_position + stuff_size;
    const uint64_t enc_overlap_file_size = SIZE_TO_ENC_SIZE(overlap_file_size);
    // write far out into the file
    const uint64_t wayout_file_position = buffer_size * 12;
    const uint64_t wayout_file_size = wayout_file_position + stuff_size;
    const uint64_t enc_wayout_file_size = SIZE_TO_ENC_SIZE(wayout_file_size);
    // write back near the beginning again
    const uint64_t wayback_file_position = overlap_file_position + 3 * stuff_size;
    const uint64_t wayback_file_size = wayback_file_position + stuff_size;
    const uint64_t enc_wayback_file_size = SIZE_TO_ENC_SIZE(wayback_file_size);

    AutoBuffer buffer_encrypt(2 * wayout_file_size);
    AutoBuffer buffer_buffered(2 * wayout_file_size);

    OUTSTR (("==========\n"));
    OUTSTR (("==========\n"));
    OUTSTR (("Creating empty files\n"));
    {
        REQUIRE_RC(KDirectoryCreateFile (cwd, &sys_encrypt, true, 0660, kcmInit, name_encrypt));
        REQUIRE_RC(KDirectoryCreateFile (cwd, &sys_buffered, true, 0660, kcmInit, name_buffered));
        {
            REQUIRE_RC (KEncFileMakeUpdate_v2 (&enc_encrypt, sys_encrypt, &key));
            REQUIRE_RC (KEncFileMakeUpdate_v2 (&enc_buffered, sys_buffered, &key));
            {
                REQUIRE_RC (KBufFileMakeWrite (&buf_buffered, enc_buffered, true, buffered_buffer_size));
                {
                    REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                    REQUIRE_RC (KFileSize (buf_buffered, &size_buffered));

                    REQUIRE (size_encrypt == empty_file_size);
                    REQUIRE (size_buffered == empty_file_size);
                }
                REQUIRE_RC (KFileRelease (buf_buffered));

                REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                REQUIRE_RC (KFileSize (enc_buffered, &size_buffered));

                REQUIRE (size_encrypt == empty_file_size);
                REQUIRE (size_buffered == empty_file_size);
            }
            REQUIRE_RC (KFileRelease (enc_encrypt));
            REQUIRE_RC (KFileRelease (enc_buffered));
        }
        REQUIRE_RC (KFileSize (sys_encrypt, &size_encrypt));
        REQUIRE_RC (KFileSize (sys_buffered, &size_buffered));

        REQUIRE (size_encrypt == enc_empty_file_size);
        REQUIRE (size_buffered == enc_empty_file_size);

        REQUIRE_RC (KFileRelease (sys_encrypt));
        REQUIRE_RC (KFileRelease (sys_buffered));
    }

    OUTSTR (("reopening empty_files and exampining header and footer\n"));
    {
        REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_encrypt, true,
                                            name_encrypt));
        REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_buffered, true,
                                            name_buffered));
        {
            union
            {
                char raw[1024];
                struct
                {
                    KEncFileHeader head;
                    KEncFileFooter foot;
                } cooked;
            } u;
                
            const char sig[] = "NCBInenc";

            // validate contents of the "empty" encrypted contents"
            REQUIRE (sizeof u.cooked.head == 16);
            REQUIRE (sizeof u.cooked.foot == 16);
            REQUIRE (sizeof u.cooked == 32);

            REQUIRE_RC (KFileRead (sys_encrypt, 0, &u, sizeof u, &num_encrypt));

            REQUIRE (num_encrypt == sizeof u.cooked);
            REQUIRE (memcmp (u.cooked.head.file_sig, sig, strlen (sig)) == 0);
            REQUIRE (u.cooked.head.version == 2);
            REQUIRE (u.cooked.foot.block_count == 0);
            REQUIRE (u.cooked.foot.crc_checksum == 0);

            REQUIRE_RC (KFileRead (sys_buffered, 0, &u, sizeof u, &num_buffered));

            REQUIRE (num_buffered == sizeof u.cooked);
            REQUIRE (memcmp (u.cooked.head.file_sig, sig, strlen (sig)) == 0);
            REQUIRE (u.cooked.head.version == 2);
            REQUIRE (u.cooked.foot.block_count == 0);
            REQUIRE (u.cooked.foot.crc_checksum == 0);
        }
        REQUIRE_RC (KFileRelease (sys_encrypt));
        REQUIRE_RC (KFileRelease (sys_buffered));
    }

    OUTSTR (("reopening empty_files and writing at beginning\n"));
    {
        REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_encrypt, true, name_encrypt));
        REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_buffered, true, name_buffered));
        {
            REQUIRE_RC (KEncFileMakeUpdate_v2 (&enc_encrypt, sys_encrypt, &key));
            REQUIRE_RC (KEncFileMakeUpdate_v2 (&enc_buffered, sys_buffered, &key));
            {
                REQUIRE_RC (KBufFileMakeWrite (&buf_buffered, enc_buffered, true, buffered_buffer_size));
                {
                    REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                    REQUIRE_RC (KFileSize (buf_buffered, &size_buffered));

                    REQUIRE (size_encrypt == empty_file_size);
                    REQUIRE (size_buffered == empty_file_size);

                    REQUIRE_RC (KFileWriteAll (enc_encrypt, beginning_file_position, stuff, stuff_size, &num_encrypt));
                    REQUIRE_RC (KFileWriteAll (buf_buffered, beginning_file_position, stuff, stuff_size, &num_buffered));

                    REQUIRE (num_encrypt == stuff_size);
                    REQUIRE (num_buffered == stuff_size);

                    REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                    REQUIRE_RC (KFileSize (buf_buffered, &size_buffered));
    
                    REQUIRE (size_encrypt == beginning_file_size);
//                     REQUIRE (size_buffered == beginning_file_size);

                    memset (buffer_encrypt, 0xFF, buffer_encrypt.GetSize());
                    memset (buffer_buffered, 0xFF, buffer_buffered.GetSize());

                    REQUIRE_RC (KFileReadAll (enc_encrypt, 0, buffer_encrypt, buffer_encrypt.GetSize(), &num_encrypt));
                    REQUIRE_RC (KFileReadAll (buf_buffered, 0, buffer_buffered, buffer_buffered.GetSize(), &num_buffered));

                    REQUIRE (num_encrypt == stuff_size);
//
// Test had to be rethought as KBufFile reads beyond end of virtual file!!!!
//                    REQUIRE (num_encrypt == stuff_size);

                    REQUIRE (memcmp (stuff, buffer_encrypt, num_encrypt) == 0);
//                    REQUIRE (memcmp (stuff, buffer_buffered, num_buffered) == 0);
                    REQUIRE (memcmp (stuff, buffer_buffered, num_encrypt) == 0);

                    REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                    REQUIRE_RC (KFileSize (buf_buffered, &size_buffered));
    
                    REQUIRE (size_encrypt == beginning_file_size);
//                    REQUIRE (size_buffered == beginning_file_size);
                }
                REQUIRE_RC (KFileRelease (buf_buffered));
                buf_buffered = NULL;

                REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                REQUIRE_RC (KFileSize (enc_buffered, &size_buffered));
    
                REQUIRE (size_encrypt == stuff_size);
                REQUIRE (size_buffered == stuff_size);
            }
            REQUIRE_RC (KFileRelease (enc_encrypt));
            REQUIRE_RC (KFileRelease (enc_buffered));

            enc_encrypt = NULL;
            enc_buffered = NULL;
        }
        REQUIRE_RC (KFileSize (sys_encrypt, &size_encrypt));
        REQUIRE_RC (KFileSize (sys_buffered, &size_buffered));

//         OUTMSG (("encrypt %lu buffered %lu both should be 16 + 64 + 32768 + "
//                  "16 or %lu\n", size_encrypt, size_buffered, 16 + 64 + 32768 + 16));

        REQUIRE (size_encrypt == enc_beginning_file_size);
        REQUIRE (size_buffered == enc_beginning_file_size);

        REQUIRE_RC (KFileRelease (sys_encrypt));
        REQUIRE_RC (KFileRelease (sys_buffered));
    }

    OUTSTR (("reopening files and writing at a buffer overlap position skipping some forward\n"));
    {
        REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_encrypt, true, name_encrypt));
        REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_buffered, true, name_buffered));
        {
            REQUIRE_RC (KEncFileMakeUpdate_v2 (&enc_encrypt, sys_encrypt, &key));
            REQUIRE_RC (KEncFileMakeUpdate_v2 (&enc_buffered, sys_buffered, &key));
            {
                REQUIRE_RC (KBufFileMakeWrite (&buf_buffered, enc_buffered, true, buffered_buffer_size));
                {
                    REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                    REQUIRE_RC (KFileSize (buf_buffered, &size_buffered));

                    REQUIRE (size_encrypt == beginning_file_size);
//                    REQUIRE (size_buffered == beginning_file_size);

                    REQUIRE_RC (KFileWriteAll (enc_encrypt, overlap_file_position, stuff, stuff_size, &num_encrypt));
                    REQUIRE_RC (KFileWriteAll (buf_buffered, overlap_file_position, stuff, stuff_size, &num_buffered));

                    REQUIRE (num_encrypt == stuff_size);
                    REQUIRE (num_buffered == stuff_size);

                    REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                    REQUIRE_RC (KFileSize (buf_buffered, &size_buffered));

                    REQUIRE (size_encrypt == overlap_file_size);
//                    REQUIRE (size_buffered == overlap_file_size);
                }
                REQUIRE_RC (KFileRelease (buf_buffered));
                buf_buffered = NULL;

                REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                REQUIRE_RC (KFileSize (enc_buffered, &size_buffered));
    
                REQUIRE (size_encrypt == overlap_file_size);
                REQUIRE (size_buffered == overlap_file_size);
            }
            REQUIRE_RC (KFileRelease (enc_encrypt));
            REQUIRE_RC (KFileRelease (enc_buffered));

            enc_encrypt = NULL;
            enc_buffered = NULL;
        }
        REQUIRE_RC (KFileSize (sys_encrypt, &size_encrypt));
        REQUIRE_RC (KFileSize (sys_buffered, &size_buffered));

        REQUIRE (size_encrypt == enc_overlap_file_size);
        REQUIRE (size_buffered == enc_overlap_file_size);

        REQUIRE_RC (KFileRelease (sys_encrypt));
        REQUIRE_RC (KFileRelease (sys_buffered));
    }

    OUTSTR (("reopening files and writing at a buffer position skipping farther forward then backward\n"));
    {
        REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_encrypt, true, name_encrypt));
        REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_buffered, true, name_buffered));
        {
            REQUIRE_RC (KEncFileMakeUpdate_v2 (&enc_encrypt, sys_encrypt, &key));
            REQUIRE_RC (KEncFileMakeUpdate_v2 (&enc_buffered, sys_buffered, &key));
            {
                REQUIRE_RC (KBufFileMakeWrite (&buf_buffered, enc_buffered, true, buffered_buffer_size));
                {
                    REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                    REQUIRE_RC (KFileSize (buf_buffered, &size_buffered));

                    REQUIRE (size_encrypt == overlap_file_size);
//                    REQUIRE (size_buffered == overlap_file_size);

                    REQUIRE_RC (KFileWriteAll (enc_encrypt, wayout_file_position, stuff, stuff_size, &num_encrypt));
                    REQUIRE_RC (KFileWriteAll (buf_buffered, wayout_file_position, stuff, stuff_size, &num_buffered));

                    REQUIRE (num_encrypt == stuff_size);
                    REQUIRE (num_buffered == stuff_size);

                    REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                    REQUIRE_RC (KFileSize (buf_buffered, &size_buffered));

                    REQUIRE (size_encrypt == wayout_file_size);
//                    REQUIRE (size_buffered == wayout_file_size);

                    REQUIRE_RC (KFileWriteAll (enc_encrypt, wayback_file_position, stuff, stuff_size, &num_encrypt));
                    REQUIRE_RC (KFileWriteAll (buf_buffered, wayback_file_position, stuff, stuff_size, &num_buffered));

                    REQUIRE (num_encrypt == stuff_size);
                    REQUIRE (num_buffered == stuff_size);

                    REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                    REQUIRE_RC (KFileSize (buf_buffered, &size_buffered));

                    REQUIRE (size_encrypt == wayout_file_size);
//                    REQUIRE (size_buffered == wayout_file_size);

                }
                REQUIRE_RC (KFileRelease (buf_buffered));
                buf_buffered = NULL;
            }
            REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
            REQUIRE_RC (KFileSize (enc_buffered, &size_buffered));

            REQUIRE (size_encrypt == wayout_file_size);
            REQUIRE (size_buffered == wayout_file_size);

            REQUIRE_RC (KFileRelease (enc_encrypt));
            REQUIRE_RC (KFileRelease (enc_buffered));

            enc_encrypt = NULL;
            enc_buffered = NULL;
        }
        REQUIRE_RC (KFileSize (sys_encrypt, &size_encrypt));
        REQUIRE_RC (KFileSize (sys_buffered, &size_buffered));

        REQUIRE (size_encrypt == enc_wayout_file_size);
        REQUIRE (size_buffered == enc_wayout_file_size);

//         OUTMSG (("%lu %lu %lu\n",size_encrypt,size_buffered,(uint64_t)wayout_file_size));

        REQUIRE_RC (KFileRelease (sys_encrypt));
        REQUIRE_RC (KFileRelease (sys_buffered));
    }
    OUTSTR (("reopening files and checking full contents\n"));
    {
        REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_encrypt, true, name_encrypt));
        REQUIRE_RC(KDirectoryOpenFileWrite (cwd, &sys_buffered, true, name_buffered));

        REQUIRE_RC (KFileSize (sys_encrypt, &size_encrypt));
        REQUIRE_RC (KFileSize (sys_buffered, &size_buffered));

        REQUIRE (size_encrypt == enc_wayout_file_size);
        REQUIRE (size_buffered == enc_wayout_file_size);

//         OUTMSG (("%lu %lu %lu\n",size_encrypt,size_buffered,(uint64_t)wayout_file_size));
        {
            REQUIRE_RC (KEncFileMakeUpdate_v2 (&enc_encrypt, sys_encrypt, &key));
            REQUIRE_RC (KEncFileMakeUpdate_v2 (&enc_buffered, sys_buffered, &key));
            {
                REQUIRE_RC (KBufFileMakeWrite (&buf_buffered, enc_buffered, true, buffered_buffer_size));
                {
                    REQUIRE_RC (KFileSize (enc_encrypt, &size_encrypt));
                    REQUIRE_RC (KFileSize (buf_buffered, &size_buffered));

                    REQUIRE (size_encrypt == wayout_file_size);
//                    REQUIRE (size_buffered == wayout_file_size);

//                     OUTMSG (("%lu %lu %lu\n",size_encrypt,size_buffered,(uint64_t)wayout_file_size));

                    REQUIRE_RC (KFileReadAll (enc_encrypt, 0, buffer_encrypt, buffer_encrypt.GetSize(), &num_encrypt));
//                    REQUIRE_RC (KFileReadAll (buf_buffered, 0, buffer_buffered, buffer_buffered.GetSize(), &num_buffered));
                    REQUIRE_RC (KFileReadAll (buf_buffered, 0, buffer_buffered, wayout_file_size, &num_buffered));

//                    OUTMSG (("%zu %zu %lu\n",(size_t)num_encrypt,(size_t)num_buffered,(uint64_t)wayout_file_size));
                    REQUIRE (num_encrypt == wayout_file_size);
                    REQUIRE (num_buffered == wayout_file_size);

//                      OUTMSG (("=====\n%95.95s\n-----\n%95.95s\n=====\n",stuff,buffer_encrypt));
                    REQUIRE (memcmp (buffer_encrypt + beginning_file_position, stuff, stuff_size) == 0);
                    REQUIRE (memcmp (buffer_encrypt + overlap_file_position, stuff, stuff_size) == 0);
                    REQUIRE (memcmp (buffer_encrypt + wayback_file_position, stuff, stuff_size) == 0);
                    REQUIRE (memcmp (buffer_encrypt + wayout_file_position, stuff, stuff_size) == 0);

                    REQUIRE (memcmp (buffer_buffered + beginning_file_position, stuff, stuff_size) == 0);
                    REQUIRE (memcmp (buffer_buffered + overlap_file_position, stuff, stuff_size) == 0);
                    REQUIRE (memcmp (buffer_buffered + wayback_file_position, stuff, stuff_size) == 0);
                    REQUIRE (memcmp (buffer_buffered + wayout_file_position, stuff, stuff_size) == 0);

                    for (size_t ii = beginning_file_size ; ii < overlap_file_position; ++ii)
                    {
//                         OUTMSG (("+++ %zu\n", ii));
                        REQUIRE (buffer_buffered[ii] == '\0');
                    }
                    for (size_t ii = overlap_file_size ; ii < wayback_file_position; ++ii)
                    {
//                         OUTMSG (("+++ %zu\n", ii));
                        REQUIRE (buffer_buffered[ii] == '\0');
                    }
                    for (size_t ii = wayback_file_size ; ii < wayout_file_position; ++ii)
                    {
//                         OUTMSG (("+++ %zu\n", ii));
                        REQUIRE (buffer_buffered[ii] == '\0');
                    }
                    for (size_t ii = beginning_file_size ; ii < overlap_file_position; ++ii)
                    {
//                         OUTMSG (("+++ %zu\n", ii));
                        REQUIRE (buffer_buffered[ii] == '\0');
                    }
                    for (size_t ii = overlap_file_size ; ii < wayback_file_position; ++ii)
                    {
//                         OUTMSG (("+++ %zu\n", ii));
                        REQUIRE (buffer_buffered[ii] == '\0');
                    }
                    for (size_t ii = wayback_file_size ; ii < wayout_file_position; ++ii)
                    {
//                         OUTMSG (("+++ %zu\n", ii));
                        REQUIRE (buffer_buffered[ii] == '\0');
                    }
                }
                REQUIRE_RC (KFileRelease (buf_buffered));
                buf_buffered = NULL;
            }
            REQUIRE_RC (KFileRelease (enc_encrypt));
            REQUIRE_RC (KFileRelease (enc_buffered));
        }
        REQUIRE_RC (KFileRelease (sys_encrypt));
        REQUIRE_RC (KFileRelease (sys_buffered));
    }

// verify size and contents

    REQUIRE_RC (KDirectoryRemove (cwd, true, name_buffered));
    REQUIRE_RC (KDirectoryRemove (cwd, true, name_encrypt));

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
        rc_t rc=EncV2FileTestSuite(argc, argv);
        return rc;
    }

}
