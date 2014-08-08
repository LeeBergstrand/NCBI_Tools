/*  $Id: ns_cmds.cpp 391224 2013-03-06 16:01:51Z kazimird $
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
 *   Government have not placed any restriction on its use or reproduction.
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
 * Authors:  Dmitry Kazimirov
 *
 * File Description: NetSchedule-specific commands of the grid_cli application.
 *
 */

#include <ncbi_pch.hpp>

#include "ns_cmd_impl.hpp"
#include "util.hpp"

#include <connect/services/remote_app.hpp>
#include <connect/services/grid_rw_impl.hpp>

#include <corelib/rwstream.hpp>

#include <ctype.h>

BEGIN_NCBI_SCOPE

void CGridCommandLineInterfaceApp::SetUp_NetScheduleCmd(
        CGridCommandLineInterfaceApp::EAPIClass api_class,
        CGridCommandLineInterfaceApp::EAdminCmdSeverity cmd_severity)
{
    string queue(!m_Opts.queue.empty() ? m_Opts.queue : "noname");

    if (!IsOptionSet(eID) && !IsOptionSet(eJobId))
        m_NetScheduleAPI = CNetScheduleAPI(m_Opts.ns_service,
            m_Opts.auth, queue);
    else if (IsOptionExplicitlySet(eNetSchedule)) {
        string host, port;

        if (!NStr::SplitInTwo(m_Opts.ns_service, ":", host, port)) {
            NCBI_THROW(CArgException, eInvalidArg,
                    "When job ID is given, '--" NETSCHEDULE_OPTION "' "
                    "must be a host:port server address.");
        }

        m_NetScheduleAPI = CNetScheduleAPI(m_Opts.ns_service,
            m_Opts.auth, queue);
        m_NetScheduleAPI.GetService().GetServerPool().StickToServer(host,
            (unsigned short) NStr::StringToInt(port));
    } else {
        CNetScheduleKey key(m_Opts.id);
        key.host.push_back(':');
        key.host.append(NStr::NumericToString(key.port));
        m_NetScheduleAPI = CNetScheduleAPI(key.host, m_Opts.auth, queue);
    }

#ifdef NCBI_GRID_XSITE_CONN_SUPPORT
    if (IsOptionSet(eAllowXSiteConn))
        m_NetScheduleAPI.GetService().AllowXSiteConnections();
#endif

    if (IsOptionSet(eCompatMode)) {
        m_NetScheduleAPI.UseOldStyleAuth();
        if (IsOptionSet(eWorkerNode))
            m_NetScheduleAPI.EnableWorkerNodeCompatMode();
    }

    // Specialize NetSchedule API.
    switch (api_class) {
    case eNetScheduleAPI:
        break;
    case eNetScheduleAdmin:
        if (cmd_severity != eReadOnlyAdminCmd) {
            if (!IsOptionExplicitlySet(eNetSchedule)) {
                NCBI_THROW(CArgException, eNoValue, "'--" NETSCHEDULE_OPTION
                        "' must be explicitly specified.");
            }
            if (IsOptionAcceptedAndSetImplicitly(eQueue)) {
                NCBI_THROW(CArgException, eNoValue, "'--" QUEUE_OPTION
                        "' must be specified explicitly (not via $"
                        LOGIN_TOKEN_ENV ").");
            }
        }
        /* FALL THROUGH */
    case eWorkerNodeAdmin:
        m_NetScheduleAdmin = m_NetScheduleAPI.GetAdmin();
        break;
    case eNetScheduleExecutor:
        m_NetScheduleExecutor = m_NetScheduleAPI.GetExecutor();

        if (!IsOptionSet(eLoginToken) &&
                !IsOptionSet(eClientNode) && !IsOptionSet(eClientSession)) {
            NCBI_THROW(CArgException, eNoArg, "client identification required "
                    "(see the '" LOGIN_COMMAND "' command).");
        }
        /* FALL THROUGH */
    case eNetScheduleSubmitter:
        m_NetScheduleSubmitter = m_NetScheduleAPI.GetSubmitter();

        SetUp_NetCacheCmd(eNetCacheAPI);
        m_GridClient.reset(new CGridClient(m_NetScheduleSubmitter,
                m_NetCacheAPI, CGridClient::eManualCleanup,
                        CGridClient::eProgressMsgOn));
        break;
    default:
        _ASSERT(0);
        break;
    }

    if (!IsOptionSet(eClientNode)) {
        string user, host;
        g_GetUserAndHost(&user, &host);
        DefineClientNode(user, host);
    }

    if (!m_Opts.client_node.empty())
        m_NetScheduleAPI.SetClientNode(m_Opts.client_node);

    if (!IsOptionSet(eClientSession))
        DefineClientSession(NStr::NumericToString(CProcess::GetCurrentPid()),
                NStr::NumericToString(GetFastLocalTime().GetTimeT()),
                GetDiagContext().GetStringUID());

    if (!m_Opts.client_session.empty())
        m_NetScheduleAPI.SetClientSession(m_Opts.client_session);
}

