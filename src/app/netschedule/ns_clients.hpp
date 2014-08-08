#ifndef NETSCHEDULE_CLIENTS__HPP
#define NETSCHEDULE_CLIENTS__HPP

/*  $Id: ns_clients.hpp 389403 2013-02-15 16:38:48Z satskyse $
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
 * Authors:  Sergey Satskiy
 *
 * File Description:
 *   NetSchedule clients registry supporting facilities
 *
 */

#include <connect/services/netservice_protocol_parser.hpp>

#include "ns_types.hpp"
#include "ns_access.hpp"

#include <string>


BEGIN_NCBI_SCOPE

// Forward declaration
class CQueue;
class CNSAffinityRegistry;
class CJobStatusTracker;


// The CClientId serves two types of clients:
// - old style clients; they have peer address only
// - new style clients; they have all three pieces,
//                      address, node id and session id
class CNSClientId
{
    public:
        CNSClientId();
        CNSClientId(unsigned int            peer_addr,
                    const TNSProtoParams &  params);

        void Update(unsigned int            peer_addr,
                    const TNSProtoParams &  params);

        // true if it is a new client identification
        bool IsComplete(void) const;

        // Getters/setters
        unsigned int GetAddress(void) const
        { return m_Addr; }
        const string &  GetNode(void) const
        { return m_ClientNode; }
        const string &  GetSession(void) const
        { return m_ClientSession; }
        unsigned short  GetControlPort(void) const
        { return m_ControlPort; }
        const string &  GetClientHost(void) const
        { return m_ClientHost; }
        const string &  GetProgramName(void) const
        { return m_ProgName; }
        const string &  GetClientName(void) const
        { return m_ClientName; }
        unsigned int GetCapabilities(void) const
        { return m_Capabilities; }
        void SetCapabilities(unsigned int  capabilities)
        { m_Capabilities = capabilities; }
        void SetClientName(const string &  client_name);
        void SetClientHost(const string &  client_host)
        { m_ClientHost = client_host; }
        void SetControlPort(unsigned short  port)
        { m_ControlPort = port; }
        void AddCapability(unsigned int  capabilities)
        { m_Capabilities |= capabilities; }
        void ResetCapabilities(void)
        { m_Capabilities = 0; }
        void RemoveCapability(unsigned int  capabilities)
        { m_Capabilities &= ~capabilities; }
        void CheckAccess(TNSClientRole  role, const CQueue *  queue);
        bool CheckVersion(const CQueue *  queue);
        unsigned int GetID(void) const
        { return m_ID; }
        void SetID(unsigned int  id)
        { m_ID = id; }

    private:
        static string x_AccessViolationMessage(unsigned int  deficit);

    private:
        unsigned int        m_Addr;           // Client peer address
        string              m_ProgName;       // Program name - formed by API
                                              // and usually is an exe name
        string              m_ClientName;     // Client name - taken from the
                                              // app config file
        string              m_ClientNode;     // Client node,
                                              // e.g. service10:9300
        string              m_ClientSession;  // Session of working
                                              //  with netschedule.
        unsigned short      m_ControlPort;    // Client control port
        string              m_ClientHost;     // Client host name if passed in
                                              // the handshake line.

        // Capabilities - that is combination of ENSAccess
        // rights, which can be performed by this connection
        unsigned int        m_Capabilities;

        unsigned int        m_Unreported;
        bool                m_VersionControl;

        // 0 for old style clients
        // non 0 for new style clients.
        // This identifier is set at the moment of touching the clients
        // registry. The id is needed to support affinities. The affinity
        // registry will store IDs of the clients which informed that a certain
        // affinity is preferred.
        unsigned int        m_ID;
};



// The CNSClient stores information about new style clients only;
// The information includes the given jobs,
// type of the client (worker node, submitter) etc.

// Note: access to the instances of this class is possible only via the client
// registry and the corresponding container access is always done under a lock.
// So it is safe to do any modifications in the members without any locks here.
class CNSClient
{
    public:
        // Used for a bit mask to identify what kind of
        // operations the client tried to perform
        enum ENSClientType {
            eSubmitter  = 1,
            eWorkerNode = 2,
            eReader     = 4
        };

    public:
        CNSClient();
        CNSClient(const CNSClientId &  client_id,
                  time_t *             blacklist_timeout);
        bool Clear(void);
        TNSBitVector GetRunningJobs(void) const
        { return m_RunningJobs; }
        TNSBitVector GetReadingJobs(void) const
        { return m_ReadingJobs; }
        TNSBitVector GetBlacklistedJobs(void) const;
        bool IsJobBlacklisted(unsigned int  job_id) const;
        void SetWaitPort(unsigned short  port)
        { m_WaitPort = port; }
        string GetSession(void) const
        { return m_Session; }
        time_t GetRegistrationTime(void) const
        { return m_RegistrationTime; }
        time_t GetSessionStartTime(void) const
        { return m_SessionStartTime; }
        time_t GetSessionResetTime(void) const
        { return m_SessionResetTime; }
        time_t GetLastAccess(void) const
        { return m_LastAccess; }
        void RegisterSocketWriteError(void)
        { ++m_NumberOfSockErrors; }

