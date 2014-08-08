#include <vdb/manager.h> // VDBManager

#include <ktst/unit_test.hpp> // TEST_CASE
#include <kfg/config.h> 

#include <sysalloc.h>
#include <cstdlib>

#include <common/test_assert.h>

TEST_SUITE( VdbTestSuite )

extern "C" rc_t VDBManagerRunPeriodicTasks(const VDBManager*);

TEST_CASE( VdbMgr ) {
    const VDBManager *mgr = NULL;
    rc_t rc = VDBManagerMakeRead(NULL, NULL);
    if (rc == 0)
        FAIL( "FAIL: VDBManagerMakeRead(NULL) succeed" );

    rc = VDBManagerMakeRead(&mgr, NULL);
    if (rc != 0)
        FAIL( "failed to make VDB manager" );

    rc = VDBManagerAddRef(mgr);
    if (rc != 0)
        FAIL( "failed to AddRef VDB manager" );

    rc = VDBManagerRelease(mgr);
    if (rc != 0)
        FAIL( "failed to release VDB manager" );

    rc = VDBManagerVersion(mgr, NULL);
    if (rc == 0)
        FAIL( "FAIL: VDBManagerVersion(mgr, NULL) succeed" );

    uint32_t version = 0;
    rc = VDBManagerVersion(NULL, &version);
    if (rc == 0)
        FAIL( "FAIL: VDBManagerVersion(NULL) succeed" );

    rc = VDBManagerVersion(mgr, &version);
    if (rc == 0)
        CHECK_GE( version, (uint32_t)0x2000000 );
    else
        FAIL( "failed to get VDB manager Version" );

    rc = VDBManagerRunPeriodicTasks(NULL);
    if (rc == 0)
        FAIL( "FAIL: VDBManagerRunPeriodicTasks(NULL) succeed" );

    rc = VDBManagerRunPeriodicTasks(mgr);
    if (rc != 0)
        FAIL( "failed VDBManagerRunPeriodicTasks" );

    // In the following, NULL and "" are interpreted as "."

    // VDBManagerAddSchemaIncludePath
    REQUIRE_RC(VDBManagerAddSchemaIncludePath(mgr, NULL));
    REQUIRE_RC(VDBManagerAddSchemaIncludePath(mgr, ""));
#ifndef _WIN32
    REQUIRE_RC(VDBManagerAddSchemaIncludePath(mgr, "/"));
    REQUIRE_RC(VDBManagerAddSchemaIncludePath(mgr, "/usr"));
#else    
    REQUIRE_RC_FAIL(VDBManagerAddSchemaIncludePath(mgr, "/"));
    REQUIRE_RC_FAIL(VDBManagerAddSchemaIncludePath(mgr, "/usr"));
#endif

    // VDBManagerAddLoadLibraryPath
    REQUIRE_RC(VDBManagerAddLoadLibraryPath(mgr, NULL));
    REQUIRE_RC(VDBManagerAddLoadLibraryPath(mgr, ""));
#ifndef _WIN32
    REQUIRE_RC(VDBManagerAddLoadLibraryPath(mgr, "/"));
    REQUIRE_RC(VDBManagerAddLoadLibraryPath(mgr, "/usr"));
#else
    REQUIRE_RC_FAIL(VDBManagerAddLoadLibraryPath(mgr, "/"));
    REQUIRE_RC_FAIL(VDBManagerAddLoadLibraryPath(mgr, "/usr"));
#endif
    //

    struct Test { int i; };
    Test t;
    t.i = 12345;
    rc = VDBManagerGetUserData(mgr, NULL);
    if (rc == 0)
        FAIL( "FAIL: VDBManagerGetUserData(mgr, NULL) succeed" );
    void *data = NULL;
    rc = VDBManagerGetUserData(NULL, &data);
    if (rc == 0)
        FAIL( "FAIL: VDBManagerGetUserData(NULL) succeed" );
    rc = VDBManagerGetUserData(mgr, &data);
    if (rc != 0)
        FAIL( "failed VDBManagerGetUserData" );
    if (data != NULL)
        FAIL( "VDBManagerGetUserData != NULL" );

    rc = VDBManagerSetUserData(NULL, &t, free);
    if (rc == 0)
        FAIL( "FAIL: VDBManagerSetUserData(NULL) succeed" );
    rc = VDBManagerSetUserData(mgr, &t, NULL);
    if (rc != 0)
        FAIL( "failed VDBManagerSetUserData" );
    rc = VDBManagerGetUserData(mgr, &data);
    if (rc != 0)
        FAIL( "failed VDBManagerGetUserData after Set" );
    if (data == NULL)
        FAIL( "VDBManagerGetUserData == NULL after Get" );
    else {
        Test *r = static_cast<Test*>(data);
        CHECK_EQUAL( r->i, 12345 );
    }

    Test *pt = static_cast<Test*>(malloc(sizeof *pt));
    rc = VDBManagerSetUserData(mgr, pt, free);
    if (rc != 0)
        FAIL( "failed VDBManagerSetUserData(free)" );

    //

    rc = VDBManagerRelease(mgr);
    if (rc != 0)
        FAIL( "failed to release VDB manager" );
    mgr = NULL;

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

const char UsageDefaultName[] = "test-vdb";

rc_t CC KMain ( int argc, char *argv [] )
{
    KConfigDisableUserSettings();
    rc_t rc=VdbTestSuite(argc, argv);
    return rc;
}

}
