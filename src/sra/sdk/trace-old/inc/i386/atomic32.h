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
        "push %%ebx;"
        "mov (%%ecx), %%eax;"
        "mov %%edx, %%ebx;"
        "add %%eax, %%ebx;"
        "lock;"
        "cmpxchg %%ebx, (%%ecx);"
        "jne .-8;"
        "pop %%ebx"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( i )
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
        "push %%ebx;"
        "mov (%%ecx), %%eax;"
        "mov %%edx, %%ebx;"
        "add %%eax, %%ebx;"
        "lock;"
        "cmpxchg %%ebx, (%%ecx);"
        "jne .-8;"
        "mov %%ebx, %%eax;"
        "pop %%ebx"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( i )
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
        :"=m" (v->counter)
        :"m" (v->counter)
    );
}

static __inline__ void tracedb_atomic32_dec ( tracedb_atomic32_t *v )
{
    __asm__ __volatile__
    (
        "lock;"
        "decl %0"
        :"=m" (v->counter)
        :"m" (v->counter)
    );
}

/* decrement by one and test result for 0 */
static __inline__ int tracedb_atomic32_dec_and_test ( tracedb_atomic32_t *v )
{
    unsigned char c;
    __asm__ __volatile__
    (
        "lock;"
        "decl %0;"
        "sete %1"
        :"=m" (v->counter), "=qm" (c)
        :"m" (v->counter) : "memory"
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
        "incl %0;"
        "sete %1"
        :"=m" (v->counter), "=qm" (c)
        :"m" (v->counter) : "memory"
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
        "cmpxchg %%edx,(%%ecx)"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( s ), "a" ( t )
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
        "push %%ebx;"
        "mov (%%ecx), %%eax;"
        "cmp %%esi, %%eax;"
        "mov %%edx, %%ebx;"
        "jge .+10;"
        "add %%eax, %%ebx;"
        "lock;"
        "cmpxchg %%ebx, (%%ecx);"
        "jne .-12;"
        "pop %%ebx"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( i ), "S" ( t )
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
        "push %%ebx;"
        "mov (%%ecx), %%eax;"
        "cmp %%esi, %%eax;"
        "mov %%edx, %%ebx;"
        "jg .+10;"
        "add %%eax, %%ebx;"
        "lock;"
        "cmpxchg %%ebx, (%%ecx);"
        "jne .-12;"
        "pop %%ebx"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( i ), "S" ( t )
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
        "push %%ebx;"
        "mov (%%ecx), %%eax;"
        "cmp %%esi, %%eax;"
        "mov %%edx, %%ebx;"
        "jne .+10;"
        "add %%eax, %%ebx;"
        "lock;"
        "cmpxchg %%ebx, (%%ecx);"
        "jne .-12;"
        "pop %%ebx"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( i ), "S" ( t )
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
        "push %%ebx;"
        "mov (%%ecx), %%eax;"
        "cmp %%esi, %%eax;"
        "mov %%edx, %%ebx;"
        "je .+10;"
        "add %%eax, %%ebx;"
        "lock;"
        "cmpxchg %%ebx, (%%ecx);"
        "jne .-12;"
        "pop %%ebx"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( i ), "S" ( t )
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
        "push %%ebx;"
        "mov (%%ecx), %%eax;"
        "cmp %%esi, %%eax;"
        "mov %%edx, %%ebx;"
        "jl .+10;"
        "add %%eax, %%ebx;"
        "lock;"
        "cmpxchg %%ebx, (%%ecx);"
        "jne .-12;"
        "pop %%ebx"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( i ), "S" ( t )
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
        "push %%ebx;"
        "mov (%%ecx), %%eax;"
        "cmp %%esi, %%eax;"
        "mov %%edx, %%ebx;"
        "jle .+10;"
        "add %%eax, %%ebx;"
        "lock;"
        "cmpxchg %%ebx, (%%ecx);"
        "jne .-12;"
        "pop %%ebx"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( i ), "S" ( t )
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
        "push %%ebx;"
        "mov (%%ecx), %%eax;"
        "bt $0, %%ax;"
        "mov %%edx, %%ebx;"
        "jnc .+10;"
        "add %%eax, %%ebx;"
        "lock;"
        "cmpxchg %%ebx, (%%ecx);"
        "jne .-15;"
        "pop %%ebx"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( i )
    );
    return rtn;
}

static __inline__
int tracedb_atomic32_read_and_add_even ( tracedb_atomic32_t *v, int i )
{
    int rtn;
    __asm__ __volatile__
    (
        "push %%ebx;"
        "mov (%%ecx), %%eax;"
        "bt $0, %%ax;"
        "mov %%edx, %%ebx;"
        "jc .+10;"
        "add %%eax, %%ebx;"
        "lock;"
        "cmpxchg %%ebx, (%%ecx);"
        "jne .-15;"
        "pop %%ebx"
        : "=a" ( rtn ), "=c" ( v )
        : "c" ( v ), "d" ( i )
    );
    return rtn;
}


#ifdef __cplusplus
}
#endif

#endif /* _h_trace_atomic32_ */