        unsigned short GetAndResetWaitPort(void);

        void RegisterRunningJob(unsigned int  job_id);
        void RegisterReadingJob(unsigned int  job_id);
        void RegisterSubmittedJobs(size_t  count);
        void RegisterBlacklistedJob(unsigned int  job_id);
        bool MoveReadingJobToBlacklist(unsigned int  job_id);
        bool MoveRunningJobToBlacklist(unsigned int  job_id);
        void UnregisterRunningJob(unsigned int  job_id);
        void UnregisterReadingJob(unsigned int  job_id);

        bool Touch(const CNSClientId &  client_id,
                   TNSBitVector &       running_jobs,
                   TNSBitVector &       reading_jobs);
        string Print(const string &               node_name,
                     const CQueue *               queue,
                     const CNSAffinityRegistry &  aff_registry,
                     bool                         verbose) const;
        unsigned int GetID(void) const
        { return m_ID; }
        void SetID(unsigned int  id)
        { m_ID = id; }
        unsigned int GetType(void) const
        { return m_Type; }
        void AppendType(unsigned int  type_to_append)
        { m_Type |= type_to_append; }
        TNSBitVector  GetPreferredAffinities(void) const
        { return m_Affinities; }
        void  AddPreferredAffinities(const TNSBitVector &  aff);
        void  AddPreferredAffinity(unsigned int  aff);
        void  RemovePreferredAffinities(const TNSBitVector &  aff);
        void  RemovePreferredAffinity(unsigned int  aff);
        void  SetPreferredAffinities(const TNSBitVector &  aff);

        // WGET support
        void  RegisterWaitAffinities(const TNSBitVector &  aff);
        TNSBitVector  GetWaitAffinities(void) const
        { return m_WaitAffinities; }
        bool  IsRequestedAffinity(const TNSBitVector &  aff,
                                  bool                  use_preferred) const;
        void  ClearWaitAffinities(void)
        { m_WaitAffinities.clear(); }
        bool  ClearPreferredAffinities(void);

        bool  GetAffinityReset(void) const
        { return m_AffReset; }
        void  SetAffinityReset(bool  value)
        { m_AffReset = value; }
        bool  AnyAffinity(void) const
        { return m_Affinities.any(); }
        bool  AnyWaitAffinity(void) const
        { return m_WaitAffinities.any(); }
        unsigned int  GetPeerAddress(void) const
        { return m_Addr; }

        void  CheckBlacklistedJobsExisted(const CJobStatusTracker &  tracker);

    private:
        bool            m_Cleared;        // Set to true when CLRN is received
                                          // If true => m_Session == "n/a"
        unsigned int    m_Type;           // bit mask of ENSClientType
        unsigned int    m_Addr;           // Client peer address
        unsigned short  m_ControlPort;    // Worker node control port
        string          m_ClientHost;     // Client host as given in the
                                          // handshake line.
        time_t          m_RegistrationTime;
        time_t          m_SessionStartTime;
        time_t          m_SessionResetTime;
        time_t          m_LastAccess;     // The last time the client accessed
                                          // netschedule
        string          m_Session;        // Client session id

        TNSBitVector    m_RunningJobs;      // The jobs the client is currently
                                            // executing
        TNSBitVector    m_ReadingJobs;      // The jobs the client is currently
                                            // reading
        mutable
        TNSBitVector    m_BlacklistedJobs;  // The jobs that should not be given
                                            // to the node neither for
                                            // executing nor for reading
        unsigned short  m_WaitPort;         // Port, provided in WGET command or
                                            // 0 otherwise
        unsigned int    m_ID;               // Client identifier, see comments
                                            // for CNSClientId::m_ID
        TNSBitVector    m_Affinities;       // The client preferred affinities
        TNSBitVector    m_WaitAffinities;   // The list of affinities the client
                                            // waits for on WGET
        size_t          m_NumberOfSubmitted;// Number of submitted jobs
        size_t          m_NumberOfRead;     // Number of jobs given for reading
        size_t          m_NumberOfRun;      // Number of jobs given for executing
        bool            m_AffReset;         // true => affinities were reset due to
                                            // client inactivity timeout
        size_t          m_NumberOfSockErrors;

        time_t *        m_BlacklistTimeout;
        mutable
        map<unsigned int,
            time_t>     m_BlacklistLimits;  // job id -> last second the job is in
                                            // the blacklist

        string  x_TypeAsString(void) const;
        void    x_AddToBlacklist(unsigned int  job_id);
        void    x_UpdateBlacklist(void) const;
        void    x_UpdateBlacklist(unsigned int  job_id) const;
        time_t  x_GetBlacklistLimit(unsigned int  job_id) const;
        string  x_GetFormattedBlacklistLimit(unsigned int  job_id) const;
};


END_NCBI_SCOPE

#endif /* NETSCHEDULE_CLIENTS__HPP */

