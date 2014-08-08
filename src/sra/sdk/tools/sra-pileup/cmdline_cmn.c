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

#include "cmdline_cmn.h"

#include <kapp/args.h>

#include <klib/rc.h>
#include <klib/log.h>
#include <klib/out.h>

#include <sra/srapath.h>

#include <vfs/path.h>
#include <vfs/path-priv.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <os-native.h>
#include <sysalloc.h>

#include <common/test_assert.h>

const char * ref_usage[] = { "Filter by position on genome.",
                             "Name can either be file specific name",
                             "(ex: \"chr1\" or \"1\").",
                             "\"from\" and \"to\" are 1-based coordinates",
                             NULL };

const char * outf_usage[] = { "Output will be written to this file",
                              "instead of std-out", NULL };

const char * table_usage[] = { "Which alignment table(s) to use (p|s|e):", 
                               "p - primary, s - secondary, e - evidence-interval", 
                               "(default = p)", NULL };

const char * gzip_usage[] = { "Compress output using gzip", NULL };

const char * bzip_usage[] = { "Compress output using bzip2", NULL };

const char * inf_usage[] = { "File with all input-parameters / options", NULL };

const char * schema_usage[] = { "optional schema-file to be used", NULL };

#define OPTION_REF     "aligned-region"
#define ALIAS_REF      "r"

#define OPTION_OUTF    "outfile"
#define ALIAS_OUTF     "o"

#define OPTION_TABLE   "table"
#define ALIAS_TABLE    "t"

#define OPTION_GZIP    "gzip"
#define ALIAS_GZIP     NULL

#define OPTION_BZIP    "bzip2"
#define ALIAS_BZIP     NULL

#define OPTION_INF    "infile"
#define ALIAS_INF     "f"

#define OPTION_SCHEMA "schema"
#define ALIAS_SCHEMA  "S"

OptDef CommonOptions[] =
{
    /*name,           alias,         hfkt, usage-help,    maxcount, needs value, required */
    { OPTION_REF,     ALIAS_REF,     NULL, ref_usage,     0,        true,        false },
    { OPTION_OUTF,    ALIAS_OUTF,    NULL, outf_usage,    1,        true,        false },
    { OPTION_TABLE,   ALIAS_TABLE,   NULL, table_usage,   1,        true,        false },
    { OPTION_GZIP,    ALIAS_GZIP,    NULL, gzip_usage,    1,        false,       false },
    { OPTION_BZIP,    ALIAS_BZIP,    NULL, bzip_usage,    1,        false,       false },
    { OPTION_INF,     ALIAS_INF,     NULL, inf_usage,     0,        true,        false },
    { OPTION_SCHEMA,  ALIAS_SCHEMA,  NULL, schema_usage,  1,        true,        false }
};


/* =========================================================================================== */

static rc_t get_str_option( const Args *args, const char *name, const char ** res )
{
    uint32_t count;
    rc_t rc = ArgsOptionCount( args, name, &count );
    *res = NULL;
    if ( rc != 0 )
    {
        LOGERR( klogInt, rc, "ArgsOptionCount() failed" );
    }
    else
    {
        if ( count > 0 )
        {
            rc = ArgsOptionValue( args, name, 0, res );
            if ( rc != 0 )
            {
                LOGERR( klogInt, rc, "ArgsOptionValue() failed" );
            }
        }
    }
    return rc;
}


static rc_t get_bool_option( const Args *args, const char *name, bool *res, const bool def )
{
    uint32_t count;
    rc_t rc = ArgsOptionCount( args, name, &count );
    if ( rc == 0 && count > 0 )
    {
        *res = true;
    }
    else
    {
        *res = def;
    }
    return rc;
}

/* =========================================================================================== */

