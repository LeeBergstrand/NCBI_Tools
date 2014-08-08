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

#include "test-rowlen.vers.h"

#include <vdb/cursor.h>
#include <vdb/manager.h>
#include <vdb/schema.h>
#include <vdb/table.h>

#include <klib/log.h>
#include <klib/rc.h>
#include <klib/text.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>


#define LIMIT 1000
#define BUFSIZE 6400

int64_t Limit = LIMIT;



#define TABLE_NAME "table_row_len"

bool verbose = false;
bool failed = false;

const char * schema_path = TABLE_NAME;
const char schema_text[] =
    "version 1;"
    "include 'vdb/vdb.vschema';"
    "table " TABLE_NAME " #1.0"
    "{"
    " column   B1[3] dat3   = .dat3;"
    " physical B1[3] .dat3  = dat3;"
    " column   U32   len3   = .len3;"
    " physical U32   .len3  = len3;"
    " column   U32   clen3  = row_len (dat3);"
    " column   U32   plen3  = row_len (.dat3);"
    " column   U64   dat64  = .dat64;"
    " physical U64   .dat64 = dat64;"
    " column   U32   len64  = .len64;"
    " physical U32   .len64 = len64;"
    " column   U32   clen64 = row_len (dat64);"
    " column   U32   plen64 = row_len (.dat64);"
    "};";

rc_t open_table (VDBManager * mgr, VSchema * schema, VTable ** table)
{
    rc_t rc;
    rc = VDBManagerVCreateTable (mgr, table, schema, TABLE_NAME, kcmInit, schema_path, NULL);
    if (rc)
    {
        LOGERR (klogErr, rc, "Failed to create/init Table");
    }
    return rc;
}

rc_t open_write_cursor (VTable * table, VCursor ** cursor,
                        uint32_t * dat, uint32_t * len, 
                        const char * dat_name, const char* len_name)
{
    rc_t rc;

    rc = VTableCreateCursorWrite (table, cursor, kcmInsert);
    if (rc)
    {
        LOGERR (klogErr, rc, "Failed to create Cursor");
        *cursor = NULL;
    }
    else
    {
        do
        {
            rc = VCursorAddColumn (*cursor, dat, dat_name);
            if (rc)
            {
                pLOGERR (klogErr, rc, "Failed to add column $(C)", PLOG_S(C), dat_name);
                break;
            }

            rc = VCursorAddColumn (*cursor, len, len_name);
            if (rc)
            {
                pLOGERR (klogErr, rc, "Failed to add column $(C)", PLOG_S(C), len_name);
                break;
            }       

            rc = VCursorOpen (*cursor);
            if (rc)
            {
                LOGERR (klogErr, rc, "Failed to open cursor");
                break;
            }

            return 0;
        }
        while (0);
    }
    VCursorRelease (*cursor);
    *dat = 0;
    *len = 0;
    *cursor = NULL;
    return rc;
}

rc_t open_read_cursor (const VTable * table, const VCursor ** cursor,
                       uint32_t * idx1, uint32_t * idx2, uint32_t * idx3, uint32_t * idx4,
                       const char * name1, const char * name2, const char * name3, const char * name4)
{
    rc_t rc;

    rc = VTableCreateCursorRead (table, cursor);
    if (rc)
    {
        LOGERR (klogErr, rc, "Failed to create Cursor");
    }
    else
    {
        do
        {
            rc = VCursorAddColumn (*cursor, idx1, name1);
            if (rc)
            {
                pLOGERR (klogErr, rc, "Failed to add column $(C)", PLOG_S(C), name1);
                break;
            }

            rc = VCursorAddColumn (*cursor, idx2, name2);
            if (rc)
            {
                pLOGERR (klogErr, rc, "Failed to add column $(C)", PLOG_S(C), name2);
                break;
            }

            rc = VCursorAddColumn (*cursor, idx3, name3);
            if (rc)
            {
                pLOGERR (klogErr, rc, "Failed to add column $(C)", PLOG_S(C), name3);
                break;
            }

            rc = VCursorAddColumn (*cursor, idx4, name4);
            if (rc)
            {
                pLOGERR (klogErr, rc, "Failed to add column $(C)", PLOG_S(C), name4);
                break;
            }

            rc = VCursorOpen (*cursor);
            if (rc)
            {
                LOGERR (klogErr, rc, "Failed to open cursor");
                break;
            }

            return 0;
        }
        while (0);

        VCursorRelease (*cursor);
    }
    *idx1 = *idx2 = *idx3 = *idx4 = 0;
    *cursor = NULL;
    return rc;
}

