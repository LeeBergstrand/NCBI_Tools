#ifndef _h_tracecmp_
#define _h_tracecmp_

#ifndef _h_queue_
#include "queue.h"
#endif

#ifndef _h_mmem_
#include "mmem.h"
#endif

#ifndef _h_icompress_
#include "icompress.h"
#endif

#ifndef _h_nuenc_
#include "nuenc.h"
#endif

#ifndef _h_nudec_
#include "nudec.h"
#endif

#ifndef _h_tracedata_
#include "tracedata.h"
#endif

#define TRACEDB_USING_GZIP 1
#define TRACEDB_USING_GUNZIP 1
#define TRACEDB_USING_BZIP2 0
#define TRACEDB_USING_ICOMPRESS 1
#define TRACEDB_USING_UCOMPRESS 0

#if TRACEDB_USING_GZIP || TRACEDB_USING_BZIP2

#ifndef _h_trace_atomic32_
#include "trace_atomic32.h"
#endif

#endif

#if TRACEDB_USING_GZIP
#include <zlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * Gzip
 * Gunzip
 *  compression processor
 */
typedef z_stream tracedb_Gzip, tracedb_Gunzip;


/*--------------------------------------------------------------------------
 * TraceDataCompression
 *  for compressing trace data before writing to db
 */
typedef struct TraceDataCompression TraceDataCompression;
struct TraceDataCompression
{
    /* function to be invoked following data push */
    int ( * pipe_func ) ( void *data );
    void *pipe_data;

    /* i/o queues */
    tracedb_Queue *in;
    tracedb_Queue out;

    /* working memory */
    tracedb_Memory buffer;

    /* nucleotide compressor */
    tracedb_NucEnc nuenc;

#if TRACEDB_USING_ICOMPRESS
    /* signed integer compressor */
    tracedb_ICompress ic;
#endif

#if TRACEDB_USING_UCOMPRESS
    /* unsigned integer compressor */
    tracedb_UCompress uc;
#endif

#if TRACEDB_USING_GZIP
    /* gzip compressor */
    tracedb_Gzip gz;
#endif

#if TRACEDB_USING_GUNZIP
    /* gunzip decompressor */
    tracedb_Gunzip guz;
#endif

    /* timeouts */
    unsigned int rd_timeout;
    unsigned int wr_timeout;
};

/* TraceDataCompressionInit
 */
int TraceDataCompressionInit ( TraceDataCompression *dc,
    tracedb_Queue *in, unsigned int rd_timeout, unsigned int wr_timeout,
    unsigned int qlen );

/* TraceDataCompressionSetPipeFunc
 *  sets a pipe function for compression element
 *
 *  a pipe function is used to form a pipe between
 *  processing stages that normally run on separate threads
 */
#define TraceDataCompressionSetPipeFunc( dc, f, data ) \
    ( void ) ( ( dc ) . pipe_data = ( data ), ( dc ) . pipe_func = ( f ) )

/* TraceDataCompressionExecPipeFunc
 */
#define TraceDataCompressionExecPipeFunc( dc ) \
    ( ( dc ) -> pipe_func != NULL ? \
      ( * ( dc ) -> pipe_func ) ( ( dc ) -> pipe_data ) : 0 )

/* TraceDataCompressionRun
 */
int TraceDataCompressionRun ( TraceDataCompression *dc );

/* TraceDataCompressionPipeFunc
 *  a cute little pipe function for single threaded use
 *
 *  may be used as the pipe function on object
 */
int TraceDataCompressionPipeFunc ( void *data );

/* TraceDataCompressionWhack
 */
void TraceDataCompressionWhack ( TraceDataCompression *dc );


/*--------------------------------------------------------------------------
 * TraceDataDecompression
 *  for decompressing trace data after writing to db
 */
typedef struct TraceDataDecompression TraceDataDecompression;
struct TraceDataDecompression
{
    /* nucleotide decompressor */
    tracedb_NucDec nucdec;

#if TRACEDB_USING_ICOMPRESS
    /* signed integer decompressor */
    tracedb_IDecompress id;
#endif

#if TRACEDB_USING_UCOMPRESS
    /* unsigned integer decompressor */
    tracedb_UDecompress ud;
#endif

#if TRACEDB_USING_GZIP
    /* gzip decompressor */
    tracedb_Gunzip guz;
    tracedb_atomic32_t guz_refcount;
#endif
};

/* TraceDataDecompressionInit
 *  prepares tables for going crazy
 */
int TraceDataDecompressionInit ( TraceDataDecompression *dd );

/* TraceDataCompressionExpand
 *  expands indicated columns into a new structure
 */
int TraceDataDecompressionExpand ( const TraceDataDecompression *dd,
    const TraceData *src, TraceData **dst, unsigned int fields );

/* TraceDataDecompressionWhack
 */
void TraceDataDecompressionWhack ( TraceDataDecompression *dd );

#ifdef __cplusplus
}
#endif

#endif /* _h_tracecmp_ */
