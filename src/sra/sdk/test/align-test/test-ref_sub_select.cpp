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
#include <ktst/unit_test.hpp>

#include <cstdlib>
#include <klib/printf.h>
#include <klib/callconv.h>
#include <klib/debug.h>
#include <klib/out.h>
#include <klib/log.h>
#include <klib/rc.h>
#include <klib/checksum.h>
#include <kapp/main.h>
#include <kapp/args.h>
#include <kfs/directory.h>
#include <kfs/file.h>
#include <kdb/manager.h>
#include <vdb/manager.h>
#include <vdb/database.h>
#include <vdb/schema.h>
#include <vdb/table.h>
#include <insdc/insdc.h>
#include <align/writer-reference.h>
#include <align/refseq-mgr.h>
#include <sra/srapath.h>

#include <os-native.h>
#include <sysalloc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strtol.h>
#include <ctype.h>
#include <time.h>

#include <common/test_assert.h>

// hack - since tests are for functions local for a particular source file
#include "../../libs/axf/ref-tbl.c"
#include "../../libs/axf/ref-tbl-sub-select.c"

TEST_SUITE_WITH_USAGE(RefTableSubSelect_TestSuite);

#define DBG(flag,msg) DBGMSG(DBG_APP,DBG_FLAG(DBG_APP_##flag), msg)

const char* output_path = NULL;
time_t seed = 0;

typedef struct {
    const char* id;
    INSDC_coord_zero offset;
    const char* seq;
    const void* cigar;
    int64_t row_id;
    /* for evidence only */
    INSDC_coord_zero allele_offset;
    const char* allele;
    const void* allele_cigar;
} TAlgRec;

class RefTableSubSelectFixture {

    VDBManager* m_vmgr;
    VSchema* m_schema;
    VDatabase* m_db;
    char m_path[4096];
    KDirectory* m_dir;
    const VDatabase* m_db_r;

protected:
    static const size_t sm_cursor_cache_size = 1024 * 1024 * 1024;
    static const uint32_t sm_max_seq_len = 1000;

    const ReferenceMgr* m_rmgr;
    RefTableSubSelect* m_rss;
    const RefSeqMgr* m_refseqmgr;

public:
    RefTableSubSelectFixture(void)
        : m_vmgr(0), m_schema(0), m_db(0), m_dir(0), m_db_r(0), m_rmgr(0), m_rss(0), m_refseqmgr(0)
    {
        size_t w;
        string_printf(m_path, sizeof(m_path), &w, "%s/RefTableSubSelectTest-%u", output_path, seed);
    }

    ~RefTableSubSelectFixture(void) {
        VDBManagerRelease(m_vmgr);
        KDirectoryRelease(m_dir);
    }

    rc_t InitW()
    {
        rc_t rc = 0;
        KDirectory* d = 0;

        if( (rc = KDirectoryNativeDir(&d)) == 0 &&
            (rc = KDirectoryCreateDir(d, 0777, kcmInit, "%s", m_path)) == 0 &&
            (rc = KDirectoryOpenDirUpdate(d, &m_dir, true, m_path)) == 0 &&
            (rc = VDBManagerMakeUpdate(&m_vmgr, NULL)) == 0 &&
            (rc = VDBManagerMakeSchema(m_vmgr, &m_schema)) == 0 &&
            (rc = VSchemaParseFile(m_schema, "align/align.vschema")) == 0 ) {
            rc = VDBManagerCreateDB(m_vmgr, &m_db, m_schema, "NCBI:align:db:alignment_evidence", kcmInit, "%s/db", m_path);
        }
        KDirectoryRelease(d);
        return rc;
    }

    rc_t FiniW(void)
    {
        rc_t rc1 = ReferenceMgr_Release(m_rmgr, true, NULL, false);
        rc_t rc2 = VDatabaseRelease(m_db);
        VSchemaRelease(m_schema);
        return rc1 ? rc1 : rc2;
    }

    rc_t WriteCfg(const char* cfg)
    {
        rc_t rc = 0;
        KFile* f;
        if( (rc = KDirectoryCreateFile(m_dir, &f, false, 0777, kcmInit, "ref.cfg")) == 0 ) {
            rc = KFileWrite(f, 0, cfg, strlen(cfg), NULL);
            KFileRelease(f);
        }
        return rc;
    }

