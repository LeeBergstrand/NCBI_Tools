/*==============================================================================

  tracedb-priv.h

    private trace db definitions
    
 */


#ifndef _h_tracedb_priv_
#define _h_tracedb_priv_

#ifndef _h_tracedb_
#include "tracedb.h"
#endif

#include <endian.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRACEDB_VERSION
#define TRACEDB_VERSION 2
#endif

#ifndef TRACE_IDX
#if TRACEDB_VERSION == 1
#define TRACE_IDX "idx"
#elif TRACEDB_VERSION == 2
#define TRACE_IDX "idx2"
#endif
#endif

#define TRACEDB_CONCAT_VER( s, ver ) \
    s ## ver
#define TRACEDB_CVERSTR( s, ver ) \
    TRACEDB_CONCAT_VER ( s, ver )
#define TRACEDB_CVER( s ) \
    TRACEDB_CVERSTR ( s, TRACEDB_VERSION )

#define TRACEDB_CONCAT_VERMSG( s, ver, msg ) \
    s ## ver ## msg
#define TRACEDB_CVERSTRMSG( s, ver, msg ) \
    TRACEDB_CONCAT_VERMSG ( s, ver, msg )
#define TRACEDB_CVERMSG( s, msg ) \
    TRACEDB_CVERSTRMSG ( s, TRACEDB_VERSION, msg )


/*--------------------------------------------------------------------------
 * data file boundaries
 */
#define TRACEDB_FILEBITS 30 /* 1GB */
#define TRACEDB_FILESIZE ( 1U << TRACEDB_FILEBITS )
#define TRACEDB_FILEMASK ( TRACEDB_FILESIZE - 1 )
#define TRACEDB_FILENO( id ) ( unsigned int ) ( ( id ) >> TRACEDB_FILEBITS )
#define TRACEDB_FILEOFF( id ) ( uint64_t ) ( ( id ) & TRACEDB_FILEMASK )


/*--------------------------------------------------------------------------
 * index boundaries
 */
#define TRACEDB_IDXBITS 13 /* 8K */
#define TRACEDB_IDXSIZE ( 1U << TRACEDB_IDXBITS )
#define TRACEDB_IDXMASK ( TRACEDB_IDXSIZE - 1 )


/*--------------------------------------------------------------------------
 * VERBOSE
 *  logging macro
 */
#define TRACEDB_VERBOSE( lvl, db, x ) \
    ( ( lvl ) > ( db ) -> verbose ? 0 : ( printf x, fflush ( stdout ) ) )


/*--------------------------------------------------------------------------
 * TraceHdr
 *  a header object recorded immediately in front of a blob
 *
 *  ( id_hi << 8 ) + id_lo [ 0 ] => id # version 1
 *  ( id_hi << 8 ) + id_lo [ 3 ] => id # version 2+
 *
 *  version 1 used a small-endian layout with big-endian data.
 *  all fields are in network order for portability.
 *
 *  with general capitulation to little-endian processors on the part
 *  of major manufacturers, version 2 will be defined to have all fields
 *  in little-endian format.
 *
 *  it was originally thought that these headers would be used
 *  forensically, but now it appears that they may be accessed more
 *  frequently as part of ongoing integrity scans.
 */
typedef struct TraceHdr TraceHdr;
struct TraceHdr
{
    union
    {
        uint8_t id_low [ 4 ];
        uint32_t size;
    } u;
    uint32_t id_hi;
    uint32_t crc;
};

/* TraceHdrGetID
 *  return ti_t for blob
 */
#define TraceHdr1GetID( hdr ) \
    ( ( ( ( ti_t ) htonl ( ( long ) ( hdr ) . id_hi ) ) << 8 ) +   \
      ( hdr ) . u . id_low [ 0 ] )
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TraceHdr2GetID( hdr ) \
    ( ( * ( const ti_t* ) & ( hdr ) ) >> 24 )
#elif __BYTE_ORDER == __BIG_ENDIAN
#define TraceHdr2GetID( hdr ) \
    ( ( ( ( ti_t ) bswap_32 ( ( long ) ( hdr ) . id_hi ) ) << 8 ) +   \
      ( hdr ) . u . id_low [ 3 ] )
#else
#error "only supporting big and little endian"
#endif
#define TraceHdrGetID( hdr ) \
    TRACEDB_CVERMSG ( TraceHdr, GetID ) ( hdr )

/* TraceHdrGetSize
 *  return size_t for blob
 */
#define TraceHdr1GetSize( hdr ) \
        htonl ( ( long ) ( hdr ) . u . size )
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TraceHdr2GetSize( hdr ) \
    ( ( ( const TraceHdr* ) & ( hdr ) ) -> u . size & 0xFFFFFF )
#elif __BYTE_ORDER == __BIG_ENDIAN
#define TraceHdr2GetSize( hdr ) \
    ( bswap_32 ( ( long ) ( hdr ) . u . size ) & 0xFFFFFF )
#endif
#define TraceHdrGetSize( hdr ) \
    TRACEDB_CVERMSG ( TraceHdr, GetSize ) ( hdr )

/* TraceHdrGetCRC
 *  return crc for blob
 */
#define TraceHdr1GetCRC( hdr ) \
        htonl ( ( long ) ( hdr ) . crc )
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TraceHdr2GetCRC( hdr ) \
    ( ( const TraceHdr* ) & ( hdr ) ) -> crc
