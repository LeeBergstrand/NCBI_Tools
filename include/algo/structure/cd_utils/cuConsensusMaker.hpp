/* $Id: cuConsensusMaker.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
 * Author:  Charlie
 *
 * File Description:
 *
 *       Make consensus and remaster with it
 *
 * ===========================================================================
 */

#ifndef CU_CONSENSUS_MAKER_HPP
#define CU_CONSENSUS_MAKER_HPP

#include "objects/seqalign/Seq_align_set.hpp"
#include <algo/structure/cd_utils/cuCdCore.hpp>
#include <algo/structure/cd_utils/cuBlock.hpp>
#include <algo/structure/cd_utils/cuResidueProfile.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
BEGIN_SCOPE(cd_utils)

class NCBI_CDUTILS_EXPORT ConsensusMaker
{
public:

	//ConsensusMaker(CRef<CSeq_align_set> seqAlign, CCdCore* cd);
	ConsensusMaker(CCdCore* cd, double incl = 0.5);
	~ConsensusMaker();

	void makeConsensus();
	ResidueProfiles& getResidueProfiles() {return m_rp;}
	const string& getConsensus();
	CRef< CSeq_entry > getConsensusSeqEntry();
	const BlockModelPair& getGuideAlignment()const;
	BlockModelPair& getGuideAlignment();
	CRef< CSeq_align > getGuideSeqAlign();
	CRef<CSeq_align_set> remasterWithConsensus()const;
	void remasterWithConsensus(bool extended);

	void skipUnalignedSeg(int threshold);
	//void columnInclusionThreshold(double percentage){m_inclusionRule = percentage;};

    double GetInclusionRule() const {return m_inclusionRule;}
	
	static CRef<CSeq_align_set> degapAlignment(CCdCore* cd);
	static void degapAlignment(CCdCore* cd, list< CRef< CSeq_align > >& seqAligns);
	static void degapCdAlignment(CCdCore* cd);
	static void degapCdAlignmentToPending(CCdCore* cd);
private:
	
	void addRows();

	string m_consensus;
	CCdCore* m_cd;
	list< CRef< CSeq_align > > m_seqAligns;
	CRef< CSeq_id > m_masterSeqId;
	CRef< CSeq_id > m_conSeqId;
	bool m_made;
	double m_inclusionRule;
	ResidueProfiles m_rp;
};

END_SCOPE(cd_utils)
END_NCBI_SCOPE

#endif
