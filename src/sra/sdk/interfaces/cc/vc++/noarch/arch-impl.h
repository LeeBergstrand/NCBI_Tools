/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#ifndef _h_noarch_arch_impl_
#define _h_noarch_arch_impl_

#include <stdint.h>

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* this table is very simple to calculate
   but simpler yet to use for lookup */
static const int8_t lsbit_map [] =
{
    -1, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
     4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

static __inline
int16_t uint16_lsbit ( uint16_t self )
{
    /* detect no bits are set */
    if ( self == 0 )
        return -1;

    /* detect bits set in lower byte */
    if ( ( uint8_t ) self != 0 )
        return lsbit_map [ ( uint8_t ) self ];

    /* return bit set in upper byte */
    return lsbit_map [ self >> 8 ] + 8;
}

static __inline
int32_t uint32_lsbit ( uint32_t self )
{
    /* detect no bits are set */
    if ( self == 0 )
        return -1;

    /* detect bits set in lower word */
    if ( ( uint16_t ) self != 0 )
        return uint16_lsbit ( ( uint16_t ) self );

    /* return bit set in upper word */
    return uint16_lsbit ( self >> 16 ) + 16;
}

typedef struct int128_t int128_t;
struct int128_t
{
    uint64_t lo;
    int64_t hi;
};

static __inline
int64_t int128_hi ( const int128_t *self )
{
    return self -> hi;
}

static __inline
uint64_t int128_lo ( const int128_t *self )
{
    return self -> lo;
}

static __inline
void int128_sethi ( int128_t *self, int64_t i )
{
    self -> hi = i;
}

static __inline
void int128_setlo ( int128_t *self, uint64_t i )
{
    self -> lo = i;
}

typedef struct uint128_t uint128_t;
struct uint128_t
{
    uint64_t lo;
    uint64_t hi;
};

static __inline
uint64_t uint128_hi ( const uint128_t *self )
{
    return self -> hi;
}

static __inline
uint64_t uint128_lo ( const uint128_t *self )
{
    return self -> lo;
}

static __inline
void uint128_sethi ( uint128_t *self, uint64_t i )
{
    self -> hi = i;
}

static __inline
void uint128_setlo ( uint128_t *self, uint64_t i )
{
    self -> lo = i;
}

static __inline
void int128_add ( int128_t *self, const int128_t *i )
{
    uint64_t carry = ( ( const uint32_t* ) self ) [ 0 ] + ( ( const uint32_t* ) i ) [ 0 ];
    self -> hi += i -> hi;
    carry = ( ( const uint32_t* ) self ) [ 1 ] + ( ( const uint32_t* ) i ) [ 1 ] + ( carry >> 32 );
    self -> lo += i -> lo;
    self -> hi += carry >> 32;
}

static __inline
void int128_sub ( int128_t *self, const int128_t *i )
{
    int carry = i -> lo > self -> lo;
    self -> hi -= i -> hi;
    self -> lo -= i -> lo;
    self -> hi -= carry;
}

static __inline
void int128_sar ( int128_t *self, uint32_t i )
{
    if ( i < 64 )
    {
        self -> lo = ( self -> hi << ( 64 - i ) ) |  ( self -> lo >> i );
        self -> hi >>= i;
    }
    else
    {
        self -> lo = self -> hi >> ( i - 64 );
        self -> hi >>= 63;
    }
}

static __inline
void int128_shl ( int128_t *self, uint32_t i )
{
    if ( i < 64 )
    {
        self -> hi = ( self -> hi << i ) | ( int64_t ) ( self -> lo >> ( 64 - i ) );
        self -> lo <<= i;
    }
    else
    {
        self -> hi = ( int64_t ) ( self -> lo << ( i - 64 ) );
        self -> lo = 0;
    }
}

static __inline
void uint128_and ( uint128_t *self, const uint128_t *i )
{
    self -> lo &= i -> lo;
    self -> hi &= i -> hi;
}

static __inline
void uint128_or ( uint128_t *self, const uint128_t *i )
{
    self -> lo |= i -> lo;
    self -> hi |= i -> hi;
}

static __inline
void uint128_orlo ( uint128_t *self, uint64_t i )
{
    self -> lo |= i;
}

static __inline
void uint128_xor ( uint128_t *self, const uint128_t *i )
{
    self -> lo ^= i -> lo;
    self -> hi ^= i -> hi;
}

static __inline
void uint128_not ( uint128_t *self )
{
    self -> lo = ~ self -> lo;
    self -> hi = ~ self -> hi;
}

static __inline
void uint128_shr ( uint128_t *self, uint32_t i )
{
    if ( i < 64 )
    {
        self -> lo = ( self -> hi << ( 64 - i ) ) |  ( self -> lo >> i );
        self -> hi >>= i;
    }
    else
    {
        self -> lo = self -> hi >> ( i - 64 );
        self -> hi >>= 63;
    }
}

static __inline
void uint128_shl ( uint128_t *self, uint32_t i )
{
    if ( i < 64 )
    {
        self -> hi = ( self -> hi << i ) | ( self -> lo >> ( 64 - i ) );
        self -> lo <<= i;
    }
    else
    {
        self -> hi = self -> lo << ( i - 64 );
        self -> lo = 0;
    }
}

static __inline
void uint128_bswap ( uint128_t *self )
{
    uint64_t tmp = _byteswap_uint64 ( self -> lo );
    self -> lo = _byteswap_uint64 ( self -> hi );
    ( ( uint64_t* ) self ) [ 1 ] = tmp;
}

static __inline
void uint128_bswap_copy ( uint128_t *to, const uint128_t *from )
{
    to -> lo = _byteswap_uint64 ( from -> hi );
    to -> hi = _byteswap_uint64 ( from -> lo );
}

#ifdef __cplusplus
}
#endif

#endif /* _h_noarch_arch_impl_ */
