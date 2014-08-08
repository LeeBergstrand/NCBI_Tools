/*==============================================================================

  icompress.h

    taken from nudec.h which was
    taken from C++ nuenc.hpp

 */


#ifndef _h_icompress_
#define _h_icompress_

#ifndef _h_itypes_
#include "itypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * CmpHeader
 *  header to compressed block
 */
typedef struct tracedb_CmpHeader tracedb_CmpHeader;
struct tracedb_CmpHeader
{
    /* encoded data were signed or not */
    uint8_t sign : 1;

    /* decode to 1, 2, 4 or 8 bytes.
       uses codes 0 => 1, 1 => 2, 2 => 4, 3 => 8 */
    uint8_t bytes : 2;

    /* remainder from last packet */
    uint8_t rem : 5;
};


/*--------------------------------------------------------------------------
 * CmpBlock
 *  a block of compressed data
 */
typedef struct tracedb_CmpBlock tracedb_CmpBlock;
struct tracedb_CmpBlock
{
    /* encoding of data within block
       the codes are interpreted as a function
       of original data width:

       code   bytes   bits
       ====   =====   ====
        0       *      2
        1       *      4
        2       1      5
        2       2+     8
        3       1      8
        3       2+     7+

        for original sizes of 16+ bits, code
        3 represents a special 8 bit encoding
        where bit 7 is used as a continuance
        indicator, thus giving 7 real bits
        in a variable length format. */
    uint8_t bits : 2;

    /* number of bytes in data from 1..64,
       regardless of block data encoding */
    uint8_t size : 6;

    /* compressed data */
    uint8_t data [ 64 ];
};


/*--------------------------------------------------------------------------
 * CmpError
 *  error codes
 */
enum tracedb_CmpError
{
    tracedb_cmpSuccess,
    tracedb_cmpBadParam,
    tracedb_cmpInvalid,
    tracedb_cmpNoBuffer,
    tracedb_cmpFailed
};

/* CmpErrorName
 *  returns error name from CmpError
 */
const char *tracedb_CmpErrorName ( int err );

/* CmpErrorStr
 *  returns error string from CmpError
 */
const char *tracedb_CmpErrorStr ( int err );


/*--------------------------------------------------------------------------
 * ICompress
 *  signed integer compression
 */
typedef struct tracedb_ICompress tracedb_ICompress;
struct tracedb_ICompress
{
    int dummy;
};

/* ICompressInit
 *  currently nothing to do
 */
#define tracedb_ICompressInit( ic ) \
    0

/* ICompressXX
 *  perform compression on a signed 8 bit sequence
 *
 *  "src_sz" means not only the size of "src", but also minimally
 *  the size of "dst" and is the score to beat in order to return
 *  cmpSuccess.
 *
 *  "dst_sz" is an in/out parameter giving the minimum buffer size on
 *  input and the resultant size after successful decompression.
 */
int tracedb_ICompress8 ( const tracedb_ICompress *ic, void *dst, size_t *dst_sz,
    const int8_t *src, size_t src_sz );
int tracedb_ICompress32 ( const tracedb_ICompress *ic, void *dst, size_t *dst_sz,
    const int32_t *src, size_t src_sz );
int tracedb_ICompress64 ( const tracedb_ICompress *ic, void *dst, size_t *dst_sz,
    const int64_t *src, size_t src_sz );

/* ICompressWhack
 *  currently nothing to do
 */
#define tracedb_ICompressWhack( ic ) \
    ( ( void ) 0 )


/*--------------------------------------------------------------------------
 * IDecompress
 *  signed integer decompression
 */
typedef struct tracedb_IDecompress tracedb_IDecompress;
struct tracedb_IDecompress
{
    uint32_t signed2to8 [ 256 ];
    uint16_t signed4to8 [ 256 ];
};


/* IDecompressInit
 *  initialize lookup tables
 */
int tracedb_IDecompressInit ( tracedb_IDecompress *id );

/* IDecompressXX
 *  expand compressed integer data
 *
 *  "src_sz" means only the size of "src".
 *
 *  "dst_sz" is an in/out parameter giving the minimum buffer size on
 *  input and the resultant size after successful decompression.
 */
int tracedb_IDecompress8 ( const tracedb_IDecompress *id, int8_t *dst, size_t *dst_sz,
    const void *src, size_t src_sz );
int tracedb_IDecompress32 ( const tracedb_IDecompress *id, int32_t *dst, size_t *dst_sz,
    const void *src, size_t src_sz );
int tracedb_IDecompress64 ( const tracedb_IDecompress *id, int64_t *dst, size_t *dst_sz,
    const void *src, size_t src_sz );

/* IDecompressWhack
 *  clean up
 */
#define tracedb_IDecompressWhack( id ) \
    ( ( void ) 0 )


/*--------------------------------------------------------------------------
 * UCompress
 *  unsigned integer compression
 */
typedef struct tracedb_UCompress tracedb_UCompress;
struct tracedb_UCompress
{
    int dummy;
};

/* UCompressInit
 *  currently nothing to do
 */
#define tracedb_UCompressInit( uc ) \
    0

/* UCompressXX
 *  perform compression on a signed 8 bit sequence
 *
 *  "src_sz" means not only the size of "src", but also minimally
 *  the size of "dst" and is the score to beat in order to return
 *  cmpSuccess.
 *
 *  "dst_sz" is an in/out parameter giving the minimum buffer size on
 *  input and the resultant size after successful decompression.
 */
int tracedb_UCompress8 ( const tracedb_UCompress *uc, void *dst, size_t *dst_sz,
    const uint8_t *src, size_t src_sz );
int tracedb_UCompress32 ( const tracedb_UCompress *uc, void *dst, size_t *dst_sz,
    const uint32_t *src, size_t src_sz );
int tracedb_UCompress64 ( const tracedb_UCompress *uc, void *dst, size_t *dst_sz,
    const uint64_t *src, size_t src_sz );

/* UCompressWhack
 *  currently nothing to do
 */
#define tracedb_UCompressWhack( uc ) \
    ( ( void ) 0 )


/*--------------------------------------------------------------------------
 * UDecompress
 *  unsigned integer decompression
 */
typedef struct tracedb_UDecompress tracedb_UDecompress;
struct tracedb_UDecompress
{
    uint32_t unsigned2to8 [ 256 ];
    uint16_t unsigned4to8 [ 256 ];
};


/* UDecompressInit
 *  initialize lookup tables
 */
int tracedb_UDecompressInit ( tracedb_UDecompress *ud );

/* UDecompressXX
 *  expand compressed integer data
 *
 *  "src_sz" means only the size of "src".
 *
 *  "dst_sz" is an in/out parameter giving the minimum buffer size on
 *  input and the resultant size after successful decompression.
 */
int tracedb_UDecompress8 ( const tracedb_UDecompress *ud, uint8_t *dst, size_t *dst_sz,
    const void *src, size_t src_sz );
int tracedb_UDecompress32 ( const tracedb_UDecompress *ud, uint32_t *dst, size_t *dst_sz,
    const void *src, size_t src_sz );
int tracedb_UDecompress64 ( const tracedb_UDecompress *ud, uint64_t *dst, size_t *dst_sz,
    const void *src, size_t src_sz );

/* UDecompressWhack
 *  currently nothing to do
 */
#define tracedb_UDecompressWhack( ud ) \
    ( ( void ) 0 )


#ifdef __cplusplus
}
#endif

#endif /* _h_icompress_ */
