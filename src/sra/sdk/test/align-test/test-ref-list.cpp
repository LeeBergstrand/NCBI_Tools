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

#include <klib/callconv.h>
#include <klib/debug.h>
#include <klib/out.h>
#include <klib/log.h>
#include <klib/rc.h>
#include <kapp/main.h>
#include <kapp/args.h>
#include <vdb/manager.h>
#include <vdb/table.h>
#include <insdc/insdc.h>
#include <align/iterator.h>
#include <align/reference.h>
#include <sra/srapath.h>

#include <os-native.h>
#include <sysalloc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strtol.h>
#include <ctype.h>

#include <common/test_assert.h>

TEST_SUITE_WITH_USAGE(ReferenceList_TestSuite);

#define CURSOR_CACHE (32 * 1024 * 1024)
#define DBG(flag,msg) DBGMSG(DBG_APP,DBG_FLAG(DBG_APP_##flag), msg)

typedef struct TAlignedRegion_struct {
    char name[1024];
    struct {
        INSDC_coord_zero from;
        INSDC_coord_zero to;
    } r[10240];
    int rq;
    INSDC_coord_zero max_to;
} TAlignedRegion;

struct SParam {
    const char* accession;
    const char* path;
    /* region filter data */
    TAlignedRegion* region;
    uint32_t region_qty;
} param;

class ReferenceListFixture {

    const VDBManager* mgr;

protected:
    const ReferenceList* refl;

public:
    ReferenceListFixture(void)
        : mgr(0), refl(0)
    {
        if( VDBManagerMakeRead(&mgr, NULL) != 0 ) {
            FAIL("fixture c-tor failed");
        }
    }
    ~ReferenceListFixture(void) {
        ReferenceList_Release(refl);
        if( VDBManagerRelease(mgr) != 0 ) {
            FAIL("fixture d-tor failed");
        }
    }

    rc_t MakeList(const char* name, const uint32_t numbins) {
        return ReferenceList_MakePath(&refl, mgr, param.path,
            ereferencelist_usePrimaryIds | ereferencelist_useSecondaryIds,
            CURSOR_CACHE, name, numbins);
    }

    rc_t PrintList(void) {
        rc_t rc = 0;
        uint32_t i, count = 0;
        
        rc = ReferenceList_Count(refl, &count);
        for(i = 0; rc == 0 && i < count; i++) {
            const ReferenceObj* obj;
            if( (rc = ReferenceList_Get(refl, &obj, i)) == 0 ) {
                const char* name = NULL;
                const char* seqid = NULL;
                INSDC_coord_len len;
                bool ex;
                char* path = NULL;
                if( (rc = ReferenceObj_SeqId(obj, &seqid)) == 0 &&
                    (rc = ReferenceObj_Name(obj, &name)) == 0 &&
                    (rc = ReferenceObj_SeqLength(obj, &len)) == 0 &&
                    (rc = ReferenceObj_External(obj, &ex, &path)) == 0 ) {
                        OUTMSG(("NAME:%s\tSEQ_ID:%s\tLN:%u\tEX:%s\tLC:%s\n", name, seqid, len, ex ? "external" : "local", path));
                }
                free(path);
                ReferenceObj_Release(obj);
            }
        }
        return rc;
    }

    rc_t ReadObj(const ReferenceObj* obj) {
        rc_t rc = 0;
        INSDC_coord_len len;
        const char* name;

        if( (rc = ReferenceObj_Name(obj, &name)) == 0 &&
            (rc = ReferenceObj_SeqLength(obj, &len)) == 0 ) {
            uint8_t buf[4096];
            INSDC_coord_zero pos = 0;
            INSDC_coord_len writ;

            while( rc == 0 && pos < len ) {
                INSDC_coord_len q = sizeof(buf);
                if( len - pos < q ) {
                    q = len - pos;
                }
                OUTMSG(("reading pos %i %u\n", pos, q));
                if( (rc = ReferenceObj_Read(obj, pos, q, buf, &writ)) == 0 ) {
                    pos += writ;
                }
            }
            if( rc == 0 && pos != len ) {
                rc = RC(rcExe, rcNoTarg, rcReading, rcSize, rcUnequal);
            } else {
                OUTMSG(("READ %s %u bases\n", name, pos));
            }
        }
        return rc;
    }

