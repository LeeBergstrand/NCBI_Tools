#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] =
    "$Id: objmgrfree_query_data.cpp 103491 2007-05-04 17:18:18Z kazimird $";
#endif /* SKIP_DOXYGEN_PROCESSING */

/* ===========================================================================
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
 * Author:  Christiam Camacho, Kevin Bealer
 *
 */

/** @file objmgrfree_query_data.cpp
 * NOTE: This file contains work in progress and the APIs are likely to change,
 * please do not rely on them until this notice is removed.
 */

#include <ncbi_pch.hpp>

// BLAST API includes
#include <algo/blast/api/blast_options.hpp>
#include <algo/blast/api/blast_exception.hpp>
#include <algo/blast/api/objmgrfree_query_data.hpp>

// Serial includes
#include <serial/iterator.hpp>

// Object includes
#include <objects/seqset/Seq_entry.hpp>

// Private BLAST API headers
#include "blast_setup.hpp"
#include "bioseq_extract_data_priv.hpp"

/** @addtogroup AlgoBlast
 *
 * @{
 */

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
BEGIN_SCOPE(blast)

/////////////////////////////////////////////////////////////////////////////
//
// CObjMgrFree_LocalQueryData
//
/////////////////////////////////////////////////////////////////////////////

class CObjMgrFree_LocalQueryData : public ILocalQueryData
{
public:
    CObjMgrFree_LocalQueryData(CConstRef<CBioseq_set> bioseq_set,
                               const CBlastOptions* options);
    
    virtual BLAST_SequenceBlk* GetSequenceBlk();
    virtual BlastQueryInfo* GetQueryInfo();
    
    /// Get the number of queries.
    virtual size_t GetNumQueries();
    
    /// Get the Seq_loc for the sequence indicated by index.
    virtual CConstRef<CSeq_loc> GetSeq_loc(size_t index);
    
    /// Get the length of the sequence indicated by index.
    virtual size_t GetSeqLength(size_t index);
    
private:
    const CBlastOptions* m_Options;
    CConstRef<CBioseq_set> m_BioseqSet;
    
    CRef<IBlastQuerySource> m_QuerySource;
};

CObjMgrFree_LocalQueryData::CObjMgrFree_LocalQueryData
    (CConstRef<CBioseq_set> bioseq_set, const CBlastOptions* options)
    : m_Options(options), m_BioseqSet(bioseq_set)
{
    bool is_prot = 
        Blast_QueryIsProtein(options->GetProgramType()) ? true : false;
    m_QuerySource.Reset(new CBlastQuerySourceBioseqSet(*bioseq_set, is_prot));
}

BLAST_SequenceBlk*
CObjMgrFree_LocalQueryData::GetSequenceBlk()
{
    if (m_SeqBlk.Get() == NULL) {
        if (m_BioseqSet.NotEmpty()) {
            m_SeqBlk.Reset(SafeSetupQueries(*m_QuerySource,
                                            m_Options,
                                            GetQueryInfo(),
                                            m_Messages));
        } else {
            NCBI_THROW(CBlastException, eInvalidArgument,
                       "Missing source data in " +
                       string(NCBI_CURRENT_FUNCTION));
        }
    }
    return m_SeqBlk;
}

BlastQueryInfo*
CObjMgrFree_LocalQueryData::GetQueryInfo()
{
    if (m_QueryInfo.Get() == NULL) {
        if (m_BioseqSet.NotEmpty()) {
            m_QueryInfo.Reset(SafeSetupQueryInfo(*m_QuerySource, m_Options));
        } else {
            NCBI_THROW(CBlastException, eInvalidArgument,
                       "Missing source data in " +
                       string(NCBI_CURRENT_FUNCTION));
        }
    }
    return m_QueryInfo;
}

size_t 
CObjMgrFree_LocalQueryData::GetNumQueries()
{
    size_t retval = m_QuerySource->Size();
    _ASSERT(retval == (size_t)GetQueryInfo()->num_queries);
    return retval;
}

CConstRef<CSeq_loc> 
CObjMgrFree_LocalQueryData::GetSeq_loc(size_t index)
{
    return m_QuerySource->GetSeqLoc(index);
}

size_t 
CObjMgrFree_LocalQueryData::GetSeqLength(size_t index)
{
    return m_QuerySource->GetLength(index);
}

/** 
 * @brief Convert a constant reference to a Bioseq-set to
 * IRemoteQueryData::TSeqLocs
 * 
 * @param bioseq_set input data[in]
 */
