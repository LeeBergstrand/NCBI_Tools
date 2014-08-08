/*  $Id: automation.cpp 391224 2013-03-06 16:01:51Z kazimird $
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
 * File Description: Declaration of the automation processor.
 *
 */

#include <ncbi_pch.hpp>

#include "automation.hpp"

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

USING_NCBI_SCOPE;

#define PROTOCOL_VERSION 2

void CArgArray::Exception(const char* what)
{
    if (m_Location.empty()) {
        NCBI_THROW(CAutomationException, eInvalidInput, what);
    } else {
        NCBI_THROW_FMT(CAutomationException, eInvalidInput, m_Location <<
                ": " << what);
    }
}

void SNetCacheServiceAutomationObject::CEventHandler::OnWarning(
        const string& warn_msg, CNetServer server)
{
    m_AutomationProc->SendWarning(warn_msg, m_AutomationProc->
            ReturnNetCacheServerObject(m_NetCacheAPI, server));
}

const string& SNetCacheServiceAutomationObject::GetType() const
{
    static const string object_type("ncsvc");

    return object_type;
}

const void* SNetCacheServiceAutomationObject::GetImplPtr() const
{
    return m_NetCacheAPI;
}

SNetCacheServerAutomationObject::SNetCacheServerAutomationObject(
        CAutomationProc* automation_proc,
        const string& service_name,
        const string& client_name) :
    SNetCacheServiceAutomationObject(automation_proc,
            CNetCacheAPI(service_name, client_name))
{
    if (m_Service.IsLoadBalanced()) {
        NCBI_THROW(CAutomationException, eCommandProcessingError,
                "NetCacheServer constructor: "
                "'server_address' must be a host:port combination");
    }

    m_NetServer = m_Service.Iterate().GetServer();

    m_NetCacheAPI.SetEventHandler(
            new CEventHandler(automation_proc, m_NetCacheAPI));
}

const string& SNetCacheServerAutomationObject::GetType() const
{
    static const string object_type("ncsrv");

    return object_type;
}

const void* SNetCacheServerAutomationObject::GetImplPtr() const
{
    return m_NetServer;
}

const string& SNetScheduleServiceAutomationObject::GetType() const
{
    static const string object_type("nssvc");

    return object_type;
}

const void* SNetScheduleServiceAutomationObject::GetImplPtr() const
{
    return m_NetScheduleAPI;
}

SNetScheduleServerAutomationObject::SNetScheduleServerAutomationObject(
        CAutomationProc* automation_proc,
        const string& service_name,
        const string& queue_name,
        const string& client_name) :
    SNetScheduleServiceAutomationObject(automation_proc,
            CNetScheduleAPI(service_name, client_name, queue_name))
{
    if (m_Service.IsLoadBalanced()) {
        NCBI_THROW(CAutomationException, eCommandProcessingError,
                "NetScheduleServer constructor: "
                "'server_address' must be a host:port combination");
    }

    m_NetServer = m_Service.Iterate().GetServer();

    m_NetScheduleAPI.SetEventHandler(
            new CEventHandler(automation_proc, m_NetScheduleAPI));
}

const string& SNetScheduleServerAutomationObject::GetType() const
{
    static const string object_type("nssrv");

    return object_type;
}

const void* SNetScheduleServerAutomationObject::GetImplPtr() const
{
    return m_NetServer;
}

TAutomationObjectRef CAutomationProc::FindObjectByPtr(const void* impl_ptr) const
{
    TPtrToObjectRefMap::const_iterator it = m_ObjectByPointer.find(impl_ptr);
    return it != m_ObjectByPointer.end() ? it->second : TAutomationObjectRef();
}

TAutomationObjectRef CAutomationProc::ReturnNetCacheServerObject(
        CNetCacheAPI::TInstance ns_api,
        CNetServer::TInstance server)
{
    TAutomationObjectRef object(FindObjectByPtr(server));
    if (!object) {
        object = new SNetCacheServerAutomationObject(this, ns_api, server);
        AddObject(object, server);
    }
    return object;
}

