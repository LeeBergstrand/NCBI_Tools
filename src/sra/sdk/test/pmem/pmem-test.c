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

#include <klib/out.h>
#include <klib/debug.h>
#include <klib/rc.h>
#include <klib/container.h>

#include <kfs/directory.h>
#include <kfs/file.h>
#include <kfs/pagefile.h>
#include <kfs/pmem.h>

#include <kapp/main.h>
#include <klib/checksum.h>
#include <sysalloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <common/test_assert.h>

#if _PROFILING
#define CRC32( crc, buff, size ) \
    ( crc )
#endif


const char UsageDefaultName[] = "pmem-test";


rc_t CC UsageSummary ( const char * progname )
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        "  %s test-block-count\n"
        "\n", progname );
}


rc_t CC Usage ( const Args * args )
{
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
    rc_t rc;

    if (args == NULL)
        rc = RC (rcApp, rcArgv, rcAccessing, rcSelf, rcNull);
    else
        rc = ArgsProgram (args, &fullpath, &progname);

    UsageSummary (progname);

    KOutMsg ("Options:\n");

    HelpOptionsStandard();

    HelpVersion (fullpath, KAppVersion());

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
    return 0x1000000;
}

struct test_params
{
    char * temp_dir;

    uint64_t test_loops;
    size_t page_file_cache_limit;
    size_t membank_alloc_size;
    size_t membank_block_size;
    size_t membank_block_count;
    size_t membank_block_diff;
    uint64_t membank_limit;
    uint8_t release_factor;
    bool remove_backing_on_success;
};
typedef struct test_params test_params;

struct test_block
{
    SLNode head;
    uint64_t pmem_id;
    uint64_t pmem_size;
    uint32_t crc32;
    uint8_t used;
};
typedef struct test_block test_block;


struct mem_bank_ctx
{
    KMemBank * mem_bank;
    uint8_t * buffer;
    size_t buffer_size;
    size_t block_size;
    size_t block_diff;
    uint8_t release_factor;
    uint64_t handled;
};
typedef struct mem_bank_ctx mem_bank_ctx;


static
rc_t make_test_block( test_block ** tb, const uint64_t bytes )
{
    *tb = malloc( sizeof( **tb ) );
    if ( *tb == NULL )
        return RC ( rcExe, rcBuffer, rcAllocating, rcMemory, rcExhausted );
    (*tb)->pmem_size = bytes;
    (*tb)->used = 0;
    return 0;
}


static
void CC destroy_test_block( SLNode * n, void * data )
{
    test_block *tb = ( test_block * ) n;
    free ( tb );
}


static
bool CC block_writer( SLNode *n, void *data )
{
    test_block * tb = ( test_block * ) n;
    mem_bank_ctx * ctx = ( mem_bank_ctx * ) data;
    uint64_t i;
    size_t num_writ;
    rc_t rc;

    if ( tb->used == 1 )
        return false;

    tb->pmem_size = ctx->block_size;
    tb->pmem_size += ( rand() % ctx->block_diff );
    rc = KMemBankAlloc ( ctx->mem_bank, &( tb->pmem_id ), tb->pmem_size, false );
    if ( rc != 0 )
    {
        DBGMSG ( DBG_APP, -1, ( "KMemBankAlloc() = %R\n", rc ));
        return true;
    }

    for ( i = 0; i < tb->pmem_size; ++i )
        ctx->buffer[ i ] = ( rand() & 0x0FF );
    tb->crc32 = CRC32( 0, ctx->buffer, tb->pmem_size );

    rc = KMemBankWrite ( ctx->mem_bank, tb->pmem_id, 0, ctx->buffer, 
                        tb->pmem_size, &num_writ );
    if ( rc != 0 )
    {
        DBGMSG ( DBG_APP, -1, ( "KMemBankWrite() = %R\n", rc ));
        return true;
    }
    if ( tb->pmem_size != num_writ )
    {
        DBGMSG ( DBG_APP, -1, ( "KMemBankWrite() to_write != written\n" ));
        return true;
    }
    tb->used = 1;
    ctx->handled ++;
    return false;
}


