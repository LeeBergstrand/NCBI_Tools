#ifndef WN_COMMIT_THREAD__HPP
#define WN_COMMIT_THREAD__HPP


/*  $Id: wn_commit_thread.hpp 383181 2012-12-12 16:51:50Z kazimird $
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
 * File Description:
 *    NetSchedule Worker Node - job committer thread, declarations.
 */

#include <connect/services/grid_worker.hpp>

BEGIN_NCBI_SCOPE

/////////////////////////////////////////////////////////////////////////////
//
/// @internal
class CRequestContextSwitcher
{
public:
    CRequestContextSwitcher(CRequestContext* new_request_context)
    {
        m_SavedRequestContext = &CDiagContext::GetRequestContext();
        CDiagContext::SetRequestContext(new_request_context);
    }

    ~CRequestContextSwitcher()
    {
        CDiagContext::SetRequestContext(m_SavedRequestContext);
    }

private:
    CRef<CRequestContext> m_SavedRequestContext;
};

/////////////////////////////////////////////////////////////////////////////
//
/// @internal
class CJobCommitterThread : public CThread
{
public:
    CJobCommitterThread(CGridWorkerNode* worker_node) :
        m_WorkerNode(worker_node),
        m_Semaphore(0, 1)
    {
    }

    CWorkerNodeJobContext* AllocJobContext();

    void PutJobContextBackAndCommitJob(CWorkerNodeJobContext* job_context);

    void Stop();

private:
    typedef CWorkerNodeTimeline<CWorkerNodeJobContext> TCommitJobTimeline;

    virtual void* Main();

    bool x_CommitJob(CWorkerNodeJobContext* job_context);

    void WakeUp()
    {
        if (m_ImmediateActions.IsEmpty())
            m_Semaphore.Post();
    }

    CGridWorkerNode* m_WorkerNode;
    CSemaphore m_Semaphore;
    TCommitJobTimeline m_ImmediateActions, m_Timeline, m_JobContextPool;
    CFastMutex m_TimelineMutex;

    typedef CGuard<CFastMutex, SSimpleUnlock<CFastMutex>,
            SSimpleLock<CFastMutex> > TFastMutexUnlockGuard;
};

END_NCBI_SCOPE

#endif // WN_COMMIT_THREAD__HPP