rc_t get_common_options( Args * args, common_options *opts )
{
    rc_t rc = get_str_option( args, OPTION_OUTF, &opts->output_file );

    if ( rc == 0 )
        rc = get_str_option( args, OPTION_INF, &opts->input_file );

    if ( rc == 0 )
        rc = get_bool_option( args, OPTION_GZIP, &opts->gzip_output, false );

    if ( rc == 0 )
        rc = get_bool_option( args, OPTION_BZIP, &opts->bzip_output, false );

    if ( rc == 0 )
        rc = get_str_option( args, OPTION_SCHEMA, &opts->schema_file );

    if ( rc == 0 )
    {
        const char * table2use = NULL;
        rc = get_str_option( args, OPTION_TABLE, &table2use );
        opts->tab_select = primary_ats;
        if ( rc == 0 && table2use != NULL )
        {
            size_t l = string_size ( table2use );
            opts->tab_select = 0;
            if ( ( string_chr ( table2use, l, 'p' ) != NULL )||
                 ( string_chr ( table2use, l, 'P' ) != NULL ) )
            { opts->tab_select |= primary_ats; };

            if ( ( string_chr ( table2use, l, 's' ) != NULL )||
                 ( string_chr ( table2use, l, 'S' ) != NULL ) )
            { opts->tab_select |= secondary_ats; };

            if ( ( string_chr ( table2use, l, 'e' ) != NULL )||
                 ( string_chr ( table2use, l, 'E' ) != NULL ) )
            { opts->tab_select |= evidence_ats; };
        }
    }

    return rc;
}

void print_common_helplines( void )
{
    HelpOptionLine ( ALIAS_REF, OPTION_REF, "name[:from-to]", ref_usage );
    HelpOptionLine ( ALIAS_OUTF, OPTION_OUTF, "output-file", outf_usage );
    HelpOptionLine ( ALIAS_TABLE, OPTION_TABLE, "table", table_usage );
    HelpOptionLine ( ALIAS_BZIP, OPTION_BZIP, "bzip", bzip_usage );
    HelpOptionLine ( ALIAS_GZIP, OPTION_GZIP, "gzip", gzip_usage );
}


OptDef * CommonOptions_ptr( void )
{
    return &CommonOptions[ 0 ];
}

size_t CommonOptions_count( void )
{
    return ( sizeof CommonOptions / sizeof CommonOptions [ 0 ] );
}


/* =========================================================================================== */


static int cmp_pchar( const char * a, const char * b )
{
    int res = 0;
    if ( ( a != NULL )&&( b != NULL ) )
    {
        size_t len_a = string_size( a );
        size_t len_b = string_size( b );
        res = string_cmp ( a, len_a, b, len_b, ( len_a < len_b ) ? len_b : len_a );
    }
    return res;
}


/* =========================================================================================== */


typedef struct range
{
    uint32_t start;
    uint32_t end;
} range;


static range * make_range( const uint64_t start, const uint64_t end )
{
    range *res = calloc( sizeof *res, 1 );
    if ( res != NULL )
    {
        res->start = start;
        res->end = end;
    }
    return res;
}


static int cmp_range( const range * a, const range * b )
{

    int res = ( a->start - b->start );
    if ( res == 0 )
        res = ( a->end - b->end );
    return res;
}


static bool range_overlapp( const range * a, const range * b )
{
    return ( !( ( b->end < a->start ) || ( b->start > a->end ) ) );
}


/* =========================================================================================== */

typedef struct reference_region
{
    BSTNode node;
    const char * name;
    Vector ranges;
} reference_region;


static reference_region * make_reference_region( const char *name )
{
    reference_region *res = calloc( sizeof *res, 1 );
    if ( res != NULL )
    {
        res->name = string_dup_measure ( name, NULL );
        VectorInit ( &res->ranges, 0, 5 );
    }
    return res;
}


static int CC cmp_range_wrapper( const void *item, const void *n )
{   return cmp_range( item, n ); }


static rc_t add_ref_region_range( reference_region * self, const uint64_t start, const uint64_t end )
{
    rc_t rc = 0;
    range *r = make_range( start, end );
    if ( r == NULL )
        rc = RC( rcApp, rcNoTarg, rcConstructing, rcMemory, rcExhausted );
    else
    {
        rc = VectorInsert ( &self->ranges, r, NULL, cmp_range_wrapper );
        if ( rc != 0 )
            free( r );
    }
    return rc;
}


