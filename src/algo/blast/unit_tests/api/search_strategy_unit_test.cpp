/*  $Id: search_strategy_unit_test.cpp 379488 2012-11-01 15:12:15Z camacho $
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
*   Unit test module to test search_strategy.cpp
*
* ===========================================================================
*/
#include <ncbi_pch.hpp>
#include <corelib/test_boost.hpp>
#include <algo/blast/api/search_strategy.hpp>
#include <algo/blast/api/remote_blast.hpp>
#include <algo/blast/api/blast_nucl_options.hpp>
#include <algo/blast/api/blast_prot_options.hpp>
#include <algo/blast/api/psiblast_options.hpp>
#include <algo/blast/api/phiblast_prot_options.hpp>
#include <algo/blast/api/blast_advprot_options.hpp>
#include <algo/blast/api/objmgr_query_data.hpp>
#include <objmgr/object_manager.hpp>
#include <objects/scoremat/PssmWithParameters.hpp>
#include <objects/scoremat/Pssm.hpp>

#include "test_objmgr.hpp"
#include <serial/serial.hpp>
#include <serial/objostr.hpp>
#include <serial/exception.hpp>
#include <util/range.hpp>


using namespace std;
using namespace ncbi;
using namespace ncbi::objects;
using namespace ncbi::blast;

BOOST_AUTO_TEST_SUITE(search_strategy)

BOOST_AUTO_TEST_CASE(emptyInput)
{
    CRef<CBlast4_request> empty_request(new CBlast4_request);
    BOOST_REQUIRE_THROW(CImportStrategy import_strat(empty_request), CBlastException);
}

BOOST_AUTO_TEST_CASE(testMegablast)
{
    const char* fname = "data/ss.asn";
    ifstream in(fname);
    BOOST_REQUIRE(in);
    CRef<CBlast4_request> request = ExtractBlast4Request(in);
    CImportStrategy import_strat(request);
    BOOST_REQUIRE(import_strat.GetService() == "megablast");
    BOOST_REQUIRE(import_strat.GetProgram() == "blastn");
    BOOST_REQUIRE(import_strat.GetTask() == "megablast");
    BOOST_REQUIRE(import_strat.GetDBFilteringID() == 40);

    CRef<objects::CBlast4_queries> query = import_strat.GetQueries();
    BOOST_REQUIRE(query->IsPssm() == false);
    BOOST_REQUIRE(query->IsSeq_loc_list() == true);

    CRef<blast::CBlastOptionsHandle> opts_handle = import_strat.GetOptionsHandle();
    BOOST_REQUIRE_EQUAL(opts_handle->GetHitlistSize(), 500); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetCullingLimit(), 0); 
    CBlastNucleotideOptionsHandle* blastn_opts = dynamic_cast<CBlastNucleotideOptionsHandle*> (&*opts_handle);
    BOOST_REQUIRE_EQUAL(blastn_opts->GetMatchReward(), 1); 
    BOOST_REQUIRE_EQUAL(blastn_opts->GetMismatchPenalty(), -2); 
}

