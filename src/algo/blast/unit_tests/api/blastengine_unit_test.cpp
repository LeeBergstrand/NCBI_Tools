/*  $Id: blastengine_unit_test.cpp 378183 2012-10-18 17:33:44Z madden $
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
* Author:  Tom Madden
*
* File Description:
*   Unit test module to test the code in blast_engine.cpp
*
* ===========================================================================
*/
#include <ncbi_pch.hpp>
#include <corelib/test_boost.hpp>

#include <corelib/ncbitime.hpp>
#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>
#include <objmgr/util/sequence.hpp>

#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <objects/seqalign/Seq_align.hpp>
#include <objects/seqalign/Seq_align_set.hpp>
#include <objects/seqalign/Std_seg.hpp>
#include <objects/seqalign/Dense_seg.hpp>
#include <objects/seqalign/Score.hpp>
#include <serial/serial.hpp>
#include <serial/iterator.hpp>
#include <serial/objostr.hpp>

#include <algo/blast/api/blast_nucl_options.hpp>
#include <algo/blast/api/disc_nucl_options.hpp>
#include <algo/blast/api/seqsrc_multiseq.hpp>
#include <algo/blast/api/seqsrc_seqdb.hpp>
#include <algo/blast/api/local_blast.hpp>
#include <algo/blast/api/objmgr_query_data.hpp>
#include <algo/blast/api/prelim_stage.hpp>
#include <blast_objmgr_priv.hpp>

#include <algo/blast/core/blast_options.h>
#include <algo/blast/core/blast_engine.h>
#include <algo/blast/core/blast_gapalign.h>
#include <algo/blast/core/blast_traceback.h>
#include <algo/blast/core/blast_encoding.h>
#include <algo/blast/core/blast_setup.h>
#include <algo/blast/core/blast_hspstream.h>

#include <algo/blast/api/blast_nucl_options.hpp>

#include <algo/blast/api/repeats_filter.hpp>

#include "test_objmgr.hpp"
#include "blast_test_util.hpp"

using namespace std;
using namespace ncbi;
using namespace ncbi::objects;
using namespace ncbi::blast;

struct BlastEngineTestFixture {

    TSeqLocVector m_vQuery;
    TSeqLocVector m_vSubject;

    ~BlastEngineTestFixture() {
        m_vQuery.clear();
        m_vSubject.clear();
    }

    void setupQueryAndSubject(int query_gi, int subject_gi) 
    {
        CRef<CSeq_loc> query_loc(new CSeq_loc());
        query_loc->SetWhole().SetGi(query_gi);
        CScope* query_scope = new CScope(CTestObjMgr::Instance().GetObjMgr());
        query_scope->AddDefaults();
        m_vQuery.push_back(SSeqLoc(query_loc, query_scope));
        
        CRef<CSeq_loc> subject_loc(new CSeq_loc());
        subject_loc->SetWhole().SetGi(subject_gi);
        CScope* subject_scope = new CScope(CTestObjMgr::Instance().GetObjMgr());
        subject_scope->AddDefaults();
        m_vSubject.push_back(SSeqLoc(subject_loc, subject_scope));
    }
};

BOOST_FIXTURE_TEST_SUITE(blastengine, BlastEngineTestFixture)

void testLongMatchDiagnostics(BlastDiagnostics* diagnostics)
{
    BlastUngappedStats* ungapped_stats = 
        diagnostics->ungapped_stat;
    BlastGappedStats* gapped_stats = 
        diagnostics->gapped_stat;

    BOOST_REQUIRE_EQUAL(22667784, (int)ungapped_stats->lookup_hits);
    BOOST_REQUIRE_EQUAL(296537, ungapped_stats->init_extends);
    BOOST_REQUIRE_EQUAL(1256, ungapped_stats->good_init_extends);
    BOOST_REQUIRE_EQUAL(1252, gapped_stats->extensions);
    BOOST_REQUIRE_EQUAL(23, gapped_stats->good_extensions);
}

void testShortMatchDiagnostics(BlastDiagnostics* diagnostics)
{
    BlastUngappedStats* ungapped_stats = 
        diagnostics->ungapped_stat;
    BlastGappedStats* gapped_stats = 
        diagnostics->gapped_stat;

    BOOST_REQUIRE_EQUAL(1152, (int)ungapped_stats->lookup_hits);
    BOOST_REQUIRE_EQUAL(24, ungapped_stats->init_extends);
    BOOST_REQUIRE_EQUAL(9, ungapped_stats->good_init_extends);
    BOOST_REQUIRE_EQUAL(8, gapped_stats->extensions);
    BOOST_REQUIRE_EQUAL(8, gapped_stats->good_extensions);
}



