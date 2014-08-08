#include "tracedata.h"
#include "tracecmp.h"

#if _DEBUGGING
#include "logging.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#if TRACEDB_USING_GZIP
/*--------------------------------------------------------------------------
 * Gunzip
 *  decompression processor
 */

/* GunzipInit
 */
static
int tracedb_GunzipInit ( tracedb_Gunzip *guz )
{
    guz -> zalloc = NULL;
    guz -> zfree = NULL;
    guz -> opaque = NULL;

    return inflateInit ( guz );
}

static
int tracedb_GunzipDecompress ( tracedb_Gunzip *guz, void *dst, size_t *dst_sz,
    const void *src, size_t src_sz )
{
    int status;

    /* input parameters should be const. API says otherwise */
    guz -> next_in = ( void* ) src;
    guz -> avail_in = ( unsigned int ) src_sz;

    guz -> next_out = dst;
    guz -> avail_out = ( unsigned int ) * dst_sz;

    while ( 1 )
    {
        status = inflate ( guz, Z_FINISH );
        if ( status == Z_STREAM_END )
            break;

        switch ( status )
        {
        case Z_MEM_ERROR:
            return ENOMEM;
        case Z_BUF_ERROR:
            return ENOBUFS;
        case Z_OK:
            break;
        default:
            return -1;
        }
    }

    * dst_sz = guz -> total_out;

    return 0;
}

#define tracedb_GunzipReset( guz ) \
    inflateReset ( guz )

#define tracedb_GunzipWhack( guz ) \
    inflateEnd ( guz )

static
int tracedb_gunzip ( const TraceDataDecompression *ddc,
    void *dst, size_t *dst_sz, const void *src, size_t src_sz )
{
    if ( src_sz != 0 )
    {
        int status;
        TraceDataDecompression *dd = ( TraceDataDecompression* ) ddc;
        if ( tracedb_atomic32_test_and_set ( & dd -> guz_refcount, 1, 0 ) == 0 )
        {
            status = tracedb_GunzipDecompress ( & dd -> guz,
                dst, dst_sz, src, src_sz );
            tracedb_GunzipReset ( & dd -> guz );
            tracedb_atomic32_set ( & dd -> guz_refcount, 0 );
        }
        else
        {
            tracedb_Gunzip guz;
            status = tracedb_GunzipInit ( & guz );
            if ( status == Z_OK )
            {
                status = tracedb_GunzipDecompress ( & guz, dst, dst_sz, src, src_sz );
                tracedb_GunzipWhack ( & guz );
            }
        }
        return status;
    }
    * dst_sz = 0;
    return Z_OK;
}
#endif /* USING_GZIP */


#if TRACEDB_USING_BZIP2

#include <bzlib.h>

/*--------------------------------------------------------------------------
 * Bunzip2
 *  decompression processor
 */
typedef bz_stream tracedb_Bunzip2;

/* Bunzip2Init
 *  initialize for decompression
 */
static
int tracedb_Bunzip2Init ( tracedb_Bunzip2 *bz )
{
    bz -> bzalloc = NULL;
    bz -> bzfree = NULL;
    bz -> opaque = NULL;

    return BZ2_bzDecompressInit ( bz, 0, 0 );
}

/* Bunzip2Decompress
 */
static
int tracedb_Bunzip2Decompress ( tracedb_Bunzip2 *bz,
    void *dst, size_t *dst_len, const void *src, size_t src_len )
{
    int status;

    bz -> next_in = ( void* ) src;
    bz -> avail_in = ( unsigned int ) src_len;

    bz -> next_out = dst;
    bz -> avail_out = ( unsigned int ) * dst_len;

    while ( 1 )
    {
        status = BZ2_bzDecompress ( bz );
        if ( status == BZ_STREAM_END )
            break;

        if ( status < 0 ) switch ( status )
        {
        case BZ_MEM_ERROR:
            return ENOMEM;
        case BZ_OUTBUFF_FULL:
            return ENOBUFS;
        default:
            return -1;
        }

        if ( bz -> avail_out == 0 )
            return ENOBUFS;

        if ( bz -> avail_in == 0 )
            return -1;
    }

    if ( sizeof * dst_len == 8 )
        * dst_len = ( ( size_t ) bz -> total_out_hi32 << 32 ) + bz -> total_out_lo32;
    else
        * dst_len = bz -> total_out_lo32;

    return 0;
}

