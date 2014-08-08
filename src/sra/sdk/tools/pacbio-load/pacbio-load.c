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
*  and reliability of the software and data", the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties", express or implied", including
*  warranties of performance", merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#include "pacbio-load.vers.h"

#include "pl-context.h"
#include "pl-tools.h"
#include "pl-zmw.h"
#include "pl-basecalls_cmn.h"
#include "pl-sequence.h"
#include "pl-consensus.h"
#include "pl-passes.h"
#include "pl-metrics.h"

#include <klib/out.h>
#include <klib/namelist.h>
#include <klib/text.h>
#include <klib/rc.h>
#include <klib/log.h>

#include <kdb/meta.h>
#include <kdb/database.h>

#include <vdb/vdb-priv.h>
#include <vdb/manager.h>
#include <vdb/schema.h>
#include <vdb/database.h>
#include <vdb/table.h>
#include <vdb/cursor.h>

#include <sra/sraschema.h>

#include <kapp/main.h>
#include <kapp/args.h>
#include <kapp/loader-meta.h>

#include <hdf5/kdf5.h>

#include <kfs/arrayfile.h>

#include <sysalloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const char UsageDefaultName[] = "pacbio-load";

rc_t CC UsageSummary ( const char * progname )
{
    OUTMSG ( ("\n"
        "Usage:\n"
        "  %s <hdf5-file> -o<target>\n"
        "\n", progname) );
    return 0;
}

static const char* schema_usage[] = { "schema-name to be used", NULL };
static const char* output_usage[] = { "target to be created", NULL };
static const char* force_usage[] = { "forces an existing target to be overwritten", NULL };
static const char* tabs_usage[] = { "load only these tabs (SCPM), dflt=all", 
                                     " S...Sequence",
                                     " C...Consensus", 
                                     " P...Passes", 
                                     " M...Metrics", NULL };
static const char* progress_usage[] = { "show load-progress", NULL };


rc_t CC Usage ( const Args * args )
{
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
    rc_t rc;

    if (args == NULL)
        rc = RC (rcApp, rcArgv, rcAccessing, rcSelf, rcNull);
    else
        rc = ArgsProgram (args, &fullpath, &progname);
    if (rc)
        progname = fullpath = UsageDefaultName;

    UsageSummary (progname);

    KOutMsg ("Options:\n");

    HelpOptionLine ( ALIAS_OUTPUT, OPTION_OUTPUT, "output", output_usage );

    HelpOptionLine ( ALIAS_SCHEMA, OPTION_SCHEMA, "schema", schema_usage );
    HelpOptionLine ( ALIAS_FORCE, OPTION_FORCE, "force", force_usage );
    HelpOptionLine ( ALIAS_TABS, OPTION_TABS, "tabs", tabs_usage );
    HelpOptionLine ( ALIAS_WITH_PROGRESS, OPTION_WITH_PROGRESS, 
                     "load-progress", progress_usage );
    XMLLogger_Usage();
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
    return PACBIO_LOAD_VERS;
}


static bool pacbio_is_schema_dflt( const char * schema )
{
    size_t asize = string_size ( schema );
    size_t bsize = string_size ( DFLT_SCHEMA );
    uint32_t max_chars = ( asize > bsize ) ? asize : bsize;
    return ( string_cmp ( schema, asize, DFLT_SCHEMA, bsize, max_chars ) == 0 );
}


static rc_t pacbio_extract_path( const KDirectory *dir, const char *schema_name,
                                 char * dst, size_t dst_len )
{
    rc_t rc = KDirectoryResolvePath ( dir, true, dst, dst_len, schema_name );
    if ( rc != 0 )
        PLOGERR( klogErr, ( klogErr, rc, "cannot resolve path to schema-file '$(name)'",
                            "name=%s", schema_name ));
    else
    {
        char *ptr = strrchr ( dst, '/' );
        if ( ptr == 0 )
        {
            rc = RC( rcExe, rcNoTarg, rcAllocating, rcParam, rcInvalid );
            PLOGERR( klogErr, ( klogErr, rc, "cannot extract the path of '$(name)'",
                                "name=%s", schema_name ));
        }
        else
            *ptr = 0;
    }
    return rc;
}


