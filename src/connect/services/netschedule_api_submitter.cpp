/*  $Id: netschedule_api_submitter.cpp 390466 2013-02-27 20:17:57Z kazimird $
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
 * Author:  Anatoliy Kuznetsov, Maxim Didenko, Dmitry Kazimirov
 *
 * File Description:
 *   Implementation of NetSchedule API.
 *
 */

#include <ncbi_pch.hpp>

#include "netschedule_api_impl.hpp"

#include <connect/services/netschedule_api.hpp>
#include <connect/services/util.hpp>

#include <stdio.h>

BEGIN_NCBI_SCOPE

#define FORCED_SST_INTERVAL_SEC 0
#define FORCED_SST_INTERVAL_NANOSEC 500 * 1000 * 1000

//////////////////////////////////////////////////////////////////////////////
static void s_SerializeJob(string& cmd, const CNetScheduleJob& job,
    unsigned short udp_port, unsigned wait_time)
{
    cmd.push_back('"');
    cmd.append(NStr::PrintableString(job.input));
    cmd.push_back('"');

    if (udp_port != 0) {
        cmd.append(" port=");
        cmd.append(NStr::UIntToString(udp_port));
        cmd.append(" timeout=");
        cmd.append(NStr::UIntToString(wait_time));
    }

    if (!job.affinity.empty()) {
        SNetScheduleAPIImpl::VerifyAffinityAlphabet(job.affinity);
        cmd.append(" aff=");
        cmd.append(job.affinity);
    }

    if (job.mask != CNetScheduleAPI::eEmptyMask) {
        cmd.append(" msk=");
        cmd.append(NStr::UIntToString(job.mask));
    }
}

inline
void static s_CheckInputSize(const string& input, size_t max_input_size)
{
    if (input.length() >  max_input_size) {
        NCBI_THROW(CNetScheduleException, eDataTooLong,
                   "Input data too long.");
    }
}

string CNetScheduleSubmitter::SubmitJob(CNetScheduleJob& job)
{
    return m_Impl->SubmitJobImpl(job, 0, 0);
}

string SNetScheduleSubmitterImpl::SubmitJobImpl(CNetScheduleJob& job,
        unsigned short udp_port, unsigned wait_time, CNetServer* server)
{
    size_t max_input_size = m_API->GetServerParams().max_input_size;
    s_CheckInputSize(job.input, max_input_size);

    string cmd = "SUBMIT ";

    s_SerializeJob(cmd, job, udp_port, wait_time);

    g_AppendClientIPAndSessionID(cmd);

    if (!job.group.empty()) {
        SNetScheduleAPIImpl::VerifyJobGroupAlphabet(job.group);
        cmd.append(" group=");
        cmd.append(job.group);
    }

    CNetServer::SExecResult exec_result(
            m_API->m_Service.FindServerAndExec(cmd));

    if ((job.job_id = exec_result.response).empty()) {
        NCBI_THROW(CNetServiceException, eCommunicationError,
                   "Invalid server response. Empty key.");
    }

    if (server != NULL)
        *server = exec_result.conn->m_Server;

    return job.job_id;
}

