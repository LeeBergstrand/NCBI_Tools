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

#include <klib/defs.h>
#include <klib/out.h>
#include <klib/rc.h>
#include <klib/checksum.h>

#include <kapp/main.h>
#include <kapp/args.h>

#include <stdio.h>
#include <string.h>

#include <common/test_assert.h>

struct test_values {
    const char *test;
    size_t length;
    const char *expect;
};

/* This is 'a' x 1 million */
static char sha_long_test[1000000];

/* This is the first 25 lines of this file
 * i.e. the standard preample on all our files
 */
static char NCBI_PDN_text[1262];

static void sha1(uint8_t digest[20], const char data[], size_t len)
{
    SHA1State st;

    SHA1StateInit(&st);
    SHA1StateAppend(&st, data, len);
    SHA1StateFinish(&st, digest);
}

static void sha256(uint8_t digest[32], const char data[], size_t len)
{
    SHA256State st;

    SHA256StateInit(&st);
    SHA256StateAppend(&st, data, len);
    SHA256StateFinish(&st, digest);
}

static void sha384(uint8_t digest[48], const char data[], size_t len)
{
    SHA384State st;

    SHA384StateInit(&st);
    SHA384StateAppend(&st, data, len);
    SHA384StateFinish(&st, digest);
}

static void sha512(uint8_t digest[64], const char data[], size_t len)
{
    SHA512State st;

    SHA512StateInit(&st);
    SHA512StateAppend(&st, data, len);
    SHA512StateFinish(&st, digest);
}

static int test_sha1()
{
    struct test_values test[] = {
        /* The first 3 are from FIPS180-2 */
        { "abc", 3,
          "\xa9\x99\x3e\x36\x47\x06\x81\x6a\xba\x3e\x25\x71\x78\x50\xc2\x6c\x9c\xd0\xd8\x9d" },

        { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
          "\x84\x98\x3e\x44\x1c\x3b\xd2\x6e\xba\xae\x4a\xa1\xf9\x51\x29\xe5\xe5\x46\x70\xf1" },

        { sha_long_test, 1000000,
          "\x34\xaa\x97\x3c\xd4\xc4\xda\xa4\xf6\x1e\xeb\x2b\xdb\xad\x27\x31\x65\x34\x01\x6f" },

        /* hash generated with
         * head -n 25 basic-test.c | sha1sum -b | awk '{ print $1 }' | perl -n -e 'print "\\x".join("\\x", m/../g)."\n";'
         */
        { NCBI_PDN_text, 1262,
          "\x81\x06\x56\xa2\xbf\x75\x8b\x7c\x22\x83\x88\xd5\x6a\x4a\x3a\x01\xa5\x2f\xc7\xc3" },
    };
    uint8_t digest[20];
    unsigned i;
    unsigned const n = sizeof(test)/sizeof(test[0]);
    bool failed = false;

    for (i = 0; i != n; ++i) {
        sha1(digest, test[i].test, test[i].length);
        if (memcmp(digest, test[i].expect, sizeof(digest)) != 0) {
            unsigned j;

            printf("SHA-1 test #%u failed!\n", i + 1);
            printf("         got: "); for (j = 0; j != sizeof(digest); ++j) printf("%02x", digest[j]); printf("\n");
            printf("    expected: "); for (j = 0; j != sizeof(digest); ++j) printf("%02x", (uint8_t) test[i].expect[j]); printf("\n");
            failed = true;
        }
    }
    return !failed ? 0 : 1;
}

