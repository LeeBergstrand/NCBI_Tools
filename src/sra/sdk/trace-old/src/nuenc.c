#include "nuenc.h"
#include "logging.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>


/*--------------------------------------------------------------------------
 * NucEnc
 *  loves to encode
 *
 *  initializes tables
 */

int tracedb_NucEncInit ( tracedb_NucEnc *enc )
{
    int i;

    /* the set of legal, canonical ASCII nucleotide values */
    const char IUPACna [] = "ACMGRSVTWYHKDBN";

    /* the set of legal combinations */
    const char conv4to2na [] = "\x00\x01\x03\x07"
        "\x10\x11\x13\x17\x30\x31\x33\x37\x70\x71\x73\x77";

    if ( enc == NULL )
        return tracedb_nucBadParam;

    /* initialize the canonical input table */
    memset ( enc -> IUPACna, -1, sizeof enc -> IUPACna );
    for ( i = 0; i < sizeof IUPACna - 1; ++ i )
    {
        int ch = IUPACna [ i ];
        enc -> IUPACna [ ch ] = enc -> IUPACna [ ch + 'a' - 'A' ] = ( int8_t ) i;
    }

    /* allow some pseudo codes */
    enc -> IUPACna [ ( int ) '1' ] = enc -> IUPACna [ ( int ) 'A' ];
    enc -> IUPACna [ ( int ) '2' ] = enc -> IUPACna [ ( int ) 'C' ];
    enc -> IUPACna [ ( int ) '3' ] = enc -> IUPACna [ ( int ) 'T' ];
    enc -> IUPACna [ ( int ) '4' ] = enc -> IUPACna [ ( int ) 'G' ];

    /* all input may be represented in 4 bit codes.
       much may be represented as 2 bit codes. this
       table provides both the mapping and validation

       valid combinations representable in 2 bit are:

       AA CA GA TA | 00 10 30 70
       AC CC GC TC | 01 11 31 71
       AG CG GG TG | 03 13 33 73
       AT CT GT TT | 07 17 37 77
    */
    memset ( enc -> conv4to2na, -1, sizeof enc -> conv4to2na );
    for ( i = 0; i < sizeof conv4to2na - 1; ++ i )
        enc -> conv4to2na [ ( int ) conv4to2na [ i ] ] = i;

    return tracedb_nucSuccess;
}


/* NucEncEncode
 *  encodes our guy
 */
