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

#include "curl-fetch.vers.h"

#include <kapp/main.h>
#include <kapp/args.h>
#include <klib/out.h>
#include <klib/log.h>
#include <klib/text.h>
#include <kfs/directory.h>
#include <kfs/file.h>
#include <klib/rc.h>
#include <sysalloc.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <kns/url-fetcher.h>
#include <kns/entrez-fetcher.h>

#define OPTION_URL "url"
#define DEFAULT_URL "http://eutils.ncbi.nlm.nih.gov/entrez/eutils/efetch.fcgi"

#define OPTION_SEQ_ID "seq_id"
#define DEFAULT_SEQ_ID "NC_000001"

#define OPTION_MAX_SEQLEN "max_seq_len"
#define DEFAULT_MAX_SEQLEN 1024

#define OPTION_ROW_ID "row_id"
#define DEFAULT_ROW_ID 1

#define OPTION_ROW_COUNT "row_count"
#define DEFAULT_ROW_COUNT 1

#define OPTION_OUTPUT "out"

#define OPTION_PROT "prot"

typedef struct fetch_context
{
    /* where to fetch from */
    const char *server;

    /* what to fetch */
    const char *seq_id;
    uint64_t max_seq_len;
    uint64_t row_id;
    uint64_t row_count;
    
    /* where to fetch to */
    const char *output;

    /* how to fetch */
    bool verbose;
} fetch_context;
typedef fetch_context* p_fetch_context;


const char UsageDefaultName[] = "curl-fetch";

rc_t CC UsageSummary ( const char * progname )
{
    return KOutMsg ("\n"
                    "Usage:\n"
                    "  %s <path> [<path> ...] [options]\n"
                    "\n", progname);
}

static const char * url_usage[]        = { "url = where to fetch from", NULL };
static const char * seq_id_usage[]     = { "what sequence to fetch", NULL };
static const char * seq_len_usage[]    = { "how big is the max. seq-len", NULL };
static const char * row_id_usage[]     = { "what row to fetch", NULL };
static const char * row_count_usage[]  = { "how many rows to fetch", NULL };
static const char * output_usage[]     = { "where to fetch to", NULL };
static const char * prot_usage[]       = { "show protocol", NULL };

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

    HelpOptionLine ( NULL, OPTION_URL, NULL, url_usage );
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
    return CURL_FETCH_VERS;
}


static void init_fetch_context( p_fetch_context ctx )
{
    if ( ctx )
    {
        ctx->server = NULL;
        ctx->seq_id = NULL;
        ctx->max_seq_len = 0;
        ctx->row_id = 0;
        ctx->output = NULL;
        ctx->verbose = false;
    }
}


static void free_context_ptr( char ** ptr )
{
    if ( *ptr )
    {
        free( *ptr );
        *ptr = NULL;
    }
}


static void free_fetch_context( p_fetch_context ctx )
{
    if ( ctx )
    {
        free_context_ptr( (char**)&( ctx->server ) );
        free_context_ptr( (char**)&( ctx->seq_id ) );
        free_context_ptr( (char**)&( ctx->output ) );
    }
}


static const char* get_str_option( const Args *my_args, const char *name )
{
    const char* res = NULL;
    uint32_t count;
    rc_t rc = ArgsOptionCount( my_args, name, &count );
    if ( ( rc == 0 )&&( count > 0 ) )
    {
        ArgsOptionValue( my_args, name, 0, &res );
    }
    return res;
}


static uint64_t get_int_option( const Args *my_args,
                                const char *name,
                                const uint64_t def )
{
    uint64_t res = def;
    uint32_t count;
    rc_t rc = ArgsOptionCount( my_args, name, &count );
    if ( ( rc == 0 )&&( count > 0 ) )
    {
        const char *s;
        rc = ArgsOptionValue( my_args, name, 0,  &s );
        if ( rc == 0 )
        {
            char *endp;
            res = strtoll( s, &endp, 10 );
        }
    }
    return res;
}

static bool get_bool_option( const Args *my_args,
                             const char *name,
                             const bool def )
{
    bool res = def;
    uint32_t count;
    rc_t rc = ArgsOptionCount( my_args, name, &count );
    if ( ( rc == 0 )&&( count > 0 ) )
        res = true;
    return res;
}


