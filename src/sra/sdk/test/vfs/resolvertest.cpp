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
* Unit tests for VResolver interfaces
*/

#include <ktst/unit_test.hpp>
#include <sysalloc.h>

#include <stdexcept>
#include <cstring>
#include <cstdlib>

#include <stdio.h>

#include <klib/text.h>

#include <kfg/config.h>

#include <kfs/directory.h>
#include <kfs/file.h>
#include <kfs/cacheteefile.h>

#include <vfs/resolver.h>
#include <vfs/manager.h>
#include <vfs/path.h>

#include <common/test_assert.h>

TEST_SUITE(ResolverTestSuite);

using namespace std;

class ResolverFixture 
{
public:
    ResolverFixture()
    : mgr(0), wd(0), kfgFile(0), cfg(0), res(0), resolvedPath(0), accPath(0)
    {
    }
    ~ResolverFixture() 
    {
        if (kfgFile && KFileRelease(kfgFile) != 0)
           throw logic_error("ResolverFixture: KFileRelease failed");
        if (cfg && KConfigRelease(cfg) != 0)
           throw logic_error("ResolverFixture: KConfigRelease failed");
        if (wd && KDirectoryRelease(wd) != 0)
           throw logic_error("ResolverFixture: KDirectoryRelease failed");
        if (mgr && VFSManagerRelease(mgr) != 0)
           throw logic_error("ResolverFixture: VFSManagerRelease failed");
        if (resolvedPath && VPathRelease(resolvedPath) != 0)
           throw logic_error("ResolverFixture: VPathRelease(resolvedPath) failed");
        if (accPath && VPathRelease(accPath) != 0)
           throw logic_error("ResolverFixture: VPathRelease(accPath) failed");
        if (res && VResolverRelease(res) != 0)
           throw logic_error("ResolverFixture: VResolverRelease failed");
    }
    void LoadConfig()
    {
        if (KDirectoryNativeDir(&wd) != 0 || wd == 0)
           throw logic_error("LoadConfig: KDirectoryNativeDir failed");
        if (KDirectoryOpenFileRead(wd, &kfgFile, "resolver.kfg") != 0 || kfgFile == 0)
           throw logic_error("LoadConfig: KDirectoryOpenFileRead failed");
        if (KConfigMake(&cfg, NULL) != 0 || cfg == 0)
           throw logic_error("LoadConfig: KConfigMake failed");
        if (KConfigLoadFile(cfg, "resolver.kfg", kfgFile) != 0) 
           throw logic_error("LoadConfig: KConfigLoadFile failed");
           
        const KConfigNode* node;
        if (KConfigOpenNodeRead(cfg, &node, "test-root") != 0)
           throw logic_error("LoadConfig: KConfigOpenNodeRead(\"test-root\") failed");
        char buffer[1024];
        size_t num_read;
        if (KConfigNodeRead(node, 0, buffer, sizeof(buffer), &num_read, 0) != 0 || num_read <= 1)
           throw logic_error("LoadConfig: KConfigNodeRead failed");
        kfgRoot=string(buffer, num_read);
        if (KConfigNodeRelease(node) != 0)
           throw logic_error("LoadConfig: KConfigNodeRelease failed");
    }
    void GetResolver()
    {
        LoadConfig();
        if (VFSManagerMake(&mgr) != 0 || mgr == 0)
           throw logic_error("GetResolver: VFSManagerMake failed");
           
        if (VFSManagerMakeResolver(mgr, &res,  cfg) != 0 || res == 0)
           throw logic_error("GetResolver: VFSManagerMakeResolver failed");
    }
    string VPathToString(const VPath* path)
    {
        const String* p;
        if (VPathMakeString(path, &p))
           throw logic_error("VPathToString: VPathMakeString failed");
        string ret(p->addr);
        free( (void*)p );
        return ret;
    }
    