/* Bunzip2Whack
 */
#define tracedb_Bunzip2Whack( bz ) \
    BZ2_bzDecompressEnd ( bz )

/* bunzip
 *  run Bunzip2 on guy
 */
static
int tracedb_bunzip ( void *dst, size_t *dst_sz,
        const void *src, size_t src_sz )
{
    if ( src_sz != 0 )
    {
        Bunzip2 bz;
        int status = tracedb_Bunzip2Init ( & bz );
        if ( status == BZ_OK )
        {
            status = tracedb_Bunzip2Decompress ( & bz, dst, dst_sz, src, src_sz );
            tracedb_Bunzip2Whack ( & bz );
        }
        return status;
    }
    * dst_sz = 0;
    return BZ_OK;
}
#endif /* USING_BZIP2 */


/*--------------------------------------------------------------------------
 * TraceData
 *  structure for retrieval of data blob
 */

/* TraceDataGetBasecall
 *  returns a pointer to the NULL terminated basecall sequence
 *
 *  uncompressed return type is const char*
 */
const void *TraceDataGetBasecall ( const TraceData *td, size_t *sizep )
{
    size_t size;
    const void *col;
    unsigned int num_cols;

    if ( td == NULL )
        return NULL;

    num_cols = td -> extended_cmp != tracedb_extNULL ? 5 : 4;

    switch ( td -> size_var )
    {
    case 0:
        col = & td -> col_sizes . v8 [ num_cols ];
        size = td -> col_sizes . v8 [ 0 ];
        break;
    case 1:
        col = & td -> col_sizes . v16 [ num_cols ];
        size = td -> col_sizes . v16 [ 0 ];
        break;
    default:
        col = & td -> col_sizes . v32 [ num_cols ];
        size = td -> col_sizes . v32 [ 0 ];
    }

    if ( sizep != NULL )
        * sizep = size;
    return col;
}

/* TraceDataGetQualscore
 *  returns a pointer to the qualscore sequence
 *
 *  uncompressed return type is const uint8_t*
 */
const void *TraceDataGetQualscore ( const TraceData *td, size_t *sizep )
{
    size_t size;
    const char *col = TraceDataGetBasecall ( td, & size );

    if ( col == NULL )
        return NULL;

    col += size;

    switch ( td -> size_var )
    {
    case 0:
        size = td -> col_sizes . v8 [ 1 ];
        break;
    case 1:
        size = td -> col_sizes . v16 [ 1 ];
        break;
    default:
        size = td -> col_sizes . v32 [ 1 ];
    }

    if ( sizep != NULL )
        * sizep = size;
    return col;
}

/* TraceDataGetPeakindex
 *  returns a pointer to the peakindex sequence
 *
 *  the peaks are real indices, not deltas
 *
 *  uncompressed return type is const uint32_t*
 */
const void *TraceDataGetPeakindex ( const TraceData *td, size_t *sizep )
{
    size_t size;
    const char *col = TraceDataGetQualscore ( td, & size );

    if ( col == NULL )
        return NULL;

    col += size;

    switch ( td -> size_var )
    {
    case 0:
        size = td -> col_sizes . v8 [ 2 ];
        break;
    case 1:
        size = td -> col_sizes . v16 [ 2 ];
        break;
    default:
        size = td -> col_sizes . v32 [ 2 ];
    }

    if ( size > 0 ) switch ( td -> peakindex_cmp )
    {
    case tracedb_piNone:
    case tracedb_piDelta4:
        if ( ( ( size_t ) col & 3 ) != 0 )
            col = ( const char* ) ( ( size_t ) col | 3 ) + 1;
        break;
    }

    if ( sizep != NULL )
        * sizep = size;
    return col;
}

/* TraceDataGetComment
 *  returns a pointer to the comment sequence
 *
 *  uncompressed return type is const char*
 */
const void *TraceDataGetComment ( const TraceData *td, size_t *sizep )
{
    size_t size;
    const char *col = TraceDataGetPeakindex ( td, & size );

    if ( col == NULL )
        return NULL;

    col += size;

    switch ( td -> size_var )
    {
    case 0:
        size = td -> col_sizes . v8 [ 3 ];
        break;
    case 1:
        size = td -> col_sizes . v16 [ 3 ];
        break;
    default:
        size = td -> col_sizes . v32 [ 3 ];
    }

    if ( sizep != NULL )
        * sizep = size;
    return col;
}