void CGridCommandLineInterfaceApp::JobInfo_PrintStatus(
        CNetScheduleAPI::EJobStatus status)
{
    string job_status(CNetScheduleAPI::StatusToString(status));

    if (m_Opts.output_format == eJSON)
        printf("{\"status\": \"%s\"}\n", job_status.c_str());
    else
        PrintLine(job_status);
}

int CGridCommandLineInterfaceApp::Cmd_JobInfo()
{
    SetUp_NetScheduleCmd(eNetScheduleAdmin, eReadOnlyAdminCmd);

    if (IsOptionSet(eDeferExpiration)) {
        CNetScheduleAPI::EJobStatus status =
                m_NetScheduleAPI.GetSubmitter().GetJobStatus(m_Opts.id);
        if (IsOptionSet(eStatusOnly)) {
            JobInfo_PrintStatus(status);
            return 0;
        }
    } else if (IsOptionSet(eStatusOnly)) {
        JobInfo_PrintStatus(
                m_NetScheduleAPI.GetExecutor().GetJobStatus(m_Opts.id));
        return 0;
    }

    if (IsOptionSet(eProgressMessageOnly)) {
        CNetScheduleJob job;
        job.job_id = m_Opts.id;
        m_NetScheduleAPI.GetProgressMsg(job);
        if (m_Opts.output_format == eJSON)
            printf("{\"progress_message\": \"%s\"}\n",
                    job.progress_msg.c_str());
        else
            if (!job.progress_msg.empty())
                PrintLine(job.progress_msg);
        return 0;
    }

    switch (m_Opts.output_format) {
    case eRaw:
        if (IsOptionSet(eBrief)) {
            fprintf(stderr, GRID_APP_NAME " " JOBINFO_COMMAND ": option '--"
                    BRIEF_OPTION "' cannot be used with '"
                    RAW_OUTPUT_FORMAT "' output format.\n");
            return 2;
        }
        m_NetScheduleAdmin.DumpJob(NcbiCout, m_Opts.id);
        break;

    case eJSON:
        {
            CJobInfoToJSON job_info_to_json;
            ProcessJobInfo(m_NetScheduleAPI, m_Opts.id,
                    &job_info_to_json, !IsOptionSet(eBrief));
            g_PrintJSON(stdout, job_info_to_json.GetRootNode());
        }
        break;

    default:
        {
            CPrintJobInfo print_job_info;
            ProcessJobInfo(m_NetScheduleAPI, m_Opts.id,
                    &print_job_info, !IsOptionSet(eBrief));
        }
    }

    return 0;
}

class CBatchSubmitAttrParser
{
public:
    CBatchSubmitAttrParser(FILE* input_stream) :
        m_InputStream(input_stream),
        m_LineNumber(0)
    {
    }
    bool NextLine();
    bool NextAttribute();
    EOption GetAttributeType() const {return m_JobAttribute;}
    const string& GetAttributeValue() const {return m_JobAttributeValue;}
    size_t GetLineNumber() const {return m_LineNumber;}
    void ReportInvalidJobInputAttr();

private:
    FILE* m_InputStream;
    size_t m_LineNumber;
    string m_Line;
    CAttrListParser m_AttrParser;
    EOption m_JobAttribute;
    string m_JobAttributeValue;
};

bool CBatchSubmitAttrParser::NextLine()
{
    if (m_InputStream == NULL)
        return false;

    ++m_LineNumber;
    m_Line.resize(0);

    char buffer[64 * 1024];
    size_t bytes_read;

    while (fgets(buffer, sizeof(buffer), m_InputStream) != NULL)
        if ((bytes_read = strlen(buffer)) > 0) {
            if (buffer[bytes_read - 1] != '\n')
                m_Line.append(buffer, bytes_read);
            else {
                m_Line.append(buffer, bytes_read - 1);
                m_AttrParser.Reset(m_Line);
                return true;
            }
        }

    m_InputStream = NULL;

    if (m_Line.empty())
        return false;
    else {
        m_AttrParser.Reset(m_Line);
        return true;
    }
}

bool CBatchSubmitAttrParser::NextAttribute()
{
    m_JobAttribute = eUntypedArg;

#define ATTR_CHECK_SET(name, type) \
    if (attr_name.length() == sizeof(name) - 1 && \
            memcmp(attr_name.data(), name, sizeof(name) - 1) == 0) { \
        m_JobAttribute = type; \
        break; \
    }

    CTempString attr_name;

    CAttrListParser::ENextAttributeType next_attr_type =
        m_AttrParser.NextAttribute(attr_name, m_JobAttributeValue);

    if (next_attr_type == CAttrListParser::eNoMoreAttributes)
        return false;

    switch (attr_name[0]) {
    case 'i':
        ATTR_CHECK_SET("input", eInput);
        break;
    case 'a':
        ATTR_CHECK_SET("args", eRemoteAppArgs);
        ATTR_CHECK_SET("affinity", eAffinity);
        break;
    case 'e':
        ATTR_CHECK_SET("exclusive", eExclusiveJob);
    }

#define AT_POS(pos) " at line " << m_LineNumber << \
    ", column " << (pos - m_Line.data() + 1)

    switch (m_JobAttribute) {
    case eUntypedArg:
        NCBI_THROW_FMT(CArgException, eInvalidArg,
            "unknown attribute " << attr_name <<
                AT_POS(attr_name.data()));

    case eExclusiveJob:
        break;

    default:
        if (next_attr_type != CAttrListParser::eAttributeWithValue) {
            NCBI_THROW_FMT(CArgException, eInvalidArg,
                "attribute " << attr_name <<
                    " requires a value" << AT_POS(attr_name.data()));
        }
    }

    return true;
}