#define RR_NAME  1
#define RR_START 2
#define RR_END   3


static void put_c( char *s, size_t size, size_t *dst, char c )
{
    if ( *dst < ( size - 1 ) )
        s[ *dst ] = c;
    (*dst)++;
}

static void finish_txt( char *s, size_t size, size_t *dst )
{
    if ( *dst > size )
        s[ size - 1 ] = 0;
    else
        s[ *dst ] = 0;
    *dst = 0;
}

static uint64_t finish_num( char *s, size_t size, size_t *dst )
{
    uint64_t res = 0;
    char *endp;
    finish_txt( s, size, dst );
    res = strtou64( s, &endp, 10 );
    return res;
}


/* s = refname:1000-2000 */
static void parse_definition( const char *s, char * name, size_t len,
                              uint64_t *start, uint64_t *end )
{
    size_t n = string_size( s );

    *start = 0;
    *end   = 0;
    name[ 0 ] = 0;
    if ( n > 0 )
    {
        size_t i, st, dst = 0;
        char tmp[ 32 ];
        st = RR_NAME;
        for ( i = 0; i < n; ++i )
        {
            char c = s[ i ];
            switch( st )
            {
                case RR_NAME  : if ( c == ':' )
                                {
                                    finish_txt( name, len, &dst );
                                    st = RR_START;
                                }
                                else
                                {
                                    put_c( name, len, &dst, c );
                                }
                                break;

                case RR_START : if ( c == '-' )
                                {
                                    *start = finish_num( tmp, sizeof tmp, &dst );
                                    st = RR_END;
                                }
                                else if ( ( c >= '0' )&&( c <= '9' ) )
                                {
                                    put_c( tmp, sizeof tmp, &dst, c );
                                }
                                break;

                case RR_END   : if ( ( c >= '0' )&&( c <= '9' ) )
                                {
                                    put_c( tmp, sizeof tmp, &dst, c );
                                }
                                break;
            }
        }
        switch( st )
        {
            case RR_NAME  : finish_txt( name, len, &dst );
                            break;

            case RR_START : *start = finish_num( tmp, sizeof tmp, &dst );
                            break;

            case RR_END   : *end = finish_num( tmp, sizeof tmp, &dst );
                            break;
        }
    }
}


static void CC release_range_wrapper( void * item, void * data )
{
    free( item );
}


static void free_reference_region( reference_region * self )
{
    free( (void*)self->name );
    VectorWhack ( &self->ranges, release_range_wrapper, NULL );
    free( self );
}


static void check_ref_region_ranges( reference_region * self )
{
    uint32_t n = VectorLength( &self->ranges );
    uint32_t i = 0;
    range *a = NULL;
    while ( i < n )
    {
        range *b = VectorGet ( &self->ranges, i );
        bool remove = false;
        if ( a != NULL )
        {
            remove = range_overlapp( a, b );
            if ( remove )
            {
                range *r;
                a->end = b->end;
                VectorRemove ( &self->ranges, i, (void**)&r );
                free( r );
                n--;
            }
        }
        if ( !remove )
        {
            a = b;
            ++i;
        }
    }
}


/* =========================================================================================== */


static int CC reference_vs_pchar_wrapper( const void *item, const BSTNode *n )
{
    const reference_region * r = ( const reference_region * )n;
    return cmp_pchar( (const char *)item, r->name );
}

static reference_region * find_reference_region( BSTree * regions, const char * name )
{
    return ( reference_region * ) BSTreeFind ( regions, name, reference_vs_pchar_wrapper );
}

static int CC ref_vs_ref_wrapper( const BSTNode *item, const BSTNode *n )
{
   const reference_region * a = ( const reference_region * )item;
   const reference_region * b = ( const reference_region * )n;
   return cmp_pchar( a->name, b->name );
}

