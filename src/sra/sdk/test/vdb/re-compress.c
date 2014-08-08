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

#include <vdb/manager.h>
#include <vdb/schema.h>
#include <vdb/database.h>
#include <vdb/table.h>
#include <vdb/cursor.h>
#include <kdb/meta.h>

#include <kapp/main.h>
#include <kapp/args.h>

#include <klib/status.h>
#include <klib/out.h>
#include <klib/log.h>

#include <klib/text.h>
#include <klib/rc.h>

#include <string.h>
#include <assert.h>
#include <ctype.h>

#define BLOBBING 0

static
rc_t copy_row ( const VCursor *scurs, uint32_t sidx, VCursor *dcurs, uint32_t didx )
{
    const void *base;
    uint32_t elem_bits, boff, row_len;
    rc_t rc = VCursorCellData ( scurs, sidx, & elem_bits, & base, & boff, & row_len );
    if ( rc == 0 )
    {
        rc = VCursorWrite ( dcurs, didx, elem_bits, base, boff, row_len );
        if ( rc == 0 )
            rc = VCursorCommitRow ( dcurs );
    }
    return rc;
}

static
rc_t copy ( const VCursor *scurs, uint32_t sidx, VCursor *dcurs, uint32_t didx, bool preserve_blobbing )
{
    rc_t rc;

    uint32_t dots;

    for ( dots = 0, rc = 0; rc == 0 && ! Quitting (); )
    {
        rc = VCursorOpenRow ( scurs );
        if ( rc == 0 )
        {
            rc = VCursorOpenRow ( dcurs );
            if ( rc == 0 )
            {
                int64_t row_id;
                rc = VCursorRowId ( scurs, & row_id );
                if ( rc == 0 && ( row_id & ( ( 1 << 18 ) - 1 ) ) == 1 )
                {
                    if ( dots == 0 )
                        KOutMsg ( "%'16ld .", row_id );
                    else
                        KOutMsg ( "." );
                    if ( ++ dots == 60 )
                    {
                        KOutMsg ( "\n" );
                        dots = 0;
                    }
                }
                rc = copy_row ( scurs, didx, dcurs, didx );
                VCursorCloseRow ( dcurs );

                if ( rc == 0 && preserve_blobbing )
                {
                    /* at this point, commit cursor to keep in sync with source */
                }
            }
            VCursorCloseRow ( scurs );
        }
    }

    KOutMsg ( "\n" );

    if ( GetRCState ( rc ) == rcNotFound )
        rc = 0;

    return rc;
}

static
rc_t run ( VDBManager *mgr, const VCursor *scurs, uint32_t sidx,
    const VSchema *dst_schema, const char *dstpath, const char *colname,
    bool preserve_blobbing )
{
    VTable *dtbl;
    rc_t rc = VDBManagerCreateTable ( mgr, & dtbl, dst_schema, "re_compress_tmp_tbl", kcmInit, dstpath );
    if ( rc == 0 )
    {
        VCursor *dcurs;
        rc = VTableCreateCursorWrite ( dtbl, & dcurs, kcmInsert );
        if ( rc == 0 )
        {
            uint32_t didx;
            rc = VCursorAddColumn ( dcurs, & didx, colname );
            if ( rc == 0 )
            {
                rc = VCursorSetRowId ( scurs, 1 );
                if ( rc == 0 )
                    VCursorSetRowId ( dcurs, 1 );
                if ( rc == 0 )
                    rc = VCursorOpen ( scurs );
                if ( rc == 0 )
                    rc = VCursorOpen ( dcurs );
                if ( rc == 0 )
                    rc = copy ( scurs, sidx, dcurs, didx, preserve_blobbing );
            }

            VCursorRelease ( dcurs );
        }
     
        VTableRelease ( dtbl );
    }
    return rc;
}

