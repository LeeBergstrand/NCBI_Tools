/*  $Id: aln_builders.cpp 377671 2012-10-15 15:57:48Z grichenk $
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
* Authors:  Kamen Todorov
*
* File Description:
*   Alignment builders
*
* ===========================================================================
*/


#include <ncbi_pch.hpp>

#include <objtools/alnmgr/aln_builders.hpp>
#include <objtools/alnmgr/aln_rng_coll_oper.hpp>
#include <objtools/alnmgr/aln_serial.hpp>
#include <corelib/ncbitime.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);


void
MergePairwiseAlns(CPairwiseAln& existing,
                  const CPairwiseAln& addition,
                  const CAlnUserOptions::TMergeFlags& flags)
{
    CPairwiseAln difference(existing.GetFirstId(),
                            existing.GetSecondId(),
                            existing.GetPolicyFlags());
    SubtractAlnRngCollections(addition, // minuend
                              existing, // subtrahend
                              difference);
#ifdef _TRACE_MergeAlnRngColl
    cerr << endl;
    cerr << "existing:" << endl << existing << endl;
    cerr << "addition:" << endl << addition << endl;
    cerr << "difference = addition - existing:" << endl << difference << endl;
#endif
    ITERATE(CPairwiseAln, rng_it, difference) {
        existing.insert(*rng_it);
    }
    if ((flags & CAlnUserOptions::fIgnoreInsertions) == 0) {
        int gap_flags = CPairwiseAln::TAlnRngColl::fAllowAbutting |
            CPairwiseAln::TAlnRngColl::fAllowMixedDir |
            CPairwiseAln::TAlnRngColl::fAllowOverlap;
        CPairwiseAln::TAlnRngColl gaps_coll(addition.GetInsertions(),
            gap_flags);
        CPairwiseAln::TAlnRngColl gaps_truncated(gap_flags);
        SubtractAlnRngCollections(gaps_coll, existing, gaps_truncated);
        existing.AddInsertions(gaps_truncated);
    }
#ifdef _TRACE_MergeAlnRngColl
    cerr << "result = existing + difference:" << endl << existing << endl;
#endif
}


class CMergedPairwiseAln : public CObject
{
public:
    typedef CAnchoredAln::TPairwiseAlnVector TPairwiseAlnVector;

    CMergedPairwiseAln(const CAlnUserOptions::TMergeFlags& merge_flags)
        : m_MergeFlags(merge_flags),
          m_NumberOfDirectAlns(0)
    {
    };

    void insert(const CRef<CPairwiseAln>& pairwise) {
        CRef<CPairwiseAln> addition(pairwise);

        if (m_MergeFlags & CAlnUserOptions::fTruncateOverlaps) {
            x_TruncateOverlaps(addition);
        }
        x_AddPairwise(*addition);
    }

    const TPairwiseAlnVector& GetPairwiseAlns() const {
        return m_PairwiseAlns;
    };

    const CAlnUserOptions::TMergeFlags& GetMergedFlags() const {
        return m_MergeFlags;
    }

