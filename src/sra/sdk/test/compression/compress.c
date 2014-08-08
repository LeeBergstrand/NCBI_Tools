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
#include <klib/out.h>
#include <klib/log.h>
#include <klib/text.h>
#include <klib/rc.h>
#include <klib/namelist.h>
#include <klib/printf.h>
#include <kfs/directory.h>
#include <kfs/file.h>
#include <kfs/gzip.h>
#include <kfs/bzip.h>
#include <kfs/szip.h>
#include <kfs/defs.h>
#include <sysalloc.h>

#include "definitions.h"
#include "kcompress.vers.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define OPTION_METHOD    "method"
#define ALIAS_METHOD     "m"

#define OPTION_DIRECTION "direction"
#define ALIAS_DIRECTION  "d"

static const char * method_usage[]     = { "method    zip|bzip|szip", NULL };
static const char * direction_usage[]  = { "direction c(ompress)|d(decompress))", NULL };


OptDef CompressOptions[] =
{
/*    name             alias  fkt.  usage-txt,      cnt, needs value, required */
    { OPTION_METHOD,    "m", NULL, method_usage,     1, true,  false },
    { OPTION_DIRECTION, "d", NULL, direction_usage,  1, true,  false }
};

const char UsageDefaultName[] = "kcompress";

rc_t CC UsageSummary ( const char * progname )
{
    return KOutMsg ("\n"
                    "Usage:\n"
                    "  %s src-path dst-path [options]\n"
                    "\n", progname);
    return 0;
}


rc_t CC Usage ( const Args * args )
{
    const char * progname;
    const char * fullpath;
    rc_t rc;

    if ( args == NULL )
        rc = RC ( rcApp, rcArgv, rcAccessing, rcSelf, rcNull );
    else
        rc = ArgsProgram ( args, &fullpath, &progname );
    if ( rc )
        progname = fullpath = UsageDefaultName;

    UsageSummary ( progname );

    OUTMSG (( "Options:\n" ));
    HelpOptionLine ( ALIAS_METHOD, OPTION_METHOD, "method", method_usage );
    HelpOptionLine ( ALIAS_DIRECTION, OPTION_DIRECTION, "direction", direction_usage );

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
    return KCOMPRESS_VERS;
}


typedef struct compress_context
{
    /* where to fetch from */
    const char *src_file;
    const char *dst_file;
    uint8_t method;
    uint8_t direction;
} compress_context;
typedef compress_context* p_compress_context;