static
rc_t update_dest_schema ( const VSchema *src_schema, const VCursor *scurs, uint32_t idx,
    VSchema *dst_schema, const char *colname, const char *encoding )
{
    VTypedecl td;
    rc_t rc = VCursorDatatype ( scurs, idx, & td, NULL );
    if ( rc == 0 )
    {
        char buffer [ 256 ];
        rc = VTypedeclToText ( & td, src_schema, buffer, sizeof buffer );
        if ( rc == 0 )
        {
            rc = VSchemaResolveTypedecl ( dst_schema, & td, buffer );
            if ( rc == 0 )
            {
                VSchemaRuntimeTable *rttbl;
                rc = VSchemaMakeRuntimeTable ( dst_schema, & rttbl, "re_compress_tmp_tbl", NULL );
                if ( rc == 0 )
                {
                    rc = VSchemaRuntimeTableAddColumn ( rttbl, & td, encoding, colname );
                    if ( rc == 0 )
                        rc = VSchemaRuntimeTableCommit ( rttbl );

                    VSchemaRuntimeTableClose ( rttbl );
                }
            }
        }
    }

    return rc;
}

static
rc_t re_compress ( VDBManager *mgr, const VSchema *src_schema, VSchema *dst_schema,
    const char *srcpath, const char *dstpath, const char *tblname, const char *colname,
    const char *encoding, bool preserve_blobbing )
{
    rc_t rc = KOutMsg ( "mgr = 0x%p, src_schema = 0x%p, dst_schema = 0x%p, srcpath = '%s', dstpath = '%s', "
                        "tblname = '%s', colname = '%s', encoding = '%s', blobbing = %s\n",
                        mgr, src_schema, dst_schema, srcpath, dstpath, tblname, colname, encoding,
                        preserve_blobbing ? "true" : "false" );

    /* open source table or database-then-table   */
    const VTable *stbl;
    rc = VDBManagerOpenTableRead ( mgr, & stbl, src_schema, "%s", srcpath );
    if ( rc != 0 && tblname != NULL && tblname [ 0 ] != 0 )
    {
        const VDatabase *db;
        rc_t rc2 = VDBManagerOpenDBRead ( mgr, & db, src_schema, "%s", srcpath );
        if ( rc2 == 0 )
        {
            rc = VDatabaseOpenTableRead ( db, & stbl, "%s", tblname );
            VDatabaseRelease ( db );
        }
    }
    if ( rc == 0 )
    {
        /* create read cursor and add column          */
        const VCursor *scurs;
        rc = VTableCreateCursorRead ( stbl, & scurs );
        if ( rc == 0 )
        {
            uint32_t idx;
            rc = VCursorAddColumn ( scurs, & idx, "%s", colname );
            if ( rc == 0 )
            {
                const VSchema *tbl_schema;

                /* strip column spec to name */
                const char *p = strrchr ( colname, ')' );
                if ( p != NULL )
                    colname = p + 1;
                while ( isspace ( * colname ) )
                    ++ colname;

                /* update destination schema */
                rc = VTableOpenSchema ( stbl, & tbl_schema );
                if ( rc == 0 )
                {
                    rc = update_dest_schema ( tbl_schema, scurs, idx, dst_schema, colname, encoding );
                    VSchemaRelease ( tbl_schema );
                }

                /* perform copy and close cursor, table */
                rc = run ( mgr, scurs, idx, dst_schema, dstpath, colname, preserve_blobbing );
            }

            VCursorRelease ( scurs );
        }

        VTableRelease ( stbl );
    }
    return rc;
}

ver_t CC KAppVersion ( void )
{
    return 0x00090000;
}

static
const char *schema_usage [] =
{
    "path to schema source\n",
    NULL
};

static
const char *encoding_usage [] =
{
    "name of output encoding. "
    "this is a 'physical' column encoding in schema.\n",
    NULL
};

#if BLOBBING
static
const char *blobbing_usage [] =
{
    "preserve source blobbing boundaries\n",
    NULL
};
#endif

