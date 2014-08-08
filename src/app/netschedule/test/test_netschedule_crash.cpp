/*  $Id: test_netschedule_crash.cpp 385218 2013-01-07 20:52:45Z satskyse $
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
 * File Description:  NetSchedule crash test. Used to run many instances of it
 *                    to simulate load for the netscheduled daemon.
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbireg.hpp>
#include <corelib/ncbi_system.hpp>
#include <corelib/ncbimisc.hpp>

#include <connect/services/netschedule_api.hpp>
#include <connect/services/netschedule_key.hpp>
#include <connect/ncbi_socket.hpp>
#include <connect/ncbi_core_cxx.hpp>
#include <connect/ncbi_types.h>

#include <sys/types.h>
#include <unistd.h>


USING_NCBI_SCOPE;


///////////////////////////////////////////////////////////////////////

#define WORKER_NODE_PORT 9595

/// Test application
///
/// @internal
///
class CTestNetScheduleCrash : public CNcbiApplication
{
public:
    void Init(void);
    int Run(void);

    void GetStatus( CNetScheduleExecutor &        executor,
                    const vector<unsigned int> &  jobs );
    void GetReturn( vector<CNetScheduleAPI *> & clients,
                    unsigned int                count,
                    unsigned int                nclients );
    vector<unsigned int>  Submit( vector<CNetScheduleAPI *> & clients,
                                  const string &              service,
                                  unsigned int                jcount,
                                  unsigned int                naff,
                                  unsigned int                ngroup,
                                  unsigned int                notif_port,
                                  unsigned int                nclients,
                                  const string &              queue );
    vector<unsigned int>  GetDone( vector<CNetScheduleAPI *> &  clients,
                                   unsigned int                 nclients );
    void  MainLoop( CNetScheduleSubmitter &  submitter,
                    CNetScheduleExecutor &   executor,
                    unsigned int             jcount,
                    const string &           queue);

    CTestNetScheduleCrash() :
        m_KeyGenerator(NULL)
    {}

    ~CTestNetScheduleCrash()
    {
        if (m_KeyGenerator)
            delete m_KeyGenerator;
    }
private:
    CNetScheduleKeyGenerator *  m_KeyGenerator;
};



void CTestNetScheduleCrash::Init(void)
{
    // Avoid sockets to stay in TIME_WAIT state
    GetConfig().Set("netservice_api", "use_linger2", "true",
                    IRegistry::fNoOverride);

    //
    CONNECT_Init();
    SetDiagPostFlag(eDPF_Trace);
    SetDiagPostLevel(eDiag_Info);

    // Setup command line arguments and parameters

    // Create command-line argument descriptions class
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    // Specify USAGE context
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              "NetSchedule crash test prog='test 1.0'");

    arg_desc->AddKey("service",
                     "service_name",
                     "NetSchedule service name (format: host:port or servcie_name).",
                     CArgDescriptions::eString);

    arg_desc->AddKey("queue",
                     "queue_name",
                     "NetSchedule queue name (like: noname).",
                     CArgDescriptions::eString);

    arg_desc->AddOptionalKey("jobs",
                             "jobs",
                             "Number of jobs to submit",
                             CArgDescriptions::eInteger);

    arg_desc->AddOptionalKey("delay",
                             "delay",
                             "Number of seconds between operations",
                             CArgDescriptions::eInteger);

    arg_desc->AddOptionalKey("naff",
                             "naff",
                             "Number of affinities",
                             CArgDescriptions::eInteger);

    arg_desc->AddOptionalKey("ngroup",
                             "ngroup",
                             "Number of groups",
                             CArgDescriptions::eInteger);

    arg_desc->AddOptionalKey("notifport",
                             "notifport",
                             "If given then jobs are submitted with notif port"
                             " and 100 sec notif timeout",
                             CArgDescriptions::eInteger);

    arg_desc->AddOptionalKey("batch",
                             "batch",
                             "Max number of jobs in a loop",
                             CArgDescriptions::eInteger);

    arg_desc->AddOptionalKey("nclients",
                             "nclients",
                             "Number of clients to simulate",
                             CArgDescriptions::eInteger);

    arg_desc->AddOptionalKey("main",
                             "main",
                             "Main loop (submit-get-put) only",
                             CArgDescriptions::eBoolean);

    // Setup arg.descriptions for this application
    SetupArgDescriptions(arg_desc.release());
}



vector<unsigned int>
CTestNetScheduleCrash::Submit( vector<CNetScheduleAPI *> &  clients,
                               const string &               service,
                               unsigned int                 jcount,
                               unsigned int                 naff,
                               unsigned int                 ngroup,
                               unsigned int                 notif_port,
                               unsigned int                 nclients,
                               const string &               queue )
{
    string                  input = "Crash test for " + queue;
    vector<unsigned int>    jobs;
    unsigned int            caff = 0;
    unsigned int            cgroup = 0;
    unsigned int            client = 0;
    string                  aff = "";
    string                  group = "";


    CNetServer              server = clients[0]->GetService().Iterate().GetServer();
    CNetScheduleSubmitter   submitter = clients[0]->GetSubmitter();


    jobs.reserve(jcount);
    NcbiCout << "Submit " << jcount << " jobs..." << NcbiEndl;

    CStopWatch      sw(CStopWatch::eStart);
    for (unsigned i = 0; i < jcount; ++i) {

        if (nclients > 0) {
            server = clients[client]->GetService().Iterate().GetServer();
            submitter = clients[client]->GetSubmitter();
            if (++client >= nclients)
                client = 0;
        }


        if (naff > 0) {
            char        buffer[ 16 ];
            sprintf( buffer, "aff%d", caff++ );
            if (caff >= naff)
                caff = 0;
            aff = string( buffer );
        }

        if (ngroup > 0) {
            char        buffer[ 16 ];
            sprintf( buffer, "group%d", cgroup++ );
            if (cgroup >= ngroup)
                cgroup = 0;
            group = string( buffer );
        }

        // Two options here: with/without notifications
        if (notif_port == 0) {
            CNetScheduleJob         job(input);
            job.affinity = aff;
            job.group = group;
            submitter.SubmitJob(job);

            CNetScheduleKey     key( job.job_id );
            jobs.push_back( key.id );
            if (m_KeyGenerator == NULL)
                m_KeyGenerator = new CNetScheduleKeyGenerator( key.host,
                                                               key.port,
                                                               queue );
        }
        else {
            // back door - use manually formed command
            char    buffer[ 1024 ];
            sprintf( buffer,
                     "SUBMIT input=\"Crash test for %s\" port=%d timeout=100",
                     queue.c_str(), notif_port );
            string  cmd = string( buffer );
            if (aff.empty() == false)
                cmd += " aff=" + aff;
            if (group.empty() == false)
                cmd += " group=" + group;
            CNetServer::SExecResult     result = server.ExecWithRetry( cmd );

            // Dirty hack: 'OK:' need to be stripped
            CNetScheduleKey     key( result.response.c_str() );
            jobs.push_back( key.id );
            if (m_KeyGenerator == NULL)
                m_KeyGenerator = new CNetScheduleKeyGenerator( key.host,
                                                               key.port,
                                                               queue );
        }


        if (i % 1000 == 0) {
            NcbiCout << "." << flush;
        }

    }
    double          elapsed = sw.Elapsed();
    double          avg = elapsed / jcount;

    NcbiCout << NcbiEndl << "Done." << NcbiEndl;

    NcbiCout.setf(IOS_BASE::fixed, IOS_BASE::floatfield);
    NcbiCout << "Elapsed: "  << elapsed << " sec." << NcbiEndl;
    NcbiCout << "Avg time: " << avg << " sec." << NcbiEndl;

    return jobs;
}


void CTestNetScheduleCrash::GetStatus( CNetScheduleExecutor &        executor,
                                       const vector<unsigned int> &  jobs )
{
    NcbiCout << NcbiEndl
             << "Get status of " << jobs.size() << " jobs..." << NcbiEndl;

    //CNetScheduleAPI::EJobStatus         status;
    unsigned                            i = 0;
    CStopWatch                          sw(CStopWatch::eStart);

    ITERATE(vector<unsigned int>, it, jobs) {
        unsigned int        job_id = *it;

        //status =
        executor.GetJobStatus(m_KeyGenerator->Generate(job_id));
        if (i++ % 1000 == 0) {
            NcbiCout << "." << flush;
        }
    }
    double      elapsed = sw.Elapsed();
    double      avg = elapsed / (double)jobs.size();

    NcbiCout.setf(IOS_BASE::fixed, IOS_BASE::floatfield);
    NcbiCout << NcbiEndl
             << "Elapsed :"  << elapsed << " sec." << NcbiEndl;
    NcbiCout << "Avg time :" << avg << " sec." << NcbiEndl;
    return;
}


/* Returns up to count jobs */
void  CTestNetScheduleCrash::GetReturn( vector<CNetScheduleAPI *> & clients,
                                        unsigned int                count,
                                        unsigned int                nclients )
{
    NcbiCout << NcbiEndl << "Take and Return " << count << " jobs..." << NcbiEndl;

    typedef pair<string, string> TJobIdAuthTokenPair;
    vector<TJobIdAuthTokenPair>    jobs_returned;
    jobs_returned.reserve(count);
    unsigned int            client = 0;
    CNetScheduleExecutor    executor = clients[0]->GetExecutor();

    unsigned        cnt = 0;
    CStopWatch      sw(CStopWatch::eStart);

    for (; cnt < count; ++cnt) {
        if (nclients > 0) {
            executor = clients[client]->GetExecutor();
            if (++client >= nclients)
                client = 0;
        }

        CNetScheduleJob     job;
        bool                job_exists = executor.GetJob(job);

        if (!job_exists)
            continue;

        jobs_returned.push_back(
                TJobIdAuthTokenPair(job.job_id, job.auth_token));
    }
    ITERATE(vector<TJobIdAuthTokenPair>, it, jobs_returned) {
        try {
            executor.ReturnJob(it->first, it->second);
        }
        catch (CException& e)
        {
            NcbiCout << e.what() << NcbiEndl;
        }
    }
    double          elapsed = sw.Elapsed();

    if (cnt > 0) {
        double          avg = elapsed / cnt;

        NcbiCout << "Returned " << cnt << " jobs." << NcbiEndl;

        NcbiCout.setf(IOS_BASE::fixed, IOS_BASE::floatfield);
        NcbiCout << "Jobs processed: " << cnt << NcbiEndl;
        NcbiCout << "Elapsed: "  << elapsed << " sec." << NcbiEndl;
        NcbiCout << "Avg time: " << avg << " sec." << NcbiEndl;
    }
    else
        NcbiCout << "Returned 0 jobs." << NcbiEndl;

    return;
}