rc_t close_cursor (const VTable * table, const VCursor * cursor)
{
    rc_t rc1, rc2;
    rc1 = VCursorRelease (cursor);
    rc2 = VTableRelease (table);
    return (rc1?rc1:rc2);
}

void static_3_bit (uint64_t ix, uint8_t * data, uint32_t * len)
{
    *data = 1;
    *len = 1;
}
void static_64_bit (uint64_t ix, uint8_t * data, uint32_t * len)
{
    *(uint64_t*)data = 1;
    *len = 1;
}
void fixed_3_bit (uint64_t ix, uint8_t * data, uint32_t * len)
{
    *data = ix;
    *len = 2;
}
void fixed_64_bit (uint64_t ix, uint8_t * data, uint32_t * len)
{
    uint64_t * ldata;
    ldata = (uint64_t *)data;

    ldata[0] = ix;
    ldata[1] = ~ix;
    *len = 2;
}
void variable_3_bit (uint64_t ix, uint8_t * data, uint32_t * len)
{
    int jx;
    for (jx = 0; jx < BUFSIZE; ++jx)
        data[jx] = (uint8_t)jx;

    *len = ix % ((BUFSIZE * 8)/3);
}
void variable_64_bit (uint64_t ix, uint8_t * data, uint32_t * len)
{
    uint64_t * ldata;
    int jx;

    ldata = (uint64_t *)data;
    for (jx = 0; jx < (BUFSIZE / sizeof (uint64_t)); ++jx)
        ldata [jx] = jx + ix;
    *len = ix % (BUFSIZE / sizeof (uint64_t));
}

typedef void (*FUNC)(uint64_t, uint8_t*, uint32_t*);

typedef struct test_params
{
    const char * test_name;
    const char * dat_name;
    const char * len_name;
    const char * clen_name;
    const char * plen_name;
    int64_t      bits;
    FUNC         func;
} test_params;


