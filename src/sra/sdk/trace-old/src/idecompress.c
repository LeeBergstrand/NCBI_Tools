#include "icompress.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <endian.h>

#if _DEBUGGING
#define TRACEDB_TEST_RECOMPRESS 0
#endif


/*--------------------------------------------------------------------------
 * IDecompress
 *  signed integer decompression
 */

/* IDecompressInit
 *  initialize lookup tables
 */
int tracedb_IDecompressInit ( tracedb_IDecompress *id )
{
    unsigned int i;

#if TRACEDB_TEST_RECOMPRESS
    tracedb_ICompress ic;
#endif

    for ( i = 0; i < 256; ++ i )
    {
        union
        {
            struct
            {
#if __BYTE_ORDER == __LITTLE_ENDIAN
                int8_t b3 : 2;
                int8_t b2 : 2;
                int8_t b1 : 2;
                int8_t b0 : 2;
#else
                int8_t b0 : 2;
                int8_t b1 : 2;
                int8_t b2 : 2;
                int8_t b3 : 2;
#endif
            } s;
            uint8_t u8;
        } u0;

        union
        {
            int8_t i8 [ 4 ];
            uint32_t u32;
        } u1;

        u0 . u8 = ( uint8_t ) i;
        u1 . i8 [ 0 ] = u0 . s . b0;
        u1 . i8 [ 1 ] = u0 . s . b1;
        u1 . i8 [ 2 ] = u0 . s . b2;
        u1 . i8 [ 3 ] = u0 . s . b3;

        id -> signed2to8 [ i ] = u1 . u32;
    }

    for ( i = 0; i < 256; ++ i )
    {
        union
        {
            struct
            {
#if __BYTE_ORDER == __LITTLE_ENDIAN
                int8_t b1 : 4;
                int8_t b0 : 4;
#else
                int8_t b0 : 4;
                int8_t b1 : 4;
#endif
            } s;
            uint8_t u8;
        } u0;

        union
        {
            int8_t i8 [ 2 ];
            uint16_t u16;
        } u1;

        u0 . u8 = ( uint8_t ) i;
        u1 . i8 [ 0 ] = u0 . s . b0;
        u1 . i8 [ 1 ] = u0 . s . b1;

        id -> signed4to8 [ i ] = u1 . u16;
    }

#if TRACEDB_TEST_RECOMPRESS
    if ( tracedb_ICompressInit ( & ic ) == 0 )
    {
        uint8_t cmpBuffer [ 8 ];
        size_t cmp_sz = sizeof cmpBuffer;
        int status = tracedb_ICompress8 ( & ic, cmpBuffer, & cmp_sz, "\xFF\x01\xFE\x00\xFF", 5 );
        if ( status == cmpSuccess )
        {
            int8_t dcmpBuffer [ 8 ];
            size_t dcmp_sz = sizeof dcmpBuffer;
            status = tracedb_IDecompress8 ( id, dcmpBuffer, & dcmp_sz, cmpBuffer, cmp_sz );
            if ( status == 0 )
            {
                assert ( dcmp_sz == 5 );
                assert ( dcmpBuffer [ 0 ] == -1 );
                assert ( dcmpBuffer [ 1 ] == 1 );
                assert ( dcmpBuffer [ 2 ] == -2 );
                assert ( dcmpBuffer [ 3 ] == 0 );
                assert ( dcmpBuffer [ 4 ] == -1 );
            }
        }

        tracedb_ICompressWhack ( & ic );
    }
#endif
    return tracedb_cmpSuccess;
}
