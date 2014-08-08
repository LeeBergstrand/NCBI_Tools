#include "icompress.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <endian.h>


/*--------------------------------------------------------------------------
 * IDecompress
 *  unsigned integer decompression
 */

/* IDecompress32
 *  expand compressed integer data
 *
 *  "src_sz" means only the size of "src".
 *
 *  "dst_sz" is an in/out parameter giving the minimum buffer size on
 *  input and the resultant size after successful decompression.
 */
int tracedb_IDecompress32 ( const tracedb_IDecompress *id,
    int32_t *dst, size_t *dst_szp,
    const void *sp, size_t src_sz )
{
    size_t dst_sz;
    unsigned int i, j, outsize, blk_sz;

    const uint8_t *src;
    const tracedb_CmpHeader *hdr;
    const tracedb_CmpBlock *blk;

    union
    {
        int8_t i8 [ 4 ];
        uint32_t u32;
    } u0;

    union
    {
        int8_t i8 [ 2 ];
        uint16_t u16;
    } u1;

    union
    {
#if __BYTE_ORDER == __LITTLE_ENDIAN
        struct
        {
            int8_t val : 7;
            int8_t extend : 1;
        } s;
        struct
        {
            uint8_t val : 7;
            uint8_t extend : 1;
        } u;
#else
        struct
        {
            int8_t extend : 1;
            int8_t val : 7;
        } s;
        struct
        {
            uint8_t extend : 1;
            uint8_t val : 7;
        } u;
#endif
        uint8_t u8;
    } i7;

    int32_t i32;

    if ( id == NULL || dst_szp == NULL )
        return tracedb_cmpBadParam;

    if ( src_sz == 0 )
    {
        * dst_szp = 0;
        return tracedb_cmpSuccess;
    }

    dst_sz = * dst_szp;

    if ( dst == NULL || sp == NULL )
        return tracedb_cmpBadParam;

    if ( dst_sz <= src_sz )
        return tracedb_cmpNoBuffer;

    src = sp;
    hdr = sp;
    blk = ( const tracedb_CmpBlock* ) & src [ 1 ];

    if ( hdr -> sign == 0 || hdr -> bytes != 2 )
        return tracedb_cmpInvalid;

    blk_sz = blk -> size + 1;
    dst_sz >>= 2;

    /* process all but last block */
    for ( outsize = 0, i = blk_sz + 2; i < ( unsigned int ) src_sz; i += blk_sz + 1 )
    {
        switch ( blk -> bits )
        {
        case 0:
            /* two bit unsigned */
            if ( dst_sz < ( size_t ) ( blk_sz << 2 ) )
                return tracedb_cmpNoBuffer;

            for ( j = 0; j < blk_sz; ++ j )
            {
                u0 . u32 = id -> signed2to8 [ blk -> data [ j ] ];
                dst [ outsize + 0 ] = u0 . i8 [ 0 ];
                dst [ outsize + 1 ] = u0 . i8 [ 1 ];
                dst [ outsize + 2 ] = u0 . i8 [ 2 ];
                dst [ outsize + 3 ] = u0 . i8 [ 3 ];

                outsize += 4;
                dst_sz -= 4;
            }
            break;

        case 1:
            /* four bit unsigned */
            if ( dst_sz < ( size_t ) ( blk_sz + blk_sz ) )
                return tracedb_cmpNoBuffer;

            for ( j = 0; j < blk_sz; ++ j )
            {
                u1 . u16 = id -> signed4to8 [ blk -> data [ j ] ];
                dst [ outsize + 0 ] = u1 . i8 [ 0 ];
                dst [ outsize + 1 ] = u1 . i8 [ 1 ];

                outsize += 2;
                dst_sz -= 2;
            }
            break;

        case 2:
            /* eight bit unsigned */
            if ( dst_sz < ( size_t ) blk_sz )
                return tracedb_cmpNoBuffer;

            for ( j = 0; j < blk_sz; ++ j )
                dst [ outsize + j ] = ( int8_t ) blk -> data [ j ];

            outsize += blk_sz;
            dst_sz -= blk_sz;
            break;

        default:
            /* variable length 7 bit unsigned */
            for ( j = 0; j < blk_sz; ++ j )
            {
                i7 . u8 = blk -> data [ j ];
                i32 = i7 . s . val;

                while ( i7 . u . extend )
                {
                    if ( ++ j == blk_sz )
                        return tracedb_cmpInvalid;
                    i7 . u8 = blk -> data [ j ];
                        
                    i32 = ( i32 << 7 ) | i7 . u . val;
                }

                if ( dst_sz == 0 )
                    return tracedb_cmpNoBuffer;

                dst [ outsize ] = i32;
                ++ outsize;
                -- dst_sz;
            }
        }

        /* move to next block */
        blk = ( const tracedb_CmpBlock* ) & src [ i ];
        blk_sz = blk -> size + 1;
    }

    switch ( blk -> bits )
    {
    case 0:
        /* compensate for partial last byte */
        if ( hdr -> rem != 0 )
            -- blk_sz;

        for ( j = 0; j < blk_sz; ++ j )
        {
            if ( dst_sz < 4 )
                return tracedb_cmpNoBuffer;

            u0 . u32 = id -> signed2to8 [ blk -> data [ j ] ];
            dst [ outsize + 0 ] = u0 . i8 [ 0 ];
            dst [ outsize + 1 ] = u0 . i8 [ 1 ];
            dst [ outsize + 2 ] = u0 . i8 [ 2 ];
            dst [ outsize + 3 ] = u0 . i8 [ 3 ];

            outsize += 4;
            dst_sz -= 4;
        }

        if ( hdr -> rem != 0 )
        {
            if ( dst_sz < ( size_t ) hdr -> rem )
                return tracedb_cmpNoBuffer;

            u0 . u32 = id -> signed2to8 [ blk -> data [ j ] ];
            switch ( hdr -> rem )
            {
            case 3:
                dst [ outsize + 2 ] = u0 . i8 [ 2 ];
            case 2:
                dst [ outsize + 1 ] = u0 . i8 [ 1 ];
            default:
                dst [ outsize + 0 ] = u0 . i8 [ 0 ];
            }

            outsize += hdr -> rem;
        }
        break;

    case 1:
        /* four bit unsigned */
        if ( hdr -> rem != 0 )
            -- blk_sz;

        for ( j = 0; j < blk_sz; ++ j )
        {
            if ( dst_sz < 2 )
                return tracedb_cmpNoBuffer;

            u1 . u16 = id -> signed4to8 [ blk -> data [ j ] ];
            dst [ outsize + 0 ] = u1 . i8 [ 0 ];
            dst [ outsize + 1 ] = u1 . i8 [ 1 ];

            outsize += 2;
            dst_sz -= 2;
        }

        if ( hdr -> rem != 0 )
        {
            if ( dst_sz == 0 )
                return tracedb_cmpNoBuffer;

            u1 . u16 = id -> signed4to8 [ blk -> data [ j ] ];
            dst [ outsize + 0 ] = u1 . i8 [ 0 ];

            ++ outsize;
        }
        break;

    case 2:
        /* eight bit unsigned */
        if ( dst_sz < ( size_t ) blk_sz )
            return tracedb_cmpNoBuffer;

        for ( j = 0; j < blk_sz; ++ j )
            dst [ outsize + j ] = ( int8_t ) blk -> data [ j ];

        outsize += blk_sz;
        dst_sz -= blk_sz;
        break;

    default:
        /* variable length 7 bit unsigned */
        for ( j = 0; j < blk_sz; ++ j )
        {
            i7 . u8 = blk -> data [ j ];
            i32 = i7 . s . val;

            while ( i7 . u . extend )
            {
                if ( ++ j == blk_sz )
                    return tracedb_cmpInvalid;
                i7 . u8 = blk -> data [ j ];
                        
                i32 = ( i32 << 7 ) | i7 . u . val;
            }

            if ( dst_sz == 0 )
                return tracedb_cmpNoBuffer;

            dst [ outsize ] = i32;
            ++ outsize;
            -- dst_sz;
        }
    }

    * dst_szp = outsize << 2;
    return tracedb_cmpSuccess;
}