#elif __BYTE_ORDER == __BIG_ENDIAN
#define TraceHdr2GetCRC( hdr ) \
    bswap_32 ( ( long ) ( hdr ) . crc )
#endif
#define TraceHdrGetCRC( hdr ) \
    TRACEDB_CVERMSG ( TraceHdr, GetCRC ) ( hdr )


/*--------------------------------------------------------------------------
 * TraceIdx
 *  holds a 5 byte object locator ( oid ) and a 3 byte size
 *
 *  originally, the 5 byte object locator was taken from a lower
 *  layer of persistent memory management that used 8 byte offsets
 *  into a persistent heap, modified by 1 to distinguish between valid
 *  offsets and NULL ( i.e. the value 1 represented a valid heap
 *  offset of 0, while the value 0 represented NULL ).
 *
 *  TraceIdx1 uses the lower 5 bytes of such an offset as its
 *  object id, and retains the 1-based modification.
 *
 *  with version 2, the lower heap layer is no longer used, and
 *  the 8 byte offsets are generated directly. as for the issue of
 *  distinguishing NULL from offset 0, there can never be an offset
 *  of zero because before every blob is its header, while the
 *  value stored in the index is that of the blob. thus, the very
 *  first blob stored will have an index id of 12 due to the size
 *  of the header that precedes it.
 *
 *  finally, it should be obvious from the paragraph above that the
 *  blob offset/oid is meant to address the blob, not its  header.
 *  subtracting sizeof ( TraceHdr ) from that offset will give the
 *  offset to the corresponding header.
 */
typedef struct TRACEDB_CVER ( TraceIdx ) TraceIdx;
struct TraceIdx1
{
    uint32_t oid_hi;
    union
    {
        uint8_t oid_lo [ 4 ];
        uint32_t size;
    } u;
};

struct TraceIdx2
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    union
    {
        uint8_t oid_lo [ 4 ];
        uint32_t size;
    } u;
    uint32_t oid_hi;
#else
    uint32_t oid_hi;
    union
    {
        uint8_t oid_lo [ 4 ];
        uint32_t size;
    } u;
#endif
};

enum
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    TraceIdxIDLow = 3
#else
    TraceIdxIDLow = 0
#endif
};

/* TraceIdxGetOID
 *  retrieve oid from TraceIdx structure
 */
#define TraceIdx1GetOID( idx ) \
    ( ( ( ( uint64_t ) ( idx ) . oid_hi ) << 8 ) + \
    ( idx ) . u . oid_lo [ TraceIdxIDLow ] )
#define TraceIdx2GetOID( idx ) \
    ( ( * ( const uint64_t* ) & ( idx ) ) >> 24 )
#define TraceIdxGetOID( idx ) TRACEDB_CVERMSG ( TraceIdx, GetOID ) ( idx )

/* TraceIdxGetSize
 *  retrieve size from TraceIdx structure
 */
#define TraceIdx1GetSize( idx ) \
    ( ( idx ) . u . size & 0xFFFFFF )
#define TraceIdx2GetSize( idx ) \
    ( ( uint32_t ) ( * ( uint64_t* ) & ( idx ) ) & 0xFFFFFF )
#define TraceIdxGetSize( idx ) TRACEDB_CVERMSG ( TraceIdx, GetSize ) ( idx )


/*--------------------------------------------------------------------------
 * IdxHeader
 *  the index file begins with an endian indication and version number
 *  the indices immediately follow
 *
 *  "ti_start" is 1-based
 */
enum
{
    tracedb_eByteOrderTag = 0x05031988,
    tracedb_eByteOrderReverse = 0x88190305,
    tracedb_eCurrentVersion = TRACEDB_VERSION
};

typedef struct TRACEDB_CVER ( tracedb_IdxHeader ) tracedb_IdxHeader;
struct tracedb_IdxHeader1
{
    uint32_t endian;
    uint32_t version;
    ti_t ti_start;
};

struct tracedb_IdxHeader2
{
    uint32_t endian;
    uint32_t version;
    ti_t ti_start;
    ti_t ti_stop;
};

/* IdxHeaderGetIDStop
 *  determines last id
 */
#define tracedb_IdxHeader1GetIDStop( hdr, eof ) \
    ( uint64_t ) ( ( ( eof ) - sizeof ( tracedb_IdxHeader1 ) ) / sizeof ( TraceIdx ) )
#define tracedb_IdxHeader2GetIDStop( hdr, eof ) \
    ( ( const tracedb_IdxHeader2* ) ( hdr ) ) -> ti_stop
#define tracedb_IdxHeaderGetIDStop( hdr, eof ) \
    TRACEDB_CVERMSG ( tracedb_IdxHeader, GetIDStop ) ( hdr, eof )


/*--------------------------------------------------------------------------
 * TraceDB
 *  private interface
 */

typedef struct TraceDBHeaderInfo TraceDBHeaderInfo;
struct TraceDBHeaderInfo
{
    struct { uint64_t oid; size_t size; } idx;
    struct { ti_t id; size_t size; uint32_t crc; } hdr;
};

/* TraceDBReadHeaderInfo
 *  reads header information from a trace blob
 */
int TraceDBReadHeaderInfo ( TraceDB *db, const char *path, ti_t id,
    TraceDBHeaderInfo *info );


#ifdef __cplusplus
}
#endif

#endif /* _h_tracedb_priv_ */