void CBatchSubmitAttrParser::ReportInvalidJobInputAttr()
{
    NCBI_THROW_FMT(CArgException, eInvalidArg, "Exactly one of "
        "either \"input\" or \"args\" attribute is required "
        "at line " << GetLineNumber());
}

static const string s_NotificationTimestampFormat("Y/M/D h:m:s.l");

void CGridCommandLineInterfaceApp::PrintJobStatusNotification(
        CNetScheduleNotificationHandler& submit_job_handler,
        const string& job_key,
        const string& server_host)
{
    CNetScheduleAPI::EJobStatus job_status = CNetScheduleAPI::eJobNotFound;
    int last_event_index = -1;

    const char* format = "%s \"%s\" from %s [invalid]\n";

    if (submit_job_handler.CheckJobStatusNotification(job_key,
            &job_status, &last_event_index))
        format = "%s \"%s\" from %s [valid, "
                "job_status=%s, last_event_index=%d]\n";

    printf(format, GetFastLocalTime().
            AsString(s_NotificationTimestampFormat).c_str(),
            submit_job_handler.GetMessage().c_str(),
            server_host.c_str(),
            CNetScheduleAPI::StatusToString(job_status).c_str(),
            last_event_index);
}

void CGridCommandLineInterfaceApp::CheckJobInputStream(
        CNcbiOstream& job_input_stream)
{
    if (job_input_stream.bad()) {
        NCBI_THROW(CIOException, eWrite, "Error while writing job input");
    }
}

void CGridCommandLineInterfaceApp::PrepareRemoteAppJobInput(const string& args,
        CNcbiOstream& job_input_stream)
{
    CRemoteAppRequest request(m_GridClient->GetNetCacheAPI());

    // Roughly estimate the maximum embedded input size.
    size_t input_size = m_GridClient->GetMaxServerInputSize();
    request.SetMaxInlineSize(input_size == 0 ? kMax_UInt :
            input_size - input_size / 10);

    request.SetCmdLine(args);
    request.Send(job_input_stream);

    CheckJobInputStream(job_input_stream);
}

void CGridCommandLineInterfaceApp::SubmitJob_Batch()
{
    CBatchSubmitAttrParser attr_parser(m_Opts.input_stream);

    if (m_Opts.batch_size <= 1) {
        while (attr_parser.NextLine()) {
            bool input_defined = false;
            while (attr_parser.NextAttribute()) {
                const string& attr_value(attr_parser.GetAttributeValue());
                switch (attr_parser.GetAttributeType()) {
                case eInput:
                    if (input_defined)
                        attr_parser.ReportInvalidJobInputAttr();
                    input_defined = true;
                    {
                        CNcbiOstream& job_input_stream(
                            m_GridClient->GetOStream());
                        job_input_stream.write(attr_value.data(),
                            attr_value.length());
                        CheckJobInputStream(job_input_stream);
                    }
                    break;
                case eRemoteAppArgs:
                    if (input_defined)
                        attr_parser.ReportInvalidJobInputAttr();
                    input_defined = true;
                    PrepareRemoteAppJobInput(attr_value,
                            m_GridClient->GetOStream());
                    break;
                case eAffinity:
                    m_GridClient->SetJobAffinity(attr_value);
                    break;
                case eExclusiveJob:
                    m_GridClient->SetJobMask(CNetScheduleAPI::eExclusiveJob);
                    break;
                default:
                    _ASSERT(0);
                    break;
                }
            }
            if (!input_defined)
                attr_parser.ReportInvalidJobInputAttr();
            if (IsOptionSet(eGroup))
                m_GridClient->SetJobGroup(m_Opts.job_group);
            fprintf(m_Opts.output_stream,
                "%s\n", m_GridClient->Submit(m_Opts.affinity).c_str());
        }
    } else {
        CGridJobBatchSubmitter& batch_submitter(
            m_GridClient->GetJobBatchSubmitter());
        unsigned remaining_batch_size = m_Opts.batch_size;

        while (attr_parser.NextLine()) {
            if (remaining_batch_size == 0) {
                batch_submitter.Submit(m_Opts.job_group);
                const vector<CNetScheduleJob>& jobs =
                    batch_submitter.GetBatch();
                ITERATE(vector<CNetScheduleJob>, it, jobs)
                    fprintf(m_Opts.output_stream,
                        "%s\n", it->job_id.c_str());
                batch_submitter.Reset();
                remaining_batch_size = m_Opts.batch_size;
            }
            batch_submitter.PrepareNextJob();
            bool input_defined = false;
            while (attr_parser.NextAttribute()) {
                const string& attr_value(attr_parser.GetAttributeValue());
                switch (attr_parser.GetAttributeType()) {
                case eInput:
                    if (input_defined)
                        attr_parser.ReportInvalidJobInputAttr();
                    input_defined = true;
                    {
                        CNcbiOstream& job_input_stream(
                                batch_submitter.GetOStream());
                        job_input_stream.write(attr_value.data(),
                                attr_value.length());
                        CheckJobInputStream(job_input_stream);
                    }
                    break;
                case eRemoteAppArgs:
                    if (input_defined)
                        attr_parser.ReportInvalidJobInputAttr();
                    input_defined = true;
                    PrepareRemoteAppJobInput(attr_value,
                            batch_submitter.GetOStream());
                    break;
                case eAffinity:
                    batch_submitter.SetJobAffinity(attr_value);
                    break;
                case eExclusiveJob:
                    batch_submitter.SetJobMask(CNetScheduleAPI::eExclusiveJob);
                    break;
                default:
                    _ASSERT(0);
                    break;
                }
            }
            if (!input_defined)
                attr_parser.ReportInvalidJobInputAttr();
            --remaining_batch_size;
        }
        if (remaining_batch_size < m_Opts.batch_size) {
            batch_submitter.Submit(m_Opts.job_group);
            const vector<CNetScheduleJob>& jobs =
                batch_submitter.GetBatch();
            ITERATE(vector<CNetScheduleJob>, it, jobs)
                fprintf(m_Opts.output_stream, "%s\n", it->job_id.c_str());
            batch_submitter.Reset();
        }
    }
}

