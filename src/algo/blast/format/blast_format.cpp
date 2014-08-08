#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] = "$Id: blast_format.cpp 390540 2013-02-28 14:38:34Z fongah2 $";
#endif /* SKIP_DOXYGEN_PROCESSING */

/*
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's offical duties as a United States Government employee and
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
* ===========================================================================*/

/*****************************************************************************

Author: Jason Papadopoulos

******************************************************************************/

/** @file blast_format.cpp
 * Produce formatted blast output
*/

#include <ncbi_pch.hpp>
#include <algo/blast/format/blast_format.hpp>
#include <objects/seq/Seq_annot.hpp>
#include <objects/seq/Seq_descr.hpp>
#include <objmgr/seq_loc_mapper.hpp>
#include <objmgr/util/sequence.hpp>
#include <algo/blast/core/blast_stat.h>
#include <corelib/ncbiutil.hpp>                 // for FindBestChoice
#include <algo/blast/api/sseqloc.hpp>
#include <algo/blast/api/objmgr_query_data.hpp>

#include <algo/blast/format/blastxml_format.hpp>
#include <algo/blast/format/data4xmlformat.hpp>       /* NCBI_FAKE_WARNING */
#include <algo/blast/format/build_archive.hpp>
#include <serial/objostrxml.hpp>

#include <corelib/ncbistre.hpp>

#ifndef SKIP_DOXYGEN_PROCESSING
USING_NCBI_SCOPE;
USING_SCOPE(blast);
USING_SCOPE(objects);
USING_SCOPE(align_format);
#endif

CBlastFormat::CBlastFormat(const blast::CBlastOptions& options, 
                           blast::CLocalDbAdapter& db_adapter,
                 blast::CFormattingArgs::EOutputFormat format_type, 
                 bool believe_query, CNcbiOstream& outfile,
                 int num_summary, 
                 int num_alignments, 
                 CScope & scope,
                 const char *matrix_name /* = BLAST_DEFAULT_MATRIX */,
                 bool show_gi /* = false */, 
                 bool is_html /* = false */,
                 int qgencode /* = BLAST_GENETIC_CODE */, 
                 int dbgencode /* = BLAST_GENETIC_CODE */,
                 bool use_sum_statistics /* = false */,
                 bool is_remote_search /* = false */,
                 int dbfilt_algorithm /* = -1 */,
                 const string& custom_output_format /* = kEmptyStr */,
                 bool is_megablast /* = false */,
                 bool is_indexed /* = false */,
                 const blast::CIgBlastOptions *ig_opts /* = NULL */,
                 const blast::CLocalDbAdapter* domain_db_adapter /* = NULL*/)
        : m_FormatType(format_type), m_IsHTML(is_html), 
          m_DbIsAA(db_adapter.IsProtein()), m_BelieveQuery(believe_query),
          m_Outfile(outfile), m_NumSummary(num_summary),
          m_NumAlignments(num_alignments), m_HitlistSize(options.GetHitlistSize()),
          m_Program(Blast_ProgramNameFromType(options.GetProgramType())), 
          m_DbName(kEmptyStr),
          m_QueryGenCode(qgencode), m_DbGenCode(dbgencode),
          m_ShowGi(show_gi), m_ShowLinkedSetSize(false),
          m_IsUngappedSearch(!options.GetGappedMode()),
          m_MatrixName(matrix_name),
          m_Scope(& scope),
          m_IsBl2Seq(false),
          m_IsRemoteSearch(is_remote_search),
          m_QueriesFormatted(0),
          m_Megablast(is_megablast),
          m_IndexedMegablast(is_indexed), 
          m_CustomOutputFormatSpec(custom_output_format),
          m_IgOptions(ig_opts),
          m_Options(&options)
{
    m_DbName = db_adapter.GetDatabaseName();
    m_IsBl2Seq = (m_DbName == kEmptyStr ? true : false);
    if (m_IsBl2Seq) {
        m_SeqInfoSrc.Reset(db_adapter.MakeSeqInfoSrc());
    } else {
        CBlastFormatUtil::GetBlastDbInfo(m_DbInfo, m_DbName, m_DbIsAA,
                                   dbfilt_algorithm, is_remote_search);
    }
    if (m_FormatType == CFormattingArgs::eXml) {
        m_AccumulatedQueries.Reset(new CBlastQueryVector());
        m_BlastXMLIncremental.Reset(new SBlastXMLIncremental());
    }
    if (use_sum_statistics && m_IsUngappedSearch) {
        m_ShowLinkedSetSize = true;
    }
    if ( m_Program == "blastn" &&
         options.GetMatchReward() == 0 &&
         options.GetMismatchPenalty() == 0 )
    {
       /* This combination is an indicator that we have used matrices
        * solely to develop the hsp score.  Also for the time being it
        * indicates that KA stats are not available. -RMH- 
        */
        m_DisableKAStats = true;
    }
    else
    {
        m_DisableKAStats = false;
    }

    CAlignFormatUtil::GetAsciiProteinMatrix(m_MatrixName, m_ScoringMatrix);

    if (options.GetProgram() == eDeltaBlast) {
        _ASSERT(options.GetProgramType() == eBlastTypePsiBlast);
        m_Program = "deltablast";

        if (domain_db_adapter) {
            CBlastFormatUtil::GetBlastDbInfo(m_DomainDbInfo,
                                         domain_db_adapter->GetDatabaseName(),
                                         true, -1, is_remote_search);
        }
    }
}

CBlastFormat::CBlastFormat(const blast::CBlastOptions& opts, 
                 const vector< CBlastFormatUtil::SDbInfo >& dbinfo_list,
                 blast::CFormattingArgs::EOutputFormat format_type, 
                 bool believe_query, CNcbiOstream& outfile,
                 int num_summary, 
                 int num_alignments,
                 CScope& scope,
                 bool show_gi, 
                 bool is_html, 
                 bool is_remote_search,
                 const string& custom_output_format)
        : m_FormatType(format_type),
          m_IsHTML(is_html), 
          m_DbIsAA(!Blast_SubjectIsNucleotide(opts.GetProgramType())),
          m_BelieveQuery(believe_query),
          m_Outfile(outfile),
          m_NumSummary(num_summary),
          m_NumAlignments(num_alignments),
          m_HitlistSize(opts.GetHitlistSize()),
          m_Program(Blast_ProgramNameFromType(opts.GetProgramType())), 
          m_DbName(kEmptyStr),
          m_QueryGenCode(opts.GetQueryGeneticCode()),
          m_DbGenCode(opts.GetDbGeneticCode()),
          m_ShowGi(show_gi),
          m_ShowLinkedSetSize(false),
          m_IsUngappedSearch(!opts.GetGappedMode()),
          m_MatrixName(opts.GetMatrixName()),
          m_Scope(&scope),
          m_IsBl2Seq(false),
          m_IsRemoteSearch(is_remote_search),
          m_QueriesFormatted(0),
          m_Megablast(opts.GetProgram() == eMegablast ||
                      opts.GetProgram() == eDiscMegablast),
          m_IndexedMegablast(opts.GetMBIndexLoaded()), 
          m_CustomOutputFormatSpec(custom_output_format),
          m_Options(&opts)
{
    m_DbInfo.assign(dbinfo_list.begin(), dbinfo_list.end());
    vector< CBlastFormatUtil::SDbInfo >::const_iterator itInfo;
    for (itInfo = m_DbInfo.begin(); itInfo != m_DbInfo.end(); itInfo++)
    {
        m_DbName += itInfo->name + " ";
    }

    m_IsBl2Seq = false;

    if (m_FormatType == CFormattingArgs::eXml) {
        m_AccumulatedQueries.Reset(new CBlastQueryVector());
        m_BlastXMLIncremental.Reset(new SBlastXMLIncremental());
    }

    if (opts.GetSumStatisticsMode() && m_IsUngappedSearch) {
        m_ShowLinkedSetSize = true;
    }

    if ( m_Program == "blastn" &&
         opts.GetMatchReward() == 0 &&
         opts.GetMismatchPenalty() == 0 )
    {
       /* This combination is an indicator that we have used matrices
        * solely to develop the hsp score.  Also for the time being it
        * indicates that KA stats are not available. -RMH-
        */
        m_DisableKAStats = true;
    }
    else
    {
        m_DisableKAStats = false;
    }

    CAlignFormatUtil::GetAsciiProteinMatrix(m_MatrixName, m_ScoringMatrix);

    if (opts.GetProgram() == eDeltaBlast) {
        _ASSERT(opts.GetProgramType() == eBlastTypePsiBlast);
        m_Program = "deltablast";
    }
}

