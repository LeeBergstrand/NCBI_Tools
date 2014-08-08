#ifndef GBLOADER_REQUEST_RESULT__HPP_INCLUDED
#define GBLOADER_REQUEST_RESULT__HPP_INCLUDED

/*  $Id: request_result.hpp 390833 2013-03-02 19:17:52Z dicuccio $
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
*  ===========================================================================
*
*  Author: Eugene Vasilchenko
*
*  File Description:
*   Class for storing results of reader's request and thread synchronization
*
* ===========================================================================
*/

#include <corelib/ncbiobj.hpp>
#include <corelib/ncbimtx.hpp>
#include <corelib/ncbitime.hpp>
#include <objects/seq/seq_id_handle.hpp>
#include <objects/seqsplit/ID2S_Seq_annot_Info.hpp>
#include <util/mutex_pool.hpp>
#include <objmgr/impl/tse_loadlock.hpp>
#include <objtools/data_loaders/genbank/blob_id.hpp>

#include <map>
#include <set>
#include <string>
#include <vector>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CSeq_id;
class CSeq_id_Handle;
class CID2_Blob_Id;
class CID2S_Seq_annot_Info;
class CBlob_id;
class CTSE_Info;
class CTimer;

class CGBDataLoader;
class CLoadInfo;
class CLoadInfoLock;
class CLoadLock_Base;
class CReaderRequestResult;
class CReaderRequestConn;
class CReaderAllocatedConnection;

/////////////////////////////////////////////////////////////////////////////
// resolved information classes
/////////////////////////////////////////////////////////////////////////////

class NCBI_XREADER_EXPORT CLoadInfo : public CObject
{
public:
    CLoadInfo(void);
    ~CLoadInfo(void);

    bool IsLoaded(void) const
        {
            return m_LoadLock;
        }

private:
    friend class CLoadInfoLock;
    friend class CLoadLock_Base;

    CLoadInfo(const CLoadInfo&);
    CLoadInfo& operator=(const CLoadInfo&);

    CInitMutex<CObject> m_LoadLock;
};


class NCBI_XREADER_EXPORT CLoadInfoSeq_ids : public CLoadInfo
{
public:
    typedef CSeq_id_Handle TSeq_id;
    typedef vector<TSeq_id> TSeq_ids;
    typedef TSeq_ids::const_iterator const_iterator;

    CLoadInfoSeq_ids(void);
    CLoadInfoSeq_ids(const CSeq_id_Handle& seq_id);
    CLoadInfoSeq_ids(const string& seq_id);
    ~CLoadInfoSeq_ids(void);

    bool IsLoadedGi(void);
    int GetGi(void) const
        {
            _ASSERT(m_GiLoaded);
            return m_Gi;
        }
    void SetLoadedGi(int gi);

    bool IsLoadedAccVer(void);
    CSeq_id_Handle GetAccVer(void) const
        {
            _ASSERT(m_AccLoaded);
            _ASSERT(!m_Acc || m_Acc.GetSeqId()->GetTextseq_Id());
            return m_Acc;
        }
    void SetLoadedAccVer(const CSeq_id_Handle& acc);

    bool IsLoadedLabel(void);
    const string& GetLabel(void) const
        {
            _ASSERT(m_LabelLoaded);
            return m_Label;
        }
    void SetLoadedLabel(const string& label);

    bool IsLoadedTaxId(void);
    int GetTaxId(void) const
        {
            _ASSERT(m_TaxIdLoaded);
            return m_TaxId;
        }
    void SetLoadedTaxId(int taxid);

    const_iterator begin(void) const
        {
            return m_Seq_ids.begin();
        }
    const_iterator end(void) const
        {
            return m_Seq_ids.end();
        }
    size_t size(void) const
        {
            return m_Seq_ids.size();
        }
    bool empty(void) const
        {
            return m_Seq_ids.empty();
        }

    typedef int TState;
    TState GetState(void) const
        {
            return m_State;
        }
    void SetState(TState state)
        {
            m_State = state;
        }
    
public:
    TSeq_ids    m_Seq_ids;
    bool        m_GiLoaded;
    bool        m_AccLoaded;
    bool        m_LabelLoaded;
    bool        m_TaxIdLoaded;
    int         m_Gi;
    CSeq_id_Handle m_Acc;
    string      m_Label;
    int         m_TaxId;
    TState      m_State;
};


