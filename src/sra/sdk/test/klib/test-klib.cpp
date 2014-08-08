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

/**
* Unit tests for klib interface
*/

#include <ktst/unit_test.hpp>

#include <cstdlib>
#include <cstring>

#include <klib/sort.h>
#include <klib/printf.h>
#include <klib/data-buffer.h>
#include <klib/log.h>

#include <klib/text.h>

#include <common/test_assert.h>

using namespace std;

TEST_SUITE(KlibTestSuite);

///////////////////////////////////////////////// ksort 

static char UserData[]="User data";

// this is used by qsort and (indirectly) ksort
int CC baseCompare( const void* a, const void* b )
{
    const char* pa=(const char*)a;
    const char* pb=(const char*)b;
    if (*pa < *pb)
    {
        return -1;
    }
    if (*pa > *pb)
    {
        return 1;
    }
    return 0;
}
// this is used by ksort
int CC compare( const void* a, const void* b, void *data )
{
    // if data is not pointing where we waint it to, do not sort
    const char* pdata=(const char*)data;
    if (pdata != 0 && string_cmp(pdata, string_size(pdata), UserData, string_size(UserData), string_size(UserData)) != 0)
    {
        return 0;
    }
    return baseCompare(a, b);
}

TEST_CASE(KLib_ksort_no_elements)
{
    char arr[1]={13};
    ksort(arr, 0, sizeof (char), compare, UserData);
    // do not crash or change the target
    REQUIRE_EQ(arr[0], (char)13);
}
TEST_CASE(KLib_ksort_one_element)
{
    char arr[1]={13};
    ksort(arr, 1, sizeof (char), compare, UserData);
    // do not crash or change the target
    REQUIRE_EQ(arr[0], (char)13);
}

TEST_CASE(KLib_ksort_simple)
{
    const int Size=5;
    char arr[Size]={2, 4, 1, 0, 3};
    ksort(arr, Size, sizeof (char), compare, UserData);
    REQUIRE_EQ(arr[0], (char)0);
    REQUIRE_EQ(arr[1], (char)1);
    REQUIRE_EQ(arr[2], (char)2);
    REQUIRE_EQ(arr[3], (char)3);
    REQUIRE_EQ(arr[4], (char)4);
}
TEST_CASE(KLib_ksort_vs_qsort)
{
    const int Size=5;
    char karr[Size]={2, 4, 1, 0, 3};
    char qarr[Size];
    memcpy(qarr, karr, sizeof(karr));

    ksort(karr, Size, sizeof (char), compare, 0); // do not pass any user data
    qsort(qarr, Size, sizeof (char), baseCompare);
    REQUIRE_EQ(memcmp(karr, qarr, sizeof(karr)), 0);
}

// an example of a bad function (compating pointers not values, in reverse order) that causes an implementaion of ksort to crash.
int CC badCompare( const void* a, const void* b, void *data )
{
    const char* pa=(const char*)a;
    const char* pb=(const char*)b;
    if (pa < pb)
    {
        return 1;
    }
    if (pa > pb)
    {
        return -1;
    }
    return 0;
}
TEST_CASE(KLib_ksort_problem)
{
    const int Size=5;
    {
        char arr[Size]={2, 4, 1, 0, 3};
        ksort(arr, Size, sizeof (char), badCompare, 0);
        // we just do not want this to crash since the compare function is not working properly and the eventual sort order is undefined
    }
}

///////////////////////////////////////////////// macro based ksort 

TEST_CASE(KSORT_simple)
{
    char karr[]={2, 4, 1, 0, 3};
    const int ElemSize = sizeof(karr[0]);
    const int Size= sizeof(karr) / ElemSize;
    char qarr[Size];
    memcpy(qarr, karr, sizeof(karr));

#define CMP(a, b) (*(char*)(a) < *(char*)(b) ? -1 : *(char*)(a) > *(char*)(b))
#define SWAP(a, b, offset, size) 
    KSORT(karr, Size, ElemSize, 0, ElemSize);
    ksort(qarr, Size, ElemSize, compare, 0);
    REQUIRE_EQ(memcmp(karr, qarr, sizeof(karr)), 0);    
#undef CMP
#undef SWAP 
}

static
int CC cmp_int64_t ( const void *a, const void *b, void *data )
{
    const int64_t *ap = (const int64_t *)a;
    const int64_t *bp = (const int64_t *)b;

    if ( * ap < * bp )
        return -1;
    return * ap > * bp;
}

