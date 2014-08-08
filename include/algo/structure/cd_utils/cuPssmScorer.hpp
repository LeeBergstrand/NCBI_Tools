/* $Id: cuPssmScorer.hpp 153038 2009-02-23 16:42:57Z lanczyck $
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
 * Author:  Charlie Liu
 *
 * File Description:
 *
 *       Score an seq-align against PSSM
 *
 * ===========================================================================
 */


#ifndef CU_PSSM_SCORER_HPP
#define CU_PSSM_SCORER_HPP

#include <objects/scoremat/PssmWithParameters.hpp>
#include <objects/seqalign/Seq_align.hpp>
#include <objects/seq/Bioseq.hpp>
#include <algo/structure/cd_utils/cuBlock.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
BEGIN_SCOPE(cd_utils)

class PssmScorer
{
public:
	PssmScorer(CRef< CPssmWithParameters > pssm);
	//assume the master is the query/consensus in pssm
	int score(const CRef<CSeq_align>  align, const CRef<CBioseq> bioseq);
	int score(BlockModelPair& bmp, const CRef<CBioseq> bioseq);

    //  Get the raw scores:  index by score[column][row]
    const vector< vector<int> >& getRawScores() const { return m_scoresFromPssm;}

private:
	inline int scoreOneColumn(int col, char aa);
	CRef< CPssmWithParameters > m_pssm;
	vector< vector<int> > m_scoresFromPssm;
};

END_SCOPE(cd_utils)
END_NCBI_SCOPE

#endif