class NCBI_XREADER_EXPORT CBlob_Info
{
public:
    typedef TBlobContentsMask TContentsMask;
    typedef set<string> TNamedAnnotNames;

    explicit CBlob_Info(TContentsMask contents);
    ~CBlob_Info(void);

    TContentsMask GetContentsMask(void) const
        {
            return m_Contents;
        }
    const TNamedAnnotNames& GetNamedAnnotNames(void) const
        {
            return m_NamedAnnotNames;
        }

    void AddNamedAnnotName(const string& name)
        {
            m_NamedAnnotNames.insert(name);
        }
    void AddAnnotInfo(const CID2S_Seq_annot_Info& info);
    bool IsSetAnnotInfo(void) const
        {
            return !m_AnnotInfo.empty();
        }
    typedef vector< CConstRef<CID2S_Seq_annot_Info> > TAnnotInfo;
    const TAnnotInfo& GetAnnotInfo(void) const
        {
            return m_AnnotInfo;
        }

    bool Matches(const CBlob_id& blob_id,
                 TContentsMask mask,
                 const SAnnotSelector* sel) const;

private:
    TContentsMask   m_Contents;
    TNamedAnnotNames m_NamedAnnotNames;
    TAnnotInfo m_AnnotInfo;
};


class NCBI_XREADER_EXPORT CLoadInfoBlob_ids : public CLoadInfo
{
public:
    typedef CSeq_id_Handle TSeq_id;
    typedef TSeq_id TKey;
    typedef CBlob_id TBlobId;
    typedef CBlob_Info TBlob_Info;
    typedef map<CRef<TBlobId>, TBlob_Info> TBlobIds;
    typedef TBlobIds::const_iterator const_iterator;

    CLoadInfoBlob_ids(const TSeq_id& id, const SAnnotSelector* sel);
    CLoadInfoBlob_ids(const pair<TSeq_id, string>& key);
    ~CLoadInfoBlob_ids(void);

    const TSeq_id& GetSeq_id(void) const
        {
            return m_Seq_id;
        }

    const_iterator begin(void) const
        {
            return m_Blob_ids.begin();
        }
    const_iterator end(void) const
        {
            return m_Blob_ids.end();
        }
    bool empty(void) const
        {
            return m_Blob_ids.empty();
        }
    size_t size(void) const
        {
            return m_Blob_ids.size();
        }
    void clear(void)
        {
            m_Blob_ids.clear();
        }

    TBlob_Info& AddBlob_id(const TBlobId& id, const TBlob_Info& info);

    typedef int TState;
    TState GetState(void) const
        {
            return m_State;
        }
    void SetState(TState state)
        {
            m_State = state;
        }

public:
    TSeq_id     m_Seq_id;
    TBlobIds    m_Blob_ids;
    TState      m_State;

    // lock/refresh support
    double      m_RefreshTime;
};


/*
class NCBI_XREADER_EXPORT CLoadInfoBlob : public CLoadInfo
{
public:
    typedef CBlob_id TBlobId;
    typedef TBlobId TKey;

    CLoadInfoBlob(const TBlobId& id);
    ~CLoadInfoBlob(void);

    const TBlobId& GetBlob_id(void) const
        {
            return m_Blob_id;
        }

    CRef<CTSE_Info> GetTSE_Info(void) const;

    enum EBlobState {
        eState_normal,
        eState_suppressed_temp,
        eState_suppressed,
        eState_withdrawn
    };

    EBlobState GetBlobState(void) const
        {
            return m_Blob_State;
        }
    void SetBlobState(EBlobState state)
        {
            m_Blob_State = state;
        }
    bool IsNotLoadable(void) const
        {
            return GetBlobState() >= eState_withdrawn;
        }

public:
    TBlobId    m_Blob_id;

    EBlobState m_Blob_State;

    // typedefs for various info
    typedef set<CSeq_id_Handle> TSeq_ids;

    // blob
    // if null -> not loaded yet
    CRef<CTSE_Info> m_TSE_Info;

    // set of Seq-ids with sequences in this blob
    TSeq_ids        m_Seq_ids;
};
*/


/////////////////////////////////////////////////////////////////////////////
// resolved information locks
/////////////////////////////////////////////////////////////////////////////

class NCBI_XREADER_EXPORT CLoadInfoLock : public CObject
{
public:
    ~CLoadInfoLock(void);