    rc_t Mgr(void) {
        return ReferenceMgr_Make(&m_rmgr, m_db, m_vmgr, 0, "ref.cfg", m_path, sm_max_seq_len, sm_cursor_cache_size, 0);
    }

    rc_t Write(TAlgRec* r, ETableWriterAlgn_TableType type, uint32_t options)
    {
        rc_t rc = 0;
        const TableWriterAlgn* algn;
        if( (rc = TableWriterAlgn_Make(&algn, m_db, type, options)) == 0 ) {
            TableWriterAlgnData data;
            struct {
                INSDC_coord_zero read_start[10];
                INSDC_coord_len read_len[10];
                bool has_ref_offset[1024];
                int32_t ref_offset[1024];
                bool has_mismatch[1024];
                char mismatch[1024];
                int64_t ref_id;
                INSDC_coord_zero ref_start;
                int64_t seq_spot_id;
                INSDC_coord_one seq_read_id;
                bool ref_orientation;
                uint32_t ref_ploidy;
                uint32_t mapq;
            } tmp;

            memset(&data, 0, sizeof(data));

            data.seq_spot_id.buffer = &tmp.seq_spot_id;
            data.seq_spot_id.elements = 1;
            data.seq_read_id.buffer = &tmp.seq_read_id;
            data.seq_read_id.elements = 1;
            data.read_start.buffer = &tmp.read_start;
            data.read_len.buffer = &tmp.read_len;
            data.has_ref_offset.buffer = tmp.has_ref_offset;
            data.ref_offset.buffer = tmp.ref_offset;
            data.ref_id.buffer = &tmp.ref_id;
            data.ref_start.buffer = &tmp.ref_start;
            data.has_mismatch.buffer = tmp.has_mismatch;
            data.mismatch.buffer = tmp.mismatch;
            data.ref_orientation.buffer = &tmp.ref_orientation;
            data.ref_orientation.elements = 1;
            data.ref_ploidy.buffer = &tmp.ref_ploidy;
            data.ref_ploidy.elements = 1;
            data.mapq.buffer = &tmp.mapq;
            data.mapq.elements = 1;

            const char* prev = NULL;
            INSDC_coord_zero prev_o = 0;
            int64_t row_id = 1;
            while( rc == 0 && r->id != 0 ) {
                if( prev == NULL || (strcmp(prev, r->id) != 0 || prev_o != r->offset) ) {
                    if( prev != NULL ) {
                        rc = TableWriterAlgn_Write(algn, &data, NULL);
                        row_id++;
                    }
                    memset(&tmp, 0, sizeof(tmp));
                    data.ploidy = 0;
                    prev = r->id;
                    prev_o = r->offset;
                }
                r->row_id = row_id;
                if( rc == 0 ) {
                    rc = ReferenceMgr_Compress(m_rmgr, 0, r->id, r->offset, r->seq, strlen((const char*)r->seq),
                                               r->cigar, strlen((const char*)r->cigar),
                                               r->allele_offset, r->allele, r->allele ? strlen((const char*)r->allele) : 0, 0,
                                               r->allele_cigar, r->allele_cigar ? strlen((const char*)r->allele_cigar) : 0, &data);
                }
                r++;
            }
            if( rc == 0 ) {
                rc = TableWriterAlgn_Write(algn, &data, NULL);
            }
            rc_t rc1 = TableWriterAlgn_Whack(algn, rc == 0, NULL);
            rc = rc ? rc : rc1;
        }
        return rc;
    }

    rc_t InitR()
    {
        rc_t rc = RefSeqMgr_Make(&m_refseqmgr, m_vmgr, 0, sm_cursor_cache_size, 0);
        return rc ? rc : VDBManagerOpenDBRead(m_vmgr, &m_db_r, NULL, "%s/db", m_path);
    }

    rc_t FiniR(void)
    {
        RefTableSubSelect_Whack((void*)m_rss);
        RefSeqMgr_Release(m_refseqmgr);
        return VDatabaseRelease(m_db_r);
    }

