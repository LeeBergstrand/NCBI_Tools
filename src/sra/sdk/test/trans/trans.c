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

#include "trans.vers.h"

#include <kapp/main.h>
#include <kapp/args.h>

#include <klib/text.h>
#include <klib/out.h>
#include <klib/rc.h>
#include <klib/log.h>

#include <align/iterator.h>

#include <os-native.h>
#include <sysalloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "support.h"
#include "options.h"

#include "t_al_iter.h"
#include "t_reflist.h"
#include "t_pl_iter.h"
#include "t_plset_iter.h"
#include "t_walk_1_alignment.h"
#include "tokenizer.h"

#include <common/test_assert.h>

static const char * func_usage[] = { "FUNCTION:",
                                     "1... test inner iterator",
                                     "2... test reference list obj",
                                     "3... test placement-iter",
                                     "4... test placement-set-iter",
                                     "5... test 1 alignment",
                                      NULL };
static const char * name_usage[] = { "file-name", NULL };
static const char * ref_usage[] = { "ref-name", NULL };
static const char * reflen_usage[] = { "ref-len", NULL };

static const char * hmm_usage[]  = { "HAS_MISMATCH   (0/1)", NULL };
static const char * hro_usage[]  = { "HAS_REF_OFFSET (0/1)", NULL };
static const char * read_usage[] = { "READ (ACGTN)", NULL };
static const char * ro_usage[]   = { "REF_OFFSET ( number )", NULL };
static const char * rp_usage[]   = { "REF_POS ( number )", NULL };
static const char * outf_usage[] = { "file to write output to", NULL };
static const char * ndbg_usage[] = { "no extended info generated", NULL };
static const char * ske_usage[]  = { "skip empty refpositions", NULL };

OptDef MyOptions[] =
{
    /*name,          alias,        hfkt, usage-help,   maxcount, needs value, required */
    { OPTION_FUNC,   ALIAS_FUNC,   NULL, func_usage,   1,        true,        true },
    { OPTION_NAME,   ALIAS_NAME,   NULL, name_usage,   1,        true,        false },
    { OPTION_REF,    ALIAS_REF,    NULL, ref_usage,    1,        true,        false },
    { OPTION_REFLEN, ALIAS_REFLEN, NULL, reflen_usage, 0,        true,        false },
    { OPTION_HMM,    ALIAS_HMM,    NULL, hmm_usage,    1,        true,        true },
    { OPTION_HRO,    ALIAS_HRO,    NULL, hro_usage,    1,        true,        true },
    { OPTION_READ,   ALIAS_READ,   NULL, read_usage,   1,        true,        true },
    { OPTION_RO,     ALIAS_RO,     NULL, ro_usage,     0,        true,        false },
    { OPTION_RP,     ALIAS_RP,     NULL, rp_usage,     0,        true,        true },
    { OPTION_OUTF,   ALIAS_OUTF,   NULL, outf_usage,   1,        true,        false },
    { OPTION_NDBG,   ALIAS_NDBG,   NULL, ndbg_usage,   1,        true,        false },
    { OPTION_SKE,    ALIAS_SKE,    NULL, ske_usage,    1,        true,        false }
};


const char UsageDefaultName[] = "trans";


rc_t CC UsageSummary ( const char * progname )
{
    return KOutMsg ("\n"
                    "Usage:\n"
                    "  %s <path> [options]\n"
                    "\n", progname);
}


rc_t CC Usage ( const Args * args )
{
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
    rc_t rc;

    if ( args == NULL )
        rc = RC ( rcApp, rcArgv, rcAccessing, rcSelf, rcNull );
    else
        rc = ArgsProgram ( args, &fullpath, &progname );

    if ( rc )
        progname = fullpath = UsageDefaultName;

    UsageSummary ( progname );
    KOutMsg ( "Options:\n" );
    HelpOptionLine ( ALIAS_FUNC, OPTION_FUNC, "FUNC", func_usage );
    HelpOptionLine ( ALIAS_NAME, OPTION_NAME, "NAME", name_usage );
    HelpOptionLine ( ALIAS_REF, OPTION_REF, "REF", ref_usage );
    HelpOptionLine ( ALIAS_HMM, OPTION_HMM, "HMM", hmm_usage );
    HelpOptionLine ( ALIAS_HRO, OPTION_HRO, "HRO", hro_usage );
    HelpOptionLine ( ALIAS_READ, OPTION_READ, "READ", read_usage );
    HelpOptionLine ( ALIAS_RO, OPTION_RO, "RO", ro_usage );
    HelpOptionLine ( ALIAS_RP, OPTION_RP, "RP", rp_usage );
    HelpOptionLine ( ALIAS_OUTF, OPTION_OUTF, "output-file", outf_usage );
    HelpOptionLine ( ALIAS_NDBG, OPTION_NDBG, "nodebug", ndbg_usage );
    HelpOptionLine ( ALIAS_SKE, OPTION_SKE, "skip-empty", ske_usage );
    HelpOptionsStandard ();
    HelpVersion ( fullpath, KAppVersion() );
    return rc;
}


/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
ver_t CC KAppVersion ( void )
{
    return TRANS_VERS;
}


/* =========================================================================================== */


rc_t CC KMain( int argc, char *argv [] )
{
    rc_t rc = KOutHandlerSet( write_to_FILE, stdout );
    if ( rc != 0 )
        LOGERR( klogInt, rc, "KOutHandlerSet() failed" );
    else
    {
        Args * args;

        KLogHandlerSetStdErr();
        rc = ArgsMakeAndHandle( &args, argc, argv, 1,
            MyOptions, sizeof MyOptions / sizeof MyOptions [ 0 ] );
        if ( rc == 0 )
        {
            uint32_t func;
            rc = get_uint32_option( args, OPTION_FUNC, &func, 1 );
            if ( rc == 0 )
            {
                const char * output_file;
                if ( rc == 0 )
                    rc = get_str_option( args, OPTION_OUTF, &output_file );
                if ( rc == 0 )
                {
                    stdout_redir * redir = NULL;
                    if ( output_file != NULL )
                        rc = make_stdout_redir( &redir, output_file, 32 * 1024  );

                    switch( func )
                    {
                    case 1 : rc = test_al_iter( args ); break;
                    case 2 : rc = test_ref_list( args ); break;
                    case 3 : rc = test_placement_iter( args ); break;
                    case 4 : rc = test_placement_set_iter( args ); break;
                    case 5 : rc = walk_1_alignment( args ); break;
                    case 6 : rc = test_tokenizer( args ); break;
                    }

                   release_stdout_redirection( redir );
                }
            }
            ArgsWhack( args );
        }
    }
    return rc;
}
