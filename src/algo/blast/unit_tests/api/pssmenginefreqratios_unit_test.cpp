/*  $Id: pssmenginefreqratios_unit_test.cpp 171622 2009-09-25 15:08:10Z avagyanv $
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
 * Author:  Christiam Camacho
 *
 */

/** @file pssmcreate-cppunit.cpp
 * Unit test module for creation of PSSMs from frequency ratios
 */
#include <ncbi_pch.hpp>
#include <corelib/test_boost.hpp>

// ASN.1 object includes
#include <objects/scoremat/PssmWithParameters.hpp>

// C++ BLAST APIs
#include <algo/blast/api/pssm_engine.hpp>
#include <algo/blast/api/psi_pssm_input.hpp>
#include "psiblast_aux_priv.hpp"        // for CScorematPssmConverter

// Standard scoring matrices
#include <util/tables/raw_scoremat.h>

// Local utility header files
#include "blast_test_util.hpp"

using namespace std;
using namespace ncbi;

struct CPssmEngineFreqRatiosTestFixture
{
    CPssmEngineFreqRatiosTestFixture() {}
    ~CPssmEngineFreqRatiosTestFixture() {}

    void createNullPssmInputFreqRatios(void) {
        blast::IPssmInputFreqRatios* null_ptr = NULL;
        blast::CPssmEngine pssm_engine(null_ptr);
    }
};

BOOST_FIXTURE_TEST_SUITE(PssmEngineFreqRatios, CPssmEngineFreqRatiosTestFixture)

BOOST_AUTO_TEST_CASE(testRejectNullPssmInputFreqRatios)
{
    BOOST_REQUIRE_THROW(createNullPssmInputFreqRatios(),
                        blast::CBlastException);
}

/// All entries in the frequecy ratios matrix are 0, and therefore the
/// PSSM's scores are those of the underlying scoring matrix
BOOST_AUTO_TEST_CASE(AllZerosFreqRatios)
{
    const Uint4 kQueryLength = 10;
    const Uint1 kQuery[kQueryLength] = 
    { 15,  9, 10,  4, 11, 11, 19, 17, 17, 17 };
    CNcbiMatrix<double> freq_ratios(BLASTAA_SIZE, kQueryLength);

    auto_ptr<blast::IPssmInputFreqRatios> pssm_input;
    pssm_input.reset(new blast::CPsiBlastInputFreqRatios
                        (kQuery, kQueryLength, freq_ratios));
    blast::CPssmEngine pssm_engine(pssm_input.get());
    auto_ptr< CNcbiMatrix<int> > pssm
        (blast::CScorematPssmConverter::GetScores(*pssm_engine.Run()));

    const SNCBIPackedScoreMatrix* score_matrix = &NCBISM_Blosum62;
    const Uint1 kGapResidue = AMINOACID_TO_NCBISTDAA[(int)'-'];
    stringstream ss;
    for (size_t i = 0; i < pssm->GetCols(); i++) {
        for (size_t j = 0; j < pssm->GetRows(); j++) {

            // Exceptional residues get value of BLAST_SCORE_MIN
            if (j == kGapResidue) {
                ss.str("");
                ss << "Position " << i << " residue " 
                       << TestUtil::GetResidue(j) << " differ on PSSM";
                BOOST_REQUIRE_MESSAGE((*pssm)(j, i) == BLAST_SCORE_MIN,
                                      ss.str());
            } else {
                int score = 
                    (int)NCBISM_GetScore(score_matrix,
                                            pssm_input->GetQuery()[i], j);

                ss.str("");
                ss << "Position " << i << " residue " 
                       << TestUtil::GetResidue(j) << " differ on PSSM: "
                       << "expected=" << NStr::IntToString(score) 
                       << " actual=" << NStr::IntToString((*pssm)(j, i));
                BOOST_REQUIRE_MESSAGE
                    (score-1 <= (*pssm)(j, i) || (*pssm)(j, i) <= score+1,
                     ss.str());
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
