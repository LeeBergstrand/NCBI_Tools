/*  $Id: dust_filter.cpp 161402 2009-05-27 17:35:47Z camacho $
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
 * Author: Tom Madden
 *
 * Initial Version Creation Date:  June 20, 2005
 *
 *
 * */

/// @file dust_filter.cpp
/// Calls sym dust lib in algo/dustmask and returns CSeq_locs for use by BLAST.

#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] = "$Id: dust_filter.cpp 161402 2009-05-27 17:35:47Z camacho $";
#endif /* SKIP_DOXYGEN_PROCESSING */

#include <ncbi_pch.hpp>
#include "dust_filter.hpp"
#include <serial/iterator.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objmgr/util/sequence.hpp>
#include <objmgr/seq_loc_mapper.hpp>
#include <algo/blast/api/blast_types.hpp>
#include <algo/blast/api/blast_nucl_options.hpp>

#include <objmgr/seq_vector.hpp>

#include <algo/dustmask/symdust.hpp>

#include <string.h>

/** @addtogroup AlgoBlast
 *
 * @{
 */

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
BEGIN_SCOPE(blast)

void
Blast_FindDustFilterLoc(TSeqLocVector& queries, 
                        const CBlastNucleotideOptionsHandle* nucl_handle)
{
    // Either non-blastn search or dust filtering not desired.
    if (nucl_handle == NULL || nucl_handle->GetDustFiltering() == false)
       return;

    Blast_FindDustFilterLoc(queries, nucl_handle->GetDustFilteringLevel(),
                          nucl_handle->GetDustFilteringWindow(),
                          nucl_handle->GetDustFilteringLinker());
}

/// Auxiliary function to create CSeq_loc_Mapper from a copy of the target
/// Seq-loc.
static CRef<CSeq_loc_Mapper>
s_CreateSeqLocMapper(CSeq_id& query_id, 
                     const CSeq_loc* target_seqloc, 
                     CScope* scope)
{
    _ASSERT(target_seqloc);
    _ASSERT(scope);

    // Create a Seq-loc for the entire query sequence
    CRef<CSeq_loc> entire_slp(new CSeq_loc);
    entire_slp->SetWhole().Assign(query_id);

    return CRef<CSeq_loc_Mapper>
        (new CSeq_loc_Mapper(*entire_slp, 
                             const_cast<CSeq_loc&>(*target_seqloc), 
                             scope));
}

void s_CombineDustMasksWithUserProvidedMasks(CSeqVector& data,
           CConstRef<CSeq_loc> seqloc,
           CRef<CScope> scope,
           CRef<CSeq_id> query_id, 
           CRef<CSeq_loc>& orig_query_mask,
           Uint4 level, Uint4 window, Uint4 linker)
{
    CSymDustMasker duster(level, window, linker);

    CRef<CPacked_seqint> masked_locations =
        duster.GetMaskedInts(*query_id, data);
    CPacked_seqint::Tdata locs = masked_locations->Get();
    if (locs.empty()) {
        return;
    }

    CRef<CSeq_loc> query_masks(new CSeq_loc);
    ITERATE(CPacked_seqint::Tdata, masked_loc, locs) {
        CRef<CSeq_loc> seq_interval(new CSeq_loc(*query_id, 
                                                 (*masked_loc)->GetFrom(), 
                                                 (*masked_loc)->GetTo()));
        query_masks->Add(*seq_interval);
    }

    CRef<CSeq_loc_Mapper> mapper = s_CreateSeqLocMapper(*query_id, seqloc,
                                                        scope);
    query_masks.Reset(mapper->Map(*query_masks));

    const int kTopFlags = CSeq_loc::fStrand_Ignore|CSeq_loc::fMerge_All|CSeq_loc::fSort;
    if (orig_query_mask.NotEmpty() && !orig_query_mask->IsNull()) {
        CRef<CSeq_loc> tmp = orig_query_mask->Add(*query_masks,  kTopFlags, 0);
        orig_query_mask.Reset(tmp);
    } else {
        query_masks->Merge(kTopFlags, 0);
        orig_query_mask.Reset(query_masks);
    }

    if (orig_query_mask->IsNull() || orig_query_mask->IsEmpty()) {
        orig_query_mask.Reset();
        return;
    }

    // in the event this happens, change to Seq-interval so that
    // CSeq_loc::ChangeToPackedInt can process it
    if (orig_query_mask->IsWhole()) {
        orig_query_mask.Reset
            (new CSeq_loc(*query_id, 0, 
                          sequence::GetLength(*query_id, scope) -1));
    }
    orig_query_mask->ChangeToPackedInt();
    _ASSERT(orig_query_mask->IsPacked_int());
}

void
Blast_FindDustFilterLoc(TSeqLocVector& queries, 
                        Uint4 level, Uint4 window, Uint4 linker)
{

    NON_CONST_ITERATE(TSeqLocVector, query, queries)
    {
        CSeqVector data(*query->seqloc, *query->scope, 
                        CBioseq_Handle::eCoding_Iupac);
        CRef<CSeq_id> query_id
            (const_cast<CSeq_id*>(query->seqloc->GetId()));
        s_CombineDustMasksWithUserProvidedMasks(data, query->seqloc,
                                                query->scope, query_id,
                                                query->mask, level, window,
                                                linker);
    }

}

void
Blast_FindDustFilterLoc(CBlastQueryVector& queries, 
                        Uint4 level, Uint4 window, Uint4 linker)
{
    for(size_t i = 0; i < queries.Size(); i++) 
    {
        CSeqVector data(*queries.GetQuerySeqLoc(i), *queries.GetScope(i),
                        CBioseq_Handle::eCoding_Iupac);

        CRef<CSeq_id> query_id
            (const_cast<CSeq_id*>(queries.GetQuerySeqLoc(i)->GetId()));
        CRef<CSeq_loc> masks = queries.GetMasks(i);
        s_CombineDustMasksWithUserProvidedMasks(data,
                                                queries.GetQuerySeqLoc(i),
                                                queries.GetScope(i), query_id,
                                                masks, level, window, linker);
        if (masks.NotEmpty()) {
            TMaskedQueryRegions mqr = 
                PackedSeqLocToMaskedQueryRegions(masks, eBlastTypeBlastn);
            queries.SetMaskedRegions(i, mqr);
        }
    }
}

END_SCOPE(blast)
END_NCBI_SCOPE

/* @} */