    VFSManager* mgr;
    KDirectory* wd;
    const KFile* kfgFile;
    KConfig* cfg;
    VResolver* res;
    const VPath* resolvedPath;
    VPath* accPath;
    string kfgRoot;
};

#define ResolveLocal(acc, resolved)\
    GetResolver();\
    REQUIRE_RC(VPathMake(&accPath, acc));\
    REQUIRE_RC(VResolverLocal(res, accPath, &resolvedPath));\
    REQUIRE_EQ(VPathToString(resolvedPath), string(resolved));


//// A temp place for the one test I'mworking on
FIXTURE_TEST_CASE(XRemoteUriExistsNoKFile, ResolverFixture)
{
    GetResolver();
    REQUIRE_RC(VPathMake(&accPath, "ncbi-acc:SRR000001"));
    REQUIRE_RC(VResolverRemote(res, accPath, &resolvedPath, NULL)); 
    REQUIRE_EQ(string("http://ftp-trace.ncbi.nlm.nih.gov/sra/sra-instant/reads/ByRun/sra/SRR/SRR000/SRR000001/SRR000001.sra"), 
               VPathToString(resolvedPath));
}



// basics
FIXTURE_TEST_CASE(Make, ResolverFixture)
{
    REQUIRE_RC(VFSManagerMake(&mgr));
    REQUIRE_RC(KDirectoryNativeDir(&wd));
    REQUIRE_RC(KDirectoryOpenFileRead(wd, &kfgFile, "resolver.kfg"));
    REQUIRE_RC(KConfigMake(&cfg, NULL));
    REQUIRE_RC(KConfigLoadFile(cfg, NULL, kfgFile));

    REQUIRE_RC(VFSManagerMakeResolver(mgr, &res,  cfg));
    REQUIRE_NOT_NULL(res);
}


// user/public repository
FIXTURE_TEST_CASE(PublicUriExistsSra, ResolverFixture)
{
    ResolveLocal("ncbi-acc:SRR999998", kfgRoot+"/ncbi/public/sra/SRR999998.sra");
}
FIXTURE_TEST_CASE(PublicUriExistsRefseq, ResolverFixture)
{
    ResolveLocal("ncbi-acc:ZZ999999.9", kfgRoot+"/ncbi/public/refseq/ZZ999999.9");
}
FIXTURE_TEST_CASE(PublicUriExistsWgsLong, ResolverFixture)
{
    ResolveLocal("ncbi-acc:YYZZ99000001.1", kfgRoot+"/ncbi/public/wga/WGS/YY/ZZ/YYZZ99");
}    
FIXTURE_TEST_CASE(PublicUriExistsWgsShort, ResolverFixture)
{
    ResolveLocal("ncbi-acc:YYZZ99000001", kfgRoot+"/ncbi/public/wga/WGS/YY/ZZ/YYZZ99");
}
FIXTURE_TEST_CASE(PublicUriExistsWgsRefseq, ResolverFixture)
{
    ResolveLocal("ncbi-acc:YYZZ99000001?vdb-ctx=refseq", "ncbi-file:" + kfgRoot + "/ncbi/public/wga/YYZZ99#tbl/YYZZ99000001");
}

// // user/protected repositories
// not yet implemented 
// FIXTURE_TEST_CASE(ProtectedUriExists1, ResolverFixture)
// {
    // ResolveLocal("ncbi-acc:SRR999997", kfgRoot+"/ncbi/protected1/SRR999997"); // not implemented
// }
// FIXTURE_TEST_CASE(ProtectedUriExists2, ResolverFixture)
// {
    // ResolveLocal("ncbi-acc:SRR999996", kfgRoot+"/ncbi/protected2/sra/SRR999996"); // not implemented
// }

// site repositories - tests assume some external accessions exist:
//  /.../traces04/sra0/SRR/000004/SRR005020
//  /.../traces01/refseq/AE016830.1
//  /.../traces01/refseq/AAAB01, an archive containing AAAB01000001.1
//  /.../traces01/wgs01/WGS/AB/AB/ABAB01.1