BOOST_AUTO_TEST_CASE(testBlastp)
{
    const char* fname = "data/ss.blastp.asn";
    ifstream in(fname);
    BOOST_REQUIRE(in);
    CRef<CBlast4_request> request = ExtractBlast4Request(in);
    CImportStrategy import_strat(request);
    BOOST_REQUIRE(import_strat.GetService() == "plain");
    BOOST_REQUIRE(import_strat.GetProgram() == "blastp");

    CRef<objects::CBlast4_queries> query = import_strat.GetQueries();
    BOOST_REQUIRE(query->IsPssm() == false);
    BOOST_REQUIRE(query->IsSeq_loc_list() == false);
    const CBioseq_set& bss = query->GetBioseq_set();
    list<CRef<CSeq_entry> > seq_entry = bss.GetSeq_set();
    BOOST_REQUIRE(seq_entry.front()->GetSeq().GetLength() == 232);

    CRef<objects::CBlast4_subject> subject = import_strat.GetSubject(); 
    BOOST_REQUIRE(subject->IsDatabase() == true);
    BOOST_REQUIRE(subject->GetDatabase() == "refseq_protein");

    CRef<blast::CBlastOptionsHandle> opts_handle = import_strat.GetOptionsHandle();
    BOOST_REQUIRE_EQUAL(opts_handle->GetHitlistSize(), 500); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetCullingLimit(), 0); 
    CBlastAdvancedProteinOptionsHandle* blastp_opts = dynamic_cast<CBlastAdvancedProteinOptionsHandle*> (&*opts_handle);
    BOOST_REQUIRE(!strcmp("BLOSUM62", blastp_opts->GetMatrixName()));
    BOOST_REQUIRE_EQUAL(3, blastp_opts->GetWordSize());

}

BOOST_AUTO_TEST_CASE(testBlastnBl2seq)
{
    const char* fname = "data/ss.bl2seq.asn";
    ifstream in(fname);
    BOOST_REQUIRE(in);
    CRef<CBlast4_request> request = ExtractBlast4Request(in);
    CImportStrategy import_strat(request);
    BOOST_REQUIRE(import_strat.GetService() == "plain");
    BOOST_REQUIRE(import_strat.GetProgram() == "blastn");
    BOOST_REQUIRE(import_strat.GetTask() == "blastn");

    CRef<objects::CBlast4_queries> query = import_strat.GetQueries();
    BOOST_REQUIRE(query->IsPssm() == false);
    BOOST_REQUIRE(query->IsSeq_loc_list() == false);
    const CBioseq_set& bss = query->GetBioseq_set();
    list<CRef<CSeq_entry> > seq_entry = bss.GetSeq_set();
    BOOST_REQUIRE(seq_entry.front()->GetSeq().GetLength() == 2772);

    CRef<objects::CBlast4_subject> subject = import_strat.GetSubject(); 
    BOOST_REQUIRE(subject->IsDatabase() == false);
    list<CRef< CBioseq> > subject_list = subject->GetSequences();
    BOOST_REQUIRE(subject_list.size() == 1);
    BOOST_REQUIRE(subject_list.front()->GetLength() == 9180);

    CRef<blast::CBlastOptionsHandle> opts_handle = import_strat.GetOptionsHandle();
    BOOST_REQUIRE_EQUAL(opts_handle->GetHitlistSize(), 500); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetCullingLimit(), 0); 
    CBlastNucleotideOptionsHandle* blastn_opts = dynamic_cast<CBlastNucleotideOptionsHandle*> (&*opts_handle);
    BOOST_REQUIRE_EQUAL(blastn_opts->GetMatchReward(), 2); 
    BOOST_REQUIRE_EQUAL(blastn_opts->GetMismatchPenalty(), -3); 
}