    void ReleaseLock(void);
    void SetLoaded(CObject* obj = 0);
    const CLoadInfo& GetLoadInfo(void) const
        {
            return *m_Info;
        }

protected:
    friend class CReaderRequestResult;

    CLoadInfoLock(CReaderRequestResult& owner,
                  const CRef<CLoadInfo>& info);

private:
    CReaderRequestResult&   m_Owner;
    CRef<CLoadInfo>         m_Info;
    CInitGuard              m_Guard;

private:
    CLoadInfoLock(const CLoadInfoLock&);
    CLoadInfoLock& operator=(const CLoadInfoLock&);
};


class NCBI_XREADER_EXPORT CLoadLock_Base
{
public:
    typedef CLoadInfo TInfo;
    typedef CLoadInfoLock TLock;
    typedef CReaderRequestResult TMutexSource;

    bool IsLoaded(void) const
        {
            return Get().IsLoaded();
        }
    void SetLoaded(CObject* obj = 0);

    TInfo& Get(void)
        {
            return *m_Info;
        }
    const TInfo& Get(void) const
        {
            return *m_Info;
        }

    const CRef<TLock> GetLock(void) const
        {
            return m_Lock;
        }

protected:
    void Lock(TInfo& info, TMutexSource& src);

private:
    CRef<TInfo> m_Info;
    CRef<TLock> m_Lock;
};


class NCBI_XREADER_EXPORT CLoadLockBlob_ids : public CLoadLock_Base
{
public:
    typedef CLoadInfoBlob_ids TInfo;

    CLoadLockBlob_ids(void)
        {
        }
    CLoadLockBlob_ids(TMutexSource& src,
                      const CSeq_id_Handle& seq_id,
                      const SAnnotSelector* sel);
    CLoadLockBlob_ids(TMutexSource& src,
                      const CSeq_id_Handle& seq_id,
                      const string& na_accs);
    
    TInfo& Get(void)
        {
            return static_cast<TInfo&>(CLoadLock_Base::Get());
        }
    const TInfo& Get(void) const
        {
            return static_cast<const TInfo&>(CLoadLock_Base::Get());
        }
    TInfo& operator*(void)
        {
            return Get();
        }
    TInfo* operator->(void)
        {
            return &Get();
        }
    const TInfo& operator*(void) const
        {
            return Get();
        }
    const TInfo* operator->(void) const
        {
            return &Get();
        }

    CBlob_Info& AddBlob_id(const CBlob_id& blob_id,
                           const CBlob_Info& blob_info);

private:
    void x_Initialize(TMutexSource& src, const CSeq_id_Handle& seq_id);
};


class NCBI_XREADER_EXPORT CLoadLockSeq_ids : public CLoadLock_Base
{
public:
    typedef CLoadInfoSeq_ids TInfo;

    CLoadLockSeq_ids(TMutexSource& src, const string& seq_id);
    CLoadLockSeq_ids(TMutexSource& src, const CSeq_id_Handle& seq_id);
    CLoadLockSeq_ids(TMutexSource& src, const CSeq_id_Handle& seq_id,
                     const SAnnotSelector* sel);

    TInfo& Get(void)
        {
            return static_cast<TInfo&>(CLoadLock_Base::Get());
        }
    const TInfo& Get(void) const
        {
            return static_cast<const TInfo&>(CLoadLock_Base::Get());
        }
    TInfo& operator*(void)
        {
            return Get();
        }
    TInfo* operator->(void)
        {
            return &Get();
        }
    const TInfo& operator*(void) const
        {
            return Get();
        }
    const TInfo* operator->(void) const
        {
            return &Get();
        }

    void AddSeq_id(const CSeq_id_Handle& seq_id);
    void AddSeq_id(const CSeq_id& seq_id);

    CLoadLockBlob_ids& GetBlob_ids(void)
        {
            return m_Blob_ids;
        }

private:
    CLoadLockBlob_ids m_Blob_ids;
};


class NCBI_XREADER_EXPORT CLoadLockBlob : public CTSE_LoadLock
{
public:
    CLoadLockBlob(void);
    CLoadLockBlob(CReaderRequestResult& src, const CBlob_id& blob_id);

    typedef int TBlobState;
    typedef int TBlobVersion;
    typedef list< CRef<CID2S_Seq_annot_Info> > TAnnotInfo;

