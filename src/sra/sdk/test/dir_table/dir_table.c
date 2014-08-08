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

#include "dir_table.vers.h"

#include <klib/out.h>
#include <klib/namelist.h>
#include <klib/text.h>
#include <klib/rc.h>
#include <klib/writer.h>
#include <klib/vector.h>
#include <klib/printf.h>
#include <kapp/main.h>
#include <kfs/directory.h>
#include <kfs/file.h>
#include <sysalloc.h>
#include <vdb/manager.h>
#include <vdb/schema.h>
#include <sra/sraschema.h>
#include <vdb/table.h>
#include <vdb/cursor.h>
#include <kfg/config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <os-native.h> /* snprintf on Windows */
 
const char UsageDefaultName[] = "dir_table";
#define def_name UsageDefaultName

rc_t CC UsageSummary ( const char * progname )
{
    OUTMSG ( ("\n"
        "Usage:\n"
        "  %s <path>\n"
        "\n", progname) );
    return 0;
}


rc_t CC Usage ( const Args * args )
{
    const char * progname;
    const char * fullpath;
    rc_t rc;

    if ( args == NULL )
        rc = RC (rcApp, rcArgv, rcAccessing, rcSelf, rcNull);
    else
        rc = ArgsProgram (args, &fullpath, &progname);
    if ( rc )
        progname = fullpath = def_name;

    UsageSummary ( progname );

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
    return DIR_TABLE_VERS;
}


static rc_t write_row( VCursor *cursor, uint32_t idx, uint32_t nidx, uint32_t sidx )
{
    rc_t rc = VCursorOpenRow( cursor );
    if ( rc != 0 )
        OUTMSG(( "VCursorOpenRow() -> %R\n", rc ));
    else
    {
        rc_t rc1;
        char s[ 128 ];
        int n = snprintf( s, sizeof s, "here comes line # %u", idx );
        rc_t rc = VCursorWrite( cursor, nidx, 8, s, 0, n );
        if ( rc != 0 )
            OUTMSG(( "VCursorWrite( DIR_NAME ) -> %R\n", rc ));
        else
        {
            rc = VCursorWrite( cursor, sidx, 32, &idx, 0, 1 );
            if ( rc != 0 )
                OUTMSG(( "VCursorWrite( DIR_SIZE ) -> %R\n", rc ));
            else
            {
                rc = VCursorCommitRow( cursor );
                if ( rc != 0 )
                    OUTMSG(( "VCursorCommitRow() -> %R\n", rc ));
            }
        }

        rc1 = VCursorCloseRow( cursor );
        if ( rc1 != 0 )
            OUTMSG(( "VCursorCloseRow() -> %R\n", rc1 ));
    }
    return rc;
}

static rc_t write_loop( VCursor *cursor, uint32_t nidx, uint32_t sidx )
{
    uint32_t i;
    rc_t rc = 0;
    for ( i = 0; i < 10 && rc == 0; ++i )
    {
        rc = write_row( cursor, i, nidx, sidx );
    }
    return rc;
}

static rc_t write_2_cursor( VCursor *cursor )
{
    uint32_t name_idx, size_idx;
    rc_t rc1;
    rc_t rc = VCursorAddColumn( cursor, &name_idx, "DIR_NAME" );
    if ( rc != 0 )
        OUTMSG(( "VCursorAddColumn( DIR_NAME ) -> %R\n", rc ));
    else
    {
        rc = VCursorAddColumn( cursor, &size_idx, "DIR_SIZE" );
        if ( rc != 0 )
            OUTMSG(( "VCursorAddColumn( DIR_SIZE ) -> %R\n", rc ));
        else
        {
            rc = VCursorOpen( cursor );
            if ( rc != 0 )
                OUTMSG(( "VCursorOpen() -> %R\n", rc ));
            else
            {
                rc = write_loop( cursor, name_idx, size_idx );
                if ( rc == 0 )
                {
                    rc = VCursorCommit( cursor );
                    if ( rc != 0 )
                        OUTMSG(( "VCursorCommit() -> %R\n", rc ));
                }
            }
        }
    }
    rc1 = VCursorRelease( cursor );
    if ( rc1 != 0 )
        OUTMSG(( "VCursorRelease() -> %R\n", rc1 ));
    return rc;
}


static rc_t write_something( VDBManager *manager, const VSchema * schema, const char * path )
{
    VTable *table;
    rc_t rc = VDBManagerCreateTable ( manager, &table, schema, "dir_tab", kcmInit, path );
    if ( rc != 0 )
        OUTMSG(( "VDBManagerCreateTable() -> %R\n", rc ));
    else
    {
        VCursor *cursor;
        rc = VTableCreateCursorWrite( table, &cursor, kcmInsert );
        if ( rc != 0 )
            OUTMSG(( "VTableCreateCursorWrite() -> %R\n", rc ));
        else
        {
            rc = write_2_cursor( cursor );
            VCursorRelease( cursor );
        }
        VTableRelease ( table );
    }
    return rc;
}