int CGridCommandLineInterfaceApp::Cmd_SubmitJob()
{
    SetUp_NetScheduleCmd(eNetScheduleSubmitter);

    if (IsOptionSet(eBatch))
        SubmitJob_Batch();
    else {
        CNcbiOstream& job_input_stream = m_GridClient->GetOStream();

        if (IsOptionSet(eRemoteAppArgs))
            PrepareRemoteAppJobInput(m_Opts.remote_app_args, job_input_stream);
        else if (IsOptionSet(eInput)) {
            job_input_stream.write(m_Opts.input.data(), m_Opts.input.length());
            CheckJobInputStream(job_input_stream);
        } else {
            char buffer[16 * 1024];
            size_t bytes_read;

            while ((bytes_read = fread(buffer, 1,
                    sizeof(buffer), m_Opts.input_stream)) > 0) {
                job_input_stream.write(buffer, bytes_read);
                CheckJobInputStream(job_input_stream);
                if (feof(m_Opts.input_stream))
                    break;
            }
        }

        m_GridClient->SetJobGroup(m_Opts.job_group);
        m_GridClient->SetJobAffinity(m_Opts.affinity);

        if (IsOptionSet(eExclusiveJob))
            m_GridClient->SetJobMask(CNetScheduleAPI::eExclusiveJob);

        if (!IsOptionSet(eWaitTimeout))
            PrintLine(m_GridClient->Submit());
        else {
            m_GridClient->CloseStream();

            CNetScheduleJob& job = m_GridClient->GetJob();

            CAbsTimeout abs_timeout(m_Opts.timeout, 0);

            CNetScheduleNotificationHandler submit_job_handler;

            submit_job_handler.SubmitJob(m_NetScheduleSubmitter,
                job, m_Opts.timeout);

            PrintLine(job.job_id);

            if (!IsOptionSet(eDumpNSNotifications)) {
                CNetScheduleAPI::EJobStatus status =
                        submit_job_handler.WaitForJobCompletion(job,
                                abs_timeout, m_NetScheduleAPI);

                PrintLine(CNetScheduleAPI::StatusToString(status));

                if (status == CNetScheduleAPI::eDone) {
                    if (IsOptionSet(eRemoteAppArgs))
                        MarkOptionAsSet(eRemoteAppStdOut);
                    DumpJobInputOutput(job.output);
                }
            } else {
                submit_job_handler.PrintPortNumber();

                string server_host;

                if (submit_job_handler.WaitForNotification(abs_timeout,
                        &server_host))
                    PrintJobStatusNotification(submit_job_handler,
                            job.job_id, server_host);
            }
        }
    }

    return 0;
}

