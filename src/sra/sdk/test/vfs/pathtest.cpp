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
* Unit tests for VPath interface
*/

#include <ktst/unit_test.hpp>

#include <cstdlib>
#include <stdexcept>

#include <sysalloc.h>
#include <klib/text.h>
#include <vfs/path.h>

#include <common/test_assert.h>

TEST_SUITE(VPathTestSuite);

using namespace std;

class PathFixture
{
public:
    PathFixture()
    {
    }
    ~PathFixture()
    {
    }
    string PathToString(const string& p_path)
    {
        string filename = "filename";
        VPath* path;
        if (VPathMake(&path, p_path.c_str()))
           throw logic_error("PathToString: VPathMake failed");
        const String* uri;
        if (VPathMakeString(path, &uri))
           throw logic_error("PathToString: VPathMakeString failed");
        
        string ret = string(uri->addr, uri->len);
        
        free((void*)uri);
        if (VPathRelease(path))
           throw logic_error("PathToString: VPathRelease failed");
          
        return ret;
    }
};

FIXTURE_TEST_CASE(MakeStringPlain, PathFixture)
{
    string filename = "filename";
    REQUIRE_EQ(PathToString(filename), filename);
}

FIXTURE_TEST_CASE(MakeStringScheme, PathFixture)
{
    string filename = "ncbi-acc:filename";
    REQUIRE_EQ(PathToString(filename), filename);
}

FIXTURE_TEST_CASE(MakeStringBadScheme, PathFixture)
{
    string filename = "ncbi-file:filename";
    REQUIRE_EQ(PathToString(filename), filename);
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
const char UsageDefaultName[] = "test-resolver";

rc_t CC KMain ( int argc, char *argv [] )
{
    rc_t rc=VPathTestSuite(argc, argv);
    return rc;
}

}