int tracedb_NucEncEncode ( const tracedb_NucEnc *enc, uint8_t *dst,
    const char *srcp, size_t *sizep )
{
    /* ensure that input characters are unsigned */
    const unsigned char *src = ( const unsigned char* ) srcp;

    /* compressed block */
    tracedb_NucBlock *blk;

    /* buffer size */
    size_t size;

    /* block and output size */
    unsigned int i, block_len, outsize;

    /* working variables */
    int v0, v1, v2, v3, v4, v5;

    /* must have encoder and size */
    if ( enc == NULL || sizep == NULL )
        return tracedb_nucBadParam;

    /* if input size is 0, then there's nothing to do */
    size = * sizep;
    if ( size == 0 )
        return tracedb_nucSuccess;

    /* if size is not zero, must have buffers */
    if ( src == NULL || dst == NULL || ( uint8_t* ) src == dst )
        return tracedb_nucBadParam;

    /* with a single byte header and 1 byte block
       headers, a 4 byte input is the break even point.
       for compression, we need beyond 4 bytes */
    if ( size < 4 )
        return tracedb_nucFailed;

    /* record the unaligned/trailing byte count */
    dst [ 0 ] = ( uint8_t ) ( size & 3 );

    /* work on 4 input bytes at a time */
    for ( block_len = 0, outsize = 1, i = 3;
          i < ( unsigned int ) size; i += 4 )
    {
        /* read and validate 4 input values */
        v0 = enc -> IUPACna [ src [ i - 3 ] ];
        v1 = enc -> IUPACna [ src [ i - 2 ] ];
        v2 = enc -> IUPACna [ src [ i - 1 ] ];
        v3 = enc -> IUPACna [ src [ i ] ];
        if ( v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0 )
            return tracedb_nucInvalid;

        /* convert to 4 bit representation */
        v4 = ( v0 << 4 ) | v1;
        v5 = ( v2 << 4 ) | v3;

        /* try 2 bit representation */
        v0 = enc -> conv4to2na [ v4 ];
        v1 = enc -> conv4to2na [ v5 ];
        if ( v0 < 0 || v1 < 0 )
        {
            /* must use 4 bit */
            if ( block_len == 0 )
            {
                /* create new block */
                blk = ( tracedb_NucBlock* ) & dst [ outsize ];
                blk -> two_bit = 0;
                ++ outsize;
            }
            else if ( blk -> two_bit )
            {
                /* close old block */
                blk -> size = ( uint8_t ) block_len - 1;

                /* create new block */
                blk = ( tracedb_NucBlock* ) & dst [ outsize ];
                block_len = 0;
                blk -> two_bit = 0;
                ++ outsize;
            }

            /* slam 4 nibbles in there */
            blk -> data [ block_len ] = v4;
            blk -> data [ block_len + 1 ] = v5;

            /* account for the bytes */
            outsize += 2;

            /* check for full block
               note that block_len is always even */
            if ( ( block_len += 2 ) == 128 )
            {
                /* close block */
                blk -> size = 128 - 1;
                block_len = 0;
            }
        }
        else
        {
            /* can use 2 bit */
            if ( block_len == 0 )
            {
                /* create new block */
                blk = ( tracedb_NucBlock* ) & dst [ outsize ];
                blk -> two_bit = 1;
                ++ outsize;
            }
            else if ( ! blk -> two_bit )
            {
                /* close old block */
                blk -> size = ( uint8_t ) block_len - 1;

                /* create new block */
                blk = ( tracedb_NucBlock* ) & dst [ outsize ];
                block_len = 0;
                blk -> two_bit = 1;
                ++ outsize;
            }

            /* slam 4 half-nibbles in there */
            blk -> data [ block_len ] = ( v0 << 4 ) | v1;

            /* account for the byte */
            ++ outsize;

            /* check for full block */
            if ( ++ block_len == 128 )
            {
                /* close block */
                blk -> size = 128 - 1;
                block_len = 0;
            }
        }
    }

    /* work on remaining 0..3 bytes */
    i -= 3;
    if ( dst [ 0 ] != 0 )
    {
        v1 = v2 = v3 = 0;
        switch ( dst [ 0 ] )
        {
        case 3:
            v2 = enc -> IUPACna [ src [ i + 2 ] ];
            if ( v2 < 0 )
                return tracedb_nucInvalid;
        case 2:
            v1 = enc -> IUPACna [ src [ i + 1 ] ];
            if ( v1 < 0 )
                return tracedb_nucInvalid;
        default:
            v0 = enc -> IUPACna [ src [ i + 0 ] ];
            if ( v0 < 0 )
                return tracedb_nucInvalid;
        }

        v4 = ( v0 << 4 ) | v1;
        v5 = ( v2 << 4 ) | v3;

        v0 = enc -> conv4to2na [ v4 ];
        v1 = enc -> conv4to2na [ v5 ];
        if ( v0 < 0 || v1 < 0 )
        {
            /* must use 4 bit */
            if ( block_len == 0 )
            {
                /* create new block */
                assert ( outsize < ( unsigned int ) size );
                blk = ( tracedb_NucBlock* ) & dst [ outsize ];
                blk -> two_bit = 0;
                ++ outsize;
            }
            else if ( blk -> two_bit )
            {
                /* close old block */
                blk -> size = ( uint8_t ) block_len - 1;

                /* create new block */
                assert ( outsize < ( unsigned int ) size );
                blk = ( tracedb_NucBlock* ) & dst [ outsize ];
                block_len = 0;
                blk -> two_bit = 0;
                ++ outsize;
            }

            /* slam nibbles in there */
            assert ( outsize < ( unsigned int ) size );
            blk -> data [ block_len ] = ( uint8_t ) v4;
            ++ outsize;
            ++ block_len;

            if ( dst [ 0 ] > 2 )
            {
                assert ( outsize < ( unsigned int ) size );
                blk -> data [ block_len ] = ( uint8_t ) v5;
                ++ outsize;
                ++ block_len;
            }

            /* correct extra guys */
            dst [ 0 ] &= 1;
        }
        else
        {
            /* can use 2 bit */
            if ( block_len == 0 )
            {
                /* create new block */
                assert ( outsize < ( unsigned int ) size );
                blk = ( tracedb_NucBlock* ) & dst [ outsize ];
                blk -> two_bit = 1;
                ++ outsize;
            }
            else if ( ! blk -> two_bit )
            {
                /* close old block */
                blk -> size = ( uint8_t ) block_len - 1;

                /* create new block */
                assert ( outsize < ( unsigned int ) size );
                blk = ( tracedb_NucBlock* ) & dst [ outsize ];
                block_len = 0;
                blk -> two_bit = 1;
                ++ outsize;
            }

            /* slam up to 4 half-nibbles in there */
            assert ( outsize < ( unsigned int ) size );
            blk -> data [ block_len ] = ( v0 << 4 ) | v1;
            ++ outsize;
            ++ block_len;
        }
    }

    /* close an open block */
    if ( block_len != 0 )
        blk -> size = ( uint8_t ) block_len - 1;

    * sizep = outsize;
    return tracedb_nucSuccess;
}