/* TraceDataGetExtended
 *  returns a pointer to the extended data sequence
 *
 *  uncompressed return type is const char*
 */
const void *TraceDataGetExtended ( const TraceData *td, size_t *sizep )
{
    size_t size;
    const char *col;

    if ( td == NULL || td -> extended_cmp == tracedb_extNULL )
        return NULL;

    col = TraceDataGetComment ( td, & size );

    if ( col == NULL )
        return NULL;

    col += size;

    switch ( td -> size_var )
    {
    case 0:
        size = td -> col_sizes . v8 [ 4 ];
        break;
    case 1:
        size = td -> col_sizes . v16 [ 4 ];
        break;
    default:
        size = td -> col_sizes . v32 [ 4 ];
    }

    if ( sizep != NULL )
        * sizep = size;
    return col;
}


/*--------------------------------------------------------------------------
 * TraceDataDecompression
 *  for decompressing trace data after writing to db
 */

/* TraceDataDecompressionInit
 *  prepares tables for going crazy
 */
int TraceDataDecompressionInit ( TraceDataDecompression *dd )
{
    int status;

    if ( dd == NULL )
        return EINVAL;

    status = tracedb_NucDecInit ( & dd -> nucdec );
    if ( status != 0 )
        return status;

#if TRACEDB_USING_ICOMPRESS
    status = tracedb_IDecompressInit ( & dd -> id );
    if ( status != 0 )
        return status;
#endif

#if TRACEDB_USING_UCOMPRESS
    status = tracedb_UDecompressInit ( & dd -> ud );
    if ( status != 0 )
        return status;
#endif

#if TRACEDB_USING_GZIP
    status = tracedb_GunzipInit ( & dd -> guz );
    tracedb_atomic32_set ( & dd -> guz_refcount, 0 );
#endif

    return status;
}

/* TraceDataDecompressBasecall
 */
static
int TraceDataDecompressBasecall ( const TraceDataDecompression *dd,
    TraceData *td, const TraceData *src, char **d, const char **s )
{
    int status;
    size_t ssize = td -> col_sizes . v32 [ 0 ];
    size_t dsize = td -> size - ( * d - ( char* ) td );

    switch ( src -> basecall_cmp )
    {
    case tracedb_bcNone:
        status = 0;
        break;
#if 0
    case tracedb_bcGzip:
        status = gunzip ( dd, * d, & dsize, * s, ssize );
        if ( status == Z_OK )
        {
            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 0 ] = ( uint32_t ) dsize;
            assert ( td -> basecall_cmp == bcNone );
            return 0;
        }
        break;
#endif
#if 0
    case bcBzip2:
        status = bunzip ( * d, & dsize, * s, ssize );
        if ( status == BZ_OK )
        {
            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 0 ] = ( uint32_t ) dsize;
            assert ( td -> basecall_cmp == bcNone );
            return 0;
        }
        break;
#endif
    case tracedb_bcNucEnc:
        status = tracedb_NucDecDecode ( & dd -> nucdec, * d, & dsize,
            ( const uint8_t* ) * s, ssize );
        if ( status == tracedb_nucSuccess )
        {
            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 0 ] = ( uint32_t ) dsize;
            assert ( td -> basecall_cmp == tracedb_bcNone );
            return 0;
        }
        break;

    default:
        status = -1;
    }

    /* keep existing format */
    memcpy ( * d, *s, ssize );
    * d += ssize;
    * s += ssize;
    td -> basecall_cmp = src -> basecall_cmp;

    return status;
}


/* TraceDataDecompressQualscore
 */
static
int TraceDataDecompressQualscore ( const TraceDataDecompression *dd,
    TraceData *td, const TraceData *src, char **d, const char **s )
{
    int status;
    size_t ssize = td -> col_sizes . v32 [ 1 ];
    size_t dsize = td -> size - ( * d - ( char* ) td );

    size_t i;
    int8_t *dp;

    switch ( src -> qualscore_cmp )
    {
    case tracedb_qsNone:
        status = 0;
        break;
#if 0
    case tracedb_qsGzip:
        status = gunzip ( dd, * d, & dsize, * s, ssize );
        if ( status == Z_OK )
        {
            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 1 ] = ( uint32_t ) dsize;
            assert ( td -> qualscore_cmp == tracedb_qsNone );
            return 0;
        }
        break;
#endif
#if 0
    case tracedb_qsBzip2:
        status = bunzip ( * d, & dsize, * s, ssize );
        if ( status == BZ_OK )
        {
            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 1 ] = ( uint32_t ) dsize;
            assert ( td -> qualscore_cmp == tracedb_qsNone );
            return 0;
        }
        break;
