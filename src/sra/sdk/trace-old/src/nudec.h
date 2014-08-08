/*==============================================================================

  nudec.h

    taken from C++ nuenc.hpp
    some modifications applied...
 */


#ifndef _h_nudec_
#define _h_nudec_

#ifndef _h_itypes_
#include "itypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * NucBlock
 *  a block of compressed data
 */
typedef struct tracedb_NucBlock tracedb_NucBlock;
struct tracedb_NucBlock
{
    /* encoding is either 4 bits or 2 bits */
    uint8_t two_bit : 1;

    /* number of bytes in data from 1..128 */
    uint8_t size : 7;

    /* compressed data */
    uint8_t data [ 128 ];
};


/*--------------------------------------------------------------------------
 * NucEncError
 *  error codes
 */
enum tracedb_NucEncError
{
    tracedb_nucSuccess,
    tracedb_nucBadParam,
    tracedb_nucInvalid,
    tracedb_nucFailed
};

/* NucEncErrorName
 *  returns error name from NucEncError
 */
const char *tracedb_NucEncErrorName ( int err );

/* NucEncErrorStr
 *  returns error string from NucEncError
 */
const char *tracedb_NucEncErrorStr ( int err );


/*--------------------------------------------------------------------------
 * NucDec
 *  loves to decode
 */
typedef struct tracedb_NucDec tracedb_NucDec;
struct tracedb_NucDec
{
    /* conversion from 4 bit encoding to 8 bit */
    uint16_t conv4to8na [ 256 ];

    /* conversion from 2 bit encoding to 8 bit */
    uint32_t conv2to8na [ 256 ];
};

/* NucDecInit
 *  initialize structure for decoding
 */
int tracedb_NucDecInit ( tracedb_NucDec *dec );

/* NucDecDecode
 *  decodes block
 *
 *  "src_sz" is the size of the compressed block
 *  "dst_sz" is the size of the destination buffer on input
 *  and the size of the decompressed block on output
 *
 *  returns NucEncError where nucFailed means
 *  insufficient output buffer space
 */
int tracedb_NucDecDecode ( const tracedb_NucDec *dec, char *dst,
    size_t *dst_sz, const uint8_t *src, size_t src_sz );

/* NucDecWhack
 *  destroys the structure
 */
#define tracedb_NucDecWhack( dec ) \
    ( void ) 0


#ifdef __cplusplus
}
#endif

#endif /* _h_nudec_ */