int CGridCommandLineInterfaceApp::Cmd_WatchJob()
{
    SetUp_NetScheduleCmd(eNetScheduleSubmitter);

    if (!IsOptionSet(eWaitTimeout)) {
        fprintf(stderr, GRID_APP_NAME " " WATCHJOB_COMMAND
            ": option '--" WAIT_TIMEOUT_OPTION "' is required.\n");
        return 2;
    }

    if (!IsOptionSet(eWaitForJobStatus) && !IsOptionSet(eWaitForJobEventAfter))
        m_Opts.job_status_mask = ~((1 << CNetScheduleAPI::ePending) |
                (1 << CNetScheduleAPI::eRunning));

    CAbsTimeout abs_timeout(m_Opts.timeout, 0);

    CNetScheduleNotificationHandler submit_job_handler;

    CNetScheduleAPI::EJobStatus job_status;
    int last_event_index = -1;

    if (!submit_job_handler.RequestJobWatching(m_NetScheduleAPI, m_Opts.id,
            abs_timeout, &job_status, &last_event_index)) {
        fprintf(stderr, GRID_APP_NAME ": unexpected error while "
                "setting up a job event listener.\n");
        return 3;
    }

    if (!IsOptionSet(eDumpNSNotifications)) {
        if (last_event_index <= m_Opts.last_event_index &&
                (m_Opts.job_status_mask & (1 << job_status)) == 0)
            job_status = submit_job_handler.WaitForJobEvent(m_Opts.id,
                    abs_timeout, m_NetScheduleAPI, m_Opts.job_status_mask,
                    m_Opts.last_event_index, &last_event_index);

        printf("%d\n%s\n", last_event_index,
                CNetScheduleAPI::StatusToString(job_status).c_str());
    } else {
        if (last_event_index > m_Opts.last_event_index) {
            fprintf(stderr, "Job event index (%d) has already "
                    "exceeded %d; won't wait.\n",
                    last_event_index, m_Opts.last_event_index);
            return 6;
        }
        if ((m_Opts.job_status_mask & (1 << job_status)) != 0) {
            fprintf(stderr, "Job is already '%s'; won't wait.\n",
                    CNetScheduleAPI::StatusToString(job_status).c_str());
            return 6;
        }

        submit_job_handler.PrintPortNumber();

        string server_host;

        while (submit_job_handler.WaitForNotification(abs_timeout,
                &server_host))
            PrintJobStatusNotification(submit_job_handler,
                    m_Opts.id, server_host);
    }

    return 0;
}

int CGridCommandLineInterfaceApp::DumpJobInputOutput(
    const string& data_or_blob_id)
{
    char buffer[16 * 1024];
    size_t bytes_read;

    if (IsOptionSet(eRemoteAppStdOut) || IsOptionSet(eRemoteAppStdErr)) {
        auto_ptr<IReader> reader(new CStringOrBlobStorageReader(data_or_blob_id,
                m_NetCacheAPI));

        auto_ptr<CNcbiIstream> input_stream(new CRStream(reader.release(), 0,
                0, CRWStreambuf::fOwnReader | CRWStreambuf::fLeakExceptions));

        CRemoteAppResult remote_app_result(m_NetCacheAPI);
        remote_app_result.Receive(*input_stream);

        CNcbiIstream& std_stream(IsOptionSet(eRemoteAppStdOut) ?
                remote_app_result.GetStdOut() : remote_app_result.GetStdErr());

        std_stream.exceptions((ios::iostate) 0);

        while (!std_stream.eof()) {
            std_stream.read(buffer, sizeof(buffer));
            if (std_stream.bad())
                goto Error;
            bytes_read = (size_t) std_stream.gcount();
            if (fwrite(buffer, 1, bytes_read,
                    m_Opts.output_stream) < bytes_read)
                goto Error;
        }

        return 0;
    }

    try {
        CStringOrBlobStorageReader reader(data_or_blob_id, m_NetCacheAPI);

        while (reader.Read(buffer, sizeof(buffer), &bytes_read) != eRW_Eof)
            if (fwrite(buffer, 1, bytes_read,
                    m_Opts.output_stream) < bytes_read)
                goto Error;
    }
    catch (CStringOrBlobStorageRWException& e) {
        if (e.GetErrCode() != CStringOrBlobStorageRWException::eInvalidFlag)
            throw;
        if (fwrite(data_or_blob_id.data(), 1, data_or_blob_id.length(),
                m_Opts.output_stream) < data_or_blob_id.length())
            goto Error;
    }

    return 0;

Error:
    fprintf(stderr, GRID_APP_NAME ": error while writing job data.\n");
    return 3;
}

int CGridCommandLineInterfaceApp::PrintJobAttrsAndDumpInput(
    const CNetScheduleJob& job)
{
    PrintLine(job.job_id);
    if (!job.auth_token.empty())
        printf("%s ", job.auth_token.c_str());
    if (!job.affinity.empty()) {
        string affinity(NStr::PrintableString(job.affinity));
        printf(job.mask & CNetScheduleAPI::eExclusiveJob ?
            "affinity=\"%s\" exclusive\n" : "affinity=\"%s\"\n",
            affinity.c_str());
    } else
        printf(job.mask & CNetScheduleAPI::eExclusiveJob ?
            "exclusive\n" : "\n");
    return DumpJobInputOutput(job.input);
}