static rc_t pacbio_load_schema( ld_context *lctx, const char *schema_name )
{
    rc_t rc;

    if ( pacbio_is_schema_dflt( schema_name ) )
    {
        rc = VDBManagerMakeSRASchema ( lctx->vdb_mgr, &lctx->schema );
        if ( rc != 0 )
            LOGERR( klogErr, rc, "cannot create sra-schema" );

        if ( rc == 0 )
        {
            rc = VSchemaParseFile ( lctx->schema, schema_name );
            if ( rc != 0 )
                PLOGERR( klogErr, ( klogErr, rc, "cannot to parse schema file '$(schema)'",
                                    "schema=%s", schema_name ) );
        }
    }
    else
    {
        rc = VDBManagerMakeSchema ( lctx->vdb_mgr, &lctx->schema );
        if ( rc != 0 )
            LOGERR( klogErr, rc, "cannot create sra-schema" );
        else
        {
            char path[ 4096 ];
            rc = pacbio_extract_path( lctx->wd, schema_name, path, sizeof path );
            if ( rc == 0 )
            {
                rc = VSchemaAddIncludePath ( lctx->schema, path );
                if ( rc != 0 )
                    PLOGERR( klogErr, ( klogErr, rc, "cannot add schema-include-path '$(path)'",
                                        "path=%s", path ) );
                else
                {
                    rc = VSchemaParseFile ( lctx->schema, schema_name );
                    if ( rc != 0 )
                        PLOGERR( klogErr, ( klogErr, rc, "cannot to parse schema file '$(schema)'",
                                            "schema=%s", schema_name ) );
                }
            }
        }
    }
    return rc;
}


static rc_t pacbio_make_alias( VDatabase * vdb_db,
                               const char *existing_obj, const char *alias_to_create )
{
    KDatabase *kdb;
    rc_t rc = VDatabaseOpenKDatabaseUpdate ( vdb_db, & kdb );
    if ( rc == 0 )
    {
        rc = KDatabaseAliasTable ( kdb, existing_obj, alias_to_create );
        KDatabaseRelease ( kdb );
    }
    return rc;
}

static rc_t pacbio_load( context *ctx, ld_context *lctx, 
                         bool cache_content, bool check_src_obj )
{
    bool consensus_present = true;
    rc_t rc1 = 0;
    rc_t rc = KLoadProgressbar_Make( &lctx->xml_progress, 0 );
    if ( rc != 0 )
        LOGERR( klogErr, rc, "cannot create LoadProgressBar" );

    if ( rc == 0 )
    {
        rc = VDBManagerMakeUpdate ( &lctx->vdb_mgr, lctx->wd );
        if ( rc != 0 )
            LOGERR( klogErr, rc, "cannot create vdb-update-manager" );
    }

    if ( rc == 0 )
        rc = pacbio_load_schema( lctx, ctx->schema_name );

    if ( rc == 0 )
    {
        rc = VSchemaParseFile ( lctx->schema, ctx->schema_name );
        if ( rc != 0 )
            PLOGERR( klogErr, ( klogErr, rc, "cannot to parse schema file '$(schema)'",
                                "schema=%s", ctx->schema_name ) );
    }

    if ( rc == 0 )
    {
        rc = MakeHDF5RootDir ( lctx->wd, &lctx->hdf5_dir, false, ctx->src_path );
        if ( rc != 0 )
            PLOGERR( klogErr, ( klogErr, rc, "cannot open hdf5-source-file '$(srcfile)'",
                                "srcfile=%s", ctx->src_path ) );

    }

    if ( rc == 0 )
    {
        KCreateMode cmode = kcmMD5 | kcmParents;
        if ( ctx->force )
            cmode |= kcmInit;
        else
            cmode |= kcmCreate;
        rc = VDBManagerCreateDB( lctx->vdb_mgr, & lctx->database, lctx->schema, 
                                 PACBIO_SCHEMA_DB, cmode, ctx->dst_path );
        if ( rc != 0 )
            PLOGERR( klogErr, ( klogErr, rc, "cannot create output-database '$(dst)'",
                                "dst=%s", ctx->dst_path ) );
    }

    if ( rc == 0 && ctx_ld_sequence( ctx ) )
        rc = load_seq( lctx, cache_content, check_src_obj );

    if ( rc == 0 && ctx_ld_consensus( ctx ) )
    {
        rc1 = load_consensus( lctx, cache_content, check_src_obj );
        if ( rc1 != 0 )
        {
            LOGMSG( klogWarn, "the consensus-group is missing" );
            VDatabaseDropTable ( lctx->database, "CONSENSUS" );
            consensus_present = false;
        }
    }

    if ( rc == 0 )
    {
        if ( rc1 == 0 )
        {
            /* CONSENSUS exists, create a alias named SEQUENCE to CONSENSUS */
            /* rc = pacbio_make_alias( lctx->database, "CONSENSUS", "SEQUENCE" ); */
        }
        else
        {
            /* CONSENSUS does not exist, create a alias named SEQUENCE to PULSE */
            /* rc = pacbio_make_alias( lctx->database, "PULSE", "SEQUENCE" ); */
        }
    }

    if ( rc == 0 && ctx_ld_passes( ctx )&& consensus_present )
    {
        rc1 = load_passes( lctx, cache_content, check_src_obj );
        if ( rc1 != 0 )
            LOGMSG( klogWarn, "the passes-table is missing" );
    }

    if ( rc == 0 && ctx_ld_metrics( ctx ) )
    {
        rc1 = load_metrics( lctx, cache_content, check_src_obj );
        if ( rc1 != 0 )
            LOGMSG( klogWarn, "the metrics-table is missing" );
    }

    return rc;

}