#if defined(__APPLE__)
    #define TRACES01 string("/net/traces01")
    #define TRACES04 string("/net/traces04")
#elif defined(_WIN32)
    #define TRACES01 string("//panfs/traces01")
    #define TRACES04 string("//traces04")
#else
    #define TRACES01 string("/panfs/traces01.be-md.ncbi.nlm.nih.gov")
    #define TRACES04 string("/netmnt/traces04")
#endif

FIXTURE_TEST_CASE(SiteUriExistsSra, ResolverFixture)
{
    ResolveLocal("ncbi-acc:SRR005020", TRACES04 + "/sra0/SRR/000004/SRR005020");
}

FIXTURE_TEST_CASE(SiteUriExistsRefseq, ResolverFixture)
{
    ResolveLocal("ncbi-acc:AL111168.1", TRACES04 + "/refseq/AL111168.1");
}

FIXTURE_TEST_CASE(SiteUriExistsRefseqArchived, ResolverFixture)
{
    ResolveLocal("ncbi-acc:AAAB01000001.1", TRACES04 + "/wgs01/WGS/AA/AB/AAAB01");
}
FIXTURE_TEST_CASE(SiteUriExistsWgs, ResolverFixture)
{
    ResolveLocal("ncbi-acc:ABAB01000001", TRACES04 + "/wgs01/WGS/AB/AB/ABAB01");
}

FIXTURE_TEST_CASE(SiteNameExistsSra, ResolverFixture)
{
    ResolveLocal("SRR005020", TRACES04 + "/sra0/SRR/000004/SRR005020");
}

FIXTURE_TEST_CASE(SiteUriDoesNotExist, ResolverFixture)
{
    GetResolver();
    REQUIRE_RC(VPathMake(&accPath, "ncbi-acc:SRR999999"));
    REQUIRE_RC_FAIL(VResolverLocal(res, accPath, &resolvedPath));
}

FIXTURE_TEST_CASE(SiteNameDoesNotExist, ResolverFixture)
{
    GetResolver();
    REQUIRE_RC(VPathMake(&accPath, "SRR999999"));
    REQUIRE_RC_FAIL(VResolverLocal(res, accPath, &resolvedPath));
}

FIXTURE_TEST_CASE(SiteNamePath, ResolverFixture)
{
    GetResolver();
    REQUIRE_RC(VPathMake(&accPath, (TRACES04 + "/sra0/SRR/000004/SRR005020").c_str())); // file exists but pathname should be rejected
    REQUIRE_RC_FAIL(VResolverLocal(res, accPath, &resolvedPath));
}

FIXTURE_TEST_CASE(SiteBadScheme, ResolverFixture)
{
    GetResolver();
    REQUIRE_RC(VPathMake(&accPath, "ncbi-file:SRR005020")); 
    REQUIRE_RC_FAIL(VResolverLocal(res, accPath, &resolvedPath));
}

FIXTURE_TEST_CASE(SiteUriDisabled, ResolverFixture)
{
    LoadConfig();
    // disable one of the site repositories
    KConfigNode* node;
    REQUIRE_RC(KConfigOpenNodeUpdate(cfg, &node, "repository/site/traces04/disabled"));
    REQUIRE_RC(KConfigNodeWrite(node, "true", strlen("true")));
    REQUIRE_RC(KConfigNodeRelease(node));

    // apply Config to create a Resolver
    REQUIRE_RC(VFSManagerMake(&mgr));
    REQUIRE_NOT_NULL(mgr);
    REQUIRE_RC(VFSManagerMakeResolver(mgr, &res,  cfg));
    REQUIRE_NOT_NULL(res);
    
    // should not find the accessions in the disabled repository
    REQUIRE_RC(VPathMake(&accPath, "ncbi-acc:SRR0050200")); 
    REQUIRE_RC_FAIL(VResolverLocal(res, accPath, &resolvedPath));
    
    // other site repositories unaffected
    REQUIRE_RC(VPathMake(&accPath, "ncbi-acc:AM180355.1"));
    REQUIRE_RC(VResolverLocal(res, accPath, &resolvedPath));
    REQUIRE_EQ(string(TRACES04 + "/refseq/AM180355.1"), VPathToString(resolvedPath));
}

