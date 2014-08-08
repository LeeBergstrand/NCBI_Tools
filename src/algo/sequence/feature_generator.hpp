#ifndef ALGO_SEQUENCE___FEAT_GEN__HPP
#define ALGO_SEQUENCE___FEAT_GEN__HPP

/*  $Id: feature_generator.hpp 392017 2013-03-13 14:50:58Z mozese2 $
 * ===========================================================================
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
 * Authors:  Vyacheslav Chetvernin
 *
 * File Description:
 *
 */

#include <corelib/ncbistd.hpp>
#include <corelib/ncbiobj.hpp>

#include <algo/sequence/gene_model.hpp>
#include <objects/seq/MolInfo.hpp>
#include <objects/seqalign/Spliced_seg.hpp>
#include <objects/seqloc/Na_strand.hpp>
#include <objmgr/seq_loc_mapper.hpp>

#include <util/range_coll.hpp>

BEGIN_NCBI_SCOPE

USING_SCOPE(objects);

namespace {

struct SExon {
    TSignedSeqPos prod_from;
    TSignedSeqPos prod_to;
    TSignedSeqPos genomic_from;
    TSignedSeqPos genomic_to;
};

}

struct CFeatureGenerator::SImplementation {
    SImplementation(objects::CScope& scope);
    ~SImplementation();

    CRef<objects::CScope> m_scope;
    TFeatureGeneratorFlags m_flags;
    TSeqPos m_min_intron;
    TSeqPos m_allowed_unaligned;
    bool m_is_gnomon;
    bool m_is_best_refseq;

    typedef map<int,CRef<CSeq_feat> > TGeneMap;
    TGeneMap genes;

    void StitchSmallHoles(objects::CSeq_align& align);
    void TrimHolesToCodons(objects::CSeq_align& align);
    void TrimEnds(objects::CSeq_align& align);
    void MaximizeTranslation(objects::CSeq_align& align);

    CConstRef<objects::CSeq_align> CleanAlignment(const objects::CSeq_align& align_in);
    CConstRef<objects::CSeq_align>
    AdjustAlignment(const objects::CSeq_align& align, TSeqRange range);
    CRef<CSeq_feat> ConvertAlignToAnnot(const objects::CSeq_align& align,
                             objects::CSeq_annot& annot,
                             objects::CBioseq_set& seqs,
                             int gene_id, const objects::CSeq_feat* cdregion,
                             bool call_on_align_list);
    void ConvertAlignToAnnot(const list< CRef<objects::CSeq_align> > &aligns,
                             objects::CSeq_annot &annot,
                             objects::CBioseq_set &seqs);
    void SetFeatureExceptions(objects::CSeq_feat& feat,
                            const objects::CSeq_align* align,
                            objects::CSeq_feat* cds_feat = NULL,
                            const objects::CSeq_feat* cds_feat_on_mrna = NULL,
                            TSeqPos *clean_match_count = NULL);
    void SetPartialFlags(CRef<CSeq_feat> gene_feat,
                         CRef<CSeq_feat> mrna_feat,
                         CRef<CSeq_feat> cds_feat);

    void RecomputePartialFlags(objects::CSeq_annot& annot);

    TSignedSeqRange GetCds(const objects::CSeq_id& seqid);

    enum ETrimSide {
        eTrimProduct,
        eTrimGenomic
    };
    static void TrimLeftExon(int trim_amount, ETrimSide side,
                      vector<SExon>::reverse_iterator left_edge,
                      vector<SExon>::reverse_iterator& exon_it,
                      objects::CSpliced_seg::TExons::reverse_iterator& spl_exon_it,
                      objects::ENa_strand product_strand,
                      objects::ENa_strand genomic_strand);
    static void TrimRightExon(int trim_amount, ETrimSide side,
                       vector<SExon>::iterator& exon_it,
                       vector<SExon>::iterator right_edge,
                       objects::CSpliced_seg::TExons::iterator& spl_exon_it,
                       objects::ENa_strand product_strand,
                       objects::ENa_strand genomic_strand);
private:

    struct SMapper
    {
    public:

        SMapper(const CSeq_align& aln, CScope& scope,
                TSeqPos allowed_unaligned = 10,
                CSeq_loc_Mapper::TMapOptions opts = 0);

        const CSeq_loc& GetRnaLoc();
        CSeq_align::TDim GetGenomicRow() const;
        CSeq_align::TDim GetRnaRow() const;
        CRef<CSeq_loc> Map(const CSeq_loc& loc);
        void IncludeSourceLocs(bool b = true);
        void SetMergeNone();

    private:

        /// This has special logic to set partialness based on alignment
        /// properties
        /// In addition, we need to interpret partial exons correctly
        CRef<CSeq_loc> x_GetLocFromSplicedExons(const CSeq_align& aln) const;
        CRef<CSeq_loc_Mapper> x_Mapper();