rc_t run_test (VTable * table, test_params * pb)
{
    VCursor * cursor;
    const VCursor * rcursor;
    int64_t ix;
    int64_t rowid;
    uint32_t dat;
    uint32_t len;
    uint32_t clen;
    uint32_t plen;
    rc_t rc;
    rc_t orc;
    uint8_t b [BUFSIZE];

    cursor = NULL;

    do
    {
        if (verbose)
            printf ("%s call open_write_cursot\n", __func__);
        rc = open_write_cursor (table, &cursor,
                                &dat, &len,
                                pb->dat_name, pb->len_name);
        if (rc)
        {
            LOGERR (klogDebug1, rc, "failed to create write cursor");
            cursor = NULL;
            break;
        }
        for (ix = 0; ix < Limit; ++ix)
        {
            if (verbose)
                printf ("%s call VCursorOpenRow\n", __func__);
            rc = VCursorOpenRow (cursor);
            if (rc)
            {
                LOGERR (klogErr, rc, "Failed to Open Cursor");
                break;
            }
            else
            {
                uint32_t c[1];

                pb->func(ix, b, c);
                if (verbose)
                    printf ("%s call VCursorWrite %" LD64 "\n", __func__, ix);
                rc = VCursorWrite (cursor, dat, pb->bits, b, 0, *c);
                if (rc)
                {
                    pLOGERR (klogErr, rc, "Write fail dat row $(R)", PLOG_I64(R), ix);
                    break;
                }

                if (verbose)
                    printf ("%s call VCursorWrite %" LD64 "\n", __func__, ix);
                rc = VCursorWrite (cursor, len, 32, &c, 0, 1);
                if (rc)
                {
                    pLOGERR (klogErr, rc, "Write fail len row $(R)", PLOG_I64(R), ix);
                    break;
                }

                if (verbose)
                    printf ("%s call VCursorCommitRow\n", __func__);
                rc = VCursorCommitRow (cursor);
                if (rc)
                {
                    pLOGERR (klogErr, rc, "Commit fail row $(R)", PLOG_I64(R), ix);
                    break;
                }

                if (verbose)
                    printf ("%s call VCursorCloseRow\n", __func__);
                rc = VCursorCloseRow (cursor);
                if (rc)
                {
                    pLOGERR (klogErr, orc, "Commit fail row $(R)", PLOG_I64(R), ix);
                    break;
                }
            }
            if (rc)
                break;
        } /* for (ix = 0; ix < Limit; ++ix) */
        if (ix != Limit)
            fprintf (stderr, "Quit early %d\n", (int)ix);
        if (rc)
        {
            pLOGERR (klogInfo, rc, "failed in loop $(T) $(R)", 
                     PLOG_2(PLOG_S(T),PLOG_I64(R)), pb->test_name, ix);
        }
        else
        {
            if (verbose)
                printf ("%s call VCursorCommit\n", __func__);
            orc = VCursorCommit (cursor);
            if (orc && (rc == 0))
                rc = orc;
        }
        if (verbose)
            printf ("%s call VCursorRelease\n", __func__);
        orc = VCursorRelease (cursor);
        if (orc && (rc == 0))
            rc = orc;
        if (rc)
            break;

        if (verbose)
            printf ("%s call open_read_cursor\n",__func__);
        rc = open_read_cursor (table, &rcursor, 
                               &len, &plen, &clen, &dat,
                               pb->len_name, pb->plen_name, pb->clen_name, pb->dat_name);
        if (rc)
        {
            LOGERR (klogErr, rc, "failed to open read cursor");
            break;
        }

        for (ix = 0; ix < Limit; ++ix)
        {
            uint32_t l;
            uint32_t p;
            uint32_t c;
            uint32_t r;
            uint32_t x;

            rc = VCursorRowId (rcursor, &rowid);
            if (rc)
            {
                pLOGERR (klogErr, rc, "failed to get rowid $(R)", PLOG_I64(R), ix);
                break;
            }

            if (rowid != ix+1)
            {
                fprintf (stderr, "ROWID failure %" LD64 ":%" LD64 "\n", ix, rowid);
                failed = true;
            }

            rc = VCursorOpenRow (rcursor);
            if (rc)
            {
                pLOGERR (klogErr, rc, "failed to open row $(R)", PLOG_I64(R), ix);
                break;
            }

            rc = VCursorRead (rcursor, len, 32, &l, 1, &r);
            if (rc)
            {
                pLOGERR (klogErr, rc, "failed to read column $(N) $(R)", PLOG_2(PLOG_S(N),PLOG_I64(R)), pb->len_name, ix);
                break;
            }

            rc = VCursorRead (rcursor, clen, 32, &c, 1, &r);
            if (rc)
            {
                pLOGERR (klogErr, rc, "failed to read column $(N) $(R)", PLOG_2(PLOG_S(N),PLOG_I64(R)), pb->clen_name, ix);
                break;
            }

            rc = VCursorRead (rcursor, plen, 32, &p, 1, &r);
            if (rc)
            {
                pLOGERR (klogErr, rc, "failed to read column $(N) $(R)", PLOG_2(PLOG_S(N),PLOG_I64(R)), pb->plen_name, ix);
                break;
            }

/*            rc = VCursorReadBits (rcursor, dat, pb->bits, 0, b, 0, (BUFSIZE*8)/pb->bits, &r, &x);
            if (rc)
            {
                pLOGERR (klogErr, rc, "failed to read column $(N) $(R)", PLOG_2(PLOG_S(N),PLOG_I64(R)), pb->dat_name, ix);
                break;
            }
*/

            VCursorCloseRow (rcursor);

            if (l != p)
            {
                fprintf (stderr, "error in physical column row_len() %u != %u\n", l, p);
                failed = true;
            }
            if (l != c)
            {
                fprintf (stderr, "error in physical column row_len() %u != %u\n", l, c);
                failed = true;
            }
        }

        if (verbose)
            printf ("%s call VCursorRelease\n",__func__);
        orc = VCursorRelease (rcursor);
        if (orc)
        {
            LOGERR (klogErr, rc, "release was funky");
        }
        if (orc && (rc == 0))
            rc = orc;
        if (rc)
            break;

    }
    while (0);
    return rc;
}
#define S(P) #P
#define TEST(B,T) { S(P) " bit " S(T), "dat" S(B), "len" S(B), "clen" S(B), "plen" S(B), B, T##_##B##_bit }
test_params tests [6] = 
{
    TEST(3,static),
    TEST(64,static),
    TEST(3,fixed),
    TEST(64,fixed),
    TEST(3,variable),
    TEST(64,variable)
};
rc_t run_tests (void)
{
    VDBManager * mgr;
    rc_t rc;

    if (verbose)
        printf("%s call VDBManagerMakeUpdate\n", __func__);
    rc = VDBManagerMakeUpdate (&mgr, NULL);
    if (rc)
    {
        LOGERR (klogInt, rc, "Failed to open VDBManager");
        return rc;
    }
    else
    {
        VSchema  * schema;

        if (verbose)
            printf("%s call VDBManagerMakeSchema\n", __func__);
        rc = VDBManagerMakeSchema (mgr, &schema);
        printf("%s schema == %p\n", __func__, (void*)schema);
        if (rc)
            LOGERR (klogInt, rc, "Failed to make empty schema");
        else
        {
            if(verbose)
                printf("%s call VSchemaParseText\n", __func__);
            rc = VSchemaParseText (schema, "rowlen_schema", schema_text, string_size (schema_text));
            if (rc)
                LOGERR (klogInt, rc, "Failed to parse internal schema");
            else
            {
                int ix;

                for ( ix = 0; ix < 6; ++ix)
                {
                    VTable * table;
                    rc_t orc;

                    if (verbose)
                        printf("%s call open_table\n", __func__);
                    rc = open_table (mgr, schema, &table);
                    if (rc)
                    {
                        LOGERR (klogErr, rc, "Failed to open table");
                        break;
                    }

                    if (verbose)
                        printf("%s call run_test\n", __func__);
                    rc = run_test (table, &tests[ix]);
                    if (rc)
                    {
                        pLOGERR (klogErr, rc, "Failed $(D)", PLOG_S(D), tests[ix].test_name);
                    }

                    if (verbose)
                        printf("%s call VTableRelease\n", __func__);
                    orc = VTableRelease (table);
                    if (orc)
                    {
                        LOGERR (klogErr, rc, "failed to close table");
                    }
                    if (orc && (rc == 0))
                        rc = orc;
                    if (rc)
                        break;
                }
            }

            if (verbose)
                printf("%s call VSchemaRelease\n", __func__);
            VSchemaRelease (schema);
        }
        if (verbose)
            printf("%s call VDBManagerRelease\n", __func__);
        VDBManagerRelease (mgr);
    }
    return rc;
}