TAutomationObjectRef CAutomationProc::ReturnNetScheduleServerObject(
        CNetScheduleAPI::TInstance ns_api,
        CNetServer::TInstance server)
{
    TAutomationObjectRef object(FindObjectByPtr(server));
    if (!object) {
        object = new SNetScheduleServerAutomationObject(this, ns_api, server);
        AddObject(object, server);
    }
    return object;
}

struct SServerAddressToJson : public IExecToJson
{
    SServerAddressToJson(int which_part) : m_WhichPart(which_part) {}

    virtual CJsonNode ExecOn(CNetServer server);

    int m_WhichPart;
};

CJsonNode SServerAddressToJson::ExecOn(CNetServer server)
{
    switch (m_WhichPart) {
    case 1:
        return CJsonNode::NewStringNode(g_NetService_gethostnamebyaddr(
                server.GetHost()));
    case 2:
        return CJsonNode::NewNumberNode(server.GetPort());
    }
    return CJsonNode::NewStringNode(server.GetServerAddress());
}

bool SNetServiceAutomationObject::Call(const string& method,
        CArgArray& arg_array, CJsonNode& reply)
{
    if (method == "get_name")
        reply.PushString(m_Service.GetServiceName());
    else if (method == "get_address") {
        SServerAddressToJson server_address_proc(arg_array.NextNumber(0));

        reply.PushNode(g_ExecToJson(server_address_proc,
                m_Service, m_ActualServiceType));
    } else if (method == "server_info") {
        reply.PushNode(g_ServerInfoToJson(m_Service, m_ActualServiceType));
    } else if (method == "exec") {
        string command(arg_array.NextString());
        reply.PushNode(g_ExecAnyCmdToJson(m_Service, m_ActualServiceType,
                command, arg_array.NextBoolean(false)));
    } else
#ifdef NCBI_GRID_XSITE_CONN_SUPPORT
        if (method == "allow_xsite_connections")
            m_Service.AllowXSiteConnections();
        else
#endif
            return false;

    return true;
}

bool SNetCacheServiceAutomationObject::Call(const string& method,
        CArgArray& arg_array, CJsonNode& reply)
{
    if (method == "get_servers") {
        CJsonNode object_ids(CJsonNode::NewArrayNode());
        for (CNetServiceIterator it = m_NetCacheAPI.GetService().Iterate();
                it; ++it)
            object_ids.PushNumber(m_AutomationProc->
                    ReturnNetCacheServerObject(m_NetCacheAPI, *it)->GetID());
        reply.PushNode(object_ids);
    } else
        return SNetServiceAutomationObject::Call(method, arg_array, reply);

    return true;
}

bool SNetCacheServerAutomationObject::Call(const string& method,
        CArgArray& arg_array, CJsonNode& reply)
{
    return SNetCacheServiceAutomationObject::Call(method, arg_array, reply);
}

static void ExtractVectorOfStrings(CArgArray& arg_array,
        vector<string>& result)
{
    CJsonNode arg(arg_array.NextNode());
    if (!arg.IsNull())
        ITERATE(CJsonNode::TArray, it, arg.GetArray()) {
            result.push_back(arg_array.GetString(*it));
        }
}

bool SNetScheduleServerAutomationObject::Call(const string& method,
        CArgArray& arg_array, CJsonNode& reply)
{
    if (method == "server_status")
        reply.PushNode(g_LegacyStatToJson(m_NetServer,
                arg_array.NextBoolean(false)));
    else if (method == "job_group_info")
        reply.PushNode(g_GenericStatToJson(m_NetServer,
                eNetScheduleStatJobGroups, arg_array.NextBoolean(false)));
    else if (method == "client_info")
        reply.PushNode(g_GenericStatToJson(m_NetServer,
                eNetScheduleStatClients, arg_array.NextBoolean(false)));
    else if (method == "notification_info")
        reply.PushNode(g_GenericStatToJson(m_NetServer,
                eNetScheduleStatNotifications, arg_array.NextBoolean(false)));
    else if (method == "affinity_info")
        reply.PushNode(g_GenericStatToJson(m_NetServer,
                eNetScheduleStatAffinities, arg_array.NextBoolean(false)));
    else if (method == "change_preferred_affinities") {
        vector<string> affs_to_add;
        ExtractVectorOfStrings(arg_array, affs_to_add);
        vector<string> affs_to_del;
        ExtractVectorOfStrings(arg_array, affs_to_del);
        m_NetScheduleAPI.GetExecutor().ChangePreferredAffinities(
                &affs_to_add, &affs_to_del);
    } else
        return SNetScheduleServiceAutomationObject::Call(
                method, arg_array, reply);

    return true;
}