// This test has a very long (26 mb) subject sequence that is broken into 
// chunks in BLAST_SearchEngineCore that are no longer than 5 meg.  This 
// test was written to verify a fix for long sequences. Only preliminary
// stage of the BLAST search is performed. Also tests that all diagnostic
// hit counts are set correctly.
// CBlastPrelimSearch class is used with a BlastSeqSrc constructed from a 
// single sequence. This can be done, because CBlastPrelimSearch requires 
// a database-based BlastSeqSrc only for the Seq-align creation.
BOOST_AUTO_TEST_CASE(testTBLASTNLongMatchBlastEngine) {
    const Int4 kNumHspsEnd=23;
    const Int8 kEffectiveSearchSpace = 1050668186940LL;
    const int kQueryGi = 9790067;
    const int kSubjectGi = 30698605;
    const EBlastProgramType kProgramType = eBlastTypeTblastn;
    const EProgram kProgram = eTblastn;

    CRef<CBlastOptionsHandle> opts_handle(
        CBlastOptionsFactory::Create(kProgram));

    setupQueryAndSubject(kQueryGi, kSubjectGi);

    opts_handle->SetEffectiveSearchSpace(kEffectiveSearchSpace);
    opts_handle->SetFilterString("F");
    CRef<CBlastOptions> options(&opts_handle->SetOptions());

    BlastSeqSrc* seq_src = MultiSeqBlastSeqSrcInit(m_vSubject, kProgramType);
    CRef<IQueryFactory> query_factory(new CObjMgr_QueryFactory(m_vQuery));
    CBlastPrelimSearch prelim_search(query_factory, options, seq_src);
    CRef<SInternalData> id(prelim_search.Run());

    testLongMatchDiagnostics(id->m_Diagnostics->GetPointer());

    const int kQueryOffsetFinal[kNumHspsEnd] = 
        { 407, 486, 421, 569, 265, 320, 266, 321, 727, 659, 
          92, 1, 1, 727, 422, 216, 167, 825, 167, 831, 216, 369, 49 };
    const int kQueryLengthFinal[kNumHspsEnd] = 
        { 164, 85, 62, 67, 58, 74, 56, 69, 56, 66, 147, 69, 
          73, 61, 40, 26, 35, 54, 35, 48, 21, 69, 22 };
    const int kScoreFinal[kNumHspsEnd] = 
        { 368, 199, 160, 104, 99, 95, 94, 92, 94, 89, 108, 
          101, 97, 95, 89, 86, 84, 84, 83, 79, 75, 74, 74};
    const double kEvalueFinal[kNumHspsEnd] = 
        {1.90868e-35, 4.47098e-34, 4.47098e-34, 4.47098e-34, 
         4.23245e-08, 4.23245e-08, 3.29958e-07, 3.29958e-07, 
         7.11395e-07, 7.11395e-07, 8.64076e-05, 0.000570668, 
         0.001678, 0.00287725, 0.0145032, 0.0325588, 
         0.0558201, 0.0558201, 0.0730883, 0.214807, 0.631249,
         0.826482, 0.826482 };
    
    CBlastHSPResults results
        (prelim_search.ComputeBlastHSPResults
         (id->m_HspStream->GetPointer()));

    BOOST_REQUIRE_EQUAL(1, results->num_queries);
    BOOST_REQUIRE_EQUAL(1, results->hitlist_array[0]->hsplist_count);
    BlastHSPList* hsplist = results->hitlist_array[0]->hsplist_array[0];
    
    BOOST_REQUIRE_EQUAL(kNumHspsEnd, hsplist->hspcnt);
    
    // Check that HSPs are sorted by score; then sort HSPs by e-value, 
    // to assure the proper order for the following assertions
    BOOST_REQUIRE(Blast_HSPListIsSortedByScore(hsplist));
    Blast_HSPListSortByEvalue(hsplist);
    
    for (int index=0; index<kNumHspsEnd; index++) {
        BlastHSP* tmp_hsp = hsplist->hsp_array[index];
        BOOST_REQUIRE_EQUAL(kQueryOffsetFinal[index], tmp_hsp->query.offset);
        BOOST_REQUIRE_EQUAL(kQueryLengthFinal[index], 
                             tmp_hsp->query.end - tmp_hsp->query.offset);
        BOOST_REQUIRE_EQUAL(kScoreFinal[index], tmp_hsp->score);
        BOOST_REQUIRE(fabs((kEvalueFinal[index]-tmp_hsp->evalue) /
                            kEvalueFinal[index]) < 0.001);
    }
    
    BlastSeqSrcFree(seq_src);
}

