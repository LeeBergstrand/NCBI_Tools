#ifndef _h_trace_atomic_
#define _h_trace_atomic_

#ifndef _h_trace_atomic32_
#include "trace_atomic32.h"
#endif

#ifndef _h_trace_atomic64_
#include "trace_atomic64.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if TRACEDB_DFLT_ATOMIC_BITS == 32
#define TRACEDB_ATOMIC_NAME( suffix ) \
    tracedb_atomic32_ ## suffix
typedef int tracedb_atomic_int;
#else
#define TRACEDB_ATOMIC_NAME( suffix ) \
    tracedb_atomic64_ ## suffix
typedef long int tracedb_atomic_int;
#endif

typedef struct TRACEDB_ATOMIC_NAME ( t ) tracedb_atomic_t;

/* ( * v ) */
#define tracedb_atomic_read( v ) \
    TRACEDB_ATOMIC_NAME ( read ) ( v )

/* ( * v ) = i */
#define tracedb_atomic_set( v, i ) \
    TRACEDB_ATOMIC_NAME ( set ) ( v, i )

/* prior = ( * v ), ( * v ) += i, prior */
#define tracedb_atomic_read_and_add( v, i ) \
    TRACEDB_ATOMIC_NAME ( read_and_add ) ( v, i )

/* ( * v ) += i */
#define tracedb_atomic_add( v, i ) \
    TRACEDB_ATOMIC_NAME ( add ) ( v, i )

/* ( * v ) += i */
#define tracedb_atomic_add_and_read( v, i ) \
    TRACEDB_ATOMIC_NAME ( add_and_read ) ( v, i )

/* ( void ) ++ ( * v ) */
#define tracedb_atomic_inc( v ) \
    TRACEDB_ATOMIC_NAME ( inc ) ( v )

/* ( void ) -- ( * v ) */
#define tracedb_atomic_dec( v ) \
    TRACEDB_ATOMIC_NAME ( dec ) ( v )

/* -- ( * v ) == 0 */
#define tracedb_atomic_dec_and_test( v ) \
    TRACEDB_ATOMIC_NAME ( dec_and_test ) ( v )

/* ++ ( * v ) == 0
   when atomic_dec_and_test uses predecrement, you want
   postincrement to this function. so it isn't very useful */
#define tracedb_atomic_inc_and_test( v ) \
    TRACEDB_ATOMIC_NAME ( inc_and_test ) ( v )

/* ( * v ) -- == 0
   HERE's useful */
#define tracedb_atomic_test_and_inc( v ) \
    TRACEDB_ATOMIC_NAME ( test_and_inc ) ( v )

/* prior = ( * v ), ( * v ) = ( prior == t ? s : prior ), prior */
#define tracedb_atomic_test_and_set( v, s, t ) \
    TRACEDB_ATOMIC_NAME ( test_and_set ) ( v, s, t )

/* N.B. - THIS FUNCTION IS FOR 64 BIT PTRS ONLY */
static __inline__ void *tracedb_atomic_test_and_set_ptr ( void *volatile *v, void *s, void *t )
{
    void *rtn;
    __asm__ __volatile__
    (
        "lock;"
        "cmpxchg %%rdx,(%%rcx)"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( s ), "a" ( t )
    );
    return rtn;
}

/* val = ( * v ), ( ( * v ) = ( val < t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_lt( v, i, t ) \
    TRACEDB_ATOMIC_NAME ( read_and_add_lt ) ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( val <= t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_le( v, i, t ) \
    TRACEDB_ATOMIC_NAME ( read_and_add_le ) ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( val == t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_eq( v, i, t ) \
    TRACEDB_ATOMIC_NAME ( read_and_add_eq ) ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( val != t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_ne( v, i, t ) \
    TRACEDB_ATOMIC_NAME ( read_and_add_ne ) ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( val >= t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_ge( v, i, t ) \
    TRACEDB_ATOMIC_NAME ( read_and_add_ge ) ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( val > t ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_gt( v, i, t ) \
    TRACEDB_ATOMIC_NAME ( read_and_add_gt ) ( v, i, t )

/* val = ( * v ), ( ( * v ) = ( ( val & 1 ) == 1 ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_odd( v, i ) \
    TRACEDB_ATOMIC_NAME ( read_and_add_odd ) ( v, i )

/* val = ( * v ), ( ( * v ) = ( ( val & 1 ) == 0 ) ? val + i : val ), val */
#define tracedb_atomic_read_and_add_even( v, i ) \
    TRACEDB_ATOMIC_NAME ( read_and_add_even ) ( v, i )

/* DEPRECATED */

/* val = ( * v ), ( * v ) = ( val < t ? val + i : val ), ( val < t ? 1 : 0 ) */
#define tracedb_atomic_add_if_lt( v, i, t ) \
    TRACEDB_ATOMIC_NAME ( add_if_lt ) ( v, i, t )

/* val = ( * v ), ( * v ) = ( val <= t ? val + i : val ), ( val <= t ? 1 : 0 ) */
#define tracedb_atomic_add_if_le( v, i, t ) \
    TRACEDB_ATOMIC_NAME ( add_if_le ) ( v, i, t )

/* val = ( * v ), ( * v ) = ( val == t ? val + i : val ), ( val == t ? 1 : 0 ) */
#define tracedb_atomic_add_if_eq( v, i, t ) \
    TRACEDB_ATOMIC_NAME ( add_if_eq ) ( v, i, t )

/* val = ( * v ), ( * v ) = ( val >= t ? val + i : val ), ( val >= t ? 1 : 0 ) */
#define tracedb_atomic_add_if_ge( v, i, t ) \
    TRACEDB_ATOMIC_NAME ( add_if_ge ) ( v, i, t )

/* val = ( * v ), ( * v ) = ( val > t ? val + i : val ), ( val > t ? 1 : 0 ) */
#define tracedb_atomic_add_if_gt( v, i, t ) \
    TRACEDB_ATOMIC_NAME ( add_if_gt ) ( v, i, t )

#undef LOCK


#ifdef __cplusplus
}
#endif

#endif /* _h_trace_atomic_ */