int CGridCommandLineInterfaceApp::Cmd_GetJobInput()
{
    SetUp_NetScheduleCmd(eNetScheduleSubmitter);

    CNetScheduleJob job;
    job.job_id = m_Opts.id;

    if (m_NetScheduleAPI.GetJobDetails(job) == CNetScheduleAPI::eJobNotFound) {
        fprintf(stderr, GRID_APP_NAME ": job %s has expired.\n",
                job.job_id.c_str());
        return 3;
    }

    return DumpJobInputOutput(job.input);
}

int CGridCommandLineInterfaceApp::Cmd_GetJobOutput()
{
    SetUp_NetScheduleCmd(eNetScheduleSubmitter);

    CNetScheduleJob job;
    job.job_id = m_Opts.id;
    CNetScheduleAPI::EJobStatus status = m_NetScheduleAPI.GetJobDetails(job);

    switch (status) {
    case CNetScheduleAPI::eDone:
    case CNetScheduleAPI::eReading:
    case CNetScheduleAPI::eConfirmed:
    case CNetScheduleAPI::eReadFailed:
        break;

    default:
        fprintf(stderr, GRID_APP_NAME
            ": cannot retrieve job output for job status %s.\n",
            CNetScheduleAPI::StatusToString(status).c_str());
        return 3;
    }

    return DumpJobInputOutput(job.output);
}

int CGridCommandLineInterfaceApp::Cmd_ReadJob()
{
    SetUp_NetScheduleCmd(eNetScheduleSubmitter);

    if (!IsOptionSet(eConfirmRead) && !IsOptionSet(eFailRead) &&
            !IsOptionSet(eRollbackRead)) {
        string job_id, auth_token;
        CNetScheduleAPI::EJobStatus job_status;

        if (m_NetScheduleSubmitter.Read(&job_id, &auth_token, &job_status,
                m_Opts.timeout, m_Opts.job_group)) {
            PrintLine(job_id);
            PrintLine(CNetScheduleAPI::StatusToString(job_status));

            if (IsOptionSet(eReliableRead))
                PrintLine(auth_token);
            else {
                if (job_status == CNetScheduleAPI::eDone) {
                    CNetScheduleJob job;
                    job.job_id = job_id;
                    m_NetScheduleSubmitter.GetJobDetails(job);
                    int ret_code = DumpJobInputOutput(job.output);
                    if (ret_code != 0)
                        return ret_code;
                }
                m_NetScheduleSubmitter.ReadConfirm(job_id, auth_token);
            }
        }
    } else {
        if (!IsOptionSet(eJobId)) {
            fprintf(stderr, GRID_APP_NAME " " READJOB_COMMAND
                ": option '--" JOB_ID_OPTION "' is required.\n");
            return 2;
        }

        if (IsOptionSet(eConfirmRead))
            m_NetScheduleSubmitter.ReadConfirm(m_Opts.id, m_Opts.auth_token);
        else if (IsOptionSet(eFailRead))
            m_NetScheduleSubmitter.ReadFail(m_Opts.id, m_Opts.auth_token,
                    m_Opts.error_message);
        else
            m_NetScheduleSubmitter.ReadRollback(m_Opts.id, m_Opts.auth_token);
    }

    return 0;
}

int CGridCommandLineInterfaceApp::Cmd_CancelJob()
{
    if (IsOptionSet(eJobGroup)) {
        SetUp_NetScheduleCmd(eNetScheduleAPI);

        m_NetScheduleAPI.GetSubmitter().CancelJobGroup(m_Opts.job_group);
    } else if (IsOptionSet(eAllJobs)) {
        SetUp_NetScheduleCmd(eNetScheduleAdmin, eSevereAdminCmd);

        m_NetScheduleAdmin.CancelAllJobs();
    } else {
        SetUp_NetScheduleCmd(eNetScheduleAPI);

        m_NetScheduleAPI.GetSubmitter().CancelJob(m_Opts.id);
    }

    return 0;
}

