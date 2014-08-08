#ifndef OBJECTS_ALNMGR___ALNMERGER__HPP
#define OBJECTS_ALNMGR___ALNMERGER__HPP

/*  $Id: alnmerger.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author:  Kamen Todorov, NCBI
*
* File Description:
*   Alignment sequences
*
*/


#include <objects/seqalign/Seq_align.hpp>
#include <objtools/alnmgr/alnexception.hpp>
#include <objtools/alnmgr/alnmatch.hpp>
#include <objtools/alnmgr/alndiag.hpp>
#include <objtools/alnmgr/task_progress.hpp>
#include <objtools/alnmgr/alnsegments.hpp>

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::



class CAlnMixMatches;
class CAlnMixMatch;
class CAlnMixSequences;
class CAlnMixSeq;


class NCBI_XALNMGR_EXPORT CAlnMixMerger : 
    public CObject, 
    public CTaskProgressReporter
{
public:
    
    typedef CAlnMixMatches::TCalcScoreMethod TCalcScoreMethod;

    // Constructor
    CAlnMixMerger(CRef<CAlnMixMatches>& aln_mix_matches,
                  TCalcScoreMethod calc_score = 0);


    void Reset();

    enum EMergeFlags {
        fTruncateOverlaps     = 0x0001, // otherwise put on separate rows
        fNegativeStrand       = 0x0002,
        fGapJoin              = 0x0004, // join equal len segs gapped on refseq
        fMinGap               = 0x0008, // minimize segs gapped on refseq
        fRemoveLeadTrailGaps  = 0x0010, // Remove all leading or trailing gaps
        fSortSeqsByScore      = 0x0020, // Better scoring seqs go towards the top
        fSortInputByScore     = 0x0040, // Process better scoring input alignments first
        fQuerySeqMergeOnly    = 0x0080, // Only put the query seq on same row, 
                                        // other seqs from diff densegs go to diff rows
        fFillUnalignedRegions = 0x0100,
        fAllowTranslocation   = 0x0200  // allow translocations when truncating overlaps
    };
    typedef int TMergeFlags; // binary OR of EMergeFlags

    // Merge matches
    void               Merge            (TMergeFlags flags = 0);


    // Obtain the resulting alignment
    const CDense_seg&  GetDenseg        (void) const;
    const CSeq_align&  GetSeqAlign      (void) const;

private:

    void x_Reset               (void);
    void x_Merge               (void);
    void x_CreateDenseg        (void);
    void x_SetSeqFrame         (CAlnMixMatch* match, CAlnMixSeq*& seq);

    enum ESecondRowFits {
        eSecondRowFitsOk,
        eForceSeparateRow,
        eInconsistentStrand,
        eInconsistentFrame,
        eFirstRowOverlapBelow,
        eFirstRowOverlapAbove,
        eInconsistentOverlap,
        eSecondRowOverlap,
        eSecondRowInconsistency,
        eTranslocation,
        eIgnoreMatch
    };
    typedef int TSecondRowFits;

    TSecondRowFits x_SecondRowFits(CAlnMixMatch * match) const;


    typedef vector<CRef<CAlnMixMatch> > TMatches;
    typedef vector<CRef<CAlnMixSeq> >   TSeqs;
    typedef map<pair<CAlnMixSeq*, CAlnMixSeq*>,
                CDiagRangeCollection>   TPlanes;

    const size_t&               m_DsCnt;

    CRef<CDense_seg>            m_DS;
    CRef<CSeq_align>            m_Aln;

    TMergeFlags                 m_MergeFlags;

    CRef<CAlnMixMatches>        m_AlnMixMatches;
    TMatches&                   m_Matches;

    CRef<CAlnMixSequences>      m_AlnMixSequences;
    TSeqs&                      m_Seqs;
    vector<CRef<CAlnMixSeq> >&  m_Rows;
    list<CRef<CAlnMixSeq> >&    m_ExtraRows;

    CRef<CAlnMixSegments>       m_AlnMixSegments;

    size_t                      m_MatchIdx;

    bool                        m_SingleRefseq;
    bool                        m_IndependentDSs;

    TCalcScoreMethod            x_CalculateScore;

    TPlanes                     m_Planes;
};



///////////////////////////////////////////////////////////
///////////////////// inline methods //////////////////////
///////////////////////////////////////////////////////////


inline
const CDense_seg& CAlnMixMerger::GetDenseg() const
{
    if ( !m_DS ) {
        NCBI_THROW(CAlnException, eMergeFailure,
                   "CAlnMixMerger::GetDenseg(): "
                   "Dense_seg is not available until after Merge()");
    }
    return *m_DS;
}


inline
const CSeq_align& CAlnMixMerger::GetSeqAlign() const
{
    if ( !m_Aln ) {
        NCBI_THROW(CAlnException, eMergeFailure,
                   "CAlnMixMerger::GetSeqAlign(): "
                   "Seq_align is not available until after Merge()");
    }
    return *m_Aln;
}


///////////////////////////////////////////////////////////
////////////////// end of inline methods //////////////////
///////////////////////////////////////////////////////////


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

#endif // OBJECTS_ALNMGR___ALNMERGER__HPP