#endif
#if 0
    case tracedb_qsU8:
        status = UDecompress8 ( & dd -> ud, ( uint8_t* ) * d,
            & dsize, ( const uint8_t* ) * s, ssize );
        if ( status == cmpSuccess )
        {
            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 1 ] = ( uint32_t ) dsize;
            assert ( td -> qualscore_cmp == tracedb_qsNone );
            return 0;
        }
        break;
#endif
    case tracedb_qsDeltaI8:
        status = tracedb_IDecompress8 ( & dd -> id, ( int8_t* ) * d,
            & dsize, ( const uint8_t* ) * s, ssize );
        if ( status == tracedb_cmpSuccess )
        {
            for ( i = 1, dp = ( int8_t* ) * d; i < dsize; ++ i )
                dp [ i ] += dp [ i - 1 ];

            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 1 ] = ( uint32_t ) dsize;
            assert ( td -> qualscore_cmp == tracedb_qsNone );
            return 0;
        }
        break;

    default:
        status = -1;
    }

    /* keep existing format */
    if ( ssize != 0 )
    {
        const int8_t *sp;

        dp = ( int8_t* ) * d;
        sp = ( const int8_t* ) * s;

        for ( dp [ 0 ] = sp [ 0 ], i = 1; i < ssize; ++ i )
            dp [ i ] = sp [ i ] + dp [ i - 1 ];
    }
        
    * d += ssize;
    * s += ssize;
    td -> qualscore_cmp = src -> qualscore_cmp;

    return status;
}


/* TraceDataDecompressPeakindex
 */
static
int TraceDataDecompressPeakindex ( const TraceDataDecompression *dd,
    TraceData *td, const TraceData *src, char **d, const char **s )
{
    int status;
    size_t ssize = td -> col_sizes . v32 [ 2 ];
    size_t dsize = td -> size - ( * d - ( char* ) td );

    int32_t *dp;
    unsigned int i, len;

    switch ( src -> peakindex_cmp )
    {
    case tracedb_piNone:
        status = 0;
        break;

    case tracedb_piDelta1:
    case tracedb_piDelta4:
        if ( ssize != 0 )
        {
            const int32_t *sp = ( const int32_t* ) * s;
            dp = ( int32_t* ) * d;

            dp [ 0 ] = sp [ 0 ];
            for ( len = ( unsigned int ) ( ssize >> 2 ), i = 1; i < len; ++ i )
                dp [ i ] = sp [ i ] + dp [ i - 1 ];

            * d += ssize;
            * s += ssize;
        }
        assert ( td -> peakindex_cmp == tracedb_piNone );
        return 0;
#if 0
    case tracedb_piDeltaGzip:
        status = gunzip ( dd, * d, & dsize, * s, ssize );
        if ( status == Z_OK )
        {
            dp = ( int32_t* ) * d;

            for ( len = ( unsigned int ) ( dsize >> 2 ), i = 1; i < len; ++ i )
                dp [ i ] += dp [ i - 1 ];

            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 2 ] = ( uint32_t ) dsize;
            assert ( td -> peakindex_cmp == tracedb_piNone );
            return 0;
        }
        break;
#endif
#if 0
    case tracedb_piDeltaBzip2: /* currently undefined */
        status = bunzip ( * d, & dsize, * s, ssize );
        if ( status == BZ_OK )
        {
            dp = ( int32_t* ) * d;

            for ( len = ( unsigned int ) ( dsize >> 2 ), i = 1; i < len; ++ i )
                dp [ i ] += dp [ i - 1 ];

            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 2 ] = ( uint32_t ) dsize;
            assert ( td -> peakindex_cmp == tracedb_piNone );
            return 0;
        }
        break;