    TBlobState GetBlobState(void) const;
    void SetBlobState(TBlobState state);
    bool IsSetBlobVersion(void) const;
    TBlobVersion GetBlobVersion(void) const;
    void SetBlobVersion(TBlobVersion);
};

/*
class NCBI_XREADER_EXPORT CLoadLockBlob : public CLoadLock_Base
{
public:
    typedef CLoadInfoBlob TInfo;

    CLoadLockBlob(TMutexSource& src, const CBlob_id& blob_id);
    
    TInfo& Get(void)
        {
            return static_cast<TInfo&>(CLoadLock_Base::Get());
        }
    const TInfo& Get(void) const
        {
            return static_cast<const TInfo&>(CLoadLock_Base::Get());
        }
    TInfo& operator*(void)
        {
            return Get();
        }
    TInfo* operator->(void)
        {
            return &Get();
        }
    const TInfo& operator*(void) const
        {
            return Get();
        }
    const TInfo* operator->(void) const
        {
            return &Get();
        }
};
*/


/////////////////////////////////////////////////////////////////////////////
// whole request lock
/////////////////////////////////////////////////////////////////////////////


class NCBI_XREADER_EXPORT CReaderRequestResult
{
public:
    CReaderRequestResult(const CSeq_id_Handle& requested_id);
    virtual ~CReaderRequestResult(void);

    typedef string TKeySeq_ids;
    typedef CSeq_id_Handle TKeySeq_ids2;
    typedef CLoadInfoSeq_ids TInfoSeq_ids;
    typedef pair<CSeq_id_Handle, string> TKeyBlob_ids;
    typedef CLoadInfoBlob_ids TInfoBlob_ids;
    typedef CBlob_id TKeyBlob;
    typedef CTSE_Lock TTSE_Lock;
    //typedef CLoadInfoBlob TInfoBlob;
    typedef CLoadLockBlob TLockBlob;
    typedef int TLevel;
    typedef int TBlobVersion;
    typedef int TBlobState;

    virtual CGBDataLoader* GetLoaderPtr(void);

    virtual CRef<TInfoSeq_ids>  GetInfoSeq_ids(const TKeySeq_ids& seq_id) = 0;
    virtual CRef<TInfoSeq_ids>  GetInfoSeq_ids(const TKeySeq_ids2& seq_id) = 0;
    virtual CRef<TInfoBlob_ids> GetInfoBlob_ids(const TKeyBlob_ids& seq_id) = 0;
    //virtual CRef<TInfoBlob>     GetInfoBlob(const TKeyBlob& blob_id) = 0;
    CTSE_LoadLock GetBlobLoadLock(const TKeyBlob& blob_id);
    bool IsBlobLoaded(const TKeyBlob& blob_id);

    virtual CTSE_LoadLock GetTSE_LoadLock(const TKeyBlob& blob_id) = 0;
    virtual CTSE_LoadLock GetTSE_LoadLockIfLoaded(const TKeyBlob& blob_id) = 0;

    typedef vector<CBlob_id> TLoadedBlob_ids;
    virtual void GetLoadedBlob_ids(const CSeq_id_Handle& idh,
                                   TLoadedBlob_ids& blob_ids) const;

    bool SetBlobVersion(const TKeyBlob& blob_id, TBlobVersion version);
    bool SetNoBlob(const TKeyBlob& blob_id, TBlobState blob_state);
    void ReleaseNotLoadedBlobs(void);

    // load ResultBlob
    //virtual CRef<CTSE_Info> GetTSE_Info(const TLockBlob& blob);
    //CRef<CTSE_Info> GetTSE_Info(const TKeyBlob& blob_id);
    //virtual void SetTSE_Info(TLockBlob& blob, const CRef<CTSE_Info>& tse);
    //void SetTSE_Info(const TKeyBlob& blob_id, const CRef<CTSE_Info>& tse);

    //bool AddTSE_Lock(const TLockBlob& blob);
    //bool AddTSE_Lock(const TKeyBlob& blob_id);
    void AddTSE_Lock(const TTSE_Lock& lock);
    typedef set<TTSE_Lock> TTSE_LockSet;
    const TTSE_LockSet& GetTSE_LockSet(void) const
        {
            return m_TSE_LockSet;
        }
    void SaveLocksTo(TTSE_LockSet& locks);

    void ReleaseLocks(void);