static rc_t pacbio_check_sourcefile( context *ctx, ld_context *lctx )
{
    rc_t rc = 0;
    uint32_t src_path_type = KDirectoryPathType ( lctx->wd, ctx->src_path );
    if ( ( src_path_type & kptFile ) == 0 )
    {
        rc = RC ( rcExe, rcFile, rcValidating, rcItem, rcNotFound );
        LOGERR( klogErr, rc, "source-file not found" );
    }
    else
    {
        if ( ( src_path_type & kptAlias ) != 0 )
        {
            char resolved[ 2048 ];
            rc = KDirectoryResolveAlias ( lctx->wd, true, resolved,
                                          2084, ctx->src_path );
            if ( rc != 0 )
            {
                LOGERR( klogErr, rc, "cannot resolve srcfile-link" );
            }
            else
            {
                free( ctx->src_path );
                ctx->src_path = string_dup_measure ( resolved, NULL );
            }
        }
    }
    return rc;
}


OptDef MyOptions[] =
{
    { OPTION_SCHEMA, ALIAS_SCHEMA, NULL, schema_usage, 5, true, false },
    { OPTION_FORCE, ALIAS_FORCE, NULL, force_usage, 1, false, false },
    { OPTION_WITH_PROGRESS, ALIAS_WITH_PROGRESS, NULL, progress_usage, 1, false, false },
    { OPTION_TABS, ALIAS_TABS, NULL, tabs_usage, 1, true, false },
    { OPTION_OUTPUT, ALIAS_OUTPUT, NULL, output_usage, 1, true, true }
};


static rc_t pacbio_meta_entry( ld_context *lctx, const char * toolname )
{
    KMetadata* meta = NULL;
    
    rc_t rc = VDatabaseOpenMetadataUpdate( lctx->database, &meta );
    if ( rc != 0 )
    {
        LOGERR( klogErr, rc, "Cannot open database-metadata" );
    }
    else
    {
        KMDataNode *node = NULL;

        rc = KMetadataOpenNodeUpdate( meta, &node, "/" );
        if ( rc != 0 )
        {
            LOGERR( klogErr, rc, "Cannot open database-metadata-root" );
        }
        else
        {
            rc = KLoaderMeta_Write( node, toolname, __DATE__, "PacBio HDF5", PACBIO_LOAD_VERS );
            if ( rc != 0 )
            {
                LOGERR( klogErr, rc, "Cannot write pacbio metadata node" );
            }
            KMDataNodeRelease( node );
        }
        KMetadataRelease( meta );
    }

    return rc;
}

rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;

    rc_t rc = ArgsMakeAndHandle ( &args, argc, argv, 2,
                                  MyOptions, sizeof MyOptions / sizeof ( OptDef ),
                                  XMLLogger_Args, XMLLogger_ArgsQty  );

    KLogHandlerSetStdErr();
    if ( rc != 0 )
    {
        LOGERR( klogErr, rc, "error creating internal structure" );
    }
    else
    {
        ld_context lctx;

        lctx_init( &lctx );
        rc = KDirectoryNativeDir ( &lctx.wd );
        if ( rc != 0 )
        {
            LOGERR( klogErr, rc, "error creating internal structure" );
        }
        else
        {
            rc = XMLLogger_Make( &lctx.xml_logger, lctx.wd, args );
            if ( rc != 0 )
            {
                LOGERR( klogErr, rc, "error creating internal structure" );
            }
            else
            {
                context ctx;
                rc = ctx_init( args, &ctx );
                if ( rc == 0 )
                {
                    rc = pacbio_check_sourcefile( &ctx, &lctx );
                    if ( rc == 0 )
                    {
                        lctx.with_progress = ctx.with_progress;
                        ctx_show( &ctx );
                        lctx.dst_path = ctx.dst_path;

                        rc = pacbio_load( &ctx, &lctx, false, false );
                        if ( rc == 0 )
                        {
                            rc = pacbio_meta_entry( &lctx, argv[ 0 ] );
                        }
                    }
                    ctx_free( &ctx );
                }
            }
        }
        lctx_free( &lctx );
        ArgsWhack ( args );
    }

    return rc;
}