static void setup_fetch_context( const Args * args, p_fetch_context ctx )
{
    ctx->server = string_dup_measure ( get_str_option( args, OPTION_URL ), NULL );
    if ( ctx->server == NULL )
        ctx->server = string_dup_measure ( DEFAULT_URL, NULL );

    ctx->seq_id  = string_dup_measure ( get_str_option( args, OPTION_SEQ_ID ), NULL );
    if ( ctx->seq_id == NULL )
        ctx->seq_id = string_dup_measure ( DEFAULT_SEQ_ID, NULL );

    ctx->max_seq_len = get_int_option( args, OPTION_MAX_SEQLEN, DEFAULT_MAX_SEQLEN );
    ctx->row_id = get_int_option( args, OPTION_ROW_ID, DEFAULT_ROW_ID );
    ctx->row_count = get_int_option( args, OPTION_ROW_COUNT, DEFAULT_ROW_COUNT );
    
    ctx->output = string_dup_measure ( get_str_option( args, OPTION_OUTPUT ), NULL );
    ctx->verbose = get_bool_option( args, OPTION_PROT, false );
}


OptDef FetchOptions[] =
{
/*    name             alias  fkt.  usage-txt,      cnt, needs value, required */
    { OPTION_URL,        "u", NULL, url_usage,        1, true,  false },
    { OPTION_SEQ_ID,     "s", NULL, seq_id_usage,     1, true,  false },
    { OPTION_MAX_SEQLEN, "m", NULL, seq_len_usage,    1, true,  false },
    { OPTION_ROW_ID,     "r", NULL, row_id_usage,     1, true,  false },
    { OPTION_ROW_COUNT,  "c", NULL, row_count_usage , 1, true,  false },
    { OPTION_OUTPUT,     "o", NULL, output_usage,     1, true,  false },
    { OPTION_PROT,       "p", NULL, prot_usage,       1, false, false }
};


static rc_t write_to_file( const char * filename, const char *s, size_t len )
{
    rc_t rc;
    KDirectory *dir;
    
    rc = KDirectoryNativeDir ( &dir );
    if ( rc == 0 )
    {
        KFile *f;
        rc = KDirectoryCreateFile ( dir, &f, false, 0664, kcmCreate, filename );
        if ( rc == 0 )
        {
            size_t num_writ;
            rc = KFileWrite ( f, 0, s, len, &num_writ );
            KFileRelease ( f );
        }
        KDirectoryRelease ( dir );
    }
    return rc;
}

static void handle_buffer( const p_fetch_context ctx, char * buff, const size_t in_buff )
{
    if ( ctx->output )
        write_to_file( ctx->output, buff, in_buff );
    else
    {
        buff[ in_buff ] = 0;
        OUTMSG( ( buff ) );
    }
}


static rc_t perfrom_fetch( const p_fetch_context ctx )
{
    /* first make the url-fetcher, because it is neccessary to create the sra-fetcher... */
    KUrlFetcher *url_fetcher;
    rc_t rc = KUrlFetcherCurlMake( &url_fetcher, ctx->verbose );
    if ( rc == 0 )
    {
        /* then make the sra-fetcher, with the url-fetcher as parameter... */
        KEntrezFetcher *entrez_fetcher;
        rc = KEntrezFetcherMake( &entrez_fetcher, url_fetcher );
        if ( rc == 0 )
        {
            /* fill the paramters into the entrez-fechtcher and get the buffsize... */
            size_t buffsize;
            rc = KEntrezFetcherSetup ( entrez_fetcher,
                            ctx->server, ctx->seq_id, ctx->max_seq_len, 
                            ctx->row_id, ctx->row_count, &buffsize );
            if ( rc == 0 )
            {
                char * buff;
                buff = malloc( buffsize );
                if ( buff != NULL )
                {
                    size_t num_read;
                    /* perform the fetch-operation... */
                    rc = KEntrezFetcherRead ( entrez_fetcher, buff, buffsize, &num_read );
                    if ( rc == 0 )
                        handle_buffer( ctx, buff, num_read );
                    free( buff );
                }
            }
            KEntrezFetcherRelease( entrez_fetcher );
        }
        KUrlFetcherRelease( url_fetcher );
    }
    return rc;
}


rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;
    rc_t rc;

    rc = ArgsMakeAndHandle ( &args, argc, argv, 1,
                FetchOptions, sizeof ( FetchOptions ) / sizeof ( OptDef ) );
    if ( rc == 0 )
    {
        fetch_context ctx;
        
        /* make and fill with default values */
        init_fetch_context( &ctx );

        /* extract values from args */
        setup_fetch_context( args, &ctx );

        rc = perfrom_fetch( &ctx );
        
        free_fetch_context( &ctx );
        ArgsWhack ( args );
    }
    else
        OUTMSG( ( "ArgsMakeAndHandle() failed %R\n", rc ) );

    return rc;
}