void CNetScheduleSubmitter::SubmitJobBatch(vector<CNetScheduleJob>& jobs,
        const string& job_group)
{
    // verify the input data
    size_t max_input_size = m_Impl->m_API->GetServerParams().max_input_size;

    ITERATE(vector<CNetScheduleJob>, it, jobs) {
        const string& input = it->input;
        s_CheckInputSize(input, max_input_size);
    }

    // Batch submit command.
    string cmd = "BSUB";

    g_AppendClientIPAndSessionID(cmd);

    if (!job_group.empty()) {
        SNetScheduleAPIImpl::VerifyJobGroupAlphabet(job_group);
        cmd.append(" group=");
        cmd.append(job_group);
    }

    CNetServer::SExecResult exec_result(
        m_Impl->m_API->m_Service.FindServerAndExec(cmd));

    cmd.reserve(max_input_size * 6);
    string host;
    unsigned short port = 0;
    for (unsigned i = 0; i < jobs.size(); ) {

        // Batch size should be reasonable not to trigger network timeout
        const unsigned kMax_Batch = 10000;

        size_t batch_size = jobs.size() - i;
        if (batch_size > kMax_Batch) {
            batch_size = kMax_Batch;
        }

        cmd.erase();
        cmd = "BTCH ";
        cmd.append(NStr::UIntToString((unsigned) batch_size));

        exec_result.conn->WriteLine(cmd);

        unsigned batch_start = i;
        for (unsigned j = 0; j < batch_size; ++j,++i) {
            cmd.erase();
            s_SerializeJob(cmd, jobs[i], 0, 0);

            exec_result.conn->WriteLine(cmd);
        }

        string resp = exec_result.conn.Exec("ENDB");

        if (resp.empty()) {
            NCBI_THROW(CNetServiceException, eProtocolError,
                    "Invalid server response. Empty key.");
        }

        // parse the batch answer
        // FORMAT:
        //  first_job_id host port

        {{
        const char* s = resp.c_str();
        unsigned first_job_id = ::atoi(s);

        if (host.empty()) {
            for (; *s != ' '; ++s) {
                if (*s == 0) {
                    NCBI_THROW(CNetServiceException, eProtocolError,
                            "Invalid server response. Batch answer format.");
                }
            }
            ++s;
            if (*s == 0) {
                NCBI_THROW(CNetServiceException, eProtocolError,
                        "Invalid server response. Batch answer format.");
            }
            for (; *s != ' '; ++s) {
                if (*s == 0) {
                    NCBI_THROW(CNetServiceException, eProtocolError,
                            "Invalid server response. Batch answer format.");
                }
                host.push_back(*s);
            }
            ++s;
            if (*s == 0) {
                NCBI_THROW(CNetServiceException, eProtocolError,
                        "Invalid server response. Batch answer format.");
            }

            port = atoi(s);
            if (port == 0) {
                NCBI_THROW(CNetServiceException, eProtocolError,
                        "Invalid server response. Port=0.");
            }
        }

        // assign job ids, protocol guarantees all jobs in batch will
        // receive sequential numbers, so server sends only first job id
        //
        CNetScheduleKeyGenerator key_gen(host, port, m_Impl->m_API->m_Queue);
        for (unsigned j = 0; j < batch_size; ++j) {
            key_gen.Generate(&jobs[batch_start].job_id, first_job_id);
            ++first_job_id;
            ++batch_start;
        }

        }}


    } // for

    exec_result.conn.Exec("ENDS");
}

class CReadCmdExecutor : public INetServerFinder
{
public:
    CReadCmdExecutor(const string& cmd,
            string& job_id,
            string& auth_token,
            CNetScheduleAPI::EJobStatus& job_status) :
        m_Cmd(cmd),
        m_JobId(job_id),
        m_AuthToken(auth_token),
        m_JobStatus(job_status)
    {
    }

    virtual bool Consider(CNetServer server);

private:
    string m_Cmd;
    string& m_JobId;
    string& m_AuthToken;
    CNetScheduleAPI::EJobStatus& m_JobStatus;
};

bool CReadCmdExecutor::Consider(CNetServer server)
{
    string response = server.ExecWithRetry(m_Cmd).response;

    if (response.empty() || response[0] == '0')
        return false;

    m_JobId.erase();
    m_AuthToken.erase();
    m_JobStatus = CNetScheduleAPI::eDone;

    CUrlArgs read_response_parser(response);

    ITERATE(CUrlArgs::TArgs, it, read_response_parser.GetArgs()) {
        switch (it->name[0]) {
        case 'j':
            if (it->name == "job_key")
                m_JobId = it->value;
            break;
        case 'a':
            if (it->name == "auth_token")
                m_AuthToken = it->value;
            break;
        case 's':
            if (it->name == "status")
                m_JobStatus = CNetScheduleAPI::StringToStatus(it->value);
            break;
        }
    }

    return true;
}

bool CNetScheduleSubmitter::Read(string* job_id, string* auth_token,
        CNetScheduleAPI::EJobStatus* job_status, unsigned timeout,
        const string& job_group)
{
    string cmd("READ ");

    if (timeout > 0) {
        cmd += " timeout=";
        cmd += NStr::UIntToString(timeout);
    }
    if (!job_group.empty()) {
        SNetScheduleAPIImpl::VerifyJobGroupAlphabet(job_group);
        cmd += " group=";
        cmd += job_group;
    }

    g_AppendClientIPAndSessionID(cmd);

    CReadCmdExecutor read_executor(cmd, *job_id, *auth_token, *job_status);

    return m_Impl->m_API->m_Service.FindServer(&read_executor,
        CNetService::eRandomize) != NULL;
}