// Tests a tblastn search with a short subject sequence. Only preliminary
// stage of the BLAST search is performed. Checks obtained HSPs and the
// diagnostic hit counts.
BOOST_AUTO_TEST_CASE(testTBLASTNShortMatchBlastEngine) {
    const Int4 kNumHspsEnd=8;
    const Int8 kEffectiveSearchSpace = 1050668186940LL;
    const int kQueryGi = 9790067;
    const int kSubjectGi = 38547463;
    const EBlastProgramType kProgramType = eBlastTypeTblastn;
    const EProgram kProgram = eTblastn;
    
    setupQueryAndSubject(kQueryGi, kSubjectGi);

    CRef<CBlastOptionsHandle> opts_handle(
        CBlastOptionsFactory::Create(kProgram));

    opts_handle->SetEffectiveSearchSpace(kEffectiveSearchSpace);
    opts_handle->SetFilterString("F");
    CRef<CBlastOptions> options(&opts_handle->SetOptions());

    BlastSeqSrc* seq_src = MultiSeqBlastSeqSrcInit(m_vSubject, kProgramType);
    CRef<IQueryFactory> query_factory(new CObjMgr_QueryFactory(m_vQuery));
    CBlastPrelimSearch prelim_search(query_factory, options, seq_src);
    CRef<SInternalData> id(prelim_search.Run());


    testShortMatchDiagnostics(id->m_Diagnostics->GetPointer());

   const int kQueryOffsetFinal[kNumHspsEnd] = { 98, 425, 320, 340, 823, 675, 247, 103};
   const int kQueryLengthFinal[kNumHspsEnd] = { 223,211,  35,  13,  46,  19,  25,  24};
   const int kScoreFinal[kNumHspsEnd] = {1138, 173, 72, 46, 40, 36, 32, 30}; 
   const double kEvalueFinal[kNumHspsEnd] = 
       {2.52769e-153, 4.98722e-18, 2.4525e-05, 0.0352845, 0.187202, 0.568557, 1.72476, 3.0028};

   CBlastHSPResults results
       (prelim_search.ComputeBlastHSPResults
        (id->m_HspStream->GetPointer()));

   BOOST_REQUIRE_EQUAL(1, results->num_queries);
   BOOST_REQUIRE_EQUAL(1, results->hitlist_array[0]->hsplist_count);
   BlastHSPList* hsplist = results->hitlist_array[0]->hsplist_array[0];

   BOOST_REQUIRE_EQUAL(kNumHspsEnd, hsplist->hspcnt);

   // Check that HSPs are sorted by score; then sort HSPs by e-value, 
   // to assure the proper order for the following assertions
   BOOST_REQUIRE(Blast_HSPListIsSortedByScore(hsplist));
   Blast_HSPListSortByEvalue(hsplist);

   for (int index=0; index<kNumHspsEnd; index++)
   {
        BlastHSP* tmp_hsp = hsplist->hsp_array[index];
        BOOST_REQUIRE_EQUAL(kQueryOffsetFinal[index], 
                             tmp_hsp->query.offset);
        BOOST_REQUIRE_EQUAL(kQueryLengthFinal[index], 
                             tmp_hsp->query.end - tmp_hsp->query.offset);
        BOOST_REQUIRE_EQUAL(kScoreFinal[index], tmp_hsp->score);
        BOOST_REQUIRE(fabs(kEvalueFinal[index]-tmp_hsp->evalue) < 1.0e-10 ||
                      fabs((kEvalueFinal[index]-tmp_hsp->evalue) /
                            kEvalueFinal[index]) < 0.01);
   }
   BlastSeqSrcFree(seq_src);
}

