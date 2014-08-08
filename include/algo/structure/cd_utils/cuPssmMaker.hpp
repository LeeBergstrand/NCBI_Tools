/* $Id: cuPssmMaker.hpp 355628 2012-03-07 15:22:08Z lanczyck $
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
 *       Make PSSM from a CD
 *
 * ===========================================================================
 */


#ifndef CU_PSSM_MAKER_HPP
#define CU_PSSM_MAKER_HPP
#include <algo/blast/api/pssm_input.hpp>
#include <algo/blast/api/blast_exception.hpp>
#include <algo/structure/cd_utils/cuCdCore.hpp>
#include <algo/structure/cd_utils/cuCppNCBI.hpp>
#include <algo/structure/cd_utils/cuResidueProfile.hpp>
#include <algo/structure/cd_utils/cuConsensusMaker.hpp>
#include <objects/scoremat/Pssm.hpp>
//#include <objects/scoremat/PssmParameters.hpp>
//#include <objects/scoremat/PssmFinalData.hpp>
//#include <objects/scoremat/PssmIntermediateData.hpp>
#include <objects/scoremat/PssmWithParameters.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(cd_utils)

USING_SCOPE(objects);
USING_SCOPE(blast);

struct PssmMakerOptions
{
	PssmMakerOptions();

    //  Return true if any of the IntermediateData flags is true.
    bool IsRequestingIntermediateData();

	short pseudoCount;
	double scalingFactor;
	string matrixName;

    //  These flags control what appears in Pssm's IntermediateData
	bool requestInformationContent;            /**< request information content */
    bool requestResidueFrequencies;            /**< request observed residue frequencies */
    bool requestWeightedResidueFrequencies;   /**< request observed weighted residue frequencies */
    bool requestFrequencyRatios;               /**< request frequency ratios */
    bool requestNumIndepObs;               /**< request number of independent observations per position */
    bool gaplessColumnWeights;

	int unalignedSegThreshold;  // -1 : do not skip unaligned segment
	double inclusionThreshold;
	bool reuseUid;
};

class NCBI_CDUTILS_EXPORT CdPssmInput : public IPssmInputData, public ColumnReader
{
public:
	CdPssmInput(ResidueProfiles& profiles, PssmMakerOptions& config, bool useConsensus);
	~CdPssmInput();

	void read(ColumnResidueProfile& crp);
	void Process();
	
    /// Get the query sequence used as master for the multiple sequence
    /// alignment in ncbistdaa encoding.
    unsigned char* GetQuery();

    /// Get the query's length
    unsigned int GetQueryLength();

    /// Obtain the multiple sequence alignment structure
    PSIMsa* GetData();

    /// Obtain the options for the PSSM engine
    const PSIBlastOptions* GetOptions();
	/// Obtain the options for the PSSM engine
    PSIBlastOptions* SetOptions();

	const char* GetMatrixName();

    /// Obtain the diagnostics data that is requested from the PSSM engine
    /// Its results will be populated in the PssmWithParameters ASN.1 object
    const PSIDiagnosticsRequest* GetDiagnosticsRequest();

private:
	ResidueProfiles& m_profiles;
	PSIBlastOptions* m_options;
	//PssmMakerOptions& m_config;
	bool m_useConsensus;
	// Structure representing the multiple sequence alignment
    PSIMsa*                         m_msa;
    /// Multiple sequence alignment dimensions
    PSIMsaDimensions                m_msaDimensions;
	PSIDiagnosticsRequest			m_diagRequest;
	string m_matrixName; //such as BLOSUM62
	unsigned char* m_query;
	unsigned int m_queryLength;
	int m_currentCol;
	
	void moveUpLongestRow();
	int countResiduesInRow(int row);
	void copyRow(PSIMsaCell* src, PSIMsaCell* dest);
	void unalignLeadingTrailingGaps();
};

//  Moved from cuCD.hpp to avoid unnecessary Blast library dependencies for that module.
NCBI_CDUTILS_EXPORT 
int findHighestScoringRowByPssm(CCdCore* ccd);

class NCBI_CDUTILS_EXPORT PssmMaker
{
public:
	PssmMaker(CCdCore* cd, bool useConsensus=true, bool addQueryToPssm=true);
	~PssmMaker();
	
	void setOptions(const PssmMakerOptions& option);
	//void SetIdentityFilterThreshold(double threshold){m_identityFilterThreshold = threshold;};
	CRef<CPssmWithParameters> make();
	//applicable only to useConsensus=false, use trunct master as query
	bool getTrunctMaster(CRef< CSeq_entry >& seqEntry);
	const BlockModelPair& getGuideAlignment();
	const string& getConsensus();
	short getPseudoCount() {return m_pseudoCount;}
	ConsensusMaker& getConsensusMaker() {return *m_conMaker;}
	void printAlignment(string& fileName);
    void printAlignmentByColumn(string& fileName);

    //  Get the residues corresponding to each column of the PSSM.
    void getPssmColumnResidues(map<unsigned int, string>& columnMap);

private:
	ConsensusMaker* m_conMaker;
	bool m_useConsensus;
	bool m_addQuery;
	CRef< CSeq_entry > m_masterSeqEntry;
	vector<char> m_trunctMaster;
	CCdCore* m_cd;
	PssmMakerOptions m_config;
	short m_pseudoCount;
	CdPssmInput* m_pssmInput;
	void modifyQuery(CRef< CSeq_entry > query);
	CRef<CPssmWithParameters> makeDefaultPssm();
	CRef<CPssmWithParameters> m_pssmMade;
};

END_SCOPE(cd_utils)
END_NCBI_SCOPE

#endif