    void SortInsertions(void)
    {
        NON_CONST_ITERATE(TPairwiseAlnVector, it, m_PairwiseAlns) {
            (*it)->SortInsertions();
        }
    }

private:
    void x_TruncateOverlaps(CRef<CPairwiseAln>& addition)
    {
        /// Truncate, if requested
        ITERATE(TPairwiseAlnVector, aln_it, m_PairwiseAlns) {
            const CPairwiseAln& existing = **aln_it;
            CRef<CPairwiseAln> truncated
                (new CPairwiseAln(addition->GetFirstId(),
                                  addition->GetSecondId(),
                                  addition->GetPolicyFlags()));
            SubtractAlnRngCollections(*addition,  // minuend
                                      existing,   // subtrahend
                                      *truncated);// difference

#ifdef _TRACE_MergeAlnRngColl
            cerr << endl;
            cerr << "existing:" << endl << existing << endl;
            cerr << "addition:" << endl << *addition << endl;
            cerr << "truncated = addition - existing:" << endl << *truncated << endl;
#endif

            if ((m_MergeFlags & CAlnUserOptions::fIgnoreInsertions) == 0) {
                int gap_flags = CPairwiseAln::TAlnRngColl::fAllowAbutting |
                    CPairwiseAln::TAlnRngColl::fAllowMixedDir |
                    CPairwiseAln::TAlnRngColl::fAllowOverlap;
                CPairwiseAln::TAlnRngColl gaps_coll(addition->GetInsertions(),
                    gap_flags);
                CPairwiseAln::TAlnRngColl gaps_truncated(gap_flags);
                SubtractAlnRngCollections(gaps_coll, existing, gaps_truncated);
                addition = truncated;
                addition->AddInsertions(gaps_truncated);
            }
            else {
                addition = truncated;
            }
        }
    }

    
    bool x_ValidNeighboursOnFirstDim(const CPairwiseAln::TAlnRng& left,
                                     const CPairwiseAln::TAlnRng& right) {
        if (left.GetFirstToOpen() > right.GetFirstFrom()) {
            // Overlap on first dimension
            return false;
        }
        return true;
    }

    bool x_ValidNeighboursOnSecondDim(const CPairwiseAln::TAlnRng& left,
                                      const CPairwiseAln::TAlnRng& right) {
        if (left.GetSecondToOpen() > right.GetSecondFrom()) {
            if (m_MergeFlags & CAlnUserOptions::fAllowTranslocation) {
                if (left.GetSecondFrom() < right.GetSecondToOpen()) {
                    // Overlap on second dimension
                    return false;
                }
                // Allowed translocation
            } else {
                // Either overlap or 
                // unallowed translocation (unsorted on second dimension)
                return false;
            }
        }
        return true;
    }


    bool x_CanInsertRng(CPairwiseAln& a, const CPairwiseAln::TAlnRng& r) {
        PAlignRangeFromLess<CPairwiseAln::TAlnRng> p;
        CPairwiseAln::const_iterator it = lower_bound(a.begin(), a.end(), r.GetFirstFrom(), p);

        if (it != a.begin())   { // Check left
            const CPairwiseAln::TAlnRng& left = *(it - 1);
            if ( !x_ValidNeighboursOnFirstDim(left, r)  ||
                 !x_ValidNeighboursOnSecondDim(r.IsDirect() ? left : r,
                                               r.IsDirect() ? r : left) ) {
                return false;
            }
        }
        if (it != a.end()) { // Check right
            const CPairwiseAln::TAlnRng& right = *it;
            if ( !x_ValidNeighboursOnFirstDim(r, right)  ||
                 !x_ValidNeighboursOnSecondDim(r.IsDirect() ? r : right,
                                               r.IsDirect() ? right : r) ) {
                return false;
            }
        }
        return true;
    }