int CGridCommandLineInterfaceApp::Cmd_RequestJob()
{
    SetUp_NetScheduleCmd(eNetScheduleExecutor);

    CNetScheduleExecutor::EJobAffinityPreference affinity_preference =
            CNetScheduleExecutor::eExplicitAffinitiesOnly;

    switch (IsOptionSet(eUsePreferredAffinities, OPTION_N(0)) |
            IsOptionSet(eClaimNewAffinities, OPTION_N(1)) |
            IsOptionSet(eAnyAffinity, OPTION_N(2))) {
    case OPTION_N(2) + OPTION_N(0):
        affinity_preference = CNetScheduleExecutor::ePreferredAffsOrAnyJob;
        break;

    case OPTION_N(1):
    case OPTION_N(1) + OPTION_N(0):
        affinity_preference = CNetScheduleExecutor::eClaimNewPreferredAffs;
        break;

    case OPTION_N(0):
        affinity_preference = CNetScheduleExecutor::ePreferredAffinities;
        break;

    case 0:
        if (IsOptionSet(eAffinityList))
            break;
        /* FALL THROUGH */

    case OPTION_N(2):
        affinity_preference = CNetScheduleExecutor::eAnyJob;
        break;

    default:
        fprintf(stderr, GRID_APP_NAME ": options '--"
            CLAIM_NEW_AFFINITIES_OPTION "' and '--" ANY_AFFINITY_OPTION
            "' are mutually exclusive.\n");
        return 2;
    }

    m_NetScheduleExecutor.SetAffinityPreference(affinity_preference);

    CNetScheduleJob job;

    if (!IsOptionSet(eDumpNSNotifications)) {
        if (m_NetScheduleExecutor.GetJob(job, m_Opts.timeout, m_Opts.affinity))
            return PrintJobAttrsAndDumpInput(job);
    } else {
        CAbsTimeout abs_timeout(m_Opts.timeout, 0);

        CNetScheduleNotificationHandler wait_job_handler;

        wait_job_handler.PrintPortNumber();

        if (wait_job_handler.RequestJob(m_NetScheduleExecutor, job,
                wait_job_handler.CmdAppendTimeoutAndClientInfo(
                CNetScheduleNotificationHandler::MkBaseGETCmd(
                affinity_preference, m_Opts.affinity), &abs_timeout))) {
            fprintf(stderr, "%s\nA job has been returned; won't wait.\n",
                    job.job_id.c_str());
            return 6;
        }

        string server_host;
        CNetServer server;
        string server_address;

        while (wait_job_handler.WaitForNotification(abs_timeout,
                &server_host)) {
            const char* format = "%s \"%s\" from %s [invalid]\n";

            if (wait_job_handler.CheckRequestJobNotification(
                    m_NetScheduleExecutor, &server)) {
                server_address = server.GetServerAddress();
                format = "%s \"%s\" from %s [valid, server=%s]\n";
            }

            printf(format, GetFastLocalTime().AsString(
                    s_NotificationTimestampFormat).c_str(),
                    wait_job_handler.GetMessage().c_str(),
                    server_host.c_str(),
                    server_address.c_str());
        }
    }

    return 0;
}

int CGridCommandLineInterfaceApp::Cmd_CommitJob()
{
    SetUp_NetScheduleCmd(eNetScheduleExecutor);

    CNetScheduleJob job;

    job.job_id = m_Opts.id;
    job.ret_code = m_Opts.return_code;
    job.auth_token = m_Opts.auth_token;

    if (IsOptionSet(eJobOutputBlob))
        job.output = "K " + m_Opts.job_output_blob;
    else {
        auto_ptr<IEmbeddedStreamWriter> writer(new CStringOrBlobStorageWriter(
                m_NetScheduleAPI.GetServerParams().max_output_size,
                m_NetCacheAPI, job.output));

        char buffer[16 * 1024];
        size_t bytes_read;

        if (!IsOptionSet(eJobOutput))
            while ((bytes_read = fread(buffer, 1,
                    sizeof(buffer), m_Opts.input_stream)) > 0) {
                if (writer->Write(buffer, bytes_read) != eRW_Success)
                    goto ErrorExit;
                if (feof(m_Opts.input_stream))
                    break;
            }
        else
            if (writer->Write(m_Opts.job_output.data(),
                    m_Opts.job_output.length()) != eRW_Success)
                goto ErrorExit;
    }

    if (!IsOptionSet(eFailJob))
        m_NetScheduleExecutor.PutResult(job);
    else {
        job.error_msg = m_Opts.error_message;
        m_NetScheduleExecutor.PutFailure(job);
    }

    return 0;

ErrorExit:
    fprintf(stderr, GRID_APP_NAME ": error while submitting job output.\n");
    return 3;
}

int CGridCommandLineInterfaceApp::Cmd_ReturnJob()
{
    SetUp_NetScheduleCmd(eNetScheduleExecutor);

    m_NetScheduleExecutor.ReturnJob(m_Opts.id, m_Opts.auth_token);

    return 0;
}

int CGridCommandLineInterfaceApp::Cmd_ClearNode()
{
    SetUp_NetScheduleCmd(eNetScheduleExecutor);

    m_NetScheduleExecutor.ClearNode();

    return 0;
}

int CGridCommandLineInterfaceApp::Cmd_UpdateJob()
{
    SetUp_NetScheduleCmd(eNetScheduleAPI);

    if (IsOptionSet(eExtendLifetime) ||
            IsOptionSet(eProgressMessage)) {
        CNetScheduleExecutor executor(m_NetScheduleAPI.GetExecutor());

        if (IsOptionSet(eExtendLifetime))
            executor.JobDelayExpiration(m_Opts.id,
                    (unsigned) m_Opts.extend_lifetime_by);

        if (IsOptionSet(eProgressMessage)) {
            CNetScheduleJob job;
            job.job_id = m_Opts.id;
            job.progress_msg = m_Opts.progress_message;
            executor.PutProgressMsg(job);
        }
    }

    return 0;
}

