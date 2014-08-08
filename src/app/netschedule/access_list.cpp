/*  $Id: access_list.cpp 373158 2012-08-27 13:16:23Z satskyse $
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
 *   NetSchedule access list.
 */
#include <ncbi_pch.hpp>

#include "access_list.hpp"
#include "ns_handler.hpp"

#include <connect/ncbi_socket.hpp>

BEGIN_NCBI_SCOPE


// is host allowed to connect
bool CNetScheduleAccessList::IsAllowed(unsigned ha) const
{
    CReadLockGuard guard(m_Lock);

    if (m_Hosts.any() == false)
        return true;

    return m_Hosts[ha];
}


// Delimited lists of hosts allowed into the system
string CNetScheduleAccessList::SetHosts(const string& host_names)
{
    string              accepted_hosts;
    vector<string>      hosts;
    NStr::Tokenize(host_names, ";, \n\r", hosts, NStr::eMergeDelims);

    CWriteLockGuard guard(m_Lock);
    TNSBitVector        old_hosts = m_Hosts;
    m_Hosts.clear();

    ITERATE(vector<string>, it, hosts) {
        const string &      hn = *it;

        if (NStr::CompareNocase(hn, "localhost") == 0) {
            string          my_name = CSocketAPI::gethostname();
            unsigned int    ha = CSocketAPI::gethostbyname(my_name);

            if (ha != 0) {
                m_Hosts.set_bit(ha, true);
                if (!accepted_hosts.empty())
                    accepted_hosts += ", ";
                accepted_hosts += my_name;
                continue;
            }
        }

        unsigned int        ha = CSocketAPI::gethostbyname(hn);
        if (ha != 0) {
            m_Hosts.set_bit(ha, true);
            if (!accepted_hosts.empty())
                accepted_hosts += ", ";
            accepted_hosts += hn;
        }
        else
            ERR_POST("'" << hn << "' is not a valid host name. Ignored.");
    }

    if (old_hosts == m_Hosts)
        return "";
    return accepted_hosts;
}


// Used to print as a comma separated string - part of output
// or as output in the socket with leading 'OK:'
string CNetScheduleAccessList::Print(const string &  prefix,
                                     const string &  separator) const
{
    string                      s;
    CReadLockGuard              guard(m_Lock);
    TNSBitVector::enumerator    en(m_Hosts.first());

    for(; en.valid(); ++en) {
        if (!s.empty())
            s += separator;
        s += prefix + CSocketAPI::gethostbyaddr(*en);
    }
    return s;
}


END_NCBI_SCOPE