    void x_AddPairwise(const CPairwiseAln& addition) {
        TPairwiseAlnVector::iterator aln_it, aln_end;
        const CPairwiseAln::TAlignRangeVector& gaps = addition.GetInsertions();
        CPairwiseAln::TAlignRangeVector::const_iterator gap_it = gaps.begin();
        ITERATE(CPairwiseAln, rng_it, addition) {

            // What alignments can we possibly insert it to?
            if (m_MergeFlags & CAlnUserOptions::fAllowMixedStrand) {
                aln_it = m_PairwiseAlns.begin();
                aln_end = m_PairwiseAlns.end();
            } else {
                if (rng_it->IsDirect()) {
                    aln_it = m_PairwiseAlns.begin();
                    aln_end = aln_it + m_NumberOfDirectAlns;
                } else {
                    aln_it = m_PairwiseAlns.begin() + m_NumberOfDirectAlns;
                    aln_end = m_PairwiseAlns.end();
                }
            }

            // Which alignment do we insert it to?
            while (aln_it != aln_end) {
#ifdef _TRACE_MergeAlnRngColl
                cerr << endl;
                cerr << *rng_it << endl;
                cerr << **aln_it << endl;
                cerr << "m_MergeFlags: " << m_MergeFlags << endl;
#endif
                _ASSERT(m_MergeFlags & CAlnUserOptions::fAllowMixedStrand  ||
                        (rng_it->IsDirect() && (*aln_it)->IsSet(CPairwiseAln::fDirect))  ||
                        (rng_it->IsReversed() && (*aln_it)->IsSet(CPairwiseAln::fReversed)));
                if (x_CanInsertRng(**aln_it, *rng_it)) {
                    break;
                }
                ++aln_it;
            }
            if (aln_it == aln_end) {
                CRef<CPairwiseAln> new_aln
                    (new CPairwiseAln(addition.GetFirstId(),
                                      addition.GetSecondId(),
                                      addition.GetPolicyFlags()));
                /* adjust policy flags here? */

                aln_it = m_PairwiseAlns.insert(aln_it, new_aln);

                if (rng_it->IsDirect()  &&
                    !(m_MergeFlags & CAlnUserOptions::fAllowMixedStrand)) {
                    ++m_NumberOfDirectAlns;
                }
            }
            (*aln_it)->insert(*rng_it);
#ifdef _TRACE_MergeAlnRngColl
            cerr << *rng_it;
            cerr << **aln_it;
#endif
            _ASSERT(m_MergeFlags & CAlnUserOptions::fAllowMixedStrand  ||
                    (rng_it->IsDirect() && (*aln_it)->IsSet(CPairwiseAln::fDirect))  ||
                    (rng_it->IsReversed() && (*aln_it)->IsSet(CPairwiseAln::fReversed)));

            // Add gaps
            CPairwiseAln::const_iterator next_rng_it = rng_it;
            ++next_rng_it;
            CPairwiseAln::TPos next_rng_pos = -1;
            if (next_rng_it != addition.end()) {
                next_rng_pos = next_rng_it->GetFirstFrom();
            }
            if ((m_MergeFlags & CAlnUserOptions::fIgnoreInsertions) == 0) {
                // Add all gaps up to the next non-gap range
                while (gap_it != gaps.end()  &&
                    (gap_it->GetFirstFrom() <= next_rng_pos  ||  next_rng_pos < 0)) {
                    (*aln_it)->AddInsertion(*gap_it);
                    gap_it++;
                }
            }
        }
    }

    const CAlnUserOptions::TMergeFlags m_MergeFlags;
    TPairwiseAlnVector m_PairwiseAlns;
    size_t m_NumberOfDirectAlns;
};



ostream& operator<<(ostream& out, const CMergedPairwiseAln& merged)
{
    out << "MergedPairwiseAln contains: " << endl;
    out << "  TMergeFlags: " << merged.GetMergedFlags() << endl;
    ITERATE(CMergedPairwiseAln::TPairwiseAlnVector, aln_it, merged.GetPairwiseAlns()) {
        out << **aln_it;
    };
    return out;
}


typedef vector<CRef<CMergedPairwiseAln> > TMergedVec;

void BuildAln(const TMergedVec& merged_vec,
              CAnchoredAln& out_aln)
{
    typedef CAnchoredAln::TDim TDim;

    // Determine the size
    size_t total_number_of_rows = 0;
    ITERATE(TMergedVec, merged_i, merged_vec) {
        total_number_of_rows += (*merged_i)->GetPairwiseAlns().size();
    }

    // Resize the container
    out_aln.SetPairwiseAlns().resize(total_number_of_rows);

    // Copy pairwises
    TDim row = 0;
    ITERATE(TMergedVec, merged_i, merged_vec) {
        ITERATE(CAnchoredAln::TPairwiseAlnVector, pairwise_i, (*merged_i)->GetPairwiseAlns()) {
            out_aln.SetPairwiseAlns()[row] = *pairwise_i;
            ++row;
        }
    }
}


void 
SortAnchoredAlnVecByScore(TAnchoredAlnVec& anchored_aln_vec)
{
    sort(anchored_aln_vec.begin(),
         anchored_aln_vec.end(), 
         PScoreGreater<CAnchoredAln>());
}