#endif
    case tracedb_piDelta2I32:
        status = tracedb_IDecompress32 ( & dd -> id, ( int32_t* ) * d,
            & dsize, ( const uint8_t* ) * s, ssize );
        if ( status == tracedb_cmpSuccess )
        {
            dp = ( int32_t* ) * d;

            for ( len = ( unsigned int ) ( dsize >> 2 ), i = 1; i < len; ++ i )
                dp [ i ] += dp [ i - 1 ];
            for ( i = 1; i < len; ++ i )
                dp [ i ] += dp [ i - 1 ];

            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 2 ] = ( uint32_t ) dsize;
            assert ( td -> peakindex_cmp == tracedb_piNone );
            return 0;
        }
        break;

    default:
        status = -1;
    }

    /* keep existing format */
    memcpy ( * d, *s, ssize );
    * d += ssize;
    * s += ssize;
    td -> peakindex_cmp = src -> peakindex_cmp;

    return status;
}


/* TraceDataDecompressComment
 */
static
int TraceDataDecompressComment ( const TraceDataDecompression *dd,
    TraceData *td, const TraceData *src, char **d, const char **s )
{
    int status;
    size_t ssize = td -> col_sizes . v32 [ 3 ];
    size_t dsize = td -> size - ( * d - ( char* ) td );

    switch ( src -> comment_cmp )
    {
    case tracedb_cmtNone:
        status = 0;
        break;
#if 1
    case tracedb_cmtGzip:
        status = tracedb_gunzip ( dd, * d, & dsize, * s, ssize );
        if ( status == Z_OK )
        {
            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 3 ] = ( uint32_t ) dsize;
            assert ( td -> comment_cmp == tracedb_cmtNone );
            return 0;
        }
        break;
#endif
#if 0
    case tracedb_cmtBzip2:
        status = bunzip ( * d, & dsize, * s, ssize );
        if ( status == BZ_OK )
        {
            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 3 ] = ( uint32_t ) dsize;
            assert ( td -> comment_cmp == tracedb_cmtNone );
            return 0;
        }
        break;
#endif

    default:
        status = -1;
    }

    /* keep existing format */
    memcpy ( * d, *s, ssize );
    * d += ssize;
    * s += ssize;
    td -> comment_cmp = src -> comment_cmp;

    return status;
}


/* TraceDataDecompressExtended
 */
static
int TraceDataDecompressExtended ( const TraceDataDecompression *dd,
    TraceData *td, const TraceData *src, char **d, const char **s )
{
    int status;
    size_t ssize = td -> col_sizes . v32 [ 4 ];
    size_t dsize = td -> size - ( * d - ( char* ) td );

    switch ( src -> extended_cmp )
    {
    case tracedb_extNULL:
        return 0;

    case tracedb_extNone:
        status = 0;
        break;
#if 1
    case tracedb_extGzip:
        status = tracedb_gunzip ( dd, * d, & dsize, * s, ssize );
        if ( status == Z_OK )
        {
            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 4 ] = ( uint32_t ) dsize;
            assert ( td -> extended_cmp == tracedb_extNone );
            return 0;
        }
        break;
#endif
#if 0
    case tracedb_extBzip2:
        status = bunzip ( * d, & dsize, * s, ssize );
        if ( status == BZ_OK )
        {
            * d += dsize;
            * s += ssize;
            td -> col_sizes . v32 [ 4 ] = ( uint32_t ) dsize;
            assert ( td -> extended_cmp == tracedb_extNone );
            return 0;
        }
        break;
#endif

    default:
        status = -1;
    }

    /* keep existing format */
    memcpy ( * d, *s, ssize );
    * d += ssize;
    * s += ssize;
    td -> extended_cmp = src -> extended_cmp;

    return status;
}


/* TraceDataCompressionExpand
 *  expands indicated columns into a new structure
 */