static const string kHTML_Prefix =
"<HTML>\n"
"<TITLE>BLAST Search Results</TITLE>\n"
"<BODY BGCOLOR=\"#FFFFFF\" LINK=\"#0000FF\" VLINK=\"#660099\" ALINK=\"#660099\">\n"
"<PRE>\n";

static const string kHTML_Suffix =
"</PRE>\n"
"</BODY>\n"
"</HTML>";

void 
CBlastFormat::PrintProlog()
{
    // no header for some output types
    if (m_FormatType >= CFormattingArgs::eXml)
        return;

    if (m_IsHTML) {
        m_Outfile << kHTML_Prefix << "\n";
    }
    // Make sure no-one confuses us with the standard BLASTN 
    // algorithm.  -RMH-
    if ( m_Program == "blastn" &&
         m_DisableKAStats == true )
    {
      CBlastFormatUtil::BlastPrintVersionInfo("rmblastn", m_IsHTML,
                                              m_Outfile);
      m_Outfile << "\n\n";
      m_Outfile << "Reference: Robert M. Hubley, Arian Smit\n";
      m_Outfile << "RMBlast - RepeatMasker Search Engine\n";
      m_Outfile << "2010 <http://www.repeatmasker.org>";
    }else
    {
      CBlastFormatUtil::BlastPrintVersionInfo(m_Program, m_IsHTML,
                                              m_Outfile);
    }

    if (m_IsBl2Seq) {
        return;
    }

    m_Outfile << "\n\n";
    if (m_Program == "deltablast") {
        CBlastFormatUtil::BlastPrintReference(m_IsHTML, kFormatLineLength, 
                              m_Outfile, CReference::eDeltaBlast);
        m_Outfile << "\n";
    }

    if (m_Megablast)
        CBlastFormatUtil::BlastPrintReference(m_IsHTML, kFormatLineLength, 
                                          m_Outfile, CReference::eMegaBlast);
    else
        CBlastFormatUtil::BlastPrintReference(m_IsHTML, kFormatLineLength, 
                                          m_Outfile);

    if (m_Megablast && m_IndexedMegablast)
    {
        m_Outfile << "\n";
        CBlastFormatUtil::BlastPrintReference(m_IsHTML, kFormatLineLength, 
                              m_Outfile, CReference::eIndexedMegablast);
    }

    if (m_Program == "psiblast" || m_Program == "deltablast") {
        m_Outfile << "\n";
        CBlastFormatUtil::BlastPrintReference(m_IsHTML, kFormatLineLength, 
                              m_Outfile, CReference::eCompAdjustedMatrices);
    }
    if (m_Program == "psiblast" || m_Program == "blastp") {
        m_Outfile << "\n";
        CBlastFormatUtil::BlastPrintReference(m_IsHTML, kFormatLineLength, 
                              m_Outfile, CReference::eCompBasedStats,
                              (bool)(m_Program == "psiblast"));
    }

    if (m_Program == "deltablast" || !m_DomainDbInfo.empty()) {
        m_Outfile << "\n\n";
        if (!m_DomainDbInfo.empty()) {
        	m_Outfile << "\n\n" << "Conserved Domain ";
        	CBlastFormatUtil::PrintDbReport(m_DomainDbInfo, kFormatLineLength, 
                                        m_Outfile, true);
        }
    }
    else {
        m_Outfile << "\n\n";
    }
    CBlastFormatUtil::PrintDbReport(m_DbInfo, kFormatLineLength, 
                                    m_Outfile, true);
}

void
CBlastFormat::x_PrintOneQueryFooter(const blast::CBlastAncillaryData& summary)
{
    /* Skip printing KA parameters if the program is rmblastn -RMH- */
    if ( m_DisableKAStats )
      return;

    const Blast_KarlinBlk *kbp_ungap = 
        (m_Program == "psiblast" || m_Program == "deltablast")
        ? summary.GetPsiUngappedKarlinBlk() 
        : summary.GetUngappedKarlinBlk();
    const Blast_GumbelBlk *gbp = summary.GetGumbelBlk();
    m_Outfile << "\n";
    if (kbp_ungap) {
        CBlastFormatUtil::PrintKAParameters(kbp_ungap->Lambda, 
                                            kbp_ungap->K, kbp_ungap->H,
                                            kFormatLineLength, m_Outfile,
                                            false, gbp);
    }

    const Blast_KarlinBlk *kbp_gap = 
        (m_Program == "psiblast" || m_Program == "deltablast")
        ? summary.GetPsiGappedKarlinBlk()
        : summary.GetGappedKarlinBlk();
    m_Outfile << "\n";
    if (kbp_gap) {
        CBlastFormatUtil::PrintKAParameters(kbp_gap->Lambda, 
                                            kbp_gap->K, kbp_gap->H,
                                            kFormatLineLength, m_Outfile,
                                            true, gbp);
    }

    m_Outfile << "\n";
    m_Outfile << "Effective search space used: " << 
                        summary.GetSearchSpace() << "\n";
}

/// Auxialiary function to determine if there are local IDs in the identifiers
/// of the query sequences
/// @param queries query sequence(s) [in]
static bool 
s_HasLocalIDs(CConstRef<CBlastQueryVector> queries)
{
    bool retval = false;
    ITERATE(CBlastQueryVector, itr, *queries) {
        if (blast::IsLocalId((*itr)->GetQuerySeqLoc()->GetId())) {
            retval = true;
            break;
        }
    }
    return retval;
}

void 
CBlastFormat::x_ConfigCShowBlastDefline(CShowBlastDefline& showdef, 
                                        int skip_from, int skip_to, int index,
                                        int num_descriptions_to_show /* = -1 */)
{
    int flags = 0;
    if (m_ShowLinkedSetSize)
        flags |= CShowBlastDefline::eShowSumN;
    if (m_IsHTML){
        flags |= CShowBlastDefline::eHtml;
        if (index >= 0) {
            showdef.SetResultPosIndex(index); 
        }
    }
    if (m_ShowGi)
        flags |= CShowBlastDefline::eShowGi;
    if (num_descriptions_to_show == 0)
        flags |= CShowBlastDefline::eNoShowHeader;

    showdef.SetOption(flags);
    showdef.SetDbName(m_DbName);
    showdef.SetDbType(!m_DbIsAA);
    showdef.SetSkipRange(skip_from, skip_to);
}

