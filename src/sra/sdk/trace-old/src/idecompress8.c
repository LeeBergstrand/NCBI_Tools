#include "icompress.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <endian.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TRACEDB_U2_HIGH 1
#define TRACEDB_U2_LOW 0
#else
#define TRACEDB_U2_HIGH 0
#define TRACEDB_U2_LOW 1
#endif


/*--------------------------------------------------------------------------
 * IDecompress
 *  signed integer decompression
 */

/* IDecompress8
 *  expand compressed integer data
 *
 *  "src_sz" means only the size of "src".
 *
 *  "dst_sz" is an in/out parameter giving the minimum buffer size on
 *  input and the resultant size after successful decompression.
 */
int tracedb_IDecompress8 ( const tracedb_IDecompress *id,
    int8_t *dst, size_t *dst_szp,
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
        struct
        {
#if __BYTE_ORDER == __LITTLE_ENDIAN
            int16_t b2 : 5;
            int16_t b1 : 5;
            int16_t b0 : 5;
            int16_t ignore : 1;
#else
            int16_t ignore : 1;
            int16_t b0 : 5;
            int16_t b1 : 5;
            int16_t b2 : 5;
#endif
        } s;
        uint8_t u8 [ 2 ];
    } u2;


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

    if ( hdr -> sign == 0 || hdr -> bytes != 0 )
        return tracedb_cmpInvalid;

    blk_sz = blk -> size + 1;

    /* process all but last block */
    for ( outsize = 0, i = blk_sz + 2; i < ( unsigned int ) src_sz; i += blk_sz + 1 )
    {
        switch ( blk -> bits )
        {
        case 0:
            /* two bit signed (!) */
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
            /* four bit signed */
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
            /* five bit signed */
            if ( dst_sz < ( size_t ) ( ( blk_sz >> 1 ) * 3 ) )
                return tracedb_cmpNoBuffer;
            if ( ( blk_sz & 1 ) != 0 )
                return tracedb_cmpInvalid;

            for ( j = 0; j < blk_sz; j += 2 )
            {
                u2 . u8 [ TRACEDB_U2_HIGH ] = blk -> data [ j + 0 ];
                u2 . u8 [ TRACEDB_U2_LOW ] = blk -> data [ j + 1 ];

                dst [ outsize + 0 ] = u2 . s . b0;
                dst [ outsize + 1 ] = u2 . s . b1;
                dst [ outsize + 2 ] = u2 . s . b2;

                outsize += 3;
                dst_sz -= 3;
            }
            break;

        default:
            /* straight eight bit */
            if ( dst_sz < ( size_t ) blk_sz )
                return tracedb_cmpNoBuffer;

            for ( j = 0; j < blk_sz; ++ j )
                dst [ outsize + j ] = blk -> data [ j ];

            outsize += blk_sz;
            dst_sz -= blk_sz;
        }

        /* move to next block */
        blk = ( const tracedb_CmpBlock* ) & src [ i ];
        blk_sz = blk -> size + 1;
    }

    /* now process last block */
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
        /* four bit signed */
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
        /* five bit signed */
        if ( ( blk_sz & 1 ) != 0 )
            return tracedb_cmpInvalid;
        
        if ( hdr -> rem != 0 )
            blk_sz -= 2;
        
        for ( j = 0; j < blk_sz; j += 2 )
        {
            u2 . u8 [ TRACEDB_U2_HIGH ] = blk -> data [ j + 0 ];
            u2 . u8 [ TRACEDB_U2_LOW ] = blk -> data [ j + 1 ];
            
            dst [ outsize + 0 ] = u2 . s . b0;
            dst [ outsize + 1 ] = u2 . s . b1;
            dst [ outsize + 2 ] = u2 . s . b2;
            
            outsize += 3;
            dst_sz -= 3;
        }

        if ( hdr -> rem != 0 )
        {
            if ( dst_sz < ( size_t ) hdr -> rem )
                return tracedb_cmpNoBuffer;

            u2 . u8 [ TRACEDB_U2_HIGH ] = blk -> data [ j + 0 ];
            u2 . u8 [ TRACEDB_U2_LOW ] = blk -> data [ j + 1 ];
            switch ( hdr -> rem )
            {
            case 2:
                dst [ outsize + 1 ] = u2 . s . b1;
            default:
                dst [ outsize + 0 ] = u2 . s . b0;
            }

            outsize += hdr -> rem;
        }
        break;
        
    default:
        /* straight eight bit */
        if ( dst_sz < ( size_t ) blk_sz )
            return tracedb_cmpNoBuffer;
        
        for ( j = 0; j < blk_sz; ++ j )
            dst [ outsize + j ] = ( int8_t ) blk -> data [ j ];
        
        outsize += blk_sz;
    }

    * dst_szp = outsize;
    return tracedb_cmpSuccess;
}
