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
* Unit tests for SRA interfaces
*/

#include <cstdlib>
#include <cstring>

#include <ktst/unit_test.hpp>
#include <sra/srapath.hpp>
#include <klib/namelist.hpp>
#include <klib/printf.h>

#include <common/test_assert.h>

TEST_SUITE(SraTestSuite);

using namespace std;

std::string kfgDir="./";

static
string 
MakeDataPath(const char* path)
{
    return kfgDir+path;
}

// SRAPath tests
class SraPathFixture 
{
public:
    SraPathFixture() : path(0)
    {
        string kfgFile(kfgDir+"sratest.kfg");
#ifndef _WIN32
        setenv("VDB_CONFIG", kfgFile.c_str(), 1); 
#else
        _putenv((string("VDB_CONFIG=")+kfgFile).c_str()); 
#endif
        rc_t rc=SRAPath::Make(&path);
        if (rc != 0)
        {
            char buf[4096];
            size_t num_writ;
            string_printf(buf, sizeof(buf), &num_writ, "SRAPath::Make failed: %R", rc);
            FAIL(buf);
        }
        memset(pathBuff, 0, MaxPathBuff);
    }
    ~SraPathFixture() 
    {
        SRAPathRelease(path);
    }

    SRAPath* path;  
    static const size_t MaxPathBuff=4096;  
    char pathBuff[MaxPathBuff];
};
const size_t SraPathFixture::MaxPathBuff;

FIXTURE_TEST_CASE(SraPathVersion, SraPathFixture)
{
    REQUIRE_NOT_NULL(path);
    {
        uint32_t version=0xffffffff;
        REQUIRE_RC(SRAPathVersion(path, &version));
        REQUIRE_NE(version, 0xffffffff);
    }
}