static rc_t read_cfg_str( const KConfig * cfg, const char * key,
                          char ** value )
{
    const KConfigNode *node;
    rc_t rc;
    
    *value = NULL;
    rc = KConfigOpenNodeRead ( cfg, &node, key );
    if ( rc == 0 )
    {
        size_t num_read, remaining;
        /* first we ask about the size to be read */
        rc = KConfigNodeRead ( node, 0, NULL, 0, &num_read, &remaining );
        if ( rc == 0 )
        {
            *value = malloc( remaining + 1 );
            if ( *value )
            {
                size_t to_read = remaining;
                rc = KConfigNodeRead ( node, 0, *value, to_read, &num_read, &remaining );
                if ( rc == 0 )
                    (*value)[ num_read ] = 0;
            }
            else
                rc = RC( rcExe, rcNoTarg, rcConstructing, rcMemory, rcExhausted );
        }
        KConfigNodeRelease( node );
    }
    return rc;
}


static char * read_cfg_key( const char * key )
{
    KConfig *cfg;
    char * res = NULL;

    rc_t rc = KConfigMake ( &cfg, NULL );
    if ( rc != 0 )
        OUTMSG(( "KConfigMake() -> %R\n", rc ));
    else
    {
        rc = read_cfg_str( cfg, key, &res );
        if ( rc != 0 )
            OUTMSG(( "read_cfg_str( '%s' ) -> %R\n", key, rc ));
        KConfigRelease ( cfg );
    }
    return res;
}


const char * schema_path_key = "vdb/schema/paths";
static char * build_schema_path( const char * to_load )
{
    char * res = NULL;
    char * schema_path = read_cfg_key( schema_path_key );
    if ( schema_path != NULL )
    {
        size_t len = string_size ( schema_path ) + string_size( to_load ) + 2;
        res = malloc( len );
        if ( res != NULL )
        {
            rc_t rc = string_printf ( res, len, NULL, "%s%s", schema_path, to_load );
            if ( rc != 0 )
                OUTMSG(( "string_printf() -> %R\n", rc ));
        }
        free( schema_path );
    }
    return res;
}


static rc_t load_vdb_schema( VSchema * schema, const char * to_load )
{
    rc_t rc = -1;
    char * path = build_schema_path( to_load );
    if ( path != NULL )
    {
        rc = VSchemaParseFile ( schema, path );
        if ( rc != 0 )
            OUTMSG(( "VSchemaParseFile( '%s' ) -> %R\n", path, rc ));
        free( path );
    }
    return rc;
}


const char * schema_to_load = "/vdb/vdb.vschema";
static rc_t prepare_table( VSchema * schema )
{
    VSchemaRuntimeTable *tbl;

    rc_t rc = load_vdb_schema( schema, schema_to_load );
    if ( rc != 0 )
        return rc;
    rc = VSchemaMakeRuntimeTable ( schema, &tbl, "dir_tab #1", NULL );
    if ( rc != 0 )
        OUTMSG(( "VSchemaMakeRuntimeTable() -> %R\n", rc ));
    else
    {
        rc_t rc1;

        rc = VSchemaRuntimeTableAddAsciiColumn ( tbl, "DIR_NAME" );
        if ( rc != 0 )
            OUTMSG(( "VSchemaRuntimeTableAddAsciiColumn() -> %R\n", rc ));
        if ( rc == 0 )
        {
            rc = VSchemaRuntimeTableAddIntegerColumn ( tbl, 32, false, "DIR_SIZE" );
            if ( rc != 0 )
                OUTMSG(( "VSchemaRuntimeTableAddAsciiColumn() -> %R\n", rc ));
            else
            {
                rc = VSchemaRuntimeTableCommit ( tbl );
                if ( rc != 0 )
                    OUTMSG(( "VSchemaRuntimeTableCommit() -> %R\n", rc ));
            }
        }
        rc1 = VSchemaRuntimeTableClose ( tbl );
        if ( rc1 != 0 )
            OUTMSG(( "VSchemaRuntimeTableClose() -> %R\n", rc1 ));
    }
    return rc;
}


static rc_t make_dir_table( char * tab_name )
{
    KDirectory *native_dir;

    rc_t rc = KDirectoryNativeDir( &native_dir );
    if ( rc != 0 )
        OUTMSG(( "KDirectoryNativeDir() -> %R\n", rc ));
    else
    {
        VDBManager *manager;
        rc = VDBManagerMakeUpdate ( &manager, native_dir );
        if ( rc != 0 )
            OUTMSG(( "VDBManagerMakeUpdate() -> %R\n", rc ));
        else
        {
            VSchema *schema;
            rc = VDBManagerMakeSchema( manager, &schema );
            if ( rc != 0 )
                OUTMSG(( "VDBManagerMakeSRASchema() -> %R\n", rc ));
            else
            {
                rc = prepare_table( schema );
                if ( rc == 0 )
                    rc = write_something( manager, schema, tab_name );
                VSchemaRelease( schema );
            }
            VDBManagerRelease( manager );
        }
        KDirectoryRelease( native_dir );
    }
    return rc;
}

rc_t CC KMain ( int argc, char *argv [] )
{
    if ( argc > 1 )
        return make_dir_table( argv[ 1 ] );
    else
        return make_dir_table( "dir_tab" );
    return 0;
}

