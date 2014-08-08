/*  $Id: winmask_filter.cpp 356370 2012-03-13 19:29:11Z camacho $
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
 * Author: Kevin Bealer
 *
 * Initial Version Creation Date:  April 17th, 2008
 *
 * File Description:
 *     Blast wrappers for WindowMasker filtering.
 *
 * */

/// @file winmask_filter.cpp
/// Blast wrappers for WindowMasker filtering.

#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] = "$Id: winmask_filter.cpp 356370 2012-03-13 19:29:11Z camacho $";
#endif /* SKIP_DOXYGEN_PROCESSING */

#include <ncbi_pch.hpp>
#include "winmask_filter.hpp"
#include <sstream>
#include <serial/iterator.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqalign/Seq_align.hpp>
#include <objects/seqalign/Seq_align_set.hpp>
#include <objects/seqalign/Dense_seg.hpp>
#include <objmgr/util/sequence.hpp>
#include <algo/blast/api/blast_types.hpp>

#include <algo/blast/api/seqsrc_seqdb.hpp>

#include <algo/blast/api/local_blast.hpp>
#include <algo/blast/api/objmgr_query_data.hpp>
#include <algo/blast/api/blast_nucl_options.hpp>
#include <algo/blast/api/windowmask_filter.hpp>
#include "blast_setup.hpp"

#include <algo/blast/core/blast_seqsrc.h>
#include <algo/blast/core/blast_hits.h>
#include <algo/blast/core/blast_filter.h>

#include <algo/blast/api/blast_aux.hpp>
#include <algo/winmask/seq_masker.hpp>
#include <corelib/env_reg.hpp>

/** @addtogroup AlgoBlast
 *
 * @{
 */

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
BEGIN_SCOPE(blast)

CSeqMasker* s_BuildSeqMasker(const string & lstat)
{
    Uint1 arg_window_size            = 0; // [allow setting of this field?]
    Uint4 arg_window_step            = 1;
    Uint1 arg_unit_step              = 1;
    Uint4 arg_textend                = 0; // [allow setting of this field?]
    Uint4 arg_cutoff_score           = 0; // [allow setting of this field?]
    Uint4 arg_max_score              = 0; // [allow setting of this field?]
    Uint4 arg_min_score              = 0; // [allow setting of this field?]
    Uint4 arg_set_max_score          = 0; // [allow setting of this field?]
    Uint4 arg_set_min_score          = 0; // [allow setting of this field?]
    bool  arg_merge_pass             = false;
    Uint4 arg_merge_cutoff_score     = 0;
    Uint4 arg_abs_merge_cutoff_dist  = 0;
    Uint4 arg_mean_merge_cutoff_dist = 0;
    Uint1 arg_merge_unit_step        = 0;
    const string & arg_trigger       = "mean";
    Uint1 tmin_count                 = 0;
    bool  arg_discontig              = false;
    Uint4 arg_pattern                = 0;
    
    // enable/disable some kind of optimization
    bool  arg_use_ba                 = true;
    
    // Get a sequence masker.
    
    CSeqMasker* masker = NULL;
    
    try {
        masker = new CSeqMasker( lstat,
                                 arg_window_size,
                                 arg_window_step,
                                 arg_unit_step,
                                 arg_textend,
                                 arg_cutoff_score,
                                 arg_max_score,
                                 arg_min_score,
                                 arg_set_max_score,
                                 arg_set_min_score,
                                 arg_merge_pass,
                                 arg_merge_cutoff_score,
                                 arg_abs_merge_cutoff_dist,
                                 arg_mean_merge_cutoff_dist,
                                 arg_merge_unit_step,
                                 arg_trigger,
                                 tmin_count,
                                 arg_discontig,
                                 arg_pattern,
                                 arg_use_ba );
    }
    catch(CException & e) {
        NCBI_THROW(CBlastException, eSetup, e.what());
    }
    
    return masker;
}

