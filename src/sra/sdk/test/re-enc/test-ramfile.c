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
#include <kfs/ramfile.h>
#include <kfs/impl.h>
#include <klib/status.h>
#include <klib/out.h>
#include <klib/rc.h>
#include <sysalloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <common/test_assert.h>




rc_t test_1 ()
{
    rc_t rc = 0;
    KStsLevel ll = KStsLevelGet();
    const KFile * rfile;
    KFile * wfile;
    KFile * ufile;
    size_t ix, jx, kx;
    uint64_t pos;
    char input [64 * 1024];
    char output [64 * 1024];
    char buffer [4096 + 1];

    /* lazy cleanup valgrind would find unreleased memory on error */

    do
    {
        /* ---------- */
        for (ix = 0; ix < sizeof input; ++ix)
            input[ix] = (char)(0x30 + (ix & 0x3F));

        rc = KRamFileMakeWrite (&wfile, buffer, sizeof buffer - 1);
        if (rc)
        {
            break;
        }
#if 1
        if (ll > 0)
            OUTMSG (("Starting Test 1: KRamFile simple write at position with overwrites\n"));

        for (kx = 0; kx < 1000 * 1000; kx += 1000 )
        {
            if (ll > 1)
                OUTMSG (("1 Position %u\n",jx));

            for (jx = 1; jx < sizeof (buffer) - 1; ++jx)
            {
                size_t max;

                if (ll > 1)
                    OUTMSG (("1 Incrementing by %u\n",jx));

                for (ix = 0; ix <= sizeof (buffer); ix += jx)
                {
                    size_t num_writ;

                    if (ll > 2)
                        OUTMSG (("1 Test %u %u %u\n",kx,jx,ix));

                    max = ix;
                    if (max > sizeof (buffer) - 1)
                        max = sizeof (buffer) - 1;

                    memset (buffer, 0, sizeof buffer);

                    rc = KFileWrite (wfile, kx, input, ix, &num_writ);
                
                    if (rc)
                    {
                        OUTMSG (("%s: failed write pos %u increment %u length %u\n", __func__, kx, jx, ix));
                        break;
                    }

                    if (memcmp (buffer, input, max) != 0)
                        OUTMSG (("Error in simple write test at length %d\n", ix));
                }
                if (rc)
                    break;
            }
            if (rc)
                break;
        }

#endif
#if 1
        if (ll > 0)
            OUTMSG (("Starting Test 2: KRamFile append write\n"));

        for (kx = 0; kx < 1000 * 1000; kx += 1000 )
        {
            if (ll > 1)
                OUTMSG (("2 Position %u\n",jx));

            for (jx = 1; jx < sizeof (buffer) - 1; ++jx) 
            {
                memset (buffer, 0, sizeof buffer);

                if (ll > 1)
                    OUTMSG (("2 Incrementing by %u\n",jx));

                for (ix = 0; ix < sizeof (buffer) - 1; )
                {
                    size_t num_writ;

                    if (ll > 2)
                        OUTMSG (("Test %u %u %u\n",kx,jx,ix));

                    /* appenda new write onto the existing write */
                    rc = KFileWrite (wfile, kx + ix, input + ix, jx,  &num_writ);
                    if (rc)
                    {
                        OUTMSG (("%s: failed write appeand of %u at %u\n", __func__, jx, ix));
                        break;
                    }
                    ix += num_writ;
                    if (memcmp (buffer, input, ix) != 0)
                    {
                        OUTMSG (("Error in append write test at length %d\n", ix));
                        OUTMSG (("%s\n", buffer));
                    }
                }
                if (rc)
                    break;
            }
            if (rc)
                break;
        }

        if (rc)
            break;
#endif



    } while (0);

    return rc;
}


rc_t run(bool dontstop)
{
    rc_t rc = 0;

    do
    {
        rc_t rc1;

        rc1 = test_1 ();
        if (rc1)
        {
            OUTMSG (("Failed Test 1\n"));
            if (!dontstop)
                break;
            if (rc == 0)
                rc = rc1;
        }


    } while (0);
    return rc;
}

#define OPTION_DONTSTOP "dont-stop"
#define ALIAS_DONTSTOP  "i"

const char * dontstop_usage[] = { "Keep running after errors", NULL };

OptDef Options[] = 
{
    { OPTION_DONTSTOP, ALIAS_DONTSTOP, NULL, dontstop_usage, 0, false, false }
};



ver_t CC KAppVersion ( void )
{
    return 0;
}

const char UsageDefaultName[] = "test-ramfile.c";
rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        "  %s [OPTIONS]\n"
        "\n"
        "Summary:\n"
        "  Test the KRamfile type.\n",
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
        uint32_t count;

        rc = ArgsParamCount (args, &count);
        if (rc)
            ;
        else
        {
            bool dontstop;

            dontstop = (count != 0);

            rc = run(dontstop);
        }
    }

    return rc;
}
