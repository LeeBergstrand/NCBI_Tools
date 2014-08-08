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
#include <kfs/pagefile.h>
#include <kfs/impl.h>
#include <klib/log.h>
#include <klib/out.h>
#include <klib/rc.h>
#include <sysalloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <common/test_assert.h>

static
rc_t test_readonly ( const KDirectory *dir, const KFile *in )
{
    return 0;
}

static
rc_t test_update1 ( KDirectory *dir, const KFile *in )
{
    KFile *out;
    rc_t rc = KDirectoryCreateFile ( dir, & out, true, 0666, kcmInit, "test-pagefile.out" );
    if ( rc == 0 )
    {
        KPageFile *pfile;
        rc = KPageFileMakeUpdate ( & pfile, out, 1300, false );
        if ( rc == 0 )
        {
            size_t num_writ;
            uint64_t eof, total;

            /* okay - just copy the file over */
            rc = KFileSize ( in, & eof );
            for ( total = 0; rc == 0 && total < eof; total += num_writ )
            {
                KPage *page;
                uint32_t page_id;
                rc = KPageFileAlloc ( pfile, & page, & page_id );
                if ( rc == 0 )
                {
                    void *mem;
                    size_t bytes;
                    rc = KPageAccessUpdate ( page, & mem, & bytes );
                    if ( rc == 0 )
                    {
                        num_writ = ( size_t ) ( eof - total );
                        if ( num_writ < bytes )
                            bytes = num_writ;
                        rc = KFileRead ( in, total, mem, bytes, & num_writ );
                    }

                    KPageRelease ( page );
                }
            }

            KPageFileRelease ( pfile );
        }

        KFileRelease ( out );
    }
    return rc;
}

static
rc_t test_update ( KDirectory *dir, const KFile *in )
{
    rc_t rc;

    rc = test_update1 ( dir, in );

    return rc;
}

ver_t CC KAppVersion ( void )
{
    return 0;
}

const char UsageDefaultName[] = "test-pagefile.c";
rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        "  %s [OPTIONS]\n"
        "\n"
        "Summary:\n"
        "  Test the KPageFile type.\n",
        progname);
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
    Args * args;
    rc_t rc = ArgsMakeAndHandle (&args, argc, argv, 0);
    if (rc == 0)
    {
        KDirectory *wd;

        const char *path = "test-pagefile.c";
        if ( argc == 2 )
        path = argv [ 1 ];

        rc = KDirectoryNativeDir ( & wd );
        if ( rc == 0 )
        {
            const KFile *in;
            rc = KDirectoryOpenFileRead ( wd, & in, path );
            if ( rc == 0 )
            {
                rc = test_update ( wd, in );
                if ( rc == 0 )
                    rc = test_readonly ( wd, in );

                KFileRelease ( in );
            }

            KDirectoryRelease ( wd );
        }
        ArgsWhack (args);
    }

    return rc;
}
