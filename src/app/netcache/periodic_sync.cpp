/*  $Id: periodic_sync.cpp 369331 2012-07-18 15:07:38Z gouriano $
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
 * Authors: Denis Vakatov, Pavel Ivanov, Sergey Satskiy
 *
 * File Description: Data structures and API to support blobs mirroring.
 *
 */

#include "nc_pch.hpp"

#include <corelib/request_ctx.hpp>
#include <util/random_gen.hpp>

#include "periodic_sync.hpp"
#include "distribution_conf.hpp"
#include "netcached.hpp"
#include "sync_log.hpp"
#include "peer_control.hpp"
#include "active_handler.hpp"
#include "nc_storage.hpp"
#include "nc_stat.hpp"


BEGIN_NCBI_SCOPE


static TSyncSlotsMap    s_SlotsList;
static TSyncSlotsMap    s_SlotsMap;
static CMiniMutex       s_RndLock;
static CRandom          s_Rnd(CRandom::TValue(time(NULL)));

typedef vector<CNCActiveSyncControl*> TSyncControls;
static TSyncControls s_SyncControls;
static CNCLogCleaner* s_LogCleaner;

static FILE* s_LogFile = NULL;



static void
s_FindServerSlot(Uint8 server_id,
                 Uint2 slot,
                 SSyncSlotData*& slot_data,
                 SSyncSlotSrv*&  slot_srv)
{
    slot_data = NULL;
    slot_srv = NULL;
    TSyncSlotsMap::const_iterator it_slot = s_SlotsMap.find(slot);
    if (it_slot == s_SlotsMap.end())
        return;
    slot_data = it_slot->second;
    slot_data->lock.Lock();
    TSlotSrvsList srvs = slot_data->srvs;
    slot_data->lock.Unlock();
    ITERATE(TSlotSrvsList, it_srv, srvs) {
        SSyncSlotSrv* this_srv = it_srv->second;
        if (this_srv->peer->GetSrvId() == server_id) {
            slot_srv = this_srv;
            return;
        }
    }
}

static ESyncInitiateResult
s_StartSync(SSyncSlotData* slot_data, SSyncSlotSrv* slot_srv, bool is_passive)
{
    CMiniMutexGuard g_slot(slot_data->lock);
    if (slot_data->cleaning  ||  slot_data->clean_required)
        return eServerBusy;

    CMiniMutexGuard g_srv(slot_srv->lock);
    if (slot_srv->sync_started) {
        if (!is_passive  ||  !slot_srv->is_passive  ||  slot_srv->started_cmds != 0)
            return eCrossSynced;
        if (slot_srv->cnt_sync_ops != 0)
            CNCStat::PeerSyncFinished(slot_srv->cnt_sync_ops, false);
        slot_srv->sync_started = false;
        --slot_data->cnt_sync_started;
    }

    if (!is_passive  &&  !slot_srv->peer->StartActiveSync())
        return eServerBusy;
    slot_srv->sync_started = true;
    slot_srv->is_passive = is_passive;
    slot_srv->cnt_sync_ops = 0;
    ++slot_srv->cur_sync_id;
    ++slot_data->cnt_sync_started;
    return eProceedWithEvents;
}

static void
s_StopSync(SSyncSlotData* slot_data, SSyncSlotSrv* slot_srv, Uint8 next_delay)
{
    slot_srv->peer->RegisterSyncStop(slot_srv->is_passive,
                                     slot_srv->next_sync_time,
                                     next_delay);
    slot_srv->sync_started = false;
    if (slot_data->cnt_sync_started == 0)
        abort();
    if (--slot_data->cnt_sync_started == 0  &&  slot_data->clean_required)
        s_LogCleaner->SetRunnable();
}

static void
s_CancelSync(SSyncSlotData* slot_data, SSyncSlotSrv* slot_srv, Uint8 next_delay)
{
    CNCStat::PeerSyncFinished(slot_srv->cnt_sync_ops, false);
    s_StopSync(slot_data, slot_srv, next_delay);
}

static void
s_CommitSync(SSyncSlotData* slot_data, SSyncSlotSrv* slot_srv)
{
    CNCStat::PeerSyncFinished(slot_srv->cnt_sync_ops, true);
    if (slot_srv->is_by_blobs)
        slot_srv->was_blobs_sync = true;
    if (!slot_srv->made_initial_sync  &&  !CNCServer::IsInitiallySynced())
    {
        slot_srv->made_initial_sync = true;
        slot_srv->peer->AddInitiallySyncedSlot();
    }
    s_StopSync(slot_data, slot_srv, CNCDistributionConf::GetPeriodicSyncInterval());
}