static rc_t add_refrange( BSTree * regions, const char * name, const uint64_t start, const uint64_t end )
{
    rc_t rc;

    reference_region * r = find_reference_region( regions, name );
    if ( r == NULL )
    {
        r = make_reference_region( name );
        if ( r == NULL )
            rc = RC( rcApp, rcNoTarg, rcConstructing, rcMemory, rcExhausted );
        else
            rc = add_ref_region_range( r, start, end );
        if ( rc == 0 )
            rc = BSTreeInsert ( regions, (BSTNode *)r, ref_vs_ref_wrapper );
        if ( rc != 0 )
            free_reference_region( r );
    }
    else
    {
        rc = add_ref_region_range( r, start, end );
    }
    return rc;
}


rc_t parse_and_add_region( BSTree * regions, const char * s )
{
    uint64_t start, end;
    char name[ 64 ];
    parse_definition( s, name, sizeof name, &start, &end );
    if ( name[ 0 ] == 0 )
        return RC( rcApp, rcNoTarg, rcConstructing, rcMemory, rcExhausted );
    else
        return add_refrange( regions, name, start, end );
}


static void CC check_refrange_wrapper( BSTNode *n, void *data )
{
    check_ref_region_ranges( ( reference_region * ) n );
}


void check_ref_regions( BSTree * regions )
{
    BSTreeForEach ( regions, false, check_refrange_wrapper, NULL );
}


static void CC release_ref_region_wrapper( BSTNode *n, void * data )
{
    free_reference_region( ( reference_region * ) n );
}


void free_ref_regions( BSTree * regions )
{    
    BSTreeWhack ( regions, release_ref_region_wrapper, NULL );
}


rc_t init_ref_regions( BSTree * tree, Args * args )
{
    uint32_t count;
    rc_t rc;

    BSTreeInit( tree );
    rc = ArgsOptionCount( args, OPTION_REF, &count );
    if ( rc != 0 )
    {
        LOGERR( klogInt, rc, "ArgsOptionCount() failed" );
    }
    else
    {
        uint32_t i;
        for ( i = 0; i < count && rc == 0; ++i )
        {
            const char * s;
            rc = ArgsOptionValue( args, OPTION_REF, i, &s );
            if ( rc != 0 )
                LOGERR( klogInt, rc, "ArgsOptionValue() failed" );
            else
                rc = parse_and_add_region( tree, s );
        }
    }
    return rc;
}


static void CC count_ref_region_wrapper( BSTNode *n, void *data )
{   
    reference_region * r = ( reference_region * ) n;
    uint32_t * count = ( uint32_t * ) data;
    *count += VectorLength( &(r->ranges) );
}


uint32_t count_ref_regions( BSTree * regions )
{
    uint32_t res = 0;
    BSTreeForEach ( regions, false, count_ref_region_wrapper, &res );
    return res;
}


typedef struct foreach_ref_region_func
{
    rc_t ( CC * on_region ) ( const char * name, uint32_t start, uint32_t end, void *data );
    const char * name;
    void * data;
    rc_t rc;
} foreach_ref_region_func;


static void CC foreach_range_vector_wrapper( void *item, void *data )
{
    range * r = ( range * ) item;
    foreach_ref_region_func * func = ( foreach_ref_region_func * )data;

    if ( func->rc == 0 )
    {
        func->rc = func->on_region( func->name, r->start, r->end, func->data );
    }
}


static void CC foreach_ref_region_wrapper( BSTNode *n, void *data )
{   
    reference_region * r = ( reference_region * ) n;
    foreach_ref_region_func * func = ( foreach_ref_region_func * )data;

    if ( func->rc == 0 )
    {
        func->name = r->name;
        VectorForEach ( &(r->ranges), false, foreach_range_vector_wrapper, data );
    }
}


rc_t foreach_ref_region( BSTree * regions,
    rc_t ( CC * on_region ) ( const char * name, uint32_t start, uint32_t end, void *data ), 
    void *data )
{
    foreach_ref_region_func func;

    func.on_region = on_region;
    func.data = data;
    func.rc = 0;
    BSTreeForEach ( regions, false, foreach_ref_region_wrapper, &func );
    return func.rc;
}