const char UsageDefaultName[] = "re-compress";

rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        "  %s [options] <src>:<column> <dst>\n"
        "  %s [options] <srcdb>:<table>:<column> <dst>\n"
        "\n"
        "Summary:\n"
        "  Copy a single VDB source column to a destination table,\n"
        "  usually with a different compression technique or blobbing.\n"
        "\n"
        "Options:\n"
        , progname
        , progname
        );
}
rc_t CC Usage ( const Args *args )
{
    ver_t version = KAppVersion ();
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
    rc_t rc;

    if (args == NULL)
        rc = RC (rcApp, rcArgv, rcAccessing, rcSelf, rcNull);
    else
        rc = ArgsProgram (args, &fullpath, &progname);

    UsageSummary (progname);

    KOutMsg ("Options:\n");

    HelpOptionLine ( "S", "schema", "path", schema_usage );
    HelpOptionLine ( "e", "encoding", "encoding", encoding_usage );
#if BLOBBING
    HelpOptionLine ( "b", "blobbing", NULL, blobbing_usage );
#endif

    HelpOptionsStandard ();

    KOutMsg ( "\n"
              "Examples:\n"
              "  # copy column using current encoding, and default blobbing\n"
              "  %s SRR012345:QUALITY test1\n"
              "\n"
              "  # copy column with a new encoding\n"
              "  %s test1:QUALITY test2 -e 'vdb:super:duper:encoding'\n"
              "\n"
              "  # as above but assume new encoding is in external schema files\n"
              "  %s test1:QUALITY test3 -S 'vdb/super-duper.vschema' \\\n"
              "     -e 'vdb:super:duper:encoding'\n"
#if BLOBBING
              "\n"
              "  # as above but preserving original blobbing boundaries\n"
              "  %s SRR012345:QUALITY test4 -b -e 'vdb:super:duper:encoding'\n"
              , progname
#endif
              , progname, progname, progname
        );

    HelpVersion ( fullpath, version );

    return rc;
}

OptDef Options [] =
{
      { "schema", "S", NULL, schema_usage, 0, true, false }
    , { "encoding", "e", NULL, encoding_usage, 1, true, false }
#if BLOBBING
    , { "blobbing", "b", NULL, blobbing_usage, 0, false, false }
#endif
};

