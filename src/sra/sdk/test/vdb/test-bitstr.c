#include <bitstr.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>


static __inline__
int bitcmp_alt(const void *A, bitsz_t aoff, const void *B, bitsz_t boff, bitsz_t sz) {
    const uint8_t *a;
    const uint8_t *b;

    a = &((const uint8_t *)A)[aoff >> 3]; aoff &= 7;
    b = &((const uint8_t *)B)[boff >> 3]; boff &= 7;
    
    if (aoff == boff) {
        uint8_t lm = 0xFF >> aoff;
        uint8_t rm = 0xFF << ((8 - ((aoff + sz) & 7)) & 7);
        int diff;
        
        if (sz + aoff <= 8)
            return (int)(a[0] & lm & rm) - (b[0] & lm & rm);
        
        if (aoff) {
            diff = (int)(a[0] & lm) - (b[0] & lm);
            if (diff)
                return diff;
            ++a;
            ++b;
            sz -= 8 - aoff;
        }
        if (sz == 0)
            return 0;
        if (sz >= 8) {
            diff = memcmp(a, b, sz >> 3);
            if (diff || (sz & 7) == 0)
                return diff;
        }
        return (int)(a[sz >> 3] & rm) - (b[sz >> 3] & rm);
    }
    else {
        uint32_t ac = 0;                                    /* a accumulator            */
        unsigned abits = 0;                                 /* bits in a accumulator    */
        const bitsz_t enda = aoff + sz;                     /* end bit of a             */
        const uint8_t arm = 0xFF << ((8 - (enda & 7)) & 7); /* a right edge mask        */

        uint32_t bc = 0;                                    /* b accumulator            */
        unsigned bbits = 0;                                 /* bits in b accumulator    */
        const bitsz_t endb = boff + sz;                     /* end bit of b             */
        const uint8_t brm = 0xFF << ((8 - (endb & 7)) & 7); /* b right edge mask        */

        unsigned m;
        unsigned rem;
        uint32_t temp;
        
        m = enda < 8 ? arm : 0xFF;
        ac = (a[0] & m) << (aoff + 24);
        abits = 8 - aoff;
        if (abits > sz)
            abits = sz;
        aoff = 8;
        
        m = endb < 8 ? brm : 0xFF;
        bc = (b[0] & m) << (boff + 24);
        bbits = 8 - boff;
        if (bbits > sz)
            bbits = sz;
        boff = 8;
        
        do {
            while (aoff < enda && abits <= 24) {
                rem = enda - aoff;
                if (rem < 8) {
                    ac |= ((uint32_t)a[aoff >> 3] & arm) << (24 - abits);
                    aoff += rem;
                    abits += rem;
                    break;
                }
                else {
                    ac |= ((uint32_t)a[aoff >> 3]) << (24 - abits);
                    aoff += 8;
                    abits += 8;
                }
            }
            
            while (boff < endb && bbits <= 24) {
                rem = endb - boff;
                if (rem < 8) {
                    bc |= ((uint32_t)b[boff >> 3] & brm) << (24 - bbits);
                    boff += rem;
                    bbits += rem;
                    break;
                }
                else {
                    bc |= ((uint32_t)b[boff >> 3]) << (24 - bbits);
                    boff += 8;
                    bbits += 8;
                }
            }
            
            if (abits < bbits) {
                temp = bc & (0xFFFFFFFF << (32 - abits));
                if (ac < temp)
                    return -1;
                if (ac > temp)
                    return 1;
                
                bc <<= abits;
                bbits -= abits;
                ac = 0;
                abits = 0;
            }
            else if (abits > bbits) {
                temp = ac & (0xFFFFFFFF << (32 - bbits));
                if (temp < bc)
                    return -1;
                if (temp > bc)
                    return 1;
                
                ac <<= bbits;
                abits -= bbits;
                bc = 0;
                bbits = 0;
            }
            else {
                if (ac < bc)
                    return -1;
                if (ac > bc)
                    return 1;
                
                bc = 0;
                bbits = 0;
                ac = 0;
                abits = 0;
            }
        } while (aoff < enda || boff < endb);
        return 0;
    }
}

#define compare bitcmp
#define BITS 256

int test_compare(void) {
    uint8_t a[BITS / 8];
    uint8_t b[sizeof(a)];
    int i;
    int j;
    int fail = 0;
    
    for (i = 0; i != sizeof(a); ++i)
        a[i] = rand();
        
    memcpy(b, a, sizeof(a));
    for (i = 0; i != BITS; ++i) {
        if (compare(a, 0, b, 0, i + 1) != 0) {
            printf("AA != AA\n");
            fail = 1;
        }
    }    
    for (i = 0; i != BITS; ++i) {
        if (compare(a, i, b, i, BITS - i) != 0) {
            printf("AA != AA\n");
            fail = 1;
        }
    }    
    i = rand() % sizeof(a);
    j = rand() & 0xFF;
    
    b[i] ^= j;
    
    if (compare(a, 0, b, 0, BITS) == 0) {
        printf("AA == !AA\n");
        fail = 1;
    }
    
    a[i] ^= j;
    
    if (compare(a, 0, b, 0, BITS) != 0) {
        printf("AA ^ j != AA ^ j\n");
        fail = 1;
    }
    
    
    memset(a, 0xAA, sizeof(a));
    memset(b, 0x55, sizeof(a));
    if (compare(a, 0, b, 0, BITS) == 0) {
        printf("AA == 55\n");
        fail = 1;
    }
    if (compare(a, 0, b, 1, BITS - 1) != 0) {
        printf("AA != AA\n");
        fail = 1;
    }
    if (compare(a, 1, b, 0, BITS - 1) != 0) {
        printf("55 != 55\n");
        fail = 1;
    }

    memset(a, 0xFF, sizeof(b));
    for (i = 1; i <= BITS; ++i) {
        memset(b, 0x00, sizeof(b));
        memset(b, 0xFF, (i + 7) >> 3);
        for (j = 0; j != i; ++j) {
            if (compare(a, j, b, j, i - j) != 0) {
                printf("compare(a, %u, b, %u, %u) != 0\n", j, j, i - j);
                fail = 1;
            }
        }

        memset(b, 0x00, sizeof(b));
        for (j = 0; j + i < BITS; ++j) {
            memset(b, 0xFF, (i + j + 7) >> 3);
            if (compare(a, 0, b, j, i) != 0) {
                printf("compare(a, 0, b, %u, %u) != 0\n", j, i);
                fail = 1;
            }
            if (compare(a, j, b, 0, i) != 0) {
                printf("compare(a, %u, b, 0, %u) != 0\n", j, i);
                fail = 1;
            }
        }
    }
    
    return fail;
}

