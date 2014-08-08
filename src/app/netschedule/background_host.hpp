#ifndef NETSCHEDULE_BACKGROUND_HOST__HPP
#define NETSCHEDULE_BACKGROUND_HOST__HPP

/*  $Id: background_host.hpp 282524 2011-05-12 14:59:27Z satskyse $
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
 * File Description: Host interface for background thread. Allows thread to
 *                   report errors and to learn is it needed to execute.
 *
 *
 */

#include <util/thread_pool.hpp>

BEGIN_NCBI_SCOPE

// This interface allows the background thread to find out is it still needed
// to run (with ShouldRun method) and report error in background thread to the
// host, which can log it or process in another reasonable way
// (method ReportError).
class CBackgroundHost
{
public:
    enum ESeverity {
        eWarning = 0,
        eError,
        eFatal
    };
    virtual ~CBackgroundHost() {}
    virtual void ReportError(ESeverity severity, const string& what) = 0;
    virtual bool ShouldRun() = 0;
    virtual bool IsLog() const = 0;
};


// This interface allows its user to submit a (possibly) long-running job
// to a thread pool, where it will be executed by one of worker threads.
// It exists for irregular jobs, which do not justify their own thread and
// do not produce user output.
class CRequestExecutor // better name, please
{
public:
    virtual ~CRequestExecutor() {}
    virtual void SubmitRequest(const CRef<CStdRequest>& request) = 0;
};


END_NCBI_SCOPE

#endif /* NETSCHEDULE_BACKGROUND_HOST__HPP */