// Human against human sequence comparison which includes some repeats
// and hence has a large number of hits, even with an increased word size.
BOOST_AUTO_TEST_CASE(testBlastnWithLargeWordSize)
{
    const int kQueryGi = 186279; // Short human sequence with repeats
    const int kSubjectGi = 29791382; // Contig from human chromosome 1
    const int kNumHsps = 330;
    const EBlastProgramType kProgramType = eBlastTypeBlastn;

    setupQueryAndSubject(kQueryGi, kSubjectGi);

    BlastSeqSrc* seq_src = MultiSeqBlastSeqSrcInit(m_vSubject, kProgramType);

    CBlastNucleotideOptionsHandle opts_handle;
    opts_handle.SetWordSize(35);
    CRef<CBlastOptions> options(&opts_handle.SetOptions());

    CRef<IQueryFactory> query_factory(new CObjMgr_QueryFactory(m_vQuery));
    CBlastPrelimSearch prelim_search(query_factory, options, seq_src);
    CRef<SInternalData> id(prelim_search.Run());

    CBlastHSPResults results
        (prelim_search.ComputeBlastHSPResults
         (id->m_HspStream->GetPointer()));
    
    // Check results
    BOOST_REQUIRE_EQUAL(1, results->num_queries);
    BOOST_REQUIRE_EQUAL(1, results->hitlist_array[0]->hsplist_count);
    BlastHSPList* hsplist = results->hitlist_array[0]->hsplist_array[0];
    
    BOOST_REQUIRE_EQUAL(kNumHsps, hsplist->hspcnt);
    BlastSeqSrcFree(seq_src);
}

// Same comparison as in previous test, but with repeats filtering;
// only 1 HSP is left. 
BOOST_AUTO_TEST_CASE(testBlastnWithRepeatsFiltering)
{
    const int kQueryGi = 186279; // Short human sequence with repeats
    const int kSubjectGi = 29791382; // Contig from human chromosome 1
    const int kNumHsps = 3;
    const int kMaskedLength = 389;
    const EBlastProgramType kProgramType = eBlastTypeBlastn;

    setupQueryAndSubject(kQueryGi, kSubjectGi);

    CBlastNucleotideOptionsHandle nucl_handle;
    nucl_handle.SetTraditionalBlastnDefaults();
    nucl_handle.SetEvalueThreshold(1);
    nucl_handle.SetRepeatFiltering(true);

    Blast_FindRepeatFilterLoc(m_vQuery, &nucl_handle);
    CRef<CBlastOptions> options(&nucl_handle.SetOptions());

    Uint4 masked_length = m_vQuery[0].mask->GetPacked_int().GetLength();
    BOOST_REQUIRE_EQUAL(kMaskedLength, (int) masked_length);

    BlastSeqSrc* seq_src = MultiSeqBlastSeqSrcInit(m_vSubject, kProgramType);
    CRef<IQueryFactory> query_factory(new CObjMgr_QueryFactory(m_vQuery));
    CBlastPrelimSearch prelim_search(query_factory, options, seq_src);
    CRef<SInternalData> id(prelim_search.Run());

    CBlastHSPResults results
        (prelim_search.ComputeBlastHSPResults
         (id->m_HspStream->GetPointer()));
    
    // Check results
    BOOST_REQUIRE_EQUAL(1, results->num_queries);
    BOOST_REQUIRE_EQUAL(1, results->hitlist_array[0]->hsplist_count);
    BlastHSPList* hsplist = results->hitlist_array[0]->hsplist_array[0];
    
    BOOST_REQUIRE_EQUAL(kNumHsps, hsplist->hspcnt);
    BlastSeqSrcFree(seq_src);
}