void s_BuildMaskedRanges(CSeqMasker::TMaskList & masks,
                         const CSeq_loc        & seqloc,
                         CSeq_id               & query_id,
                         TMaskedQueryRegions   * mqr,
                         CRef<CSeq_loc>        * psl)
{
    TSeqPos query_start = seqloc.GetStart(eExtreme_Positional);
    
    // This needs to be examined further for places where a +1, -1,
    // etc is needed due to biological vs. computer science offset
    // notations.
    
    ITERATE(CSeqMasker::TMaskList, pr, masks) {
        CRef<CSeq_interval> ival(new CSeq_interval);
        
        TSeqPos
            start  = pr->first,
            end    = pr->second;
        
        ival->SetFrom (query_start + start);
        ival->SetTo   (query_start + end);
        ival->SetId   (query_id);
        ival->SetStrand(eNa_strand_both);
        
        if (mqr) {
            CRef<CSeqLocInfo> info_plus
                (new CSeqLocInfo(&* ival, CSeqLocInfo::eFramePlus1));
            mqr->push_back(info_plus);

            CRef<CSeqLocInfo> info_minus
                (new CSeqLocInfo(&* ival, CSeqLocInfo::eFrameMinus1));
            mqr->push_back(info_minus);
        }
        
        if (psl) {
            if (psl->Empty()) {
                psl->Reset(new CSeq_loc);
            }
            (**psl).SetPacked_int().Set().push_back(ival);
        }
    }
    if (psl && !psl->Empty())
    {
        const int kTopFlags = CSeq_loc::fStrand_Ignore|CSeq_loc::fMerge_All|CSeq_loc::fSort;
        CRef<CSeq_loc> tmp = (*psl)->Merge(kTopFlags, 0);
        psl->Reset(tmp);
        (*psl)->ChangeToPackedInt();
    }

}

// These templates only exist to reduce code duplication due to the
// TSeqLocVector / BlastQueryVector split.  By parameterizing on the
// query container type, several functions can call these templates
// with different types of queries and options handles, and the
// appropriate number of "glue" functions will be generated to call
// the actual taxid / filename based implementations.

template<class TQueries>
void
Blast_FindWindowMaskerLoc_Fwd(TQueries            & query,
                              const CBlastOptions * opts)
{
    if (! opts)
        return;
    
    if (opts->GetWindowMaskerDatabase()) {
        Blast_FindWindowMaskerLoc(query, opts->GetWindowMaskerDatabase());
    } else if (opts->GetWindowMaskerTaxId()) {
        Blast_FindWindowMaskerLocTaxId(query, opts->GetWindowMaskerTaxId());
    }
}

template<class TQueries>
void
Blast_FindWindowMaskerLoc_Fwd(TQueries                  & query,
                              const CBlastOptionsHandle * opts_handle)
{
    if (! opts_handle)
        return;
    
    Blast_FindWindowMaskerLoc_Fwd(query, & opts_handle->GetOptions());
}

// These four functions exist to provide non-template public
// interfaces; the work is done in the two templates above this to
// reduce duplication.

void
Blast_FindWindowMaskerLoc(CBlastQueryVector   & query,
                          const CBlastOptions * opts)
{
    Blast_FindWindowMaskerLoc_Fwd(query, opts);
}

void
Blast_FindWindowMaskerLoc(TSeqLocVector       & query,
                          const CBlastOptions * opts)
{
    Blast_FindWindowMaskerLoc_Fwd(query, opts);
}

void
Blast_FindWindowMaskerLoc(CBlastQueryVector         & query,
                          const CBlastOptionsHandle * opts)
{
    Blast_FindWindowMaskerLoc_Fwd(query, opts);
}

void
Blast_FindWindowMaskerLoc(TSeqLocVector             & query,
                          const CBlastOptionsHandle * opts)
{
    Blast_FindWindowMaskerLoc_Fwd(query, opts);
}

