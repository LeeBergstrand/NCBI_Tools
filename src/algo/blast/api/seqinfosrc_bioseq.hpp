#ifndef ALGO_BLAST_API__SEQINFOSRC_BIOSEQ__HPP
#define ALGO_BLAST_API__SEQINFOSRC_BIOSEQ__HPP

/*  $Id: seqinfosrc_bioseq.hpp 170794 2009-09-16 18:53:03Z maning $
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

/** @file seqinfosrc_bioseq.hpp
 * Defines a concrete strategy for the IBlastSeqInfoSrc interface for
 * sequence identifiers retrieval from a CBioseq/CBioseq_set object
 */

#include <algo/blast/api/blast_seqinfosrc.hpp>
#include "bioseq_extract_data_priv.hpp"

/** @addtogroup AlgoBlast
 *
 * @{
 */

BEGIN_NCBI_SCOPE

BEGIN_SCOPE(objects)
    class CBioseq;
    class CBioseq_set;
END_SCOPE(objects)

BEGIN_SCOPE(blast)

/// Implementation of the IBlastSeqInfoSrc interface to encapsulate retrieval
/// of sequence identifiers and lengths from a CBioseq/CBioseq_set object
class CBioseqSeqInfoSrc : public IBlastSeqInfoSrc
{
public:
    /// Parametrized constructor 
    /// @param bs CBioseq object from which to obtain the data [in]
    /// @param is_prot true if sequence in bs argument is protein, else 
    /// false [in]
    CBioseqSeqInfoSrc(const objects::CBioseq& bs, bool is_prot);

    /// Parametrized constructor 
    /// @param bss CBioseq_set object from which to obtain the data [in]
    /// @param is_prot true if sequences in bss argument are all proteins, else
    /// false [in]
    CBioseqSeqInfoSrc(const objects::CBioseq_set& bss, bool is_prot);

    /// Retrieve a sequence identifier given its index in the vector.
    /// @param index the ordinal number to retrieve [in]
    virtual list< CRef<objects::CSeq_id> > GetId(Uint4 index) const;

    /// Retrieve a sequence identifier given its index in the vector.
    /// @param index the ordinal number to retrieve [in]
    virtual CConstRef<objects::CSeq_loc> GetSeqLoc(Uint4 index) const;

    /// Retrieve sequence length given its index in the vector.
    /// @param index the ordinal number to retrieve [in]
    virtual Uint4 GetLength(Uint4 index) const;

    /// Returns the size of the underlying container of sequences
    virtual size_t Size() const;
    
    /// Is the subject restricted by a GI list?  (Always returns false).
    virtual bool HasGiList() const;
    
    /// Retrieves the subject masks for the corresponding index
    /// @param index the ordinal number to retrieve [in]
    /// @param target_range range for which to return masks for. Empty ranges
    /// indicate that no masks should be retrieved, whole ranges mean that
    /// masks for the whole sequence should be retrieved [in]
    /// @param retval the masks will be returned through this parameter [out]
    /// @return true if there were masks returned in retval, otherwise false.
    virtual bool GetMasks(Uint4 index, 
                          const TSeqRange& target_range,
                          TMaskedSubjRegions& retval) const;

    /// Retrieves the subject masks for the corresponding index
    /// @param index the ordinal number to retrieve [in]
    /// @param target_ranges range for which to return masks for. Empty ranges
    /// indicate that no masks should be retrieved, whole ranges mean that
    /// masks for the whole sequence should be retrieved [in]
    /// @param retval the masks will be returned through this parameter [out]
    /// @return true if there were masks returned in retval, otherwise false.
    virtual bool GetMasks(Uint4 index, 
                          const vector<TSeqRange>& target_ranges,
                          TMaskedSubjRegions& retval) const;
private:
    CBlastQuerySourceBioseqSet m_DataSource;
};

END_SCOPE(blast)
END_NCBI_SCOPE

/* @} */

#endif  /* ALGO_BLAST_API__SEQINFOSRC_BIOSEQ_HPP */