void SNetScheduleServiceAutomationObject::CEventHandler::OnWarning(
        const string& warn_msg, CNetServer server)
{
    m_AutomationProc->SendWarning(warn_msg, m_AutomationProc->
            ReturnNetScheduleServerObject(m_NetScheduleAPI, server));
}

bool SNetScheduleServiceAutomationObject::Call(const string& method,
        CArgArray& arg_array, CJsonNode& reply)
{
    if (method == "set_node_session") {
        CJsonNode arg(arg_array.NextNode());
        if (!arg.IsNull())
            m_NetScheduleAPI.SetClientNode(arg_array.GetString(arg));
        arg = arg_array.NextNode();
        if (!arg.IsNull())
            m_NetScheduleAPI.SetClientSession(arg_array.GetString(arg));
    } else if (method == "queue_info")
        reply.PushNode(g_QueueInfoToJson(m_NetScheduleAPI,
                arg_array.NextString(kEmptyStr), m_ActualServiceType));
    else if (method == "queue_class_info")
        reply.PushNode(g_QueueClassInfoToJson(m_NetScheduleAPI,
                m_ActualServiceType));
    else if (method == "reconf")
        reply.PushNode(g_ReconfAndReturnJson(m_NetScheduleAPI,
                m_ActualServiceType));
    else if (method == "parse_key") {
        CJobInfoToJSON job_key_to_json;
        job_key_to_json.ProcessJobMeta(CNetScheduleKey(arg_array.NextString()));
        reply.PushNode(job_key_to_json.GetRootNode());
    } else if (method == "job_info") {
        CJobInfoToJSON job_info_to_json;
        string job_key(arg_array.NextString());
        ProcessJobInfo(m_NetScheduleAPI, job_key,
            &job_info_to_json, arg_array.NextBoolean(true));
        reply.PushNode(job_info_to_json.GetRootNode());
    } else if (method == "job_counters") {
        CNetScheduleAdmin::TStatusMap status_map;
        string affinity(arg_array.NextString(kEmptyStr));
        string job_group(arg_array.NextString(kEmptyStr));
        m_NetScheduleAPI.GetAdmin().StatusSnapshot(status_map,
                affinity, job_group);
        CJsonNode jobs_by_status(CJsonNode::NewObjectNode());
        ITERATE(CNetScheduleAdmin::TStatusMap, it, status_map) {
            jobs_by_status.SetNumber(it->first, it->second);
        }
        reply.PushNode(jobs_by_status);
    } else if (method == "get_servers") {
        CJsonNode object_ids(CJsonNode::NewArrayNode());
        for (CNetServiceIterator it = m_NetScheduleAPI.GetService().Iterate();
                it; ++it)
            object_ids.PushNumber(m_AutomationProc->
                    ReturnNetScheduleServerObject(m_NetScheduleAPI, *it)->
                    GetID());
        reply.PushNode(object_ids);
    } else
        return SNetServiceAutomationObject::Call(method, arg_array, reply);

    return true;
}