void SNetScheduleSubmitterImpl::FinalizeRead(const char* cmd_start,
    const char* cmd_name,
    const string& job_id,
    const string& auth_token,
    const string& error_message)
{
    string cmd = cmd_start + job_id;

    cmd += " auth_token=";
    cmd += auth_token;

    if (!error_message.empty()) {
        cmd += " err_msg=\"";
        cmd += NStr::PrintableString(error_message);
        cmd += '"';
    }

    g_AppendClientIPAndSessionID(cmd);

    m_API->GetServer(job_id).ExecWithRetry(cmd);
}

void CNetScheduleSubmitter::ReadConfirm(const string& job_id,
        const string& auth_token)
{
    m_Impl->FinalizeRead("CFRM job_key=", "ReadConfirm",
            job_id, auth_token, kEmptyStr);
}

void CNetScheduleSubmitter::ReadRollback(const string& job_id,
        const string& auth_token)
{
    m_Impl->FinalizeRead("RDRB job_key=", "ReadRollback",
            job_id, auth_token, kEmptyStr);
}

void CNetScheduleSubmitter::ReadFail(const string& job_id,
        const string& auth_token, const string& error_message)
{
    m_Impl->FinalizeRead("FRED job_key=", "ReadFail",
            job_id, auth_token, error_message);
}

void CNetScheduleNotificationHandler::SubmitJob(
        CNetScheduleSubmitter::TInstance submitter,
        CNetScheduleJob& job,
        unsigned wait_time,
        CNetServer* server)
{
    submitter->SubmitJobImpl(job, GetPort(), wait_time, server);
}

static const char* const s_JobStatusAttrNames[3] =
        {"job_key", "job_status", "last_event_index"};

bool CNetScheduleNotificationHandler::CheckJobStatusNotification(
        const string& job_id, CNetScheduleAPI::EJobStatus* job_status,
        int* last_event_index /*= NULL*/)
{
    string attr_values[3];

    if (last_event_index == NULL)
        g_ParseNSOutput(m_Message, s_JobStatusAttrNames, attr_values, 2);
    else {
        g_ParseNSOutput(m_Message, s_JobStatusAttrNames, attr_values, 3);

        if (!attr_values[2].empty())
            *last_event_index = NStr::StringToInt(attr_values[2],
                    NStr::fConvErr_NoThrow);
    }

    return (*job_status = CNetScheduleAPI::StringToStatus(attr_values[1])) !=
            CNetScheduleAPI::eJobNotFound && attr_values[0] == job_id;
}

CNetScheduleAPI::EJobStatus
CNetScheduleSubmitter::SubmitJobAndWait(CNetScheduleJob& job,
                                        unsigned       wait_time)
{
    CAbsTimeout abs_timeout(wait_time, 0);

    CNetScheduleNotificationHandler submit_job_handler;

    submit_job_handler.SubmitJob(m_Impl, job, wait_time);

    return submit_job_handler.WaitForJobCompletion(job,
            abs_timeout, m_Impl->m_API);
}

CNetScheduleAPI::EJobStatus
CNetScheduleNotificationHandler::WaitForJobCompletion(
        CNetScheduleJob& job,
        CAbsTimeout& abs_timeout,
        CNetScheduleAPI ns_api)
{
    CNetScheduleAPI::EJobStatus status = CNetScheduleAPI::ePending;

    unsigned wait_sec = FORCED_SST_INTERVAL_SEC;

    bool last_timeout = false;
    for (;;) {
        CAbsTimeout timeout(wait_sec++, FORCED_SST_INTERVAL_NANOSEC);

        if (!(timeout < abs_timeout)) {
            if (abs_timeout.GetRemainingTime().IsZero())
                try {
                    return ns_api.GetJobDetails(job);
                }
                catch (CException& e) {
                    ERR_POST(job.job_id << ": error while "
                            "retrieving job details: " << e);
                    return status;
                }

            timeout = abs_timeout;
            last_timeout = true;
        }

        if (WaitForNotification(timeout)) {
            if (CheckJobStatusNotification(job.job_id, &status)) {
                if (status == CNetScheduleAPI::eDone)
                    ns_api.GetJobDetails(job);

                return status;
            }
        } else
            try {
                status = ns_api.GetJobDetails(job);
                if ((status != CNetScheduleAPI::eRunning &&
                        status != CNetScheduleAPI::ePending) || last_timeout)
                    return status;
            }
            catch (CException& e) {
                ERR_POST(job.job_id << ": error while "
                        "retrieving job details: " << e);
                if (last_timeout)
                    return status;
            }
    }
}

