#ifndef OBJTOOLS_ALNMGR___PAIRWISE_ALN__HPP
#define OBJTOOLS_ALNMGR___PAIRWISE_ALN__HPP
/*  $Id: pairwise_aln.hpp 367827 2012-06-28 16:50:47Z grichenk $
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
*   Pairwise and query-anchored alignments
*
* ===========================================================================
*/


#include <corelib/ncbistd.hpp>
#include <corelib/ncbiobj.hpp>

#include <util/align_range.hpp>
#include <util/align_range_coll.hpp>

#include <objtools/alnmgr/aln_seqid.hpp>


BEGIN_NCBI_SCOPE
USING_SCOPE(objects);


/// A pairwise aln is a collection of ranges for a pair of rows.
class CPairwiseAln :
    public CObject,
    public CAlignRangeCollection< CAlignRange<TSignedSeqPos> >
{
public:
    // Types
    typedef TSignedSeqPos                  TPos;
    typedef CRange<TPos>                   TRng;
    typedef CAlignRange<TPos>              TAlnRng;
    typedef CAlignRangeCollection<TAlnRng> TAlnRngColl;

    /// Constructor - creates an empty pairwise alignment.
    /// @sa ConvertSeqAlignToPairwiseAln
    /// @sa CreatePairwiseAlnFromSeqAlign
    CPairwiseAln(const TAlnSeqIdIRef& first_id,
                 const TAlnSeqIdIRef& second_id,
                 int flags = fDefaultPolicy)
        : TAlnRngColl(flags),
          m_FirstId(first_id),
          m_SecondId(second_id),
          m_UsingGenomic(false)
    {
    }

    /// Base width of the first row
    int GetFirstBaseWidth(void) const
    {
        return m_FirstId->GetBaseWidth();
    }

    /// Base width of the second row
    int GetSecondBaseWidth(void) const
    {
        return m_SecondId->GetBaseWidth();
    }

    /// Get first sequence id
    const TAlnSeqIdIRef& GetFirstId(void) const
    {
        return m_FirstId;
    }

    /// Get second sequence id
    const TAlnSeqIdIRef& GetSecondId(void) const
    {
        return m_SecondId;
    }

    /// If both sequences are proteins, convert coordinates to genomic anyway.
    /// This may be required if the pairwise alignment is participating in a
    /// multi-row alignment with some rows referencing genomic sequences.
    void ForceGenomicCoords(void)
    {
        if ( m_UsingGenomic ) {
            return; // already trnaslated
        }
        if (!m_FirstId->IsProtein()  ||  !m_SecondId->IsProtein()) {
            return; // already genomic
        }
        NON_CONST_ITERATE(TAlignRangeVector, rg, m_Ranges) {
            rg->SetFirstFrom(rg->GetFirstFrom()*3);
            rg->SetSecondFrom(rg->GetSecondFrom()*3);
            rg->SetLength(rg->GetLength()*3);
        }
        NON_CONST_ITERATE(TAlignRangeVector, rg, m_Insertions) {
            rg->SetFirstFrom(rg->GetFirstFrom()*3);
            rg->SetSecondFrom(rg->GetSecondFrom()*3);
            rg->SetLength(rg->GetLength()*3);
        }
        m_UsingGenomic = true;
    }

    /// Tell the pairwise alignment that the coordinates are already tranlated
    /// to genomic.
    void SetUsingGenomic(void) {
        m_UsingGenomic = true;
    }

private:
    TAlnSeqIdIRef m_FirstId;
    TAlnSeqIdIRef m_SecondId;
    // Both rows translated to genomic coordinates?
    bool          m_UsingGenomic;
};


/// CPairwiseAln iterator. Iterates over aligned ranges and gaps.
class NCBI_XALNMGR_EXPORT CPairwise_CI
{
public:
    typedef CRange<TSignedSeqPos> TSignedRange;
    typedef CPairwiseAln::TAlnRng TAlnRange;

    CPairwise_CI(void)
        : m_Direct(true),
          m_Unaligned(false)
    {
    }

    /// Iterate the whole pairwise alignment.
    CPairwise_CI(const CPairwiseAln& pairwise)
        : m_Aln(&pairwise),
          m_Direct(true),
          m_Range(TSignedRange::GetWhole()),
          m_Unaligned(false)
    {
        x_Init();
    }

    /// Iterate the selected range on the first sequence.
    CPairwise_CI(const CPairwiseAln& pairwise,
                 const TSignedRange& range)
        : m_Aln(&pairwise),
          m_Direct(true),
          m_Range(range),
          m_Unaligned(false)
    {
        x_Init();
    }

    /// Check if the iterator is in valid state.
    operator bool(void) const
    {
        return m_Aln  &&
            m_It != m_Aln->end()  &&
            m_GapIt != m_Aln->end()  &&
            m_GapIt->GetFirstFrom() < m_Range.GetToOpen()  &&
            m_It->GetFirstToOpen() > m_Range.GetFrom();
    }

    CPairwise_CI& operator++(void);

    enum ESegType {
        eAligned, ///< Aligned range
        eGap      ///< Gap or unaligned range
    };

    /// Get current segment type.
    ESegType GetSegType(void) const
    {
        _ASSERT(*this);
        return m_It == m_GapIt ? eAligned : eGap;
    }

    /// Current range on the first sequence.
    const TSignedRange& GetFirstRange(void) const
    {
        _ASSERT(*this);
        return m_FirstRg;
    }

