/*  $Id: job.cpp 389701 2013-02-20 15:59:00Z satskyse $
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
* Authors:  Victor Joukov
*
* File Description:
*   NetSchedule job
*
*/

#include <ncbi_pch.hpp>

#include "job.hpp"
#include "ns_queue.hpp"
#include "ns_handler.hpp"
#include "ns_command_arguments.hpp"
#include "ns_affinity.hpp"
#include "ns_group.hpp"


BEGIN_NCBI_SCOPE


//////////////////////////////////////////////////////////////////////////
// CJobEvent implementation

static string   s_EventAsString[] = {
    "Submit",           // eSubmit
    "BatchSubmit",      // eBatchSubmit
    "Request",          // eRequest
    "Done",             // eDone
    "Return",           // eReturn
    "Fail",             // eFail
    "Read",             // eRead
    "ReadFail",         // eReadFail
    "ReadDone",         // eReadDone
    "ReadRollback",     // eReadRollback
    "Clear",            // eClear
    "Cancel",           // eCancel
    "Timeout",          // eTimeout
    "ReadTimeout",      // eReadTimeout
    "SessionChanged",   // eSessionChanged
    "NSSubmitRollback", // eNSSubmitRollback
    "NSGetRollback",    // eNSGetRollback
    "NSReadRollback"    // eNSReadRollback
};


string CJobEvent::EventToString(EJobEvent  event)
{
    if (event < eSubmit || event > eNSReadRollback)
        return "UNKNOWN";

    return s_EventAsString[ event ];
}


CJobEvent::CJobEvent() :
    m_Dirty(false),
    m_Status(CNetScheduleAPI::eJobNotFound),
    m_Event(eUnknown),
    m_Timestamp(0),
    m_NodeAddr(0),
    m_RetCode(0)
{}


void CJobEvent::SetClientNode(const string &  client_node)
{
    m_Dirty = true;
    m_ClientNode = client_node.substr(0, kMaxWorkerNodeIdSize);
}


void CJobEvent::SetClientSession(const string &  cliet_session)
{
    m_Dirty = true;
    m_ClientSession = cliet_session.substr(0, kMaxWorkerNodeIdSize);
}


static const string     kTruncatedTail = " MSG_TRUNCATED";
void CJobEvent::SetErrorMsg(const string &  msg)
{
    m_Dirty = true;
    if (msg.size() < kMaxWorkerNodeErrMsgSize)
        m_ErrorMsg = msg;
    else
        m_ErrorMsg = msg.substr(0, kMaxWorkerNodeErrMsgSize -
                                   kTruncatedTail.size() - 1) + kTruncatedTail;
}



time_t  GetJobExpirationTime(time_t      last_touch,
                             TJobStatus  status,
                             time_t      job_submit_time,
                             time_t      job_timeout,
                             time_t      job_run_timeout,
                             time_t      queue_timeout,
                             time_t      queue_run_timeout,
                             time_t      queue_pending_timeout,
                             time_t      event_time)
{
    time_t      last_update = event_time;
    if (last_update == 0)
        last_update = last_touch;

    if (status == CNetScheduleAPI::eRunning ||
        status == CNetScheduleAPI::eReading) {
        if (job_run_timeout != 0)
            return last_update + job_run_timeout;
        return last_update + queue_run_timeout;
    }

    if (status == CNetScheduleAPI::ePending) {
        time_t      regular_expiration = last_update +
                                         queue_timeout;
        if (job_timeout != 0)
            regular_expiration = last_update + job_timeout;
        time_t      pending_expiration = job_submit_time +
                                         queue_pending_timeout;

        if (regular_expiration < pending_expiration)
            return regular_expiration;
        return pending_expiration;
    }

    if (job_timeout != 0)
        return last_update + job_timeout;
    return last_update + queue_timeout;
}




//////////////////////////////////////////////////////////////////////////
// CJob implementation