    rc_t RSSMake(const char* tbl_name)
    {
        rc_t rc;
        const VTable* t;
        const char* col = "(INSDC:dna:text)READ";

        OUTMSG(("%s for column '%s'\n", tbl_name, col));
        if( (rc = VDatabaseOpenTableRead(m_db_r, &t, tbl_name)) == 0 ) {
            rc = RefTableSubSelect_Make(&m_rss, t, NULL, col);
            VTableRelease(t);
        }
        return rc;
    }

    rc_t RSSRead(int64_t ref_row_id, INSDC_coord_zero offset, INSDC_coord_len ref_len,
                 uint32_t ref_ploidy, VRowResult* rslt)
    {
        rslt->elem_count = 0;
        return m_rss->func(m_rss, ref_row_id, offset, ref_len, ref_ploidy, rslt);
    }

    rc_t RSSRelease(void) {
        RefTableSubSelect_Whack(m_rss);
        m_rss = 0;
        return 0;
    }

    int64_t rand(int64_t from, int64_t to)
    {
        return from + (int64_t)((float)(to - from + 1) * (std::rand() / (RAND_MAX + 1.0)));
    }
};

FIXTURE_TEST_CASE(TestAll, RefTableSubSelectFixture)
{
    char buf[10 * sm_max_seq_len];
    struct seq_struct {
        const char* id;
        const char* accession;
        const bool circular;
        const INSDC_coord_len seq_len;
        int64_t rstart;
        int64_t rend;
    } seq[] = {
        {"chr6*", "NG_002433.1", false, 150447, NULL, 0},
        {"chrM",  "NC_001807.4", true,   16571,  NULL, 0}
    };

    OUTMSG(("Preparing run db...\n"));
    REQUIRE_RC(InitW());

    size_t u = 0, w;
    for(unsigned i = 0; i < sizeof(seq) / sizeof(seq[0]); i++ ) {
        REQUIRE_RC(string_printf(&buf[u], sizeof(buf) - u, &w, "%s\t%s\n", seq[i].id, seq[i].accession));
        u += w;
    }
    REQUIRE_RC(WriteCfg(buf));

    REQUIRE_RC(Mgr());

    OUTMSG(("refseqs in db:\n"));
    for(unsigned i = 0; i < sizeof(seq) / sizeof(seq[0]); i++ ) {
        const ReferenceSeq* rseq;
        REQUIRE_RC(ReferenceMgr_Verify(m_rmgr, seq[i].id, seq[i].seq_len, NULL));
        REQUIRE_RC(ReferenceMgr_GetSeq(m_rmgr, &rseq, seq[i].id));
        REQUIRE_RC(ReferenceSeq_Get1stRow(rseq, &seq[i].rstart));
        REQUIRE_RC(ReferenceSeq_Release(rseq));
        seq[i].rend = seq[i].rstart + seq[i].seq_len / (sm_max_seq_len - 1);
            OUTMSG(("%s\t%s\trows:[%li:%li]\tlen:%10u\t%s\n", seq[i].id, seq[i].accession,
                seq[i].rstart, seq[i].rend, seq[i].seq_len, seq[i].circular ? "circular" : "linear"));
    }
    // write couple dummy records into all types of alignment tables so xfunc could be used directly
    // these records must use all references from above!
    TAlgRec prim[] = {
        {seq[0].id, 44, "GAGCCAggATGccCACAGC", "11M2I6M", 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0}
    };
    REQUIRE_RC(Write(prim, ewalgn_tabletype_PrimaryAlignment, ewalgn_co_unsorted));
    
    // not dummy records, used in EVIDENCE tests
    TAlgRec allele[] = {
        /* linear */
        {seq[0].id,    36, "ACAGGCGTGAGCCACCATGCACAGCCCTGTTTTTTTTTTTTTTTTTTTT", "49M", 0, 0, 0, 0},
        {seq[0].id,    36, "ACAGcCGTGAGCCACCATGCACAGCCCTGTaTaTaTaTaTaTaTaTaTa", "49M", 0, 0, 0, 0},
        {seq[0].id,  2222, "ACAATCTTTCATTTCCCTACCCTGAAATCAATCTCTCCTACTCCATCCACCATTTCTTTATTTTAAAAAT", "70M", 0, 0, 0, 0},
        {seq[0].id,  2222, "ACAATCTgTCATaTCtCTACCCTGAcATCccTCTCTCgTACTCCATCCACCATTTtTTTAaaaaAAAAAT", "70M", 0, 0, 0, 0},
        {seq[0].id,  4444, "TCTGTCACAGGATCCAGCGTAATTGCATTAGCCTTAGTGGCTCT", "44M", 0, 0, 0, 0},
        {seq[0].id,  4444, "TCTGTCACAGcgaCCAGCGTAATTGCATTAGCCTTAGTGGCTCT", "10M3I3D31M", 0, 0, 0, 0},

        /* circular */
        {seq[1].id, 1, "ATCACAGGTCTATCACCCTATT", "22M", 0, 0, 0, 0},
        {seq[1].id, 1, "ATCACAccTCTATCACttTATT", "22M", 0, 0, 0, 0},
        {seq[1].id, 116, "TGTCGCAGTATCTGTCTTTGATTCCTGCCTCATTCTA", "37M", 0, 0, 0, 0},
        {seq[1].id, 116, "TGTCGCAGTcTCTGTCTgTGATTCCTGCaTCATgCTA", "37M", 0, 0, 0, 0},
        {seq[1].id, 16560, "ACATCACGATG", "11M", 0, 0, 0, 0},
        {seq[1].id, 16560, "ACATggCGATc", "11M", 0, 0, 0, 0},

        {0, 0, 0, 0, 0, 0, 0, 0}
    };
    REQUIRE_RC(Write(allele, ewalgn_tabletype_EvidenceInterval, 0));

    TAlgRec allele_alg[] = {
        {seq[1].id, 2, "ACTGTGGCACCGTCAACGTAGCT", "23M", 0, 2, "ACGTGCTG", "8M"},
        {0, 0, 0, 0, 0, 0, 0, 0}
    };
    REQUIRE_RC(Write(allele_alg, ewalgn_tabletype_EvidenceAlignment, 0));
    REQUIRE_RC(FiniW());

    // actual tests
    OUTMSG(("running tests...\n"));
    REQUIRE_RC(InitR());

    // prep data buffer
    KDataBuffer dbuf;
    VRowResult rslt;
    memset(&rslt, 0, sizeof(rslt));
    REQUIRE_RC(KDataBufferMakeBytes(&dbuf, 10240));
    rslt.elem_bits = dbuf.elem_bits;
    rslt.data = &dbuf;

    const INSDC_coord_len len_inc = sm_max_seq_len / rand(5, 10);
    const INSDC_coord_len chunk = sm_max_seq_len / rand(3, 7);

    const char* tables[] = {
        "PRIMARY_ALIGNMENT",
        "EVIDENCE_INTERVAL",
    };

    for(unsigned t = 0; t < sizeof(tables) / sizeof(tables[0]); t++ ) {
        REQUIRE_RC(RSSMake(tables[t]));
        /* ref_ploidy == 0 normal table read
           ref_ploidy > 0 means simulated allele read, same test is repeated below but in chained func
        */
        for(uint32_t ref_ploidy = 0; ref_ploidy < 2; ref_ploidy++ ) {
            for(unsigned i = 0; i < sizeof(seq) / sizeof(seq[0]); i++ ) {
                struct seq_struct* s = &seq[i];
                OUTMSG(("\n*** %s with ref_ploidy == %u\n", s->id, ref_ploidy));
                try {
                    /* walk each ref offset seq_len +- 3max_seq_len */
                    const INSDC_coord_zero off_max = s->seq_len + 3 * sm_max_seq_len;
                    for(INSDC_coord_zero off = - 3 * sm_max_seq_len; off < off_max; off += len_inc) {
                        INSDC_coord_len RefSeqMgr_Read_len;
                        rc_t rc = RefSeqMgr_Read(m_refseqmgr, s->accession, strlen(s->accession), off, chunk,
                                           (uint8_t*)buf, &RefSeqMgr_Read_len);
                        if( !s->circular && (off < 0 || (INSDC_coord_len)off >= s->seq_len) ) {
                            REQUIRE_RC_FAIL(rc);
                        } else {
                            REQUIRE_RC(rc);
                        }

                        INSDC_coord_zero rss_off = off;
                        int64_t r = s->rstart;
                        if( ref_ploidy == 0 ) {
                            /* we need to adjust offset to be with in single row */
                            if( rss_off >= 0 ) {
                                /* negative fails anyway */
                                r += rss_off / sm_max_seq_len;
                                rss_off %= sm_max_seq_len;
                            }
                            if( r > s->rend ) {
                                /* for normal case we would shift to next refseq here */
                                continue;
                            }
                        } else {
                            /* we need to vary r here to test code */
                            r = rand(s->rstart, s->rend);
                            rss_off = off - (r - s->rstart) * sm_max_seq_len;
                        }
                        DBG(10, ("[%li,%i,%u] ", r, rss_off, chunk));
                        rc = RSSRead(r, rss_off, chunk, ref_ploidy, &rslt);
                        if( (ref_ploidy == 0 || !s->circular) && (off < 0 || (INSDC_coord_len)off >= s->seq_len) ) {
                            REQUIRE_RC_FAIL(rc);
                            DBG(10, ("- fail ok\n"));
                        } else {
                            REQUIRE_RC(rc);
                            DBG(10, ("\nsrc: '%.*s'\nxfn: '%.*s'\n", RefSeqMgr_Read_len, buf, (uint32_t)rslt.elem_count, rslt.data->base));
                            REQUIRE(RefSeqMgr_Read_len == rslt.elem_count);
                            REQUIRE(memcmp(buf, rslt.data->base, RefSeqMgr_Read_len) == 0 );
                            DBG(10, ("- success %u bases\n", RefSeqMgr_Read_len));
                        }
                    }
                    OUTMSG(("*** %s with ref_ploidy == %u - ok\n", s->id, ref_ploidy));
                } catch(...) {
                    OUTMSG(("*** %s with ref_ploidy == %u - failed\n", s->id, ref_ploidy));
                }
            }
        }
        REQUIRE_RC(RSSRelease());
    }

    REQUIRE_RC(RSSMake("EVIDENCE_ALIGNMENT"));
    const TAlgRec* a = allele;
    const char* prev_id = NULL;
    INSDC_coord_zero prev_off = -1;
    while( a->id != NULL ) {
        uint32_t ref_ploidy = (prev_id == a->id && prev_off == a->offset) ? 2 : 1;

        struct seq_struct* s = NULL;

        for(unsigned k = 0; k < sizeof(seq) / sizeof(seq[0]); k++ ) {
            if( a->id == seq[k].id ) {
                s = &seq[k];
                break;
            }
        }
        REQUIRE(s != NULL);

        OUTMSG(("\n*** %s @%i with ref_ploidy == %u\n", a->id, a->offset, ref_ploidy));
        try {
            const INSDC_coord_len ref_len = strlen(a->seq) * 3 / 4;
            int32_t x = - ref_len - 2;
            const int32_t x_max = 2 * ref_len + 2;
            while( x < x_max ) {
                DBG(10, ("in allele @ %i %u bases ", x, ref_len));
                rc_t rc = RSSRead(a->row_id, x, ref_len, ref_ploidy, &rslt);
                if( (x < 0 && (x + (int32_t)ref_len) < 0) ) {
                    REQUIRE_RC_FAIL(rc);
                    DBG(10, ("- ends before allele start failure is ok: %R\n", rc));
                } else if( (x > 0 && (x > (int32_t)strlen(a->seq))) ) {
                    REQUIRE_RC_FAIL(rc);
                    DBG(10, ("- starts after allele ened failure is ok: %R\n", rc));
                } else {
                    INSDC_coord_len RefSeqMgr_Read_len;

                    REQUIRE_RC(rc);
                    DBG(10, ("in refseq from %i %u bases", a->offset + x, ref_len));
                    rc = RefSeqMgr_Read(m_refseqmgr, s->accession, strlen(s->accession), a->offset + x, ref_len,
                                       (uint8_t*)buf, &RefSeqMgr_Read_len);
                    REQUIRE_RC(rc);
                    REQUIRE(RefSeqMgr_Read_len == rslt.elem_count);
                    if( ref_ploidy > 1 ) {
                        /* apply allele to refseq so they match */
                        if( x < 0 ) {
                            memcpy(&buf[-x], a->seq, ref_len < strlen(a->seq) ? ref_len : strlen(a->seq));
                        } else {
                            int32_t y = strlen(a->seq) - x;
                            memcpy(buf, &a->seq[x], ref_len < (INSDC_coord_len)y ? ref_len : y);
                        }
                    }
                    DBG(10, ("\nsrc: '%.*s'\nxfn: '%.*s'\n", RefSeqMgr_Read_len, buf, rslt.elem_count, rslt.data->base));
                    REQUIRE(strncasecmp(buf, (const char*)rslt.data->base, RefSeqMgr_Read_len) == 0 );
                    DBG(10, ("- success %u bases\n", RefSeqMgr_Read_len));
                }
                x += ref_len;
            }
            OUTMSG(("*** %s @%i with ref_ploidy == %u - ok\n", a->id, a->offset, ref_ploidy));
        } catch(...) {
            OUTMSG(("*** %s @%i with ref_ploidy == %u - failed\n", a->id, a->offset, ref_ploidy));
        }
        prev_id = a->id;
        prev_off = a->offset;
        a++;
    }
    REQUIRE_RC(RSSRelease());

    REQUIRE_RC(FiniR());
}