bool CNetScheduleNotificationHandler::RequestJobWatching(
        CNetScheduleAPI::TInstance ns_api,
        const string& job_id,
        const CAbsTimeout& abs_timeout,
        CNetScheduleAPI::EJobStatus* job_status,
        int* last_event_index)
{
    string cmd("LISTEN job_key=" + job_id);

    cmd += " port=";
    cmd += NStr::NumericToString(GetPort());
    cmd += " timeout=";
    cmd += NStr::NumericToString(s_GetRemainingSeconds(abs_timeout));

    g_AppendClientIPAndSessionID(cmd);

    m_Message = ns_api->GetServer(job_id).ExecWithRetry(cmd).response;

    string attr_values[2];

    g_ParseNSOutput(m_Message, s_JobStatusAttrNames + 1, attr_values, 2);

    if (!attr_values[1].empty())
        *last_event_index = NStr::StringToInt(attr_values[1],
                NStr::fConvErr_NoThrow);

    return (*job_status = CNetScheduleAPI::StringToStatus(attr_values[0])) !=
        CNetScheduleAPI::eJobNotFound;
}

CNetScheduleAPI::EJobStatus
CNetScheduleNotificationHandler::WaitForJobEvent(
        const string& job_key,
        CAbsTimeout& abs_timeout,
        CNetScheduleAPI ns_api,
        int status_mask,
        int last_event_index,
        int *new_event_index)
{
    *new_event_index = -1;

    CNetScheduleAPI::EJobStatus job_status;

    unsigned wait_sec = FORCED_SST_INTERVAL_SEC;

    bool last_timeout = false;
    do {
        CAbsTimeout timeout(wait_sec++, FORCED_SST_INTERVAL_NANOSEC);

        if (!(timeout < abs_timeout)) {
            timeout = abs_timeout;
            last_timeout = true;
        }

        if (RequestJobWatching(ns_api, job_key,
                        timeout, &job_status, new_event_index) &&
                ((status_mask & (1 << job_status)) != 0 ||
                *new_event_index > last_event_index))
            break;

        if (abs_timeout.GetRemainingTime().IsZero())
            break;

        if (WaitForNotification(timeout) &&
                CheckJobStatusNotification(job_key, &job_status,
                        new_event_index) &&
                ((status_mask & (1 << job_status)) != 0 ||
                *new_event_index > last_event_index))
            break;
    } while (!last_timeout);

    return job_status;
}

void CNetScheduleSubmitter::CancelJob(const string& job_key)
{
    m_Impl->m_API->x_ExecOnce("CANCEL", job_key);
}

void CNetScheduleSubmitter::CancelJobGroup(const string& job_group)
{
    SNetScheduleAPIImpl::VerifyJobGroupAlphabet(job_group);
    string cmd("CANCEL group=" + job_group);
    g_AppendClientIPAndSessionID(cmd);
    m_Impl->m_API->m_Service.ExecOnAllServers(cmd);
}

CNetScheduleAPI::EJobStatus CNetScheduleSubmitter::GetJobStatus(
        const string& job_key, time_t* job_exptime)
{
    return m_Impl->m_API->GetJobStatus("SST2", job_key, job_exptime);
}

CNetScheduleAPI::EJobStatus CNetScheduleSubmitter::GetJobDetails(
        CNetScheduleJob& job, time_t* job_exptime)
{
    return m_Impl->m_API.GetJobDetails(job, job_exptime);
}

void CNetScheduleSubmitter::GetProgressMsg(CNetScheduleJob& job)
{
    m_Impl->m_API.GetProgressMsg(job);
}

END_NCBI_SCOPE