BOOST_AUTO_TEST_CASE(testDiscMegaBlastPartialRun) 
{
    const int kQueryGi = 14702146; 
    const string kDbName("data/seqn");
    const size_t kNumHits = 2;
    const int kGis[kNumHits] = { 46071158, 46072400 };
    const int kScores[kNumHits] = { 1024, 944 };
    const int kNumIdent[kNumHits] = { 458, 423 };

    CRef<CSeq_loc> query_loc(new CSeq_loc());
    query_loc->SetWhole().SetGi(kQueryGi);
    CScope* query_scope = new CScope(CTestObjMgr::Instance().GetObjMgr());
    query_scope->AddDefaults();
    m_vQuery.push_back(SSeqLoc(query_loc, query_scope));
    CSearchDatabase dbinfo(kDbName, CSearchDatabase::eBlastDbIsNucleotide);

    CDiscNucleotideOptionsHandle opts_handle;
    
    opts_handle.SetWindowSize(40);
    opts_handle.SetMatchReward(4);
    opts_handle.SetMismatchPenalty(-5);
    opts_handle.SetGapOpeningCost(5);
    opts_handle.SetGapExtensionCost(5);
    opts_handle.SetPercentIdentity(70);
    opts_handle.SetMaskAtHash(false); 

    CRef<CBlastOptionsHandle> options(&opts_handle);
    CRef<IQueryFactory> query_factory(new CObjMgr_QueryFactory(m_vQuery));
    CLocalBlast blaster(query_factory, options, dbinfo);
    CSearchResultSet results = *blaster.Run();

    // Check results
    BOOST_REQUIRE_EQUAL((int)1, (int)results.GetNumResults());
    CConstRef<CSeq_align_set> alignment = results[0].GetSeqAlign();
    BOOST_REQUIRE_EQUAL(kNumHits, alignment->Get().size());

    CRef<CSeq_align> first_hit = alignment->Get().front();
    CRef<CSeq_align> second_hit = alignment->Get().back();

    int score;
    BOOST_REQUIRE(first_hit->GetNamedScore("score", score));
    BOOST_REQUIRE_EQUAL(kScores[0], score);
    BOOST_REQUIRE(second_hit->GetNamedScore("score", score));
    BOOST_REQUIRE_EQUAL(kScores[1], score);

    int num_ident;
    BOOST_REQUIRE(first_hit->GetNamedScore("num_ident", num_ident));
    BOOST_REQUIRE_EQUAL(kNumIdent[0], num_ident);
    BOOST_REQUIRE(second_hit->GetNamedScore("num_ident", num_ident));
    BOOST_REQUIRE_EQUAL(kNumIdent[1], num_ident);

    BOOST_REQUIRE_EQUAL(kGis[0], first_hit->GetSeq_id(1).GetGi());
    BOOST_REQUIRE_EQUAL(kGis[1], second_hit->GetSeq_id(1).GetGi());

}

BOOST_AUTO_TEST_CASE(testBlastpPrelimSearch) 
{
    const string kDbName("data/seqp");
    const int kQueryGi1 = 21282798;
    const int kQueryGi2 = 129295;
    const int kNumHits = 12;
    const int kNumHitsToCheck = 3;
    const int kIndices[kNumHitsToCheck] = { 1, 4, 8 };
    const int kScores[kNumHitsToCheck] = { 519, 56, 52 };
    const int kOids[kNumHitsToCheck] = { 74, 971, 665 }; 
    const int kQueryLengths[kNumHitsToCheck] = { 297, 46, 41 };
    const int kSubjectLengths[kNumHitsToCheck] = { 298, 48, 41 };
    const EProgram kProgram = eBlastp;

    CRef<CSeq_loc> query_loc1(new CSeq_loc());
    query_loc1->SetWhole().SetGi(kQueryGi1);
    CScope* query_scope1 = new CScope(CTestObjMgr::Instance().GetObjMgr());
    query_scope1->AddDefaults();
    m_vQuery.push_back(SSeqLoc(query_loc1, query_scope1));
    CRef<CSeq_loc> query_loc2(new CSeq_loc());
    query_loc2->SetWhole().SetGi(kQueryGi2);
    CScope* query_scope2 = new CScope(CTestObjMgr::Instance().GetObjMgr());
    query_scope2->AddDefaults();
    m_vQuery.push_back(SSeqLoc(query_loc2, query_scope2));
    BlastSeqSrc* seq_src = SeqDbBlastSeqSrcInit(kDbName, true, 0, 0);

    CRef<CBlastOptionsHandle> opts_handle(
        CBlastOptionsFactory::Create(kProgram));
    CRef<CBlastOptions> options(&opts_handle->SetOptions());
    options->SetFilterString("L");  /* NCBI_FAKE_WARNING */

    CRef<IQueryFactory> query_factory(new CObjMgr_QueryFactory(m_vQuery));
    CBlastPrelimSearch prelim_search(query_factory, options, seq_src);
    CRef<SInternalData> id(prelim_search.Run());

    BlastSeqSrcFree(seq_src);

    CBlastHSPResults results
        (prelim_search.ComputeBlastHSPResults
         (id->m_HspStream->GetPointer()));
    BOOST_REQUIRE_EQUAL(2, results->num_queries);
    BOOST_REQUIRE_EQUAL(kNumHits, results->hitlist_array[0]->hsplist_count);

    BOOST_REQUIRE_CLOSE(results->hitlist_array[0]->hsplist_array[0]->best_evalue,
         results->hitlist_array[0]->hsplist_array[0]->hsp_array[0]->evalue, 
         results->hitlist_array[0]->hsplist_array[0]->hsp_array[0]->evalue/2);

    for (int index = 0; index < kNumHitsToCheck;
         ++index) {
        const int kHitIndex = kIndices[index];
        BlastHSPList* hsp_list = 
            results->hitlist_array[0]->hsplist_array[kHitIndex];
        BOOST_REQUIRE_EQUAL(kOids[index], hsp_list->oid);
        BlastHSP* hsp = hsp_list->hsp_array[0];
        BOOST_REQUIRE_EQUAL(kScores[index], hsp->score);
        BOOST_REQUIRE_EQUAL(0, hsp->num_ident);
        BOOST_REQUIRE_EQUAL(kQueryLengths[index], 
                             hsp->query.end - hsp->query.offset);
        BOOST_REQUIRE_EQUAL(kSubjectLengths[index], 
                             hsp->subject.end - hsp->subject.offset);
    }
}