// These two functions do the actual work.  If either is changed, the
// other should be too.  The TSeqLocVector vs. BlastQueryVector
// differences could be factored out into a wrapper that isolates the
// differences so that the algorithm is not duplicated.  Another
// alternative is to (continue to) replace TSeqLocVector with
// CBlastQueryVector as was originally planned.

void
Blast_FindWindowMaskerLoc(CBlastQueryVector & queries, const string & lstat)
{
    AutoPtr<CSeqMasker> masker(s_BuildSeqMasker(lstat));
    
    for(size_t j = 0; j < queries.Size(); j++) {
        CBlastSearchQuery & query = *queries.GetBlastSearchQuery(j);
        
        // Get SeqVector, query Seq-id, and range.
        
        CConstRef<CSeq_loc> seqloc = query.GetQuerySeqLoc();
        
        CSeqVector psv(*seqloc,
                       *queries.GetScope(j),
                       CBioseq_Handle::eCoding_Iupac,
                       eNa_strand_plus);
        
        CRef<CSeq_id> query_seq_id(new CSeq_id);
        query_seq_id->Assign(*seqloc->GetId());
        
        // Mask the query.
        
        AutoPtr<CSeqMasker::TMaskList> pos_masks((*masker)(psv));
        
        TMaskedQueryRegions mqr;
        
        s_BuildMaskedRanges(*pos_masks,
                            *seqloc,
                            *query_seq_id,
                            & mqr,
                            0);
        
        query.SetMaskedRegions(mqr);
    }
}

void
Blast_FindWindowMaskerLoc(TSeqLocVector & queries, const string & lstat)
{
    AutoPtr<CSeqMasker> masker(s_BuildSeqMasker(lstat));
    
    for(size_t j = 0; j < queries.size(); j++) {
        // Get SeqVector, query Seq-id, and range.
        
        CConstRef<CSeq_loc> seqloc = queries[j].seqloc;
        
        CSeqVector psv(*seqloc,
                       *queries[j].scope,
                       CBioseq_Handle::eCoding_Iupac,
                       eNa_strand_plus);
        
        CRef<CSeq_id> query_seq_id(new CSeq_id);
        query_seq_id->Assign(*seqloc->GetId());
        
        // Mask the query.
        
        AutoPtr<CSeqMasker::TMaskList> pos_masks((*masker)(psv));

        s_BuildMaskedRanges(*pos_masks,
                            *seqloc,
                            *query_seq_id,
                            0,
                            & queries[j].mask);
       
	if( queries[0].mask ) {
        CPacked_seqint::Tdata & seqint_list =
            queries[0].mask->SetPacked_int().Set();
        
        NON_CONST_ITERATE(CPacked_seqint::Tdata, itr, seqint_list) {
            if ((*itr)->CanGetStrand()) {
                switch((*itr)->GetStrand()) {
                case eNa_strand_unknown:
                case eNa_strand_both:
                case eNa_strand_plus:
                    (*itr)->ResetStrand();
                    break;
                    
                default:
                    break;
                }
            }
        }
	}
    }
}

/// Find the path to the window masker files, first checking the environment
/// variable WINDOW_MASKER_PATH, then the section WINDOW_MASKER, label
/// WINDOW_MASKER_PATH in the NCBI configuration file. If not found in either
/// location, return the current working directory
/// @sa s_FindPathToGeneInfoFiles
static string
s_FindPathToWM(void)
{
    string retval = kEmptyStr;
    const string kEnvVar("WINDOW_MASKER_PATH");
    const string kSection("WINDOW_MASKER");
    CNcbiIstrstream empty_stream(kEmptyCStr);
    CRef<CNcbiRegistry> reg(new CNcbiRegistry(empty_stream,
                                              IRegistry::fWithNcbirc));
    CRef<CSimpleEnvRegMapper> mapper(new CSimpleEnvRegMapper(kSection,
                                                             kEmptyStr));
    CRef<CEnvironmentRegistry> env_reg(new CEnvironmentRegistry);
    env_reg->AddMapper(*mapper, CEnvironmentRegistry::ePriority_Max);
    reg->Add(*env_reg, CNcbiRegistry::ePriority_MaxUser);
    retval = reg->Get(kSection, kEnvVar);
    if (retval == kEmptyStr) {
        retval = CDir::GetCwd();
    }
#if defined(NCBI_OS_MSWIN)
    // We address this here otherwise CDirEntry::IsAbsolutePath() fails
    if (NStr::StartsWith(retval, "//")) {
        NStr::ReplaceInPlace(retval, "//", "\\\\");
    }
#endif
    return retval;
}