TEST_CASE(KSORT_int64)
{
    int64_t karr[]={2, 4, 1};
    const int ElemSize = sizeof(karr[0]);
    const int Size= sizeof(karr) / ElemSize;
    int64_t qarr[Size];
    memcpy(qarr, karr, sizeof(karr));

    ksort_int64_t (karr, Size);
    ksort(qarr, Size, ElemSize, cmp_int64_t , 0);
    REQUIRE_EQ(memcmp(karr, qarr, sizeof(karr)), 0);    
}


///////////////////////////////////////////////// string_printf
TEST_CASE(KLib_print_uint64)
{
    char dst[1024];
    size_t num_writ;
    uint64_t val=UINT64_C(1152921504606846976);
    REQUIRE_RC(string_printf ( dst, sizeof(dst), &num_writ, "%lu", val));
    REQUIRE_EQ(string( dst, num_writ), string("1152921504606846976"));
}

#ifndef WINDOWS
TEST_CASE(KLib_print_problem)
{
    char dst[1024];
    size_t num_writ;
    double d=1.0;
    REQUIRE_RC(string_printf ( dst, sizeof(dst), &num_writ, "%.6f", d));
    REQUIRE_EQ(num_writ, strlen(dst));
    REQUIRE_EQ(string("1.000000"), string(dst, num_writ));
}
#endif

///////////////////////////////////////////////// KDataBuffer

TEST_CASE(KDataBuffer_Make)
{
    KDataBuffer src;

    REQUIRE_RC(KDataBufferMake(&src, 13, 9876));

    REQUIRE(src.ignore != 0);
    REQUIRE(src.base != 0);
    REQUIRE_EQ((uint64_t)13, src.elem_bits);
    REQUIRE_EQ((uint64_t)9876, src.elem_count);
    REQUIRE_EQ((bitsz_t)(13 * 9876), KDataBufferBits(&src));
    REQUIRE_EQ((uint8_t)src.bit_offset, (uint8_t) 0 ); 

    KDataBufferWhack(&src );
}

TEST_CASE(KDataBuffer_MakeBytes)
{
    KDataBuffer src;

    REQUIRE_RC(KDataBufferMakeBytes(&src, 12));

    REQUIRE_EQ((uint64_t)8, src.elem_bits);
    REQUIRE_EQ((uint64_t)12, src.elem_count);
    REQUIRE_EQ((bitsz_t)(12*8), KDataBufferBits(&src));
    REQUIRE_EQ((size_t)12, KDataBufferBytes(&src));
    REQUIRE_EQ((uint8_t)src.bit_offset, (uint8_t) 0 ); 

    KDataBufferWhack(&src );
}

TEST_CASE(KDataBuffer_MakeBits)
{
    KDataBuffer src;

    REQUIRE_RC(KDataBufferMakeBits(&src, 8));

    REQUIRE_EQ((uint64_t)1, src.elem_bits);
    REQUIRE_EQ((uint64_t)8, src.elem_count);
    REQUIRE_EQ((bitsz_t)(1*8), KDataBufferBits(&src));
    REQUIRE_EQ((size_t)1, KDataBufferBytes(&src));
    REQUIRE_EQ((uint8_t)src.bit_offset, (uint8_t) 0 ); 

    KDataBufferWhack(&src );
}

TEST_CASE(KDataBuffer_MakeBits1)
{
    KDataBuffer src;
    const size_t BIT_SZ = 7896;
    REQUIRE_RC(KDataBufferMakeBits(&src, BIT_SZ));

    REQUIRE_EQ((uint64_t)1, src.elem_bits);
    REQUIRE_EQ((uint64_t)BIT_SZ, src.elem_count);
    REQUIRE_EQ((bitsz_t)(1*BIT_SZ), KDataBufferBits(&src));
    REQUIRE_EQ((size_t)((1 * BIT_SZ + 7) / 8), KDataBufferBytes(&src));
    REQUIRE_EQ((uint8_t)src.bit_offset, (uint8_t) 0 ); 

    KDataBufferWhack(&src );
}

TEST_CASE(KDataBuffer_Sub)
{
    KDataBuffer src;
    KDataBuffer sub;

    const size_t BIT_SZ = 7896;
    REQUIRE_RC(KDataBufferMakeBits(&src, BIT_SZ));


    REQUIRE_RC(KDataBufferSub(&src, &sub, 800, 900 ));

    REQUIRE_EQ(src.elem_bits, sub.elem_bits);
    REQUIRE_EQ((uint64_t)900, sub.elem_count);
    REQUIRE_EQ((bitsz_t)(1*900), KDataBufferBits(&sub));
    REQUIRE_EQ((uint8_t*)sub.base, (uint8_t*) src.base + ( 800 >> 3 ));
    REQUIRE_EQ((uint64_t)sub.bit_offset, (uint64_t) 0 ); 

    KDataBufferWhack(&src );
    KDataBufferWhack(&sub );
}