rc_t CC KMain ( int argc, char *argv [] )
{
    Args *args;
    rc_t rc = ArgsMakeAndHandle ( & args, argc, argv, 1,
        Options, sizeof Options / sizeof Options [ 0 ] );
    if ( rc != 0 )
        LogErr ( klogErr, rc, "command line needs some work" );
    else
    {
        if ( argc < 2 )
            rc = Usage ( args );
        else
        {
            uint32_t num_params;
            rc = ArgsParamCount ( args, & num_params );
            if ( rc != 0 )
                LogErr ( klogInt, rc, "failed to obtain parameter count" );
            else if ( num_params < 2 )
            {
                rc = RC ( rcExe, rcArgv, rcParsing, rcParam, rcInsufficient );
                LogErr ( klogInt, rc, "missing parameters" );
            }
            else if ( num_params > 4 )
            {
                rc = RC ( rcExe, rcArgv, rcParsing, rcParam, rcExcessive );
                LogErr ( klogInt, rc, "more parameters than I know what to do with" );
            }
            else
            {
                const char *srcpath, *dstpath, *tblname, *colname;
                rc = ArgsParamValue ( args, 0, & srcpath );
                if ( rc == 0 )
                    rc = ArgsParamValue ( args, 1, & dstpath );
                if ( rc == 0 )
                {
                    if ( num_params == 4 )
                    {
                        tblname = dstpath;
                        rc = ArgsParamValue ( args, 2, & colname );
                        if ( rc == 0 )
                            rc = ArgsParamValue ( args, 3, & dstpath );
                    }
                    else if ( num_params == 3 )
                    {
                        colname = strchr ( dstpath, ':' );
                        if ( colname == NULL )
                        {
                            tblname = NULL;
                            colname = dstpath;
                        }
                        else
                        {
                            tblname = dstpath;
                            * ( char* ) colname ++ = 0;
                        }
                        rc = ArgsParamValue ( args, 2, & dstpath );
                    }
                    else
                    {
                        tblname = strchr ( srcpath, ':' );
                        if ( tblname != NULL )
                        {
                            * ( char* ) tblname ++ = 0;
                            colname = strchr ( tblname, ':' );
                            if ( colname == NULL )
                            {
                                colname = tblname;
                                tblname = NULL;
                            }
                            else
                            {
                                * ( char* ) colname ++ = 0;
                            }
                        }
                        else
                        {
                            rc = RC ( rcExe, rcArgv, rcParsing, rcParam, rcIncorrect );
                            pLogMsg ( klogErr, "parameter '$(param)' is malformed - expected <src>:<column>", "param=%s", srcpath );
                        }
                    }
                }

                if ( rc != 0 )
                    LogErr ( klogInt, rc, "failed to extract parameters" );
                else
                {
                    VDBManager *mgr;
                    rc = VDBManagerMakeUpdate ( & mgr, NULL );
                    if ( rc != 0 )
                        LogErr ( klogInt, rc, "failed to open vdb manager for update" );
                    else
                    {
                        uint32_t schema_paths;
                        rc = ArgsOptionCount ( args, "schema", & schema_paths );
                        if ( rc != 0 )
                            LogErr ( klogInt, rc, "failed to obtain 'schema' option count" );
                        else
                        {
                            VSchema *dst_schema = NULL;
                            rc = VDBManagerMakeSchema ( mgr, & dst_schema );
                            if ( rc != 0 )
                                LogErr ( klogInt, rc, "failed to create empty schema object" );
                            else if ( schema_paths != 0 )
                            {
                                uint32_t i;
                                for ( i = 0; i < schema_paths; ++ i )
                                {
                                    const char *schema_path;
                                    rc = ArgsOptionValue ( args, "schema", i, & schema_path );
                                    if ( rc != 0 )
                                    {
                                        LogErr ( klogInt, rc, "failed to obtain schema path" );
                                        break;
                                    }
                                    rc = VSchemaParseFile ( dst_schema, "%s", schema_path );
                                    if ( rc != 0 )
                                    {
                                        pLogErr ( klogErr, rc, "failed to parse schema path '$(path)'", "path=%s", schema_path );
                                        break;
                                    }
                                }
                            }

                            if ( rc == 0 )
                            {
                                uint32_t count;
                                rc = ArgsOptionCount ( args, "encoding", & count );
                                if ( rc != 0 )
                                    LogErr ( klogInt, rc, "failed to obtain encoding option count" );
                                else
                                {
                                    const char *encoding = NULL;
                                    if ( count != 0 )
                                    {
                                        rc = ArgsOptionValue ( args, "encoding", 0, & encoding );
                                        if ( rc != 0 )
                                            LogErr ( klogInt, rc, "failed to retrieve encoding" );
                                    }
                                    if ( rc == 0 )
                                    {
#if ! BLOBBING
                                        count = 0;
#else
                                        rc = ArgsOptionCount ( args, "blobbing", & count );
                                        if ( rc != 0 )
                                            LogErr ( klogInt, rc, "failed to obtain blobbing option count" );
                                        else
#endif
                                        {
                                            bool preserve_blobbing = count != 0;

                                            /* FINALLY! run the tool */
                                            rc = re_compress ( mgr, NULL, dst_schema, srcpath, dstpath, tblname, colname, encoding, preserve_blobbing );
                                        }
                                    }
                                }
                            }

                            VSchemaRelease ( dst_schema );
                        }

                        VDBManagerRelease ( mgr );
                    }
                }
            }
        }

        ArgsWhack ( args );
    }

    return rc;
}