string WindowMaskerTaxidToDb(const string& window_masker_path, int taxid)
{
    string path = window_masker_path;
    path += CFile::GetPathSeparator() + NStr::IntToString(taxid)
        + CFile::GetPathSeparator();
    
    const string binpath = path + "wmasker.obinary";
    const string ascpath = path + "wmasker.oascii";
    
    string retval;
    // Try the binary file first, as this is faster to process than the ASCII
    // file
    if (CFile(binpath).Exists()) {
        retval = binpath;
    } else if (CFile(ascpath).Exists()) {
        retval = ascpath;
    }
    return retval;
}

/* Unit test is in bl2seq_unit_test.cpp */
string WindowMaskerTaxidToDb(int taxid)
{
    string path = s_FindPathToWM();
    return WindowMaskerTaxidToDb(path, taxid);
}

void
Blast_FindWindowMaskerLocTaxId(CBlastQueryVector & queries, int taxid)
{
    string db = WindowMaskerTaxidToDb(taxid);
    Blast_FindWindowMaskerLoc(queries, db);
}

void
Blast_FindWindowMaskerLocTaxId(TSeqLocVector & queries, int taxid)
{
    string db = WindowMaskerTaxidToDb(taxid);
    Blast_FindWindowMaskerLoc(queries, db);
}

static void s_OldGetTaxIdWithWindowMaskerSupport(set<int>& supported_taxids)
{
    supported_taxids.clear();
    CNcbiOstrstream oss;
    const string wmpath = s_FindPathToWM();
    oss << wmpath << CFile::GetPathSeparator() << "*"
        << CFile::GetPathSeparator() << "*.*"
        << CFile::GetPathSeparator() << "wmasker.o*";
    const string path = CNcbiOstrstreamToString(oss);
    
    list<string> builds;
    FindFiles(path, builds, fFF_File);
    NON_CONST_ITERATE(list<string>, path, builds) {
        // remove the WindowMasker path and path separator
        path->erase(0, wmpath.size() + 1);  
        // then remove the remaining path
        const size_t pos = path->find(CFile::GetPathSeparator());
        path->erase(pos);
        const int taxid = NStr::StringToInt(*path, NStr::fConvErr_NoThrow);
        supported_taxids.insert(taxid);
    }
}

void GetTaxIdWithWindowMaskerSupport(set<int>& supported_taxids)
{
    supported_taxids.clear();
    CNcbiOstrstream oss;
    const string wmpath = s_FindPathToWM();
    oss << wmpath << CFile::GetPathSeparator() << "*"
        << CFile::GetPathSeparator() << "wmasker.o*";
    const string path = CNcbiOstrstreamToString(oss);
    
    list<string> builds;
    FindFiles(path, builds, fFF_File);
    NON_CONST_ITERATE(list<string>, path, builds) {
        // remove the WindowMasker path and path separator
        path->erase(0, wmpath.size() + 1);  
        // then remove the remaining path
        const size_t pos = path->find(CFile::GetPathSeparator());
        path->erase(pos);
        const int taxid = NStr::StringToInt(*path, NStr::fConvErr_NoThrow);
        supported_taxids.insert(taxid);
    }

    if (supported_taxids.empty()) {
        s_OldGetTaxIdWithWindowMaskerSupport(supported_taxids);
    }
}

END_SCOPE(blast)
END_NCBI_SCOPE

/* @} */