ver_t CC KAppVersion( void )
{
    return 0;
}

const char* output_usage[] = {"output path for temp database", NULL};

const char* usage_params[] =
{
    "path"
};

enum eArgs {
    eargs_OutputPath
};

OptDef MainArgs[] =
{
    {"output", "o", NULL, output_usage, 1, true, true}
};

const char UsageDefaultName[] = __FILE__;

rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg ( "Usage:\n"
        "\t%s [options] [-+ ALIGN] -o path\n\n", progname );
}

rc_t CC Usage( const Args* args )
{
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
    rc_t rc;
    unsigned i;

    rc = ArgsProgram(args, &fullpath, &progname);

    UsageSummary(progname);

    OUTMSG (("Options:\n"));
    for(i = 0; i < sizeof(MainArgs)/sizeof(MainArgs[0]); i++ ) {
        if( MainArgs[i].help != NULL ) {
            HelpOptionLine(MainArgs[i].aliases, MainArgs[i].name, usage_params[i], MainArgs[i].help);
        }
    }
    OUTMSG (("\n"));
    HelpOptionsStandard();

    HelpVersion(fullpath, KAppVersion());

    return rc;
}

rc_t CC KMain( int argc, char* argv[] )
{
    rc_t rc = 0;
    Args* args;
    const char* errmsg = "stop";

    if( (rc = ArgsMakeAndHandle(&args, argc, argv, 1, MainArgs, sizeof(MainArgs)/sizeof(MainArgs[0]))) == 0 ) {
        uint32_t pcount, count;

        if( (rc = ArgsParamCount(args, &pcount)) != 0 || pcount > 0 ) {
            errmsg = "parameter(s)";
            rc = argc < 2 ? 0 : RC(rcExe, rcArgv, rcParsing, rcParam, rcExcessive);
            MiniUsage(args);

        } else if( (rc = ArgsOptionCount(args, MainArgs[eargs_OutputPath].name, &count)) != 0 || count != 1 ) {
            rc = rc ? rc : RC(rcExe, rcArgv, rcParsing, rcParam, count ? rcExcessive : rcInsufficient);
            errmsg = MainArgs[eargs_OutputPath].name;
        } else if( (rc = ArgsOptionValue(args, MainArgs[eargs_OutputPath].name, 0, &output_path)) != 0 ) {
            errmsg = MainArgs[eargs_OutputPath].name;

        } else {
            seed = time(NULL);
            OUTMSG(("seed %u\n", seed));
            srand(seed);
            rc = RefTableSubSelect_TestSuite(argc, argv);
        }
        ArgsWhack(args);
    }
    if( rc != 0 ) {
        if( errmsg[0] ) {
            LOGERR(klogErr, rc, errmsg);
        } else if( KLogLastErrorCode() != rc ) {
            LOGERR(klogErr, rc, "stop");
        }
    }
    return rc;
}
