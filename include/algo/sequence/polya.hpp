/*  $Id: polya.hpp 256285 2011-03-03 16:47:00Z mozese2 $
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
* Author: Philip Johnson
*
* File Description: finds mRNA 3' modification (poly-A tails)
*
* ---------------------------------------------------------------------------
*/
#ifndef ALGO_SEQUENCE___POLYA__HPP
#define ALGO_SEQUENCE___POLYA__HPP

#include <corelib/ncbistd.hpp>
#include <util/range.hpp>

BEGIN_NCBI_SCOPE

enum EPolyTail {
    ePolyTail_None,
    ePolyTail_A3, //> 3' poly-A tail
    ePolyTail_T5  //> 5' poly-T head (submitted to db reversed?)
};


///////////////////////////////////////////////////////////////////////////////
/// PRE : two random access iterators pointing to sequence data [begin,
/// end)
/// POST: poly-A tail cleavage site, if any (-1 if not)
template <typename Iterator>
TSignedSeqPos
FindPolyA(Iterator begin, Iterator end);

///////////////////////////////////////////////////////////////////////////////
/// PRE : two random access iterators pointing to sequence data [begin,
/// end); minimum length for tail
/// POST: cleavageSite (if any) and whether we found a poly-A tail, a poly-T
/// head, or neither
template <typename Iterator>
EPolyTail
FindPolyTail(Iterator begin, Iterator end,
             TSignedSeqPos &cleavageSite,
             TSeqPos min_length = 1);

///////////////////////////////////////////////////////////////////////////////
/// PRE : two random access iterators pointing to sequence data [begin,
/// end); maximum number of non-A bases that are allowed to follow the tail
/// POST: poly-A tail range, if any (empty range if not)
template <typename Iterator>
TSeqRange
FindPolyARange(Iterator begin, Iterator end, TSeqPos max_following_bases);

///////////////////////////////////////////////////////////////////////////////
/// PRE : two random access iterators pointing to sequence data [begin,
/// end); minimum length for tail; maximum number of non-A bases that
/// are allowed to follow the tail
/// POST: poly-tail range (if any) and whether we found a poly-A tail, a poly-T
/// head, or neither
template <typename Iterator>
EPolyTail
FindPolyTail(Iterator begin, Iterator end,
             TSeqRange &tail,
             TSeqPos min_length = 1,
             TSeqPos max_following_bases = 0);


///////////////////////////////////////////////////////////////////////////////
/// Implementation [in header because of templates]

template<typename Iterator>
class CRevComp_It {
public:
    CRevComp_It(void) {}
    CRevComp_It(const Iterator &it) {
        m_Base = it;
    }

    char operator*(void) const {
        Iterator tmp = m_Base;
        switch (*--tmp) {
        case 'A': return 'T';
        case 'T': return 'A';
        case 'C': return 'G';
        case 'G': return 'C';
        default: return *tmp;
        }
    }
    CRevComp_It& operator++(void) {
        --m_Base;
        return *this;
    }
    CRevComp_It operator++(int) {
        CRevComp_It it = m_Base;
        --m_Base;
        return it;
    }
    CRevComp_It& operator--(void) {
        ++m_Base;
        return *this;
    }
    CRevComp_It operator--(int) {
        CRevComp_It it = m_Base;
        ++m_Base;
        return it;
    }
    CRevComp_It& operator+=(int i) {
        m_Base -= i;
        return *this;
    }
    CRevComp_It& operator-=(int i) {
        m_Base += i;
        return *this;
    }
    CRevComp_It operator+ (int i) const {
        CRevComp_It it(m_Base);
        it += i;
        return it;
    }
    CRevComp_It operator- (int i) const {
        CRevComp_It it(m_Base);
        it -= i;
        return it;
    }
    int operator- (const CRevComp_It &it) const {
        return it.m_Base - m_Base;
    }
    
    //booleans
    bool operator>= (const CRevComp_It &it) const {
        return m_Base <= it.m_Base;
    }
    bool operator>  (const CRevComp_It &it) const {
        return m_Base < it.m_Base;
    }
    bool operator<= (const CRevComp_It &it) const {
        return m_Base >= it.m_Base;
    }
    bool operator<  (const CRevComp_It &it) const {
        return m_Base > it.m_Base;
    }
    bool operator== (const CRevComp_It &it) const {
        return m_Base == it.m_Base;
    }
    bool operator!= (const CRevComp_It &it) const {
        return m_Base != it.m_Base;
    }
private:
    Iterator m_Base;
};


///////////////////////////////////////////////////////////////////////////////
// PRE : same conditions as STL 'search', but iterators must have ptrdiff_t
// difference type
// POST: same as STL 'search'
template <typename ForwardIterator1, typename ForwardIterator2>
ForwardIterator1 ItrSearch(ForwardIterator1 first1, ForwardIterator1 last1,
                           ForwardIterator2 first2, ForwardIterator2 last2)
{
    ptrdiff_t d1 = last1 - first1;
    ptrdiff_t d2 = last2 - first2;
    if (d1 < d2) {
        return last1;
    }

    ForwardIterator1 current1 = first1;
    ForwardIterator2 current2 = first2;

    while (current2 != last2) {
        if (!(*current1 == *current2)) {
            if (d1-- == d2) {
                return last1;
            } else {
                current1 = ++first1;
                current2 = first2;
            }
        } else {
            ++current1;
            ++current2;
        }
    }
    return (current2 == last2) ? first1 : last1;
}

