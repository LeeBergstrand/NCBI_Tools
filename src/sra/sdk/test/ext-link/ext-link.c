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

#include "ext-link.vers.h"
#include <kapp/main.h>
#include <kapp/args.h>
#include <klib/out.h>
#include <klib/log.h>
#include <klib/rc.h>
#include <zlib.h>
#include <bzlib.h>
#include <sysalloc.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <common/test_assert.h>

const char UsageDefaultName[] = "ext-link";
#define def_name UsageDefaultName

rc_t CC UsageSummary ( const char * progname )
{
    OUTMSG ( ("\n"
        "Usage:\n"
        "< no parameters >\n"
        "The purpose of this binary is to test that linking against the compression-libs succeeds.\n"
        "\n", progname) );
    return 0;
}

/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
ver_t CC KAppVersion ( void )
{
    return EXT_LINK_VERS;
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

const char * s_Z_OK            = "Z_OK";
const char * s_Z_STREAM_END    = "Z_STREAM_END";
const char * s_Z_NEED_DICT     = "Z_NEED_DICT";
const char * s_Z_ERRNO         = "Z_ERRNO";
const char * s_Z_STREAM_ERROR  = "Z_STREAM_ERROR";
const char * s_Z_DATA_ERROR    = "Z_DATA_ERROR";
const char * s_Z_MEM_ERROR     = "Z_MEM_ERROR";
const char * s_Z_BUF_ERROR     = "Z_BUF_ERROR";
const char * s_Z_VERSION_ERROR = "Z_VERSION_ERROR";
const char * s_Z_UNKOWN        = "UNKNOWN ERROR";

const char * z_res_2_str( const int z_res )
{
    switch ( z_res )
    {
    case Z_OK            : return s_Z_OK; break;
    case Z_STREAM_END    : return s_Z_STREAM_END; break;
    case Z_NEED_DICT     : return s_Z_NEED_DICT; break;
    case Z_ERRNO         : return s_Z_ERRNO; break;
    case Z_STREAM_ERROR  : return s_Z_STREAM_ERROR; break;
    case Z_DATA_ERROR    : return s_Z_DATA_ERROR; break;
    case Z_MEM_ERROR     : return s_Z_MEM_ERROR; break;
    case Z_BUF_ERROR     : return s_Z_BUF_ERROR; break;
    case Z_VERSION_ERROR : return s_Z_VERSION_ERROR; break;
    }
    return s_Z_UNKOWN;
}

void fill_buffer_random( char * buf, const size_t n )
{
    size_t i;

    srand ( 732 );
    for ( i = 0; i < n; ++i )
        buf[ i ] = ( rand() % 256 );
}

/*
    performing a test z-lib compression and decompression
    checks if the decompressed data is the same as befor the compression
*/
rc_t perform_test_compression( void )
{
    z_stream strm;
    int z_res;
    char src_buf[2048];
    char deflated[4096];
    uInt deflated_n;
    char inflated[4096];

    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;

    z_res = deflateInit ( &strm, Z_DEFAULT_COMPRESSION );
    OUTMSG (( "zlib deflateInit() -> %s\n", z_res_2_str( z_res ) ));
    if ( z_res != Z_OK )
        return RC( rcExe, rcCondition, rcValidating, rcInterface, rcInvalid );

    fill_buffer_random ( src_buf, sizeof src_buf );

    strm.next_in   = (Bytef*)src_buf;
    strm.avail_in  = sizeof src_buf;
    strm.next_out  = (Bytef*)deflated;
    strm.avail_out = sizeof deflated;

    z_res = deflate ( &strm, Z_FINISH );
    OUTMSG (( "zlib deflate() -> %s\n", z_res_2_str( z_res ) ));
    OUTMSG (( "%u bytes consumed, %u bytes produced\n", strm.total_in, strm.total_out ));
    deflated_n = (uInt)strm.total_out;

    z_res = deflateEnd ( &strm );
    OUTMSG (( "zlib deflateEnd() -> %s\n", z_res_2_str( z_res ) ));
    if ( z_res != Z_OK )
        return RC( rcExe, rcCondition, rcValidating, rcInterface, rcInvalid );

    z_res = inflateInit ( &strm );
    OUTMSG (( "zlib inflateInit() -> %s\n", z_res_2_str( z_res ) ));
    if ( z_res != Z_OK )
        return RC( rcExe, rcCondition, rcValidating, rcInterface, rcInvalid );

    strm.next_in   = (Bytef*)deflated;
    strm.avail_in  = deflated_n;
    strm.next_out  = (Bytef*)inflated;
    strm.avail_out = sizeof inflated;

    z_res = inflate ( &strm, Z_FINISH );
    OUTMSG (( "zlib inflate() -> %s\n", z_res_2_str( z_res ) ));
    OUTMSG (( "%u bytes consumed, %u bytes produced\n", strm.total_in, strm.total_out ));

    if ( sizeof src_buf != strm.total_out )
        OUTMSG (( "size difference between deflate and inflate = %d\n", strm.total_out - deflated_n ));
    else
    {
        if ( memcmp ( src_buf, inflated, sizeof src_buf ) != 0 )
            OUTMSG (( "content differs between deflate and inflate\n" ));
        else
            OUTMSG (( "correct deflate/inflate pair\n" ));
    }

    z_res = inflateEnd ( &strm );
    OUTMSG (( "zlib inflateEnd() -> %s\n", z_res_2_str( z_res ) ));
    if ( z_res != Z_OK )
        return RC( rcExe, rcCondition, rcValidating, rcInterface, rcInvalid );

    return 0;
}


rc_t call_zlib( void )
{
    const char * zlib_vers;

    zlib_vers = zlibVersion();
    if ( zlib_vers == NULL )
        return RC( rcExe, rcCondition, rcValidating, rcInterface, rcInvalid );
    OUTMSG (( "zlib version is : '%s'\n", zlib_vers ));

    return 0;
}


rc_t call_bz2( void )
{
    const char * bz2_vers = NULL;

    bz2_vers = BZ2_bzlibVersion();
    if ( bz2_vers == NULL )
        return RC( rcExe, rcCondition, rcValidating, rcInterface, rcInvalid );
    OUTMSG (( "bz2 version is : '%s'\n", bz2_vers ));

    return 0;
}


rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;
    rc_t rc;

    rc = ArgsMakeAndHandle ( &args, argc, argv, 1, NULL, 0 );
    if ( rc == 0 )
    {
        rc = call_zlib();
        OUTMSG (( "calling zlib : %R\n", rc ));

        rc = call_bz2();
        OUTMSG (( "calling bz2 : %R\n", rc ));

        ArgsWhack ( args );
    }
    else
        OUTMSG( ( "ArgsMakeAndHandle() failed %R\n", rc ) );

    return rc;
}