BOOST_AUTO_TEST_CASE(LoadWebPsiBlastSearchStrategyAfterRound1) 
{
    const char* fname = "data/webpsi.1stround.ss.asn";
    ifstream in(fname);
    CRef<CBlast4_request> request = ExtractBlast4Request(in);
    CImportStrategy import_strat(request);
    BOOST_REQUIRE_EQUAL(import_strat.GetTask(), "psiblast");
    BOOST_REQUIRE_EQUAL(import_strat.GetService(), "plain");
    BOOST_REQUIRE_EQUAL(import_strat.GetProgram(), "blastp");

    CRef<objects::CBlast4_queries> query = import_strat.GetQueries();
    BOOST_REQUIRE(query->IsPssm() == false);
    BOOST_REQUIRE(query->IsSeq_loc_list() == false);
    const CBioseq_set& bss = query->GetBioseq_set();
    list<CRef<CSeq_entry> > seq_entry = bss.GetSeq_set();
    BOOST_REQUIRE(seq_entry.front()->GetSeq().GetLength() == 320);

    CRef<objects::CBlast4_subject> subject = import_strat.GetSubject(); 
    BOOST_REQUIRE_EQUAL(subject->IsDatabase(), true);
    BOOST_REQUIRE_EQUAL(subject->GetDatabase(), "nr");

    BOOST_REQUIRE(import_strat.GetAlgoOptions() != NULL);
    BOOST_REQUIRE(import_strat.GetProgramOptions() == NULL);
    BOOST_REQUIRE(import_strat.GetWebFormatOptions() != NULL);

    CRef<blast::CBlastOptionsHandle> opts_handle = import_strat.GetOptionsHandle();
    BOOST_REQUIRE_EQUAL(opts_handle->GetHitlistSize(), 500); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetWindowSize(), 40); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetMaskAtHash(), false); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetGappedMode(), true); 
    CPSIBlastOptionsHandle* psi_opts = dynamic_cast<CPSIBlastOptionsHandle*> (&*opts_handle);
    BOOST_REQUIRE(psi_opts != NULL);
    BOOST_REQUIRE_EQUAL(psi_opts->GetGapExtensionCost(), 1); 
    BOOST_REQUIRE_EQUAL(psi_opts->GetGapOpeningCost(), 11); 
    BOOST_REQUIRE_EQUAL(psi_opts->GetMatrixName(), "BLOSUM62"); 
    BOOST_REQUIRE_EQUAL(import_strat.GetPsiNumOfIterations(), 1);
}

BOOST_AUTO_TEST_CASE(LoadWebPsiBlastSearchStrategyAfterRound2) 
{
    const char* fname = "data/webpsi.2ndround.ss.asn";
    ifstream in(fname);
    CRef<CBlast4_request> request = ExtractBlast4Request(in);
    CImportStrategy import_strat(request);
    BOOST_REQUIRE_EQUAL(import_strat.GetTask(), "psiblast");
    BOOST_REQUIRE_EQUAL(import_strat.GetService(), "psi");
    BOOST_REQUIRE_EQUAL(import_strat.GetProgram(), "blastp");

    CRef<objects::CBlast4_queries> query = import_strat.GetQueries();
    BOOST_REQUIRE(query->IsPssm() == true);
    BOOST_REQUIRE(query->IsSeq_loc_list() == false);
    BOOST_REQUIRE(query->IsBioseq_set() == false);
    const CPssmWithParameters& pssm = query->GetPssm();
    BOOST_REQUIRE_EQUAL(pssm.HasQuery(), true);
    BOOST_REQUIRE_EQUAL(pssm.GetQuery().IsSeq(), true);
    const TSeqPos kQueryLength(320);
    BOOST_REQUIRE_EQUAL(pssm.GetQuery().GetSeq().GetLength(), kQueryLength);
    BOOST_REQUIRE_EQUAL(pssm.GetPssm().GetQueryLength(), kQueryLength);

    CRef<objects::CBlast4_subject> subject = import_strat.GetSubject(); 
    BOOST_REQUIRE_EQUAL(subject->IsDatabase(), true);
    BOOST_REQUIRE_EQUAL(subject->GetDatabase(), "nr");

    BOOST_REQUIRE(import_strat.GetAlgoOptions() != NULL);
    BOOST_REQUIRE(import_strat.GetProgramOptions() == NULL);
    BOOST_REQUIRE(import_strat.GetWebFormatOptions() != NULL);

    CRef<blast::CBlastOptionsHandle> opts_handle = import_strat.GetOptionsHandle();
    BOOST_REQUIRE_EQUAL(opts_handle->GetHitlistSize(), 500); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetWindowSize(), 40); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetMaskAtHash(), false); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetGappedMode(), true); 
    CPSIBlastOptionsHandle* psi_opts = dynamic_cast<CPSIBlastOptionsHandle*> (&*opts_handle);
    BOOST_REQUIRE(psi_opts != NULL);
    BOOST_REQUIRE_EQUAL(psi_opts->GetGapExtensionCost(), 1); 
    BOOST_REQUIRE_EQUAL(psi_opts->GetGapOpeningCost(), 11); 
    BOOST_REQUIRE_EQUAL(psi_opts->GetMatrixName(), "BLOSUM62"); 
    BOOST_REQUIRE_EQUAL(psi_opts->GetInclusionThreshold(), 0.001); 
    BOOST_REQUIRE_EQUAL(psi_opts->GetPseudoCount(), 0); 
    BOOST_REQUIRE_EQUAL(import_strat.GetPsiNumOfIterations(), 2);
}

