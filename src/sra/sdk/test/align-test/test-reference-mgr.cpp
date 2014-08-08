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

TEST_SUITE_WITH_USAGE(ReferenceMgr_TestSuite);

#define DBG(flag,msg) DBGMSG(DBG_APP,DBG_FLAG(DBG_APP_##flag), msg)

const char* output_path = NULL;
time_t seed = 0;

class ReferenceMgrFixture {

    VDBManager* m_mgr;
    VSchema* m_schema;
    VDatabase* m_db;
    char m_path[4096];
    KDirectory* m_dir;

protected:
    static const size_t sm_cursor_cache_size = 1024 * 1024 * 1024;
    static const int sm_line_width = 20;

    const ReferenceMgr* m_r;

public:
    ReferenceMgrFixture(void)
        : m_mgr(0), m_schema(0), m_db(0), m_dir(0), m_r(0)
    {
        size_t w;
        string_printf(m_path, sizeof(m_path), &w, "%s/ReferenceMgrTest-%u", output_path, seed);
    }

    ~ReferenceMgrFixture(void) {
        KDirectoryRelease(m_dir);
    }

    rc_t Init()
    {
        rc_t rc = 0;
        KDirectory* d = 0;

        if( (rc = KDirectoryNativeDir(&d)) == 0 &&
            (rc = KDirectoryCreateDir(d, 0777, kcmInit, "%s", m_path)) == 0 &&
            (rc = KDirectoryOpenDirUpdate(d, &m_dir, true, m_path)) == 0 &&
            (rc = VDBManagerMakeUpdate(&m_mgr, NULL)) == 0 &&
            (rc = VDBManagerMakeSchema(m_mgr, &m_schema)) == 0 &&
            (rc = VSchemaParseFile(m_schema, "align/align.vschema")) == 0 ) {
            rc = VDBManagerCreateDB(m_mgr, &m_db, m_schema, "NCBI:align:db:alignment_sorted", kcmInit, "%s/db", m_path);
        }
        KDirectoryRelease(d);
        return rc;
    }