CAutomationProc::CAutomationProc(CPipe& pipe, FILE* protocol_dump) :
    m_Pipe(pipe),
    m_JSONWriter(pipe, m_Writer),
    m_ProtocolDumpFile(protocol_dump),
    m_OKNode(CJsonNode::NewBooleanNode(true)),
    m_ErrNode(CJsonNode::NewBooleanNode(false)),
    m_WarnNode(CJsonNode::NewStringNode("WARNING"))
{
    m_Writer.Reset(m_WriteBuf, sizeof(m_WriteBuf));

    m_Pid = (Int8) CProcess::GetCurrentPid();

    CJsonNode greeting(CJsonNode::NewArrayNode());
    greeting.PushString(GRID_APP_NAME);
    greeting.PushNumber(PROTOCOL_VERSION);

    if (protocol_dump != NULL) {
        string pid_str(NStr::NumericToString(m_Pid));

        m_DumpInputHeaderFormat.assign(1, '\n');
        m_DumpInputHeaderFormat.append(71 + pid_str.length(), '=');
        m_DumpInputHeaderFormat.append("\n----- IN ------ %s (pid: ");
        m_DumpInputHeaderFormat.append(pid_str);
        m_DumpInputHeaderFormat.append(") -----------------------\n");

        m_DumpOutputHeaderFormat.assign("----- OUT ----- %s (pid: ");
        m_DumpOutputHeaderFormat.append(pid_str);
        m_DumpOutputHeaderFormat.append(") -----------------------\n");

        m_ProtocolDumpTimeFormat = "Y/M/D h:m:s.l";
    }

    SendMessage(greeting);
}

TAutomationObjectRef CAutomationProc::CreateObject(const string& class_name,
        CArgArray& arg_array)
{
    const void* impl_ptr;
    TAutomationObjectRef new_object;
    try {
        if (class_name == "ncsvc") {
            string service_name(arg_array.NextString());
            string client_name(arg_array.NextString());
            SNetCacheServiceAutomationObject* ncsvc_object_ptr =
                    new SNetCacheServiceAutomationObject(this,
                            service_name, client_name);
            impl_ptr = ncsvc_object_ptr->m_NetCacheAPI;
            new_object.Reset(ncsvc_object_ptr);
        } else if (class_name == "ncsrv") {
            string service_name(arg_array.NextString());
            string client_name(arg_array.NextString());
            SNetCacheServerAutomationObject* ncsrv_object_ptr =
                    new SNetCacheServerAutomationObject(this,
                            service_name, client_name);
            impl_ptr = ncsrv_object_ptr->m_NetCacheAPI;
            new_object.Reset(ncsrv_object_ptr);
        } else if (class_name == "nssvc") {
            string service_name(arg_array.NextString(kEmptyStr));
            string queue_name(arg_array.NextString(kEmptyStr));
            string client_name(arg_array.NextString(kEmptyStr));
            SNetScheduleServiceAutomationObject* nssvc_object_ptr =
                    new SNetScheduleServiceAutomationObject(this,
                        service_name, queue_name, client_name);
            impl_ptr = nssvc_object_ptr->m_NetScheduleAPI;
            new_object.Reset(nssvc_object_ptr);
        } else if (class_name == "nssrv") {
            string service_name(arg_array.NextString(kEmptyStr));
            string queue_name(arg_array.NextString(kEmptyStr));
            string client_name(arg_array.NextString(kEmptyStr));
            SNetScheduleServiceAutomationObject* nssrv_object_ptr =
                    new SNetScheduleServerAutomationObject(this,
                        service_name, queue_name, client_name);
            impl_ptr = nssrv_object_ptr->m_NetScheduleAPI;
            new_object.Reset(nssrv_object_ptr);
        } else {
            NCBI_THROW_FMT(CAutomationException, eInvalidInput,
                    "Unknown class '" << class_name << "'");
        }
    }
    catch (CException& e) {
        NCBI_THROW_FMT(CAutomationException, eCommandProcessingError,
                "Error in '" << class_name << "' constructor: " << e.GetMsg());
    }
    AddObject(new_object, impl_ptr);
    return new_object;
}