int TraceDataDecompressionExpand ( const TraceDataDecompression *dd,
    const TraceData *src, TraceData **dst, unsigned int fields )
{
    int status;
    TraceData *td;

    if ( dd == NULL || src == NULL || dst == NULL )
        return EINVAL;

    * dst = NULL;

    if ( fields == 0 )
        td = malloc ( sizeof *td );
    else if ( src -> size < sizeof * td )
        return EINVAL;
    else
        td = malloc ( src -> size );

    if ( td == NULL )
        return errno;
    
    td -> size = src -> size;
    td -> size_var = 2;
    
    td -> basecall_cmp = tracedb_bcNone;
    td -> qualscore_cmp = tracedb_qsNone;
    td -> peakindex_cmp = tracedb_piNone;
    td -> comment_cmp = tracedb_cmtNone;
    td -> extended_cmp = tracedb_extNULL;

    if ( fields == 0 )
    {
        status = 0;
        td -> col_sizes . v32 [ 0 ] = 0;
        td -> col_sizes . v32 [ 1 ] = 0;
        td -> col_sizes . v32 [ 2 ] = 0;
        td -> col_sizes . v32 [ 3 ] = 0;
    }
    else
    {
        const char *s;
        char *d = ( char* ) & td -> col_sizes . v32 [ 4 ];

        switch ( src -> size_var )
        {
        case 0:
            s = ( const char* ) & src -> col_sizes . v8 [ 4 ];
            td -> col_sizes . v32 [ 0 ] = src -> col_sizes . v8 [ 0 ];
            td -> col_sizes . v32 [ 1 ] = src -> col_sizes . v8 [ 1 ];
            td -> col_sizes . v32 [ 2 ] = src -> col_sizes . v8 [ 2 ];
            td -> col_sizes . v32 [ 3 ] = src -> col_sizes . v8 [ 3 ];
            if ( src -> extended_cmp != tracedb_extNULL )
            {
                td -> col_sizes . v32 [ 4 ] = src -> col_sizes . v8 [ 4 ];
                d += 4;
                s += 1;
            }
            break;
        case 1:
            s = ( const char* ) & src -> col_sizes . v16 [ 4 ];
            td -> col_sizes . v32 [ 0 ] = src -> col_sizes . v16 [ 0 ];
            td -> col_sizes . v32 [ 1 ] = src -> col_sizes . v16 [ 1 ];
            td -> col_sizes . v32 [ 2 ] = src -> col_sizes . v16 [ 2 ];
            td -> col_sizes . v32 [ 3 ] = src -> col_sizes . v16 [ 3 ];
            if ( src -> extended_cmp != tracedb_extNULL )
            {
                td -> col_sizes . v32 [ 4 ] = src -> col_sizes . v16 [ 4 ];
                d += 4;
                s += 2;
            }
            break;
        default:
            s = ( const char* ) & src -> col_sizes . v32 [ 4 ];
            td -> col_sizes . v32 [ 0 ] = src -> col_sizes . v32 [ 0 ];
            td -> col_sizes . v32 [ 1 ] = src -> col_sizes . v32 [ 1 ];
            td -> col_sizes . v32 [ 2 ] = src -> col_sizes . v32 [ 2 ];
            td -> col_sizes . v32 [ 3 ] = src -> col_sizes . v32 [ 3 ];
            if ( src -> extended_cmp != tracedb_extNULL )
            {
                td -> col_sizes . v32 [ 4 ] = src -> col_sizes . v32 [ 4 ];
                d += 4;
                s += 4;
            }
        }

        if ( ( fields & td_basecall ) == 0 )
        {
            s += td -> col_sizes . v32 [ 0 ];
            td -> col_sizes . v32 [ 0 ] = 0;
            status = 0;
        }
        else
        {
            status = TraceDataDecompressBasecall ( dd, td, src, & d, & s );
        }

        if ( status == 0 )
        {
            if ( ( fields & td_qualscore ) == 0 )
            {
                s += td -> col_sizes . v32 [ 1 ];
                td -> col_sizes . v32 [ 1 ] = 0;
            }
            else
            {
                status = TraceDataDecompressQualscore ( dd, td, src, & d, & s );
            }
        }

        if ( status == 0 )
        {
            /* if peakindex is NOT NULL
               this test is performed here to prevent
               pointer alignment on NULL columns, which
               is not done, i.e. would throw everything off */
            if ( td -> col_sizes . v32 [ 2 ] != 0 )
            {
                switch ( src -> peakindex_cmp )
                {
                case tracedb_piNone:
                case tracedb_piDelta4:

                    /* peakindex source is on a 4 byte alignment */
                    if ( ( ( size_t ) s & 3 ) != 0 )
                        s = ( const char* ) ( ( size_t ) s | 3 ) + 1;
                    break;
                }
            
                if ( ( fields & td_peakindex ) == 0 )
                {
                    s += td -> col_sizes . v32 [ 2 ];
                    td -> col_sizes . v32 [ 2 ] = 0;
                }
                else
                {
                    /* peakindex dest is always on a 4 byte alignment */
                    if ( ( ( size_t ) d & 3 ) != 0 )
                        d = ( char* ) ( ( size_t ) d | 3 ) + 1;

                    status = TraceDataDecompressPeakindex ( dd, td, src, & d, & s );
                }
            }
        }

        if ( status == 0 )
        {
            if ( ( fields & td_comment ) == 0 )
            {
                s += td -> col_sizes . v32 [ 3 ];
                td -> col_sizes . v32 [ 3 ] = 0;
            }
            else
            {
                status = TraceDataDecompressComment ( dd, td, src, & d, & s );
            }
        }

        if ( status == 0 && src -> extended_cmp != tracedb_extNULL )
        {
            td -> extended_cmp = tracedb_extNone;

            if ( ( fields & td_extended ) == 0 )
                td -> col_sizes . v32 [ 4 ] = 0;
            else
            {
                status = TraceDataDecompressExtended ( dd, td, src, & d, & s );
            }
        }

        if ( status != 0 )
        {
            free ( td );
            td = NULL;
        }
    }
    
    * dst = td;
    return status;
}