static IRemoteQueryData::TSeqLocs
s_ConstBioseqSetToSeqLocs(CConstRef<CBioseq_set> bioseq_set)
{
    CTypeConstIterator<CBioseq> itr(ConstBegin(*bioseq_set, eDetectLoops)); 
    CBlastQuerySourceBioseqSet query_source(*bioseq_set, itr->IsAa());

    IRemoteQueryData::TSeqLocs retval;
    for (TSeqPos i = 0; i < query_source.Size(); i++) {
        CRef<CSeq_loc> sl(const_cast<CSeq_loc*>(&*query_source.GetSeqLoc(i)));
        retval.push_back(sl);
    }
    return retval;
}
/////////////////////////////////////////////////////////////////////////////
//
// CObjMgrFree_RemoteQueryData
//
/////////////////////////////////////////////////////////////////////////////

class CObjMgrFree_RemoteQueryData : public IRemoteQueryData
{
public:
    /// Construct query data from a CBioseq_set..
    /// @param queries Queries expressed as a CBioseq_set.
    CObjMgrFree_RemoteQueryData(CConstRef<CBioseq_set> bioseq_set);

    /// Accessor for the CBioseq_set.
    virtual CRef<CBioseq_set> GetBioseqSet();

    /// Accessor for the TSeqLocs.
    virtual TSeqLocs GetSeqLocs();

private:
    /// Queries, expressed as a CBioseq_set.
    CConstRef<CBioseq_set> m_ClientBioseqSet;
};

CObjMgrFree_RemoteQueryData::CObjMgrFree_RemoteQueryData
    (CConstRef<CBioseq_set> bioseq_set)
    : m_ClientBioseqSet(bioseq_set)
{}

CRef<CBioseq_set>
CObjMgrFree_RemoteQueryData::GetBioseqSet()
{
    if (m_Bioseqs.Empty()) {
        if (m_ClientBioseqSet.NotEmpty()) {
            m_Bioseqs.Reset(const_cast<CBioseq_set*>(&*m_ClientBioseqSet));
        } else {
            NCBI_THROW(CBlastException, eInvalidArgument,
                       "Missing source data in " +
                       string(NCBI_CURRENT_FUNCTION));
        }
    }
    return m_Bioseqs;
}

IRemoteQueryData::TSeqLocs
CObjMgrFree_RemoteQueryData::GetSeqLocs()
{
    if (m_SeqLocs.empty()) {
        if (m_ClientBioseqSet.NotEmpty()) {
            m_SeqLocs = s_ConstBioseqSetToSeqLocs(m_ClientBioseqSet);
        } else {
            NCBI_THROW(CBlastException, eInvalidArgument,
                       "Missing source data in " +
                       string(NCBI_CURRENT_FUNCTION));
        }
    }
    return m_SeqLocs;
}

/////////////////////////////////////////////////////////////////////////////
//
// CObjMgrFree_QueryFactory
//
/////////////////////////////////////////////////////////////////////////////

/// Auxiliary function to convert a CBioseq into a CRef<CBioseq_set>
CRef<CBioseq_set>
x_BioseqSetFromBioseq(const CBioseq& bioseq)
{
    CRef<CSeq_entry> seq_entry(new CSeq_entry);
    seq_entry->SetSeq(const_cast<CBioseq&>(bioseq));
    CRef<CBioseq_set> retval(new CBioseq_set);
    retval->SetSeq_set().push_back(seq_entry);
    return retval;
}

CObjMgrFree_QueryFactory::CObjMgrFree_QueryFactory
    (CConstRef<objects::CBioseq> b)
    : m_Bioseqs(x_BioseqSetFromBioseq(*b))
{}

CObjMgrFree_QueryFactory::CObjMgrFree_QueryFactory
    (CConstRef<objects::CBioseq_set> b)
    : m_Bioseqs(b)
{}

CRef<ILocalQueryData>
CObjMgrFree_QueryFactory::x_MakeLocalQueryData(const CBlastOptions* opts)
{
    CRef<ILocalQueryData> retval;
    
    if (m_Bioseqs.NotEmpty()) {
        retval.Reset(new CObjMgrFree_LocalQueryData(m_Bioseqs, opts));
    } else {
        NCBI_THROW(CBlastException, eInvalidArgument,
                   "Missing source data in " +
                   string(NCBI_CURRENT_FUNCTION));
    }
    
    return retval;
}

CRef<IRemoteQueryData>
CObjMgrFree_QueryFactory::x_MakeRemoteQueryData()
{
    CRef<IRemoteQueryData> retval;

    if (m_Bioseqs.NotEmpty()) {
        retval.Reset(new CObjMgrFree_RemoteQueryData(m_Bioseqs));
    } else {
        NCBI_THROW(CBlastException, eInvalidArgument,
                   "Missing source data in " +
                   string(NCBI_CURRENT_FUNCTION));
    }

    return retval;
}

END_SCOPE(blast)
END_NCBI_SCOPE

/* @} */
