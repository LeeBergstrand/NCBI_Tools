#ifndef _h_trace_atomic64_
#define _h_trace_atomic64_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct tracedb_atomic64_t tracedb_atomic64_t;
struct tracedb_atomic64_t
{
    volatile long int counter;
};

/* int atomic64_read ( const atomic64_t *v ); */
#define tracedb_atomic64_read( v ) \
    ( ( v ) -> counter )

/* void atomic64_set ( atomic64_t *v, long int i ); */
#define tracedb_atomic64_set( v, i ) \
    ( ( void ) ( ( ( v ) -> counter ) = ( i ) ) )

/* add to v -> counter and return the prior value */
static __inline__ long int tracedb_atomic64_read_and_add ( tracedb_atomic64_t *v, long int i )
{
    long int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%rax;"
        "mov %%rsi, %%rcx;"
        "add %%rax, %%rcx;"
        "lock;"
        "cmpxchg %%rcx, (%%rdi);"
        "jne .-11"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i )
        : "%rcx"
    );
    return rtn;
}

/* if no read is needed, define the least expensive atomic add */
#define tracedb_atomic64_add( v, i ) \
    tracedb_atomic64_read_and_add ( v, i )

/* add to v -> counter and return the result */
static __inline__ long int tracedb_atomic64_add_and_read ( tracedb_atomic64_t *v, long int i )
{
    long int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%rax;"
        "mov %%rsi, %%rcx;"
        "add %%rax, %%rcx;"
        "lock;"
        "cmpxchg %%rcx, (%%rdi);"
        "jne .-11;"
        "mov %%rcx, %%rax"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i )
        : "%rcx"
    );
    return rtn;
}

/* just don't try to find out what the result was */
static __inline__ void tracedb_atomic64_inc ( tracedb_atomic64_t *v )
{
    __asm__ __volatile__
    (
        "lock;"
        "incq %0"
        : "=m" ( v -> counter )
        : "m" ( v -> counter )
    );
}

static __inline__ void tracedb_atomic64_dec ( tracedb_atomic64_t *v )
{
    __asm__ __volatile__
    (
        "lock;"
        "decq %0"
        : "=m" ( v -> counter )
        : "m" ( v -> counter )
    );
}

/* decrement by one and test result for 0 */
static __inline__ int tracedb_atomic64_dec_and_test ( tracedb_atomic64_t *v )
{
    unsigned char c;
    __asm__ __volatile__
    (
        "lock;"
        "decq %2;"
        "sete %%al"
        : "=a" ( c ), "=m" ( v -> counter )
        : "m" ( v -> counter )
    );
    return c;
}

/* when atomic64_dec_and_test uses predecrement, you want
   postincrement to this function. so it isn't very useful */
static __inline__ int tracedb_atomic64_inc_and_test ( tracedb_atomic64_t *v )
{
    unsigned char c;
    __asm__ __volatile__
    (
        "lock;"
        "incq %2;"
        "sete %%al"
        : "=a" ( c ), "=m" ( v -> counter )
        : "m" ( v -> counter )
    );
    return c;
}

/* HERE's useful */
#define tracedb_atomic64_test_and_inc( v ) \
    ( tracedb_atomic64_read_and_add ( v, 1L ) == 0 )

static __inline__ long int tracedb_atomic64_test_and_set ( tracedb_atomic64_t *v, long int s, long int t )
{
    long int rtn;
    __asm__ __volatile__
    (
        "lock;"
        "cmpxchg %%rsi,(%%rdi)"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( s ), "a" ( t )
    );
    return rtn;
}

/* conditional modifications */
static __inline__
long int tracedb_atomic64_read_and_add_lt ( tracedb_atomic64_t *v, long int i, long int t )
{
    long int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%rax;"
        "cmp %%rdx, %%rax;"
        "mov %%rsi, %%rcx;"
        "jge .+12;"
        "add %%rax, %%rcx;"
        "lock;"
        "cmpxchg %%rcx, (%%rdi);"
        "jne .-16"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%rcx"
    );
    return rtn;
}

#define tracedb_atomic64_add_if_lt( v, i, t ) \
    ( tracedb_atomic64_read_and_add_lt ( v, i, t ) < ( t ) )