        const CSeq_align& m_aln;
        CScope& m_scope;
        CRef<CSeq_loc_Mapper> m_mapper;
        CSeq_align::TDim m_genomic_row;
        CRef<CSeq_loc> rna_loc;
        TSeqPos m_allowed_unaligned;
        CSeq_loc_Mapper::TMapOptions m_opts;
    };

    void TransformProteinAlignToTranscript(CConstRef<CSeq_align>& align,
                                           CRef<CSeq_feat>& cd_feat);
    void x_CollectMrnaSequence(CSeq_inst& inst,
                               const CSeq_align& align,
                               const CSeq_loc& loc,
                               bool add_unaligned_parts = true,
                               bool mark_transcript_deletions = true,
                               bool* has_gap = NULL,
                               bool* has_indel = NULL);
    const CBioseq& x_CreateMrnaBioseq(const CSeq_align& align,
                            CRef<CSeq_loc> loc,
                            const CTime& time,
                            size_t model_num,
                            CBioseq_set& seqs,
                            CSeq_id& rna_id,
                            CRef<CSeq_feat> cdregion);
    const CBioseq& x_CreateProteinBioseq(CSeq_loc* cds_loc,
                               CSeq_feat& cds_on_mrna,
                               const CTime& time,
                               size_t model_num,
                               CBioseq_set& seqs);
    CRef<CSeq_feat> x_CreateMrnaFeature(const CSeq_align& align,
                                        CRef<CSeq_loc> loc,
                                        const CTime& time,
                                        size_t model_num,
                                        CBioseq_set& seqs,
                                        const CSeq_id& rna_id);
    void x_CreateGeneFeature(CRef<CSeq_feat> &gene_feat,
                             const CBioseq_Handle& handle,
                             SMapper& mapper,
                             CRef<CSeq_loc> loc,
                             const CSeq_id& genomic_id,
                             int gene_id = 0);
    CRef<CSeq_feat> x_CreateCdsFeature(CRef<objects::CSeq_feat> cdregion_on_mrna,
                                       const CSeq_align& align,
                                       CRef<CSeq_loc> loc,
                                       const CTime& time,
                                       size_t model_num,
                                       CBioseq_set& seqs,
                                       CSeq_loc_Mapper::TMapOptions opts);
    CRef<CSeq_feat> x_CreateNcRnaFeature(const objects::CSeq_feat* ncrnafeature_on_mrna,
                                         const CSeq_align& align,
                                         CRef<CSeq_loc> loc,
                                         CSeq_loc_Mapper::TMapOptions opts);
    CRef<CSeq_loc> x_PropagateFeatureLocation(const objects::CSeq_feat* ncrnafeature_on_mrna,
                                              const CSeq_align& align,
                                              CRef<CSeq_loc> loc,
                                              CSeq_loc_Mapper::TMapOptions opts,
                                              TSeqPos &offset);
    void x_CheckInconsistentDbxrefs(CConstRef<CSeq_feat> gene_feat,
                                    CConstRef<CSeq_feat> cds_feat);
    void x_CopyAdditionalFeatures(const CBioseq_Handle& handle,
                                  SMapper& mapper,
                                  CSeq_annot& annot);
    void x_HandleRnaExceptions(CSeq_feat& feat,
                               const CSeq_align* align,
                               CSeq_feat* cds_feat,
                               const CSeq_feat* cds_feat_on_mrna);
    void x_HandleCdsExceptions(CSeq_feat& feat,
                               const CSeq_align* align,
                               const CSeq_feat* cds_feat_on_mrna,
                               TSeqPos *clean_match_count);
    void x_SetExceptText(CSeq_feat& feat,
                         const string &except_text);
    void x_SetComment(CSeq_feat &rna_feat,
                      CSeq_feat *cds_feat,
                      const CSeq_feat *cds_feat_on_mrna,
                      const CSeq_align *align,
                      const CRangeCollection<TSeqPos> &mismatch_locs,
                      const CRangeCollection<TSeqPos> &insert_locs,
                      const CRangeCollection<TSeqPos> &delete_locs,
                      const map<TSeqPos,TSeqPos> &delete_sizes,
                      bool partial_unaligned_edge);
    string x_ConstructRnaName(const CBioseq_Handle& handle);

    CMappedFeat GetCdsOnMrna(const objects::CSeq_id& rna_id);

    // merge into single interval or, if cross the origin, into two intervals abutting at the origin
    CRef<CSeq_loc> MergeSeq_locs(const CSeq_loc* loc1, const CSeq_loc* loc2 = NULL);

    CRef<CSeq_loc> FixOrderOfCrossTheOriginSeqloc(const CSeq_loc& loc,
                                                  TSeqPos outside_point,
                                                  CSeq_loc::TOpFlags flags = CSeq_loc::fSort);
};

END_NCBI_SCOPE

#endif  // ALGO_SEQUENCE___FEAT_GEN__HPP