void
CBlastFormat::x_SplitSeqAlign(CConstRef<CSeq_align_set> full_alignment,
                       CSeq_align_set& repeated_seqs,
                       CSeq_align_set& new_seqs,
                       blast::CPsiBlastIterationState::TSeqIds& prev_seqids)
{
    static const CSeq_align::TDim kSubjRow = 1;
    _ASSERT( !prev_seqids.empty() );
    _ASSERT( !full_alignment->IsEmpty() );
    _ASSERT(repeated_seqs.IsEmpty());
    _ASSERT(new_seqs.IsEmpty());

    unsigned int count = 0;
    ITERATE(CSeq_align_set::Tdata, alignment, full_alignment->Get()) {
        CSeq_id_Handle subj_id =
            CSeq_id_Handle::GetHandle((*alignment)->GetSeq_id(kSubjRow));
        if (prev_seqids.find(subj_id) != prev_seqids.end()) {
            // if found among previously seen Seq-ids...
            repeated_seqs.Set().push_back(*alignment);
        } else {
            // ... else add them as new
            new_seqs.Set().push_back(*alignment);
        }
        count++;
        if(count >= m_NumSummary)
        	break;
    }
}

bool
s_IsGlobalSeqAlign(CConstRef<objects::CSeq_align_set> seqalign_set)
{
   bool kIsGlobal = (seqalign_set->IsSet() && seqalign_set->CanGet() &&
          seqalign_set->Get().front()->CanGetType() &&
          seqalign_set->Get().front()->GetType() == CSeq_align_Base::eType_global);

   return kIsGlobal;
}


void
CBlastFormat::x_DisplayDeflines(CConstRef<CSeq_align_set> aln_set, 
                                unsigned int itr_num,
                                blast::CPsiBlastIterationState::TSeqIds& prev_seqids,
                                int additional,
                                int index,
                                int defline_length )
{

    if (itr_num != numeric_limits<unsigned int>::max() && 
        !prev_seqids.empty()) {
        // Split seq-align-set
        CSeq_align_set repeated_seqs, new_seqs;
        x_SplitSeqAlign(aln_set, repeated_seqs, new_seqs, prev_seqids);

        // Show deflines for 'repeat' sequences
        {{
            CShowBlastDefline showdef(repeated_seqs, *m_Scope, 
                                      kFormatLineLength,
                                  	  repeated_seqs.Size());
            x_ConfigCShowBlastDefline(showdef);
            showdef.SetupPsiblast(NULL, CShowBlastDefline::eRepeatPass);
            showdef.DisplayBlastDefline(m_Outfile);
        }}
        m_Outfile << "\n";

        // Show deflines for 'new' sequences
        {{
            CShowBlastDefline showdef(new_seqs, *m_Scope, kFormatLineLength,
                              	  	  new_seqs.Size());
            x_ConfigCShowBlastDefline(showdef);
            showdef.SetupPsiblast(NULL, CShowBlastDefline::eNewPass);
            showdef.DisplayBlastDefline(m_Outfile);
        }}

    } else {
        
        CShowBlastDefline showdef(*aln_set, *m_Scope, 
                                  defline_length == -1 ? kFormatLineLength:defline_length,
                                  m_NumSummary + additional);
        x_ConfigCShowBlastDefline(showdef, -1, -1, index,
                                  m_NumSummary+additional);
        showdef.DisplayBlastDefline(m_Outfile);
    }
    m_Outfile << "\n";
}

int
s_SetFlags(string& program, 
    blast::CFormattingArgs::EOutputFormat format_type,
    bool html, bool showgi, bool isbl2seq, bool disableKAStats)
{
   // set the alignment flags
    int flags = CDisplaySeqalign::eShowBlastInfo;

    if ( isbl2seq ) {
        flags |= CDisplaySeqalign::eShowNoDeflineInfo;
    }
    
    if (html)
        flags |= CDisplaySeqalign::eHtml;
    if (showgi)
        flags |= CDisplaySeqalign::eShowGi;

    if (format_type >= CFormattingArgs::eQueryAnchoredIdentities &&
        format_type <= CFormattingArgs::eFlatQueryAnchoredNoIdentities) {
        flags |= CDisplaySeqalign::eMergeAlign;
    }
    else {
        flags |= CDisplaySeqalign::eShowBlastStyleId |
                 CDisplaySeqalign::eShowMiddleLine;
    }

    if (format_type == CFormattingArgs::eQueryAnchoredIdentities ||
        format_type == CFormattingArgs::eFlatQueryAnchoredIdentities) {
        flags |= CDisplaySeqalign::eShowIdentity;
    }
    if (format_type == CFormattingArgs::eQueryAnchoredIdentities ||
        format_type == CFormattingArgs::eQueryAnchoredNoIdentities) {
        flags |= CDisplaySeqalign::eMasterAnchored;
    }
    if (program == "tblastx") {
        flags |= CDisplaySeqalign::eTranslateNucToNucAlignment;
    }

    if (disableKAStats)
        flags |= CDisplaySeqalign::eShowRawScoreOnly;

    return flags;
}

// Port of jzmisc.c's AddAlignInfoToSeqAnnotEx (CVS revision 6.11)
CRef<objects::CSeq_annot>
CBlastFormat::x_WrapAlignmentInSeqAnnot(CConstRef<objects::CSeq_align_set> alnset) const
{
	return CBlastFormatUtil::CreateSeqAnnotFromSeqAlignSet(*alnset,
														   ProgramNameToEnum(m_Program),
														   m_DbName);
}

void 
CBlastFormat::x_PrintStructuredReport(const blast::CSearchResults& results,
              CConstRef<blast::CBlastQueryVector> queries)
{
   CConstRef<CSeq_align_set> aln_set = results.GetSeqAlign();

    // ASN.1 formatting is straightforward
    if (m_FormatType == CFormattingArgs::eAsnText) {
        if (results.HasAlignments()) {
            m_Outfile << MSerial_AsnText << *x_WrapAlignmentInSeqAnnot(aln_set);
        }
        return;
    } else if (m_FormatType == CFormattingArgs::eAsnBinary) {
        if (results.HasAlignments()) {
            m_Outfile << MSerial_AsnBinary <<
                *x_WrapAlignmentInSeqAnnot(aln_set);
        }
        return;
    } else if (m_FormatType == CFormattingArgs::eXml) {
        CRef<CSearchResults> res(const_cast<CSearchResults*>(&results));
        m_AccumulatedResults.push_back(res);
        CConstRef<CSeq_id> query_id = results.GetSeqId();
        // FIXME: this can be a bottleneck with large numbers of queries
        ITERATE(CBlastQueryVector, itr, *queries) {
            if (query_id->Match(*(*itr)->GetQueryId())) {
                m_AccumulatedQueries->push_back(*itr);
                break;
            }
        }
        CCmdLineBlastXMLReportData report_data(m_AccumulatedQueries, 
                                               m_AccumulatedResults,
                                               *m_Options, m_DbName, m_DbIsAA,
                                               m_QueryGenCode, m_DbGenCode,
                                               m_IsRemoteSearch);
        objects::CBlastOutput xml_output;
        BlastXML_FormatReport(xml_output, &report_data, &m_Outfile, 
           m_BlastXMLIncremental.GetPointer());
        m_AccumulatedResults.clear();
        m_AccumulatedQueries->clear();
        return;
    }
}