int CGridCommandLineInterfaceApp::Cmd_QueueInfo()
{
    SetUp_NetScheduleCmd(eNetScheduleAdmin, eReadOnlyAdminCmd);

    if (!IsOptionSet(eQueueClasses)) {
        if ((IsOptionSet(eQueueArg) ^ IsOptionSet(eAllQueues)) == 0) {
            fprintf(stderr, GRID_APP_NAME " " QUEUEINFO_COMMAND
                    ": either the '" QUEUE_ARG "' argument or the '--"
                    ALL_QUEUES_OPTION "' option must be specified.\n");
            return 1;
        }
        if (m_Opts.output_format == eJSON)
            g_PrintJSON(stdout, g_QueueInfoToJson(m_NetScheduleAPI,
                    IsOptionSet(eQueueArg) ? m_Opts.queue : kEmptyStr,
                    m_NetScheduleAPI.GetService().GetServiceType()));
        else if (!IsOptionSet(eAllQueues))
            m_NetScheduleAdmin.PrintQueueInfo(m_Opts.queue, NcbiCout);
        else {
            CNetService service(m_NetScheduleAPI.GetService());
            string client_name(service.GetServerPool().GetClientName());
            CNetScheduleAdmin::TQueueList qlist;
            m_NetScheduleAdmin.GetQueueList(qlist);
            bool load_balanced = service.IsLoadBalanced();
            ITERATE(CNetScheduleAdmin::TQueueList,
                    server_and_its_queues, qlist) {
                string server_address(
                        server_and_its_queues->server.GetServerAddress());
                if (load_balanced)
                    NcbiCout << "[server " << server_address << ']' << NcbiEndl;
                ITERATE(list<string>, queue_name,
                        server_and_its_queues->queues) {
                    NcbiCout << '[' << *queue_name << ']' << NcbiEndl;
                    CNetScheduleAPI(server_address, client_name,
                            *queue_name).GetAdmin().PrintQueueInfo(*queue_name,
                                    NcbiCout);
                    NcbiCout << NcbiEndl;
                }
                if (load_balanced)
                    NcbiCout << NcbiEndl;
            }
        }
    } else if (m_Opts.output_format == eJSON)
        g_PrintJSON(stdout, g_QueueClassInfoToJson(m_NetScheduleAPI,
                m_NetScheduleAPI.GetService().GetServiceType()));
    else
        m_NetScheduleAPI.GetService().PrintCmdOutput("STAT QCLASSES",
                NcbiCout, CNetService::eMultilineOutput);

    return 0;
}

int CGridCommandLineInterfaceApp::Cmd_DumpQueue()
{
    SetUp_NetScheduleCmd(eNetScheduleAdmin, eReadOnlyAdminCmd);

    m_NetScheduleAdmin.DumpQueue(NcbiCout, m_Opts.start_after_job,
            m_Opts.job_count, m_Opts.job_status, m_Opts.job_group);

    return 0;
}

int CGridCommandLineInterfaceApp::Cmd_CreateQueue()
{
    SetUp_NetScheduleCmd(eNetScheduleAdmin, eSevereAdminCmd);

    m_NetScheduleAdmin.CreateQueue(m_Opts.id,
            m_Opts.queue_class, m_Opts.queue_description);

    return 0;
}

int CGridCommandLineInterfaceApp::Cmd_GetQueueList()
{
    SetUp_NetScheduleCmd(eNetScheduleAdmin, eReadOnlyAdminCmd);

    CNetScheduleAdmin::TQueueList queues;

    m_NetScheduleAdmin.GetQueueList(queues);

    typedef set<string> TServerSet;
    typedef map<string, TServerSet> TQueueRegister;

    TQueueRegister queue_register;

    ITERATE (CNetScheduleAdmin::TQueueList, it, queues) {
        string server_address =
                g_NetService_gethostnamebyaddr(it->server.GetHost());
        server_address += ':';
        server_address += NStr::UIntToString(it->server.GetPort());

        ITERATE(std::list<std::string>, queue, it->queues) {
            queue_register[*queue].insert(server_address);
        }
    }

    ITERATE(TQueueRegister, it, queue_register) {
        NcbiCout << it->first;
        if (it->second.size() != queues.size()) {
            const char* sep = " (limited to ";
            ITERATE(TServerSet, server, it->second) {
                NcbiCout << sep << *server;
                sep = ", ";
            }
            NcbiCout << ")";
        }
        NcbiCout << NcbiEndl;
    }

    return 0;
}

int CGridCommandLineInterfaceApp::Cmd_DeleteQueue()
{
    SetUp_NetScheduleCmd(eNetScheduleAdmin, eSevereAdminCmd);

    m_NetScheduleAdmin.DeleteQueue(m_Opts.id);

    return 0;
}

END_NCBI_SCOPE