// remote repository
FIXTURE_TEST_CASE(RemoteUriExistsNoKFile, ResolverFixture)
{
    GetResolver();
    REQUIRE_RC(VPathMake(&accPath, "ncbi-acc:SRR000001"));
    REQUIRE_RC(VResolverRemote(res, accPath, &resolvedPath, NULL)); 
    REQUIRE_EQ(string("http://ftp-trace.ncbi.nlm.nih.gov/sra/sra-instant/reads/ByRun/sra/SRR/SRR000/SRR000001/SRR000001.sra"), 
               VPathToString(resolvedPath));
}

FIXTURE_TEST_CASE(RemoteUriExists, ResolverFixture)
{
    GetResolver();
    REQUIRE_RC(VPathMake(&accPath, "ncbi-acc:SRR000001"));
    const KFile* file;
    REQUIRE_RC(VResolverRemote(res, accPath, &resolvedPath, &file)); 
    REQUIRE_EQ(string("http://ftp-trace.ncbi.nlm.nih.gov/sra/sra-instant/reads/ByRun/sra/SRR/SRR000/SRR000001/SRR000001.sra"), 
               VPathToString(resolvedPath));
    REQUIRE_NOT_NULL(file);
    //TODO: verify file
}

FIXTURE_TEST_CASE(RemoteUriDoesNotExist, ResolverFixture)
{
    GetResolver();
    REQUIRE_RC(VPathMake(&accPath, "ncbi-acc:SRR888888"));
    const KFile* file;
    REQUIRE_RC_FAIL(VResolverRemote(res, accPath, &resolvedPath, &file)); 
}

FIXTURE_TEST_CASE(RemoteUriExistsRefseq, ResolverFixture)
{
    GetResolver();
    REQUIRE_RC(VPathMake(&accPath, "ncbi-acc:AE016830.1"));
    REQUIRE_RC(VResolverRemote(res, accPath, &resolvedPath, NULL)); 
    REQUIRE_EQ(string("http://ftp-trace.ncbi.nlm.nih.gov/sra/refseq/AE016830.1"), 
               VPathToString(resolvedPath));
}

FIXTURE_TEST_CASE(RemoteUriExistsRefseqArchived, ResolverFixture)
{
    GetResolver();
    REQUIRE_RC(VPathMake(&accPath, "ncbi-acc:AAAB01000001.1?vdb-ctx=refseq"));
    REQUIRE_RC(VResolverRemote(res, accPath, &resolvedPath, NULL)); 
    REQUIRE_EQ(string("http://ftp-trace.ncbi.nlm.nih.gov/sra/refseq/AAAB01#tbl/AAAB01000001.1"), 
               VPathToString(resolvedPath));
}

