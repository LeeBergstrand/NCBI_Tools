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

#include "vdb-dump-context.h"
#include "vdb-dump-helper.h"

#include <klib/rc.h>
#include <klib/log.h>
#include <klib/status.h>
#include <klib/text.h>
#include <kapp/args.h>
#include <os-native.h>
#include <sysalloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/********************************************************************
the dump context contains all informations needed to execute the dump
********************************************************************/

static rc_t vdco_set_str( char **dst, const char *src )
{
    size_t len;
    if ( dst == NULL )
    {
        return RC( rcVDB, rcNoTarg, rcWriting, rcParam, rcNull );
    }
    if ( *dst != NULL )
    {
        free( *dst );
        *dst = NULL;
    }
    if ( src == NULL )
    {
        return RC( rcVDB, rcNoTarg, rcWriting, rcParam, rcNull );
    }
    len = strlen( src );
    if ( len == 0 )
    {
        return RC( rcVDB, rcNoTarg, rcWriting, rcItem, rcEmpty );
    }
    *dst = (char*)malloc( len + 1 );
    if ( *dst == NULL )
    {
        return RC( rcVDB, rcNoTarg, rcWriting, rcMemory, rcExhausted );
    }
    strcpy( *dst, src );
    return 0;
}

static void vdco_init_values( p_dump_context ctx )
{
    ctx->path = NULL;
    ctx->table = NULL;
    ctx->columns = NULL;
    ctx->excluded_columns = NULL;
    ctx->filter = NULL;

    ctx->print_row_id = true;
    ctx->print_in_hex = false;
    ctx->lf_after_row = 1;
    ctx->print_column_names = true;
    ctx->print_dna_bases = false;
    ctx->max_line_len = 0;
    ctx->indented_line_len = 0;

    ctx->help_requested = false;
    ctx->usage_requested = false;
    ctx->schema_dump_requested = false;
    ctx->table_enum_requested = false;
    ctx->version_requested = false;
    ctx->column_enum_requested = false;
    ctx->column_enum_short = false;
    ctx->id_range_requested = false;
    ctx->without_sra_types = false;
    ctx->dont_check_accession = false;
    ctx->print_num_elem = false;
    ctx->objver_requested = false;
    ctx->objtype_requested = false;
}

rc_t vdco_init( dump_context **ctx )
{
    rc_t rc = 0;
    if ( ctx == NULL )
    {
        rc = RC( rcVDB, rcNoTarg, rcConstructing, rcParam, rcNull );
    }
    if ( rc == 0 )
    {
        (*ctx) = (p_dump_context)calloc( 1, sizeof **ctx );
        if ( *ctx == NULL )
        {
            rc = RC( rcVDB, rcNoTarg, rcConstructing, rcMemory, rcExhausted );
        }
        if ( rc == 0 )
        {
            VectorInit( &((*ctx)->schema_list), 0, 5 );
            vdco_init_values( *ctx );
            rc = vdn_make( &((*ctx)->row_generator) );
            DISP_RC( rc, "num_gen_make() failed" );
        }
    }
    return rc;
}

static void CC vdco_schema_list_entry_whack( void *item, void *data )
{
    free( item );
}

rc_t vdco_destroy( p_dump_context ctx )
{
    rc_t rc = 0;
    if ( ctx == NULL )
    {
        rc = RC( rcVDB, rcNoTarg, rcDestroying, rcParam, rcNull );
    }
    if ( rc == 0 )
    {
        VectorWhack( &(ctx->schema_list),
                     vdco_schema_list_entry_whack, NULL );
        if ( ctx->table != NULL )
        {
            free( (void*)ctx->table );
            ctx->table = NULL;
        }
        if ( ctx->columns != NULL )
        {
            free( (void*)ctx->columns );
            ctx->columns = NULL;
        }
        if ( ctx->excluded_columns != NULL )
        {
            free( (void*)ctx->excluded_columns );
            ctx->excluded_columns = NULL;
        }
        vdn_destroy( ctx->row_generator );
        free( ctx );
    }
    return rc;
}


static rc_t vdco_add_schema( p_dump_context ctx, const char *src )
{
    rc_t rc = 0;
    if ( ( ctx == NULL )||( src == NULL ) )
    {
        rc = RC( rcVDB, rcNoTarg, rcWriting, rcParam, rcNull );
    }
    if ( rc == 0 )
    {
        char *s = string_dup_measure ( src, NULL );
        if ( s != NULL )
        {
            rc = VectorAppend( &(ctx->schema_list), NULL, s );
            DISP_RC( rc, "VectorAppend() failed" );
            if ( rc != 0 )
            {
                free( s );
            }
        }
        else
            rc = RC( rcVDB, rcNoTarg, rcWriting, rcParam, rcNull );
    }
    return rc;
}