static void
s_DoCleanLog(CNCLogCleaner* cleaner, Uint2 slot)
{
    cleaner->CreateNewDiagCtx();
    CSrvDiagMsg().StartRequest()
                 .PrintParam("_type", "clean")
                 .PrintParam("slot", slot);

    Uint8 cleaned = CNCSyncLog::Clean(slot);
    cleaner->GetDiagCtx()->SetBytesWr(Int8(cleaned));
    CSrvDiagMsg().StopRequest();
    cleaner->ReleaseDiagCtx();
}

void
CNCLogCleaner::ExecuteSlice(TSrvThreadNum /* thr_idx */)
{
    if (CTaskServer::IsInShutdown())
        return;

// clean operation log used for synchronization between peers
// that is remove already synced ones

    if (m_NextSlotIt == s_SlotsList.end()) {
        m_NextSlotIt = s_SlotsList.begin();
        RunAfter(CNCDistributionConf::GetCleanAttemptInterval());
        return;
    }

    Uint8 min_period = CNCDistributionConf::GetMinForcedCleanPeriod();

    SSyncSlotData* slot_data = m_NextSlotIt->second;
    Uint2 slot = slot_data->slot;
    slot_data->lock.Lock();
// if no sync currently in progress
    if (slot_data->cnt_sync_started == 0) {
        slot_data->cleaning = true;
        slot_data->lock.Unlock();
        s_DoCleanLog(this, slot);
        slot_data->lock.Lock();
        slot_data->cleaning = false;
        if (slot_data->clean_required) {
            slot_data->clean_required = false;
            m_LastForceTime[slot] = CSrvTime::Current().AsUSec();
        }
    }
    else if (!slot_data->clean_required
                &&  !CNCPeerControl::HasServersForInitSync()
                &&  CNCSyncLog::IsOverLimit(slot)
                &&  CSrvTime::Current().AsUSec() - m_LastForceTime[slot] >= min_period)
    {
        slot_data->clean_required = true;
    }
    slot_data->lock.Unlock();

    ++m_NextSlotIt;
    SetRunnable();
}

CNCLogCleaner::CNCLogCleaner(void)
{
    m_NextSlotIt = s_SlotsList.begin();
}

CNCLogCleaner::~CNCLogCleaner(void)
{}



SSyncSlotData::SSyncSlotData(Uint2 slot_)
    : slot(slot_),
      cnt_sync_started(0),
      cleaning(false),
      clean_required(false)
{}

SSyncSlotSrv::SSyncSlotSrv(CNCPeerControl* peer_)
    : peer(peer_),
      sync_started(false),
      was_blobs_sync(false),
      made_initial_sync(false),
      started_cmds(0),
      next_sync_time(0),
      cur_sync_id(0)
{}



bool
CNCPeriodicSync::Initialize(void)
{
    s_LogFile = fopen(CNCDistributionConf::GetPeriodicLogFile().c_str(), "a");

    const vector<Uint2>& slots = CNCDistributionConf::GetSelfSlots();
    for (Uint2 i = 0; i < slots.size(); ++i) {
        SSyncSlotData* data = new SSyncSlotData(slots[i]);
        Uint2 sort_seed;
        do {
            sort_seed = s_Rnd.GetRand(0, numeric_limits<Uint2>::max());
        }
        while (s_SlotsList.find(sort_seed) != s_SlotsList.end());
        s_SlotsList[sort_seed] = data;
        s_SlotsMap[data->slot] = data;
    }

    Uint4 cnt_to_sync = 0;
    const TNCPeerList& peers = CNCDistributionConf::GetPeers();
    ITERATE(TNCPeerList, it_peer, peers) {
        CNCPeerControl* peer = CNCPeerControl::Peer(it_peer->first);
        const vector<Uint2>& commonSlots =
                        CNCDistributionConf::GetCommonSlots(it_peer->first);
        ITERATE(vector<Uint2>, it_slot, commonSlots) {
            SSyncSlotData* slot_data = s_SlotsMap[*it_slot];
            SSyncSlotSrv* slot_srv = new SSyncSlotSrv(peer);
            Uint2 sort_seed;
            do {
                sort_seed = s_Rnd.GetRand(0, numeric_limits<Uint2>::max());
            }
            while (slot_data->srvs.find(sort_seed) != slot_data->srvs.end());
            slot_data->srvs[sort_seed] = slot_srv;
        }
        if (!commonSlots.empty()) {
            peer->SetSlotsForInitSync(Uint2(commonSlots.size()));
            ++cnt_to_sync;
        }
    }
    CNCPeerControl::SetServersForInitSync(cnt_to_sync);

    Uint1 cnt_syncs = CNCDistributionConf::GetCntActiveSyncs();
    for (Uint1 i = 0; i < cnt_syncs; ++i) {
        s_SyncControls.push_back(new CNCActiveSyncControl());
        s_SyncControls[i]->SetRunnable();
    }

    s_LogCleaner = new CNCLogCleaner();
    s_LogCleaner->SetRunnable();

    if (cnt_to_sync == 0)
        CNCServer::InitialSyncComplete();

    return true;
}

