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
* Unit tests for Kfs interface
*/

#include <cstring>

#include <ktst/unit_test.hpp>
#include <kfs/mmap.h>
#include <kfs/directory.h>
#include <kfs/impl.h>

#include <common/test_assert.h>

using namespace std;

TEST_SUITE(KfsTestSuite);

TEST_CASE(KMMapMakeRead_and_KMMapRelease)
{   // create a temporary file, open it with KMMapMakeRead, close KMMap, try to delete 

    KDirectory *wd;
    REQUIRE_RC(KDirectoryNativeDir ( & wd ));

    const char* fileName="test.file";

    {   // create temp file, close
        KFile* file;
        const char* contents="contents";
        REQUIRE_RC(KDirectoryCreateFile(wd, &file, true, 0664, kcmInit, fileName));
        size_t num_writ=0;
        REQUIRE_RC(KFileWrite(file, 0, contents, strlen(contents), &num_writ));
        REQUIRE_RC(KFileRelease(file));
    }

    {   // open, memory-map, close
        const KFile* file;
        REQUIRE_RC(KDirectoryOpenFileRead(wd, &file, fileName));
        const KMMap * mm;
        REQUIRE_RC(KMMapMakeRead(&mm, file));
        REQUIRE_RC(KMMapRelease(mm));

        REQUIRE_RC(KFileRelease(file));
    }

    // now, remove the file
    // on Windows: used to return ACCESS_DENIED, not removed file 
    // (cause: no call to UnmapViewOfFile in libs\kfs\win\KMapUnmap)
    REQUIRE_RC(KDirectoryRemove(wd, false, fileName)); 

    REQUIRE_RC(KDirectoryRelease ( wd ));
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

const char UsageDefaultName[] = "test-kfs";

rc_t CC KMain ( int argc, char *argv [] )
{
    KConfigDisableUserSettings();
    rc_t rc=KfsTestSuite(argc, argv);
    return rc;
}

}
