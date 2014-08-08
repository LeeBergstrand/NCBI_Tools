#ifndef OBJECTS_ALNMGR___ALNSEGMENTS__HPP
#define OBJECTS_ALNMGR___ALNSEGMENTS__HPP

/*  $Id: alnsegments.hpp 355293 2012-03-05 15:17:16Z vasilche $
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


#include <corelib/ncbiobj.hpp>
#include <objtools/alnmgr/alnseq.hpp>

BEGIN_NCBI_SCOPE

#ifndef BEGIN_objects_SCOPE
#  define BEGIN_objects_SCOPE BEGIN_SCOPE(objects)
#  define END_objects_SCOPE END_SCOPE(objects)
#endif
BEGIN_objects_SCOPE // namespace ncbi::objects::


class CAlnMixSegment;


class NCBI_XALNMGR_EXPORT CAlnMixSegments : public CObject
{
public:
    
    typedef int (*TCalcScoreMethod)(const string& s1,
                                    const string& s2,
                                    bool s1_is_prot,
                                    bool s2_is_prot);

    // Constructor
    CAlnMixSegments(CRef<CAlnMixSequences>&  aln_mix_sequences,
                    TCalcScoreMethod calc_score = 0);


    typedef list<CRef<CAlnMixSegment> > TSegmentsContainer;
    typedef list<CAlnMixSegment*>       TSegments;

    void Build               (bool gap_join = false,
                              bool min_gap = false,
                              bool remove_leading_and_trailing_gaps = false);
    void FillUnalignedRegions(void);

private:
    friend class CAlnMixMerger;

    void x_ConsolidateGaps(TSegmentsContainer& gapped_segs);
    void x_MinimizeGaps   (TSegmentsContainer& gapped_segs);

    TSegments                   m_Segments;
    
    CRef<CAlnMixSequences>      m_AlnMixSequences;
    vector<CRef<CAlnMixSeq> >&  m_Rows;
    list<CRef<CAlnMixSeq> >&    m_ExtraRows;
    TCalcScoreMethod            x_CalculateScore;
};


class NCBI_XALNMGR_EXPORT CAlnMixStarts : public map<TSeqPos, CRef<CAlnMixSegment> >
{
public:
    CAlnMixStarts::const_iterator current;
};


class NCBI_XALNMGR_EXPORT CAlnMixSegment : public CObject
{
public:
    struct SSeqComp {
        bool operator () (const CAlnMixSeq* seq1, const CAlnMixSeq* seq2) const {
            return seq1->m_SeqIdx < seq2->m_SeqIdx  ||
                seq1->m_SeqIdx == seq2->m_SeqIdx  &&  
                seq1->m_ChildIdx < seq2->m_ChildIdx;
        }
    };
    typedef map<CAlnMixSeq*, CAlnMixStarts::iterator, SSeqComp> TStartIterators;
        
    void StartItsConsistencyCheck(const CAlnMixSeq& seq,
                                  const TSeqPos&    start,
                                  size_t            match_idx) const;

    void SetStartIterator(CAlnMixSeq* seq, CAlnMixStarts::iterator iter)
    {
        m_StartIts.insert(TStartIterators::value_type(seq, iter))
            .first->second = iter;
    }

    TSeqPos         m_Len;
    TStartIterators m_StartIts;
    int             m_DsIdx; // used by the truncate algorithm
};



END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

#endif // OBJECTS_ALNMGR___ALNSEGMENTS__HPP
