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

#include <kapp/args.h>

#include <klib/log.h>
#include <klib/rc.h>
#include <klib/text.h>
#include <klib/out.h>

#include <kfs/file.h>
#include <kfs/directory.h>
#include <kfs/buffile.h>

#include <insdc/insdc.h>

#include "trans_struct.h"
#include "options.h"

#include <stdlib.h> /* atoi() */
#include <stdio.h> /* fwrite() */

#include <common/test_assert.h>

/* =========================================================================================== */


rc_t get_str_option( const Args *args, const char *name, const char ** res )
{
    uint32_t count;
    rc_t rc = ArgsOptionCount( args, name, &count );
    *res = NULL;
    if ( rc != 0 )
        LOGERR( klogInt, rc, "ArgsOptionCount() failed" );
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


rc_t get_uint32_option( const Args *args, const char *name,
                        uint32_t *res, const uint32_t def )
{
    const char * s;
    rc_t rc = get_str_option( args, name, &s );
    if ( rc == 0 && s != NULL )
        *res = atoi( s );
    else
        *res = def;
    return rc;
}


rc_t get_int32_option( const Args *args, const char *name,
                       int32_t *res, const int32_t def )
{
    const char * s;
    rc_t rc = get_str_option( args, name, &s );
    if ( rc == 0 && s != NULL )
        *res = atoi( s );
    else
        *res = def;
    return rc;
}


rc_t get_uint32_array( const Args *args, const char *name,
                       uint32_t *res, uint32_t *count )
{
    uint32_t max = *count;
    rc_t rc = ArgsOptionCount( args, name, count );
    if ( rc != 0 )
        LOGERR( klogInt, rc, "ArgsOptionCount() failed" );
    else
    {
        uint32_t i;
        if ( *count > max ) *count = max;
        for ( i = 0; i < *count && rc == 0; ++ i )
        {
            const char * s;
            rc = ArgsOptionValue( args, name, i, &s );
            if ( rc == 0 )
                *(res++) = atoi( s );
        }
    }
    return rc;
}


/* =========================================================================================== */


/***************************************
    N (0x4E)  n (0x6E)  <--> 0x0
    A (0x41)  a (0x61)  <--> 0x1
    C (0x43)  c (0x63)  <--> 0x2
    M (0x4D)  m (0x6D)  <--> 0x3
    G (0x47)  g (0x67)  <--> 0x4
    R (0x52)  r (0x72)  <--> 0x5
    S (0x53)  s (0x73)  <--> 0x6
    V (0x56)  v (0x76)  <--> 0x7
    T (0x54)  t (0x74)  <--> 0x8
    W (0x57)  w (0x77)  <--> 0x9
    Y (0x59)  y (0x79)  <--> 0xA
    H (0x48)  h (0x68)  <--> 0xB
    K (0x4B)  k (0x6B)  <--> 0xC
    D (0x44)  d (0x64)  <--> 0xD
    B (0x42)  b (0x62)  <--> 0xE
    N (0x4E)  n (0x6E)  <--> 0xF
***************************************/

static INSDC_4na_bin ascii_2_4na_tab[] = 
{
/*         0x0  0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F */
/* 0x00 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 0x10 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 0x20 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 0x30 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 0x40 */ 0,   0x1, 0xE, 0x2, 0xD, 0,   0,   0x4, 0xB, 0,   0,   0xC, 0,   0x3, 0,   0,
/* 0x50 */ 0,   0,   0x5, 0x6, 0x8, 0,   0x7, 0x9, 0,   0xA, 0,   0,   0,   0,   0,   0,
/* 0x60 */ 0,   0x1, 0xE, 0x2, 0xD, 0,   0,   0x4, 0xB, 0,   0,   0xC, 0,   0x3, 0,   0,
/* 0x70 */ 0,   0,   0x5, 0x6, 0x8, 0,   0x7, 0x9, 0,   0xA, 0,   0,   0,   0,   0,   0,
/* 0x80 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 0x90 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 0xA0 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 0xB0 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 0xC0 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 0xD0 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 0xE0 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 0xF0 */ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};


static char _4na_2_ascii_tab[] =
{
/*  0x0  0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F */
    'N', 'A', 'C', 'M', 'G', 'R', 'S', 'V', 'T', 'W', 'Y', 'H', 'K', 'D', 'B', 'N',
    'n', 'a', 'c', 'm', 'g', 'r', 's', 'v', 't', 'w', 'y', 'h', 'k', 'd', 'b', 'n'
};


INSDC_4na_bin ascii_to_4na( const char c )
{
    return ascii_2_4na_tab[ c & 0xFF ];
}


char _4na_to_ascii( INSDC_4na_bin c, bool reverse )
{
    uint8_t mask = ( reverse ? 0x10 : 0 );
    return _4na_2_ascii_tab[ ( ( c | mask ) & 0x0F ) ];
}


INSDC_4na_bin * dup_2_4na( const char * s )
{
    size_t l = string_size ( s );
    INSDC_4na_bin * res = malloc( l );
    if ( res != NULL )
    {
        uint32_t i;
        for ( i = 0; i < l; ++i )
            res[ i ] = ascii_to_4na( s[ i ] );
    }
    return res;
}


char * dup_2_ascii( const INSDC_4na_bin * b, size_t len, bool reverse )
{
    char * res = malloc( len + 1 );
    if ( res != NULL )
    {
        uint32_t i;
        for ( i = 0; i < len; ++i )
            res[ i ] = _4na_to_ascii( b[ i ], reverse );
        res[ i ] = 0;
    }
    return res;
}


rc_t get_ro( Args * args, const char * name, const int32_t ** RO, uint32_t * ro_count )
{
    rc_t rc = ArgsOptionCount( args, name, ro_count );
    *RO = NULL;
    if ( ( rc == 0 )&&( *ro_count > 0 ) )
    {
        *RO = calloc( sizeof **RO, *ro_count );
        if ( *RO == NULL )
            rc = RC( rcApp, rcNoTarg, rcConstructing, rcMemory, rcExhausted );
        else
        {
            uint32_t i;
            for ( i = 0; i < *ro_count && rc == 0; ++i )
            {
                const char * s;
                rc = ArgsOptionValue( args, name, i, &s );
                if ( rc == 0 )
                {
                    ((int32_t*)(*RO))[i] = atoi( s );
                }
            }
        }
    }
    return rc;
}


void print_ro( const int32_t * RO, uint32_t ro_count )
{
    uint32_t i;

    OUTMSG(( "RO  : { " ));
    for ( i = 0; i < ro_count; ++i)
    {
        if ( i > 0 )
            OUTMSG(( ", %i", RO[i] ));
        else
            OUTMSG(( "%i", RO[i] ));
    }
    OUTMSG(( " }\n" ));
}


bool * make_bool_array( const char * s )
{
    size_t len = string_size ( s );
    bool * res = malloc( len );
    if ( res )
    {
        size_t idx;
        for ( idx = 0; idx < len; ++idx )
            res[ idx ] = ( s[ idx ] == '1' );
    }
    return res;
}


uint32_t count_true( const bool * v, uint32_t len )
{
    uint32_t i, res = 0;
    for ( i = 0; i < len; ++i )
        if ( v[ i ] )
            res++;
    return res;
}


int32_t vector_sum( const int32_t * v, uint32_t len, bool ignore_first )
{
    uint32_t i, start = ( ignore_first ? 1 : 0 );
    int32_t res = 0;
    for ( i = start; i < len; ++i )
        res += v[ i ];
    return res;
}


/* =========================================================================================== */


rc_t CC write_to_FILE( void *f, const char *buffer, size_t bytes, size_t *num_writ )
{
    * num_writ = fwrite ( buffer, 1, bytes, f );
    if ( * num_writ != bytes )
        return RC( rcExe, rcFile, rcWriting, rcTransfer, rcIncomplete );
    return 0;
}


/* =========================================================================================== */

typedef struct stdout_redir
{
    KWrtWriter org_writer;
    void* org_data;
    KFile* kfile;
    uint64_t pos;
} stdout_redir;


static rc_t CC stdout_redir_callback ( void* self, const char* buffer, size_t bufsize, size_t* num_writ )
{
    rc_t rc = 0;
    stdout_redir * writer = ( stdout_redir * ) self;

    do {
        rc = KFileWrite( writer->kfile, writer->pos, buffer, bufsize, num_writ );
        if ( rc == 0 )
        {
            buffer += *num_writ;
            bufsize -= *num_writ;
            writer->pos += *num_writ;
        }
    } while ( rc == 0 && bufsize > 0 );
    return rc;
}


rc_t make_stdout_redir ( stdout_redir ** writer, const char * filename, size_t bufsize )
{
    KDirectory *dir;
    rc_t rc = 0;
    stdout_redir * wr = NULL;

    if ( writer == NULL )
    {
        *writer = NULL;
        rc = RC ( rcApp, rcNoTarg, rcAllocating, rcSelf, rcNull );
    }
    else
    {
        wr = calloc( 1, sizeof *wr );
        if ( wr == NULL )
            rc = RC ( rcApp, rcNoTarg, rcAllocating, rcMemory, rcExhausted );
    }

    if ( rc == 0 )
    {
        rc = KDirectoryNativeDir( &dir );
        if ( rc == 0 )
        {
            KFile *of;
            rc = KDirectoryCreateFile ( dir, &of, false, 0664, kcmInit | kcmCreate, "%s", filename );
            if ( rc == 0 )
            {
                KFile* buf;
                rc = KBufFileMakeWrite( &buf, of, false, bufsize );
                if ( rc == 0 )
                {
                    wr->kfile = buf;
                    wr->org_writer = KOutWriterGet();
                    wr->org_data = KOutDataGet();
                    rc = KOutHandlerSet( stdout_redir_callback, wr );
                    if ( rc != 0 )
                        LOGERR( klogInt, rc, "KOutHandlerSet() failed" );
                }
                KFileRelease( of );
            }
            KDirectoryRelease( dir );
        }
    }

    if ( rc == 0 )
        *writer = wr;
    else
    {
        if ( wr != NULL )
            free( wr );
    }

    return rc;
}


void release_stdout_redirection( stdout_redir * writer )
{
    if ( writer != NULL )
    {
        KFileRelease( writer->kfile );
        if( writer->org_writer != NULL )
        {
            KOutHandlerSet( writer->org_writer, writer->org_data );
        }
        writer->org_writer = NULL;
        free( writer );
    }
}


/* =========================================================================================== */


rc_t make_trans_opt( trans_opt * opt, Args * args )
{
    uint32_t n1 = N_OFS, n2 = N_OFS;
    rc_t rc = get_str_option( args, OPTION_NAME, &opt->fname );
    if ( rc == 0 )
        rc = get_str_option( args, OPTION_REF, &opt->ref_name );
    if ( rc == 0 )
        rc = get_uint32_array( args, OPTION_RO, opt->ref_offset, &n1 );
    if ( rc == 0 )
        rc = get_uint32_array( args, OPTION_REFLEN, opt->ref_len, &n2 );
    opt->count = ( ( n1 > n2 ) ? n2 : n1 );
    return rc;
}


/* =========================================================================================== */


rc_t make_trans_ctx( trans_ctx * ctx, trans_opt * opt, bool open_reference )
{
    rc_t rc = AlignMgrMakeRead ( &ctx->almgr );
    if ( rc != 0 )
        LOGERR( klogInt, rc, "AlignMgrMake() failed" );

    if ( rc == 0 )
    {
        rc = KDirectoryNativeDir( &ctx->dir );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "KDirectoryNativeDir() failed" );
    }

    if ( rc == 0 )
    {
        rc = VDBManagerMakeRead ( &ctx->vdb_mgr, ctx->dir );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "VDBManagerMakeRead() failed" );
    }

    if ( rc == 0 )
    {
        if ( opt->fname != NULL )
        {
            rc = VDBManagerOpenDBRead ( ctx->vdb_mgr, &ctx->db, NULL, "%s", opt->fname );
            if ( rc != 0 )
                LOGERR( klogInt, rc, "VDBManagerOpenDBRead() failed" );
        }
        else
            rc = RC ( rcApp, rcArgv, rcAccessing, rcParam, rcNull );
    }

    if ( rc == 0 )
    {
        rc = ReferenceList_MakeDatabase( &ctx->ref_list, ctx->db,
                                         ereferencelist_4na |
                                         ereferencelist_usePrimaryIds,
                                         0, NULL, 0 );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "ReferenceList_MakeDatabase() failed" );
    }

    if ( rc == 0 && open_reference )
    {
        OUTMSG(( "looking for reference '%s'\n", opt->ref_name ));
        rc = ReferenceList_Find( ctx->ref_list, &ctx->ref_obj, opt->ref_name, string_size( opt->ref_name ) );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "ReferenceList_Find() failed" );
    }

    return rc;
}


void free_trans_ctx( trans_ctx * ctx )
{
    if ( ctx->dir != NULL ) KDirectoryRelease( ctx->dir );
    ctx->dir = NULL;
    if ( ctx->vdb_mgr != NULL ) VDBManagerRelease( ctx->vdb_mgr );
    ctx->vdb_mgr = NULL;
    if ( ctx->db != NULL ) VDatabaseRelease ( ctx->db );
    ctx->db = NULL;
    if ( ctx->ref_list != NULL ) ReferenceList_Release( ctx->ref_list );
    ctx->ref_list = NULL;
    if ( ctx->almgr != NULL ) AlignMgrRelease ( ctx->almgr );
    ctx->almgr = NULL;
}