/* =========================================================================================== */

#if TOOLS_USE_SRAPATH != 0
static bool is_this_a_filesystem_path( const char * path )
{
    bool res = false;
    size_t i, n = string_size ( path );
    for ( i = 0; i < n && !res; ++i )
    {
        char c = path[ i ];
        res = ( c == '.' || c == '/' || c == '\\' );
    }
    return res;
}
#endif

#if TOOLS_USE_SRAPATH != 0
static char *translate_accession( SRAPath *my_sra_path,
                           const char *accession,
                           const size_t bufsize )
{
    rc_t rc;
    char * res = calloc( 1, bufsize );
    if ( res == NULL ) return NULL;

    rc = SRAPathFind( my_sra_path, accession, res, bufsize );
    if ( GetRCState( rc ) == rcNotFound )
    {
        free( res );
        return NULL;
    }
    else if ( GetRCState( rc ) == rcInsufficient )
    {
        free( res );
        return translate_accession( my_sra_path, accession, bufsize * 2 );
    }
    else if ( rc != 0 )
    {
        free( res );
        return NULL;
    }
    return res;
}
#endif

#if TOOLS_USE_SRAPATH != 0
static rc_t resolve_accession( const KDirectory *my_dir, char ** path )
{
    SRAPath *my_sra_path;
    rc_t rc = 0;

    if ( strchr ( *path, '/' ) != NULL )
        return 0;

    rc = SRAPathMake( &my_sra_path, my_dir );
    if ( rc != 0 )
    {
        if ( GetRCState ( rc ) != rcNotFound || GetRCTarget ( rc ) != rcDylib )
        {
            if ( rc != 0 )
            {
                LOGERR( klogInt, rc, "SRAPathMake() failed" );
            }
        }
        else
            rc = 0;
    }
    else
    {
        if ( !SRAPathTest( my_sra_path, *path ) )
        {
            char *buf = translate_accession( my_sra_path, *path, 64 );
            if ( buf != NULL )
            {
                free( (char*)(*path) );
                *path = buf;
            }
        }
        SRAPathRelease( my_sra_path );
    }
    return rc;
}
#endif


/* =========================================================================================== */


/****************************************************************************************
    splits an argument

    example: "/path/file=grp1" into path = "/path/file" and attribute = "grp1"
    or
    example: "/path/file" into path = "/path/file" and attribute = NULL

****************************************************************************************/
static rc_t split_argument( const char *argument, char ** path, char ** attribute, char delim )
{
    if ( argument == NULL || path == NULL || attribute == NULL )
        return RC( rcApp, rcNoTarg, rcConstructing, rcParam, rcNull );
    else
    {
        char * delim_ptr = string_chr ( argument, string_size ( argument ), delim );
        if ( delim_ptr == NULL )
        {
            *path = string_dup_measure( argument, NULL );
            *attribute = NULL;
        }
        else
        {
            size_t len = string_size( argument );
            size_t len1 = ( delim_ptr - argument );
            *path = string_dup ( argument, len1 );
            if ( delim_ptr < argument + len - 1 )
                *attribute = string_dup ( delim_ptr + 1, len - ( len1 + 1 ) );
            else
                *attribute = NULL;
        }
    }
    return 0;
}


static rc_t split_vpath_into_path_and_readgroup( VPath *vpath, const char *argument, char ** path, char ** attribute )
{
    size_t zz;
    char readgroup_buffer[ 256 ];
    rc_t rc1 = VPathOption( vpath, vpopt_readgroup, readgroup_buffer, sizeof readgroup_buffer - 1, &zz );
    if ( rc1 == 0 )
        *attribute = string_dup( readgroup_buffer, zz );
    *path = string_dup( argument, string_size( argument ) );
    return 0;
}