    rc_t Fini(void)
    {
        rc_t rc1 = ReferenceMgr_Release(m_r, false, NULL, false);
        rc_t rc2 = VDatabaseRelease(m_db);
        rc_t rc3 = VSchemaRelease(m_schema);
        rc_t rc4 = VDBManagerRelease(m_mgr);
        return rc1 ? rc1 : rc2 ? rc2 : rc3 ? rc3 : rc4;
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

    void FastaGet(INSDC_coord_len len, char* fasta, uint8_t* md5)
    {
        MD5State smd5;

        MD5StateInit(&smd5);

        while( len-- > 0 ) {
            char c = INSDC_4na_accept_CHARSET[rand(0, sizeof(INSDC_4na_accept_CHARSET) - 2)];
            *fasta = c;
            c = (c == '.') ? 'N' : toupper(c);
            MD5StateAppend(&smd5, &c, 1);
            fasta++;
        }
        *fasta = '\0';
        MD5StateFinish(&smd5, md5);
    }
    
    rc_t FastaNew(const char* id, const char* fasta, bool well_formed)
    {
        rc_t rc = 0;
        KFile* f;
        if( (rc = KDirectoryCreateFile(m_dir, &f, false, 0666, kcmInit, "%s.fasta", id)) == 0 ) {
            rc = FastaWrite(f, id, 0, fasta, well_formed);
            KFileRelease(f);
        }
        return rc;
    }
    
    rc_t FastaAdd(const char* file, const char* id, const char* fasta, bool well_formed)
    {
        rc_t rc = 0;
        uint64_t pos = 0;
        KFile* f;

        if( (rc = KDirectoryFileSize(m_dir, &pos, "%s.fasta", file)) != 0 ) {
            rc = KDirectoryCreateFile(m_dir, &f, false, 0666, kcmInit, "%s.fasta", file);
            KFileRelease(f);
            pos = 0;
        }
        if( (rc = KDirectoryOpenFileWrite(m_dir, &f, false, "%s.fasta", file)) == 0 ) {
            rc = FastaWrite(f, id, pos, fasta, well_formed);
            KFileRelease(f);
        }
        return rc;
    }

    rc_t FastaWrite(KFile* f, const char* id, uint64_t pos, const char* fasta, bool well_formed)
    {
        rc_t rc = 0;
        char buf[256];
        size_t w, line = 0;

        if( (rc = string_printf(buf, sizeof(buf), &w, "%.*s>%s test data, please discard\n",
                pos ? rand(1, 5) : 0, pos ? "\n\n\n\n\n" : "", id)) == 0 ) {
            rc = KFileWrite(f, pos, buf, w, NULL);
            pos +=w;
            while( rc == 0 && *fasta != '\0' ) {
                if( (rc = KFileWrite(f, pos++, fasta, 1, NULL)) == 0 ) {
                    line++;
                    fasta++;
                    if( well_formed && (line % sm_line_width) == 0 ) {
                        rc = KFileWrite(f, pos++, "\n", 1, NULL);
                        line = 0;
                    } else if( !well_formed && (line % rand(sm_line_width - 2, sm_line_width + 2)) == 0 ) {
                        rc = KFileWrite(f, pos++, "\n", 1, NULL);
                        line = 0;
                    }
                }
            }
            if( rc == 0 ) {
                rc = KFileWrite(f, pos, "\n\n\n\n", rand(1, 4), NULL);
            }
        }
        return rc;
    }

    rc_t Mgr(void) {
        return ReferenceMgr_Make(&m_r, m_db, m_mgr, 0, "ref.cfg", m_path, 0, sm_cursor_cache_size, 0);
    }

    rc_t MgrImport(const char* grp)
    {
        rc_t rc = 0;
        const KFile* f;

        if( grp != NULL ) {
            if( (rc = KDirectoryOpenFileRead(m_dir, &f, "%s.fasta", grp)) == 0 ) {
                rc = ReferenceMgr_FastaFile(m_r, f);
                KFileRelease(f);
            }
        }
        return rc;
    }

    rc_t UnMgr(void) {
        rc_t rc1 = ReferenceMgr_Release(m_r, false, NULL, false);
        rc_t rc2 = VDatabaseDropTable(m_db, "REFERENCE");
        if( GetRCObject(rc2) == rcPath && GetRCState(rc2) == rcNotFound ) {
            rc2 = 0;
        }
        m_r = 0;
        return rc1 ? rc1 : rc2;
    }

    int rand(int from, int to)
    {
        return from + (int)((float)(to - from + 1)* (std::rand() / (RAND_MAX + 1.0)));
    }
};

FIXTURE_TEST_CASE(InvalidConfig, ReferenceMgrFixture)
{
    rc_t rc;
    char buf[8192];
    const char* bad[][3] = {
            {" chr1", "    NC_000001.9",  "\tchr1.aso\n"},
            {"chr1",  "NC_000001.9",      "\n"},
            {"chr1",  "NC_000001.9",      ""},
            {"chr1\n",  "NC_000001.9\n",      ""}
        };

    REQUIRE_RC(rc = Init());

    for(unsigned i = 0; rc == 0 && i < sizeof(bad) / sizeof(bad[0]); i++ ) {
        size_t w;
        REQUIRE_RC(rc = string_printf(buf, sizeof(buf), &w, "%s%s%s", bad[i][0], bad[i][1], bad[i][2]));
        if( rc == 0 ) {
            REQUIRE_RC(rc = WriteCfg(buf));
            if( rc == 0 ) {
                CHECK_RC_FAIL(Mgr());
                REQUIRE_RC(UnMgr());
            }
        }
    }
    REQUIRE_RC(Fini());
}

FIXTURE_TEST_CASE(ValidConfig, ReferenceMgrFixture)
{
    rc_t rc;
    char buf[100 * 1024];
    const unsigned chr_1st = 0;
    const char* bld36[][3] = {
        {"chr1",  "    NC_000001.9",  "\tchr1.aso\n"},
        {"chr2",  " NC_000002.10", "\tchr2.aso\n"},
        {"chr3",  " NC_000003.10", " chr3.aso\n"},
        {"chr4",  " NC_000004.10", " chr4.aso\n"},
        {"chr5",  " NC_000005.8",  "    chr5.aso\n"},
        {"chr6",  " NC_000006.10", "\t  \t chr6.aso\n"},
        {"chr7",  " NC_000007.12", " chr7.aso\n"},
        {"chr8",  " NC_000008.9",  " chr8.aso\n"},
        {"chr9",  " NC_000009.10", " chr9.aso\n"},
        {"chr10", " NC_000010.9",  " chr10.aso\n"},
        {"chr11", " NC_000011.8",  " chr11.aso\n"},
        {"", "",  "\n"}, // empty line in cfg is ok now
        {"chr12", "\tNC_000012.10", " chr12.aso\n"},
        {"chr13", "\tNC_000013.9",  " chr13.aso\n"},
        {"chr14", "\t\tNC_000014.7",  "\t\t\tchr14.aso\n"},
        {"chr15", "\t \tNC_000015.8",  " chr15.aso\n"},
        {"chr16", "\tNC_000016.8",  " chr16.aso\r\n"},
        {"chr17", " NC_000017.9",  " chr17.aso\r"},
        {"chr18", " NC_000018.8",  " chr18.aso\n"},
        {"chr19", " NC_000019.8",  " chr19.aso\n"},
        {"chr20", " NC_000020.9",  " chr20.aso\n"},
        {"chr21", " NC_000021.7",  " chr21.aso\n\r\n\r\r"},
        {"chr22", " NC_000022.9",  " chr22.aso\n"},
        {"chrX",  " NC_000023.9",  " chrX.aso\n"},
        {"chrY",  " NC_000024.8",  " chrY.aso\n"},
        {"chrM",  " NC_001807.4",  ""}
        };

    struct {
        const char* grp;
        const char* id;
        INSDC_coord_len len;
        bool well_formed;
        uint8_t md5[16];
        char fasta[100 * 1024];
    } local[] = {
        {NULL, "local-empty-will-fail", 0, true},
        {NULL, "local-single-base", 1, true},
        {NULL, "local-less_than_line", sm_line_width - 2, true},
        {NULL, "local-single_line", sm_line_width, true},
        {NULL, "local-multi_line", sm_line_width * 3 + sm_line_width / 2, true},
        {NULL, "local-a_little_over_line", sm_line_width + 2, true},

        {"tiny", "local-1-single-base", 1, true},
        {"tiny", "local-1-less_than_line", sm_line_width - 2, true},
        {"tiny", "local-1-single_line", sm_line_width, true},
        {"tiny", "local-1-multi_line", sm_line_width * 3 + sm_line_width / 2, true},
        {"tiny", "local-1-a_little_over_line", sm_line_width + 2, true},
        {"tiny", "local-1-long", 50 * 1024, true},
        {"tiny", "local-1-longest", 100 * 1024 - 2, true},

        {"xxx", "local-2-1", 256, true},
        {"xxx", "local-2-2", 645, false},
        {"xxx", "local-2-3", 256, true},
        {"xxx", "local-2-4", 452, false},
        {"xxx", "local-2-5", 256, true},
        {"xxx", "local-2-6", 45645, false},
        {"xxx", "local-2-7", 256, true},
        {"xxx", "local-2-8", 67545, false},
        {"xxx", "local-2-9", 256, true}
    };

    REQUIRE_RC(rc = Init());

    size_t u = 0, w;
    for(unsigned i = chr_1st; rc == 0 && i < sizeof(bld36) / sizeof(bld36[0]); i++ ) {
        REQUIRE_RC(rc = string_printf(&buf[u], sizeof(buf) - u, &w, "%s%s%s", bld36[i][0], bld36[i][1], bld36[i][2]));
        u += w;
    }
    REQUIRE_RC(rc = WriteCfg(buf));

    for(unsigned i = 0; rc == 0 && i < sizeof(local) / sizeof(local[0]); i++ ) {

        FastaGet(local[i].len, local[i].fasta, local[i].md5);
        if( local[i].grp != NULL ) {
            REQUIRE_RC(rc = FastaAdd(local[i].grp, local[i].id, local[i].fasta, local[i].well_formed));
        } else {
            REQUIRE_RC(rc = FastaNew(local[i].id, local[i].fasta, local[i].well_formed));
        }
    }

    OUTMSG(("verifying..\n"));
    if( rc == 0 ) {
        REQUIRE_RC(rc = Mgr());
        for(unsigned i = 0; rc == 0 && i < sizeof(local) / sizeof(local[0]); i++ ) {
            REQUIRE_RC(MgrImport(local[i].grp));
        }
        for(unsigned i = 0; rc == 0 && i < sizeof(local) / sizeof(local[0]); i++ ) {
            const ReferenceSeq* s;
            OUTMSG(("%s...", local[i].id));
            rc = ReferenceMgr_GetSeq(m_r, &s, local[i].id);
            if( local[i].len == 0 ) {
                CHECK_RC_FAIL(rc);
                rc = 0;
            } else {
                CHECK_RC(rc);
                if( rc == 0 ) {
                    CHECK_RC(rc = ReferenceMgr_Verify(m_r, local[i].id, local[i].len, local[i].md5));
                    INSDC_coord_len l;
                    INSDC_coord_zero off;
                    for(off = 0; rc == 0 && off < local[i].len; off++) {
                        CHECK_RC(rc = ReferenceSeq_Read(s, off, 1, (uint8_t*)&buf[off], &l));
                        CHECK(l == 1);
                    }
                    if( rc == 0 && strncmp(local[i].fasta, buf, local[i].len) != 0 ) {
                        OUTMSG(("'%.*s' <> '%.*s'\n", off, buf, local[i].len, local[i].fasta));
                        rc = 333;
                    }
                }
                ReferenceSeq_Release(s);
            }
            OUTMSG(("%s\n", rc ? "failed" : "ok"));
            rc = 0;
        }
        REQUIRE_RC(UnMgr());
    }

    OUTMSG(("cycling refseqs..\n"));
    if( rc == 0 ) {
        CHECK_RC(rc = Mgr());
        for(unsigned i = chr_1st; rc == 0 && i < sizeof(bld36) / sizeof(bld36[0]); i++ ) {
            if( bld36[i][0][0] != '\0' ) {
                const ReferenceSeq* s = 0;
                OUTMSG(("%s...", bld36[i][0]));
                CHECK_RC(rc = ReferenceMgr_GetSeq(m_r, &s, bld36[i][0]));
                ReferenceSeq_Release(s);
                OUTMSG(("%s\n", rc ? "failed" : "ok"));
            }
        }
        CHECK_RC(UnMgr());
    }
    REQUIRE_RC(Fini());
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
            rc = ReferenceMgr_TestSuite(argc, argv);
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