// SRAPathFull, SRAPathTest
FIXTURE_TEST_CASE(SRAPathFull_should_work_on_an_existing_NCBI_accession, SraPathFixture)
{
    REQUIRE_RC(SRAPathFull(path, MakeDataPath("data/rep01/").c_str(), "ncbi01", "SRR001656", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(MakeDataPath("data/rep01/ncbi01/SRR/000001/SRR001656"), std::string(pathBuff));
    // the resulting path should pass SRAPathTest
    REQUIRE(SRAPathTest(path, pathBuff));    
    // same without '/' at the end of rep server
    REQUIRE_RC(SRAPathFull(path, MakeDataPath("data/rep01").c_str(), "ncbi01", "SRR001656", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(MakeDataPath("data/rep01/ncbi01/SRR/000001/SRR001656"), std::string(pathBuff));
    // without rep server or volume
    REQUIRE_RC(SRAPathFull(path, "", "", "SRR001656", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(std::string("SRR/000001/SRR001656"), std::string(pathBuff));
    // without volume
    REQUIRE_RC(SRAPathFull(path, MakeDataPath("data/rep01").c_str(), "", "SRR001656", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(MakeDataPath("data/rep01/SRR/000001/SRR001656"), std::string(pathBuff));
    // without rep server
    REQUIRE_RC(SRAPathFull(path, "", "ncbi01", "SRR001656", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(std::string("ncbi01/SRR/000001/SRR001656"), std::string(pathBuff));
}

FIXTURE_TEST_CASE(SRAPathFull_should_work_on_an_existing_EBI_accession, SraPathFixture)
{
    REQUIRE_RC(SRAPathFull(path, MakeDataPath("data/rep01/").c_str(), "ebi01", "ERR033753", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(MakeDataPath("data/rep01/ebi01/ERR/ERR033/ERR033753"), std::string(pathBuff));
    // the resulting path should pass SRAPathTest
    REQUIRE(SRAPathTest(path, pathBuff));    
    // without '/' at the end of rep server
    REQUIRE_RC(SRAPathFull(path, MakeDataPath("data/rep01").c_str(), "ebi01", "ERR033753", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(MakeDataPath("data/rep01/ebi01/ERR/ERR033/ERR033753"), std::string(pathBuff));
    // without rep server
    REQUIRE_RC(SRAPathFull(path, "", "ebi01", "ERR033753", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(std::string("ebi01/ERR/ERR033/ERR033753"), std::string(pathBuff));
    // without volume, parsed as an NCBI accession (bank 000032)
    REQUIRE_RC(SRAPathFull(path, "", "", "ERR033753", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(std::string("ERR/000032/ERR033753"), std::string(pathBuff));
    // without rep server or volume, parsed as an NCBI accession (bank 000032)
    REQUIRE_RC(SRAPathFull(path, MakeDataPath("data/rep01").c_str(), "", "ERR033753", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(MakeDataPath("data/rep01/ERR/000032/ERR033753"), std::string(pathBuff));
}
FIXTURE_TEST_CASE(SRAPathFull_should_work_on_an_existing_DDBJ_accession, SraPathFixture)
{
    REQUIRE_RC(SRAPathFull(path, MakeDataPath("data/rep01/").c_str(), "ddbj01", "SRR002000", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(MakeDataPath("data/rep01/ddbj01/SRR/000001/SRR002000"), std::string(pathBuff));
    // the resulting path should pass SRAPathTest
    REQUIRE(SRAPathTest(path, pathBuff));    
    // same without '/' at the end of rep server
    REQUIRE_RC(SRAPathFull(path, MakeDataPath("data/rep01").c_str(), "ddbj01", "SRR002000", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(MakeDataPath("data/rep01/ddbj01/SRR/000001/SRR002000"), std::string(pathBuff));
    // without rep server
    REQUIRE_RC(SRAPathFull(path, "", "ddbj01", "SRR002000", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(std::string("ddbj01/SRR/000001/SRR002000"), std::string(pathBuff));
    // without rep server or volume, parsed as an NCBI accession
    REQUIRE_RC(SRAPathFull(path, "", "", "SRR002000", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(std::string("SRR/000001/SRR002000"), std::string(pathBuff));
    // without volume, parsed as an NCBI accession 
    REQUIRE_RC(SRAPathFull(path, MakeDataPath("data/rep01").c_str(), "", "SRR002000", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(MakeDataPath("data/rep01/SRR/000001/SRR002000"), std::string(pathBuff));
}
FIXTURE_TEST_CASE(SRAPathFull_should_work_on_an_existing_REFSEQ_accession_using_flat_naming, SraPathFixture)
{
    REQUIRE_RC(SRAPathFull(path, MakeDataPath("data/rep01/").c_str(), "refseq", "SRR002274", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(MakeDataPath("data/rep01/refseq/SRR002274"), std::string(pathBuff));
    // the resulting path should pass SRAPathTest
    REQUIRE(SRAPathTest(path, pathBuff));    
}
FIXTURE_TEST_CASE(SRAPathFull_should_work_on_an_existing_WGS_accession, SraPathFixture)
{
    REQUIRE_RC(SRAPathFull(path, MakeDataPath("data/rep01/").c_str(), "wgs", "AAQQ01", pathBuff, MaxPathBuff));
    REQUIRE_EQUAL(MakeDataPath("data/rep01/wgs/WGS/AA/QQ/AAQQ01"), std::string(pathBuff));
    // the resulting path should pass SRAPathTest
    REQUIRE(SRAPathTest(path, pathBuff));    
}
FIXTURE_TEST_CASE(SRAPathFull_should_reject_badly_named_WGS_accession, SraPathFixture)
{
    // has to have at least 4 characters in the name
    REQUIRE_RC_FAIL(SRAPathFull(path, MakeDataPath("data/rep01/").c_str(), "wgs", "AAQ", pathBuff, MaxPathBuff));
}

FIXTURE_TEST_CASE(SRAPathFull_should_report_insufficient_buffer, SraPathFixture)
{
    REQUIRE_RC_FAIL(SRAPathFull(path, "/panfs/traces01", "sra2", "SRR001656", pathBuff, 2));
}
    
FIXTURE_TEST_CASE(SRAPathTest_should_reject_nonexistent_accession, SraPathFixture)
{
    REQUIRE(!SRAPathTest(path, "/panfs/not/an/accession/"));
}

// SRAPathFind
/* macro for use inside test cases for SRAPathFind */
#define TEST_ACCESSION(accession, location) \
    REQUIRE_RC(SRAPathFind(path, accession, pathBuff, MaxPathBuff)); \
    REQUIRE_EQUAL(std::string(location accession), std::string(pathBuff)); \
    /* the resulting path should pass SRAPathTest */ \
    REQUIRE(SRAPathTest(path, pathBuff))

FIXTURE_TEST_CASE(SRAPathFind_should_find_an_existing_NCBI_accession, SraPathFixture)
{
    TEST_ACCESSION("SRR001656",             "data/rep01/ncbi01/SRR/000001/");

    // single file version of SRR001656 (tar)
    REQUIRE_RC(SRAPathFind(path, "SRR001656.tar", pathBuff, MaxPathBuff)); 
    REQUIRE_EQUAL(std::string("data/rep01/ncbi01/SRR/000001/SRR001656.tar"), std::string(pathBuff));
    // compressed single file version of SRR001656
    REQUIRE_RC(SRAPathFind(path, "SRR001657.tar.gz", pathBuff, MaxPathBuff)); 
    REQUIRE_EQUAL(std::string("data/rep01/ncbi01/SRR/000001/SRR001657.tar.gz"), std::string(pathBuff));
}
FIXTURE_TEST_CASE(SRAPathFind_should_find_an_existing_EBI_accession, SraPathFixture)
{
    TEST_ACCESSION("ERR033753",             "data/rep01/ebi01/ERR/ERR033/");
    TEST_ACCESSION("ERR033753.sra",         "data/rep01/ebi01/ERR/ERR033/");
    TEST_ACCESSION("ERR033753.lite.sra",    "data/rep01/ebi01/ERR/ERR033/");
    TEST_ACCESSION("ERR033753.csra",        "data/rep01/ebi01/ERR/ERR033/");

    // single file version ot ERR033753 (kar) - SRAPathTest will fail on it, so we can't use TEST_ACCESSION
    REQUIRE_RC(SRAPathFind(path, "ERR033754.kar", pathBuff, MaxPathBuff)); 
    REQUIRE_EQUAL(std::string("data/rep01/ebi01/ERR/ERR033/ERR033754.kar"), std::string(pathBuff));
}
FIXTURE_TEST_CASE(SRAPathFind_should_find_an_existing_DDBJ_accession, SraPathFixture)
{
    TEST_ACCESSION("SRR002000", "data/rep01/ddbj01/SRR/000001/");
}
FIXTURE_TEST_CASE(SRAPathFind_should_find_an_existing_REFSEQ_accession_using_flat_naming, SraPathFixture)
{
    TEST_ACCESSION("SRR002274", "data/rep01/refseq/");
}
FIXTURE_TEST_CASE(SRAPathFind_should_find_an_existing_WGS_accession, SraPathFixture)
{
    TEST_ACCESSION("AAQQ01", "data/rep01/wgs/WGS/AA/QQ/");
}
FIXTURE_TEST_CASE(SRAPathFind_should_not_find_a_nonexisting_accession, SraPathFixture)
{    
    REQUIRE_RC_FAIL(SRAPathFind(path, "SRR999999", pathBuff, MaxPathBuff));
}
FIXTURE_TEST_CASE(SRAPathFind_should_report_insufficient_buffer, SraPathFixture)
{
    REQUIRE_RC_FAIL(SRAPathFind(path, "SRR001656", pathBuff, 1));
}    

FIXTURE_TEST_CASE(SRAPathFind_should_gracefully_reject_http_path, SraPathFixture)
{
    REQUIRE_RC_FAIL(SRAPathFind(path, "http://ftp-private.ncbi.nlm.nih.gov/sra/refseq/AAAB01000001.1", pathBuff, MaxPathBuff));
}

// SRAPathClear
FIXTURE_TEST_CASE(SRAPathClear_should_empty_the_SRA_PathList, SraPathFixture)
{
    REQUIRE_RC(SRAPathClear(path));
    REQUIRE_RC_FAIL(SRAPathFind(path, "SRR001656", pathBuff, MaxPathBuff));
}

// SRAPathAddRepPath, SRAPathAddVolPath
FIXTURE_TEST_CASE(SRAPathAddRepPath_and_SRAPathAddVolPath_should_enable_searching, SraPathFixture)
{   
    // clear search path, make sure accession does not get found
    REQUIRE_RC(SRAPathClear(path));
    REQUIRE_RC_FAIL(SRAPathFind(path, "SRR001656", pathBuff, MaxPathBuff));
    // populate server/volume search path, make sure the accession gets found
    REQUIRE_RC(SRAPathAddRepPath(path, "data/rep01"));
    REQUIRE_RC(SRAPathAddVolPath(path, "ncbi01"));
    REQUIRE_RC(SRAPathFind(path, "SRR001656", pathBuff, MaxPathBuff));
}

// SRAPathList 
FIXTURE_TEST_CASE(SRAPathList_should_be_rejected, SraPathFixture)
{
    struct KNamelist * nl;
    REQUIRE_RC_FAIL(SRAPathList(path, &nl, true));
}

// New style resolution: SRAPathFull, SRAPathTest
// locate accessions in different locations with varying server and volume names (see sratest.kfg for details of dir structure)
FIXTURE_TEST_CASE(SRAPathFull_new_NCBI_accession, SraPathFixture)
{   
    TEST_ACCESSION("SRR001657", "data/srv1/ncbi1/SRR/000001/");
    TEST_ACCESSION("SRR001658", "data/srv1/ncbi2/SRR/000001/");
    TEST_ACCESSION("SRR001659", "data/srv2/ncbi2/SRR/000001/");
}
FIXTURE_TEST_CASE(SRAPathFull_new_EBI_accession, SraPathFixture)
{   
    TEST_ACCESSION("ERR033754", "data/srv1/ebi1/ERR/ERR033/");
    TEST_ACCESSION("ERR033755", "data/srv1/ebi2/ERR/ERR033/");
    TEST_ACCESSION("ERR033756", "data/srv2/ebi2/ERR/ERR033/");
}
FIXTURE_TEST_CASE(SRAPathFull_new_DDBJ_accession, SraPathFixture)
{ 
    TEST_ACCESSION("SRR002002", "data/srv1/ddbj1/SRR/000001/");
    TEST_ACCESSION("SRR002003", "data/srv1/ddbj2/SRR/000001/");
    TEST_ACCESSION("SRR002005", "data/srv2/ddbj2/SRR/000001/");
}
FIXTURE_TEST_CASE(SRAPathFull_new_REFSEQ_accession, SraPathFixture)
{
    TEST_ACCESSION("SRR002275", "data/srv2/refseq/");
}
FIXTURE_TEST_CASE(SRAPathFull_new_WSG_accession, SraPathFixture)
{ 
    TEST_ACCESSION("AAQQ02", "data/srv1/wgs1/WGS/AA/QQ/");
    TEST_ACCESSION("AAQQ03", "data/srv1/wgs2/WGS/AA/QQ/");
    TEST_ACCESSION("AAQQ04", "data/srv2/wgs2/WGS/AA/QQ/");
}

//////////////////////////////////////////// Main
extern "C"
{

#include <kapp/args.h>
#include <kfg/config.h>

ver_t CC KAppVersion ( void )
{
    return 0x1000000;
}
rc_t CC UsageSummary (const char * progname)
{
    return 0;
}

rc_t CC Usage ( const Args * args )
{
    return 0;
}
const char UsageDefaultName[] = "test-sra";

rc_t CC KMain ( int argc, char *argv [] )
{
    KConfigDisableUserSettings();
    rc_t rc=SraTestSuite(argc, argv);
    return rc;
}

}