static int test_sha256()
{
    struct test_values test[] = {
        /* The first 3 are from FIPS180-2 */
        { "abc", 3,
          "\xba\x78\x16\xbf\x8f\x01\xcf\xea\x41\x41\x40\xde\x5d\xae\x22\x23"
          "\xb0\x03\x61\xa3\x96\x17\x7a\x9c\xb4\x10\xff\x61\xf2\x00\x15\xad" },

        { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
          "\x24\x8d\x6a\x61\xd2\x06\x38\xb8\xe5\xc0\x26\x93\x0c\x3e\x60\x39"
          "\xa3\x3c\xe4\x59\x64\xff\x21\x67\xf6\xec\xed\xd4\x19\xdb\x06\xc1" },

        { sha_long_test, 1000000,
          "\xcd\xc7\x6e\x5c\x99\x14\xfb\x92\x81\xa1\xc7\xe2\x84\xd7\x3e\x67"
          "\xf1\x80\x9a\x48\xa4\x97\x20\x0e\x04\x6d\x39\xcc\xc7\x11\x2c\xd0" },

        /* hash generated with
         * head -n 25 basic-test.c | sha256sum -b | awk '{ print $1 }' | perl -n -e 'print "\\x".join("\\x", m/../g)."\n";'
         */
        { NCBI_PDN_text, 1262,
          "\x30\xbf\x9f\x65\xdd\xcd\x5c\x6a\xd0\x24\x4a\xf5\xd8\x7d\x1a\x9f"
          "\x92\x0c\x25\x84\xf1\xaa\xc5\x1c\xd0\xef\x90\xe2\xb9\x0e\x2b\x10" },
    };
    uint8_t digest[32];
    unsigned i;
    unsigned const n = sizeof(test)/sizeof(test[0]);
    bool failed = false;

    for (i = 0; i != n; ++i) {
        sha256(digest, test[i].test, test[i].length);
        if (memcmp(digest, test[i].expect, sizeof(digest)) != 0) {
            unsigned j;

            printf("SHA-256 test #%u failed!\n", i + 1);
            printf("         got: "); for (j = 0; j != sizeof(digest); ++j) printf("%02x", digest[j]); printf("\n");
            printf("    expected: "); for (j = 0; j != sizeof(digest); ++j) printf("%02x", (uint8_t) test[i].expect[j]); printf("\n");
            failed = true;
        }
    }
    return !failed ? 0 : 1;
}

static int test_sha384()
{
    struct test_values test[] = {
        /* The first 3 are from FIPS180-2 */
        { "abc", 3,
          "\xcb\x00\x75\x3f\x45\xa3\x5e\x8b\xb5\xa0\x3d\x69\x9a\xc6\x50\x07"
          "\x27\x2c\x32\xab\x0e\xde\xd1\x63\x1a\x8b\x60\x5a\x43\xff\x5b\xed"
          "\x80\x86\x07\x2b\xa1\xe7\xcc\x23\x58\xba\xec\xa1\x34\xc8\x25\xa7" },

        { "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
          "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 112,
          "\x09\x33\x0c\x33\xf7\x11\x47\xe8\x3d\x19\x2f\xc7\x82\xcd\x1b\x47"
          "\x53\x11\x1b\x17\x3b\x3b\x05\xd2\x2f\xa0\x80\x86\xe3\xb0\xf7\x12"
          "\xfc\xc7\xc7\x1a\x55\x7e\x2d\xb9\x66\xc3\xe9\xfa\x91\x74\x60\x39" },

        { sha_long_test, 1000000,
          "\x9d\x0e\x18\x09\x71\x64\x74\xcb\x08\x6e\x83\x4e\x31\x0a\x4a\x1c"
          "\xed\x14\x9e\x9c\x00\xf2\x48\x52\x79\x72\xce\xc5\x70\x4c\x2a\x5b"
          "\x07\xb8\xb3\xdc\x38\xec\xc4\xeb\xae\x97\xdd\xd8\x7f\x3d\x89\x85" },

        /* hash generated with
         * head -n 25 basic-test.c | sha384sum -b | awk '{ print $1 }' | perl -n -e 'print "\\x".join("\\x", m/../g)."\n";'
         */
        { NCBI_PDN_text, 1262,
          "\xb8\xe8\x50\xdf\xc6\x09\x20\x99\xac\x9f\x80\x1b\xd8\xa2\xf3\xa9"
          "\x75\x51\xd6\xe3\x30\x94\x2b\x33\xc8\x32\x04\xdf\xff\xd8\xa0\xb4"
          "\xfb\x8e\x9d\x5d\x0e\x97\x30\x1c\xce\xd5\x87\xf9\x77\x54\x4e\x7d" },
    };
    uint8_t digest[48];
    unsigned i;
    unsigned const n = sizeof(test)/sizeof(test[0]);
    bool failed = false;

    for (i = 0; i != n; ++i) {
        sha384(digest, test[i].test, test[i].length);
        if (memcmp(digest, test[i].expect, sizeof(digest)) != 0) {
            unsigned j;

            printf("SHA-384 test #%u failed!\n", i + 1);
            printf("         got: "); for (j = 0; j != sizeof(digest); ++j) printf("%02x", digest[j]); printf("\n");
            printf("    expected: "); for (j = 0; j != sizeof(digest); ++j) printf("%02x", (uint8_t) test[i].expect[j]); printf("\n");
            failed = true;
        }
    }
    return !failed ? 0 : 1;
}