int test_copy(void) {
    uint8_t a[BITS / 8];
    uint8_t b[BITS / 8];
    uint8_t c[BITS / 8];
    int i;
    int j;

    memset(c, 0, sizeof(c));
    memset(a, 0xFF, sizeof(a));
    
    c[1] = 0xFF;
    
    memset(b, 0, sizeof(b));
    bitcpy(b, 8, a, 0, 8);
    
    if (memcmp(b, c, sizeof(b)) != 0) {
        printf("bitcpy failed; 8 bits set, offset 8\n");
        return 1;
    }
    
    c[2] = 0xFF;
    
    memset(b, 0, sizeof(b));
    bitcpy(b, 8, a, 0, 16);
    
    if (memcmp(b, c, sizeof(b)) != 0) {
        printf("bitcpy failed; 16 bits set, offset 8\n");
        return 1;
    }
    
    c[3] = 0xFF;
    
    memset(b, 0, sizeof(b));
    bitcpy(b, 8, a, 0, 24);
    
    if (memcmp(b, c, sizeof(b)) != 0) {
        printf("bitcpy failed; 24 bits set, offset 8\n");
        return 1;
    }
    
    c[4] = 0xFF;
    
    memset(b, 0, sizeof(b));
    bitcpy(b, 8, a, 0, 32);
    
    if (memcmp(b, c, sizeof(b)) != 0) {
        printf("bitcpy failed; 32 bits set, offset 8\n");
        return 1;
    }
    
    memset(c, 0xFF, sizeof(c));
    memset(a, 0x00, sizeof(a));
    
    c[1] = 0x00;
    
    memset(b, 0xFF, sizeof(b));
    bitcpy(b, 8, a, 0, 8);
    
    if (memcmp(b, c, sizeof(b)) != 0) {
        printf("bitcpy failed; 8 bits clear, offset 8\n");
        return 1;
    }
    
    c[2] = 0x00;
    
    memset(b, 0xFF, sizeof(b));
    bitcpy(b, 8, a, 0, 16);
    
    if (memcmp(b, c, sizeof(b)) != 0) {
        printf("bitcpy failed; 16 bits clear, offset 8\n");
        return 1;
    }
    
    c[3] = 0x00;
    
    memset(b, 0xFF, sizeof(b));
    bitcpy(b, 8, a, 0, 24);
    
    if (memcmp(b, c, sizeof(b)) != 0) {
        printf("bitcpy failed; 24 bits clear, offset 8\n");
        return 1;
    }
    
    c[4] = 0x00;
    
    memset(b, 0xFF, sizeof(b));
    bitcpy(b, 8, a, 0, 32);
    
    if (memcmp(b, c, sizeof(b)) != 0) {
        printf("bitcpy failed; 32 bits clear, offset 8\n");
        return 1;
    }
    
    memset(c, 0, sizeof(c));
    memset(a, 0xFF, sizeof(a));
    
    c[0] = 0xFF;
    
    for (i = 0; i != 32; ++i) {
        memset(b, 0, sizeof(b));
        bitcpy(b, 0, a, i, 8);
        
        if (memcmp(b, c, sizeof(b)) != 0) {
            printf("bitcpy failed; 8 bits set, offset %u -> offset 0\n", i);
            return 1;
        }
    }
    
    memset(c, 0, sizeof(c));
    memset(a, 0xFF, sizeof(a));
    
    c[1] = 0xFF;
    
    for (i = 0; i != 32; ++i) {
        memset(b, 0, sizeof(b));
        bitcpy(b, 8, a, i, 8);
        
        if (memcmp(b, c, sizeof(b)) != 0) {
            printf("bitcpy failed; 8 bits set, offset %u -> offset 8\n", i);
            return 1;
        }
    }
    
    memset(c, 0, sizeof(c));
    memset(a, 0xFF, sizeof(a));
    
    c[1] = 0x7F;
    c[2] = 0x80;
    
    for (i = 0; i != 32; ++i) {
        memset(b, 0, sizeof(b));
        bitcpy(b, 9, a, i, 8);
        
        if (memcmp(b, c, sizeof(b)) != 0) {
            printf("bitcpy failed; 8 bits set, offset %u -> offset 9\n", i);
            return 1;
        }
    }
    return 0;
}

int main(int argc, const char * argv[]) {
    srand(time(0));
    
    if (test_compare())
        return 1;
    if (test_copy())
        return 1;
    return 0;
}