static rc_t vdco_set_filter( p_dump_context ctx, const char *src )
{
    rc_t rc = 0;
    if ( ( ctx == NULL )||( src == NULL ) )
    {
        rc = RC( rcVDB, rcNoTarg, rcWriting, rcParam, rcNull );
    }
    if ( rc == 0 )
    {
        rc = vdco_set_str( (char**)&(ctx->filter), src );
        DISP_RC( rc, "dump_context_set_str() failed" );
    }
    return rc;
}

/* not static because can be called directly from vdb-dump.c */
rc_t vdco_set_table( p_dump_context ctx, const char *src )
{
    rc_t rc = 0;
    if ( ( ctx == NULL )||( src == NULL ) )
    {
        rc = RC( rcVDB, rcNoTarg, rcWriting, rcParam, rcNull );
    }
    if ( rc == 0 )
    {
        rc = vdco_set_str( (char**)&(ctx->table), src );
        DISP_RC( rc, "dump_context_set_str() failed" );
    }
    return rc;
}

static rc_t vdco_set_columns( p_dump_context ctx, const char *src )
{
    rc_t rc = 0;
    if ( ( ctx == NULL )||( src == NULL ) )
    {
        rc = RC( rcVDB, rcNoTarg, rcWriting, rcParam, rcNull );
    }
    if ( rc == 0 )
    {
        rc = vdco_set_str( (char**)&(ctx->columns), src );
        DISP_RC( rc, "dump_context_set_str() failed" );
    }
    return rc;
}

static rc_t vdco_set_excluded_columns( p_dump_context ctx, const char *src )
{
    rc_t rc = 0;
    if ( ( ctx == NULL )||( src == NULL ) )
    {
        rc = RC( rcVDB, rcNoTarg, rcWriting, rcParam, rcNull );
    }
    if ( rc == 0 )
    {
        rc = vdco_set_str( (char**)&(ctx->excluded_columns), src );
        DISP_RC( rc, "dump_context_set_str() failed" );
    }
    return rc;
}

static rc_t vdco_set_row_range( p_dump_context ctx, const char *src )
{
    rc_t rc = 0;
    if ( ( ctx == NULL )||( src == NULL ) )
    {
        rc = RC( rcVDB, rcNoTarg, rcWriting, rcParam, rcNull );
    }
    if ( rc == 0 )
    {
        vdn_parse( ctx->row_generator, src );
    }
    return rc;
}


static bool vdco_set_format( p_dump_context ctx, const char *src )
{
    if ( ctx == NULL ) return false;
    if ( src == NULL ) return false;
    if ( strcmp( src, "csv" ) == 0 )
        ctx->format = df_csv;
    else if ( strcmp( src, "xml" ) == 0 )
        ctx->format = df_xml;
    else if ( strcmp( src, "json" ) == 0 )
        ctx->format = df_json;
    else if ( strcmp( src, "piped" ) == 0 )
        ctx->format = df_piped;
    else if ( strcmp( src, "tab" ) == 0 )
        ctx->format = df_tab;
    else ctx->format = df_default;
    return true;
}


static bool vdco_set_boolean_char( dump_context *ctx, const char * src )
{
    ctx->c_boolean = 0;
    if ( ctx == NULL ) return false;
    if ( src == NULL ) return false;
    if ( strcmp( src, "T" ) == 0 )
        ctx->c_boolean = 'T';
    else if ( strcmp( src, "1" ) == 0 )
        ctx->c_boolean = '1';
    return true;
}


static bool vdco_get_bool_option( const Args *my_args,
                                  const char *name,
                                  const bool def )
{
    bool res = def;
    uint32_t count;
    rc_t rc = ArgsOptionCount( my_args, name, &count );
    DISP_RC( rc, "ArgsOptionCount() failed" );
    if ( rc == 0 )
        res = ( count > 0 );
    return res;
}

static bool vdco_get_bool_neg_option( const Args *my_args,
                                      const char *name,
                                      const bool def )
{
    bool res = def;
    uint32_t count;
    rc_t rc = ArgsOptionCount( my_args, name, &count );
    DISP_RC( rc, "ArgsOptionCount() failed" );
    if ( rc == 0 )
        res = ( count == 0 );
    return res;
}

static uint16_t vdco_get_uint16_option( const Args *my_args,
                                        const char *name,
                                        const uint16_t def )
{
    uint16_t res = def;
    uint32_t count;
    rc_t rc = ArgsOptionCount( my_args, name, &count );
    DISP_RC( rc, "ArgsOptionCount() failed" );
    if ( ( rc == 0 )&&( count > 0 ) )
    {
        const char *s;
        rc = ArgsOptionValue( my_args, name, 0,  &s );
        DISP_RC( rc, "ArgsOptionValue() failed" );
        if ( rc == 0 ) res = atoi( s );
    }
    return res;
}

static const char* vdco_get_str_option( const Args *my_args,
                                        const char *name )
{
    const char* res = NULL;
    uint32_t count;
    rc_t rc = ArgsOptionCount( my_args, name, &count );
    DISP_RC( rc, "ArgsOptionCount() failed" );
    if ( ( rc == 0 )&&( count > 0 ) )
    {
        rc = ArgsOptionValue( my_args, name, 0, &res );
        DISP_RC( rc, "ArgsOptionValue() failed" );
    }
    return res;
}

