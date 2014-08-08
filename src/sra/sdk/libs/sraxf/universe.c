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

#define COMMON_SYMBOLS(X) \
    X(INSDC_SRA_format_spot_name) X(NCBI_SRA__454__untyped_0) \
    X(NCBI_SRA__454__untyped_1_2a) X(NCBI_SRA__454__untyped_1_2b) \
    X(NCBI_SRA_ABI_tokenize_spot_name) X(NCBI_SRA_ABI_untyped_1) \
    X(NCBI_SRA_Helicos_tokenize_spot_name) \
    X(NCBI_SRA_Illumina_tokenize_spot_name) \
    X(NCBI_SRA_Illumina_untyped_0a) X(NCBI_SRA_Illumina_untyped_0b) \
    X(NCBI_SRA_Illumina_untyped_1a) X(NCBI_SRA_Illumina_untyped_1b) \
    X(NCBI_SRA_IonTorrent_tokenize_spot_name) \
    X(NCBI_SRA_extract_name_coord) X(NCBI_SRA__454__dynamic_read_desc) \
    X(NCBI_SRA__454__process_position) X(NCBI_SRA__454__tokenize_spot_name) \
    X(NCBI_SRA_bio_start) X(NCBI_SRA_decode_CLIP) X(NCBI_SRA_decode_INTENSITY) \
    X(NCBI_SRA_decode_NOISE) X(NCBI_SRA_decode_POSITION) \
    X(NCBI_SRA_decode_QUALITY) X(NCBI_SRA_decode_READ) \
    X(NCBI_SRA_decode_SIGNAL) X(NCBI_SRA_denormalize) \
    X(NCBI_SRA_extract_coordinates) X(NCBI_SRA_fix_read_seg) \
    X(NCBI_SRA_lookup) X(NCBI_SRA_make_position) X(NCBI_SRA_make_read_desc) \
    X(NCBI_SRA_make_spot_desc) X(NCBI_SRA_normalize) \
    /* X(NCBI_SRA_prefix_tree_to_name) */ X(NCBI_SRA_qual4_decode) \
    X(NCBI_SRA_qual4_decompress_v1) X(NCBI_SRA_read_seg_from_readn) \
    X(NCBI_SRA_rewrite_spot_name) X(NCBI_SRA_rotate) X(NCBI_SRA_swap) \
    X(NCBI_color_from_dna) X(NCBI_dna_from_color) X(NCBI_fp_extend) \
    X(NCBI_var_tokenize_var_id) X(parse_decimal) X(parse_Q) \
    X(INSDC_SRA_format_spot_name_no_coord) 

#ifdef WUNIVERSE
#  define ALL_SYMBOLS(X) COMMON_SYMBOLS(X) \
    X(NCBI_SRA_extract_name_fmt) X(NCBI_SRA_extract_spot_name) \
    X(NCBI_SRA_phred_stats_trigger) X(NCBI_SRA_qual4_encode) \
    X(NCBI_SRA_stats_trigger)
#else
#  define ALL_SYMBOLS(X) COMMON_SYMBOLS(X)
#endif

#define AS_DECL(sym) extern void sym();
ALL_SYMBOLS(AS_DECL)

#define AS_VALUE(sym) sym,
static const xf_function_t s_sraxf_functions[] = { ALL_SYMBOLS(AS_VALUE) NULL };

#define FOR_KDYLD(sym) KDyldRegisterBuiltin ( #sym, sym );

LIB_EXPORT const xf_function_t * CC register_sraxf_functions(void)
{
    COMMON_SYMBOLS(FOR_KDYLD)
    return s_sraxf_functions;
}

#ifdef WUNIVERSE
LIB_EXPORT const xf_function_t * CC register_wsraxf_functions(void)
{
    ALL_SYMBOLS(FOR_KDYLD)
    return s_sraxf_functions;
}
#endif