static __inline__
long int tracedb_atomic64_read_and_add_le ( tracedb_atomic64_t *v, long int i, long int t )
{
    long int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%rax;"
        "cmp %%rdx, %%rax;"
        "mov %%rsi, %%rcx;"
        "jg .+12;"
        "add %%rax, %%rcx;"
        "lock;"
        "cmpxchg %%rcx, (%%rdi);"
        "jne .-16"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%rcx"
    );
    return rtn;
}

#define tracedb_atomic64_add_if_le( v, i, t ) \
    ( tracedb_atomic64_read_and_add_le ( v, i, t ) <= ( t ) )

static __inline__
long int tracedb_atomic64_read_and_add_eq ( tracedb_atomic64_t *v, long int i, long int t )
{
    long int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%rax;"
        "cmp %%rdx, %%rax;"
        "mov %%rsi, %%rcx;"
        "jne .+12;"
        "add %%rax, %%rcx;"
        "lock;"
        "cmpxchg %%rcx, (%%rdi);"
        "jne .-16"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%rcx"
    );
    return rtn;
}

#define tracedb_atomic64_add_if_eq( v, i, t ) \
    ( tracedb_atomic64_read_and_add_eq ( v, i, t ) == ( t ) )

static __inline__
long int tracedb_atomic64_read_and_add_ne ( tracedb_atomic64_t *v, long int i, long int t )
{
    long int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%rax;"
        "cmp %%rdx, %%rax;"
        "mov %%rsi, %%rcx;"
        "je .+12;"
        "add %%rax, %%rcx;"
        "lock;"
        "cmpxchg %%rcx, (%%rdi);"
        "jne .-16"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%rcx"
    );
    return rtn;
}

#define tracedb_atomic64_add_if_ne( v, i, t ) \
    ( tracedb_atomic64_read_and_add_ne ( v, i, t ) != ( t ) )

static __inline__
long int tracedb_atomic64_read_and_add_ge ( tracedb_atomic64_t *v, long int i, long int t )
{
    long int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%rax;"
        "cmp %%rdx, %%rax;"
        "mov %%rsi, %%rcx;"
        "jl .+12;"
        "add %%rax, %%rcx;"
        "lock;"
        "cmpxchg %%rcx, (%%rdi);"
        "jne .-16"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%rcx"
    );
    return rtn;
}

#define tracedb_atomic64_add_if_ge( v, i, t ) \
    ( tracedb_atomic64_read_and_add_ge ( v, i, t ) >= ( t ) )

static __inline__
long int tracedb_atomic64_read_and_add_gt ( tracedb_atomic64_t *v, long int i, long int t )
{
    long int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%rax;"
        "cmp %%rdx, %%rax;"
        "mov %%rsi, %%rcx;"
        "jle .+12;"
        "add %%rax, %%rcx;"
        "lock;"
        "cmpxchg %%rcx, (%%rdi);"
        "jne .-16"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i ), "d" ( t )
        : "%rcx"
    );
    return rtn;
}

#define tracedb_atomic64_add_if_gt( v, i, t ) \
    ( tracedb_atomic64_read_and_add_gt ( v, i, t ) > ( t ) )

static __inline__
long int tracedb_atomic64_read_and_add_odd ( tracedb_atomic64_t *v, long int i )
{
    long int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%rax;"
        "bt $0, %%ax;"
        "mov %%rsi, %%rcx;"
        "jnc .+12;"
        "add %%rax, %%rcx;"
        "lock;"
        "cmpxchg %%rcx, (%%rdi);"
        "jne .-18"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i )
        : "%rcx"
    );
    return rtn;
}

static __inline__
long int tracedb_atomic64_read_and_add_even ( tracedb_atomic64_t *v, long int i )
{
    long int rtn;
    __asm__ __volatile__
    (
        "mov (%%rdi), %%rax;"
        "bt $0, %%ax;"
        "mov %%rsi, %%rcx;"
        "jc .+12;"
        "add %%rax, %%rcx;"
        "lock;"
        "cmpxchg %%rcx, (%%rdi);"
        "jne .-18"
        : "=a" ( rtn )
        : "D" ( v ), "S" ( i )
        : "%rcx"
    );
    return rtn;
}

#ifdef __cplusplus
}
#endif

#endif /* _h_trace_atomic64_ */