void vdco_set_schemas( const Args *my_args, p_dump_context ctx )
{
    uint32_t count;
    rc_t rc = ArgsOptionCount( my_args, OPTION_SCHEMA, &count );
    DISP_RC( rc, "ArgsOptionCount() failed" );
    if ( ( rc == 0 )&&( count > 0 ) )
    {
        uint32_t i;
        for ( i=0; i<count; ++i )
        {
            const char* txt = NULL;
            rc = ArgsOptionValue( my_args, OPTION_SCHEMA, i, &txt );
            DISP_RC( rc, "ArgsOptionValue() failed" );
            if ( ( rc == 0 )&&( txt != NULL ) )
            {
                rc = vdco_add_schema( ctx, txt );
                DISP_RC( rc, "dump_context_add_schema() failed" );
            }
        }
    }
}


size_t vdco_schema_count( p_dump_context ctx )
{
    if ( ctx == NULL ) return 0;
    return VectorLength( &(ctx->schema_list ) );
}

static void vdco_evaluate_options( const Args *my_args,
                                   dump_context *ctx )
{
    if ( my_args == NULL ) return;
    if ( ctx == NULL ) return;

    ctx->help_requested = vdco_get_bool_option( my_args, OPTION_HELP, false );
    ctx->print_row_id = vdco_get_bool_option( my_args, OPTION_ROW_ID_ON, false );
    ctx->lf_after_row = vdco_get_uint16_option( my_args, OPTION_LINE_FEED, 1 );
    ctx->print_column_names = vdco_get_bool_neg_option( my_args, OPTION_COLNAME_OFF, true );
    ctx->print_in_hex = vdco_get_bool_option( my_args, OPTION_IN_HEX, false );
    ctx->schema_dump_requested = vdco_get_bool_option( my_args, OPTION_SCHEMA_DUMP, false );
    ctx->table_enum_requested = vdco_get_bool_option( my_args, OPTION_TABLE_ENUM, false );
    ctx->version_requested = vdco_get_bool_option( my_args, OPTION_VERSION, false );
    ctx->column_enum_requested = vdco_get_bool_option( my_args, OPTION_COLUMN_ENUM, false );
    ctx->column_enum_short = vdco_get_bool_option( my_args, OPTION_COLUMN_SHORT, false );
    ctx->print_dna_bases = vdco_get_bool_option( my_args, OPTION_DNA_BASES, false );
    ctx->objver_requested = vdco_get_bool_option( my_args, OPTION_OBJVER, false );
    ctx->objtype_requested = vdco_get_bool_option( my_args, OPTION_OBJTYPE, false );
    ctx->max_line_len = vdco_get_uint16_option( my_args, OPTION_MAX_LINE_LEN, 0 );
    ctx->indented_line_len = vdco_get_uint16_option( my_args, OPTION_LINE_INDENT, 0 );
    ctx->id_range_requested = vdco_get_bool_option( my_args, OPTION_ID_RANGE, false );
    vdco_set_format( ctx, vdco_get_str_option( my_args, OPTION_FORMAT ) );
    ctx->without_sra_types = vdco_get_bool_option( my_args, OPTION_WITHOUT_SRA, false );
    ctx->dont_check_accession = vdco_get_bool_option( my_args, OPTION_WITHOUT_ACCESSION, false );
    ctx->print_num_elem = vdco_get_bool_option( my_args, OPTION_NUMELEM, false );
    ctx->sum_num_elem = vdco_get_bool_option( my_args, OPTION_NUMELEMSUM, false );
    ctx->show_blobbing = vdco_get_bool_option( my_args, OPTION_SHOW_BLOBBING, false );
    ctx->enum_phys = vdco_get_bool_option( my_args, OPTION_ENUM_PHYS, false );

    vdco_set_table( ctx, vdco_get_str_option( my_args, OPTION_TABLE ) );
    vdco_set_columns( ctx, vdco_get_str_option( my_args, OPTION_COLUMNS ) );
    vdco_set_excluded_columns( ctx, vdco_get_str_option( my_args, OPTION_EXCLUDED_COLUMNS ) );
    vdco_set_row_range( ctx, vdco_get_str_option( my_args, OPTION_ROWS ) );
    vdco_set_schemas( my_args, ctx );
    vdco_set_filter( ctx, vdco_get_str_option( my_args, OPTION_FILTER ) );
    vdco_set_boolean_char( ctx, vdco_get_str_option( my_args, OPTION_BOOLEAN ) );
}

rc_t vdco_capture_arguments_and_options( const Args * args, dump_context *ctx)
{
    rc_t rc;

    vdco_evaluate_options( args, ctx );

    rc = ArgsHandleLogLevel( args );
    DISP_RC( rc, "ArgsHandleLogLevel() failed" );
    return rc;
}
