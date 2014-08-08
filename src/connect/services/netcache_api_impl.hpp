#ifndef CONNECT_SERVICES___NETCACHE_API_IMPL__HPP
#define CONNECT_SERVICES___NETCACHE_API_IMPL__HPP

/*  $Id: netcache_api_impl.hpp 395946 2013-04-15 16:43:31Z kazimird $
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
 * Author:  Dmitry Kazimirov
 *
 * File Description:
 */

#include "netcache_rw.hpp"

BEGIN_NCBI_SCOPE

class NCBI_XCONNECT_EXPORT CNetCacheServerListener :
    public INetServerConnectionListener
{
public:
    virtual void OnInit(CObject* api_impl,
        CConfig* config, const string& config_section);
    virtual void OnConnected(CNetServerConnection::TInstance conn_impl);
    virtual void OnError(const string& err_msg, SNetServerImpl* server);
    virtual void OnWarning(const string& warn_msg, SNetServerImpl* server);

    string m_Auth;
    CRef<INetEventHandler> m_EventHandler;
};

struct NCBI_XCONNECT_EXPORT SNetCacheAPIImpl : public CObject
{
    SNetCacheAPIImpl(CConfig* config, const string& section,
        const string& service, const string& client_name);

    // For use by SNetICacheClientImpl
    SNetCacheAPIImpl(SNetServiceImpl* service_impl) : m_Service(service_impl) {}

    // Special constructor for CNetCacheAPI::GetServer().
    SNetCacheAPIImpl(SNetServerInPool* server, SNetCacheAPIImpl* parent);

    IReader* GetPartReader(
        const string& blob_id,
        size_t offset,
        size_t part_size,
        size_t* blob_size,
        CNetCacheAPI::ECachingMode caching_mode);

    static CNetCacheAPI::EReadResult ReadBuffer(
        IReader& reader,
        char* buf_ptr,
        size_t buf_size,
        size_t* n_read,
        size_t blob_size);

    CNetServer GetServer(const CNetCacheKey& key)
    {
        return m_Service.GetServer(key.GetHost(), key.GetPort());
    }

    virtual CNetServerConnection InitiateWriteCmd(CNetCacheWriter* nc_writer);

    void SetPassword(const string& password);
    void AppendClientIPSessionIDPassword(string* cmd);
    string MakeCmd(const char* cmd);
    string MakeCmd(const char* cmd_base, const CNetCacheKey& key);
    CNetService FindOrCreateService(const string& service_name);

    CNetServer::SExecResult ExecMirrorAware(
        const CNetCacheKey& key, const string& cmd,
        SNetServiceImpl::EServerErrorHandling error_handling =
            SNetServiceImpl::eRethrowServerErrors);

    CNetCacheServerListener* GetListener()
    {
        return static_cast<CNetCacheServerListener*>(
                m_Service->m_Listener.GetPointer());
    }

    CNetService m_Service;

    typedef map<string, CNetService> TNetServiceByName;

    TNetServiceByName m_ServicesFromKeys;

    string m_TempDir;
    bool m_CacheInput;
    bool m_CacheOutput;

    CNetCacheAPI::EMirroringMode m_MirroringMode;

    string m_Password;
};

struct SNetCacheAdminImpl : public CObject
{
    SNetCacheAdminImpl(SNetCacheAPIImpl* nc_api_impl);

    CNetCacheAPI m_API;
};

inline SNetCacheAdminImpl::SNetCacheAdminImpl(SNetCacheAPIImpl* nc_api_impl) :
    m_API(nc_api_impl)
{
}

END_NCBI_SCOPE

#endif  /* CONNECT_SERVICES___NETCACHE_API_IMPL__HPP */