// Created with command line psiblast, one iteration
BOOST_AUTO_TEST_CASE(LoadPhiBlastSearchStrategy) 
{
    const char* fname = "data/phi.ss.asn";
    ifstream in(fname);
    CRef<CBlast4_request> request = ExtractBlast4Request(in);
    CImportStrategy import_strat(request);
    BOOST_REQUIRE_EQUAL(import_strat.GetTask(), "phiblastp");
    BOOST_REQUIRE_EQUAL(import_strat.GetService(), "plain");
    BOOST_REQUIRE_EQUAL(import_strat.GetProgram(), "blastp");

    CRef<objects::CBlast4_queries> query = import_strat.GetQueries();
    BOOST_REQUIRE(query->IsPssm() == false);
    BOOST_REQUIRE(query->IsSeq_loc_list() == false);
    const CBioseq_set& bss = query->GetBioseq_set();
    list<CRef<CSeq_entry> > seq_entry = bss.GetSeq_set();
    BOOST_REQUIRE(seq_entry.front()->GetSeq().GetLength() == 549);

    CRef<objects::CBlast4_subject> subject = import_strat.GetSubject(); 
    BOOST_REQUIRE_EQUAL(subject->IsDatabase(), true);
    BOOST_REQUIRE_EQUAL(subject->GetDatabase(), "ecoli");

    BOOST_REQUIRE(import_strat.GetAlgoOptions() != NULL);
    BOOST_REQUIRE(import_strat.GetProgramOptions() == NULL);
    BOOST_REQUIRE(import_strat.GetWebFormatOptions() != NULL);

    CRef<blast::CBlastOptionsHandle> opts_handle = import_strat.GetOptionsHandle();
    BOOST_REQUIRE_EQUAL(opts_handle->GetHitlistSize(), 500); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetWindowSize(), 40); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetMaskAtHash(), false); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetGappedMode(), true); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetOptions().GetInclusionThreshold(), 0.002); 
    BOOST_REQUIRE_EQUAL(opts_handle->GetOptions().GetPseudoCount(), 0); 
    CPHIBlastProtOptionsHandle* psi_opts = dynamic_cast<CPHIBlastProtOptionsHandle*> (&*opts_handle);
    BOOST_REQUIRE(psi_opts != NULL);
    BOOST_REQUIRE_EQUAL(psi_opts->GetGapExtensionCost(), 1); 
    BOOST_REQUIRE_EQUAL(psi_opts->GetGapOpeningCost(), 11); 
    BOOST_REQUIRE_EQUAL(psi_opts->GetMatrixName(), "BLOSUM62"); 
    BOOST_REQUIRE_EQUAL(psi_opts->GetPHIPattern(), "[GA]xxxxGK[ST]"); 
    BOOST_REQUIRE_EQUAL(import_strat.GetPsiNumOfIterations(), 1);
}

/*
 * Export Search Strategy Tests
 */