    /// Current range on the second sequence. May be empty when in a gap.
    const TSignedRange& GetSecondRange(void) const
    {
        _ASSERT(*this);
        return m_SecondRg;
    }

    /// Direction of the second sequence relative to the first one.
    bool IsDirect(void) const
    {
        _ASSERT(*this);
        return m_It->IsDirect();
    }

    /// Absolute direction of the first sequence.
    bool IsFirstDirect(void) const
    {
        return m_Direct;
    }

private:
    typedef CPairwiseAln::const_iterator TIterator;
    typedef pair<TIterator, bool> TCheckedIterator;

    void x_Init(void);
    void x_InitSegment(void);

    CConstRef<CPairwiseAln> m_Aln;
    bool         m_Direct;    // Order of segments - depends on 'first' strand
    TSignedRange m_Range;     // Total selected range on the first sequence
    TIterator    m_It;        // Next non-gap segment
    TIterator    m_GapIt;     // Last non-gap segment when in a gap
    TSignedRange m_FirstRg;   // Current range on the first sequence
    TSignedRange m_SecondRg;  // Current range on the second sequence
    bool         m_Unaligned; // Next segment is unaligned
};


/// Query-anchored alignment can be 2 or multi-dimentional.
/// @sa CreateAnchoredAlnFromAln
/// @sa CreateAnchoredAlnVec
class NCBI_XALNMGR_EXPORT CAnchoredAln : public CObject
{
public:
    // Types
    typedef int TDim;
    typedef vector< CRef<CPairwiseAln> > TPairwiseAlnVector;

    /// Default constructor
    CAnchoredAln(void)
        : m_AnchorRow(kInvalidAnchorRow),
          m_Score(0)
    {
    }

    /// NB: Copy constructor is deep on pairwise_alns so that
    /// pairwise_alns can be modified
    CAnchoredAln(const CAnchoredAln& c)
        : m_AnchorRow(c.m_AnchorRow),
          m_Score(c.m_Score)
    {
        *this = c;
    }

    /// NB: Assignment operator is deep on pairwise_alns so that
    /// pairwise_alns can be modified
    CAnchoredAln& operator=(const CAnchoredAln& c)
    {
        if (this == &c) {
            return *this; // prevent self-assignment
        }
        m_AnchorRow = c.m_AnchorRow;
        m_Score = c.m_Score;
        m_PairwiseAlns.resize(c.GetDim());
        for (TDim row = 0; row < c.GetDim(); ++row) {
            CRef<CPairwiseAln> pairwise_aln(
                new CPairwiseAln(*c.m_PairwiseAlns[row]));
            m_PairwiseAlns[row].Reset(pairwise_aln);
        }
        return *this;
    }

    /// How many rows
    TDim GetDim(void) const
    {
        return (TDim)m_PairwiseAlns.size();
    }

    /// Seq ids of the rows
    const TAlnSeqIdIRef& GetId(TDim row) const
    {
        return GetPairwiseAlns()[row]->GetSecondId();
    }

    /// The vector of pairwise alns
    const TPairwiseAlnVector& GetPairwiseAlns(void) const
    {
        return m_PairwiseAlns;
    }

    /// Which is the anchor row?
    TDim GetAnchorRow(void) const
    {
        _ASSERT(m_AnchorRow != kInvalidAnchorRow);
        _ASSERT(m_AnchorRow < GetDim());
        return m_AnchorRow;
    }

    /// What is the seq id of the anchor?
    const TAlnSeqIdIRef& GetAnchorId(void) const
    {
        return GetId(m_AnchorRow);
    }

    /// What is the total score?
    int GetScore(void) const
    {
        return m_Score;
    }

    /// Modify the number of rows.  NB: This resizes the vectors and
    /// potentially invalidates the anchor row.  Never do this unless
    /// you know what you're doing)
    void SetDim(TDim dim)
    {
        _ASSERT(m_AnchorRow == kInvalidAnchorRow); // make sure anchor is not set yet
        m_PairwiseAlns.resize(dim);
    }

    /// Modify pairwise alns
    TPairwiseAlnVector& SetPairwiseAlns(void)
    {
        return m_PairwiseAlns;
    }

    /// Modify anchor row (never do this unless you are creating a new
    /// alignment and know what you're doing). Setting the anchor row
    /// does not update the insertions - they are still aligned to the
    /// old anchor.
    void SetAnchorRow(TDim anchor_row)
    {
        m_AnchorRow = anchor_row;
    }

    /// Set the total score
    void SetScore(int score)
    {
        m_Score = score;
    }

    /// Non-const access to the total score
    int& SetScore(void)
    {
        return m_Score;
    }

    /// Split rows with mixed dir into separate rows
    /// returns true if the operation was performed
    bool SplitStrands(void);

private:
    static const TDim  kInvalidAnchorRow = -1;
    TDim               m_AnchorRow;
    TPairwiseAlnVector m_PairwiseAlns;
    int                m_Score;
};


/// Collection of anchored alignments.
/// @sa CreateAnchoredAlnVec
typedef vector< CRef<CAnchoredAln> > TAnchoredAlnVec;


/// Compare alignments by score.
template<class C>
struct PScoreGreater
{
    bool operator()(const C* const c_1, const C* const c_2) const
    {
        return c_1->GetScore() > c_2->GetScore();
    }
};


END_NCBI_SCOPE

#endif  // OBJTOOLS_ALNMGR___PAIRWISE_ALN__HPP