void
s_TranslateAnchorToAlnCoords(CPairwiseAln& out_anchor_pw, ///< output must be empty
                             const CPairwiseAln& anchor_pw)
{
    if ( anchor_pw.empty() ) return;
    CPairwiseAln::TPos aln_pos = 0; /// Start at 0
    CPairwiseAln::TPos aln_len = anchor_pw.GetFirstLength();
    bool direct = anchor_pw.begin()->IsFirstDirect();

    // There should be no gaps on anchor
    _ASSERT(anchor_pw.GetInsertions().empty());
    ITERATE (CPairwiseAln::TAlnRngColl, it, anchor_pw) {
        CPairwiseAln::TAlnRng ar = *it;
        ar.SetFirstFrom(direct ? aln_pos : aln_len - aln_pos - ar.GetLength());
        if ( !direct ) {
            ar.SetDirect(!ar.IsDirect());
            ar.SetFirstDirect(true);
        }
        out_anchor_pw.insert(ar);
        aln_pos += ar.GetLength();
    }
}


void
s_TranslatePairwiseToAlnCoords(CPairwiseAln& out_pw,   // output pairwise (needs to be empty)
                               const CPairwiseAln& pw, // input pairwise to translate from
                               const CPairwiseAln& tr, // translating (aln segments) pairwise
                               bool direct) // new anchor has the same direction as the original one?
{
    // Shift between the old anchor and the alignment.
    const CPairwiseAln::TAlignRangeVector& gaps = pw.GetInsertions();
    CPairwiseAln::TAlignRangeVector::const_iterator gap_it = gaps.begin();
    ITERATE (CPairwiseAln, it, pw) {
        CPairwiseAln::TAlnRng ar = *it;
        CPairwiseAln::TPos pos =
            tr.GetFirstPosBySecondPos(direct ? ar.GetFirstFrom() : ar.GetFirstTo());
        ar.SetFirstFrom(pos);
        if ( !direct ) {
            ar.SetDirect(!ar.IsDirect());
            ar.SetFirstDirect(true);
        }
        out_pw.insert(ar);
        if (gap_it != gaps.end()) {
            CPairwiseAln::const_iterator next_it = it;
            ++next_it;
            if (next_it != pw.end()) {
                while (gap_it != gaps.end()  &&
                    gap_it->GetFirstFrom() <= next_it->GetFirstFrom()) {
                    CPairwiseAln::TAlnRng gap_rg = *gap_it;
                    // Need to specify direction since the source point is out of
                    // anchor ranges and will produce -1.
                    CPairwiseAln::TPos new_gap_pos =
                        tr.GetFirstPosBySecondPos(gap_rg.GetFirstFrom(),
                        direct ? CPairwiseAln::eForward : CPairwiseAln::eBackwards);
                    if ( !direct ) {
                        ++new_gap_pos;
                        gap_rg.SetDirect(!gap_rg.IsDirect());
                        gap_rg.SetFirstDirect(true);
                    }
                    gap_rg.SetFirstFrom(new_gap_pos);
                    out_pw.AddInsertion(gap_rg);
                    gap_it++;
                }
            }
        }
    }
    while (gap_it != gaps.end()) {
        CPairwiseAln::TAlnRng gap_rg = *gap_it;
        CPairwiseAln::TPos new_gap_pos =
            tr.GetFirstPosBySecondPos(gap_rg.GetFirstFrom(),
            CPairwiseAln::eForward);
        // If there are no ranges ahead, try to find the last one before the gap.
        if (new_gap_pos == -1) {
            new_gap_pos = tr.GetFirstPosBySecondPos(
                gap_rg.GetFirstFrom(),
                CPairwiseAln::eBackwards) + 1;
        }
        else if ( !direct ) {
            new_gap_pos++;
        }
        gap_rg.SetFirstFrom(new_gap_pos);
        if ( !direct ) {
            gap_rg.SetDirect(!gap_rg.IsDirect());
            gap_rg.SetFirstDirect(true);
        }
        out_pw.AddInsertion(gap_rg);
        gap_it++;
    }
}