void
CBlastFormat::x_PrintTabularReport(const blast::CSearchResults& results, 
                                   unsigned int itr_num)
{
    CConstRef<CSeq_align_set> aln_set = results.GetSeqAlign();
    if (m_IsUngappedSearch && results.HasAlignments()) {
        aln_set.Reset(CDisplaySeqalign::PrepareBlastUngappedSeqalign(*aln_set));
    }
    // other output types will need a bioseq handle
    CBioseq_Handle bhandle = m_Scope->GetBioseqHandle(*results.GetSeqId(),
                                                      CScope::eGetBioseq_All);

    // tabular formatting just prints each alignment in turn
    // (plus a header)
    if (m_FormatType == CFormattingArgs::eTabular ||
        m_FormatType == CFormattingArgs::eTabularWithComments ||
        m_FormatType == CFormattingArgs::eCommaSeparatedValues) {
        const CBlastTabularInfo::EFieldDelimiter kDelim =
            (m_FormatType == CFormattingArgs::eCommaSeparatedValues
             ? CBlastTabularInfo::eComma : CBlastTabularInfo::eTab);

        CBlastTabularInfo tabinfo(m_Outfile, m_CustomOutputFormatSpec, kDelim);
        tabinfo.SetParseLocalIds(m_BelieveQuery);
        if (ncbi::NStr::ToLower(m_Program) == string("blastn"))
        	tabinfo.SetNoFetch(true);

        if (m_FormatType == CFormattingArgs::eTabularWithComments) {
            string strProgVersion =
                NStr::ToUpper(m_Program) + " " + blast::CBlastVersion().Print();
            CConstRef<CBioseq> subject_bioseq = x_CreateSubjectBioseq();
            tabinfo.PrintHeader(strProgVersion, *(bhandle.GetBioseqCore()),
                                m_DbName, results.GetRID(), itr_num, aln_set,
                                subject_bioseq);
        }

        if (results.HasAlignments()) {
    	    CSeq_align_set copy_aln_set;
            CBlastFormatUtil::PruneSeqalign(*aln_set, copy_aln_set, m_HitlistSize);

            if(string::npos != m_CustomOutputFormatSpec.find("qcovs"))
            		CBlastFormatUtil::InsertSubjectScores (copy_aln_set, bhandle);

            ITERATE(CSeq_align_set::Tdata, itr, copy_aln_set.Get()) {
                    const CSeq_align& s = **itr;
                    tabinfo.SetFields(s, *m_Scope, &m_ScoringMatrix);
                    tabinfo.Print();
            }
        }
        return;
    }
}

void
CBlastFormat::x_PrintIgTabularReport(const blast::CIgBlastResults& results)
{
    CConstRef<CSeq_align_set> aln_set = results.GetSeqAlign();
    /* TODO do we support ungapped Igblast search?
    if (m_IsUngappedSearch && results.HasAlignments()) {
        aln_set.Reset(CDisplaySeqalign::PrepareBlastUngappedSeqalign(*aln_set));
    } */
    // other output types will need a bioseq handle
    CBioseq_Handle bhandle = m_Scope->GetBioseqHandle(*results.GetSeqId(),
                                                      CScope::eGetBioseq_All);

    // tabular formatting just prints each alignment in turn
    // (plus a header)
    if (m_FormatType != CFormattingArgs::eTabular &&
        m_FormatType != CFormattingArgs::eTabularWithComments &&
        m_FormatType != CFormattingArgs::eCommaSeparatedValues) return;

    const CBlastTabularInfo::EFieldDelimiter kDelim =
            (m_FormatType == CFormattingArgs::eCommaSeparatedValues
             ? CBlastTabularInfo::eComma : CBlastTabularInfo::eTab);

    CIgBlastTabularInfo tabinfo(m_Outfile, m_CustomOutputFormatSpec, kDelim);
    tabinfo.SetParseLocalIds(m_BelieveQuery);

    string strProgVersion =
                "IG" + NStr::ToUpper(m_Program) + " " + blast::CBlastVersion().Print();
    CConstRef<CBioseq> subject_bioseq = x_CreateSubjectBioseq();

    if (m_IsHTML) {
        m_Outfile << "<html><body><pre>\n";
    }
    if (results.HasAlignments()) {
        const CRef<CIgAnnotation> & annots = results.GetIgAnnotation();
        CSeq_align_set::Tdata::const_iterator itr = aln_set->Get().begin();
        tabinfo.SetMasterFields(**itr, *m_Scope, 
                                annots->m_ChainType[0], 
                                annots->m_ChainTypeToShow, 
                                &m_ScoringMatrix);
        tabinfo.SetIgAnnotation(annots, m_IgOptions->m_IsProtein);

        tabinfo.PrintHeader(strProgVersion, *(bhandle.GetBioseqCore()),
                                m_DbName, 
                                m_IgOptions->m_DomainSystem,
                                results.GetRID(), 
                                numeric_limits<unsigned int>::max(),
                                aln_set, subject_bioseq);
 
        int j = 1;
        for (; itr != aln_set->Get().end(); ++itr) {
            tabinfo.SetFields(**itr, *m_Scope, 
                              annots->m_ChainType[j++], 
                              annots->m_ChainTypeToShow,
                              &m_ScoringMatrix);
            tabinfo.Print();
        }
    } else {
        tabinfo.PrintHeader(strProgVersion, *(bhandle.GetBioseqCore()),
                                m_DbName, 
                                m_IgOptions->m_DomainSystem,
                                results.GetRID(), 
                                numeric_limits<unsigned int>::max(),
                                0, subject_bioseq);
    }
    if (m_IsHTML) {
        m_Outfile << "\n</pre></body></html>\n";
    }
}

CConstRef<objects::CBioseq> CBlastFormat::x_CreateSubjectBioseq()
{
    if ( !m_IsBl2Seq ) {
        return CConstRef<CBioseq>();
    }

    _ASSERT(m_IsBl2Seq);
    _ASSERT(m_SeqInfoSrc);
    static Uint4 subj_index = 0;

    list< CRef<CSeq_id> > ids = m_SeqInfoSrc->GetId(subj_index++);
    CRef<CSeq_id> id = FindBestChoice(ids, CSeq_id::BestRank);
    CBioseq_Handle bhandle = m_Scope->GetBioseqHandle(*id,
                                                      CScope::eGetBioseq_All);
    // If this assertion fails, we're not able to get the subject, possibly a
    // programming error (see @note in this function's declaration - was the
    // order of calls altered?)
    _ASSERT(bhandle);

    // reset the subject index if necessary
    if (subj_index >= m_SeqInfoSrc->Size()) {
        subj_index = 0;
    }
    return bhandle.GetBioseqCore();
}

/// Auxiliary function to print the BLAST Archive in multiple output formats
static void s_PrintArchive(CRef<objects::CBlast4_archive> archive,
                         CNcbiOstream& out)
{
    if (archive.Empty()) {
        return;
    }
    string outfmt = CNcbiEnvironment().Get("ARCHIVE_FORMAT");
    if (outfmt.empty()) {
        out << MSerial_AsnText << *archive;
    } else if (!NStr::CompareNocase(outfmt, "xml")) {
        out << MSerial_Xml << *archive;
    } else if (NStr::StartsWith(outfmt, "bin", NStr::eNocase)) {
        out << MSerial_AsnBinary << *archive;
    }
}

