#ifndef _h_trace_atomic32_
#define _h_trace_atomic32_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct tracedb_atomic32_t tracedb_atomic32_t;
struct tracedb_atomic32_t
{
    volatile int counter;
};

/* int atomic32_read ( const atomic32_t *v ); */
#define tracedb_atomic32_read( v ) \
    ( ( v ) -> counter )

/* void atomic32_set ( atomic32_t *v, int i ); */
#define tracedb_atomic32_set( v, i ) \
    ( ( void ) ( ( ( v ) -> counter ) = ( i ) ) )

/* add to v -> counter and return the prior value */
static __inline__ int tracedb_atomic32_read_and_add ( tracedb_atomic32_t *v, int i )
{
    int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%eax;"
        "mov %%esi, %%ecx;"
        "add %%eax, %%ecx;"
        "lock;"
        "cmpxchg %%ecx, (%%rdi);"
        "jne .-8"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i )
        : "%ecx"
    );
    return rtn;
}

/* if no read is needed, define the least expensive atomic add */
#define tracedb_atomic32_add( v, i ) \
    tracedb_atomic32_read_and_add ( v, i )

/* add to v -> counter and return the result */
static __inline__ int tracedb_atomic32_add_and_read ( tracedb_atomic32_t *v, int i )
{
    int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%eax;"
        "mov %%esi, %%ecx;"
        "add %%eax, %%ecx;"
        "lock;"
        "cmpxchg %%ecx,(%%rdi);"
        "jne .-8;"
        "mov %%ecx, %%eax"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i )
        : "%ecx"
    );
    return rtn;
}

/* just don't try to find out what the result was */
static __inline__ void tracedb_atomic32_inc ( tracedb_atomic32_t *v )
{
    __asm__ __volatile__
    (
        "lock;"
        "incl %0"
        : "=m" ( v -> counter )
        : "m" ( v -> counter )
    );
}

static __inline__ void tracedb_atomic32_dec ( tracedb_atomic32_t *v )
{
    __asm__ __volatile__
    (
        "lock;"
        "decl %0"
        : "=m" ( v -> counter )
        : "m" ( v -> counter )
    );
}

/* decrement by one and test result for 0 */
static __inline__ int tracedb_atomic32_dec_and_test ( tracedb_atomic32_t *v )
{
    unsigned char c;
    __asm__ __volatile__
    (
        "lock;"
        "decl %2;"
        "sete %%al"
        : "=a" ( c ), "=m" ( v -> counter )
        : "m" ( v -> counter )
    );
    return c;
}

/* when atomic32_dec_and_test uses predecrement, you want
   postincrement to this function. so it isn't very useful */
static __inline__ int tracedb_atomic32_inc_and_test ( tracedb_atomic32_t *v )
{
    unsigned char c;
    __asm__ __volatile__
    (
        "lock;"
        "incl %2;"
        "sete %%al"
        : "=a" ( c ), "=m" ( v -> counter )
        : "m" ( v -> counter )
    );
    return c;
}

/* HERE's useful */
#define tracedb_atomic32_test_and_inc( v ) \
    ( tracedb_atomic32_read_and_add ( v, 1 ) == 0 )

static __inline__ int tracedb_atomic32_test_and_set ( tracedb_atomic32_t *v, int s, int t )
{
    int rtn;
    __asm__ __volatile__
    (
        "lock;"
        "cmpxchg %%esi,(%%rdi)"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( s ), "a" ( t )
    );
    return rtn;
}

/* conditional modifications */
static __inline__
int tracedb_atomic32_read_and_add_lt ( tracedb_atomic32_t *v, int i, int t )
{
    int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%eax;"
        "cmp %%edx, %%eax;"
        "mov %%esi, %%ecx;"
        "jge .+10;"
        "add %%eax, %%ecx;"
        "lock;"
        "cmpxchg %%ecx, (%%rdi);"
        "jne .-12"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%ecx"
    );
    return rtn;
}

#define tracedb_atomic32_add_if_lt( v, i, t ) \
    ( tracedb_atomic32_read_and_add_lt ( v, i, t ) < ( t ) )