static rc_t test_split_vpath_into_path_and_readgroup( VPath *vpath, const char *argument, char ** path, char ** attribute )
{
    rc_t rc = 0;
    VPUri_t uri_type = VPathGetUri_t( vpath );
    switch ( uri_type )
    {
        default:
        case vpuri_invalid:
            rc = RC( rcExe, rcParam, rcAccessing, rcPath, rcInvalid );
            break;

        case vpuri_not_supported:
            rc = RC( rcExe, rcParam, rcAccessing, rcPath, rcUnsupported );
            break;

        case vpuri_none:
            rc = split_argument( argument, path, attribute, '=' );
            break;

        case vpuri_ncbi_vfs:
        case vpuri_file:
        case vpuri_ncbi_acc:
        case vpuri_http:
            rc = split_vpath_into_path_and_readgroup( vpath, argument, path, attribute );
            break;
    }
    return rc;
}


static rc_t split_argument_into_path_and_readgroup( const char *argument, char ** path, char ** attribute )
{
    rc_t rc;
    char * colon_ptr = string_chr ( argument, string_size ( argument ), ':' );
    if ( colon_ptr == NULL )
    {
        /* we do not have a colon in the argument, that means: there is no uri-syntax involved
           ---> we can split the "old fashioned way" at the equal-sign */
        rc = split_argument( argument, path, attribute, '=' );
    }
    else
    {
        VPath * vpath;
        rc_t rc = VPathMake( &vpath, argument );
        *path = NULL;
        *attribute = NULL;

        if ( rc == 0 )
        {
            rc = test_split_vpath_into_path_and_readgroup( vpath, argument, path, attribute );
            VPathRelease( vpath );
        }
    }
    return rc;
}


/* =========================================================================================== */


rc_t foreach_argument( Args * args, KDirectory *dir, bool div_by_spotgrp, bool * empty,
    rc_t ( CC * on_argument ) ( const char * path, const char * spot_group, void * data ), void * data )
{
    uint32_t count;
    rc_t rc = ArgsParamCount( args, &count );
    if ( rc != 0 )
    {
        LOGERR( klogInt, rc, "ArgsParamCount() failed" );
    }
    else
    {
        uint32_t idx;
        if ( empty != NULL )
        {
            *empty = ( count == 0 );
        }
        for ( idx = 0; idx < count && rc == 0; ++idx )
        {
            const char *param = NULL;
            rc = ArgsParamValue( args, idx, &param );
            if ( rc != 0 )
            {
                LOGERR( klogInt, rc, "ArgsParamvalue() failed" );
            }
            else
            {
                
                char * path = NULL;
                char * spot_group = NULL;

                rc = split_argument_into_path_and_readgroup( param, &path, &spot_group );
                if ( rc == 0 && path != NULL )
                {
                    /* in case there is no spotgroup-override from the commandline AND
                       the option to divide by spot-group is set, let spot_group point
                       to an empty string ---> divide by original spot-group! */
                    if ( spot_group == NULL && div_by_spotgrp )
                    {
                        spot_group = calloc( 1, 1 );
                    }

#if TOOLS_USE_SRAPATH != 0
                    if ( !is_this_a_filesystem_path( path ) )
                    {
                        rc = resolve_accession( dir, &path );
                    }
#endif

                    if ( rc == 0 )
                    {
                        rc = on_argument( path, spot_group, data );
                    }

                    free( path );
                    if ( spot_group != NULL )
                        free( spot_group );
                }
            }
        }
    }
    return rc;
}


/* =========================================================================================== */


static rc_t prepare_whole_file( prepare_ctx * ctx )
{
    rc_t rc = 0;
    if ( ctx->reflist != NULL )
    {
        uint32_t count;
        rc = ReferenceList_Count( ctx->reflist, &count );
        if ( rc != 0 )
        {
            LOGERR( klogInt, rc, "ReferenceList_Count() failed" );
        }
        else
        {
            uint32_t idx;
            for ( idx = 0; idx < count && rc == 0; ++idx )
            {
                rc = ReferenceList_Get( ctx->reflist, &ctx->refobj, idx );
                if ( rc != 0 )
                {
                    LOGERR( klogInt, rc, "ReferenceList_Get() failed" );
                }
                else
                {
                    rc = ctx->on_section( ctx, 0, 0 );
                    if ( rc == 0 )
                    ReferenceObj_Release( ctx->refobj );
                }
            }
        }
    }
    else
    {
        ctx->refobj = NULL;
        rc = ctx->on_section( ctx, 0, 0 );
    }
    return rc;
}


