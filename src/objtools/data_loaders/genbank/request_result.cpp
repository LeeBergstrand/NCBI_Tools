/*  $Id: request_result.cpp 390833 2013-03-02 19:17:52Z dicuccio $
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
*  Author: Eugene Vasilchenko
*
*  File Description: GenBank Data loader
*
*/

#include <ncbi_pch.hpp>
#include <objtools/data_loaders/genbank/request_result.hpp>
#include <objtools/data_loaders/genbank/processors.hpp>
#include <objmgr/objmgr_exception.hpp>
#include <objmgr/impl/tse_info.hpp>
#include <objmgr/annot_selector.hpp>
#include <corelib/ncbithr.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


static inline TThreadSystemID GetThreadId(void)
{
    TThreadSystemID thread_id = 0;
    CThread::GetSystemID(&thread_id);
    return thread_id;
}


/////////////////////////////////////////////////////////////////////////////
// CResolveInfo
/////////////////////////////////////////////////////////////////////////////

CLoadInfo::CLoadInfo(void)
{
}


CLoadInfo::~CLoadInfo(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// CLoadInfoSeq_ids
/////////////////////////////////////////////////////////////////////////////

CLoadInfoSeq_ids::CLoadInfoSeq_ids(void)
    : m_GiLoaded(false),
      m_AccLoaded(false),
      m_LabelLoaded(false),
      m_TaxIdLoaded(false),
      m_State(0)
{
}


CLoadInfoSeq_ids::CLoadInfoSeq_ids(const CSeq_id_Handle& /*seq_id*/)
    : m_GiLoaded(false),
      m_AccLoaded(false),
      m_LabelLoaded(false),
      m_TaxIdLoaded(false),
      m_State(0)
{
}


CLoadInfoSeq_ids::CLoadInfoSeq_ids(const string& /*seq_id*/)
    : m_GiLoaded(false),
      m_AccLoaded(false),
      m_LabelLoaded(false),
      m_TaxIdLoaded(false),
      m_State(0)
{
}


CLoadInfoSeq_ids::~CLoadInfoSeq_ids(void)
{
}


bool CLoadInfoSeq_ids::IsLoadedGi(void)
{
    if ( m_GiLoaded ) {
        return true;
    }
    if ( !IsLoaded() ) {
        return false;
    }
    ITERATE ( CLoadInfoSeq_ids, it, *this ) {
        if ( it->Which() == CSeq_id::e_Gi ) {
            int gi;
            if ( it->IsGi() ) {
                gi = it->GetGi();
            }
            else {
                gi = it->GetSeqId()->GetGi();
            }
            SetLoadedGi(gi);
            return true;
        }
    }
    SetLoadedGi(0);
    return true;
}


void CLoadInfoSeq_ids::SetLoadedGi(int gi)
{
    _ASSERT(!m_GiLoaded || m_Gi == gi);
    m_Gi = gi;
    m_GiLoaded = true;
}


bool CLoadInfoSeq_ids::IsLoadedAccVer(void)
{
    if ( m_AccLoaded ) {
        return true;
    }
    if ( !IsLoaded() ) {
        return false;
    }
    CSeq_id_Handle acc;
    ITERATE ( CLoadInfoSeq_ids, it, *this ) {
        if ( !it->IsGi() && it->GetSeqId()->GetTextseq_Id() ) {
            acc = *it;
            break;
        }
    }
    SetLoadedAccVer(acc);
    return true;
}


void CLoadInfoSeq_ids::SetLoadedAccVer(const CSeq_id_Handle& acc)
{
    if ( !acc || acc.Which() == CSeq_id::e_Gi ) {
        _ASSERT(!acc || acc.GetGi() == 0);
        _ASSERT(!m_AccLoaded || m_Acc == CSeq_id_Handle());
        m_Acc = CSeq_id_Handle();
    }
    else {
        _ASSERT(acc.GetSeqId()->GetTextseq_Id());
        _ASSERT(!m_AccLoaded || m_Acc == acc);
        m_Acc = acc;
    }
    m_AccLoaded = true;
}


bool CLoadInfoSeq_ids::IsLoadedLabel(void)
{
    if ( m_LabelLoaded ) {
        return true;
    }
    if ( !IsLoaded() ) {
        return false;
    }
    m_Label = objects::GetLabel(m_Seq_ids);
    m_LabelLoaded = true;
    return true;
}


void CLoadInfoSeq_ids::SetLoadedLabel(const string& label)
{
    m_Label = label;
    m_LabelLoaded = true;
}


bool CLoadInfoSeq_ids::IsLoadedTaxId(void)
{
    if ( m_TaxIdLoaded ) {
        return true;
    }
    if ( IsLoaded() && (m_State & CBioseq_Handle::fState_no_data) ) {
        // update no taxid for unknown sequences
        m_TaxId = 0;
        m_TaxIdLoaded = true;
        return true;
    }
    return false;
}


void CLoadInfoSeq_ids::SetLoadedTaxId(int taxid)
{
    m_TaxId = taxid;
    m_TaxIdLoaded = true;
}


/////////////////////////////////////////////////////////////////////////////
// CLoadInfoBlob_ids
/////////////////////////////////////////////////////////////////////////////

CLoadInfoBlob_ids::CLoadInfoBlob_ids(const TSeq_id& id,
                                     const SAnnotSelector* /*sel*/)
    : m_Seq_id(id),
      m_State(0)
{
}


CLoadInfoBlob_ids::CLoadInfoBlob_ids(const pair<TSeq_id, string>& key)
    : m_Seq_id(key.first),
      m_State(0)
{
}


CLoadInfoBlob_ids::~CLoadInfoBlob_ids(void)
{
}


CLoadInfoBlob_ids::TBlob_Info&
CLoadInfoBlob_ids::AddBlob_id(const TBlobId& id, const TBlob_Info& info)
{
    _ASSERT(!IsLoaded());
    return m_Blob_ids.insert(TBlobIds::value_type(Ref(new TBlobId(id)), info))
        .first->second;
}


/////////////////////////////////////////////////////////////////////////////
// CLoadInfoBlob
/////////////////////////////////////////////////////////////////////////////
#if 0
CLoadInfoBlob::CLoadInfoBlob(const TBlobId& id)
    : m_Blob_id(id),
      m_Blob_State(eState_normal)
{
}


CLoadInfoBlob::~CLoadInfoBlob(void)
{
}


CRef<CTSE_Info> CLoadInfoBlob::GetTSE_Info(void) const
{
    return m_TSE_Info;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoadInfoLock
/////////////////////////////////////////////////////////////////////////////

CLoadInfoLock::CLoadInfoLock(CReaderRequestResult& owner,
                             const CRef<CLoadInfo>& info)
    : m_Owner(owner),
      m_Info(info),
      m_Guard(m_Info->m_LoadLock, owner)
{
}


CLoadInfoLock::~CLoadInfoLock(void)
{
}


void CLoadInfoLock::ReleaseLock(void)
{
    m_Guard.Release();
    m_Owner.ReleaseLoadLock(m_Info);
}


void CLoadInfoLock::SetLoaded(CObject* obj)
{
    _ASSERT(!m_Info->m_LoadLock);
    if ( !obj ) {
        obj = new CObject;
    }
    m_Info->m_LoadLock.Reset(obj);
    ReleaseLock();
}


/////////////////////////////////////////////////////////////////////////////
// CLoadLock_Base
/////////////////////////////////////////////////////////////////////////////

void CLoadLock_Base::Lock(TInfo& info, TMutexSource& src)
{
    m_Info.Reset(&info);
    if ( !m_Info->IsLoaded() ) {
        m_Lock = src.GetLoadLock(m_Info);
    }
}


void CLoadLock_Base::SetLoaded(CObject* obj)
{
    m_Lock->SetLoaded(obj);
}


/////////////////////////////////////////////////////////////////////////////
// CLoadLockSeq_ids
/////////////////////////////////////////////////////////////////////////////


CLoadLockSeq_ids::CLoadLockSeq_ids(TMutexSource& src, const string& seq_id)
{
    CRef<TInfo> info = src.GetInfoSeq_ids(seq_id);
    Lock(*info, src);
}


CLoadLockSeq_ids::CLoadLockSeq_ids(TMutexSource& src,
                                   const CSeq_id_Handle& seq_id)
    : m_Blob_ids(src, seq_id, 0)
{
    CRef<TInfo> info = src.GetInfoSeq_ids(seq_id);
    Lock(*info, src);
    if ( !IsLoaded() ) {
        src.SetRequestedId(seq_id);
    }
}


CLoadLockSeq_ids::CLoadLockSeq_ids(TMutexSource& src,
                                   const CSeq_id_Handle& seq_id,
                                   const SAnnotSelector* sel)
    : m_Blob_ids(src, seq_id, sel)
{
    CRef<TInfo> info = src.GetInfoSeq_ids(seq_id);
    Lock(*info, src);
    if ( !IsLoaded() ) {
        src.SetRequestedId(seq_id);
    }
}


void CLoadLockSeq_ids::AddSeq_id(const CSeq_id_Handle& seq_id)
{
    Get().m_Seq_ids.push_back(seq_id);
}


void CLoadLockSeq_ids::AddSeq_id(const CSeq_id& seq_id)
{
    AddSeq_id(CSeq_id_Handle::GetHandle(seq_id));
}


/////////////////////////////////////////////////////////////////////////////
// CBlob_Info
/////////////////////////////////////////////////////////////////////////////


CBlob_Info::CBlob_Info(TContentsMask contents)
    : m_Contents(contents)
{
}


CBlob_Info::~CBlob_Info(void)
{
}


void CBlob_Info::AddAnnotInfo(const CID2S_Seq_annot_Info& info)
{
    m_AnnotInfo.push_back(ConstRef(&info));
}


bool CBlob_Info::Matches(const CBlob_id& blob_id,
                         TContentsMask mask,
                         const SAnnotSelector* sel) const
{
    TContentsMask common_mask = GetContentsMask() & mask;
    if ( common_mask == 0 ) {
        return false;
    }

    if ( CProcessor_ExtAnnot::IsExtAnnot(blob_id) ) {
        // not named accession, but external annots
        return true;
    }

    if ( (common_mask & ~(fBlobHasExtAnnot|fBlobHasNamedAnnot)) != 0 ) {
        // not only features;
        return true;
    }

    // only features

    if ( GetNamedAnnotNames().empty() ) {
        // no filtering by name
        return true;
    }
    
    if ( !sel || !sel->IsIncludedAnyNamedAnnotAccession() ) {
        // no names included
        return false;
    }

    if ( sel->IsIncludedNamedAnnotAccession("NA*") ) {
        // all accessions are included
        return true;
    }
    
    // annot filtering by name
    ITERATE ( TNamedAnnotNames, it, GetNamedAnnotNames() ) {
        const string& name = *it;
        if ( !NStr::StartsWith(name, "NA") ) {
            // not named accession
            return true;
        }
        if ( sel->IsIncludedNamedAnnotAccession(name) ) {
            // matches
            return true;
        }
    }
    // no match by name found
    return false;
}


/////////////////////////////////////////////////////////////////////////////
// CLoadLockBlob_ids
/////////////////////////////////////////////////////////////////////////////


CLoadLockBlob_ids::CLoadLockBlob_ids(TMutexSource& src,
                                     const CSeq_id_Handle& seq_id,
                                     const SAnnotSelector* sel)
{
    TMutexSource::TKeyBlob_ids key;
    key.first = seq_id;
    if ( sel && sel->IsIncludedAnyNamedAnnotAccession() ) {
        ITERATE ( SAnnotSelector::TNamedAnnotAccessions, it,
                  sel->GetNamedAnnotAccessions() ) {
            key.second += it->first;
            key.second += ',';
        }
    }
    CRef<TInfo> info = src.GetInfoBlob_ids(key);
    Lock(*info, src);
    if ( !IsLoaded() ) {
        src.SetRequestedId(seq_id);
    }
}


CLoadLockBlob_ids::CLoadLockBlob_ids(TMutexSource& src,
                                     const CSeq_id_Handle& seq_id,
                                     const string& na_accs)
{
    TMutexSource::TKeyBlob_ids key(seq_id, na_accs);
    CRef<TInfo> info = src.GetInfoBlob_ids(key);
    Lock(*info, src);
    if ( !IsLoaded() ) {
        src.SetRequestedId(seq_id);
    }
}


CBlob_Info& CLoadLockBlob_ids::AddBlob_id(const CBlob_id& blob_id,
                                          const CBlob_Info& blob_info)
{
    return Get().AddBlob_id(blob_id, blob_info);
}


/////////////////////////////////////////////////////////////////////////////
// CLoadLockBlob
/////////////////////////////////////////////////////////////////////////////
#if 0
CLoadLockBlob::CLoadLockBlob(TMutexSource& src, const CBlob_id& blob_id)
{
    for ( ;; ) {
        CRef<TInfo> info = src.GetInfoBlob(blob_id);
        Lock(*info, src);
        if ( src.AddTSE_Lock(*this) ) {
            // locked
            break;
        }
        else {
            // failed to lock
            if ( !IsLoaded() ) {
                // not loaded yet -> OK
                break;
            }
            else {
                if ( info->IsNotLoadable() ) {
                    // private or withdrawn
                    break;
                }
                // already loaded and dropped while trying to lock
                // we need to repeat locking procedure
            }
        }
    }
}
#endif

CLoadLockBlob::CLoadLockBlob(void)
{
}


CLoadLockBlob::CLoadLockBlob(CReaderRequestResult& src,
                             const CBlob_id& blob_id)
    : CTSE_LoadLock(src.GetBlobLoadLock(blob_id))
{
    if ( IsLoaded() ) {
        src.AddTSE_Lock(*this);
    }
    else {
        if ( src.GetRequestedId() ) {
            (**this).SetRequestedId(src.GetRequestedId());
        }
    }
}


CLoadLockBlob::TBlobState CLoadLockBlob::GetBlobState(void) const
{
    return *this ? (**this).GetBlobState() : 0;
}


void CLoadLockBlob::SetBlobState(TBlobState state)
{
    if ( *this ) {
        (**this).SetBlobState(state);
    }
}


bool CLoadLockBlob::IsSetBlobVersion(void) const
{
    return *this && (**this).GetBlobVersion() >= 0;
}


CLoadLockBlob::TBlobVersion CLoadLockBlob::GetBlobVersion(void) const
{
    return (**this).GetBlobVersion();
}


void CLoadLockBlob::SetBlobVersion(TBlobVersion version)
{
    if ( *this ) {
        (**this).SetBlobVersion(version);
    }
}


/////////////////////////////////////////////////////////////////////////////
// CReaderRequestResult
/////////////////////////////////////////////////////////////////////////////


CReaderRequestResult::CReaderRequestResult(const CSeq_id_Handle& requested_id)
    : m_Level(0),
      m_Cached(false),
      m_RequestedId(requested_id),
      m_RecursionLevel(0),
      m_RecursiveTime(0),
      m_AllocatedConnection(0),
      m_RetryDelay(0)
{
}


CReaderRequestResult::~CReaderRequestResult(void)
{
    ReleaseLocks();
    _ASSERT(!m_AllocatedConnection);
}


CGBDataLoader* CReaderRequestResult::GetLoaderPtr(void)
{
    return 0;
}


void CReaderRequestResult::SetRequestedId(const CSeq_id_Handle& requested_id)
{
    if ( !m_RequestedId ) {
        m_RequestedId = requested_id;
    }
}


CReaderRequestResult::TBlobLoadInfo&
CReaderRequestResult::x_GetBlobLoadInfo(const CBlob_id& blob_id)
{
    TBlobLoadLocks::iterator iter = m_BlobLoadLocks.lower_bound(blob_id);
    if ( iter == m_BlobLoadLocks.end() || iter->first != blob_id ) {
        iter = m_BlobLoadLocks.insert(iter, TBlobLoadLocks::value_type(blob_id, TBlobLoadInfo(-1, CTSE_LoadLock())));
    }
    return iter->second;
}


CTSE_LoadLock CReaderRequestResult::GetBlobLoadLock(const CBlob_id& blob_id)
{
    TBlobLoadInfo& info = x_GetBlobLoadInfo(blob_id);
    if ( !info.second ) {
        info.second = GetTSE_LoadLock(blob_id);
        if ( info.first != -1 ) {
            info.second->SetBlobVersion(info.first);
        }
    }
    return info.second;
}


bool CReaderRequestResult::IsBlobLoaded(const CBlob_id& blob_id)
{
    TBlobLoadInfo& info = x_GetBlobLoadInfo(blob_id);
    if ( !info.second ) {
        info.second = GetTSE_LoadLockIfLoaded(blob_id);
        if ( !info.second ) {
            return false;
        }
    }
    if ( info.second.IsLoaded() ) {
        return true;
    }
    return false;
}


bool CReaderRequestResult::SetBlobVersion(const CBlob_id& blob_id,
                                          TBlobState blob_version)
{
    bool changed = false;
    TBlobLoadInfo& info = x_GetBlobLoadInfo(blob_id);
    if ( info.first != blob_version ) {
        info.first = blob_version;
        changed = true;
    }
    if ( info.second && info.second->GetBlobVersion() != blob_version ) {
        info.second->SetBlobVersion(blob_version);
        changed = true;
    }
    return changed;
}


bool CReaderRequestResult::SetNoBlob(const CBlob_id& blob_id,
                                     TBlobState blob_state)
{
    CLoadLockBlob blob(*this, blob_id);
    if ( blob.IsLoaded() ) {
        return false;
    }
    if ( blob.GetBlobState() == blob_state ) {
        return false;
    }
    blob.SetBlobState(blob_state);
    blob.SetLoaded();
    return true;
}


void CReaderRequestResult::ReleaseNotLoadedBlobs(void)
{
    for ( TBlobLoadLocks::iterator it = m_BlobLoadLocks.begin(); it != m_BlobLoadLocks.end(); ) {
        if ( it->second.second && !it->second.second.IsLoaded() ) {
            m_BlobLoadLocks.erase(it++);
        }
        else {
            ++it;
        }
    }
}


void CReaderRequestResult::GetLoadedBlob_ids(const CSeq_id_Handle& /*idh*/,
                                             TLoadedBlob_ids& /*blob_ids*/) const
{
    return;
}


#if 0
void CReaderRequestResult::SetTSE_Info(CLoadLockBlob& blob,
                                       const CRef<CTSE_Info>& tse)
{
    blob->m_TSE_Info = tse;
    AddTSE_Lock(AddTSE(tse, blob->GetBlob_id()));
    SetLoaded(blob);
}


CRef<CTSE_Info> CReaderRequestResult::GetTSE_Info(const CLoadLockBlob& blob)
{
    return blob->GetTSE_Info();
}


void CReaderRequestResult::SetTSE_Info(const CBlob_id& blob_id,
                                       const CRef<CTSE_Info>& tse)
{
    CLoadLockBlob blob(*this, blob_id);
    SetTSE_Info(blob, tse);
}


CRef<CTSE_Info> CReaderRequestResult::GetTSE_Info(const CBlob_id& blob_id)
{
    return GetTSE_Info(CLoadLockBlob(*this, blob_id));
}
#endif

CRef<CLoadInfoLock>
CReaderRequestResult::GetLoadLock(const CRef<CLoadInfo>& info)
{
    CRef<CLoadInfoLock>& lock = m_LockMap[info];
    if ( !lock ) {
        lock = new CLoadInfoLock(*this, info);
    }
    else {
        _ASSERT(lock->Referenced());
    }
    return lock;
}


void CReaderRequestResult::ReleaseLoadLock(const CRef<CLoadInfo>& info)
{
    m_LockMap[info] = null;
}


void CReaderRequestResult::AddTSE_Lock(const TTSE_Lock& tse_lock)
{
    _ASSERT(tse_lock);
    m_TSE_LockSet.insert(tse_lock);
}

#if 0
bool CReaderRequestResult::AddTSE_Lock(const TKeyBlob& blob_id)
{
    return AddTSE_Lock(CLoadLockBlob(*this, blob_id));
}


bool CReaderRequestResult::AddTSE_Lock(const CLoadLockBlob& blob)
{
    CRef<CTSE_Info> tse = blob->GetTSE_Info();
    if ( !tse ) {
        return false;
    }
    TTSE_Lock tse_lock = LockTSE(tse);
    if ( !tse_lock ) {
        return false;
    }
    AddTSE_Lock(tse_lock);
    return true;
}


TTSE_Lock CReaderRequestResult::LockTSE(CRef<CTSE_Info> /*tse*/)
{
    return TTSE_Lock();
}


TTSE_Lock CReaderRequestResult::AddTSE(CRef<CTSE_Info> /*tse*/,
                                       const TKeyBlob& blob_id)
{
    return TTSE_Lock();
}
#endif

void CReaderRequestResult::SaveLocksTo(TTSE_LockSet& locks)
{
    ITERATE ( TTSE_LockSet, it, GetTSE_LockSet() ) {
        locks.insert(*it);
    }
}


void CReaderRequestResult::ReleaseLocks(void)
{
    m_BlobLoadLocks.clear();
    m_TSE_LockSet.clear();
    NON_CONST_ITERATE ( TLockMap, it, m_LockMap ) {
        it->second = null;
    }
}


double CReaderRequestResult::StartRecursion(void)
{
    double rec_time = m_RecursiveTime;
    m_RecursiveTime = 0;
    ++m_RecursionLevel;
    return rec_time;
}


void CReaderRequestResult::EndRecursion(double saved_time)
{
    _ASSERT(m_RecursionLevel>0);
    m_RecursiveTime += saved_time;
    --m_RecursionLevel;
}


double CReaderRequestResult::GetCurrentRequestTime(double time)
{
    double rec_time = m_RecursiveTime;
    if ( rec_time > time ) {
        return 0;
    }
    else {
        m_RecursiveTime = time;
        return time - rec_time;
    }
}


/////////////////////////////////////////////////////////////////////////////
// CStandaloneRequestResult
/////////////////////////////////////////////////////////////////////////////


CStandaloneRequestResult::
CStandaloneRequestResult(const CSeq_id_Handle& requested_id)
    : CReaderRequestResult(requested_id)
{
}


CStandaloneRequestResult::~CStandaloneRequestResult(void)
{
    ReleaseLocks();
}


CRef<CLoadInfoSeq_ids>
CStandaloneRequestResult::GetInfoSeq_ids(const string& key)
{
    CRef<CLoadInfoSeq_ids>& ret = m_InfoSeq_ids[key];
    if ( !ret ) {
        ret = new CLoadInfoSeq_ids();
    }
    return ret;
}


CRef<CLoadInfoSeq_ids>
CStandaloneRequestResult::GetInfoSeq_ids(const CSeq_id_Handle& key)
{
    CRef<CLoadInfoSeq_ids>& ret = m_InfoSeq_ids2[key];
    if ( !ret ) {
        ret = new CLoadInfoSeq_ids();
    }
    return ret;
}


CRef<CLoadInfoBlob_ids>
CStandaloneRequestResult::GetInfoBlob_ids(const TKeyBlob_ids& key)
{
    CRef<CLoadInfoBlob_ids>& ret = m_InfoBlob_ids[key];
    if ( !ret ) {
        ret = new CLoadInfoBlob_ids(key.first, 0);
    }
    return ret;
}


CTSE_LoadLock
CStandaloneRequestResult::GetTSE_LoadLock(const CBlob_id& blob_id)
{
    if ( !m_DataSource ) {
        m_DataSource = new CDataSource;
    }
    CDataLoader::TBlobId key(new CBlob_id(blob_id));
    return m_DataSource->GetTSE_LoadLock(key);
}


CTSE_LoadLock
CStandaloneRequestResult::GetTSE_LoadLockIfLoaded(const CBlob_id& blob_id)
{
    if ( !m_DataSource ) {
        m_DataSource = new CDataSource;
    }
    CDataLoader::TBlobId key(new CBlob_id(blob_id));
    return m_DataSource->GetTSE_LoadLockIfLoaded(key);
}


CStandaloneRequestResult::operator CInitMutexPool&(void)
{
    return m_MutexPool;
}


CStandaloneRequestResult::TConn CStandaloneRequestResult::GetConn(void)
{
    return 0;
}


void CStandaloneRequestResult::ReleaseConn(void)
{
}


END_SCOPE(objects)
END_NCBI_SCOPE
