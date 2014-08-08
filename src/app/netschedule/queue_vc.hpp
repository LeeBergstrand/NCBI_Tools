#ifndef NETSCHEDULE_QUEUE_VC__HPP
#define NETSCHEDULE_QUEUE_VC__HPP


/*  $Id: queue_vc.hpp 364199 2012-05-23 16:21:25Z satskyse $
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
 * Authors:  Anatoliy Kuznetsov
 *
 * File Description:
 *   Net schedule queue client version control
 *
 */
/// @file queue_vc.hpp
/// NetSchedule queue client version control
///
/// @internal

#include <corelib/version.hpp>
#include <corelib/ncbistr.hpp>

BEGIN_NCBI_SCOPE

/// Netschedule queue client info
///
/// @internal
///
struct CQueueClientInfo
{
    string         client_name;
    CVersionInfo   version_info;

    CQueueClientInfo() : version_info(-1,-1,-1)
    {}

    CQueueClientInfo(const string& cname, const CVersionInfo& vinfo)
        : client_name(cname), version_info(vinfo)
    {}
};


/// All clients registered to connect
///
/// @internal
///
class CQueueClientInfoList
{
public:
    CQueueClientInfoList()
    {}

    void AddClientInfo(const CQueueClientInfo& cinfo)
    {
        CWriteLockGuard     guard(m_Lock);
        x_AddClientInfo_NoLock(cinfo);
    }

    void AddClientInfo(const string& program_name)
    {
        CWriteLockGuard     guard(m_Lock);
        list<string>        programs;
        CQueueClientInfo    program_info;

        NStr::Split(program_name, ";,", programs);
        ITERATE(list<string>, it, programs) {
            const string &  vstr = *it;
            try {
                ParseVersionString(vstr, &program_info.client_name,
                                         &program_info.version_info);
                NStr::TruncateSpacesInPlace(program_info.client_name);
                x_AddClientInfo_NoLock(program_info);
            }
            catch (CStringException&) {
                LOG_POST(Message << Warning << "Error while parsing program "
                                               "name '" << program_name <<
                                               "'. Program string '" << vstr <<
                                               "'" << " cannot be parsed "
                                               "and will be ignored.");
            }
        }
    }

    bool IsMatchingClient(const CQueueClientInfo& cinfo) const
    {
        CReadLockGuard guard(m_Lock);

        if (m_RegisteredClients.empty())
            return true;

        ITERATE(vector<CQueueClientInfo>, it, m_RegisteredClients) {
            if (NStr::CompareNocase(cinfo.client_name, it->client_name)==0) {
                if (it->version_info.IsAny())
                    return true;

                if (cinfo.version_info.IsUpCompatible(it->version_info))
                    return true;
            }
        }
        return false;
    }

    bool IsConfigured() const
    {
        CReadLockGuard guard(m_Lock);
        return !m_RegisteredClients.empty();
    }

    void Clear()
    {
        CWriteLockGuard guard(m_Lock);
        m_RegisteredClients.resize(0);
    }

    string Print(const char* sep=",") const
    {
        string s;
        ITERATE(vector<CQueueClientInfo>, it, m_RegisteredClients) {
            if (!s.empty())
                s += sep;

            s += it->client_name + ' ' + it->version_info.Print();
        }
        return s;
    }

private:
    void x_AddClientInfo_NoLock(const CQueueClientInfo& cinfo)
    {
        m_RegisteredClients.push_back(cinfo);
    }

private:
    vector<CQueueClientInfo>   m_RegisteredClients;
    mutable CRWLock            m_Lock;
};


END_NCBI_SCOPE

#endif /* NETSCHEDULE_QUEUE_VC__HPP */