static rc_t CC prepare_region_cb( const char * name, uint32_t start, uint32_t end, void * data )
{
    prepare_ctx * ctx = ( prepare_ctx * )data;
    rc_t rc = ReferenceList_Find( ctx->reflist, &ctx->refobj, name, string_size( name ) );
    if ( rc != 0 )
    {
        LOGERR( klogInt, rc, "ReferenceList_Find() failed" );
    }
    else
    {
        rc = ctx->on_section( ctx, start, end );
        if ( rc == 0 )
        ReferenceObj_Release( ctx->refobj );
    }
    return rc;
}


static rc_t prepare_db_table( prepare_ctx *ctx,
                              const VDBManager *vdb_mgr,
                              VSchema *vdb_schema,
                              const char * path )
{
    rc_t rc;
    ctx->db = NULL;
    ctx->seq_tab = NULL;

    rc = VDBManagerOpenDBRead ( vdb_mgr, &ctx->db, vdb_schema, "%s", path );
    if ( rc != 0 )
    {
        rc = VDBManagerOpenTableRead ( vdb_mgr, &ctx->seq_tab, NULL, "%s", path );
        if ( rc != 0 )
        {
            LOGERR( klogInt, rc, "VDBManagerOpenTableRead() failed" );
        }
    }
    else
    {
        rc = VDatabaseOpenTableRead( ctx->db, &ctx->seq_tab, "SEQUENCE" );
        if ( rc != 0 )
        {
            LOGERR( klogInt, rc, "VDatabaseOpenTableRead( SEQUENCE ) failed" );
        }
    }
    return rc;
}


static rc_t prepare_reflist( prepare_ctx *ctx )
{
    rc_t rc = 0;
    ctx->reflist = NULL;
    if ( ctx->db != NULL )
    {
        uint32_t reflist_options = ereferencelist_4na;

        if ( ctx->use_primary_alignments )
            reflist_options |= ereferencelist_usePrimaryIds;

        if ( ctx->use_secondary_alignments )
            reflist_options |= ereferencelist_useSecondaryIds;

        if ( ctx->use_evidence_alignments )
            reflist_options |= ereferencelist_useEvidenceIds;

        rc = ReferenceList_MakeDatabase( &ctx->reflist, ctx->db, reflist_options, 0, NULL, 0 );
        if ( rc != 0 )
        {
            LOGERR( klogInt, rc, "ReferenceList_MakeDatabase() failed" );
        }
    }
    return rc;
}


rc_t prepare_ref_iter( prepare_ctx *ctx,
                       const VDBManager *vdb_mgr,
                       VSchema *vdb_schema,
                       const char * path,
                       BSTree * regions )
{
    rc_t rc = prepare_db_table( ctx, vdb_mgr, vdb_schema, path );
    if ( rc == 0 )
    {
        rc = prepare_reflist( ctx );
        if( rc == 0 )
        {
            if ( ctx->reflist == NULL || count_ref_regions( regions ) == 0 )
            {
                /* the user has not specified a reference-range : use the whole file... */
                rc = prepare_whole_file( ctx );
            }
            else
            {
                /* pick only the requested ranges... */
                rc = foreach_ref_region( regions, prepare_region_cb, ctx );
            }
        }
        if ( ctx->reflist != NULL )
        {
            ReferenceList_Release( ctx->reflist );
        }
    }
    VTableRelease ( ctx->seq_tab );
    VDatabaseRelease ( ctx->db );
    return rc;
}


/* =========================================================================================== */


rc_t parse_inf_file( Args * args )
{
    return Args_parse_inf_file( args, OPTION_INF );
}
