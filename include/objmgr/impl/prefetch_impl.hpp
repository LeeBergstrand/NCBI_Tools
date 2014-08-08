#ifndef PREFETCH_IMPL__HPP
#define PREFETCH_IMPL__HPP

/*  $Id: prefetch_impl.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author: Aleksey Grichenko, Eugene Vasilchenko
*
* File Description:
*   Prefetch implementation
*
*/

#include <corelib/ncbistd.hpp>
#include <corelib/ncbiobj.hpp>
#include <corelib/ncbithr.hpp>
#include <objmgr/data_loader.hpp>
#include <util/thread_pool.hpp>
#include <vector>
#include <set>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CScope;
class CSeq_id;
class CSeq_id_Handle;
class CBioseq_Handle;
class CPrefetchThreadOld;
class CDataSource;
class CTSE_Lock;

class NCBI_XOBJMGR_EXPORT CPrefetchTokenOld_Impl : public CObject
{
public:
    typedef vector<CSeq_id_Handle> TIds;

    ~CPrefetchTokenOld_Impl(void);

    DECLARE_OPERATOR_BOOL(IsValid());

private:
    friend class CPrefetchTokenOld;
    friend class CPrefetchThreadOld;

    typedef CTSE_Lock TTSE_Lock;
    typedef vector<TTSE_Lock>   TFetchedTSEs;
    typedef map<TTSE_Lock, int> TTSE_Map;

    CPrefetchTokenOld_Impl(const TIds& ids, unsigned int depth);

    void x_InitPrefetch(CScope& scope);
    void x_SetNon_locking(void);

    // Hide copy methods
    CPrefetchTokenOld_Impl(const CPrefetchTokenOld_Impl&);
    CPrefetchTokenOld_Impl& operator=(const CPrefetchTokenOld_Impl&);

    bool IsValid(void) const;
    
    CBioseq_Handle NextBioseqHandle(CScope& scope);

    void AddTokenReference(void);
    void RemoveTokenReference(void);

    // Called by fetching function when id is loaded
    void AddResolvedId(size_t id_idx, TTSE_Lock tse);

    // Checked by CPrefetchThreadOld before processing next id
    bool IsEmpty(void) const;

    int            m_TokenCount;    // Number of tokens referencing this impl
    TIds           m_Ids;           // requested ids in the original order
    size_t         m_CurrentId;     // next id to return
    TFetchedTSEs   m_TSEs;          // loaded TSEs
    TTSE_Map       m_TSEMap;        // Map TSE to number of related IDs
    int            m_PrefetchDepth; // Max. number of TSEs to prefetch
    CSemaphore     m_TSESemaphore;  // Signal to fetch next TSE
    bool           m_Non_locking;   // Do not lock TSEs (used for caching)
    mutable CFastMutex m_Lock;
};


class NCBI_XOBJMGR_EXPORT CPrefetchThreadOld : public CThread
{
public:
    CPrefetchThreadOld(CDataSource& data_source);

    // Add request to the fetcher queue
    void AddRequest(CPrefetchTokenOld_Impl& token);

    // Stop the thread (since Exit() can not be called from
    // data loader's destructor).
    void Terminate(void);

protected:
    virtual void* Main(void);
    // protected destructor as required by CThread
    ~CPrefetchThreadOld(void);

private:
    // Using list to be able to delete elements
    typedef CBlockingQueue<CRef<CPrefetchTokenOld_Impl> > TPrefetchQueue;

    CDataSource&   m_DataSource;
    TPrefetchQueue m_Queue;
    CFastMutex     m_Lock;
    bool           m_Stop;      // used to stop the thread
};


END_SCOPE(objects)
END_NCBI_SCOPE

#endif  // PREFETCH_IMPL__HPP