static int test_sha512()
{
    struct test_values test[] = {
        /* The first 3 are from FIPS180-2 */
        { "abc", 3,
          "\xdd\xaf\x35\xa1\x93\x61\x7a\xba\xcc\x41\x73\x49\xae\x20\x41\x31"
          "\x12\xe6\xfa\x4e\x89\xa9\x7e\xa2\x0a\x9e\xee\xe6\x4b\x55\xd3\x9a"
          "\x21\x92\x99\x2a\x27\x4f\xc1\xa8\x36\xba\x3c\x23\xa3\xfe\xeb\xbd"
          "\x45\x4d\x44\x23\x64\x3c\xe8\x0e\x2a\x9a\xc9\x4f\xa5\x4c\xa4\x9f" },

        { "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
          "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 112,
          "\x8e\x95\x9b\x75\xda\xe3\x13\xda\x8c\xf4\xf7\x28\x14\xfc\x14\x3f"
          "\x8f\x77\x79\xc6\xeb\x9f\x7f\xa1\x72\x99\xae\xad\xb6\x88\x90\x18"
          "\x50\x1d\x28\x9e\x49\x00\xf7\xe4\x33\x1b\x99\xde\xc4\xb5\x43\x3a"
          "\xc7\xd3\x29\xee\xb6\xdd\x26\x54\x5e\x96\xe5\x5b\x87\x4b\xe9\x09" },

        { sha_long_test, 1000000,
          "\xe7\x18\x48\x3d\x0c\xe7\x69\x64\x4e\x2e\x42\xc7\xbc\x15\xb4\x63"
          "\x8e\x1f\x98\xb1\x3b\x20\x44\x28\x56\x32\xa8\x03\xaf\xa9\x73\xeb"
          "\xde\x0f\xf2\x44\x87\x7e\xa6\x0a\x4c\xb0\x43\x2c\xe5\x77\xc3\x1b"
          "\xeb\x00\x9c\x5c\x2c\x49\xaa\x2e\x4e\xad\xb2\x17\xad\x8c\xc0\x9b" },

        /* hash generated with
         * head -n 25 basic-test.c | sha512sum -b | awk '{ print $1 }' | perl -n -e 'print "\\x".join("\\x", m/../g)."\n";'
         */
        { NCBI_PDN_text, 1262,
          "\x51\x92\xdb\xb0\x5d\xcd\x18\x86\xce\x6b\x52\x1c\x1d\x38\xcc\x4d"
          "\x33\x14\x4b\x82\x96\x14\x70\xc5\xcb\x75\xc2\x10\xea\x1a\x46\x31"
          "\x91\x31\x38\xa8\xe7\x5e\xb4\xbe\xfc\xab\xa5\xc5\xa8\x3b\x23\x59"
          "\xac\xc9\x73\x94\xba\xa0\xdc\x25\x19\xb6\xa5\xeb\xf6\xa4\x4c\x87" },
    };
    uint8_t digest[64];
    unsigned i;
    unsigned const n = sizeof(test)/sizeof(test[0]);
    bool failed = false;

    for (i = 0; i != n; ++i) {
        sha512(digest, test[i].test, test[i].length);
        if (memcmp(digest, test[i].expect, sizeof(digest)) != 0) {
            unsigned j;

            printf("SHA-512 test #%u failed!\n", i + 1);
            printf("         got: "); for (j = 0; j != sizeof(digest); ++j) printf("%02x", digest[j]); printf("\n");
            printf("    expected: "); for (j = 0; j != sizeof(digest); ++j) printf("%02x", (uint8_t) test[i].expect[j]); printf("\n");
            failed = true;
        }
    }
    return !failed ? 0 : 1;
}