static __inline__
int tracedb_atomic32_read_and_add_le ( tracedb_atomic32_t *v, int i, int t )
{
    int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%eax;"
        "cmp %%edx, %%eax;"
        "mov %%esi, %%ecx;"
        "jg .+10;"
        "add %%eax, %%ecx;"
        "lock;"
        "cmpxchg %%ecx, (%%rdi);"
        "jne .-12"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%ecx"
    );
    return rtn;
}

#define tracedb_atomic32_add_if_le( v, i, t ) \
    ( tracedb_atomic32_read_and_add_le ( v, i, t ) <= ( t ) )

static __inline__
int tracedb_atomic32_read_and_add_eq ( tracedb_atomic32_t *v, int i, int t )
{
    int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%eax;"
        "cmp %%edx, %%eax;"
        "mov %%esi, %%ecx;"
        "jne .+10;"
        "add %%eax, %%ecx;"
        "lock;"
        "cmpxchg %%ecx, (%%rdi);"
        "jne .-12"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%ecx"
    );
    return rtn;
}

#define tracedb_atomic32_add_if_eq( v, i, t ) \
    ( tracedb_atomic32_read_and_add_eq ( v, i, t ) == ( t ) )

static __inline__
int tracedb_atomic32_read_and_add_ne ( tracedb_atomic32_t *v, int i, int t )
{
    int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%eax;"
        "cmp %%edx, %%eax;"
        "mov %%esi, %%ecx;"
        "je .+10;"
        "add %%eax, %%ecx;"
        "lock;"
        "cmpxchg %%ecx, (%%rdi);"
        "jne .-12"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%ecx"
    );
    return rtn;
}

#define tracedb_atomic32_add_if_ne( v, i, t ) \
    ( tracedb_atomic32_read_and_add_ne ( v, i, t ) != ( t ) )

static __inline__
int tracedb_atomic32_read_and_add_ge ( tracedb_atomic32_t *v, int i, int t )
{
    int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%eax;"
        "cmp %%edx, %%eax;"
        "mov %%esi, %%ecx;"
        "jl .+10;"
        "add %%eax, %%ecx;"
        "lock;"
        "cmpxchg %%ecx, (%%rdi);"
        "jne .-12"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%ecx"
    );
    return rtn;
}

#define tracedb_atomic32_add_if_ge( v, i, t ) \
    ( tracedb_atomic32_read_and_add_ge ( v, i, t ) >= ( t ) )

static __inline__
int tracedb_atomic32_read_and_add_gt ( tracedb_atomic32_t *v, int i, int t )
{
    int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%eax;"
        "cmp %%edx, %%eax;"
        "mov %%esi, %%ecx;"
        "jle .+10;"
        "add %%eax, %%ecx;"
        "lock;"
        "cmpxchg %%ecx, (%%rdi);"
        "jne .-12"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%ecx"
    );
    return rtn;
}

#define tracedb_atomic32_add_if_gt( v, i, t ) \
    ( tracedb_atomic32_read_and_add_gt ( v, i, t ) > ( t ) )

static __inline__
int tracedb_atomic32_read_and_add_odd ( tracedb_atomic32_t *v, int i )
{
    int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%eax;"
        "bt $0, %%ax;"
        "mov %%esi, %%ecx;"
        "jnc .+10;"
        "add %%eax, %%ecx;"
        "lock;"
        "cmpxchg %%ecx, (%%rdi);"
        "jne .-15"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i )
        : "%ecx"
    );
    return rtn;
}

static __inline__
int tracedb_atomic32_read_and_add_even ( tracedb_atomic32_t *v, int i )
{
    int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%eax;"
        "bt $0, %%ax;"
        "mov %%esi, %%ecx;"
        "jc .+10;"
        "add %%eax, %%ecx;"
        "lock;"
        "cmpxchg %%ecx, (%%rdi);"
        "jne .-15"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i )
        : "%ecx"
    );
    return rtn;
}

#ifdef __cplusplus
}
#endif

#endif /* _h_trace_atomic32_ */
