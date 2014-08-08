#ifndef ALGO_GNOMON___GNOMON_SEQ__HPP
#define ALGO_GNOMON___GNOMON_SEQ__HPP

/*  $Id: gnomon_seq.hpp 138712 2008-08-27 20:07:15Z souvorov $
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
 * Authors:  Alexandre Souvorov, Vyacheslav Chetvernin
 *
 * File Description:
 *
 */

#include <corelib/ncbistd.hpp>

#include <algo/gnomon/gnomon_model.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(gnomon)

typedef vector<double> TDVec;

enum EResidueNames { enA, enC, enG, enT, enN };

class EResidue {
public :
    EResidue() : data(enN) {}
    EResidue(EResidueNames e) : data(e) {}

    operator int() const { return int(data); }

private:
    unsigned char data;
};

class CEResidueVec : public vector<EResidue> {};

class CDoubleStrandSeq {
public:
    const CEResidueVec& operator[](EStrand s) const { return m_seq[s]; }
    CEResidueVec& operator[](EStrand s) { return m_seq[s]; }
    int size() const { return m_seq[0].size(); }
private:
    CEResidueVec m_seq[2];
};

void Convert(const CResidueVec& src, CEResidueVec& dst);
void Convert(const CEResidueVec& src, CResidueVec& dst);
void Convert(const CResidueVec& src, CDoubleStrandSeq& dst);

void ReverseComplement(const CEResidueVec& src, CEResidueVec& dst);

inline EResidue fromACGT(TResidue c)
{
    switch(c)
    {
        case 'A': 
        case 'a': 
            return enA;
        case 'C': 
        case 'c': 
            return enC;
        case 'G': 
        case 'g': 
            return enG;
        case 'T': 
        case 't': 
            return enT;
        default:
            return enN;
    }
    
}

inline TResidue Complement(TResidue c)
{
    switch(c)
    {
        case 'A': 
            return 'T';
        case 'a': 
            return 't';
        case 'C':
            return 'G'; 
        case 'c': 
            return 'g';
        case 'G':
            return 'C'; 
        case 'g': 
            return 'c';
        case 'T':
            return 'A'; 
        case 't': 
            return 'a';
        default:
            return 'N';
    }
}

extern const EResidue    k_toMinus[5];
extern const char *const k_aa_table;

inline EResidue Complement(EResidue c)
{
    return k_toMinus[c];
}

template <class BidirectionalIterator>
void ReverseComplement(const BidirectionalIterator& first, const BidirectionalIterator& last)
{
    for (BidirectionalIterator i( first ); i != last; ++i)
        *i = Complement(*i);
    reverse(first, last);
}

inline TResidue toACGT(EResidue c)
{
    switch(c) {
    case enA: 
	return 'A';
    case enC: 
	return 'C';
    case enG: 
	return 'G';
    case enT: 
	return 'T';
    case enN: 
	return 'N';
    default:
	return 'N';
    }
}

void FindAllStarts(TIVec codons[], const CEResidueVec& mrna, const CAlignMap& mrnamap, TSignedSeqRange search_region, int fixed_frame);
void FindAllStops(TIVec codons[], const CEResidueVec& mrna, const CAlignMap& mrnamap, TSignedSeqRange search_region, int fixed_frame);
void FindStartsStops(const CGeneModel& model, const CEResidueVec& contig_seq, const CEResidueVec& mrna, const CAlignMap& mrnamap,
                     TIVec starts[3],  TIVec stops[3], int& frame);
bool FindUpstreamStop(const vector<int>& stops, int start, int& stop);


END_SCOPE(gnomon)
END_NCBI_SCOPE

#endif  // ALGO_GNOMON___GNOMON_SEQ__HPP