BOOST_AUTO_TEST_CASE(testGappedOffsets)
{
    const unsigned char query[] = {'\016', '\007', '\014', '\024', '\004', '\015', '\011', 
                    '\022', '\012', '\016', '\001', '\010', '\007', '\005', '\014', '\023',
                    '\021', '\003', '\016', '\005', '\013', '\006', '\020', '\011', '\006', 
                    '\015', '\016', '\004', '\017'};

    const unsigned char subject[] = {'\000', '\000', '\000', 
                    '\004', '\015', '\011', '\022', '\012', '\016', '\001', '\010', '\007', 
                    '\005', '\014', '\023', '\021', '\003', '\016', '\005', '\013', '\006', 
                    '\020', '\011', '\006', '\015', '\016', '\004', '\017'};

     Boolean retval = FALSE;
     BlastScoreBlk *sbp = BlastScoreBlkNew(BLASTAA_SEQ_CODE, 1);
         // generate score options
     BlastScoringOptions *score_options;
     BlastScoringOptionsNew(eBlastTypeBlastp, &score_options);
     BLAST_FillScoringOptions(score_options,
                             eBlastTypeBlastp,
                             FALSE,
                             0,
                             0,
                             NULL,
                             BLAST_GAP_OPEN_PROT,
                             BLAST_GAP_EXTN_PROT);
     Blast_ScoreBlkMatrixInit(eBlastTypeBlastp, score_options, sbp, NULL);
     BlastScoringOptionsFree(score_options);

     Int4 q_offset, s_offset;

     // This one has a mismatch at the beginning of the HSPs.
     BlastHSP* hsp = Blast_HSPNew();
     hsp->query.offset = 0;
     hsp->query.end = 27;
     hsp->subject.offset = 0;
     hsp->subject.end = 26;
     hsp->score = 45;

     retval = BlastGetOffsetsForGappedAlignment(query, subject, sbp, hsp, &q_offset, &s_offset);

     BOOST_REQUIRE_EQUAL(q_offset, 22);
     BOOST_REQUIRE_EQUAL(s_offset, 21);
     BOOST_REQUIRE_EQUAL(retval, true);

     // This one should be OK
     hsp->query.offset = 4;
     hsp->query.end = 27;
     hsp->subject.offset = 3;
     hsp->subject.end = 26;
     hsp->score = 45;

     retval = BlastGetOffsetsForGappedAlignment(query, subject, sbp, hsp, &q_offset, &s_offset);

     BOOST_REQUIRE_EQUAL(q_offset, 18);
     BOOST_REQUIRE_EQUAL(s_offset, 17);
     BOOST_REQUIRE_EQUAL(retval, true);

     // This should have no solution.
     hsp->query.offset = 5;
     hsp->query.end = 20;
     hsp->subject.offset = 11;
     hsp->subject.end = 26;
     hsp->score = 45;

     retval = BlastGetOffsetsForGappedAlignment(query, subject, sbp, hsp, &q_offset, &s_offset);
     BOOST_REQUIRE_EQUAL(retval, false);
}

BOOST_AUTO_TEST_SUITE_END()

