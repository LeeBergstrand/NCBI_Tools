/*  $Id: dust_filter.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
 * */

/// @file dust_filter.hpp
/// Calls sym dust lib in algo/dustmask and returns CSeq_locs for use by BLAST.

#ifndef ALGO_BLAST_API__DUST_FILTER_HPP
#define ALGO_BLAST_API__DUST_FILTER_HPP

#include <objects/seqloc/Seq_loc.hpp>
#include <algo/blast/api/sseqloc.hpp>
#include <algo/blast/api/blast_nucl_options.hpp>
#include <algo/dustmask/symdust.hpp>

/** @addtogroup AlgoBlast
 *
 * @{
 */

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(blast)

/** Finds dust locations for a given set of sequences by calling the the symmetric dust lib. 
 * The locations are saved in the respective fields of the SSeqLoc structures. If previous masks
 * exist, they are combined with the new masks.
 *
 * @param query Vector of sequence locations. [in] [out]
 * @param nucl_handle options handle for blast search [in]
 */
NCBI_XBLAST_EXPORT
void 
Blast_FindDustFilterLoc(TSeqLocVector& query, 
                        const CBlastNucleotideOptionsHandle* nucl_handle);

/** Overloaded version of the function above which takes the filtering
 * implementation's arguments directly, TSeqLocVector version.
 * @param query Vector of sequence locations. [in] [out]
 * @param level Dust filtering level argument [in]
 * @param window Dust filtering window argument [in]
 * @param linker Dust filtering linker argument [in]
 */
NCBI_XBLAST_EXPORT
void 
Blast_FindDustFilterLoc(TSeqLocVector& query, 
                        Uint4 level = CSymDustMasker::DEFAULT_LEVEL,
                        Uint4 window = CSymDustMasker::DEFAULT_WINDOW,
                        Uint4 linker = CSymDustMasker::DEFAULT_LINKER);

/** Overloaded version of the function above which takes the filtering
 * implementation's arguments directly, CBlastQueryVector version.
 * @param query Vector of sequence locations. [in] [out]
 * @param level Dust filtering level argument [in]
 * @param window Dust filtering window argument [in]
 * @param linker Dust filtering linker argument [in]
 */
NCBI_XBLAST_EXPORT
void 
Blast_FindDustFilterLoc(CBlastQueryVector & query, 
                        Uint4 level = CSymDustMasker::DEFAULT_LEVEL,
                        Uint4 window = CSymDustMasker::DEFAULT_WINDOW,
                        Uint4 linker = CSymDustMasker::DEFAULT_LINKER);

END_SCOPE(blast)
END_NCBI_SCOPE 

/* @} */

#endif /* ALGO_BLAST_API__DUST_FILTER_HPP */