void 
CBlastFormat::WriteArchive(blast::IQueryFactory& queries,
                           blast::CBlastOptionsHandle& options_handle,
                           const CSearchResultSet& results,
                           unsigned int num_iters)
{
    CRef<objects::CBlast4_archive>  archive;
    if (m_IsBl2Seq)
    {
	CRef<CBlastQueryVector> query_vector(new CBlastQueryVector);
	for (unsigned int i=0; i<m_SeqInfoSrc->Size(); i++)
        {
		list< CRef<CSeq_id> > ids = m_SeqInfoSrc->GetId(i);
                CRef<CSeq_id> id = FindBestChoice(ids, CSeq_id::BestRank);
                CRef<CSeq_loc> seq_loc(new CSeq_loc);
                seq_loc->SetWhole(*id);
                CRef<CBlastSearchQuery> search_query(new CBlastSearchQuery(*seq_loc, *m_Scope));
                query_vector->AddQuery(search_query);
        }
        CObjMgr_QueryFactory subjects(*query_vector);
        archive = BlastBuildArchive(queries, options_handle, results, subjects);
        
    }
    else
    {
    	// Use only by psi blast
    	if(num_iters != 0)
    		archive = BlastBuildArchive(queries, options_handle, results,  m_DbName, num_iters);
    	else
    		archive = BlastBuildArchive(queries, options_handle, results,  m_DbName);
    }
    s_PrintArchive(archive, m_Outfile);
}

void
CBlastFormat::WriteArchive(objects::CPssmWithParameters & pssm,
                           blast::CBlastOptionsHandle& options_handle,
                           const CSearchResultSet& results,
                           unsigned int num_iters)
{
    CRef<objects::CBlast4_archive> archive(BlastBuildArchive(pssm, options_handle, results,  m_DbName, num_iters));
    s_PrintArchive(archive, m_Outfile);
}


void
CBlastFormat::PrintOneResultSet(const blast::CSearchResults& results,
                        CConstRef<blast::CBlastQueryVector> queries,
                        unsigned int itr_num
                        /* = numeric_limits<unsigned int>::max() */,
                        blast::CPsiBlastIterationState::TSeqIds prev_seqids
                        /* = CPsiBlastIterationState::TSeqIds() */,
                        bool is_deltablast_domain_result /* = false */)
{
    // For remote searches, we don't retrieve the sequence data for the query
    // sequence when initially sending the request to the BLAST server (if it's
    // a GI/accession/TI), so we flush the scope so that it can be retrieved
    // (needed if a self-hit is found) again. This is not applicable if the
    // query sequence(s) are specified as FASTA (will be identified by local
    // IDs).
    if (m_IsRemoteSearch && !s_HasLocalIDs(queries)) {
        ResetScopeHistory();
    }

    // Used with tabular output to print number of searches formatted at end.
    m_QueriesFormatted++;

    if (m_FormatType == CFormattingArgs::eAsnText 
      || m_FormatType == CFormattingArgs::eAsnBinary 
      || m_FormatType == CFormattingArgs::eXml )
    {
        x_PrintStructuredReport(results, queries);
        return;
    }

    if (results.HasErrors()) {
        ERR_POST(Error << results.GetErrorStrings());
        return; // errors are deemed fatal
    }
    if (results.HasWarnings()) {
        ERR_POST(Warning << results.GetWarningStrings());
    }

    if (m_FormatType == CFormattingArgs::eTabular ||
        m_FormatType == CFormattingArgs::eTabularWithComments ||
        m_FormatType == CFormattingArgs::eCommaSeparatedValues) {
        x_PrintTabularReport(results, itr_num);
        return;
    }
    const bool kIsTabularOutput = false;

    if (is_deltablast_domain_result) {
        m_Outfile << "Results from domain search" << "\n";
    }

    if (itr_num != numeric_limits<unsigned int>::max()) {
        m_Outfile << "Results from round " << itr_num << "\n";
    }

    // other output types will need a bioseq handle
    CBioseq_Handle bhandle = m_Scope->GetBioseqHandle(*results.GetSeqId(),
                                                      CScope::eGetBioseq_All);
    // If we're not able to get the query, most likely a bug. SB-981 , GP-2207
    if( !bhandle  ){
        string message = "Failed to resolve SeqId: "+results.GetSeqId()->AsFastaString();
	ERR_POST(message);
        NCBI_THROW(CException, eUnknown, message);
    }
    CConstRef<CBioseq> bioseq = bhandle.GetBioseqCore();

    // print the preamble for this query

    m_Outfile << "\n\n";
    CBlastFormatUtil::AcknowledgeBlastQuery(*bioseq, kFormatLineLength,
                                            m_Outfile, m_BelieveQuery,
                                            m_IsHTML, kIsTabularOutput,
                                            results.GetRID());
    if (m_IsBl2Seq) {
        m_Outfile << "\n";
        // FIXME: this might be configurable in the future
        const bool kBelieveSubject = false; 
        CConstRef<CBioseq> subject_bioseq = x_CreateSubjectBioseq();
        CBlastFormatUtil::AcknowledgeBlastSubject(*subject_bioseq, 
                                                  kFormatLineLength, 
                                                  m_Outfile, kBelieveSubject, 
                                                  m_IsHTML, kIsTabularOutput);
    }

    // quit early if there are no hits
    if ( !results.HasAlignments() ) {
        m_Outfile << "\n\n" 
              << "***** " << CBlastFormatUtil::kNoHitsFound << " *****" << "\n" 
              << "\n\n";
        x_PrintOneQueryFooter(*results.GetAncillaryData());
        return;
    }

    CConstRef<CSeq_align_set> aln_set = results.GetSeqAlign();
    _ASSERT(results.HasAlignments());
    if (m_IsUngappedSearch) {
        aln_set.Reset(CDisplaySeqalign::PrepareBlastUngappedSeqalign(*aln_set));
    }

    const bool kIsGlobal = s_IsGlobalSeqAlign(aln_set);

    //-------------------------------------------------
    // print 1-line summaries
    // Also disable when program is rmblastn.  At this time
    // we do not want summary bit scores/evalues for this 
    // program. -RMH-
    if ( !m_IsBl2Seq && !(m_DisableKAStats || kIsGlobal) ) {
        x_DisplayDeflines(aln_set, itr_num, prev_seqids);
    }

    //-------------------------------------------------
    // print the alignments
    m_Outfile << "\n";

    TMaskedQueryRegions masklocs;
    results.GetMaskedQueryRegions(masklocs);

    CSeq_align_set copy_aln_set;
    CBlastFormatUtil::PruneSeqalign(*aln_set, copy_aln_set, m_NumAlignments);

    int flags = s_SetFlags(m_Program, m_FormatType, m_IsHTML, m_ShowGi,
                           m_IsBl2Seq, (m_DisableKAStats || kIsGlobal));

    CDisplaySeqalign display(copy_aln_set, *m_Scope, &masklocs, NULL, m_MatrixName);
    display.SetDbName(m_DbName);
    display.SetDbType(!m_DbIsAA);

    // set the alignment flags
    display.SetAlignOption(flags);

    if (m_Program == "blastn" || m_Program == "megablast") {
            display.SetMiddleLineStyle(CDisplaySeqalign::eBar);
            display.SetAlignType(CDisplaySeqalign::eNuc);
    }
    else {
            display.SetMiddleLineStyle(CDisplaySeqalign::eChar);
            display.SetAlignType(CDisplaySeqalign::eProt);
    }

    display.SetMasterGeneticCode(m_QueryGenCode);
    display.SetSlaveGeneticCode(m_DbGenCode);
    display.SetSeqLocChar(CDisplaySeqalign::eLowerCase);
    TSeqLocInfoVector subj_masks;
    results.GetSubjectMasks(subj_masks);
    display.SetSubjectMasks(subj_masks);
    display.DisplaySeqalign(m_Outfile);

    // print the ancillary data for this query

    x_PrintOneQueryFooter(*results.GetAncillaryData());
}

