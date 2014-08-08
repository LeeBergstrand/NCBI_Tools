/*  $Id: prefetch_manager_impl.cpp 246000 2011-02-10 18:51:35Z vasilche $
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
* Author: Eugene Vasilchenko
*
* File Description:
*   Prefetch implementation
*
*/

#include <ncbi_pch.hpp>
#include <objmgr/impl/prefetch_manager_impl.hpp>
#include <objmgr/prefetch_manager.hpp>
#include <objmgr/objmgr_exception.hpp>
#include <corelib/ncbithr.hpp>
#include <corelib/ncbi_system.hpp>
#include <corelib/ncbi_safe_static.hpp>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


class CPrefetchManager_Impl;


BEGIN_SCOPE(prefetch)

/////////////////////////////////////////////////////////////////////////////
//  CCancelRequest::
//
//    Exception used to cancel requests safely, cleaning up
//    all the resources allocated.
//

class CCancelRequestException
{
public:
    // Create new exception object, initialize counter.
    CCancelRequestException(void);

    // Create a copy of exception object, increase counter.
    CCancelRequestException(const CCancelRequestException& prev);

    // Destroy the object, decrease counter. If the counter is
    // zero outside of CThread::Wrapper(), rethrow exception.
    ~CCancelRequestException(void);

    // Inform the object it has reached normal catch.
    void SetFinished(void)
    {
        m_Data->m_Finished = true;
    }
private:
    struct SData {
        SData(void)
            : m_RefCounter(1),
              m_Finished(false)
            {
            }
        int m_RefCounter;
        bool m_Finished;
    };
    SData *m_Data;
};


CCancelRequestException::CCancelRequestException(void)
    : m_Data(new SData())
{
}


CCancelRequestException::CCancelRequestException(const CCancelRequestException& prev)
    : m_Data(prev.m_Data)
{
    ++m_Data->m_RefCounter;
}


CCancelRequestException::~CCancelRequestException(void)
{
    if ( --m_Data->m_RefCounter > 0 ) {
        // Not the last object - continue to handle exceptions
        return;
    }

    bool finished = m_Data->m_Finished; // save the flag
    delete m_Data;

    if ( !finished ) {
        ERR_POST(Critical<<"CancelRequest() failed due to catch(...) in "<<
                 CStackTrace());
    }
}

END_SCOPE(prefetch)

CPrefetchRequest::CPrefetchRequest(CObjectFor<CMutex>* state_mutex,
                                   IPrefetchAction* action,
                                   IPrefetchListener* listener,
                                   unsigned int priority)
    : CThreadPool_Task(priority),
      m_StateMutex(state_mutex),
      m_Action(action),
      m_Listener(listener),
      m_Progress(0)
{
}


CPrefetchRequest::~CPrefetchRequest(void)
{
}


CPrefetchRequest::EState CPrefetchRequest::GetState(void) const
{
    switch (GetStatus()) {
    case CThreadPool_Task::eIdle:
        return eInvalid;
    case CThreadPool_Task::eQueued:
        return SPrefetchTypes::eQueued;
    case CThreadPool_Task::eExecuting:
        return eStarted;
    case CThreadPool_Task::eCompleted:
        return SPrefetchTypes::eCompleted;
    case CThreadPool_Task::eCanceled:
        return SPrefetchTypes::eCanceled;
    case CThreadPool_Task::eFailed:
        return SPrefetchTypes::eFailed;
    }

    return eInvalid;
}


void CPrefetchRequest::SetListener(IPrefetchListener* listener)
{
    CMutexGuard guard(m_StateMutex->GetData());
    if ( m_Listener ) {
        NCBI_THROW(CObjMgrException, eOtherError,
                   "CPrefetchToken::SetListener: listener already set");
    }
    m_Listener = listener;
}


void CPrefetchRequest::OnStatusChange(EStatus /* old */)
{
    if (m_Listener) {
        m_Listener->PrefetchNotify(Ref(this), GetState());
    }
}

CPrefetchRequest::TProgress
CPrefetchRequest::SetProgress(TProgress progress)
{
    CMutexGuard guard(m_StateMutex->GetData());
    if ( GetStatus() != eExecuting ) {
        NCBI_THROW(CObjMgrException, eOtherError,
                   "CPrefetchToken::SetProgress: not processing");
    }
    TProgress old_progress = m_Progress;
    if ( progress != old_progress ) {
        m_Progress = progress;
        if ( m_Listener ) {
            m_Listener->PrefetchNotify(Ref(this), eAdvanced);
        }
    }
    return old_progress;
}


CPrefetchRequest::EStatus CPrefetchRequest::Execute(void)
{
    try {
        EStatus result = CThreadPool_Task::eCompleted;
        if (m_Action.NotNull()) {
            if (! GetAction()->Execute(Ref(this))) {
                if ( IsCancelRequested() )
                    result = CThreadPool_Task::eCanceled;
                else
                    result = CThreadPool_Task::eFailed;
            }
        }
        return result;
    }
    catch ( CPrefetchCanceled& /* ignored */ ) {
        return CThreadPool_Task::eCanceled;
    }
    catch ( prefetch::CCancelRequestException& exc ) {
        exc.SetFinished();
        return CThreadPool_Task::eCanceled;
    }
}


CPrefetchManager_Impl::CPrefetchManager_Impl(unsigned max_threads,
                                             CThread::TRunMode threads_mode)
    : CThreadPool(kMax_Int, max_threads, 2, threads_mode),
      m_StateMutex(new CObjectFor<CMutex>())
{
}


CPrefetchManager_Impl::~CPrefetchManager_Impl(void)
{
}


CRef<CPrefetchRequest> CPrefetchManager_Impl::AddAction(TPriority priority,
                                                        IPrefetchAction* action,
                                                        IPrefetchListener* listener)
{
    CMutexGuard guard0(GetMainPoolMutex());
    if ( action && IsAborted() ) {
        throw prefetch::CCancelRequestException();
    }
    CMutexGuard guard(m_StateMutex->GetData());
    CRef<CPrefetchRequest> req(new CPrefetchRequest(m_StateMutex,
                                                    action,
                                                    listener,
                                                    priority));
    AddTask(req);
    return req;
}


bool CPrefetchManager::IsActive(void)
{
    CThreadPool_Thread* thread = dynamic_cast<CThreadPool_Thread*>(
                                                CThread::GetCurrentThread());
    if ( !thread ) {
        return false;
    }

    CRef<CThreadPool_Task> req = thread->GetCurrentTask();
    if ( !req ) {
        return false;
    }
    
    if ( req->IsCancelRequested() && dynamic_cast<CPrefetchRequest*>(&*req) ) {
        throw prefetch::CCancelRequestException();
    }
    
    return true;
}


END_SCOPE(objects)
END_NCBI_SCOPE