static void init_compress_context( p_compress_context ctx )
{
    if ( ctx )
    {
        ctx->src_file = NULL;
        ctx->dst_file = NULL;
        ctx->method = 0;
        ctx->direction = 0;
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


static void free_compress_context( p_compress_context ctx )
{
    if ( ctx )
    {
        free_context_ptr( (char**)&( ctx->src_file ) );
        free_context_ptr( (char**)&( ctx->dst_file ) );
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


static void setup_compress_context( const Args * args, p_compress_context ctx )
{
    uint32_t count;
    const char *value = NULL;
    rc_t rc = ArgsParamCount( args, &count );
    DISP_RC( rc, "ArgsParamCount() failed" );
    if ( rc == 0 )
    {
        rc = ArgsParamValue( args, 0, &value );
        DISP_RC( rc, "ArgsParamValue() failed" );
        if ( rc == 0 )
            ctx->src_file = string_dup_measure( value, NULL );

        rc = ArgsParamValue( args, 1, &value );
        DISP_RC( rc, "ArgsParamValue() failed" );
        if ( rc == 0 )
            ctx->dst_file = string_dup_measure( value, NULL );
    }

    value = get_str_option( args, OPTION_DIRECTION );
    if ( value )
    {
        switch ( value[0] )
        {
        case 'c' :
        case 'C' : ctx->direction = KC_COMPRESS; break;
        case 'd' :
        case 'D' : ctx->direction = KC_DECOMPRESS; break;
        }
    }
    if ( ctx->direction == 0 )
        ctx->direction = KC_DECOMPRESS;

    value = get_str_option( args, OPTION_METHOD );
    if ( value )
    {
        switch ( value[0] )
        {
        case 'g' :
        case 'G' : ctx->method = KC_GZIP; break;
        case 'b' :
        case 'B' : ctx->method = KC_BZIP; break;
        case 's' :
        case 'S' : ctx->method = KC_SZIP; break;
        }
    }
    if ( ctx->method == 0 )
        ctx->method = KC_GZIP;
}



static rc_t check_context( const compress_context * ctx )
{
    OUTMSG (( "from   : %s\n", ctx->src_file ));
    OUTMSG (( "to     : %s\n", ctx->dst_file ));
    OUTMSG (( "method : " ));
    switch( ctx->method )
    {
    case KC_GZIP : OUTMSG (( "GZIP\n" )); break;
    case KC_BZIP : OUTMSG (( "BZIP\n" )); break;
    case KC_SZIP : OUTMSG (( "SZIP\n" )); break;
    default      : OUTMSG (( "?\n" )); break;
    }
    OUTMSG (( "direct.: " ));
    if ( ctx->method == 0 )
        return RC( rcExe, rcFile, rcPacking, rcFormat, rcUnknown );

    switch( ctx->direction )
    {
    case KC_COMPRESS  : OUTMSG (( "COMPRESS\n" )); break;
    case KC_DECOMPRESS: OUTMSG (( "DECOMPRESS\n" )); break;
    default           : OUTMSG (( "?\n" )); break;
    }
    if ( ctx->direction == 0 )
        return RC( rcExe, rcFile, rcPacking, rcFormat, rcUnknown );

    return 0;
}


static rc_t prepare_compress( const compress_context * ctx,
                              KDirectory *dir, const KFile **src, KFile **dst )
{
    KFile *temp = NULL;

    rc_t rc = KDirectoryOpenFileRead ( dir, src, ctx->src_file );
    DISP_RC( rc, "KDirectoryOpenFileRead() failed" );
    if ( rc != 0 ) return rc;

    rc = KDirectoryCreateFile ( dir, &temp, false, 0664, kcmInit, ctx->dst_file );
    DISP_RC( rc, "KDirectoryCreateFile() failed" );
    if ( rc == 0 )
        switch( ctx->method )
        {
        case KC_GZIP : rc = KFileMakeGzipForWrite ( dst, temp );
                       DISP_RC( rc, "KFileMakeGzipForWrite() failed" );
                       break;
        case KC_BZIP : rc = KFileMakeBzip2ForWrite ( dst, temp );
                       DISP_RC( rc, "KFileMakeBzip2ForWrite() failed" );
                       break;
        case KC_SZIP : /* rc = KFileMakeSzipForWrite ( dst, temp ); */
                       rc = RC( rcExe, rcFile, rcPacking, rcFormat, rcUnknown );
                       DISP_RC( rc, "KFileMakeSzipForWrite() failed" );
                       break;
        }
    KFileRelease ( temp );
    return rc;
}


static rc_t prepare_decompress( const compress_context * ctx,
                                KDirectory *dir, const KFile **src, KFile **dst )
{
    const KFile *temp = NULL;

    rc_t rc = KDirectoryCreateFile ( dir, dst, false, 0664, kcmInit, ctx->dst_file );
    DISP_RC( rc, "KDirectoryCreateFile() failed" );
    if ( rc != 0 ) return rc;

    rc = KDirectoryOpenFileRead ( dir, &temp, ctx->src_file );
    DISP_RC( rc, "KDirectoryOpenFileRead() failed" );
    if ( rc == 0 )
        switch( ctx->method )
        {
        case KC_GZIP : rc = KFileMakeGzipForRead ( src, temp );
                       DISP_RC( rc, "KFileMakeGzipForRead() failed" );
                       break;
        case KC_BZIP : rc = KFileMakeBzip2ForRead ( src, temp );
                       DISP_RC( rc, "KFileMakeBzip2ForRead() failed" );
                       break;
        case KC_SZIP : /* rc = KFileMakeSzipForRead ( src, temp ); */
                       rc = RC( rcExe, rcFile, rcPacking, rcFormat, rcUnknown );
                       DISP_RC( rc, "KFileMakeSzip2ForRead() failed" );
                       break;
        }
    KFileRelease ( temp );
    return rc;
}

static rc_t compress_loop( const KFile *src, KFile *dst )
{
    rc_t rc = 0;
    uint64_t pos = 0;
    size_t bsize = 4096;
    size_t num_read = 1;

    char * buffer = malloc( bsize );
    if ( buffer == NULL )
        return RC( rcExe, rcFile, rcPacking, rcMemory, rcExhausted );

    while ( rc == 0 && num_read > 0 )
    {
        rc = KFileRead ( src, pos, buffer, bsize, &num_read );
        DISP_RC( rc, "KFileRead() failed" );
        if ( rc == 0 && num_read > 0 )
        {
            size_t num_writ;
            rc = KFileWrite ( dst, pos, buffer, num_read, &num_writ );
            DISP_RC( rc, "KFilewrite() failed" );
            pos += num_read;
        }
    }
    OUTMSG (( "%lu bytes copied\n", pos ));
    free( buffer );
    return rc;
}


static rc_t perform_compress( const compress_context * ctx )
{
    KDirectory *dir = NULL;
    const KFile *src = NULL;
    KFile *dst = NULL;

    rc_t rc = KDirectoryNativeDir( &dir );
    DISP_RC( rc, "KDirectoryNativeDir() failed" );
    if ( rc != 0 ) return rc;

    if ( ctx->direction == KC_COMPRESS )
        rc = prepare_compress( ctx, dir, &src, &dst );
    else
        rc = prepare_decompress( ctx, dir, &src, &dst );

    if ( rc == 0 )
        rc = compress_loop( src, dst );

    KFileRelease ( src );
    KFileRelease ( dst );
    return rc;
}


static rc_t compress_main( Args * args )
{
    rc_t rc;
    compress_context ctx;
    
    /* make and fill with default values */
    init_compress_context( &ctx );

    /* extract values from args */
    setup_compress_context( args, &ctx );

    rc = check_context( &ctx );
    DISP_RC( rc, "check_context() failed" );
    if ( rc == 0 )
        rc = perform_compress( &ctx );
    else
        MiniUsage( args );
    
    free_compress_context( &ctx );
    return rc;
}


static rc_t copy2_main( Args * args )
{
    const char * source_path;
    rc_t rc = ArgsParamValue( args, 0, &source_path );
    if ( rc != 0 )
        LOGERR( klogInt, rc, "ArgsParamValue( 0 ) failed" );
    else
    {
        const char * dest_path;
        rc = ArgsParamValue( args, 1, &dest_path );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "ArgsParamValue( 1 ) failed" );
        else
        {
            KDirectory *dir, *dest_dir;
            const KDirectory *source_dir;
            rc = KDirectoryNativeDir ( &dir );
            if ( rc != 0 )
                LOGERR( klogInt, rc, "KDirectoryNativeDir() failed" );
            else
            {
                rc = KDirectoryOpenDirRead( dir, &source_dir, false,
                                            source_path );
                if ( rc != 0 )
                    LOGERR( klogInt, rc, "KDirectoryOpenDirRead() failed" );
                else
                {
                    rc = KDirectoryOpenDirUpdate( dir, &dest_dir, false,
                                            dest_path );
                    if ( rc != 0 )
                        LOGERR( klogInt, rc,
                                "KDirectoryOpenDirUpdate() failed" );
                    else
                    {
                        rc = KDirectoryCopy( source_dir, dest_dir, true,
                                             source_path, dest_path );
                        if ( rc != 0 )
                            LOGERR( klogInt, rc, "copy_dirs() failed" );
                        else
                            OUTMSG(( "copy successful!\n" ));
                        KDirectoryRelease ( dest_dir );
                    }
                    KDirectoryRelease ( source_dir );
                }
                KDirectoryRelease ( dir );
            }
        }
    }
    return rc;
}


KFS_EXTERN rc_t CC KDirectoryList ( const KDirectory *self, struct KNamelist **list,
    bool ( CC * f ) ( const KDirectory *dir, const char *name, void *data ),
    void *data, const char *path, ... );


rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;

    rc_t rc = ArgsMakeAndHandle ( &args, argc, argv, 1,
                CompressOptions, sizeof ( CompressOptions ) / sizeof ( OptDef ) );
    if ( rc == 0 )
    {
/*        rc = compress_main( args ); */
        rc = copy2_main( args );

        ArgsWhack ( args );
    }
    else
        OUTMSG( ( "ArgsMakeAndHandle() failed %R\n", rc ) );

    return rc;
}