    //virtual TTSE_Lock AddTSE(CRef<CTSE_Info> tse, const TKeyBlob& blob_id);
    //virtual TTSE_Lock LockTSE(CRef<CTSE_Info> tse);

    virtual operator CInitMutexPool&(void) = 0;
    CRef<CLoadInfoLock> GetLoadLock(const CRef<CLoadInfo>& info);

    typedef int TConn;

    TLevel GetLevel() const { return m_Level; }
    void SetLevel(TLevel level) { m_Level = level; }
    bool IsCached() const { return m_Cached; }
    void SetCached() { m_Cached = true; }

    const CSeq_id_Handle& GetRequestedId(void) const { return m_RequestedId; }
    void SetRequestedId(const CSeq_id_Handle& requested_id);

    int GetRecursionLevel(void) const
        {
            return m_RecursionLevel;
        }
    double StartRecursion(void);
    void EndRecursion(double saved_time);
    friend class CRecurse;
    class CRecurse : public CStopWatch
    {
    public:
        CRecurse(CReaderRequestResult& result)
            : CStopWatch(eStart),
              m_Result(result),
              m_SaveTime(result.StartRecursion())
            {
            }
        ~CRecurse(void)
            {
                m_Result.EndRecursion(m_SaveTime);
            }
    private:
        CReaderRequestResult& m_Result;
        double m_SaveTime;
    };

    double GetCurrentRequestTime(double time);

    void ClearRetryDelay(void) { m_RetryDelay = 0; }
    void AddRetryDelay(double delay) { m_RetryDelay += delay; }
    double GetRetryDelay(void) const { return m_RetryDelay; }

private:
    friend class CLoadInfoLock;
    friend class CReaderAllocatedConnection;

    void ReleaseLoadLock(const CRef<CLoadInfo>& info);

    typedef map<CRef<CLoadInfo>, CRef<CLoadInfoLock> > TLockMap;
    typedef pair<TBlobVersion, CTSE_LoadLock> TBlobLoadInfo;
    typedef map<CBlob_id, TBlobLoadInfo> TBlobLoadLocks;

    TBlobLoadInfo& x_GetBlobLoadInfo(const TKeyBlob& blob_id);

    TLockMap        m_LockMap;
    TTSE_LockSet    m_TSE_LockSet;
    TBlobLoadLocks  m_BlobLoadLocks;
    TLevel          m_Level;
    bool            m_Cached;
    CSeq_id_Handle  m_RequestedId;
    int             m_RecursionLevel;
    double          m_RecursiveTime;
    CReaderAllocatedConnection* m_AllocatedConnection;
    double          m_RetryDelay;

private: // hide methods
    CReaderRequestResult(const CReaderRequestResult&);
    CReaderRequestResult& operator=(const CReaderRequestResult&);
};


class NCBI_XREADER_EXPORT CStandaloneRequestResult :
    public CReaderRequestResult
{
public:
    CStandaloneRequestResult(const CSeq_id_Handle& requested_id);
    virtual ~CStandaloneRequestResult(void);

    virtual CRef<TInfoSeq_ids>  GetInfoSeq_ids(const TKeySeq_ids& seq_id);
    virtual CRef<TInfoSeq_ids>  GetInfoSeq_ids(const TKeySeq_ids2& seq_id);
    virtual CRef<TInfoBlob_ids> GetInfoBlob_ids(const TKeyBlob_ids& seq_id);

    virtual CTSE_LoadLock GetTSE_LoadLock(const TKeyBlob& blob_id);
    virtual CTSE_LoadLock GetTSE_LoadLockIfLoaded(const TKeyBlob& blob_id);

    virtual operator CInitMutexPool&(void);


    virtual TConn GetConn(void);
    virtual void ReleaseConn(void);

    void ReleaseTSE_LoadLocks();

    CInitMutexPool    m_MutexPool;

    CRef<CDataSource> m_DataSource;

    map<TKeySeq_ids, CRef<TInfoSeq_ids> >   m_InfoSeq_ids;
    map<TKeySeq_ids2, CRef<TInfoSeq_ids> >  m_InfoSeq_ids2;
    map<TKeyBlob_ids, CRef<TInfoBlob_ids> > m_InfoBlob_ids;
};


END_SCOPE(objects)
END_NCBI_SCOPE

#endif//GBLOADER_REQUEST_RESULT__HPP_INCLUDED