// Test that when a query with a range restriction is NOT provided, no
// RequiredEnd and RequiredStart fields are sent over the network
BOOST_AUTO_TEST_CASE(ExportStrategy_FullQuery) {
    CRef<CSeq_id> id(new CSeq_id(CSeq_id::e_Gi, 555));
    auto_ptr<blast::SSeqLoc> sl(CTestObjMgr::Instance().CreateSSeqLoc(*id));
    TSeqLocVector queries(1, *sl.get());
    CRef<IQueryFactory> qf(new CObjMgr_QueryFactory(queries));
    const string kDbName("nt");
    CSearchDatabase db(kDbName,
                       CSearchDatabase::eBlastDbIsNucleotide);

    CRef<CSearchDatabase> target_db(&db);
    CRef<CBlastOptionsHandle> opts
        (CBlastOptionsFactory::Create(eBlastn, CBlastOptions::eRemote));

    CExportStrategy exp_ss(qf, opts, target_db);
    CRef<CBlast4_request> ss = exp_ss.GetSearchStrategy();
    BOOST_REQUIRE(ss.NotEmpty());

    bool found_query_range = false;

    const CBlast4_request_body& body = ss->GetBody();
    BOOST_REQUIRE(body.IsQueue_search());
    const CBlast4_queue_search_request& qsr = body.GetQueue_search();

    // These are the parameters that we are looking for
    vector<string> param_names;
    param_names.push_back(CBlast4Field::GetName(eBlastOpt_RequiredStart));
    param_names.push_back(CBlast4Field::GetName(eBlastOpt_RequiredEnd));

    // Get the program options
    if (qsr.CanGetProgram_options()) {
        const CBlast4_parameters& prog_options = qsr.GetProgram_options();
        ITERATE(vector<string>, pname, param_names) {
            CRef<CBlast4_parameter> p = prog_options.GetParamByName(*pname);
            if (p.NotEmpty()) {
                found_query_range = true;
                break;
            }
        }
    }
    BOOST_REQUIRE(found_query_range == false);

    // (check also the algorithm options, just in case they ever get misplaced)
    if (qsr.CanGetAlgorithm_options()) {
        const CBlast4_parameters& algo_options = qsr.GetAlgorithm_options();
        ITERATE(vector<string>, pname, param_names) {
            CRef<CBlast4_parameter> p = algo_options.GetParamByName(*pname);
            if (p.NotEmpty()) {
                found_query_range = true;
                break;
            }
        }
    }
    BOOST_REQUIRE(found_query_range == false);

    // just as a bonus, check the database
    BOOST_REQUIRE(qsr.CanGetSubject());
    BOOST_REQUIRE(qsr.GetSubject().GetDatabase() == kDbName);
}