static int test_sha()
{
    int rslt;

    if (sha_long_test[0] != 'a') {
        FILE *fp;

        memset(sha_long_test, 'a', 1000000);
        fp = fopen("basic-test.c", "rb");
        if (fp)
        {
            fread(NCBI_PDN_text, 1262, 1, fp);
            fclose(fp);
        }
    }
    rslt  = test_sha1();
    rslt |= test_sha256();
    rslt |= test_sha384();
    rslt |= test_sha512();
    return rslt;
}

static void md5(uint8_t digest[16], const void *data, size_t size)
{
    MD5State ctx;

    MD5StateInit(&ctx);
    MD5StateAppend(&ctx, data, size);
    MD5StateFinish(&ctx, digest);
}

static int test_md5()
{
    struct test_values test[] = {
        /* hash generated with
         * echo -n "abc" | md5sum -b | awk '{ print $1 }' | perl -n -e 'print "\\x".join("\\x", m/../g)."\n";'
         */
        { "abc", 3,
          "\x90\x01\x50\x98\x3c\xd2\x4f\xb0\xd6\x96\x3f\x7d\x28\xe1\x7f\x72" },

        { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
          "\x82\x15\xef\x07\x96\xa2\x0b\xca\xaa\xe1\x16\xd3\x87\x6c\x66\x4a" },

        { "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
          "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 112,
          "\x03\xdd\x88\x07\xa9\x31\x75\xfb\x06\x2d\xfb\x55\xdc\x7d\x35\x9c" },

        /* hash generated with
         * head -n 25 basic-test.c | md5sum -b | awk '{ print $1 }' | perl -n -e 'print "\\x".join("\\x", m/../g)."\n";'
         */
        { NCBI_PDN_text, 1262,
          "\xa5\xc3\xbd\x8d\xce\x21\xa0\x5e\x68\x92\xca\x76\x5d\xfe\xbc\x4a" },
    };
    uint8_t digest[16];
    unsigned i;
    unsigned const n = sizeof(test)/sizeof(test[0]);
    bool failed = false;

    for (i = 0; i != n; ++i) {
        md5(digest, test[i].test, test[i].length);
        if (memcmp(digest, test[i].expect, sizeof(digest)) != 0) {
            unsigned j;

            printf("MD5 test #%u failed!\n", i + 1);
            printf("         got: "); for (j = 0; j != sizeof(digest); ++j) printf("%02x", digest[j]); printf("\n");
            printf("    expected: "); for (j = 0; j != sizeof(digest); ++j) printf("%02x", (uint8_t) test[i].expect[j]); printf("\n");
            failed = true;
        }
    }
    return !failed ? 0 : 1;
}

static uint8_t rev[] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff };

#if 0
static uint32_t CRC32_zip(uint32_t digest, const void *data, size_t len)
{
    static bool haveTable = false;
    static uint32_t table[256];
    const uint8_t *buf = data;
    size_t i;
    uint32_t c;

    if (!haveTable) {
        unsigned n;
        
        for (n = 0; n != 256; ++n) {
            unsigned k;
            uint32_t const poly = 0xEDB88320U;

            c = (uint32_t)n;
            for (k = 0; k != 8; ++k) {
                if (c & 1)
                    c = (c >> 1) ^ poly;
                else
                    c = (c >> 1);
            }
            table[n] = c;
        }
        haveTable = true;
    }
    for (c = digest, i = 0; i != len; ++i)
        c = table[(c & 0xFF) ^ buf[i]] ^ (c >> 8);
    return c;
}
#endif