CJob::CJob() :
    m_New(true), m_Deleted(false), m_Dirty(0),
    m_Id(0),
    m_Passport(0),
    m_Status(CNetScheduleAPI::ePending),
    m_Timeout(0),
    m_RunTimeout(0),
    m_SubmNotifPort(0),
    m_SubmNotifTimeout(0),
    m_ListenerNotifAddress(0),
    m_ListenerNotifPort(0),
    m_ListenerNotifAbsTime(0),
    m_RunCount(0),
    m_ReadCount(0),
    m_AffinityId(0),
    m_Mask(0),
    m_GroupId(0),
    m_LastTouch(0)
{}


CJob::CJob(const SNSCommandArguments &  request) :
    m_New(true), m_Deleted(false), m_Dirty(fJobPart),
    m_Id(0),
    m_Passport(0),
    m_Status(CNetScheduleAPI::ePending),
    m_Timeout(0),
    m_RunTimeout(0),
    m_SubmNotifPort(request.port),
    m_SubmNotifTimeout(request.timeout),
    m_ListenerNotifAddress(0),
    m_ListenerNotifPort(0),
    m_ListenerNotifAbsTime(0),
    m_RunCount(0),
    m_ReadCount(0),
    m_ProgressMsg(""),
    m_AffinityId(0),
    m_Mask(request.job_mask),
    m_GroupId(0),
    m_LastTouch(0),
    m_ClientIP(request.ip),
    m_ClientSID(request.sid),
    m_Output("")
{
    SetInput(NStr::ParseEscapes(request.input));
}


string CJob::GetErrorMsg() const
{
    if (m_Events.empty())
        return "";
    return GetLastEvent()->GetErrorMsg();
}


int    CJob::GetRetCode() const
{
    if (m_Events.empty())
        return ~0;
    return GetLastEvent()->GetRetCode();
}


void CJob::SetInput(const string &  input)
{
    bool    was_overflow = m_Input.size() > kNetScheduleSplitSize;
    bool    is_overflow = input.size() > kNetScheduleSplitSize;

    if (is_overflow) {
        m_Dirty |= fJobInfoPart;
        if (!was_overflow)
            m_Dirty |= fJobPart;
    } else {
        m_Dirty |= fJobPart;
        if (was_overflow)
            m_Dirty |= fJobInfoPart;
    }
    m_Input = input;
}


void CJob::SetOutput(const string &  output)
{
    if (m_Output.empty() && output.empty())
        return;     // It might be the most common case for failed jobs

    bool    was_overflow = m_Output.size() > kNetScheduleSplitSize;
    bool    is_overflow = output.size() > kNetScheduleSplitSize;

    if (is_overflow) {
        m_Dirty |= fJobInfoPart;
        if (!was_overflow)
            m_Dirty |= fJobPart;
    } else {
        m_Dirty |= fJobPart;
        if (was_overflow)
            m_Dirty |= fJobInfoPart;
    }
    m_Output = output;
}


CJobEvent &  CJob::AppendEvent()
{
    m_Events.push_back(CJobEvent());
    m_Dirty |= fEventsPart;
    return m_Events[m_Events.size()-1];
}


const CJobEvent *  CJob::GetLastEvent() const
{
    // The first event is attached when a job is submitted, so
    // there is no way to have the events empty
    return &(m_Events[m_Events.size()-1]);
}


CJobEvent *  CJob::GetLastEvent()
{
    // The first event is attached when a job is submitted, so
    // there is no way to have the events empty
    m_Dirty |= fEventsPart;
    return &(m_Events[m_Events.size()-1]);
}


CJob::EAuthTokenCompareResult
CJob::CompareAuthToken(const string &  auth_token) const
{
    vector<string>      parts;

    NStr::Tokenize(auth_token, "_", parts);
    if (parts.size() < 2)
        return eInvalidTokenFormat;

    try {
        if (NStr::StringToUInt(parts[0]) != m_Passport)
            return eNoMatch;
        if (NStr::StringToSizet(parts[1]) != m_Events.size())
            return ePassportOnlyMatch;
    } catch (...) {
        // Cannot convert the value
        return eInvalidTokenFormat;
    }
    return eCompleteMatch;
}