void
s_TranslateToAlnCoords(CAnchoredAln& anchored_aln,
                       const TAlnSeqIdIRef& pseudo_seqid)
{
    CAnchoredAln::TPairwiseAlnVector& pairwises = anchored_aln.SetPairwiseAlns();
    typedef CAnchoredAln::TDim TDim;
    const TDim anchor_row = anchored_aln.GetAnchorRow();

    /// Fix the anchor pairwise, so it's expressed in aln coords:
    const CPairwiseAln& anchor_pw = *pairwises[anchor_row];

    int flags = anchor_pw.GetFlags();
    flags &= ~(CPairwiseAln::fDirect | CPairwiseAln::fReversed);

    CRef<CPairwiseAln> new_anchor_pw(new CPairwiseAln(pseudo_seqid,
                                                      anchor_pw.GetSecondId(),
                                                      flags));
    s_TranslateAnchorToAlnCoords(*new_anchor_pw, anchor_pw);
    bool direct = (new_anchor_pw->begin()->IsFirstDirect() == anchor_pw.begin()->IsFirstDirect());

    /// Translate non-anchor pairwises to aln coords:
    for (TDim row = 0;  row < (TDim)pairwises.size();  ++row) {
        if (row == anchor_row) {
            pairwises[row].Reset(new_anchor_pw);
        } else {
            const CPairwiseAln& pw = *pairwises[row];
            flags = pw.GetFlags();
            flags &= ~(CPairwiseAln::fDirect | CPairwiseAln::fReversed);
            CRef<CPairwiseAln> new_pw(new CPairwiseAln(pseudo_seqid,
                                                       pw.GetSecondId(),
                                                       flags));
            s_TranslatePairwiseToAlnCoords(*new_pw, pw, *new_anchor_pw, direct);
            pairwises[row].Reset(new_pw);
        }
    }            
}    


void x_AdjustAnchorDirection(TAnchoredAlnVec&          in_alns,
                             AutoPtr<TAnchoredAlnVec>& out_alns)
{
    // By default use the original alignments.
    out_alns.reset(&in_alns, eNoOwnership);

    // Check if all anchor rows have the same direction.
    bool have_direct = false;
    bool have_reverse = false;
    TAlnSeqIdIRef common_anchor_id;
    ITERATE(TAnchoredAlnVec, it, in_alns) {
        const CAnchoredAln& anchored = **it;
        // All anlignments must have the same anchor id. If this is not true,
        // don't try to adjust strands.
        if ( !common_anchor_id ) {
            common_anchor_id = anchored.GetAnchorId();
        }
        else if ( !common_anchor_id->GetSeqId().Equals(
            anchored.GetAnchorId()->GetSeqId()) ) {
            return;
        }
        ITERATE(CAnchoredAln::TPairwiseAlnVector, pw_it, anchored.GetPairwiseAlns()) {
            const CPairwiseAln& pw = **pw_it;
            ITERATE(CPairwiseAln, seg, pw) {
                if ( seg->IsFirstDirect() ) {
                    have_direct = true;
                }
                else {
                    have_reverse = true;
                }
                if (have_direct  &&  have_reverse) {
                    break;
                }
            }
            if (have_direct  &&  have_reverse) {
                break;
            }
        }
        if (have_direct  &&  have_reverse) {
            break;
        }
    }
    if (!have_direct  ||  !have_reverse) {
        return;
    }

    // Create new anchor with the same coordinates but on plus strand.
    out_alns.reset(new TAnchoredAlnVec, eTakeOwnership);
    ITERATE(TAnchoredAlnVec, it, in_alns) {
        const CAnchoredAln& anchored = **it;
        CAnchoredAln::TDim old_anchor_row = anchored.GetAnchorRow();
        CRef<CAnchoredAln> anchored_copy(new CAnchoredAln);
        anchored_copy->SetDim(anchored.GetDim());
        anchored_copy->SetScore(anchored.GetScore());
        anchored_copy->SetAnchorRow(old_anchor_row);
        out_alns->push_back(anchored_copy);

        for (int row = 0; row < anchored.GetDim(); ++row) {
            const CPairwiseAln& pw = *anchored.GetPairwiseAlns()[row];
            int flags = pw.GetFlags();
            flags &= ~CPairwiseAln::fMixedDir;
            CRef<CPairwiseAln> pw_copy(
                new CPairwiseAln(common_anchor_id, pw.GetSecondId(), flags));
            anchored_copy->SetPairwiseAlns()[row] = pw_copy;
            ITERATE(CPairwiseAln, seg, pw) {
                CPairwiseAln::TAlnRng seg_copy(*seg);
                seg_copy.SetFirstDirect();
                pw_copy->insert(seg_copy);
            }
        }
    }
}


