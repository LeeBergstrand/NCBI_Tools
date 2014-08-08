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
#include <klib/report.h>

// overrride KDirectoryPathType to refer to our test double
#define KDirectoryPathType KDirectoryPathType_TestImpl
// #define SRAPathMakeImpl SRAPathMakeTestImpl
extern "C" {
#include "srapath.c"
}

using namespace std;

TEST_SUITE(SrapathWbTestSuite);

//////////////////////////////////////////// tests for internal functions of srapath
#include <vector>
#include <string>
#include <stdexcept>

typedef vector<string> Paths;
Paths paths;

// test double for calls to KDirectoryPathType
uint32_t CC KDirectoryPathType_TestImpl ( const KDirectory *self, const char *path, ... )
{
	paths.push_back(path);
	return kptDir;
}

class FastFindFixture
{
public:
	FastFindFixture()
	{
        string kfgFile("./config.kfg");
#ifndef _WIN32
        setenv("VDB_CONFIG", kfgFile.c_str(), 1); 
#else
        _putenv((string("VDB_CONFIG=")+kfgFile).c_str()); 
#endif
	
		if ( SRAPathMakeImpl ( ( SRAPath * * ) & pm, NULL ) != 0)
			throw logic_error("FastFindFixture.ctor: SRAPathMakeImpl failed");
			
		paths.clear();
		
	}
	~FastFindFixture()
	{
		if ( SRAPathRelease ( ( SRAPath * ) pm ) != 0)
			throw std::logic_error("FastFindFixture.dtor: SRAPathRelease failed");
	}
	
	NCBISRAPath* pm;
	char path[1024];
};

FIXTURE_TEST_CASE(FastFindNcbi, FastFindFixture)
{
	REQUIRE_RC(FindFast( pm, "SRR000000", path, sizeof(path), NULL));
	
	REQUIRE_EQ(paths.size(), (size_t)2); // 1st call to KDirectoryPathType is for the server, 2nd call for the accession
	REQUIRE_EQ( paths[0], string("/panfs/traces01") );
	REQUIRE_EQ( paths[1], string("/panfs/traces01/sra0/SRR/000000/SRR000000") );
}

FIXTURE_TEST_CASE(FastFindEbi, FastFindFixture)
{
	REQUIRE_RC(FindFast( pm, "ERR000000", path, sizeof(path), NULL));
	
	REQUIRE_EQ(paths.size(), (size_t)2);
	REQUIRE_EQ( paths[1], string("/panfs/traces01/era2/ERR/ERR000/ERR000000") );
}

FIXTURE_TEST_CASE(FastFindDdbj, FastFindFixture)
{
	REQUIRE_RC(FindFast( pm, "DRR000000", path, sizeof(path), NULL));
	
	REQUIRE_EQ(paths.size(), (size_t)2);
	REQUIRE_EQ( paths[1], string("/panfs/traces01/dra0/DRR/000000/DRR000000") );
}

FIXTURE_TEST_CASE(FastFindWgs, FastFindFixture)
{
	REQUIRE_RC(FindFast( pm, "AAAA00", path, sizeof(path), NULL));
	
	REQUIRE_EQ(paths.size(), (size_t)2);
	REQUIRE_EQ( paths[1], string("/panfs/traces01/wgs0/WGS/AA/AA/AAAA00") );
}

FIXTURE_TEST_CASE(FastFindRefseq, FastFindFixture)
{
	REQUIRE_RC(FindFast( pm, "AAAA00.01", path, sizeof(path), NULL));
	
	REQUIRE_EQ(paths.size(), (size_t)2);
	REQUIRE_EQ( paths[1], string("/panfs/traces01/refseq/AAAA00.01") );
}

//////////////////////////////////////////// Main
extern "C"
{

#include <kapp/args.h>

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

const char UsageDefaultName[] = "wb-test-srapath";

rc_t CC KMain ( int argc, char *argv [] )
{
	ReportSilence();
    rc_t rc = SrapathWbTestSuite(argc, argv);
    return rc;
}

}
