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

#define COMMON_SYMBOLS(X) X(ALIGN_align_restore_read) X(ALIGN_cigar) \
  X(ALIGN_cigar_2) X(ALIGN_generate_has_mismatch) X(ALIGN_generate_mismatch) \
  X(ALIGN_generate_mismatch_qual) X(ALIGN_project_from_sequence) \
  X(ALIGN_raw_restore_read) X(ALIGN_ref_restore_read) \
  X(ALIGN_ref_sub_select) X(ALIGN_seq_restore_read) \
  X(NCBI_align_clip) X(NCBI_align_clip_2) X(NCBI_align_compress_quality) \
  X(NCBI_align_decompress_quality) X(NCBI_align_edit_distance) \
  X(NCBI_align_edit_distance_2) X(NCBI_align_generate_mismatch_qual_2) \
  X(NCBI_align_generate_preserve_qual) X(NCBI_align_get_clipped_cigar) \
  X(NCBI_align_get_clipped_cigar_2) X(NCBI_align_get_clipped_ref_offset) \
  X(NCBI_align_get_left_soft_clip) X(NCBI_align_get_left_soft_clip_2) \
  X(NCBI_align_get_mate_align_id) X(NCBI_align_get_mismatch_read) \
  X(NCBI_align_get_ref_delete) X(NCBI_align_get_ref_insert) \
  X(NCBI_align_get_ref_len) X(NCBI_align_get_ref_mismatch) \
  X(NCBI_align_get_ref_preserve_qual) X(NCBI_align_get_right_soft_clip) \
  X(NCBI_align_get_right_soft_clip_2) X(NCBI_align_get_right_soft_clip_3) \
  X(NCBI_align_get_right_soft_clip_4) X(NCBI_align_get_sam_flags) \
  X(NCBI_align_get_seq_preserve_qual) X(NCBI_align_local_ref_id) \
  X(NCBI_align_local_ref_start) X(NCBI_align_make_cmp_read_desc) \
  X(NCBI_align_mismatch_restore_qual) X(NCBI_align_not_my_row) \
  X(NCBI_align_raw_restore_qual) X(NCBI_align_ref_name) X(NCBI_align_ref_pos) \
  X(NCBI_align_ref_seq_id) X(NCBI_align_ref_sub_select_preserve_qual) \
  X(NCBI_align_seq_construct_read) X(NCBI_align_template_len)

#ifdef WUNIVERSE
#  define ALL_SYMBOLS(X) COMMON_SYMBOLS(X) X(NCBI_refSeq_stats)
#else
#  define ALL_SYMBOLS(X) COMMON_SYMBOLS(X)
#endif

#define AS_DECL(sym) extern void sym();
ALL_SYMBOLS(AS_DECL)

#define AS_VALUE(sym) sym,
static const xf_function_t s_axf_functions[] = { ALL_SYMBOLS(AS_VALUE) NULL };

#define FOR_KDYLD(sym) KDyldRegisterBuiltin ( #sym, sym );

LIB_EXPORT const xf_function_t * CC register_axf_functions(void)
{
    COMMON_SYMBOLS(FOR_KDYLD)
    return s_axf_functions;
}

#ifdef WUNIVERSE
LIB_EXPORT const xf_function_t * CC register_waxf_functions(void)
{
    ALL_SYMBOLS(FOR_KDYLD)
    return s_axf_functions;
}
#endif
