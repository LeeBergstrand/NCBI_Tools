/*==============================================================================

  nuenc.h

    taken from C++ nuenc.hpp
    some modifications applied...
 */


#ifndef _h_nuenc_
#define _h_nuenc_

#ifndef _h_nudec_
#include "nudec.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * NucEnc
 *  loves to encode
 */
typedef struct tracedb_NucEnc tracedb_NucEnc;
struct tracedb_NucEnc
{
    /* translates source into canonical form */
    int8_t IUPACna [ 256 ];

    /* conversion from 4bit to 2bit representation */
    int8_t conv4to2na [ 256 ];
};

/* NucEncInit
 *  initialize the structure for encoding
 */
int tracedb_NucEncInit ( tracedb_NucEnc *enc );

/* NucEncEncode
 *  encodes our guy
 *
 *  "size" is in/out and
 *   applies to both dst and src on input
 *   and to dst on output
 *
 *  returns NucEncError, where nucFailed
 *  means failure to obtain a better representation
 */
int tracedb_NucEncEncode ( const tracedb_NucEnc *enc, uint8_t *dst,
    const char *src, size_t *size );

/* NucEncWhack
 *  destroy the structure
 */
#define tracedb_NucEncWhack( enc ) \
    ( void ) 0


#ifdef __cplusplus
}
#endif

#endif /* _h_nuenc_ */
