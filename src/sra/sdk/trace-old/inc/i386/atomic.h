#ifndef _h_trace_atomic_
#define _h_trace_atomic_

#ifndef _h_trace_atomic32_
#include "trace_atomic32.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tracedb_atomic32_t tracedb_atomic_t;

/* ( * v ) */
#define tracedb_atomic_read( v ) \
    tracedb_atomic32_read ( v )

/* ( * v ) = i */
#define tracedb_tracedb_atomic_set( v, i ) \
    tracedb_atomic32_set ( v, i )

/* prior = ( * v ), ( * v ) += i, prior */
#define tracedb_atomic_read_and_add( v, i ) \
    tracedb_atomic32_read_and_add ( v, i )

/* ( * v ) += i */
#define tracedb_atomic_add( v, i ) \
    tracedb_atomic32_add ( v, i )

/* ( * v ) += i */
#define tracedb_atomic_add_and_read( v, i ) \
    tracedb_atomic32_add_and_read ( v, i )

/* ( void ) ++ ( * v ) */
#define tracedb_atomic_inc( v ) \
    tracedb_atomic32_inc ( v )

/* ( void ) -- ( * v ) */
#define tracedb_atomic_dec( v ) \
    tracedb_atomic32_dec ( v )

/* -- ( * v ) == 0 */
#define tracedb_atomic_dec_and_test( v ) \
    tracedb_atomic32_dec_and_test ( v )

/* ++ ( * v ) == 0
   when atomic_dec_and_test uses predecrement, you want
   postincrement to this function. so it isn't very useful */
#define tracedb_atomic_inc_and_test( v ) \
    tracedb_atomic32_inc_and_test ( v )

/* ( * v ) -- == 0
   HERE's useful */
#define tracedb_atomic_test_and_inc( v ) \
    tracedb_atomic32_test_and_inc ( v )

/* prior = ( * v ), ( * v ) = ( prior == t ? s : prior ), prior */
#define tracedb_atomic_test_and_set( v, s, t ) \
    tracedb_atomic32_test_and_set ( v, s, t )

/* N.B. - THIS FUNCTION IS FOR 32 BIT PTRS ONLY */
static __inline__ void *tracedb_atomic_test_and_set_ptr ( void *volatile *v, void *s, void *t )
{
    void *rtn;
    __asm__ __volatile__
    (
        "lock;"
        "cmpxchg %%edx,(%%ecx)"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( s ), "a" ( t )
    );
    return rtn;
}

/* val = ( * v ), ( ( * v ) = ( val < t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_lt( v, i, t ) \
    tracedb_atomic32_read_and_add_lt ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( val <= t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_le( v, i, t ) \
    tracedb_atomic32_read_and_add_le ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( val == t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_eq( v, i, t ) \
    tracedb_atomic32_read_and_add_eq ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( val != t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_ne( v, i, t ) \
    tracedb_atomic32_read_and_add_ne ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( val >= t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_ge( v, i, t ) \
    tracedb_atomic32_read_and_add_ge ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( val > t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_gt( v, i, t ) \
    tracedb_atomic32_read_and_add_gt ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( ( val & 1 ) == 1 ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_odd( v, i ) \
    tracedb_atomic32_read_and_add_odd ( v, i )

/* val = ( * v ), ( ( * v ) = ( ( val & 1 ) == 0 ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_even( v, i ) \
    tracedb_atomic32_read_and_add_even ( v, i )

/* DEPRECATED */

/* val = ( * v ), ( * v ) = ( val < t ? val + i : val ), ( val < t ? 1 : 0 ) */
#define tracedb_atomic_add_if_lt( v, i, t ) \
    tracedb_atomic32_add_if_lt ( v, i, t )

/* val = ( * v ), ( * v ) = ( val <= t ? val + i : val ), ( val <= t ? 1 : 0 ) */
#define tracedb_atomic_add_if_le( v, i, t ) \
    tracedb_atomic32_add_if_le ( v, i, t )

/* val = ( * v ), ( * v ) = ( val == t ? val + i : val ), ( val == t ? 1 : 0 ) */
#define tracedb_atomic_add_if_eq( v, i, t ) \
    tracedb_atomic32_add_if_eq ( v, i, t )

/* val = ( * v ), ( * v ) = ( val >= t ? val + i : val ), ( val >= t ? 1 : 0 ) */
#define tracedb_atomic_add_if_ge( v, i, t ) \
    tracedb_atomic32_add_if_ge ( v, i, t )

/* val = ( * v ), ( * v ) = ( val > t ? val + i : val ), ( val > t ? 1 : 0 ) */
#define tracedb_atomic_add_if_gt( v, i, t ) \
    tracedb_atomic32_add_if_gt ( v, i, t )

#undef LOCK

#ifdef __cplusplus
}
#endif

#endif /* _h_trace_atomic_ */
