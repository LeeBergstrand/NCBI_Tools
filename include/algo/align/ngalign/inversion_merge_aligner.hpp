#ifndef NGALIGN_INVERSION_MERGE_ALIGNER__HPP
#define NGALIGN_INVERSION_MERGE_ALIGNER__HPP

/*  $Id: inversion_merge_aligner.hpp 204783 2010-09-10 16:21:43Z dicuccio $
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
 * Authors:  Nathan Bouk
 *
 * File Description:
 *
 */

#include <corelib/ncbistd.hpp>
#include <corelib/ncbiobj.hpp>
#include <objects/seqloc/Na_strand.hpp>

#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objmgr/scope.hpp>
#include <algo/blast/api/blast_types.hpp>
#include <algo/blast/api/bl2seq.hpp>
#include <algo/blast/api/blast_options_handle.hpp>
#include <algo/blast/api/blast_nucl_options.hpp>
#include <objects/seqalign/Seq_align.hpp>
#include <objects/seqalign/Seq_align_set.hpp>
#include <objects/seqalign/Dense_seg.hpp>


#include <algo/align/ngalign/ngalign_interface.hpp>

BEGIN_NCBI_SCOPE

BEGIN_SCOPE(objects)
    class CScope;
    class CSeq_align;
    class CSeq_align_set;
    class CSeq_id;
    class CDense_seg;
END_SCOPE(objects)




class CInversionMergeAligner : public IAlignmentFactory
{
public:

    CInversionMergeAligner(int Threshold) : m_Threshold(Threshold) { ; }

    string GetName() const { return "inversion_merge_aligner"; }

    TAlignResultsRef GenerateAlignments(objects::CScope& Scope,
                                        ISequenceSet* QuerySet,
                                        ISequenceSet* SubjectSet,
                                        TAlignResultsRef AccumResults);

    static bool s_SortByPctCoverage(const CRef<objects::CSeq_align>& A,
                                    const CRef<objects::CSeq_align>& B);

protected:


private:

    int m_Threshold;


    void x_RunMerger(objects::CScope& Scope,
                     CQuerySet& QueryAligns,
                     TAlignResultsRef Results);

    void x_SortAlignSet(objects::CSeq_align_set& AlignSet);

    void x_SplitAlignmentsByStrand(const objects::CSeq_align_set& Source,
                                    objects::CSeq_align_set& Pluses,
                                    objects::CSeq_align_set& Minuses);
    void x_HandleSingleStrandMerging(objects::CSeq_align_set& Source,
                                     objects::CSeq_align_set& Results,
                                     objects::CScope& Scope);

    CRef<objects::CSeq_align>
    x_CreateDiscAlignment(const objects::CSeq_align& Dom,
                          const objects::CSeq_align& Non,
                                objects::CScope& Scope);

    CRef<objects::CSeq_align_set>
    x_MergeSeqAlignSet(const objects::CSeq_align_set& InAligns, objects::CScope& Scope);

    bool x_IsAllGap(const objects::CDense_seg& Denseg);
};



END_NCBI_SCOPE

#endif