///////////////////////////////////////////////////////////////////////////////
// PRE : two random access iterators pointing to sequence data [begin,
// end)
// POST: poly-A tail cleavage site, if any (-1 if not)
template <typename Iterator>
TSignedSeqPos FindPolyA(Iterator begin, Iterator end)
{
    TSeqRange tail_found = FindPolyARange(begin, end, 0);
    return tail_found.Empty() ? -1 : tail_found.GetFrom();
}

///////////////////////////////////////////////////////////////////////////////
/// PRE : two random access iterators pointing to sequence data [begin,
/// end)
/// POST: poly-A tail range, if any (empty range if not)
template <typename Iterator>
TSeqRange FindPolyARange(Iterator begin, Iterator end, TSeqPos max_following_bases)
{
    string motif1("AATAAA");
    string motif2("ATTAAA");

    Iterator pos = begin;

    Iterator uStrmMotif = pos;
    while (uStrmMotif != end) {
        pos = uStrmMotif;
        uStrmMotif = ItrSearch(pos, end, motif1.begin(), motif1.end());
        if (uStrmMotif == end) {
            uStrmMotif = ItrSearch(pos, end, motif2.begin(), motif2.end());
        }

        if (uStrmMotif != end) {
            if (end - uStrmMotif < 16) {  // skip over upstream motif, and at least 10 more
                break;
            }
            pos = uStrmMotif + 15;
            ++uStrmMotif;

            Iterator maxCleavage = (end - pos < 21) ? end : pos + 21;

            while (pos < maxCleavage) {
                unsigned int aRun = 0;
                for (++pos;  pos < maxCleavage  &&  aRun < 3;  ++pos) {
                    if (*pos == 'A') {
                        ++aRun;
                    } else {
                        aRun = 0;
                    }
                }
                
                Iterator cleavageSite = pos - aRun;
    
                //now let's look for poly-adenylated tail..
                unsigned int numA = 0, numOther = 0;
                for (Iterator p = cleavageSite; p < end; ++p) {
                    if (*p == 'A') {
                        ++numA;
                    } else {
                        ++numOther;
                    }
                }
    
                for(Iterator p = end - 1;
                    p >= cleavageSite && (end - p) <= max_following_bases+1;
                    --p){
                    if (numOther + numA > 0  &&
                        ((double) numA / (numA+numOther)) > 0.95) {
                        while(*p != 'A')
                            --p;
                        return TSeqRange(cleavageSite - begin, p - begin);
                    }
                    if (*p == 'A') {
                        --numA;
                    } else {
                        --numOther;
                    }
                }
            }
        }
    }

    return TSeqRange();
}

///////////////////////////////////////////////////////////////////////////////
// PRE : two random access iterators pointing to sequence data [begin,
// end)
// POST: cleavageSite (if any) and whether we found a poly-A tail, a poly-T
// head, or neither
template<typename Iterator>
EPolyTail
FindPolyTail(Iterator begin, Iterator end,
             TSignedSeqPos &cleavageSite,
             TSeqPos min_length)
{
    TSeqRange tail;
    EPolyTail type = FindPolyTail(begin, end, tail, min_length);
    if(type == ePolyTail_A3)
        cleavageSite = tail.GetFrom();
    else if(type == ePolyTail_T5)
        cleavageSite = tail.GetTo();
    return type;
}

///////////////////////////////////////////////////////////////////////////////
/// PRE : two random access iterators pointing to sequence data [begin,
/// end); minimum length for tail
/// POST: poly-tail range (if any) and whether we found a poly-A tail, a poly-T
/// head, or neither
template <typename Iterator>
EPolyTail
FindPolyTail(Iterator begin, Iterator end,
             TSeqRange &tail_result,
             TSeqPos min_length,
             TSeqPos max_following_bases)
{
    TSeqRange tail = FindPolyARange(begin, end, max_following_bases);
    if (tail.GetLength() >= min_length) {
        tail_result = tail;
        return ePolyTail_A3;
    } else {
        tail = FindPolyARange(CRevComp_It<Iterator>(end),
                              CRevComp_It<Iterator>(begin),
                              max_following_bases);

        if (tail.GetLength() >= min_length) {
            int seqLen = end - begin;
            tail_result.Set(seqLen - 1 - tail.GetTo(),
                            seqLen - 1 - tail.GetFrom());
            return ePolyTail_T5;
        }
    }

    return ePolyTail_None;
}

END_NCBI_SCOPE

#endif /*ALGO_SEQUENCE___POLYA__HPP*/