void CJob::Delete()
{
    m_Deleted = true;
    m_Dirty = 0;
}


CJob::EJobFetchResult CJob::x_Fetch(CQueue* queue)
{
    SJobDB &        job_db      = queue->m_QueueDbBlock->job_db;
    SJobInfoDB &    job_info_db = queue->m_QueueDbBlock->job_info_db;
    SEventsDB &     events_db   = queue->m_QueueDbBlock->events_db;

    m_Id            = job_db.id;

    m_Passport      = job_db.passport;
    m_Status        = TJobStatus(int(job_db.status));
    m_Timeout       = job_db.timeout;
    m_RunTimeout    = job_db.run_timeout;

    m_SubmNotifPort    = job_db.subm_notif_port;
    m_SubmNotifTimeout = job_db.subm_notif_timeout;

    m_ListenerNotifAddress = job_db.listener_notif_addr;
    m_ListenerNotifPort    = job_db.listener_notif_port;
    m_ListenerNotifAbsTime = job_db.listener_notif_abstime;

    m_RunCount      = job_db.run_counter;
    m_ReadCount     = job_db.read_counter;
    m_AffinityId    = job_db.aff_id;
    m_Mask          = job_db.mask;
    m_GroupId       = job_db.group_id;
    m_LastTouch     = job_db.last_touch;

    m_ClientIP      = job_db.client_ip;
    m_ClientSID     = job_db.client_sid;

    if (!(char) job_db.input_overflow)
        job_db.input.ToString(m_Input);
    if (!(char) job_db.output_overflow)
        job_db.output.ToString(m_Output);
    m_ProgressMsg = job_db.progress_msg;

    // JobInfoDB, can be optimized by adding lazy load
    EBDB_ErrCode        res;

    if ((char) job_db.input_overflow ||
        (char) job_db.output_overflow) {
        job_info_db.id = m_Id;
        res = job_info_db.Fetch();

        if (res != eBDB_Ok) {
            if (res != eBDB_NotFound) {
                ERR_POST("Error reading the job input/output DB, job_key " <<
                         queue->MakeKey(m_Id));
                return eJF_DBErr;
            }
            ERR_POST("DB inconsistency detected. Long input/output "
                     "expected but no record found in the DB. job_key " <<
                     queue->MakeKey(m_Id));
            return eJF_DBErr;
        }

        if ((char) job_db.input_overflow)
            job_info_db.input.ToString(m_Input);
        if ((char) job_db.output_overflow)
            job_info_db.output.ToString(m_Output);
    }

    // EventsDB
    m_Events.clear();
    CBDB_FileCursor &       cur = queue->GetEventsCursor();
    CBDB_CursorGuard        cg(cur);

    cur.SetCondition(CBDB_FileCursor::eEQ);
    cur.From << m_Id;

    for (unsigned n = 0; (res = cur.Fetch()) == eBDB_Ok; ++n) {
        CJobEvent &       event = AppendEvent();

        event.m_Status     = TJobStatus(int(events_db.status));
        event.m_Event      = CJobEvent::EJobEvent(int(events_db.event));
        event.m_Timestamp  = events_db.timestamp;
        event.m_NodeAddr   = events_db.node_addr;
        event.m_RetCode    = events_db.ret_code;
        events_db.client_node.ToString(event.m_ClientNode);
        events_db.client_session.ToString(event.m_ClientSession);
        events_db.err_msg.ToString(event.m_ErrorMsg);
        event.m_Dirty = false;
    }
    if (res != eBDB_NotFound) {
        ERR_POST("Error reading queue events db, job_key " <<
                 queue->MakeKey(m_Id));
        return eJF_DBErr;
    }

    m_New   = false;
    m_Dirty = 0;
    return eJF_Ok;
}


