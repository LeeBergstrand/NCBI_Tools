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

#include <sra/xf.h>
#include <kfs/dyload.h>
#include <stdlib.h>

#define COMMON_SYMBOLS(X) X(INSDC_SEQ_rand_4na_2na) \
    X(NCBI_WGS_tokenize_nuc_accession) X(NCBI_WGS_tokenize_prot_accession) \
    X(NCBI_lower_case_tech_reads) X(NCBI_unpack) X(NCBI_unzip) \
    X(vdb_add_row_id) X(vdb_bit_or) X(vdb_bunzip) X(vdb_ceil) X(vdb_clip) \
    X(vdb_cut) X(vdb_delta) X(vdb_deriv) X(vdb_diff) X(vdb_echo) \
    X(vdb_exists) X(vdb_extract_token) X(vdb_floor) X(vdb_funzip) \
    X(vdb_integral) X(vdb_iunzip) X(vdb_izip) X(vdb_map) X(vdb_max) \
    X(vdb_min) X(vdb_outlier_decode) X(vdb_outlier_encode) X(vdb_pack) \
    X(vdb_paste) X(vdb_rldecode) X(vdb_round) X(vdb_simple_sub_select) \
    X(vdb_simple_sub_select_1) X(vdb_sprintf) X(vdb_strtonum) \
    X(vdb_subtract_row_id) X(vdb_sum) X(vdb_trim) X(vdb_trunc) \
    X(vdb_undelta) X(vdb_unpack) X(vdb_unzip) X(vdb_vec_sum)

#ifdef WUNIVERSE
#  define ALL_SYMBOLS(X) COMMON_SYMBOLS(X) \
    X(vdb_bzip) X(vdb_checksum) X(vdb_fzip) X(vdb_rlencode) X(vdb_zip)
#else
#  define ALL_SYMBOLS(X) COMMON_SYMBOLS(X)
#endif

#define AS_DECL(sym) extern void sym();
ALL_SYMBOLS(AS_DECL)

#define AS_VALUE(sym) sym,
static const xf_function_t s_vxf_functions[] = { ALL_SYMBOLS(AS_VALUE) NULL };

#define FOR_KDYLD(sym) KDyldRegisterBuiltin ( #sym, sym );

LIB_EXPORT const xf_function_t * CC register_vxf_functions(void)
{
    COMMON_SYMBOLS(FOR_KDYLD)
    return s_vxf_functions;
}

#ifdef WUNIVERSE
LIB_EXPORT const xf_function_t * CC register_wvxf_functions(void)
{
    ALL_SYMBOLS(FOR_KDYLD)
    return s_vxf_functions;
}
#endif
