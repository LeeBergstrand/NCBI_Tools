#ifndef NETSCHEDULE_SERVER_PARAMS__HPP
#define NETSCHEDULE_SERVER_PARAMS__HPP

/*  $Id: ns_server_params.hpp 357666 2012-03-26 14:15:00Z satskyse $
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
 * Authors:  Anatoliy Kuznetsov, Victor Joukov
 *
 * File Description: [server] section of the configuration
 *
 */

#include <corelib/ncbireg.hpp>
#include <connect/server.hpp>

#include <string>

BEGIN_NCBI_SCOPE

//////////////////////////////////////////////////////////////////////////
/// Parameters for server
struct SNS_Parameters : SServer_Parameters
{
    bool            reinit;
    unsigned short  port;

    bool            use_hostname;
    unsigned        network_timeout;

    bool            is_log;
    bool            log_batch_each_job;
    bool            log_notification_thread;
    bool            log_cleaning_thread;
    bool            log_execution_watcher_thread;
    bool            log_statistics_thread;

    unsigned int    del_batch_size;     // Max number of jobs to be deleted
                                        // from BDB during one Purge() call
    unsigned int    markdel_batch_size; // Max number of jobs marked for deletion
    unsigned int    scan_batch_size;    // Max number of jobs expiration checks
                                        // during one Purge() call
    double          purge_timeout;      // Timeout in seconds between
                                        // Purge() calls
    unsigned int    max_affinities;     // Max number of affinities a client
                                        // can report as preferred.

    std::string     admin_hosts;
    std::string     admin_client_names;

    // Affinity GC settings
    unsigned int    affinity_high_mark_percentage;
    unsigned int    affinity_low_mark_percentage;
    unsigned int    affinity_high_removal;
    unsigned int    affinity_low_removal;
    unsigned int    affinity_dirt_percentage;

    void Read(const IRegistry& reg, const std::string& sname);
    void CheckAffinityGarbageCollectorSettings(void);
    void CheckGarbageCollectorSettings(void);

    unsigned GetNumParams() const;
    std::string GetParamName(unsigned n) const;
    std::string GetParamValue(unsigned n) const;
};

END_NCBI_SCOPE

#endif