vector<unsigned int>
CTestNetScheduleCrash::GetDone( vector<CNetScheduleAPI *> & clients,
                                unsigned int                nclients )
{
    unsigned                cnt = 0;
    vector<unsigned int>    jobs_processed;
    jobs_processed.reserve(10000);
    unsigned int            client = 0;

    CNetScheduleExecutor    executor = clients[0]->GetExecutor();


    NcbiCout << NcbiEndl << "Processing..." << NcbiEndl;

    CStopWatch      sw(CStopWatch::eStart);
    for (; 1; ++cnt) {
        if (nclients > 0) {
            executor = clients[client]->GetExecutor();
            if (++client >= nclients)
                client = 0;
        }

        CNetScheduleJob     job;
        bool                job_exists = executor.GetJob(job);
        if (!job_exists)
            break;

        CNetScheduleKey     key( job.job_id );
        jobs_processed.push_back( key.id );

        job.output = "JOB DONE ";
        executor.PutResult(job);

        if (cnt % 1000 == 0)
            NcbiCout << "." << flush;
    }
    double      elapsed = sw.Elapsed();

    if (cnt) {
        double      avg = elapsed / cnt;

        NcbiCout.setf(IOS_BASE::fixed, IOS_BASE::floatfield);
        NcbiCout << "Jobs processed: " << cnt << NcbiEndl;
        NcbiCout << "Elapsed: "  << elapsed << " sec." << NcbiEndl;
        NcbiCout << "Avg time: " << avg << " sec." << NcbiEndl;
    }
    else
        NcbiCout << "Processed 0 jobs." << NcbiEndl;

    return jobs_processed;
}