void
CBlastFormat::PrintOneResultSet(blast::CIgBlastResults& results,
                                CConstRef<blast::CBlastQueryVector> queries, 
                                int index)
{

    // For remote searches, we don't retrieve the sequence data for the query
    // sequence when initially sending the request to the BLAST server (if it's
    // a GI/accession/TI), so we flush the scope so that it can be retrieved
    // (needed if a self-hit is found) again. This is not applicable if the
    // query sequence(s) are specified as FASTA (will be identified by local
    // IDs).
    if (m_IsRemoteSearch && !s_HasLocalIDs(queries)) {
        ResetScopeHistory();
    }

    // Used with tabular output to print number of searches formatted at end.
    m_QueriesFormatted++;

    if (m_FormatType == CFormattingArgs::eAsnText 
      || m_FormatType == CFormattingArgs::eAsnBinary 
      || m_FormatType == CFormattingArgs::eXml )
    {
        x_PrintStructuredReport(results, queries);
        return;
    }

    if (results.HasErrors()) {
        ERR_POST(Error << results.GetErrorStrings());
        return; // errors are deemed fatal
    }
    if (results.HasWarnings()) {
        ERR_POST(Warning << results.GetWarningStrings());
    }

    if (results.GetIgAnnotation()->m_MinusStrand) {
        x_ReverseQuery(results);
    }

    if (m_FormatType == CFormattingArgs::eTabular ||
        m_FormatType == CFormattingArgs::eTabularWithComments ||
        m_FormatType == CFormattingArgs::eCommaSeparatedValues) {
        m_FormatType = CFormattingArgs::eTabularWithComments;
        x_PrintIgTabularReport(results);
        return;
    }


    const bool kIsTabularOutput = false;

    // other output types will need a bioseq handle
    CBioseq_Handle bhandle = m_Scope->GetBioseqHandle(*results.GetSeqId(),
                                                      CScope::eGetBioseq_All);
    // If this assertion fails, we're not able to get the query, most likely a
    // bug
    _ASSERT(bhandle);
    CConstRef<CBioseq> bioseq = bhandle.GetBioseqCore();

    // print the preamble for this query

    m_Outfile << "\n\n";

    CBlastFormatUtil::AcknowledgeBlastQuery(*bioseq, kFormatLineLength,
                                            m_Outfile, m_BelieveQuery,
                                            m_IsHTML, kIsTabularOutput,
                                            results.GetRID());

    // quit early if there are no hits
    if ( !results.HasAlignments() ) {
        m_Outfile << "\n\n" 
              << "***** " << CBlastFormatUtil::kNoHitsFound << " *****" << "\n" 
              << "\n\n";
        x_PrintOneQueryFooter(*results.GetAncillaryData());
        return;
    }

    CConstRef<CSeq_align_set> aln_set = results.GetSeqAlign();
    _ASSERT(results.HasAlignments());
    if (m_IsUngappedSearch) {
        aln_set.Reset(CDisplaySeqalign::PrepareBlastUngappedSeqalign(*aln_set));
    }

    //-------------------------------------------------
    // print 1-line summaries
    if ( !m_IsBl2Seq ) {
        CPsiBlastIterationState::TSeqIds prev_ids = CPsiBlastIterationState::TSeqIds();
        int additional =  results.m_NumActualV +results.m_NumActualD + results.m_NumActualJ;
        x_DisplayDeflines(aln_set, numeric_limits<unsigned int>::max(), prev_ids, additional, index, 100);
    }

    //-------------------------------------------------
    // print the alignments
    m_Outfile << "\n";

    const CBlastTabularInfo::EFieldDelimiter kDelim =
            (m_FormatType == CFormattingArgs::eCommaSeparatedValues
             ? CBlastTabularInfo::eComma : CBlastTabularInfo::eTab);

    CIgBlastTabularInfo tabinfo(m_Outfile, m_CustomOutputFormatSpec, kDelim);
    tabinfo.SetParseLocalIds(m_BelieveQuery);

    // print the master alignment
    if (results.HasAlignments()) {
        const CRef<CIgAnnotation> & annots = results.GetIgAnnotation();
        CSeq_align_set::Tdata::const_iterator itr = aln_set->Get().begin();
        tabinfo.SetMasterFields(**itr, *m_Scope, 
                                annots->m_ChainType[0], 
                                annots->m_ChainTypeToShow, 
                                &m_ScoringMatrix);
        tabinfo.SetIgAnnotation(annots, m_IgOptions->m_IsProtein);
        m_Outfile << "Domain classification requested: " << m_IgOptions->m_DomainSystem << endl << endl;
        if (m_IsHTML) {
            tabinfo.PrintHtmlSummary();
        } else {
            tabinfo.PrintMasterAlign("");
        }
    }

    TMaskedQueryRegions masklocs;
    results.GetMaskedQueryRegions(masklocs);

    int flags = CDisplaySeqalign::eMergeAlign
        + CDisplaySeqalign::eShowIdentity
        + CDisplaySeqalign::eNewTargetWindow
        + CDisplaySeqalign::eShowEndGaps
        + CDisplaySeqalign::eShowAlignStatsForMultiAlignView;

    if (m_FormatType == CFormattingArgs::eFlatQueryAnchoredNoIdentities) {
        flags -= CDisplaySeqalign::eShowIdentity;
    }
    
    if (m_IsHTML) {
          flags += CDisplaySeqalign::eHtml;
          flags += CDisplaySeqalign::eHyperLinkSlaveSeqid;
    }

    list < CRef<CDisplaySeqalign::DomainInfo> >  domain;
    string domain_name[] = {"FWR1", "CDR1", "FWR2", "CDR2", "FWR3"};
    const CRef<CIgAnnotation> & annots = results.GetIgAnnotation();
    
    for (int i=0; i<9; i = i + 2) {
        if (annots->m_DomainInfo[i] >= 0){      
            CRef<CDisplaySeqalign::DomainInfo> temp(new CDisplaySeqalign::DomainInfo);
            int start = annots->m_DomainInfo[i];
            int subject_start = annots->m_DomainInfo_S[i];

            int stop = annots->m_DomainInfo[i+1];
            int subject_stop = annots->m_DomainInfo_S[i+1];

            temp->seqloc = new CSeq_loc((CSeq_loc::TId &) aln_set->Get().front()->GetSeq_id(0),
                                        (CSeq_loc::TPoint) start,
                                        (CSeq_loc::TPoint) stop);
            temp->subject_seqloc = new CSeq_loc((CSeq_loc::TId &) aln_set->Get().front()->GetSeq_id(1),
                                                (CSeq_loc::TPoint) subject_start,
                                                (CSeq_loc::TPoint) subject_stop);
            temp->is_subject_start_valid = subject_start > 0 ? true:false;
            temp->is_subject_stop_valid = subject_stop > 0 ? true:false;
            temp->domain_name = domain_name[i/2];
            domain.push_back(temp); 
        }
    }    


    CDisplaySeqalign display(*aln_set, *m_Scope, &masklocs, NULL,  m_MatrixName);
    int num_align_to_show = results.m_NumActualV + results.m_NumActualD + 
        results.m_NumActualJ;
    if (m_DbName != m_IgOptions->m_Db[0]->GetDatabaseName()){
        num_align_to_show += m_NumAlignments;
    }
    display.SetNumAlignToShow(num_align_to_show);
    display.SetMasterDomain(&domain);
    display.SetDbName(m_DbName);
    display.SetDbType(!m_DbIsAA);
    display.SetLineLen(90);

    if (annots->m_FrameInfo[0] >= 0 && m_IgOptions->m_Translate) {
        display.SetTranslatedFrameForLocalSeq((CDisplaySeqalign::TranslatedFrameForLocalSeq) (annots->m_FrameInfo[0]%3)); 
        flags += CDisplaySeqalign::eShowTranslationForLocalSeq;
    }
    flags += CDisplaySeqalign::eShowSequencePropertyLabel;
    flags += CDisplaySeqalign::eShowInfoOnMouseOverSeqid;
    vector<string> chain_type_list;
    ITERATE(vector<string>, iter, annots->m_ChainType) {
        if (*iter=="N/A"){
            chain_type_list.push_back(NcbiEmptyString);
        } else {
            chain_type_list.push_back(*iter); 
        }
    }
    display.SetSequencePropertyLabel(&chain_type_list);
    // set the alignment flags
    display.SetAlignOption(flags);
    if (m_Program == "blastn") {
        display.SetAlignType(CDisplaySeqalign::eNuc);
    } else {
        display.SetAlignType(CDisplaySeqalign::eProt);
    }
    display.SetMasterGeneticCode(m_QueryGenCode);
    display.SetSlaveGeneticCode(m_DbGenCode);
    display.SetSeqLocChar(CDisplaySeqalign::eLowerCase);
    TSeqLocInfoVector subj_masks;
    results.GetSubjectMasks(subj_masks);
    display.SetSubjectMasks(subj_masks);

    if (m_IsHTML) {
        display.SetResultPositionIndex(index); 
        m_Outfile << "\n<CENTER><b><FONT color=\"green\">Alignments</FONT></b></CENTER>" 
                  << endl;

    } else {
        m_Outfile << "\nAlignments" << endl;
    }
       
    display.DisplaySeqalign(m_Outfile);

    // print the ancillary data for this query

    x_PrintOneQueryFooter(*results.GetAncillaryData());
    if (m_IsHTML) {
        m_Outfile << "<hr>" << endl;
    }
}