    rc_t IteratePos(const ReferenceObj* ref, PlacementIterator* iter, INSDC_coord_zero pos) {
        rc_t rc = 0;
        uint8_t base;
        INSDC_coord_len writ;
        const PlacementRecord* r, *prev = NULL;

        if( (rc = ReferenceObj_Read(ref, pos, 1, &base, &writ)) == 0 ) {
            OUTMSG(("%i %c", pos, base));
            while( (rc = PlacementIteratorNextRecordAt(iter, pos, &r)) == 0 ) {
                OUTMSG((" {%d, %d, %li}", r->pos, r->len, r->id));
                if(r->pos != pos) {
                    FAIL("position is not same");
                }
                if( prev != NULL ) {
                    if( r->len > prev->len ) {
                        FAIL("REF_LEN is out of sort");
                    } else if( r->len == prev->len ) {
                        if( r->id < prev->id ) {
                            FAIL("ALIGNMENT record id is out of sort");
                        }
                    }
                }
                PlacementRecordWhack(prev);
                prev = r;
            }
            OUTMSG(("\n"));
            PlacementRecordWhack(prev);
        }
        return rc;
    }
};

FIXTURE_TEST_CASE(Complete_List, ReferenceListFixture)
{
    const ReferenceObj* obj;


    REQUIRE_RC(MakeList(0, 0));
    REQUIRE_RC(PrintList());


    REQUIRE_RC(ReferenceList_Find(refl, &obj, "chr2", 4));
    ReferenceObj_Release(obj);

    REQUIRE_RC_FAIL(ReferenceList_Find(refl, &obj, "chr-netu", 8));
    ReferenceObj_Release(obj);
}

/*
FIXTURE_TEST_CASE(Chr21_List, ReferenceListFixture)
{
    const ReferenceObj* obj;

    REQUIRE_RC(MakeList("chr21", 0));
    REQUIRE_RC(PrintList());

    uint32_t count;
    REQUIRE_RC(ReferenceList_Count(refl, &count));
    CHECK(count == 1);

    CHECK(GetRCState(ReferenceList_Find(refl, &obj, "chr1", 4)) == rcNotFound);
    ReferenceObj_Release(obj);

    REQUIRE_RC(ReferenceList_Find(refl, &obj, "chr21", 5));

    bool circular;
    REQUIRE_RC(ReferenceObj_Circular(obj, &circular));
    CHECK(circular == false);

    //REQUIRE_RC(ReadObj(obj));

    PlacementIterator* iter;

    REQUIRE_RC(ReferenceObj_MakePlacementIterator(obj, &iter, 0, ~0, NULL, NULL, false, NULL, NULL, NULL));

    INSDC_coord_len len0;
    REQUIRE_RC(ReferenceObj_SeqLength(obj, &len0));
    CHECK(len0 != 0);

    const char* id;
    INSDC_coord_zero pos;
    INSDC_coord_len len;
    REQUIRE_RC(PlacementIteratorRefWindow(iter, &id, &pos, &len));
    CHECK(strcmp("NC_000021.7", id) == 0);
    CHECK(pos == 0);
    CHECK(len0 = len);

    rc_t rc;  
    while( (rc = PlacementIteratorNextAvailPos(iter, &pos, NULL)) == 0 ) {
        rc = IteratePos(obj, iter, pos);
        if( rc != 0 ) {
            if( (GetRCObject(rc) == rcOffset && GetRCState(rc) == rcDone)) {
            } else {
                CHECK(rc == 0);
                break;
            }
        }
    
    }
    CHECK(GetRCState(rc) == rcDone);
    ReferenceObj_Release(obj);
    PlacementIteratorRelease(iter);
}


FIXTURE_TEST_CASE(ChrM_List, ReferenceListFixture)
{
    const ReferenceObj* obj;

    REQUIRE_RC(MakeList("chrM", 0));
    REQUIRE_RC(PrintList());

    uint32_t count;
    REQUIRE_RC(ReferenceList_Count(refl, &count));
    CHECK(count == 1);

    CHECK(GetRCState(ReferenceList_Find(refl, &obj, "chrX", 4)) == rcNotFound);
    ReferenceObj_Release(obj);

    REQUIRE_RC(ReferenceList_Find(refl, &obj, "chrM", 4));

    bool circular;
    REQUIRE_RC(ReferenceObj_Circular(obj, &circular));
    CHECK(circular == true);

    REQUIRE_RC(ReadObj(obj));

    const ReferenceIter* iter;

    REQUIRE_RC(ReferenceObj_AlignIter(obj, false, 0, &iter));
    ReferenceObj_Release(obj);

    int64_t pos;
    REQUIRE_RC(ReferenceIter_GetPos(iter, &pos));
    CHECK(pos == -16571);

    rc_t rc = 0;
    while( (rc = ReferenceIter_Advance(iter, 1)) == 0 ) {
        REQUIRE_RC(ListAlignIds(iter, false));
    }
    CHECK(GetRCState(rc) == rcDone);

    ReferenceIter_Release(iter);
}*/