void CTestNetScheduleCrash::MainLoop( CNetScheduleSubmitter &  submitter,
                                      CNetScheduleExecutor &   executor,
                                      unsigned int             jcount,
                                      const string &           queue)
{
    NcbiCout << NcbiEndl << "Loop of SUBMIT->GET->PUT for "
             << jcount << " jobs. Each dot denotes 1000 loops." << NcbiEndl;

    CNetScheduleJob         job( "SUBMIT->GET->PUT loop test input" );
    string                  job_key;

    CStopWatch              sw( CStopWatch::eStart );
    for (unsigned int  k = 0; k < jcount; ++k ) {
        submitter.SubmitJob( job );

        if ( ! executor.GetJob( job ) )
            continue;

        job.output = "JOB DONE";
        executor.PutResult( job );

        if (k % 1000 == 0) {
            NcbiCout << "." << flush;
        }
    }
    double  elapsed = sw.Elapsed();

    double  avg = elapsed / jcount;
    NcbiCout.setf(IOS_BASE::fixed, IOS_BASE::floatfield);
    NcbiCout << NcbiEndl
             << "Jobs processed: " << jcount << NcbiEndl;
    NcbiCout << "Elapsed: "  << elapsed << " sec." << NcbiEndl;
    NcbiCout << "Avg time: " << avg << " sec." << NcbiEndl;
    return;
}