void 
CBlastFormat::x_ReverseQuery(blast::CIgBlastResults& results)
{
    // create a temporary seq_id
    CConstRef<CSeq_id> qid = results.GetSeqId();
    string new_id = qid->AsFastaString() + "_reversed";
    
    // create a bioseq
    CBioseq_Handle q_bh = m_Scope->GetBioseqHandle(*qid);
    int len = q_bh.GetBioseqLength();
    CSeq_loc loc(*(const_cast<CSeq_id *>(&*qid)), 0, len-1, eNa_strand_minus);
    CRef<CBioseq> q_new(new CBioseq(loc, new_id));
    CConstRef<CSeq_id> new_qid = m_Scope->AddBioseq(*q_new).GetSeqId();
    if (qid->IsLocal()) {
        string title = sequence::CDeflineGenerator().GenerateDefline(q_bh);
        if (title != "") {
            CRef<CSeqdesc> des(new CSeqdesc());
            des->SetTitle("reversed|" + title);
            m_Scope->GetBioseqEditHandle(*q_new).SetDescr().Set().push_back(des);
        }
    }

    // set up the mapping
    CSeq_loc new_loc(*(const_cast<CSeq_id *>(&*new_qid)), 0, len-1, eNa_strand_plus);
    CSeq_loc_Mapper mapper(loc, new_loc, &*m_Scope);

    // replace the alignment with the new query 
    CRef<CSeq_align_set> align_set(new CSeq_align_set());
    ITERATE(CSeq_align_set::Tdata, align, results.GetSeqAlign()->Get()) {
        CRef<CSeq_align> new_align = mapper.Map(**align, 0);
        align_set->Set().push_back(new_align);
    }
    results.SetSeqAlign().Reset(&*align_set);

    // reverse IgAnnotations
    CRef<CIgAnnotation> &annots = results.SetIgAnnotation();
    for (int i=0; i<6; i+=2) {
        int start = annots->m_GeneInfo[i];
        if (start >= 0) {
            annots->m_GeneInfo[i] = len - annots->m_GeneInfo[i+1];
            annots->m_GeneInfo[i+1] = len - start;
        }
    }

    for (int i=0; i<12; ++i) {
        int pos = annots->m_DomainInfo[i];
        if (pos >= 0) {
            annots->m_DomainInfo[i] = max(0, len - 1 - pos);
        }
    }

    for (int i=0; i<3; ++i) {
        int pos = annots->m_FrameInfo[i];
        if (pos >= 0) {
            annots->m_FrameInfo[i] = len -1 - pos;
        }
    }
}