CJob::EJobFetchResult  CJob::Fetch(CQueue *  queue, unsigned  id)
{
    SJobDB &        job_db = queue->m_QueueDbBlock->job_db;

    job_db.id = id;

    EBDB_ErrCode    res = job_db.Fetch();
    if (res != eBDB_Ok) {
        if (res != eBDB_NotFound) {
            ERR_POST("Error reading queue job db, job_key " <<
                     queue->MakeKey(id));
            return eJF_DBErr;
        }
        return eJF_NotFound;
    }
    return x_Fetch(queue);
}


bool CJob::Flush(CQueue* queue)
{
    if (m_Deleted) {
        queue->EraseJob(m_Id);
        return true;
    }

    if (!m_Dirty)
        return true;

    SJobDB &        job_db      = queue->m_QueueDbBlock->job_db;
    SJobInfoDB &    job_info_db = queue->m_QueueDbBlock->job_info_db;
    SEventsDB &     events_db   = queue->m_QueueDbBlock->events_db;

    bool            flush_job = (m_Dirty & fJobPart) || m_New;
    bool            input_overflow = m_Input.size() > kNetScheduleSplitSize;
    bool            output_overflow = m_Output.size() > kNetScheduleSplitSize;

    // JobDB (QueueDB)
    if (flush_job) {
        job_db.id             = m_Id;
        job_db.passport       = m_Passport;
        job_db.status         = int(m_Status);

        job_db.timeout        = m_Timeout;
        job_db.run_timeout    = m_RunTimeout;

        job_db.subm_notif_port    = m_SubmNotifPort;
        job_db.subm_notif_timeout = m_SubmNotifTimeout;

        job_db.listener_notif_addr    = m_ListenerNotifAddress;
        job_db.listener_notif_port    = m_ListenerNotifPort;
        job_db.listener_notif_abstime = m_ListenerNotifAbsTime;

        job_db.run_counter    = m_RunCount;
        job_db.read_counter   = m_ReadCount;
        job_db.aff_id         = m_AffinityId;
        job_db.mask           = m_Mask;
        job_db.group_id       = m_GroupId;
        job_db.last_touch     = m_LastTouch;

        job_db.client_ip      = m_ClientIP;
        job_db.client_sid     = m_ClientSID;

        if (!input_overflow) {
            job_db.input_overflow = 0;
            job_db.input = m_Input;
        } else {
            job_db.input_overflow = 1;
            job_db.input = "";
        }
        if (!output_overflow) {
            job_db.output_overflow = 0;
            job_db.output = m_Output;
        } else {
            job_db.output_overflow = 1;
            job_db.output = "";
        }
        job_db.progress_msg = m_ProgressMsg;
    }

    // JobInfoDB
    bool    nonempty_job_info = input_overflow || output_overflow;
    if (m_Dirty & fJobInfoPart) {
        job_info_db.id = m_Id;
        if (input_overflow)
            job_info_db.input = m_Input;
        else
            job_info_db.input = "";
        if (output_overflow)
            job_info_db.output = m_Output;
        else
            job_info_db.output = "";
    }
    if (flush_job)
        job_db.UpdateInsert();
    if (m_Dirty & fJobInfoPart) {
        if (nonempty_job_info)
            job_info_db.UpdateInsert();
        else
            job_info_db.Delete(CBDB_File::eIgnoreError);
    }

    // EventsDB
    unsigned n = 0;
    NON_CONST_ITERATE(vector<CJobEvent>, it, m_Events) {
        CJobEvent &         event = *it;

        if (event.m_Dirty) {
            events_db.id             = m_Id;
            events_db.event_id       = n;
            events_db.status         = int(event.m_Status);
            events_db.event          = int(event.m_Event);
            events_db.timestamp      = event.m_Timestamp;
            events_db.node_addr      = event.m_NodeAddr;
            events_db.ret_code       = event.m_RetCode;
            events_db.client_node    = event.m_ClientNode;
            events_db.client_session = event.m_ClientSession;
            events_db.err_msg        = event.m_ErrorMsg;
            events_db.UpdateInsert();
            event.m_Dirty = false;
        }
        ++n;
    }

    m_New = false;
    m_Dirty = 0;

    return true;
}