CJsonNode CAutomationProc::ProcessMessage(const CJsonNode& message)
{
    if (m_ProtocolDumpFile != NULL) {
        fprintf(m_ProtocolDumpFile, m_DumpInputHeaderFormat.c_str(),
                GetFastLocalTime().AsString(m_ProtocolDumpTimeFormat).c_str());
        g_PrintJSON(m_ProtocolDumpFile, message);
    }

    CArgArray arg_array(message.GetArray());

    string command(arg_array.NextString());

    arg_array.UpdateLocation(command);

    if (command == "exit")
        return NULL;

    CJsonNode reply(CJsonNode::NewArrayNode());
    reply.PushNode(m_OKNode);

    if (command == "call") {
        TAutomationObjectRef object_ref(ObjectIdToRef(
                (TObjectID) arg_array.NextNumber()));
        string method(arg_array.NextString());
        arg_array.UpdateLocation(method);
        if (!object_ref->Call(method, arg_array, reply)) {
            NCBI_THROW_FMT(CAutomationException, eCommandProcessingError,
                    "Unknown " << object_ref->GetType() <<
                            " method '" << method << "'");
        }
    } else if (command == "new") {
        string class_name(arg_array.NextString());
        arg_array.UpdateLocation(class_name);
        reply.PushNumber(CreateObject(class_name, arg_array)->GetID());
    } else if (command == "del") {
        TAutomationObjectRef& object(ObjectIdToRef(
                (TObjectID) arg_array.NextNumber()));
        m_ObjectByPointer.erase(object->GetImplPtr());
        object = NULL;
    } else {
        arg_array.Exception("unknown command");
    }

    return reply;
}

void CAutomationProc::SendMessage(const CJsonNode& message)
{
    if (m_ProtocolDumpFile != NULL) {
        fprintf(m_ProtocolDumpFile, m_DumpOutputHeaderFormat.c_str(),
                GetFastLocalTime().AsString(m_ProtocolDumpTimeFormat).c_str());
        g_PrintJSON(m_ProtocolDumpFile, message);
    }

    m_JSONWriter.SendMessage(message);
}

void CAutomationProc::SendWarning(const string& warn_msg,
        TAutomationObjectRef source)
{
    CJsonNode warning(CJsonNode::NewArrayNode());
    warning.PushNode(m_WarnNode);
    warning.PushString(warn_msg);
    warning.PushString(source->GetType());
    warning.PushNumber(source->GetID());
    SendMessage(warning);
}

void CAutomationProc::SendError(const CTempString& error_message)
{
    CJsonNode error(CJsonNode::NewArrayNode());
    error.PushNode(m_ErrNode);
    error.PushString(error_message);
    SendMessage(error);
}

TAutomationObjectRef& CAutomationProc::ObjectIdToRef(TObjectID object_id)
{
    size_t index = (size_t) (object_id - m_Pid);

    if (index >= m_ObjectByIndex.size() || !m_ObjectByIndex[index]) {
        NCBI_THROW_FMT(CAutomationException, eCommandProcessingError,
                "Object with id '" << object_id << "' does not exist");
    }
    return m_ObjectByIndex[index];
}

int CGridCommandLineInterfaceApp::Cmd_Automate()
{
    CPipe pipe;

    pipe.OpenSelf();

#ifdef WIN32
    setmode(fileno(stdin), O_BINARY);
    setmode(fileno(stdout), O_BINARY);
#endif

    size_t bytes_read;

    char read_buf[1024];

    CUTTPReader reader;
    CJsonOverUTTPReader suttp_reader;

    CAutomationProc proc(pipe, m_Opts.protocol_dump);

    while (pipe.Read(read_buf, sizeof(read_buf), &bytes_read) == eIO_Success) {
        reader.SetNewBuffer(read_buf, bytes_read);

        CJsonOverUTTPReader::EParsingEvent ret_code =
            suttp_reader.ProcessParsingEvents(reader);

        switch (ret_code) {
        case CJsonOverUTTPReader::eEndOfMessage:
            try {
                CJsonNode reply(proc.ProcessMessage(suttp_reader.GetMessage()));

                if (!reply)
                    return 0;

                proc.SendMessage(reply);
            }
            catch (CAutomationException& e) {
                switch (e.GetErrCode()) {
                case CAutomationException::eInvalidInput:
                    proc.SendError(e.GetMsg());
                    return 2;
                default:
                    proc.SendError(e.GetMsg());
                }
            }
            catch (CException& e) {
                proc.SendError(e.GetMsg());
            }

            suttp_reader.Reset();

            /* FALL THROUGH */

        case CJsonOverUTTPReader::eNextBuffer:
            break;

        default: // Parsing error
            return int(ret_code);
        }
    }

    return 0;
}