FIXTURE_TEST_CASE(PlacementIterator_Test, ReferenceListFixture)
{
    const ReferenceObj* obj;

    REQUIRE_RC(MakeList(NULL, 0));
    REQUIRE_RC(PrintList());

    uint32_t count;
    REQUIRE_RC(ReferenceList_Count(refl, &count));
    CHECK(count > 0);

    REQUIRE_RC(GetRCState(ReferenceList_Get(refl, &obj, 0)));


    PlacementIterator* iter;

    REQUIRE_RC(ReferenceObj_MakePlacementIterator(obj, &iter, 0, ~0, ~0, NULL, NULL, primary_align_ids, NULL, NULL, NULL));

    INSDC_coord_len len0;
    REQUIRE_RC(ReferenceObj_SeqLength(obj, &len0));
    CHECK(len0 != 0);

    const char* id = NULL;
    INSDC_coord_zero pos;
    INSDC_coord_len len;
    REQUIRE_RC(PlacementIteratorRefWindow(iter, &id, &pos, &len));
    CHECK(id != NULL && id[0] != '\0');
    CHECK(pos == 0);
    CHECK(len0 = len);

    rc_t rc;  
    while( (rc = PlacementIteratorNextAvailPos(iter, &pos, NULL)) == 0 ) {
        rc = IteratePos(obj, iter, pos);
        if( rc != 0 ) {
            if( (GetRCObject(rc) == rcOffset && GetRCState(rc) == rcDone)) {
            } else {
                CHECK(rc == 0); /* force report */
                break;
            }
        }
    
    }
    CHECK(GetRCState(rc) == rcDone);
    ReferenceObj_Release(obj);
    PlacementIteratorRelease(iter);
}

ver_t CC KAppVersion( void )
{
    return 0;
}

const char* region_usage[] = {"Filter by position on genome.",
                              "Name can either be file specific name (ex: \"chr1\" or \"1\").",
                              "\"from\" and \"to\" are 1-based coordinates", NULL};
const char* usage_params[] =
{
    "name[:from-to]"
};

enum eArgs {
    earg_region = 0
};

OptDef DumpArgs[] =
{
    {"aligned-region", NULL, NULL, region_usage, 0, true, false},
};

const char UsageDefaultName[] = "sam-dump";

rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg ( "Usage:\n"
        "\t%s [options] path-to-run[ path-to-run ...]\n\n", progname );
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
    for(i = 0; i < sizeof(DumpArgs)/sizeof(DumpArgs[0]); i++ ) {
        if( DumpArgs[i].help != NULL ) {
            HelpOptionLine(DumpArgs[i].aliases, DumpArgs[i].name, usage_params[i], DumpArgs[i].help);
        }
    }
    OUTMSG (("\n"));
    HelpOptionsStandard();

    HelpVersion(fullpath, KAppVersion());

    return rc;
}

rc_t ResolvePath(const char* accession, const char** path)
{
    rc_t rc = 0;
    static char tblpath[4096];
    static SRAPath* pmgr = NULL;

    if( accession == NULL && path == NULL ) {
        SRAPathRelease(pmgr);
    } else if( pmgr != NULL || (rc = SRAPathMake(&pmgr, NULL)) == 0 ||
               (GetRCState(rc) == rcNotFound && GetRCTarget(rc) == rcDylib) ) {
        *path = tblpath;
        tblpath[0] = '\0';
        rc = 0;
        do {
            if( pmgr != NULL && !SRAPathTest(pmgr, accession) ) {
                /* try to resolve the path using mgr */
                if ( (rc = SRAPathFind(pmgr, accession, tblpath, sizeof(tblpath))) == 0 ) {
                    break;
                }
            }
            if( strlen(accession) >= sizeof(tblpath) ) {
                rc = RC(rcExe, rcPath, rcResolving, rcBuffer, rcInsufficient);
            } else {
                strcpy(tblpath, accession);
                rc = 0;
            }
        } while(false);
    }
    return rc;
}

