#ifndef _h_mmem_
#define _h_mmem_

#ifndef _h_itypes_
#include "itypes.h"
#endif

#include <stdlib.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * Memory
 *  abstract region of addressable memory
 */
typedef struct tracedb_Memory tracedb_Memory;
struct tracedb_Memory
{
    void *addr;
    size_t size;
};

/* MemoryInit
 *  initializes to NULL
 */
void tracedb_MemoryInit ( tracedb_Memory *mem );
#define MemoryInit( m ) \
    ( void ) ( ( m ) -> addr = NULL, ( m) -> size = 0 )

/* MemoryAlloc
 *  allocate some memory
 *
 *  in keeping with other naming conventions, this could be
 *  a "*Make" name, but it really neither makes nor creates.
 *
 *  "m" is an uninitialized Memory*
 *  "bytes" is a size_t
 */
int tracedb_MemoryAlloc ( tracedb_Memory *mem, size_t bytes );
#define tracedb_MemoryAlloc( m, bytes ) \
    ( ( ( m ) -> addr = malloc ( ( m ) -> size = ( bytes ) ) ) == NULL ? \
      errno : 0 )

/* MemoryWhack
 *  release memory
 *
 *  "m" is an initialized Memory*
 */
void tracedb_MemoryWhack ( tracedb_Memory *mem );
#define tracedb_MemoryWhack( m ) \
    ( ( m ) != NULL ? free ( ( m ) -> addr ) : ( void ) 0 )

/* MemorySubrgn
 *  creates a sub-region of an existing one
 *  note that the sub-region is always a non-allocated copy
 *  and is dependent upon the lifetime of its source
 *
 *  returns "sub" if "pos" was valid
 *  or NULL otherwise
 *
 *  "size" may be 0 to indicate infinite length
 *  or may extend beyond end of source region.
 */
tracedb_Memory *tracedb_MemorySubrgn ( const tracedb_Memory *mem,
    tracedb_Memory *sub, size_t size, size_t off );
#define tracedb_MemorySubrgn( m, s, sz, off ) \
    ( ( ( m ) == NULL || ( s ) == NULL || ( off ) >= ( m ) -> size ) ? NULL : \
      ( ( ( s ) -> addr = ( char* ) ( m ) -> addr + ( off ) ), \
        ( ( s ) -> size = ( m ) -> size - ( off ) ), \
        ( ( ( sz ) != 0 && ( sz ) < ( s ) -> size ) ? \
          ( void ) ( ( s ) -> size = ( sz ) ) : ( void ) 0 ), ( s ) ) )


/*--------------------------------------------------------------------------
 * ConstMemory
 *  abstract region of addressable read-only memory
 */
typedef struct tracedb_ConstMemory tracedb_ConstMemory;
struct tracedb_ConstMemory
{
    const void *addr;
    size_t size;
};

/* ConstMemoryCast
 *  casts a non-const structure to a const structure
 */
#define tracedb_ConstMemoryCast( m ) \
    ( tracedb_ConstMemory* ) ( m )

/* ConstMemoryWhack
 *  if you're supposed to whack memory, but it's const,
 *  do it here
 */
#define tracedb_ConstMemoryWhack( m ) \
    tracedb_MemoryWhack ( ( tracedb_Memory* ) ( m ) )

/* ConstMemorySubrgn
 *  creates a sub-region of an existing one
 *  note that the sub-region is always a non-allocated copy
 *  and is dependent upon the lifetime of its source
 *
 *  returns "sub" if "pos" was valid
 *  or NULL otherwise
 *
 *  "size" may be 0 to indicate infinite length
 *  or may extend beyond end of source region.
 */
tracedb_ConstMemory *tracedb_ConstMemorySubrgn ( const tracedb_ConstMemory *mem,
    tracedb_ConstMemory *sub, size_t size, size_t off );
#define tracedb_ConstMemorySubrgn( m, s, sz, off ) \
    ( ( ( m ) == NULL || ( s ) == NULL || ( off ) >= ( m ) -> size ) ? NULL : \
      ( ( ( s ) -> addr = ( const char* ) ( m ) -> addr + ( off ) ), \
        ( ( s ) -> size = ( m ) -> size - ( off ) ), \
        ( ( ( sz ) != 0 && ( sz ) < ( s ) -> size ) ? \
          ( void ) ( ( s ) -> size = ( sz ) ) : ( void ) 0 ), ( s ) ) )

#ifdef __cplusplus
}
#endif


#endif /* _h_mmem_ */