static
bool CC block_releaser( SLNode *n, void *data )
{
    test_block * tb = ( test_block * ) n;
    mem_bank_ctx * ctx = ( mem_bank_ctx * ) data;
    rc_t rc;

    if ( tb->used == 0 )
    {
        DBGMSG ( DBG_APP, -1, ( "block_releaser() called on unused block!\n" ));
        return true;
    }

    /* release only the n-th entry... */
    if ( ( rand() % ctx->release_factor ) != 0 )
        return false;

    rc = KMemBankFree ( ctx->mem_bank, tb->pmem_id );
    if ( rc != 0 )
    {
        DBGMSG ( DBG_APP, -1, ( "KMemBankFree() = %R\n", rc ));
        return true;
    }
    ctx->handled ++;
    tb->used = 0;
    return false;
}

static
bool CC block_reader( SLNode *n, void *data )
{
    test_block * tb = ( test_block * ) n;
    mem_bank_ctx * ctx = ( mem_bank_ctx * ) data;
    uint64_t banksize;
    size_t num_read;
    uint32_t read_crc;
    rc_t rc;

    if ( tb->used == 0 )
        return false;

    rc = KMemBankSize ( ctx->mem_bank, tb->pmem_id, &banksize );
    if ( rc != 0 )
    {
        DBGMSG ( DBG_APP, -1, ( "KMemBankSize() = %R\n", rc ));
        return true;
    }
    if ( banksize != tb->pmem_size )
    {
        DBGMSG ( DBG_APP, -1, ( "KMemBankSize() != written size\n" ));
        return true;
    }

    rc = KMemBankRead ( ctx->mem_bank, tb->pmem_id, 0, ctx->buffer, tb->pmem_size, &num_read );
    if ( rc != 0 )
    {
        DBGMSG ( DBG_APP, -1, ( "KMemBankRead() = %R\n", rc ));
        return true;
    }

    read_crc = CRC32( 0, ctx->buffer, num_read );
    if ( read_crc != tb->crc32 )
        DBGMSG ( DBG_APP, -1, ( "crc's different on read!\n" ));

    ctx->handled ++;
    return false;
}

#if _PROFILING
#define REPORT( x ) \
    ( void ) 0
#else
#define REPORT( x ) \
    OUTMSG ( x )
#endif

static
rc_t test_loop( KMemBank * mem_bank, struct test_params * params )
{
    SLList blocks;
    mem_bank_ctx ctx;
    uint64_t i;
    rc_t rc = 0;
    bool res;
    
    ctx.buffer = malloc( params->membank_block_size * 2 );
    if ( ctx.buffer == NULL )
        return RC( rcExe, rcBuffer, rcAllocating, rcMemory, rcExhausted );
    ctx.buffer_size = params->membank_block_size * 2;
    ctx.block_size = params->membank_block_size;
    ctx.release_factor = params->release_factor;
    ctx.block_diff = params->membank_block_diff;
    ctx.mem_bank = mem_bank;
    SLListInit( &blocks );

    /* construct a list of blocks to be inserted into the mem_bank */
    for ( i = 0; i < params->membank_block_count && rc == 0; ++i )
    {
        test_block * block;
        rc = make_test_block( &block, params->membank_block_size );
        if ( rc == 0 )
            SLListPushTail ( &blocks, ( SLNode * )block );
    }

    /* allocate and write all the blocks */
    ctx.handled = 0;
    res = SLListDoUntil ( &blocks, block_writer, &ctx );
    REPORT (( "%lu blocks allocated\n", ctx.handled ));
    if ( res )
        return RC( rcExe, rcFunction, rcVisiting, rcItem, rcInvalid );

    /* read the blocks and compare... */
    ctx.handled = 0;
    res = SLListDoUntil ( &blocks, block_reader, &ctx );
    REPORT (( "%lu blocks checked\n", ctx.handled ));
    if ( res )
        return RC( rcExe, rcFunction, rcVisiting, rcItem, rcInvalid );

    if ( params->release_factor > 0 )
        for ( i = 0; i < params->test_loops && rc == 0; ++i )
        {
            REPORT (( "---------test-loop #%lu :\n", i+1 ));
            /* release some blocks ... */
            ctx.handled = 0;
            res = SLListDoUntil ( &blocks, block_releaser, &ctx );
            REPORT (( "%lu blocks released\n", ctx.handled ));
            if ( res )
                return RC( rcExe, rcFunction, rcVisiting, rcItem, rcInvalid );

            /* read the blocks and compare... */
            ctx.handled = 0;
            res = SLListDoUntil ( &blocks, block_reader, &ctx );
            REPORT (( "%lu blocks checked\n", ctx.handled ));
            if ( res )
                return RC( rcExe, rcFunction, rcVisiting, rcItem, rcInvalid );

            /* allocate and write the blocks */
            ctx.handled = 0;
            res = SLListDoUntil ( &blocks, block_writer, &ctx );
            REPORT (( "%lu blocks allocated\n", ctx.handled ));
            if ( res )
                return RC( rcExe, rcFunction, rcVisiting, rcItem, rcInvalid );

            /* read the blocks and compare... */
            ctx.handled = 0;
            res = SLListDoUntil ( &blocks, block_reader, &ctx );
            REPORT (( "%lu blocks checked\n", ctx.handled ));
            if ( res )
                return RC( rcExe, rcFunction, rcVisiting, rcItem, rcInvalid );
        }

    /* destroy the blocks... */
    SLListWhack ( &blocks, destroy_test_block, NULL );
    free( ctx.buffer );

    return 0;
}