static int test_CRC32_zip()
{
    struct {
        const char *test;
        size_t length;
        uint32_t expect;
    } test[] = {
        /* hash generated with
         * echo -n "abc" >/tmp/$$ ; crc32 /tmp/$$
         */
        { "abc", 3, 0x352441c2 },

        { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56, 0x171a3f5f },

        { "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
          "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 112, 0x191f3349 },

        /* hash generated with
         * head -n 25 basic-test.c >/tmp/$$ ; crc32 /tmp/$$
         */
        { NCBI_PDN_text, 1262, 0x5a695add },
    };
    uint32_t digest;
    unsigned i;
    unsigned j;
    unsigned const n = sizeof(test)/sizeof(test[0]);
    bool failed = false;

    CRC32Init();

    for (i = 0; i != n; ++i) {
        size_t k = test[i].length;

#if 1
        for (j = 0, digest = ~0; j != k; ++j) {
            uint8_t ch = rev[(uint8_t)test[i].test[j]];
            digest = CRC32(digest, &ch, 1);
        }
        digest = ~digest;
        digest =
            (rev[(digest >>  0) & 0xFF] << 24) |
            (rev[(digest >>  8) & 0xFF] << 16) |
            (rev[(digest >> 16) & 0xFF] <<  8) |
            (rev[(digest >> 24) & 0xFF] <<  0);
#else
        digest = ~CRC32_zip(~0, test[i].test, k);
#endif
        if (digest != test[i].expect) {
            printf("CRC32 test #%u failed!\n", i + 1);
            printf("         got: %08x\n", digest);
            printf("    expected: %08x\n", test[i].expect);
            failed = true;
        }
    }
    return !failed ? 0 : 1;
}

static int test_CRC32_cksum()
{
    struct {
        const char *test;
        size_t length;
        uint32_t expect;
    } test[] = {
        /* hash generated with
         * echo -n "abc" | cksum |  awk '{ print $1 }' | perl -n -e 'printf "%08x\n", $_'
         */
        { "abc", 3, 0x48aa78a2 },

        { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56, 0x97d32c84 },

        { "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
          "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 112, 0x80edd7ce },

        /* hash generated with
         * head -n 25 basic-test.c | cksum | awk '{ print $1 }' | perl -n -e 'printf "%08x\n", $_'
         */
        { NCBI_PDN_text, 1262, 0x1ff54ded },
    };
    uint32_t digest;
    unsigned i;
    unsigned const n = sizeof(test)/sizeof(test[0]);
    bool failed = false;

    CRC32Init();

    for (i = 0; i != n; ++i) {
        size_t k = test[i].length;

        digest = CRC32(0, test[i].test, k);
        while (k) {
            uint8_t ch = k & 0xFF;
            k >>= 8;
            digest = CRC32(digest, &ch, 1);
        }
        digest = ~digest;
        if (digest != test[i].expect) {
            printf("CRC32 test #%u failed!\n", i + 1);
            printf("         got: %08x\n", digest);
            printf("    expected: %08x\n", test[i].expect);
            failed = true;
        }
    }
    return !failed ? 0 : 1;
}


ver_t CC KAppVersion (void)
{
    return 0;
}

const char UsageDefaultName[] = "basic-checksum-test";

rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        " %s [Options]\n"
        "\n"
        "Summary:\n"
        "  test basic checksum stuff\n"
        "\n", progname );
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


rc_t CC KMain ( int argc, char *argv [] )
{
    rc_t rc = 0;
    Args * args;

    rc = ArgsMakeAndHandle (&args, argc, argv, 0);
    if (rc == 0)
    {
        if (test_sha() == 0)
            puts("SHA: basic tests passed");
        else
            rc = 1;
        if (test_md5() == 0)
            puts("MD5: basic tests passed");
        else
            rc |= 2;
        if (test_CRC32_zip() == 0)
            puts("CRC32 Zip: basic tests passed");
        else
            rc |= 4;
        if (test_CRC32_cksum() == 0)
            puts("CRC32 cksum: basic tests passed");
        else
            rc |= 8;
    }
    return rc;
}
