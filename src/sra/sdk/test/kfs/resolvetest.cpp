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
* Unit tests for KDirectoryResolve
*/
#include <cstring>
#include <ktst/unit_test.hpp>
#include <kfs/directory.h>
#include <kfs/impl.h>
#include <klib/out.h>
#include <kapp/args.h>

#include <common/test_assert.h>

using namespace std;

char * arg1 = NULL;

TEST_SUITE( ResolveSuite );

TEST_CASE( ResolveTest )
{
    KOutMsg ( "ResolveTest testing KDirectoryResolve\n" );

    if ( arg1 == NULL )
    {
        KOutMsg ( "no directory given to resolve\n" );
    }
    else
    {
        KDirectory *dir;
        char resolved[ 4 * 1024 ];

        REQUIRE_RC( KDirectoryNativeDir ( &dir ) );

        REQUIRE_RC( KDirectoryResolvePath ( dir, true, resolved, sizeof resolved, "%s", arg1 ) );
        KOutMsg ( "absolute: %s\n", resolved );

        REQUIRE_RC( KDirectoryResolvePath ( dir, false, resolved, sizeof resolved, "%s", arg1 ) );
        KOutMsg ( "relative: %s\n", resolved );

        REQUIRE_RC( KDirectoryRelease( dir ) );
    }
}


//////////////////////////////////////////// Main

extern "C"
{

ver_t CC KAppVersion ( void )
{
    return 0x1000000;
}

rc_t CC UsageSummary (const char * prog_name)
{
    return 0;
}

rc_t CC Usage ( const Args * args)
{
    return 0;
}

const char UsageDefaultName[] = "test-resolve";

rc_t CC KMain ( int argc, char *argv [] )
{
    if ( argc > 1 ) arg1 = argv[ 1 ];
    rc_t rc = ResolveSuite( argc, argv );
    return rc;
}

}