static
rc_t test_file( KFile * temp_file, struct test_params * params )
{
    rc_t rc;
    KPageFile * page_file;
    KMemBank * mem_bank;
                
    rc = KPageFileMakeUpdate( &page_file, temp_file, params->page_file_cache_limit, 0 );
    if ( rc != 0 )
    {
        DBGMSG ( DBG_APP, -1, ( "KPageFileMakeUpdate() = %R\n", rc ));
        return rc;
    }

    rc = KMemBankMake( &mem_bank, params->membank_alloc_size, params->membank_limit, page_file );
    if ( rc == 0 )
    {
        rc = test_loop( mem_bank, params );
        if ( rc == 0 )
            OUTMSG (( "test_loop success\n" ));
        else
            OUTMSG (( "test_loop error = %R\n", rc ));
        KMemBankRelease ( mem_bank );
    }
    else
        DBGMSG ( DBG_APP, -1, ( "KMemBankMake() = %R\n", rc ));

    KPageFileRelease( page_file );
    return rc;
}


static
rc_t test_in_directory( KDirectory * dir, test_params * params )
{
    rc_t rc;
    char filename[ 32 ];
    KFile * temp_file;

    strcpy( filename, "pmem-test-file.pm" );
    rc = KDirectoryCreateFile( dir, &temp_file, true, 0600, kcmInit, filename );
    if ( rc == 0 )
    {
        rc = test_file( temp_file, params );
        KFileRelease( temp_file );
        if ( rc == 0 && params->remove_backing_on_success )
        {
            rc_t rc1 = KDirectoryRemove ( dir, true, filename );
            if ( rc1 != 0 )
                DBGMSG ( DBG_APP, -1, ( "cannot remove backing-file : %R\n", rc ));
        }
    }
    else
        DBGMSG ( DBG_APP, -1, ( "KDirectoryCreateFile() = %R\n", rc ));
    return rc;
}

static
rc_t test_main( test_params * params )
{
    rc_t rc;
    KDirectory * native_dir;
    KDirectory * temp_dir;

    rc = KDirectoryNativeDir ( & native_dir );
    if ( rc != 0 )
    {
        DBGMSG ( DBG_APP, -1, ( "KDirectoryNativeDir() = %R\n", rc ));
        return rc;
    }
    if ( params->temp_dir != NULL )
    {
        rc = KDirectoryOpenDirUpdate ( native_dir, &temp_dir, false, params->temp_dir );
        if ( rc != 0 )
            DBGMSG ( DBG_APP, -1, ( "KDirectoryOpenDirUpdate('%s') = %R\n", params->temp_dir, rc ));
        if ( rc ==0 )
        {
            rc = test_in_directory( temp_dir, params );
            KDirectoryRelease( temp_dir );
        }
    }
    else
        rc = test_in_directory( native_dir, params );

    KDirectoryRelease( native_dir );
    return rc;
}


rc_t CC KMain ( int argc, char *argv [] )
{
    struct test_params params;

    if ( argc > 1 )
        params.temp_dir = argv[1];
    else
        params.temp_dir = NULL;

#if _PROFILING
    params.test_loops  = 2000;
#else
    params.test_loops  = 20;
#endif
    params.membank_block_count = 10000;
    params.page_file_cache_limit = 1024 * 1024;
    params.membank_alloc_size = 64;
    params.membank_block_size = 128;
    params.membank_limit = 0;
    params.release_factor = 4;
    params.membank_block_diff = 20;
    params.remove_backing_on_success = true;

    srand( clock() );

    return test_main( &params );
}