/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
uint32_t KAppVersion (void)
{
    return TEST_ROWLEN_VERS;
}
    
/* KMain - EXTERN
 *  executable entrypoint "main" is implemented by
 *  an OS-specific wrapper that takes care of establishing
 *  signal handlers, logging, etc.
 *
 *  in turn, OS-specific "main" will invoke "KMain" as
 *  platform independent main entrypoint.
 *
 *  "argc" [ IN ] - the number of textual parameters in "argv"
 *  should never be < 0, but has been left as a signed int
 *  for reasons of tradition.
 *
 *  "argv" [ IN ] - array of NUL terminated strings expected
 *  to be in the shell-native character set: ASCII or UTF-8
 *  element 0 is expected to be executable identity or path.
 */
void Usage (const char *app_name)
{
    printf ("\n"
            "  test built-in row_len function by creating several columns with \n"
            "  an assortment of types and deleting them after the test is over\n"
            "  there are no command line parameters other than help and version.\n"
            "Usage:\n"
            "  $ %s\\\n"
            "      -h,-?,--help    print this message\n"
            "      -V,--version    print and return program version number\n"
            "      -v --verbose    print extra messages showing progress\n",
            app_name
        );
}


rc_t KMain (int argc, char *argv[])
{
    int ix;
    rc_t rc;

    if (argc != 1)
    {
        for (ix = 1; ix < argc; ++ix)
        {

/* might add logging and verbose options in the future */

            if ((strcmp (argv[ix], "-V") == 0) ||
                (strcmp (argv[ix], "--version") == 0))
            {
                rc = KAppVersion();
            }
            
            else if ((strcmp (argv[ix], "-?") == 0) ||
                     (strcmp (argv[ix], "-h") == 0) ||
                     (strcmp (argv[ix], "--help") == 0))
            {
                Usage (argv[0]);
            }

            if ((strcmp (argv[ix], "-v") == 0) ||
                (strcmp (argv[ix], "--verbose") == 0))
            {
                verbose = true;
                rc = KAppVersion();
            }

/*
            if ((strcmp (argv[ix], "-k") == 0) ||
                (strcmp (argv[ix], "--iterations") == 0))
            {
                Limit = atol (
                verbose = true;
                rc = KAppVersion();
            }
*/
            else
            {
                rc = RC (rcExe, rcArgv, rcReading, rcParam, rcUnknown);
                pLOGERR (klogErr, rc, "unknown parameter $(p)", PLOG_S(p), argv[ix]);
                return rc;
            }
        }
    }
    return run_tests ();
}