void
CNCPeriodicSync::Finalize(void)
{
    if (s_LogFile)
        fclose(s_LogFile);
}

ESyncInitiateResult
CNCPeriodicSync::Initiate(Uint8  server_id,
                          Uint2  slot,
                          Uint8* local_start_rec_no,
                          Uint8* remote_start_rec_no,
                          TReducedSyncEvents* events,
                          Uint8* sync_id)
{
    SSyncSlotData* slot_data;
    SSyncSlotSrv* slot_srv;
    s_FindServerSlot(server_id, slot, slot_data, slot_srv);
    if (slot_srv) {
        slot_srv->peer->RegisterConnSuccess();
    }
    if (slot_srv == NULL
        ||  (CNCPeerControl::HasServersForInitSync()
             &&  (slot_srv->made_initial_sync
                  ||  slot_data->cnt_sync_started != 0))
        ||  CTaskServer::IsInShutdown())
    {
        return eServerBusy;
    }

    ESyncInitiateResult init_res = s_StartSync(slot_data, slot_srv, true);
    if (init_res != eProceedWithEvents)
        return init_res;

    slot_srv->started_cmds = 1;
    *sync_id = slot_srv->cur_sync_id;
    bool records_available = CNCSyncLog::GetEventsList(server_id,
                                                       slot,
                                                       local_start_rec_no,
                                                       remote_start_rec_no,
                                                       events);
    if (records_available
        ||  (CNCSyncLog::GetLogSize(slot) == 0  &&  slot_srv->was_blobs_sync))
    {
        slot_srv->is_by_blobs = false;
        return eProceedWithEvents;
    }
    else {
        slot_srv->is_by_blobs = true;
        return eProceedWithBlobs;
    }
}

ESyncInitiateResult
CNCPeriodicSync::CanStartSyncCommand(Uint8  server_id,
                                     Uint2  slot,
                                     bool   can_abort,
                                     Uint8& sync_id)
{
    SSyncSlotData* slot_data;
    SSyncSlotSrv* slot_srv;
    s_FindServerSlot(server_id, slot, slot_data, slot_srv);
    if (slot_srv == NULL)
        return eNetworkError;
    
    CMiniMutexGuard g_slot(slot_data->lock);
    if (slot_data->clean_required  &&  can_abort)
        return eServerBusy;

    CMiniMutexGuard g_srv(slot_srv->lock);
    if (!slot_srv->sync_started  ||  !slot_srv->is_passive)
        return eNetworkError;

    ++slot_srv->started_cmds;
    ++slot_srv->cnt_sync_ops;
    sync_id = slot_srv->cur_sync_id;
    return eProceedWithEvents;
}

void
CNCPeriodicSync::MarkCurSyncByBlobs(Uint8 server_id, Uint2 slot, Uint8 sync_id)
{
    SSyncSlotData* slot_data;
    SSyncSlotSrv* slot_srv;
    s_FindServerSlot(server_id, slot, slot_data, slot_srv);

    CMiniMutexGuard g_slot(slot_data->lock);
    CMiniMutexGuard g_srv(slot_srv->lock);
    if (slot_srv->sync_started  &&  slot_srv->is_passive
        &&  slot_srv->cur_sync_id == sync_id)
    {
        slot_srv->is_by_blobs = true;
    }
}

void
CNCPeriodicSync::SyncCommandFinished(Uint8 server_id, Uint2 slot, Uint8 sync_id)
{
    SSyncSlotData* slot_data;
    SSyncSlotSrv* slot_srv;
    s_FindServerSlot(server_id, slot, slot_data, slot_srv);

    CMiniMutexGuard g_slot(slot_data->lock);
    CMiniMutexGuard g_srv(slot_srv->lock);
    if (slot_srv->sync_started  &&  slot_srv->is_passive
        &&  slot_srv->cur_sync_id == sync_id)
    {
        if (slot_srv->started_cmds == 0)
            abort();
        if (--slot_srv->started_cmds == 0)
            slot_srv->last_active_time = CSrvTime::Current().AsUSec();
    }
}

void
CNCPeriodicSync::Commit(Uint8 server_id,
                        Uint2 slot,
                        Uint8 sync_id,
                        Uint8 local_synced_rec_no,
                        Uint8 remote_synced_rec_no)
{
    CNCSyncLog::SetLastSyncRecNo(server_id, slot,
                                 local_synced_rec_no,
                                 remote_synced_rec_no);
    CNCBlobStorage::SaveMaxSyncLogRecNo();

    SSyncSlotData* slot_data;
    SSyncSlotSrv* slot_srv;
    s_FindServerSlot(server_id, slot, slot_data, slot_srv);

    CMiniMutexGuard g_slot(slot_data->lock);
    CMiniMutexGuard g_srv(slot_srv->lock);
    if (slot_srv->sync_started  &&  slot_srv->is_passive
        &&  slot_srv->cur_sync_id == sync_id)
    {
        --slot_srv->cnt_sync_ops;
        s_CommitSync(slot_data, slot_srv);
    }
}