rc_t CC KMain( int argc, char* argv[] )
{
    rc_t rc = 0;
    Args* args;
    const char* errmsg = "stop";

    if( (rc = ArgsMakeAndHandle(&args, argc, argv, 1, DumpArgs, sizeof(DumpArgs)/sizeof(DumpArgs[0]))) == 0 ) {
        uint32_t pcount, count[sizeof(DumpArgs)/sizeof(DumpArgs[0])];

        memset(&param, 0, sizeof(param));

        if( (rc = ArgsParamCount(args, &pcount)) != 0 || pcount < 1 ) {
            errmsg = "";
            rc = argc < 2 ? 0 : RC(rcExe, rcArgv, rcParsing, rcParam, rcInsufficient);
            MiniUsage(args);
        } else if( (rc = ArgsOptionCount(args, DumpArgs[earg_region].name, &count[earg_region])) != 0 ) {
            errmsg = DumpArgs[earg_region].name;
        } else {
            uint32_t p, i;
            const char* arg;

            for(p = 0; rc == 0 && p < count[earg_region]; p++) {
                /* name[:[from][-[to]]] 1-based!!! */
                TAlignedRegion r;

                errmsg = DumpArgs[earg_region].name;
                if( (rc = ArgsOptionValue(args, DumpArgs[earg_region].name, p, &arg)) == 0 ) {
                    const char* c = strchr(arg, ':');
                    if( c == NULL ) {
                        strncpy(r.name, arg, sizeof(r.name));
                        r.rq = 0;
                    } else {
                        INSDC_coord_zero* v;

                        r.r[0].from = (c - arg) > sizeof(r.name) ? sizeof(r.name) : c - arg;
                        strncpy(r.name, arg, r.r[0].from);
                        r.name[r.r[0].from] = '\0';
                        r.rq = 1;
                        r.r[0].from = -1;
                        r.r[0].to = -1;
                        r.max_to = 0;
                        v = &r.r[0].from;
                        while(rc == 0 && *++c != '\0') {
                            if( *c == '-' ) {
                                v = &r.r[0].to;
                            } else if( *c == '+' ) {
                                if( *v != 0 ) {
                                    rc = RC(rcExe, rcArgv, rcProcessing, rcParam, rcOutofrange);
                                }
                            } else if( !isdigit(*c) ) {
                                rc = RC(rcExe, rcArgv, rcProcessing, rcParam, rcOutofrange);
                            } else {
                                if( *v == -1 ) {
                                    *v = 0;
                                }
                                *v = *v * 10 + (*c - '0');
                            }
                        }
                        /* convert to 0-based offset */
                        if( r.r[0].from > 0 ) {
                            r.r[0].from--;
                        } else if( r.r[0].to > 0 ) {
                            r.r[0].from = 0;
                        }
                        if(r.r[0].to > 0 ) {
                            r.r[0].to--;
                        } else if( r.r[0].from >= 0 && r.r[0].to < 0 ) {
                            r.r[0].to = r.r[0].from;
                        }
                        if( r.r[0].from < 0 && r.r[0].to < 0 ) {
                            r.rq = 0;
                        } else if( r.r[0].from > r.r[0].to ) {
                            uint64_t x = r.r[0].from;
                            r.r[0].from = r.r[0].to;
                            r.r[0].to = x;
                        }
                    }
                    if( rc == 0 ) {
                        TAlignedRegion* x = NULL;
                        for(i = 0; i < param.region_qty; i++) {
                            if( strcmp(param.region[i].name, r.name) == 0 ) {
                                x = &param.region[i];
                                break;
                            }
                        }
                        if( x == NULL ) {
                            if( (x = (TAlignedRegion*)realloc(param.region, sizeof(*param.region) * ++param.region_qty)) == NULL ) {
                                rc = RC(rcExe, rcArgv, rcProcessing, rcMemory, rcExhausted);
                            } else {
                                param.region = x;
                                memcpy(&param.region[param.region_qty - 1], &r, sizeof(r));
                            }
                        } else {
                            int32_t k = x->rq;
                            for(i = 0; i < x->rq; i++) {
                                /* sort by from asc */
                                if( r.r[0].from <= x->r[i].from ) {
                                    k = i;
                                    break;
                                }
                            }
                            if( k >= 0 ) {
                                /* insert at k position */
                                if( x->rq >= sizeof(x->r) / sizeof(x->r[0]) ) {
                                    rc = RC(rcExe, rcArgv, rcProcessing, rcBuffer, rcInsufficient);
                                } else {
                                    memmove(&x->r[k + 1], &x->r[k], sizeof(x->r[0]) * (x->rq - k));
                                    x->r[k].from = r.r[0].from;
                                    x->r[k].to = r.r[0].to;
                                    x->rq++;
                                }
                            }
                        }
                    }
                }
            }
            for(p = 0; p < param.region_qty; p++) {
                DBG(2, ("filter by %s\n", param.region[p].name));
                if( param.region[p].rq == 0 ) {
                    param.region[p].rq = 1;
                    param.region[p].r[0].from = 0;
                    param.region[p].r[0].to = 0x7FFFFFFF;
                }
                for(i = 0; i < param.region[p].rq; i++) {
                    DBG(2, ("   range: [%u:%u]\n", param.region[p].r[i].from, param.region[p].r[i].to));
                    if( param.region[p].max_to < param.region[p].r[i].to ) {
                        param.region[p].max_to = param.region[p].r[i].to;
                    }
                }
            }

            for(p = 0; rc == 0 && p < pcount; p++) {
                char* arg;
                if( (rc = ArgsParamValue(args, p, (const char**)&arg)) == 0 ) {
                    int i;
                    /* remove trailing /\ */
                    for(i = strlen(arg) - 1; i >= 0; i--) {
                        if( arg[i] != '/' && arg[i] != '\\' ) {
                            break;
                        }
                        arg[i] = '\0';
                    }
                    if( (rc = ResolvePath(arg, &param.path)) == 0 ) {
                        /* use last path element as accession */
                        param.accession = param.path;
                        for(i = strlen(param.path) - 1; i >= 0; i--) {
                            if( param.path[i] == '/' || param.path[i] == '\\' ) {
                                param.accession = &param.path[i + 1];
                                break;
                            }
                        }
                        rc = ReferenceList_TestSuite(argc, argv);
                    }
                }
            }
            ResolvePath(NULL, NULL);
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