void 
BuildAln(TAnchoredAlnVec& in_alns,
         CAnchoredAln& out_aln,
         const CAlnUserOptions& options,
         TAlnSeqIdIRef pseudo_seqid)
{
    // Types
    typedef CAnchoredAln::TDim TDim;
    typedef CAnchoredAln::TPairwiseAlnVector TPairwiseAlnVector;

    AutoPtr<TAnchoredAlnVec> adj_alns;

    x_AdjustAnchorDirection(in_alns, adj_alns);
    _ASSERT(adj_alns.get());

    /// 1. Build a single anchored_aln
    _ASSERT(out_aln.GetDim() == 0);
    bool anchor_first = (options.m_MergeFlags & CAlnUserOptions::fAnchorRowFirst) != 0;

    switch (options.m_MergeAlgo) {
    case CAlnUserOptions::eQuerySeqMergeOnly:
        ITERATE(TAnchoredAlnVec, anchored_it, *adj_alns) {
            const CAnchoredAln& anchored = **anchored_it;
            if (anchored_it == adj_alns->begin()) {
                out_aln = anchored;
                continue;
            }
            // assumption is that anchor row is the last
            for (TDim row = 0; row < anchored.GetDim(); ++row) {
                if (row == anchored.GetAnchorRow()) {
                    MergePairwiseAlns(*out_aln.SetPairwiseAlns().back(),
                                      *anchored.GetPairwiseAlns()[row],
                                      CAlnUserOptions::fTruncateOverlaps);
                } else if (!anchor_first) {
                    // swap the anchor row with the new one
                    CRef<CPairwiseAln> anchor_pairwise(out_aln.GetPairwiseAlns().back());
                    out_aln.SetPairwiseAlns().back().Reset
                        (new CPairwiseAln(*anchored.GetPairwiseAlns()[row]));
                    out_aln.SetPairwiseAlns().push_back(anchor_pairwise);
                }
            }
        }
        break;
    case CAlnUserOptions::ePreserveRows:
        if ( !adj_alns->empty() ) {
            if ( !(options.m_MergeFlags & CAlnUserOptions::fSkipSortByScore) ) {
                SortAnchoredAlnVecByScore(*adj_alns);
            }
            TMergedVec merged_vec;
            const CAnchoredAln& first_anchored = *adj_alns->front();
            merged_vec.resize(first_anchored.GetDim());
            ITERATE(TAnchoredAlnVec, anchored_it, *adj_alns) {
                const CAnchoredAln& anchored = **anchored_it;
                _ASSERT(anchored.GetDim() == first_anchored.GetDim());
                if (anchored.GetDim() != first_anchored.GetDim()) {
                    string errstr = "All input alignments need to have "
                        "the same dimension when using ePreserveRows.";
                    NCBI_THROW(CAlnException, eInvalidRequest, errstr);
                }
                _ASSERT(anchored.GetAnchorRow() == first_anchored.GetAnchorRow());
                if (anchored.GetAnchorRow() != first_anchored.GetAnchorRow()) {
                    string errstr = "All input alignments need to have "
                        "the same anchor row when using ePreserveRows.";
                    NCBI_THROW(CAlnException, eInvalidRequest, errstr);
                }
                for (TDim row = 0; row < anchored.GetDim(); ++row) {
                    CRef<CMergedPairwiseAln>& merged = merged_vec[row];
                    if (merged.Empty()) {
                        merged.Reset
                            (new CMergedPairwiseAln(row == anchored.GetAnchorRow() ?
                                                    CAlnUserOptions::fTruncateOverlaps :
                                                    options.m_MergeFlags));
                    }
                    merged->insert(anchored.GetPairwiseAlns()[row]);
                }
            }
            BuildAln(merged_vec, out_aln);
        }
        break;
    case CAlnUserOptions::eMergeAllSeqs:
    default: 
        {
            if ( !(options.m_MergeFlags & CAlnUserOptions::fSkipSortByScore) ) {
                SortAnchoredAlnVecByScore(*adj_alns);
            }
            typedef map<TAlnSeqIdIRef, CRef<CMergedPairwiseAln>, SAlnSeqIdIRefComp> TIdMergedMap;
            TIdMergedMap id_merged_map;
            TMergedVec merged_vec;

#ifdef _TRACE_MergeAlnRngColl
            static int aln_idx;
#endif
            int flags = CAlnUserOptions::fTruncateOverlaps;
            flags |= options.m_MergeFlags & CAlnUserOptions::fAllowMixedStrand;
            CRef<CMergedPairwiseAln> merged_anchor(new CMergedPairwiseAln(flags));
            if (anchor_first) {
                merged_vec.push_back(merged_anchor);
            }
            ITERATE(TAnchoredAlnVec, anchored_it, *adj_alns) {
                const CAnchoredAln&       anchored_aln = **anchored_it;
                const CAnchoredAln::TDim& anchor_row   = anchored_aln.GetAnchorRow();

                /// Anchor first (important, to insert anchor id in
                /// id_merged_map before any possible self-aligned seq
                /// gets there first).
#ifdef _TRACE_MergeAlnRngColl
                cerr << endl;
                cerr << *merged_anchor << endl;
                cerr << "inserting aln " << aln_idx << ", anchor row (" << anchor_row << ")" << endl;
                cerr << *anchored_aln.GetPairwiseAlns()[anchor_row] << endl;
#endif
                merged_anchor->insert(anchored_aln.GetPairwiseAlns()[anchor_row]);
                if (anchored_it == adj_alns->begin()) {
                    id_merged_map[anchored_aln.GetId(anchor_row)].Reset(merged_anchor);
                }

                /// Then other rows:
                for (TDim row = anchored_aln.GetDim() - 1; row >=0; --row) {
                    if (row != anchor_row) {
                        CRef<CMergedPairwiseAln>& merged = id_merged_map[anchored_aln.GetId(row)];
                        if (merged.Empty()) {
                            // first time we are dealing with this id.
                            merged.Reset
                                (new CMergedPairwiseAln(options.m_MergeFlags));
                            // and add to the out vectors
                            merged_vec.push_back(merged);
                        }
#ifdef _TRACE_MergeAlnRngColl
                        cerr << *merged << endl;
                        cerr << "inserting aln " << aln_idx << ", row " << row << endl;
                        cerr << *anchored_aln.GetPairwiseAlns()[row] << endl;
#endif
                        merged->insert(anchored_aln.GetPairwiseAlns()[row]);
                    }
                }
#ifdef _TRACE_MergeAlnRngColl
                ++aln_idx;
#endif
            }
            // finally, add the anchor
            if (!anchor_first) {
                merged_vec.push_back(merged_anchor);
            }
            NON_CONST_ITERATE(TMergedVec, ma, merged_vec) {
                (*ma)->SortInsertions();
            }
            BuildAln(merged_vec, out_aln);
        }
        break;
    }
    out_aln.SetAnchorRow(anchor_first ? 0 : out_aln.GetPairwiseAlns().size() - 1);
    if ( !(options.m_MergeFlags & CAlnUserOptions::fUseAnchorAsAlnSeq) ) {
        if ( !pseudo_seqid ) {
            CRef<CSeq_id> seq_id (new CSeq_id("lcl|pseudo [timestamp: " + CTime(CTime::eCurrent).AsString() + "]"));
            CRef<CAlnSeqId> aln_seq_id(new CAlnSeqId(*seq_id));
            pseudo_seqid.Reset(aln_seq_id);
        }
        s_TranslateToAlnCoords(out_aln, pseudo_seqid);
    }

    /// 2. Sort the ids and alns according to score, how to collect score?
}


END_NCBI_SCOPE
