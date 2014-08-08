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

#include <kapp/main.h>
#include <kapp/args.h>
#include <klib/out.h>
#include <klib/rc.h>
#include <vdb/manager.h>
#include <vdb/table.h>
#include <vdb/schema.h>
#include <vdb/cursor.h>
#include <kdb/meta.h>

#include <stdio.h>
#include <string.h>

#include <common/test_assert.h>

static const char schema[] =
"version 1; "
"include 'vdb/vdb.vschema'; "
"table checksum_test #1 { "
"   extern column U8 DATA; "
"   trigger CHECKSUM_CRC    = checksum < 'CRC-SUM',    'crc32'   > ( DATA ); "
"   trigger CHECKSUM_MD5    = checksum < 'MD5-SUM',    'md5'     > ( DATA ); "
"   trigger CHECKSUM_SHA1   = checksum < 'SHA1-SUM',   'sha1'    > ( DATA ); "
"   trigger CHECKSUM_SHA256 = checksum < 'SHA256-SUM', 'sha-256' > ( DATA ); "
"   trigger CHECKSUM_SHA384 = checksum < 'SHA384-SUM', 'sha-384' > ( DATA ); "
"   trigger CHECKSUM_SHA512 = checksum < 'SHA512-SUM', 'sha-512' > ( DATA ); "
"}";

static char *line[25];
static char data[4000];

static rc_t WriteTable(VTable *tbl)
{
    VCursor *curs;
    rc_t rc = VTableCreateCursorWrite(tbl, &curs, kcmInsert);

    while (rc == 0) {
        uint32_t col;
        unsigned i;

        rc = VCursorAddColumn(curs, &col, "DATA");
        if (rc) break;
        rc = VCursorOpen(curs);
        if (rc) break;
        
        for (i = 0; i != 25; ++i) {
            rc = VCursorOpenRow(curs);
            if (rc) break;
            rc = VCursorWrite(curs, col, 8, line[i], 0, strlen(line[i]));
            if (rc) break;
            rc = VCursorCommitRow(curs);
            if (rc) break;
            rc = VCursorCloseRow(curs);
            if (rc) break;
        }
        if (rc) break;
        VCursorCommit(curs);
        break;
    }
    VCursorRelease(curs);
    return rc;
}