// Test that when a query with a range restriction is provided, the appropriate
// RequiredEnd and RequiredStart fields are sent over the network
BOOST_AUTO_TEST_CASE(ExportStrategy_QueryWithRange) {
    CRef<CSeq_id> id(new CSeq_id(CSeq_id::e_Gi, 555));
    TSeqRange query_range(1,200);
    auto_ptr<blast::SSeqLoc> sl(CTestObjMgr::Instance().CreateSSeqLoc(*id,
                                query_range));
    TSeqLocVector queries(1, *sl.get());
    CRef<IQueryFactory> qf(new CObjMgr_QueryFactory(queries));
    const string kDbName("nt");
    CSearchDatabase db(kDbName,
                       CSearchDatabase::eBlastDbIsNucleotide);

    CRef<CSearchDatabase> target_db(&db);
    CRef<CBlastOptionsHandle> opts
        (CBlastOptionsFactory::Create(eBlastn, CBlastOptions::eRemote));

    CExportStrategy exp_ss(qf, opts, target_db);
    CRef<CBlast4_request> ss = exp_ss.GetSearchStrategy();
    BOOST_REQUIRE(ss.NotEmpty());

    bool found_query_range = false;

    const CBlast4_request_body& body = ss->GetBody();
    BOOST_REQUIRE(body.IsQueue_search());
    const CBlast4_queue_search_request& qsr = body.GetQueue_search();

    // These are the parameters that we are looking for
    vector<string> param_names;
    param_names.push_back(CBlast4Field::GetName(eBlastOpt_RequiredStart));
    param_names.push_back(CBlast4Field::GetName(eBlastOpt_RequiredEnd));

    // Get the program options
    if (qsr.CanGetProgram_options()) {
        const CBlast4_parameters& prog_options = qsr.GetProgram_options();
        ITERATE(vector<string>, pname, param_names) {
            CRef<CBlast4_parameter> p = prog_options.GetParamByName(*pname);
            if (p.NotEmpty()) {
                BOOST_REQUIRE(p->CanGetValue());
                found_query_range = true;
                if (*pname == CBlast4Field::GetName(eBlastOpt_RequiredStart)) {
                    BOOST_REQUIRE_EQUAL((int)query_range.GetFrom(),
                                        (int)p->GetValue().GetInteger());
                }
                if (*pname == CBlast4Field::GetName(eBlastOpt_RequiredEnd)) {
                    BOOST_REQUIRE_EQUAL((int)query_range.GetTo(),
                                        (int)p->GetValue().GetInteger());
                }
            }
        }
    }
    BOOST_REQUIRE(found_query_range == true);

    found_query_range = false;
    // Check that this option is NOT specified in the algorithm options
    if (qsr.CanGetAlgorithm_options()) {
        const CBlast4_parameters& algo_options = qsr.GetAlgorithm_options();
        ITERATE(vector<string>, pname, param_names) {
            CRef<CBlast4_parameter> p = algo_options.GetParamByName(*pname);
            if (p.NotEmpty()) {
                found_query_range = true;
                break;
            }
        }
    }
    BOOST_REQUIRE(found_query_range == false);

    // just as a bonus, check the database
    BOOST_REQUIRE(qsr.CanGetSubject());
    BOOST_REQUIRE(qsr.GetSubject().GetDatabase() == kDbName);
}

// Test that when no identifier is provided for the sequence data, a Bioseq
// should be submitted
BOOST_AUTO_TEST_CASE(ExportStrategy_QueryWithLocalIds) {

    CSeq_entry seq_entry;
    ifstream in("data/seq_entry_lcl_id.asn");
    in >> MSerial_AsnText >> seq_entry;
    CSeq_id& id = const_cast<CSeq_id&>(*seq_entry.GetSeq().GetFirstId());
    in.close();

    CRef<CScope> scope(new CScope(*CObjectManager::GetInstance()));
    scope->AddTopLevelSeqEntry(seq_entry);
    CRef<CSeq_loc> sl(new CSeq_loc(id, (TSeqPos)0, (TSeqPos)11));
    TSeqLocVector query_loc(1, SSeqLoc(sl, scope));
    CRef<IQueryFactory> qf(new CObjMgr_QueryFactory(query_loc));
    const string kDbName("nt");
    CSearchDatabase db(kDbName,
                       CSearchDatabase::eBlastDbIsNucleotide);

    CRef<CSearchDatabase> target_db(&db);
    CRef<CBlastOptionsHandle> opts
        (CBlastOptionsFactory::Create(eBlastn, CBlastOptions::eRemote));

    CExportStrategy exp_ss(qf, opts, target_db);
    CRef<CBlast4_request> ss = exp_ss.GetSearchStrategy();
    BOOST_REQUIRE(ss.NotEmpty());

    const CBlast4_request_body& body = ss->GetBody();
    BOOST_REQUIRE(body.IsQueue_search());
    const CBlast4_queue_search_request& qsr = body.GetQueue_search();
    BOOST_REQUIRE(qsr.CanGetQueries());
    const CBlast4_queries& b4_queries = qsr.GetQueries();
    BOOST_REQUIRE_EQUAL(query_loc.size(), b4_queries.GetNumQueries());
    BOOST_REQUIRE(b4_queries.IsBioseq_set());
    BOOST_REQUIRE( !b4_queries.IsPssm() );
    BOOST_REQUIRE( !b4_queries.IsSeq_loc_list() );

    // just as a bonus, check the database
    BOOST_REQUIRE(qsr.CanGetSubject());
    BOOST_REQUIRE(qsr.GetSubject().GetDatabase() == kDbName);
}

