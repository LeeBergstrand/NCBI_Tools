#include "nudec.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>


/*--------------------------------------------------------------------------
 * NucEncError
 *  error codes
 */

/* NucEncErrorName
 *  returns error name from NucEncError
 */
const char *tracedb_NucEncErrorName ( int err )
{
    switch ( err )
    {
    case tracedb_nucSuccess:
        return "nucSuccess";
    case tracedb_nucBadParam:
        return "nucBadParam";
    case tracedb_nucInvalid:
        return "nucInvalid";
    case tracedb_nucFailed:
        return "nucFailed";
    }

    return "<unknown>";
}

/* NucEncErrorStr
 *  returns error string from NucEncError
 */
const char *tracedb_NucEncErrorStr ( int err )
{
    switch ( err )
    {
    case tracedb_nucSuccess:
        return "operation was successful";
    case tracedb_nucBadParam:
        return "a user-supplied parameter was incorrect";
    case tracedb_nucInvalid:
        return "source data were corrupt or incomplete";
    case tracedb_nucFailed:
        return "the operation failed to achieve constraint goals";
    }

    return "<unknown>";
}


/*--------------------------------------------------------------------------
 * NucDec
 *  loves to decode
 */

/* NucDecInit
 *  initialize structure for decoding
 */
int tracedb_NucDecInit ( tracedb_NucDec *dec )
{
    int i, j;

    /* the set of legal, canonical ASCII nucleotide values */
    const char IUPACna [] = "ACMGRSVTWYHKDBN*";

    /* the set of legal combinations */
    const char conv4to2na [] = "\x00\x01\x03\x07"
        "\x10\x11\x13\x17\x30\x31\x33\x37\x70\x71\x73\x77";

    if ( dec == NULL )
        return tracedb_nucBadParam;

    /* initialize the 4 to 8 bit table */
    for ( i = 0; i < 16; ++ i )
    {
        union
        {
            uint16_t w;
            char ch [ 2 ];
        } u;

        int upper = i << 4;        
        u . ch [ 0 ] = IUPACna [ i ];

        for ( j = 0; j < 16; ++ j )
        {
            u . ch [ 1 ] = IUPACna [ j ];
            dec -> conv4to8na [ upper + j ] = u . w;
        }
    }

    /* initialize the 2 to 8 bit table */
    for ( i = 0; i < 16; ++ i )
    {
        union
        {
            uint32_t l;
            uint16_t w [ 2 ];
        } u;

        int upper = i << 4;

        u . w [ 0 ] = dec -> conv4to8na [ ( int ) conv4to2na [ i ] ];

        for ( j = 0; j < 16; ++ j )
        {
            u . w [ 1 ] = dec -> conv4to8na [ ( int ) conv4to2na [ j ] ];
            dec -> conv2to8na [ upper + j ] = u . l;
        }
    }

    return tracedb_nucSuccess;
}

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
    size_t *dst_szp, const uint8_t *src, size_t src_sz )
{
    size_t dst_sz;
    const tracedb_NucBlock *blk;

    unsigned int i, j;
    unsigned int outsize, block_len;

    if ( dec == NULL || dst_szp == NULL )
        return tracedb_nucBadParam;

    if ( src_sz == 0 )
    {
        * dst_szp = 0;
        return tracedb_nucSuccess;
    }

    if ( src_sz < 3 )
        return tracedb_nucInvalid;

    if ( src == NULL || dst == NULL || ( char* ) src == dst )
        return tracedb_nucBadParam;

    outsize = 0;
    dst_sz = * dst_szp;

    blk = ( const tracedb_NucBlock* ) & src [ 1 ];
    block_len = blk -> size + 1;

    /* process all but last block */
    for ( i = block_len + 2; i < ( unsigned int ) src_sz; i += block_len + 1 )
    {
        /* block is two bit */
        if ( blk -> two_bit )
        {
            union
            {
                uint32_t l;
                char ch [ 4 ];
            } u;

            for ( j = 0; j < block_len; ++ j )
            {
                u . l = dec -> conv2to8na [ blk -> data [ j ] ];

                if ( dst_sz < 4 )
                    return tracedb_nucFailed;

                dst [ outsize + 0 ] = u . ch [ 0 ];
                dst [ outsize + 1 ] = u . ch [ 1 ];
                dst [ outsize + 2 ] = u . ch [ 2 ];
                dst [ outsize + 3 ] = u . ch [ 3 ];

                outsize += 4;
                dst_sz -= 4;
            }
        }
        else
        {
            union
            {
                uint16_t w;
                char ch [ 2 ];
            } u;

            for ( j = 0; j < block_len; ++ j )
            {
                u . w = dec -> conv4to8na [ blk -> data [ j ] ];

                if ( dst_sz < 2 )
                    return tracedb_nucFailed;

                dst [ outsize + 0 ] = u . ch [ 0 ];
                dst [ outsize + 1 ] = u . ch [ 1 ];

                outsize += 2;
                dst_sz -= 2;
            }
        }

        /* next block */
        blk = ( const tracedb_NucBlock* ) & src [ i ];
        block_len = blk -> size + 1;
    }

    /* if the last block contains an odd number
       of source bytes, be careful on its processing */
    if ( src [ 0 ] != 0 )
        -- block_len;

    /* process last block */
    if ( blk -> two_bit )
    {
        union
        {
            uint32_t l;
            char ch [ 4 ];
        } u;

        /* process all bytes containing 4 values */
        for ( j = 0; j < block_len; ++ j )
        {
            u . l = dec -> conv2to8na [ blk -> data [ j ] ];
            
            if ( dst_sz < 4 )
                return tracedb_nucFailed;
            
            dst [ outsize + 0 ] = u . ch [ 0 ];
            dst [ outsize + 1 ] = u . ch [ 1 ];
            dst [ outsize + 2 ] = u . ch [ 2 ];
            dst [ outsize + 3 ] = u . ch [ 3 ];

            outsize += 4;
            dst_sz -= 4;
        }

        /* process last byte if it contains 1..3 values */
        if ( src [ 0 ] != 0 )
        {
            u . l = dec -> conv2to8na [ blk -> data [ j ] ];
            
            if ( dst_sz < src [ 0 ] )
                return tracedb_nucFailed;

            memcpy ( & dst [ outsize ], u . ch, src [ 0 ] );
            outsize += src [ 0 ];
        }
    }
    else
    {
        union
        {
            uint16_t w;
            char ch [ 2 ];
        } u;

        /* process all bytes containing 2 values */
        for ( j = 0; j < block_len; ++ j )
        {
            u . w = dec -> conv4to8na [ blk -> data [ j ] ];

            if ( dst_sz < 2 )
                return tracedb_nucFailed;

            dst [ outsize + 0 ] = u . ch [ 0 ];
            dst [ outsize + 1 ] = u . ch [ 1 ];

            outsize += 2;
            dst_sz -= 2;
        }

        /* process last byte if it contains a value */
        if ( src [ 0 ] != 0 )
        {
            u . w = dec -> conv4to8na [ blk -> data [ j ] ];
            
            if ( dst_sz < 1 )
                return tracedb_nucFailed;
            
            dst [ outsize ] = u . ch [ 0 ];
            ++ outsize;
        }
    }

    * dst_szp = outsize;

    return tracedb_nucSuccess;
}