// cache
class ResolverCacheFixture : public ResolverFixture
{
public:
    ResolverCacheFixture()
    : temp_file(0), curlFile(0), cachedFile(0)
    {
        GetResolver();
    }
    ~ResolverCacheFixture()
    {
        if (curlFile && KFileRelease(curlFile) != 0)
            throw logic_error("ResolverCacheFixture::~ResolverCacheFixture:KFileRelease(curlFile) failed");
        if (cachedFile && VPathRelease(cachedFile) != 0)
            throw logic_error("ResolverCacheFixture::~ResolverCacheFixture:VPathRelease failed");
        KDirectoryRemove(wd, true, cachedFileName.c_str());
        if (temp_file && KFileRelease(temp_file) != 0)
            throw logic_error("ResolverCacheFixture::~ResolverCacheFixture:KFileRelease(temp_file) failed");
    }
    void Setup(const char* acc, const string& cachedFile)
    {   
        cachedFileName = cachedFile;
        KDirectoryRemove(wd, true, cachedFileName.c_str());
        if (VPathMake(&accPath, acc) != 0)
            throw logic_error("ResolverCacheFixture::Setup:VPathMake failed");
    }
    void DownloadFile(const KFile* file, size_t expectedSize)
    {
        char* buffer = new char[expectedSize];
        size_t num_read;
        if (KFileRead(file, 0, buffer, expectedSize, &num_read) != 0)
            throw logic_error("ResolverCacheFixture::DownloadFile:KFileRead failed");
        delete [] buffer;
        if (KFileRelease(temp_file) != 0)
            throw logic_error("ResolverCacheFixture::DownloadFile:KFileRelease failed");
        temp_file = 0;
    }
    uint64_t FileSize(const string& file)
    {
        uint64_t size = 0;
        if (KDirectoryFileSize(wd, &size, file.c_str()) != 0)
            throw logic_error("ResolverCacheFixture::FileSize:KDirectoryFileSize failed");
        return size;
    }
    
    const KFile* temp_file;
    string cachedFileName;
    size_t expectedSize;
    const KFile* curlFile;
    const VPath* cachedFile;
};

FIXTURE_TEST_CASE(CacheNameDoesNotExist, ResolverCacheFixture)
{
    GetResolver();
    REQUIRE_RC(VPathMake(&accPath, "SRR999999"));
    REQUIRE_RC_FAIL(VResolverCache(res, accPath, &cachedFile, expectedSize));
}

FIXTURE_TEST_CASE(CacheRefseq, ResolverCacheFixture)
{
    Setup("AE017262.1", kfgRoot + "/ncbi/public/refseq/AE017262.1"); // removes previously cached file
    
    REQUIRE_RC(VResolverRemote(res, accPath, &resolvedPath, &curlFile)); 
    uint64_t size = 0;
    REQUIRE_RC(KFileSize(curlFile, &size));
    expectedSize = (size_t)size;
    REQUIRE_NE(expectedSize, (size_t)0);

    REQUIRE_RC(VResolverCache(res, resolvedPath, &cachedFile, expectedSize));
    REQUIRE_EQ(VPathToString(cachedFile), cachedFileName);
    
    REQUIRE_RC(KDirectoryMakeCacheTee ( wd, &temp_file, curlFile, NULL, 0, 2, false, 
                                        "%s", VPathToString(cachedFile).c_str()));
    REQUIRE_NOT_NULL(temp_file);
    
    DownloadFile(temp_file, expectedSize);
    REQUIRE_LE((uint64_t)expectedSize, FileSize(cachedFileName));
}
FIXTURE_TEST_CASE(CacheSra, ResolverCacheFixture)
{
    Setup("SRR000001", kfgRoot + "/ncbi/public/sra/SRR000001.sra");
    REQUIRE_RC(VResolverRemote(res, accPath, &resolvedPath, &curlFile)); 
    REQUIRE_RC(VResolverCache(res, resolvedPath, &cachedFile, expectedSize));
    REQUIRE_EQ(VPathToString(cachedFile), cachedFileName);
}

FIXTURE_TEST_CASE(CacheRefseqArchived1, ResolverCacheFixture)
{
    Setup("ncbi-acc:AAAB01000001.1?vdb-ctx=refseq", kfgRoot + "/ncbi/public/refseq/AAAB01");

    REQUIRE_RC(VResolverRemote(res, accPath, &resolvedPath, &curlFile)); 
    REQUIRE_RC(VResolverCache(res, resolvedPath, &cachedFile, expectedSize));
    REQUIRE_EQ(VPathToString(cachedFile), cachedFileName);
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
    KConfigDisableUserSettings();
    rc_t rc=ResolverTestSuite(argc, argv);
    return rc;
}

}