void
CBlastFormat::PrintPhiResult(const blast::CSearchResultSet& result_set,
                        CConstRef<blast::CBlastQueryVector> queries,
                        unsigned int itr_num
                        /* = numeric_limits<unsigned int>::max() */,
                        blast::CPsiBlastIterationState::TSeqIds prev_seqids
                        /* = CPsiBlastIterationState::TSeqIds() */)
{
    // For remote searches, we don't retrieve the sequence data for the query
    // sequence when initially sending the request to the BLAST server (if it's
    // a GI/accession/TI), so we flush the scope so that it can be retrieved
    // (needed if a self-hit is found) again. This is not applicable if the
    // query sequence(s) are specified as FASTA (will be identified by local
    // IDs).
    if (m_IsRemoteSearch && !s_HasLocalIDs(queries)) {
        ResetScopeHistory();
    }

    if (m_FormatType == CFormattingArgs::eAsnText 
      || m_FormatType == CFormattingArgs::eAsnBinary 
      || m_FormatType == CFormattingArgs::eXml )
    {
        ITERATE(CSearchResultSet, result, result_set) {
           x_PrintStructuredReport(**result, queries);
        }
        return;
    }

    ITERATE(CSearchResultSet, result, result_set) {
        if ((**result).HasErrors()) {
            m_Outfile << "\n" << (**result).GetErrorStrings() << "\n";
            return; // errors are deemed fatal
        }
        if ((**result).HasWarnings()) {
            m_Outfile << "\n" << (**result).GetWarningStrings() << "\n";
        }
    }

    if (m_FormatType == CFormattingArgs::eTabular ||
        m_FormatType == CFormattingArgs::eTabularWithComments ||
        m_FormatType == CFormattingArgs::eCommaSeparatedValues) {
        ITERATE(CSearchResultSet, result, result_set) {
           x_PrintTabularReport(**result, itr_num);
        }
        return;
    }

    const CSearchResults& first_results = result_set[0];

    if (itr_num != numeric_limits<unsigned int>::max()) {
        m_Outfile << "Results from round " << itr_num << "\n";
    }

    CBioseq_Handle bhandle = m_Scope->GetBioseqHandle(*first_results.GetSeqId(),
                                                      CScope::eGetBioseq_All);
    CConstRef<CBioseq> bioseq = bhandle.GetBioseqCore();

    // print the preamble for this query

    m_Outfile << "\n\n";
    CBlastFormatUtil::AcknowledgeBlastQuery(*bioseq, kFormatLineLength,
                                            m_Outfile, m_BelieveQuery,
                                            m_IsHTML, false,
                                            first_results.GetRID());

    const SPHIQueryInfo *phi_query_info = first_results.GetPhiQueryInfo();

    if (phi_query_info)
    {
        vector<int> offsets;
        for (int index=0; index<phi_query_info->num_patterns; index++)
            offsets.push_back(phi_query_info->occurrences[index].offset);

        CBlastFormatUtil::PrintPhiInfo(phi_query_info->num_patterns,
                                   string(phi_query_info->pattern), 
                                   phi_query_info->probability,
                                   offsets, m_Outfile);
    }

    // quit early if there are no hits
    if ( !first_results.HasAlignments() ) {
        m_Outfile << "\n\n" 
              << "***** " << CBlastFormatUtil::kNoHitsFound << " *****" << "\n" 
              << "\n\n";
        x_PrintOneQueryFooter(*first_results.GetAncillaryData());
        return;
    }

    _ASSERT(first_results.HasAlignments());
    //-------------------------------------------------

    ITERATE(CSearchResultSet, result, result_set)
    {
        CConstRef<CSeq_align_set> aln_set = (**result).GetSeqAlign();
        x_DisplayDeflines(aln_set, itr_num, prev_seqids);
    }

    //-------------------------------------------------
    // print the alignments
    m_Outfile << "\n";


    int flags = s_SetFlags(m_Program, m_FormatType, m_IsHTML, m_ShowGi,
                           m_IsBl2Seq, false);

    if (phi_query_info)
    {
        SPHIPatternInfo *occurrences = phi_query_info->occurrences;
        int index;
        for (index=0; index<phi_query_info->num_patterns; index++)
        {
           list <CDisplaySeqalign::FeatureInfo*> phiblast_pattern;
           CSeq_id* id = new CSeq_id;
           id->Assign(*(result_set[index]).GetSeqId());
           CDisplaySeqalign::FeatureInfo*  feature_info = new CDisplaySeqalign::FeatureInfo;
           feature_info->seqloc = new CSeq_loc(*id, (TSeqPos) occurrences[index].offset,
                  (TSeqPos) (occurrences[index].offset + occurrences[index].length - 1));
           feature_info->feature_char = '*';
           feature_info->feature_id = "pattern";
           phiblast_pattern.push_back(feature_info);

           m_Outfile << "\nSignificant alignments for pattern occurrence " << index+1
                 << " at position " << 1+occurrences[index].offset << "\n\n";

           TMaskedQueryRegions masklocs;
           result_set[index].GetMaskedQueryRegions(masklocs);
           CConstRef<CSeq_align_set> aln_set = result_set[index].GetSeqAlign();
           CSeq_align_set copy_aln_set;
           CBlastFormatUtil::PruneSeqalign(*aln_set, copy_aln_set, m_NumAlignments);

           CDisplaySeqalign display(copy_aln_set, *m_Scope, &masklocs, &phiblast_pattern,
                             m_MatrixName);

           display.SetDbName(m_DbName);
           display.SetDbType(!m_DbIsAA);

           // set the alignment flags
           display.SetAlignOption(flags);

           if (m_Program == "blastn" || m_Program == "megablast") {
               display.SetMiddleLineStyle(CDisplaySeqalign::eBar);
               display.SetAlignType(CDisplaySeqalign::eNuc);
           }
           else {
               display.SetMiddleLineStyle(CDisplaySeqalign::eChar);
               display.SetAlignType(CDisplaySeqalign::eProt);
           }

           display.SetMasterGeneticCode(m_QueryGenCode);
           display.SetSlaveGeneticCode(m_DbGenCode);
           display.SetSeqLocChar(CDisplaySeqalign::eLowerCase);
           display.DisplaySeqalign(m_Outfile);
           m_Outfile << "\n";

           NON_CONST_ITERATE(list<CDisplaySeqalign::FeatureInfo*>, itr, phiblast_pattern) {
               delete *itr;
           }
        }
    }

    // print the ancillary data for this query

    x_PrintOneQueryFooter(*first_results.GetAncillaryData());
}

void 
CBlastFormat::PrintEpilog(const blast::CBlastOptions& options)
{
    if (m_FormatType == CFormattingArgs::eTabularWithComments) {
        CBlastTabularInfo tabinfo(m_Outfile, m_CustomOutputFormatSpec);
        if (m_IsBl2Seq) {
            _ASSERT(m_SeqInfoSrc);
            m_QueriesFormatted /= m_SeqInfoSrc->Size();
        }
        tabinfo.PrintNumProcessed(m_QueriesFormatted);
        return;
    } else if (m_FormatType >= CFormattingArgs::eTabular) 
        return;  // No footer for these.

    // Most of XML is printed as it's finished.
    // the epilog closes the report.
    if (m_FormatType == CFormattingArgs::eXml) {
        m_Outfile << m_BlastXMLIncremental->m_SerialXmlEnd << endl; 
        m_AccumulatedResults.clear();
        m_AccumulatedQueries->clear();
        return;
    }

    m_Outfile << "\n\n";
    if (m_Program == "deltablast" && !m_DomainDbInfo.empty()) {
        m_Outfile << "Conserved Domain";
        CBlastFormatUtil::PrintDbReport(m_DomainDbInfo, kFormatLineLength,
                                        m_Outfile, false);
    }

    if ( !m_IsBl2Seq ) {
        CBlastFormatUtil::PrintDbReport(m_DbInfo, kFormatLineLength, 
                                        m_Outfile, false);
    }

    if (m_Program == "blastn" || m_Program == "megablast") {
        m_Outfile << "\n\nMatrix: " << "blastn matrix " <<
                        options.GetMatchReward() << " " <<
                        options.GetMismatchPenalty() << "\n";
    }
    else {
        m_Outfile << "\n\nMatrix: " << options.GetMatrixName() << "\n";
    }

    if (options.GetGappedMode() == true) {
        double gap_extension = (double) options.GetGapExtensionCost();
        if ((m_Program == "megablast" || m_Program == "blastn") && options.GetGapExtensionCost() == 0)
        { // Formula from PMID 10890397 applies if both gap values are zero.
               gap_extension = -2*options.GetMismatchPenalty() + options.GetMatchReward();
               gap_extension /= 2.0;
        }
        m_Outfile << "Gap Penalties: Existence: "
                << options.GetGapOpeningCost() << ", Extension: "
                << gap_extension << "\n";
    }
    if (options.GetWordThreshold()) {
        m_Outfile << "Neighboring words threshold: " <<
                        options.GetWordThreshold() << "\n";
    }
    if (options.GetWindowSize()) {
        m_Outfile << "Window for multiple hits: " <<
                        options.GetWindowSize() << "\n";
    }

    if (m_IsHTML) {
        m_Outfile << kHTML_Suffix << "\n";
    }
}

void CBlastFormat::ResetScopeHistory()
{
    // Do not reset the scope for BLAST2Sequences or else we'll loose the
    // sequence data! (see x_CreateSubjectBioseq)
    if (m_IsBl2Seq) {
        return;
    }

    // Our current XML/ASN.1 libraries do not have provisions for
    // incremental object input/output, so with XML output format we
    // need to accumulate the whole document before writing any data.
    
    // This means that XML output requires more memory than other
    // output formats.
    
    if (m_FormatType != CFormattingArgs::eXml) {
        m_Scope->ResetHistory();
    }
}