void
CNCPeriodicSync::Cancel(Uint8 server_id, Uint2 slot, Uint8 sync_id)
{
    SSyncSlotData* slot_data;
    SSyncSlotSrv* slot_srv;
    s_FindServerSlot(server_id, slot, slot_data, slot_srv);

    CMiniMutexGuard g_slot(slot_data->lock);
    CMiniMutexGuard g_srv(slot_srv->lock);
    if (slot_srv->sync_started  &&  slot_srv->is_passive
        &&  slot_srv->cur_sync_id == sync_id)
    {
        --slot_srv->cnt_sync_ops;
        s_CancelSync(slot_data, slot_srv, 0);
    }
}


CNCActiveSyncControl::CNCActiveSyncControl(void)
{
    SetState(&Me::x_StartScanSlots);
    m_ForceInitSync = false;
    m_NeedRehash = false;
}

CNCActiveSyncControl::~CNCActiveSyncControl(void)
{}


CNCActiveSyncControl::State
CNCActiveSyncControl::x_StartScanSlots(void)
{
    m_DidSync = false;
    m_MinNextTime = numeric_limits<Uint8>::max();
    m_LoopStart = CSrvTime::Current().AsUSec();
    m_NextSlotIt = s_SlotsList.begin();
    return &Me::x_CheckSlotOurSync;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_CheckSlotOurSync(void)
{
    if (CTaskServer::IsInShutdown())
        return NULL;
    if (m_NextSlotIt == s_SlotsList.end())
        return &Me::x_FinishScanSlots;

    m_SlotData = m_NextSlotIt->second;
    m_SlotData->lock.Lock();
    if (m_SlotData->cnt_sync_started == 0  ||  m_ForceInitSync) {
        if (m_NeedRehash) {
            TSlotSrvsList::iterator it = m_SlotData->srvs.begin();
            SSyncSlotSrv* slot_srv = it->second;
            m_SlotData->srvs.erase(it);
            Uint2 rnd;
            do {
                s_RndLock.Lock();
                rnd = s_Rnd.GetRand(0, numeric_limits<Uint2>::max());
                s_RndLock.Unlock();
            }
            while (m_SlotData->srvs.find(rnd) != m_SlotData->srvs.end());
            m_SlotData->srvs[rnd] = slot_srv;
        }
        TSlotSrvsList srvs = m_SlotData->srvs;
        m_SlotData->lock.Unlock();
        ITERATE(TSlotSrvsList, it_srv, srvs) {
            m_SlotSrv = it_srv->second;
            Uint8 next_time = max(m_SlotSrv->next_sync_time,
                                  m_SlotSrv->peer->GetNextSyncTime());
            if (next_time <= CSrvTime::Current().AsUSec()
                &&  (!CNCPeerControl::HasServersForInitSync()
                     ||  !m_SlotSrv->made_initial_sync))
            {
                return &Me::x_DoPeriodicSync;
            }
        }
    }
    else {
        m_SlotData->lock.Unlock();
    }
    return &Me::x_CheckSlotTheirSync;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_CheckSlotTheirSync(void)
{
    if (CTaskServer::IsInShutdown())
        return NULL;

    m_SlotData->lock.Lock();
    ITERATE(TSlotSrvsList, it_srv, m_SlotData->srvs) {
        SSyncSlotSrv* slot_srv = it_srv->second;
        slot_srv->lock.Lock();
        if (slot_srv->sync_started) {
            if (slot_srv->is_passive
                &&  slot_srv->started_cmds == 0
                &&  CSrvTime::Current().AsUSec() - slot_srv->last_active_time
                        >= CNCDistributionConf::GetPeriodicSyncTimeout())
            {
                s_CancelSync(m_SlotData, slot_srv, 0);
            }
        }
        else {
            Uint8 next_time = max(slot_srv->next_sync_time,
                                  slot_srv->peer->GetNextSyncTime());
// calculate next time we need sync
            m_MinNextTime = min(m_MinNextTime, next_time);
        }
        slot_srv->lock.Unlock();
    }
    m_SlotData->lock.Unlock();

    ++m_NextSlotIt;
    SetState(&Me::x_CheckSlotOurSync);
    SetRunnable();
    return NULL;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_FinishScanSlots(void)
{
    Uint8 sync_interval = CNCDistributionConf::GetPeriodicSyncInterval();
    m_ForceInitSync = CNCPeerControl::HasServersForInitSync()  &&  !m_DidSync;
    Uint8 now = CSrvTime::Current().AsUSec();
    m_NeedRehash = now - m_LoopStart >= sync_interval;

    Uint8 wait_time;
    if (m_MinNextTime > now) {
        wait_time = m_MinNextTime - now;
        if (wait_time > sync_interval)
            wait_time = sync_interval;
    }
    else {
        s_RndLock.Lock();
        wait_time = s_Rnd.GetRand(0, 10000);
        s_RndLock.Unlock();
    }

    Uint4 timeout_sec  = Uint4(wait_time / kUSecsPerSecond);
    if (wait_time % kUSecsPerSecond)
        ++timeout_sec;

    SetState(&Me::x_StartScanSlots);
    RunAfter(timeout_sec);
    return NULL;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_DoPeriodicSync(void)
{
    ESyncInitiateResult init_res = s_StartSync(m_SlotData, m_SlotSrv, false);
    if (init_res != eProceedWithEvents)
        return &Me::x_CheckSlotTheirSync;

    m_SrvId = m_SlotSrv->peer->GetSrvId();
    m_Slot = m_SlotData->slot;
    m_Result = eSynOK;
    m_SlotSrv->is_by_blobs = false;
    m_StartedCmds = 0;
    m_FinishSyncCalled = false;
    m_NextTask = eSynNoTask;
    m_StartTime = CSrvTime::Current().AsUSec();

    m_ReadOK = m_ReadERR = 0;
    m_WriteOK = m_WriteERR = 0;
    m_ProlongOK = m_ProlongERR = 0;
    m_DelOK = m_DelERR = 0;

    CreateNewDiagCtx();
    CSrvDiagMsg().StartRequest()
                 .PrintParam("_type", "sync")
                 .PrintParam("srv_id", m_SrvId)
                 .PrintParam("slot", m_Slot)
                 .PrintParam("self_id", CNCDistributionConf::GetSelfID());

    CNCSyncLog::GetLastSyncedRecNo(m_SrvId, m_Slot,
                                   &m_LocalStartRecNo, &m_RemoteStartRecNo);

    CNCActiveHandler* conn = m_SlotSrv->peer->GetBGConn();
    if (!conn) {
        m_Result = eSynNetworkError;
        return &Me::x_FinishSync;
    }

    m_StartedCmds = 1;
    conn->SyncStart(this, m_LocalStartRecNo, m_RemoteStartRecNo);
    return &Me::x_WaitSyncStarted;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_WaitSyncStarted(void)
{
    // wait for sync started
    // see CmdFinished()
    if (m_StartedCmds != 0)
        return NULL;
    if (CTaskServer::IsInShutdown())
        m_Result = eSynAborted;
    if (m_Result != eSynOK)
        return &Me::x_FinishSync;

    m_LocalSyncedRecNo = 0;
    m_RemoteSyncedRecNo = 0;
    // depending on the reply
    if (m_SlotSrv->is_by_blobs)
        return &Me::x_PrepareSyncByBlobs;
    else
        return &Me::x_PrepareSyncByEvents;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_ExecuteSyncCommands(void)
{
    // next command to execute
    x_CalcNextTask();
    // if no more commands
    if (m_NextTask == eSynNeedFinalize)
        return &Me::x_ExecuteFinalize;
    // add to list of active
    // as there are free connections between these two servers
    // these commands will be executed
    if (m_SlotSrv->peer->AddSyncControl(this))
        return &Me::x_WaitForExecutingTasks;

    m_NextTask = eSynNoTask;
    m_Result = eSynNetworkError;
    return &Me::x_FinishSync;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_ExecuteFinalize(void)
{
    if (m_FinishSyncCalled)
        abort();
    if (m_Result == eSynOK) {
        if (m_SlotSrv->peer->FinishSync(this)) {
            m_FinishSyncCalled = true;
            return &Me::x_WaitForExecutingTasks;
        }
        m_Result = eSynNetworkError;
    }
    m_StartedCmds = 0;
    m_NextTask = eSynNoTask;
    return &Me::x_FinishSync;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_WaitForExecutingTasks(void)
{
    ESynTaskType next_task;
    Uint4 started_cmds;
    m_Lock.Lock();
    next_task = m_NextTask;
    started_cmds = m_StartedCmds;
    m_Lock.Unlock();

    if (started_cmds == 0) {
        switch (next_task) {
        case eSynNoTask:
// normally, we come here later
            return &Me::x_FinishSync;
        case eSynNeedFinalize:
// normally, we come here first
            if (!m_FinishSyncCalled)
                return &Me::x_ExecuteFinalize;
            // fall through
        default:
            break;
        }
    }
    return NULL;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_FinishSync(void)
{
    x_CleanRemoteObjects();

    switch (m_Result) {
    case eSynOK:
        CNCBlobStorage::SaveMaxSyncLogRecNo();
        break;
    case eSynAborted:
        GetDiagCtx()->SetRequestStatus(eStatus_SyncAborted);
        break;
    case eSynCrossSynced:
        GetDiagCtx()->SetRequestStatus(eStatus_CrossSync);
        break;
    case eSynServerBusy:
        GetDiagCtx()->SetRequestStatus(eStatus_SyncBusy);
        break;
    case eSynNetworkError:
        GetDiagCtx()->SetRequestStatus(eStatus_BadCmd);
        break;
    }

    CSrvDiagMsg().PrintExtra()
                 .PrintParam("sync", (m_SlotSrv->is_by_blobs? "blobs": "events"))
                 .PrintParam("r_ok", m_ReadOK)
                 .PrintParam("r_err", m_ReadERR)
                 .PrintParam("w_ok", m_WriteOK)
                 .PrintParam("w_err", m_WriteERR)
                 .PrintParam("p_ok", m_ProlongOK)
                 .PrintParam("p_err", m_ProlongERR)
                 .PrintParam("d_ok", m_DelOK)
                 .PrintParam("d_err", m_DelERR);
    CSrvDiagMsg().StopRequest();
    ReleaseDiagCtx();

    if (s_LogFile) {
        Uint8 end_time = CSrvTime::Current().AsUSec();
        Uint8 log_size = CNCSyncLog::GetLogSize();
        fprintf(s_LogFile,
                "%" NCBI_UINT8_FORMAT_SPEC ",%" NCBI_UINT8_FORMAT_SPEC
                ",%u,%" NCBI_UINT8_FORMAT_SPEC ",%" NCBI_UINT8_FORMAT_SPEC
                ",%" NCBI_UINT8_FORMAT_SPEC
                ",%d,%d,%" NCBI_UINT8_FORMAT_SPEC ",%" NCBI_UINT8_FORMAT_SPEC
                ",%" NCBI_UINT8_FORMAT_SPEC ",%" NCBI_UINT8_FORMAT_SPEC
                ",%" NCBI_UINT8_FORMAT_SPEC ",%" NCBI_UINT8_FORMAT_SPEC
                ",%" NCBI_UINT8_FORMAT_SPEC ",%u,%u\n",
                CNCDistributionConf::GetSelfID(), m_SrvId, m_Slot,
                m_StartTime, end_time, end_time - m_StartTime,
                int(m_SlotSrv->is_by_blobs), m_Result, log_size,
                m_ReadOK, m_ReadERR, m_WriteOK, m_WriteERR,
                m_ProlongOK, m_ProlongERR,
                Uint4(CNCPeerControl::sm_TotalCopyRequests.Get()),
                Uint4(CNCPeerControl::sm_CopyReqsRejected.Get()));
        fflush(s_LogFile);
    }

    CMiniMutexGuard g_slot(m_SlotData->lock);
    CMiniMutexGuard g_srv(m_SlotSrv->lock);
    if (m_Result == eSynOK) {
        s_CommitSync(m_SlotData, m_SlotSrv);
    }
    else {
        s_CancelSync(m_SlotData, m_SlotSrv, CNCDistributionConf::GetFailedSyncRetryDelay());
    }
    m_DidSync = m_Result == eSynOK;

    SetState(&Me::x_CheckSlotTheirSync);
    SetRunnable();
    return NULL;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_PrepareSyncByEvents(void)
{
    m_Events2Get.clear();
    m_Events2Send.clear();
    if (CNCSyncLog::GetSyncOperations(m_SrvId, m_Slot,
                                      m_LocalStartRecNo,
                                      m_RemoteStartRecNo,
                                      m_RemoteEvents,
                                      &m_Events2Get,
                                      &m_Events2Send,
                                      &m_LocalSyncedRecNo,
                                      &m_RemoteSyncedRecNo)
        ||  (CNCSyncLog::GetLogSize(m_Slot) == 0  &&  m_SlotSrv->was_blobs_sync))
    {
        m_CurGetEvent = m_Events2Get.begin();
        m_CurSendEvent = m_Events2Send.begin();
        return &Me::x_ExecuteSyncCommands;
    }

    // sync by blob list
    m_SlotSrv->is_by_blobs = true;
    CNCActiveHandler* conn = m_SlotSrv->peer->GetBGConn();
    if (!conn) {
        m_Result = eSynNetworkError;
        return &Me::x_FinishSync;
    }

    // request blob list
    m_StartedCmds = 1;
    conn->SyncBlobsList(this);
    return &Me::x_WaitForBlobList;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_WaitForBlobList(void)
{
    if (m_StartedCmds != 0)
        return NULL;
    if (CTaskServer::IsInShutdown())
        m_Result = eSynAborted;
    if (m_Result != eSynOK)
        return &Me::x_FinishSync;

    return &Me::x_PrepareSyncByBlobs;
}

CNCActiveSyncControl::State
CNCActiveSyncControl::x_PrepareSyncByBlobs(void)
{
    m_LocalSyncedRecNo = CNCSyncLog::GetCurrentRecNo(m_Slot);
    m_RemoteSyncedRecNo = m_RemoteStartRecNo;

    ITERATE(TNCBlobSumList, it, m_LocalBlobs) {
        delete it->second;
    }
    m_LocalBlobs.clear();
    CNCBlobStorage::GetFullBlobsList(m_Slot, m_LocalBlobs);

    m_CurLocalBlob = m_LocalBlobs.begin();
    m_CurRemoteBlob = m_RemoteBlobs.begin();
    return &Me::x_ExecuteSyncCommands;
}

void
CNCActiveSyncControl::x_CleanRemoteObjects(void)
{
    ITERATE(TNCBlobSumList, it, m_RemoteBlobs) {
        delete it->second;
    }
    m_RemoteBlobs.clear();
    ITERATE(TReducedSyncEvents, it, m_RemoteEvents) {
        delete it->second.wr_or_rm_event;
        delete it->second.prolong_event;
    }
    m_RemoteEvents.clear();
}

void
CNCActiveSyncControl::x_CalcNextTask(void)
{
    switch (m_NextTask) {
    case eSynEventSend:
        ++m_CurSendEvent;
        break;
    case eSynEventGet:
        ++m_CurGetEvent;
        break;
    case eSynBlobUpdateOur:
    case eSynBlobUpdatePeer:
        ++m_CurLocalBlob;
        ++m_CurRemoteBlob;
        break;
    case eSynBlobSend:
        ++m_CurLocalBlob;
        break;
    case eSynBlobGet:
        ++m_CurRemoteBlob;
        break;
    case eSynNoTask:
        break;
    case eSynNeedFinalize:
        m_NextTask = eSynNoTask;
        return;
    }

    if (m_SlotData->clean_required  &&  m_Result != eSynNetworkError)
        m_Result = eSynAborted;

    if (m_Result == eSynNetworkError  ||  m_Result == eSynAborted)
        m_NextTask = eSynNeedFinalize;
    else if (!m_SlotSrv->is_by_blobs) {
        if (m_CurSendEvent != m_Events2Send.end())
            m_NextTask = eSynEventSend;
        else if (m_CurGetEvent != m_Events2Get.end())
            m_NextTask = eSynEventGet;
        else
            m_NextTask = eSynNeedFinalize;
    }
    else {
sync_next_key:
        if (m_CurLocalBlob != m_LocalBlobs.end()
            &&  m_CurRemoteBlob != m_RemoteBlobs.end())
        {
            if (m_CurLocalBlob->first == m_CurRemoteBlob->first) {
                if (m_CurLocalBlob->second->isEqual(*m_CurRemoteBlob->second)) {
                    // Equivalent blobs, skip them.
                    ++m_CurLocalBlob;
                    ++m_CurRemoteBlob;
                    goto sync_next_key;
                }

                // The same blob key. Test which one is newer.
                if (m_CurLocalBlob->second->isOlder(*m_CurRemoteBlob->second))
                    m_NextTask = eSynBlobUpdateOur;
                else
                    m_NextTask = eSynBlobUpdatePeer;
            }
            else if (m_CurLocalBlob->first < m_CurRemoteBlob->first)
                m_NextTask = eSynBlobSend;
            else
                m_NextTask = eSynBlobGet;
        }
        // Process the tails of the lists
        else if (m_CurLocalBlob != m_LocalBlobs.end())
            m_NextTask = eSynBlobSend;
        else if (m_CurRemoteBlob != m_RemoteBlobs.end())
            m_NextTask = eSynBlobGet;
        else
            m_NextTask = eSynNeedFinalize;
    }
}

void
CNCActiveSyncControl::x_DoEventSend(const SSyncTaskInfo& task_info,
                                    CNCActiveHandler* conn)
{
    SNCSyncEvent* event = *task_info.send_evt;
    switch (event->event_type) {
    case eSyncWrite:
        conn->SyncSend(this, event);
        break;
    case eSyncProlong:
        conn->SyncProlongPeer(this, event);
        break;
    }
}

void
CNCActiveSyncControl::x_DoEventGet(const SSyncTaskInfo& task_info,
                                   CNCActiveHandler* conn)
{
    SNCSyncEvent* event = *task_info.get_evt;
    switch (event->event_type) {
    case eSyncWrite:
        conn->SyncRead(this, event);
        break;
    case eSyncProlong:
        conn->SyncProlongOur(this, event);
        break;
    }
}

void
CNCActiveSyncControl::x_DoBlobUpdateOur(const SSyncTaskInfo& task_info,
                                        CNCActiveHandler* conn)
{
    string key(task_info.remote_blob->first);
    SNCBlobSummary* local_blob = task_info.local_blob->second;
    SNCBlobSummary* remote_blob = task_info.remote_blob->second;
    if (local_blob->isSameData(*remote_blob))
        conn->SyncProlongOur(this, key, *remote_blob);
    else
        conn->SyncRead(this, key, remote_blob->create_time);
}

void
CNCActiveSyncControl::x_DoBlobUpdatePeer(const SSyncTaskInfo& task_info,
                                         CNCActiveHandler* conn)
{
    string key(task_info.remote_blob->first);
    SNCBlobSummary* local_blob = task_info.local_blob->second;
    SNCBlobSummary* remote_blob = task_info.remote_blob->second;
    if (local_blob->isSameData(*remote_blob))
        conn->SyncProlongPeer(this, key, *local_blob);
    else
        conn->SyncSend(this, key);
}

void
CNCActiveSyncControl::x_DoBlobSend(const SSyncTaskInfo& task_info,
                                   CNCActiveHandler* conn)
{
    string key(task_info.local_blob->first);
    conn->SyncSend(this, key);
}

void
CNCActiveSyncControl::x_DoBlobGet(const SSyncTaskInfo& task_info,
                                  CNCActiveHandler* conn)
{
    string key(task_info.remote_blob->first);
    Uint8 create_time = task_info.remote_blob->second->create_time;
    conn->SyncRead(this, key, create_time);
}

void
CNCActiveSyncControl::x_DoFinalize(CNCActiveHandler* conn)
{
    if (m_Result == eSynOK) {
        CNCSyncLog::SetLastSyncRecNo(m_SrvId, m_Slot,
                                     m_LocalSyncedRecNo, m_RemoteSyncedRecNo);
        conn->SyncCommit(this, m_LocalSyncedRecNo, m_RemoteSyncedRecNo);
    }
    else if (m_Result == eSynAborted) {
        conn->SyncCancel(this);
    }
    else
        abort();
}

bool
CNCActiveSyncControl::GetNextTask(SSyncTaskInfo& task_info)
{
    m_Lock.Lock();
    if (m_NextTask == eSynNoTask)
        abort();
    task_info.task_type = m_NextTask;
    task_info.get_evt = m_CurGetEvent;
    task_info.send_evt = m_CurSendEvent;
    task_info.local_blob = m_CurLocalBlob;
    task_info.remote_blob = m_CurRemoteBlob;
    ++m_StartedCmds;
    if (m_StartedCmds == 0)
        abort();
    if (m_NextTask != eSynNeedFinalize)
        ++m_SlotSrv->cnt_sync_ops;
    x_CalcNextTask();
    bool has_more = m_NextTask != eSynNeedFinalize  &&  m_NextTask != eSynNoTask;
    m_Lock.Unlock();

    return has_more;
}

void
CNCActiveSyncControl::ExecuteSyncTask(const SSyncTaskInfo& task_info,
                                      CNCActiveHandler* conn)
{
    switch (task_info.task_type) {
    case eSynEventSend:
        x_DoEventSend(task_info, conn);
        break;
    case eSynEventGet:
        x_DoEventGet(task_info, conn);
        break;
    case eSynBlobUpdateOur:
        x_DoBlobUpdateOur(task_info, conn);
        break;
    case eSynBlobUpdatePeer:
        x_DoBlobUpdatePeer(task_info, conn);
        break;
    case eSynBlobSend:
        x_DoBlobSend(task_info, conn);
        break;
    case eSynBlobGet:
        x_DoBlobGet(task_info, conn);
        break;
    case eSynNeedFinalize:
        x_DoFinalize(conn);
        break;
    default:
        abort();
    }
}

void
CNCActiveSyncControl::CmdFinished(ESyncResult res, ESynActionType action, CNCActiveHandler* conn)
{
    m_Lock.Lock();
    if (res == eSynOK) {
        switch (action) {
        case eSynActionRead:
            ++m_ReadOK;
            break;
        case eSynActionWrite:
            ++m_WriteOK;
            break;
        case eSynActionProlong:
            ++m_ProlongOK;
            break;
        case eSynActionRemove:
            ++m_DelOK;
            break;
        case eSynActionNone:
            break;
        }
    }
    else {
        switch (action) {
        case eSynActionRead:
            ++m_ReadERR;
            break;
        case eSynActionWrite:
            ++m_WriteERR;
            break;
        case eSynActionProlong:
            ++m_ProlongERR;
            break;
        case eSynActionRemove:
            ++m_DelERR;
            break;
        case eSynActionNone:
            break;
        }
    }

    if (res == eSynAborted  &&  m_Result != eSynNetworkError)
        m_Result = eSynAborted;
    else if (res != eSynOK)
        m_Result = res;

    if (m_StartedCmds == 0)
        abort();
    if (--m_StartedCmds == 0
        &&  (m_NextTask == eSynNeedFinalize  ||  m_NextTask == eSynNoTask))
    {
        SetRunnable();
    }
    m_Lock.Unlock();
}

END_NCBI_SCOPE