/* TraceDataDecompressionWhack
 */
void TraceDataDecompressionWhack ( TraceDataDecompression *dd )
{
    if ( dd != NULL )
    {
#if TRACEDB_USING_GZIP
        tracedb_GunzipWhack ( & dd -> guz );
#endif
#if TRACEDB_USING_UCOMPRESS
        tracedb_UDecompressWhack ( & dd -> ud );
#endif
#if TRACEDB_USING_ICOMPRESS
        tracedb_IDecompressWhack ( & dd -> id );
#endif
        tracedb_NucDecWhack ( & dd -> nucdec );
    }
}


/*--------------------------------------------------------------------------
 * TraceDB
 */

static bool tracedb_decompression_inited;
static TraceDataDecompression tracedb_s_dd;

/* TraceDBInitDecompression
 *  initializes the library for data set decompression
 */
int TraceDBInitDecompression ( TraceDB *db )
{
    int status;

    if ( tracedb_decompression_inited )
        return 0;

    status = TraceDataDecompressionInit ( & tracedb_s_dd );
    if ( status == 0 )
        tracedb_decompression_inited = true;

    return status;
}

/* TraceDBGetTraceData
 *  access data blob by id
 *
 *  "fields" is a bitmap indicating which fields to retrieve
 */
int TraceDBGetTraceData ( TraceDB *db, const char *path, ti_t id,
    unsigned int fields, TraceData **td )
{
    int status;

    char buffer [ 4096 ];
    size_t num_read, remaining;

    if ( db == NULL || td == NULL )
        return EINVAL;

    * td = NULL;

    status = TraceDBRead ( db, path, id, buffer, sizeof buffer,
        0, & num_read, & remaining );
    if ( status == 0 )
    {
        TraceData *src = ( TraceData* ) buffer;

        if ( remaining != 0 )
        {
            src = malloc ( num_read + remaining );
            if ( src == NULL )
                return errno;
            memcpy ( src, buffer, num_read );

            status = TraceDBRead ( db, path, id, ( char* ) src + num_read,
                remaining, num_read, & num_read, & remaining );
            if ( status == 0 && remaining != 0 )
                status = -1;
            if ( status != 0 )
            {
                free ( src );
                return status;
            }
        }

        if ( ! tracedb_decompression_inited )
        {
#if _DEBUGGING
            tracedb_logmsg ( "TraceDBGetTraceData called before TraceDBInitDecompression\n" );
#endif
            status = -1;
        }
        else
        {
            status = TraceDataDecompressionExpand ( & tracedb_s_dd,
                                            src, td, fields );
        }

        if ( ( char* ) src != buffer )
            free ( src );
    }

    return status;
}

/* TraceDBWhackDecompression
 *  tears down decompression structures
 */
void TraceDBWhackDecompression ( TraceDB *db )
{
    if ( tracedb_decompression_inited )
    {
        TraceDataDecompressionWhack ( & tracedb_s_dd );
        tracedb_decompression_inited = false;
    }
}