int CTestNetScheduleCrash::Run(void)
{
    const CArgs &       args = GetArgs();
    const string &      service  = args["service"].AsString();
    const string &      queue = args["queue"].AsString();

    unsigned            total_jobs = 10000;
    if (args["jobs"])
        total_jobs = args["jobs"].AsInteger();

    unsigned int        delay = 10;
    if (args["delay"])
        delay = args["delay"].AsInteger();

    unsigned int        naff = 0;
    if (args["naff"])
        naff = args["naff"].AsInteger();

    unsigned int        ngroup = 0;
    if (args["ngroup"])
        ngroup = args["ngroup"].AsInteger();

    unsigned int        notif_port = 0;
    if (args["notifport"])
        notif_port = args["notifport"].AsInteger();

    unsigned int        nclients = 0;
    if (args["nclients"])
        nclients = args["nclients"].AsInteger();

    unsigned int        batch = 10000;
    if (args["batch"]) {
        batch = args["batch"].AsInteger();
        if (batch == 0)
            batch = 10000;
    }
    bool                main_only = args["main"];


    vector<CNetScheduleAPI *>       clients;
    pid_t                           pid = getpid();

    if (nclients > 0 && main_only == false) {
        for (unsigned int  k = 0; k < nclients; ++k) {
            char        buffer[ 64 ];
            sprintf( buffer, "node_%d_%d", pid, k );

            CNetScheduleAPI *   cl = new CNetScheduleAPI(service, "crash_test", queue);
            cl->SetProgramVersion("crash_test wn 1.0.4");
            cl->SetClientNode( buffer );
            cl->SetClientSession("crash_test_session");

            clients.push_back(cl);
        }
    } else {
        char        buffer[ 64 ];
        sprintf( buffer, "node_%d", pid );
        CNetScheduleAPI *   cl = new CNetScheduleAPI(service, "crash_test", queue);
        cl->SetProgramVersion("crash_test wn 1.0.4");
        cl->SetClientNode( buffer );
        cl->SetClientSession("crash_test_session");

        clients.push_back(cl);
    }

    // At least one exists for sure
    clients[0]->GetAdmin().PrintServerVersion(NcbiCout);

    CNetScheduleSubmitter               submitter = clients[0]->GetSubmitter();
    CNetScheduleExecutor                executor = clients[0]->GetExecutor();

    if (naff > 0) {
        // Enable retrieving affinities
        executor.SetAffinityPreference(
                        CNetScheduleExecutor::eClaimNewPreferredAffs);
    }

    if (main_only) {
        this->MainLoop(submitter, executor, total_jobs, queue);
        delete clients[0];
        return 0;
    }


    unsigned int    jcount;
    while (total_jobs > 0) {
        if (total_jobs < batch)
            jcount = total_jobs;
        else
            jcount = batch;
        total_jobs -= jcount;

        /* ----- Submit jobs ----- */
        vector<unsigned int>    jobs = this->Submit(clients, service, jcount, naff,
                                                    ngroup, notif_port,
                                                    nclients, queue);


        NcbiCout << NcbiEndl << "Waiting " << delay << " second(s) ..." << NcbiEndl;
        SleepMilliSec(delay * 1000);
        NcbiCout << NcbiEndl << "Ok." << NcbiEndl;


        /* ----- Get status ----- */
        this->GetStatus(executor, jobs);


        NcbiCout << NcbiEndl << "Waiting " << delay << " second(s) ..." << NcbiEndl;
        SleepMilliSec(delay * 1000);
        NcbiCout << NcbiEndl << "Ok." << NcbiEndl;


        /* ----- Get and return some jobs ----- */
        this->GetReturn(clients, jcount/100, nclients);


        NcbiCout << NcbiEndl << "Waiting " << delay << " second(s) ..." << NcbiEndl;
        SleepMilliSec(delay * 1000);
        NcbiCout << NcbiEndl << "Ok." << NcbiEndl;


        /* ----- Get status ----- */
        this->GetStatus(executor, jobs);


        NcbiCout << NcbiEndl << "Waiting " << delay << " second(s) ..." << NcbiEndl;
        SleepMilliSec(delay * 1000);
        NcbiCout << NcbiEndl << "Ok." << NcbiEndl;


        /* ----- Get jobs and say they are done ----- */
        vector<unsigned int> jobs_processed = this->GetDone(clients, nclients);


        NcbiCout << NcbiEndl << "Waiting " << delay << " second(s) ..." << NcbiEndl;
        SleepMilliSec(delay * 1000);
        NcbiCout << NcbiEndl << "Ok." << NcbiEndl;


        /* ----- Get status ----- */
        this->GetStatus(executor, jobs);
    }

    for (vector<CNetScheduleAPI *>::iterator  k = clients.begin();
         k != clients.end(); ++k)
        delete *k;

    return 0;
}


int main(int argc, const char* argv[])
{
    return CTestNetScheduleCrash().AppMain(argc, argv, 0, eDS_Default);
}