// Test that when GIs are provided as the queries, no bioseq
// should be submitted, instead a list of seqlocs should be sent
BOOST_AUTO_TEST_CASE(ExportStrategy_QueryWithGIs) {

    CRef<CScope> scope(new CScope(*CObjectManager::GetInstance()));
    typedef pair<int, int> TGiLength;
    vector<TGiLength> gis;
    gis.push_back(TGiLength(555, 624));
    gis.push_back(TGiLength(556, 310));
    ifstream in("data/seq_entry_gis.asn");
    TSeqLocVector query_loc;

    ITERATE(vector<TGiLength>, gi, gis) {
        CRef<CSeq_entry> seq_entry(new CSeq_entry);
        in >> MSerial_AsnText >> *seq_entry;
        scope->AddTopLevelSeqEntry(*seq_entry);
        CRef<CSeq_id> id(new CSeq_id(CSeq_id::e_Gi, gi->first));
        CRef<CSeq_loc> sl(new CSeq_loc(*id, 0, gi->second));
        query_loc.push_back(SSeqLoc(sl, scope));
    }
    in.close();

    CRef<IQueryFactory> qf(new CObjMgr_QueryFactory(query_loc));
    const string kDbName("nt");
    CSearchDatabase db(kDbName,
                             CSearchDatabase::eBlastDbIsNucleotide);

    CRef<CSearchDatabase> target_db(&db);
    CRef<CBlastOptionsHandle> opts
        (CBlastOptionsFactory::Create(eBlastn, CBlastOptions::eRemote));

    CExportStrategy exp_ss(qf, opts, target_db);
    CRef<CBlast4_request> ss = exp_ss.GetSearchStrategy();
    BOOST_REQUIRE(ss.NotEmpty());


    const CBlast4_request_body& body = ss->GetBody();
    BOOST_REQUIRE(body.IsQueue_search());
    const CBlast4_queue_search_request& qsr = body.GetQueue_search();
    BOOST_REQUIRE(qsr.CanGetQueries());
    const CBlast4_queries& b4_queries = qsr.GetQueries();
    BOOST_REQUIRE_EQUAL(query_loc.size(), b4_queries.GetNumQueries());
    BOOST_REQUIRE( !b4_queries.IsBioseq_set() );
    BOOST_REQUIRE( !b4_queries.IsPssm() );
    BOOST_REQUIRE( b4_queries.IsSeq_loc_list() );

    // just as a bonus, check the database
    BOOST_REQUIRE(qsr.CanGetSubject());
    BOOST_REQUIRE(qsr.GetSubject().GetDatabase() == kDbName);
}

BOOST_AUTO_TEST_CASE(ExportStrategy_CBlastOptions)
{
	CRef<CBlastOptionsHandle> optsHandle;
	optsHandle = CBlastOptionsFactory::Create(eBlastp, CBlastOptions::eRemote);
    const CBlastOptions& opts = optsHandle->SetOptions();

    CExportStrategy exp_ss(optsHandle);
    CRef<CBlast4_request> ss = exp_ss.GetSearchStrategy();
    BOOST_REQUIRE(ss.NotEmpty());

    const CBlast4_request_body& body = ss->GetBody();
    BOOST_REQUIRE(body.IsQueue_search());
    const CBlast4_queue_search_request& qsr = body.GetQueue_search();

    string program;
    string service;
    opts.GetRemoteProgramAndService_Blast3(program, service);

    BOOST_REQUIRE(qsr.GetProgram() == program);
    BOOST_REQUIRE(qsr.GetService() == service);

}

BOOST_AUTO_TEST_SUITE_END()
