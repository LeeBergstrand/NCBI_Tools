/*==============================================================================

  tracedata.h

    retrieval of trace data blobs from a TraceDB
 */


#ifndef _h_tracedata_
#define _h_tracedata_

#ifndef _h_tracedb_
#include "tracedb.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * TraceData
 *  structure for retrieval of data blob
 */
enum tracedb_BCCompression
{
    tracedb_bcNone,
    tracedb_bcGzip,
    tracedb_bcBzip2,
    tracedb_bcNucEnc
};

enum tracedb_QSCompression
{
    tracedb_qsNone,
    tracedb_qsGzip,
    tracedb_qsBzip2,
    tracedb_qsU8,
    tracedb_qsDeltaI8
};

enum tracedb_PICompression
{
    tracedb_piNone,
    tracedb_piDelta1,      /* 32 bit deltas stored on 1 byte boundaries */
    tracedb_piDelta4,      /* 32 bit deltas stored on 4 byte boundaries */
    tracedb_piDeltaGzip,
    tracedb_piDelta2I32
};

enum tracedb_CCompression
{
    tracedb_cmtNone,
    tracedb_cmtGzip,
    tracedb_cmtBzip2
};

enum tracedb_XCompression
{
    tracedb_extNULL,
    tracedb_extNone,
    tracedb_extGzip,
    tracedb_extBzip2
};

typedef struct TraceData TraceData;
struct TraceData
{
    /* size of total uncompressed record */
    uint32_t size;

    /* variant of size union to select */
    uint16_t size_var : 2;

    /* column compression codes */
    uint16_t basecall_cmp : 3;
    uint16_t qualscore_cmp : 3;
    uint16_t peakindex_cmp : 3;
    uint16_t comment_cmp : 3;
    uint16_t extended_cmp : 2;

    /* column sizes */
    union
    {
        uint8_t v8 [ 4 ];
        uint16_t v16 [ 4 ];
        uint32_t v32 [ 4 ];

    } col_sizes;

#if 0
    /* basecall column */
    char basecall [];
#endif

#if 0
    /* qualscore column */
    uint8_t qualscore [];
#endif

#if 0
    /* filler for peakindex alignment */
    const filler_len = peakindex_cmp == piNone || peakindex_cmp == piDelta ?
        ( - offsetof ( TraceData, peakindex_fill ) ) & 3 : 0;
    char peakindex_fill [ filler_len ];

    /* peakindex column */
    union
    {
        uint8_t nibbles [];
        uint8_t bytes [];
        uint32_t full [];

    } peakindex;
#endif

#if 0
    /* comment column */
    uint8_t comment [];
#endif

#if 0
    /* extended column */
    char extended [];
#endif
};

/* TraceDataColumns
 *  values for building a bitmap
 */
enum TraceDataColumns
{
    td_basecall  = ( 1 << 0 ),
    td_qualscore = ( 1 << 1 ),
    td_peakindex = ( 1 << 2 ),
    td_comment   = ( 1 << 3 ),
    td_extended  = ( 1 << 4 )
};

/* TraceDataWhack
 *  drops object
 */
void TraceDataWhack ( TraceData *td );
#define TraceDataWhack( td ) \
    free ( td )

/* TraceDataGetBasecall
 *  returns a pointer to the NULL terminated basecall sequence
 *
 *  uncompressed return type is const char*
 */
const void *TraceDataGetBasecall ( const TraceData *td, size_t *size );

/* TraceDataGetQualscore
 *  returns a pointer to the qualscore sequence
 *
 *  uncompressed return type is const uint8_t*
 */
const void *TraceDataGetQualscore ( const TraceData *td, size_t *size );

/* TraceDataGetPeakindex
 *  returns a pointer to the peakindex sequence
 *
 *  the peaks are real indices, not deltas
 *
 *  uncompressed return type is const uint32_t*
 */
const void *TraceDataGetPeakindex ( const TraceData *td, size_t *size );

/* TraceDataGetComment
 *  returns a pointer to the comment sequence
 *
 *  uncompressed return type is const char*
 */
const void *TraceDataGetComment ( const TraceData *td, size_t *size );

/* TraceDataGetExtended
 *  returns a pointer to the extended data sequence
 *
 *  uncompressed return type is const char*
 */
const void *TraceDataGetExtended ( const TraceData *td, size_t *size );


/*--------------------------------------------------------------------------
 * TraceDB
 */

/* TraceDBInitDecompression
 *  initializes the library for data set decompression
 */
int TraceDBInitDecompression ( TraceDB *db );

/* TraceDBGetTraceData
 *  access data blob by id
 *
 *  "fields" is a bitmap indicating which fields to retrieve
 */
int TraceDBGetTraceData ( TraceDB *db,
    const char *path, ti_t id,
    unsigned int fields, TraceData **tdp );

/* TraceDBWhackDecompression
 *  tears down decompression structures
 */
void TraceDBWhackDecompression ( TraceDB *db );


#ifdef __cplusplus
}
#endif

#endif /* _h_tracedata_ */