bool CJob::ShouldNotifySubmitter(time_t current_time) const
{
    // The very first event is always a submit
    if (m_SubmNotifTimeout && m_SubmNotifPort)
        return m_Events[0].m_Timestamp + m_SubmNotifTimeout >= current_time;
    return false;
}


bool CJob::ShouldNotifyListener(time_t          current_time,
                                TNSBitVector &  jobs_to_notify) const
{
    if (m_ListenerNotifAbsTime != 0 &&
        m_ListenerNotifAddress != 0 &&
        m_ListenerNotifPort != 0) {
        if (m_ListenerNotifAbsTime >= current_time) {
            // Still need to notify
            return true;
        }

        // Notifications timed out
        jobs_to_notify.set_bit(m_Id, false);
        return false;
    }

    // There was no need to notify at all
    return false;
}


// Used to DUMP a job
string CJob::Print(const CQueue &               queue,
                   const CNSAffinityRegistry &  aff_registry,
                   const CNSGroupsRegistry &    group_registry) const
{
    time_t      timeout = m_Timeout;
    time_t      run_timeout = m_RunTimeout;
    time_t      pending_timeout = queue.GetPendingTimeout();
    string      result;

    result.reserve(2048);   // Voluntary; must be enough for most of the cases

    if (m_Timeout == 0)
        timeout = queue.GetTimeout();
    if (m_RunTimeout == 0)
        run_timeout = queue.GetRunTimeout();

    CTime       exp_time(GetExpirationTime(queue.GetTimeout(),
                                           queue.GetRunTimeout(),
                                           pending_timeout,
                                           0));
    exp_time.ToLocalTime();
    CTime       touch_time(m_LastTouch);
    touch_time.ToLocalTime();


    result = "OK:id: " + NStr::IntToString(m_Id) + "\n"
             "OK:key: " + queue.MakeKey(m_Id) + "\n"
             "OK:status: " + CNetScheduleAPI::StatusToString(m_Status) + "\n"
             "OK:last_touch: " + touch_time.AsString() +"\n";

    if (m_Status == CNetScheduleAPI::eRunning ||
        m_Status == CNetScheduleAPI::eReading)
        result += "OK:erase_time: n/a (timeout: " +
                  NStr::ULongToString(timeout) +
                  " sec, pending timeout: " +
                  NStr::ULongToString(pending_timeout) + " sec)\n";
    else
        result += "OK:erase_time: " + exp_time.AsString() +
                  " (timeout: " +
                  NStr::ULongToString(timeout) +
                  " sec, pending timeout: " +
                  NStr::ULongToString(pending_timeout) + " sec)\n";

    if (m_Status != CNetScheduleAPI::eRunning &&
        m_Status != CNetScheduleAPI::eReading) {
        result += "OK:run_expiration: n/a (timeout: " +
                  NStr::ULongToString(run_timeout) + " sec)\n"
                  "OK:read_expiration: n/a (timeout: " +
                  NStr::ULongToString(run_timeout) + " sec)\n";
    } else {
        if (m_Status == CNetScheduleAPI::eRunning) {
            result += "OK:run_expiration: " + exp_time.AsString() +
                      " (timeout: " +
                      NStr::ULongToString(run_timeout) + " sec)\n"
                      "OK:read_expiration: n/a (timeout: " +
                      NStr::ULongToString(run_timeout) + " sec)\n";
        } else {
            // Reading job
            result += "OK:run_expiration: n/a (timeout: " +
                      NStr::ULongToString(run_timeout) + " sec)\n"
                      "OK:read_expiration: " + exp_time.AsString() +
                      " (timeout: " +
                      NStr::ULongToString(run_timeout) + " sec)\n";
        }
    }

    if (m_SubmNotifPort != 0)
        result += "OK:subm_notif_port: " +
                  NStr::IntToString(m_SubmNotifPort) + "\n";
    else
        result += "OK:subm_notif_port: n/a\n";

    if (m_SubmNotifTimeout != 0) {
        time_t      submit_timestamp = m_Events[0].m_Timestamp;
        CTime       subm_exp_time(submit_timestamp + m_SubmNotifTimeout);

        subm_exp_time.ToLocalTime();
        result += "OK:subm_notif_expiration: " +
                  subm_exp_time.AsString() + " (timeout: " +
                  NStr::IntToString(m_SubmNotifTimeout) + " sec)\n";
    }
    else
        result += "OK:subm_notif_expiration: n/a\n";

    if (m_ListenerNotifAddress == 0 || m_ListenerNotifPort == 0)
        result += "OK:listener_notif: n/a\n";
    else
        result += "OK:listener_notif: " +
                  CSocketAPI::gethostbyaddr(m_ListenerNotifAddress) + ":" +
                  NStr::IntToString(m_ListenerNotifPort) + "\n";

    if (m_ListenerNotifAbsTime != 0) {
        CTime       listener_exp_time(m_ListenerNotifAbsTime);

        listener_exp_time.ToLocalTime();
        result += "OK:listener_notif_expiration: " +
                  listener_exp_time.AsString() + "\n";
    }
    else
        result += "OK:listener_notif_expiration: n/a\n";


    // Print detailed information about the job events
    int                         event = 1;

    ITERATE(vector<CJobEvent>, it, m_Events) {
        unsigned int    addr = it->GetNodeAddr();

        result += "OK:event" + NStr::IntToString(event++) + ": "
                  "client=";
        if (addr == 0)
            result += "ns ";
        else
            result += CSocketAPI::gethostbyaddr(addr) + " ";

        result += "event=" + CJobEvent::EventToString(it->GetEvent()) + " "
                  "status=" +
                  CNetScheduleAPI::StatusToString(it->GetStatus()) + " "
                  "ret_code=" + NStr::IntToString(it->GetRetCode()) + " ";

        // Time part
        result += "timestamp=";
        unsigned int    start = it->GetTimestamp();
        if (start == 0) {
            result += "n/a ";
        }
        else {
            CTime   startTime(start);
            startTime.ToLocalTime();
            result += "'" + startTime.AsString() + "' ";
        }

        // The rest
        result += "node='" + it->GetClientNode() + "' "
                  "session='" + it->GetClientSession() + "' "
                  "err_msg=" + it->GetQuotedErrorMsg() + "\n";
    }

    result += "OK:run_counter: " + NStr::IntToString(m_RunCount) + "\n"
              "OK:read_counter: " + NStr::IntToString(m_ReadCount) + "\n";

    if (m_AffinityId != 0)
        result += "OK:affinity: " + NStr::IntToString(m_AffinityId) + " ('" +
                  NStr::PrintableString(aff_registry.GetTokenByID(m_AffinityId)) +
                  "')\n";
    else
        result += "OK:affinity: n/a\n";

    if (m_GroupId != 0)
        result += "OK:group: " + NStr::IntToString(m_GroupId) + " ('" +
            NStr::PrintableString(group_registry.ResolveGroup(m_GroupId)) +
            "')\n";
    else
        result += "OK:group: n/a\n";

    result += "OK:mask: " + NStr::IntToString(m_Mask) + "\n"
              "OK:input: '" + NStr::PrintableString(m_Input) + "'\n"
              "OK:output: '" + NStr::PrintableString(m_Output) + "'\n"
              "OK:progress_msg: '" + m_ProgressMsg + "'\n"
              "OK:remote_client_sid: " + m_ClientSID + "\n"
              "OK:remote_client_ip: " + m_ClientIP + "\n";

    return result;
}

END_NCBI_SCOPE