static rc_t TestTable(VDBManager *mgr)
{
    rc_t rc;
    char buf[1024];
    size_t nread;
    size_t remain;
    const VTable *tbl;

    rc = VDBManagerOpenTableRead(mgr, &tbl, NULL, "checksum_test");
    VDBManagerRelease(mgr);
    if (rc == 0) {
        const KMetadata *meta;

        rc = VTableOpenMetadataRead(tbl, &meta);
        VTableRelease(tbl);
        if (rc == 0) {
            const KMDataNode *node;

            rc = KMetadataOpenNodeRead(meta, &node, "CRC-SUM");
            if (rc == 0) {
                rc = KMDataNodeRead(node, 0, buf, sizeof(buf), &nread, &remain);
                if (rc == 0) {
                    if (nread == 4 && memcmp(buf, "\x1f\xf5\x4d\xed", 4) == 0) {
                        printf("crc-32 test passed\n");
                    }
                    else {
                        printf("crc-32 test failed\n");
                    }
                }
                else {
                    printf("crc-32 test failed to read metanode: %u\n", rc);
                }
                KMDataNodeRelease(node);
            }
            else {
                printf("crc-32 test failed to open metanode: %u\n", rc);
            }
            rc = KMetadataOpenNodeRead(meta, &node, "MD5-SUM");
            if (rc == 0) {
                rc = KMDataNodeRead(node, 0, buf, sizeof(buf), &nread, &remain);
                if (rc == 0) {
                    if (nread == 16 && memcmp(buf, "\xa5\xc3\xbd\x8d\xce\x21\xa0\x5e\x68\x92\xca\x76\x5d\xfe\xbc\x4a", 16) == 0) {
                        printf("MD5 test passed\n");
                    }
                    else {
                        printf("MD5 test failed\n");
                    }
                }
                else {
                    printf("MD5 test failed to read metanode: %u\n", rc);
                }
                KMDataNodeRelease(node);
            }
            else {
                printf("MD5 test failed to open metanode: %u\n", rc);
            }
            rc = KMetadataOpenNodeRead(meta, &node, "SHA1-SUM");
            if (rc == 0) {
                rc = KMDataNodeRead(node, 0, buf, sizeof(buf), &nread, &remain);
                if (rc == 0) {
                    if (nread == 20 && memcmp(buf, "\x81\x06\x56\xa2\xbf\x75\x8b\x7c\x22\x83\x88\xd5\x6a\x4a\x3a\x01\xa5\x2f\xc7\xc3", 20) == 0) {
                        printf("SHA1 test passed\n");
                    }
                    else {
                        printf("SHA1 test failed\n");
                    }
                }
                else {
                    printf("SHA1 test failed to read metanode: %u\n", rc);
                }
                KMDataNodeRelease(node);
            }
            else {
                printf("SHA1 test failed to open metanode: %u\n", rc);
            }
            rc = KMetadataOpenNodeRead(meta, &node, "SHA256-SUM");
            if (rc == 0) {
                rc = KMDataNodeRead(node, 0, buf, sizeof(buf), &nread, &remain);
                if (rc == 0) {
                    if (nread == 32 && memcmp(buf,
                                              "\x30\xbf\x9f\x65\xdd\xcd\x5c\x6a\xd0\x24\x4a\xf5\xd8\x7d\x1a\x9f"
                                              "\x92\x0c\x25\x84\xf1\xaa\xc5\x1c\xd0\xef\x90\xe2\xb9\x0e\x2b\x10", 32) == 0) {
                        printf("SHA-256 test passed\n");
                    }
                    else {
                        printf("SHA-256 test failed\n");
                    }
                }
                else {
                    printf("SHA-256 test failed to read metanode: %u\n", rc);
                }
                KMDataNodeRelease(node);
            }
            else {
                printf("SHA-256 test failed to open metanode: %u\n", rc);
            }
            rc = KMetadataOpenNodeRead(meta, &node, "SHA384-SUM");
            if (rc == 0) {
                rc = KMDataNodeRead(node, 0, buf, sizeof(buf), &nread, &remain);
                if (rc == 0) {
                    if (nread == 48 && memcmp(buf,
                                              "\xb8\xe8\x50\xdf\xc6\x09\x20\x99\xac\x9f\x80\x1b\xd8\xa2\xf3\xa9"
                                              "\x75\x51\xd6\xe3\x30\x94\x2b\x33\xc8\x32\x04\xdf\xff\xd8\xa0\xb4"
                                              "\xfb\x8e\x9d\x5d\x0e\x97\x30\x1c\xce\xd5\x87\xf9\x77\x54\x4e\x7d", 48) == 0) {
                        printf("SHA-384 test passed\n");
                    }
                    else {
                        printf("SHA-384 test failed\n");
                    }
                }
                else {
                    printf("SHA-384 test failed to read metanode: %u\n", rc);
                }
                KMDataNodeRelease(node);
            }
            else {
                printf("SHA-384 test failed to open metanode: %u\n", rc);
            }
            rc = KMetadataOpenNodeRead(meta, &node, "SHA512-SUM");
            if (rc == 0) {
                rc = KMDataNodeRead(node, 0, buf, sizeof(buf), &nread, &remain);
                if (rc == 0) {
                    if (nread == 64 && memcmp(buf,
                                              "\x51\x92\xdb\xb0\x5d\xcd\x18\x86\xce\x6b\x52\x1c\x1d\x38\xcc\x4d"
                                              "\x33\x14\x4b\x82\x96\x14\x70\xc5\xcb\x75\xc2\x10\xea\x1a\x46\x31"
                                              "\x91\x31\x38\xa8\xe7\x5e\xb4\xbe\xfc\xab\xa5\xc5\xa8\x3b\x23\x59"
                                              "\xac\xc9\x73\x94\xba\xa0\xdc\x25\x19\xb6\xa5\xeb\xf6\xa4\x4c\x87", 64) == 0) {
                        printf("SHA-512 test passed\n");
                    }
                    else {
                        printf("SHA-512 test failed\n");
                    }
                }
                else {
                    printf("SHA-512 test failed to read metanode: %u\n", rc);
                }
                KMDataNodeRelease(node);
            }
            else {
                printf("SHA-512 test failed to open metanode: %u\n", rc);
            }
            rc = 0;
        }
    }
    return rc;
}

static rc_t test(void)
{
    VTable *tbl;
    VDBManager *mgr = NULL;
    rc_t rc = VDBManagerMakeUpdate(&mgr, NULL);
    
    if (rc == 0) {
        VSchema *sch;
        
        rc = VDBManagerMakeSchema(mgr, &sch);
        if (rc == 0) {
            rc = VSchemaParseText(sch, NULL, schema, sizeof(schema) - 1);
            if (rc == 0)
                rc = VDBManagerCreateTable(mgr, &tbl, sch, "checksum_test", kcmInit, "checksum_test");
            VSchemaRelease(sch);
        }
    }
    if (rc == 0) {
        rc = WriteTable(tbl);
        VTableRelease(tbl);
    }
    if (rc == 0)
        rc = TestTable(mgr);
    VDBManagerRelease(mgr);
    return rc;
}

ver_t CC KAppVersion (void)
{
    return 0;
}

const char UsageDefaultName[] = "vxf-checksum-test";

rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        " %s [Options]\n"
        "\n"
        "Summary:\n"
        "  test vxf checksum stuff\n"
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
        FILE *fp;
        unsigned i;
        unsigned n;

        fp = fopen("vdb-test.c", "r");
        if (fp == NULL) {
            perror("failed to open vdb-test.c");
            return 0;
        }
        for (n = 0, i = 0; i != 25; ++i) {
            if (fgets(data + n, sizeof(data) - n, fp) == NULL) {
                perror("failed to read vdb-test.c");
                return 0;
            }
            line[i] = data + n;
            n += strlen(line[i]) + 1;
            if (n >= sizeof(data)) {
                fputs("too many bytes\n", stderr);
                return 0;
            }
        }
        rc = test();
        if (rc != 0) {
            fprintf(stderr, "test failed with rc: %u\n", rc);
            return 1;
        }
    }
    return rc;
}
