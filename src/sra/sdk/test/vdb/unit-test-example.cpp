#include <vdb/manager.h> // VDBManager
#include <vdb/schema.h> // VSchema
#include <kdb/manager.h> // KDBManager
#include <klib/text.h> // String
#include <klib/rc.h> // enum RC

#include <ktst/unit_test.hpp> // TEST_CASE

using std::string;

struct GlobalFixture {
    GlobalFixture() {
        TEST_MESSAGE( "SETUP" );

        kmgr = NULL;
        vmgr = NULL;
        schema = NULL;

        rc_t rc = KDBManagerMakeUpdate ( & kmgr, NULL );
        if (rc != 0) {
            FAIL( "failed to make KDB manager" );
        }

        rc = VDBManagerMakeUpdate ( & vmgr, NULL );
        if ( rc != 0 )
            FAIL( "failed to make VDB manager" );

        rc = VDBManagerMakeSchema ( vmgr, & schema );
        if ( rc != 0 ) {
            FAIL( "failed to make empty schema" );
        }
    }
    ~GlobalFixture() {
        TEST_MESSAGE( "TEARDOWN" );
        VSchemaRelease ( schema );
        VDBManagerRelease ( vmgr );
        KDBManagerRelease ( kmgr );
    }

    KDBManager *kmgr;
    VDBManager *vmgr;
    VSchema *schema;
};

FIXTURE_TEST_SUITE( Unit_Test_Example, GlobalFixture )

TEST_CASE( VSchemaParseFile_Test ) {
    string schema_path;
    for (int i = 1; i < GET_TEST_SUITE()->argc; ++i) {
        if (string("-s") == GET_TEST_SUITE()->argv[i]) {
            if (++i < GET_TEST_SUITE()->argc) {
                schema_path = GET_TEST_SUITE()->argv[i];
                break;
            }
        }
    }
    if (schema_path.empty()) {
        FAIL( "missing schema path" );
    }
    rc_t rc = VSchemaParseFile( GET_GLOBAL_FIXTURE()->schema, schema_path.c_str() );
    if ( rc != 0 )
        FAIL( "failed to parse schema file " + schema_path );
    else
        TEST_MESSAGE( "Successfully parsed " + schema_path );
}

/* EOF */