TEST_CASE(KDataBuffer_Sub1)
{
    KDataBuffer src;
    KDataBuffer sub;

    const size_t BIT_SZ = 7896;
    REQUIRE_RC(KDataBufferMakeBits(&src, BIT_SZ));


    REQUIRE_RC(KDataBufferSub(&src, &sub, 801, 900 ));

    REQUIRE_EQ(src.elem_bits, sub.elem_bits);
    REQUIRE_EQ((uint64_t)900, sub.elem_count);
    REQUIRE_EQ((bitsz_t)(1*900), KDataBufferBits(&sub));
    REQUIRE_EQ((uint8_t*)sub.base, (uint8_t*) src.base + ( 800 >> 3 ));
    REQUIRE_EQ((uint64_t)sub.bit_offset, (uint64_t)1); 

    KDataBufferWhack(&src );
    KDataBufferWhack(&sub );
}

TEST_CASE(KDataBuffer_MakeWritable)
{
    KDataBuffer src;
    KDataBuffer copy;

    REQUIRE_RC(KDataBufferMakeBytes(&src, 12));

    REQUIRE_RC(KDataBufferMakeWritable(&src, &copy));


    REQUIRE_EQ(src.elem_bits, copy.elem_bits);
    REQUIRE_EQ((uint64_t)12, copy.elem_count);
    REQUIRE_EQ(copy.base, src.base);
    REQUIRE_EQ((uint64_t)copy.bit_offset, (uint64_t)0); 

    KDataBufferWhack(&src);
    KDataBufferWhack(&copy);
}

TEST_CASE(KDataBuffer_MakeWritable1)
{
    KDataBuffer src;
    KDataBuffer sub;
    KDataBuffer copy;

    REQUIRE_RC(KDataBufferMakeBytes(&src, 256));

    REQUIRE_RC(KDataBufferSub(&src, &sub, 8, 12 ));

    REQUIRE_RC(KDataBufferMakeWritable(&sub, &copy));

    REQUIRE_EQ(src.elem_bits, copy.elem_bits);
    REQUIRE_EQ((uint64_t)12, sub.elem_count);
    REQUIRE_EQ((uint64_t)12, copy.elem_count);
    REQUIRE_NE(sub.base, copy.base);
    REQUIRE_EQ((uint64_t)copy.bit_offset, (uint64_t)0); 

    KDataBufferWhack(&src );
    KDataBufferWhack(&sub );
    KDataBufferWhack(&copy );
}

TEST_CASE(KDataBuffer_Resize)
{
    KDataBuffer src;
    uint32_t blob_size=4096;
    REQUIRE_RC(KDataBufferMake(&src, 8, blob_size));

    /* make sub-buffer from input */
    KDataBuffer dst;
    uint32_t hdr_size=7;
    REQUIRE_RC(KDataBufferSub ( &src, &dst, hdr_size, blob_size ));
    /* cast from 8 into 2 bits */
    REQUIRE_RC(KDataBufferCast ( &dst, &dst, 2, true ));
    /* resize to 4 times the original number of elements */
    REQUIRE_RC(KDataBufferResize ( &dst, (blob_size - hdr_size) * 4 ));

    KDataBufferWhack ( & dst );
    KDataBufferWhack ( & src );
}

//////////////////////////////////////////// Log
TEST_CASE(KLog_Formatting)
{
    unsigned long status = 161;
    REQUIRE_RC( pLogErr ( klogInfo, 0, "$(E) - $(C)", "E=%!,C=%u", status, status ) ); // fails on Windows
}

//////////////////////////////////////////// Main
extern "C"
{

#include <kapp/args.h>
#include <kfg/config.h>

ver_t CC KAppVersion ( void )
{
    return 0x1000000;
}
rc_t CC UsageSummary (const char * progname)
{
    return 0;
}

rc_t CC Usage ( const Args * args )
{
    return 0;
}

const char UsageDefaultName[] = "test-klib";

rc_t CC KMain ( int argc, char *argv [] )
{
    KConfigDisableUserSettings();
    rc_t rc=KlibTestSuite(argc, argv);
    return rc;
}

}
