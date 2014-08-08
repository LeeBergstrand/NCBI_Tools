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

#include <kapp/main.h>
#include <kapp/args.h>
#include <kfs/directory.h>
#include <kfs/file.h>
#include <kfs/impl.h>
#include <klib/log.h>
#include <klib/out.h>
#include <klib/rc.h>
#include <sysalloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <common/test_assert.h>

/* test writable KDirectory
 */
static
rc_t test_kdir_create_file ( KDirectory *dir )
{
    int i;
    rc_t rc;
    KFile *f;
    uint32_t type;
    const char *ppath = "parent-dir-";

    for ( i = 1; ; ++ i )
    {
        type = KDirectoryPathType ( dir, "%s%d", ppath, i ) & ~ kptAlias;
        switch ( type )
        {
        case kptFile:
        case kptDir:
        case kptCharDev:
        case kptBlockDev:
        case kptFIFO:
        case kptZombieFile:
            continue;
        case kptNotFound:
            break;
        default:
            return RC ( rcExe, rcDirectory, rcAccessing, rcType, rcUnrecognized );
        }

        break;
    }

    rc = KDirectoryCreateFile ( dir, & f, true, 0777, kcmInit | kcmParents,
        "%s%d/missing-dir/create-file-test", ppath, i );
    if ( rc == 0 )
        KFileRelease ( f );
    else
    {
      LOGERR ( klogInt, rc, "KDirectoryCreateFile failed" );
    }

    KDirectoryRemove ( dir, true, "%s%d", ppath, i );

    return rc;
}

static
rc_t test_writable_kdir ( KDirectory *dir )
{
    /* test each API */
    rc_t rc = 0;

    /* ... */

    if ( rc == 0 )
        rc = test_kdir_create_file ( dir );

    /* ... */

    return rc;
}

/* test readable KDirectory */
static
rc_t test_readable_kdir ( const KDirectory *dir )
{
    return 0;
}

/* test KDirectory
 *  this should be put into a common source
 */
static
rc_t test_kdir ( const KDirectory *dir )
{
    /* examine implementation interface */
    rc_t rc = 0;

    /* exercise each API */
    if ( rc == 0 )
        rc = test_readable_kdir ( dir );
    if ( rc == 0 && ! dir -> read_only )
        rc = test_writable_kdir ( ( KDirectory* ) dir );

    return rc;
}

static
rc_t test_sysfile ( void )
{
    return 0;
}

static
rc_t test_sysdir ( void )
{
    /* always start with working directory */
    KDirectory *wd;
    rc_t rc = KDirectoryNativeDir ( & wd );
    if ( rc == 0 )
    {
        /* run all tests on directory */
        rc = test_kdir ( wd );

        KDirectoryRelease ( wd );
    }

    return rc;
}

ver_t CC KAppVersion ( void )
{
    return 0;
}

const char UsageDefaultName[] = "args-test";
rc_t CC UsageSummary(const char * name)
{
    return KOutMsg (
        "Usage:\n"
        "  %s [Options]\n"
        "\n"
        "Summary:\n"
        "  test KSysDir and KSysFile\n\n",
        name);
}
rc_t CC Usage (const Args * args)
{
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
    rc_t rc;

    if (args == NULL)
        rc = RC (rcApp, rcArgv, rcAccessing, rcSelf, rcNull);
    else
        rc = ArgsProgram (args, &fullpath, &progname);

    UsageSummary (progname);

    KOutMsg ("Options:\n");

    HelpOptionsStandard();

    HelpVersion (fullpath, KAppVersion());

    return rc;
}

rc_t CC KMain ( int argc, char *argv [] )
{
    rc_t rc = 0;
    Args * args;

    rc = ArgsMakeAndHandle (&args, argc, argv, 0);
    if (rc == 0)
    {
        rc = test_sysdir ();
        if ( rc == 0 )
            rc = test_sysfile ();

        ArgsWhack (args);
    }
    return rc;
}
