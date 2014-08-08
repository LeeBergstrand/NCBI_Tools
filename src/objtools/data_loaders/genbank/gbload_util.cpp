/*  $Id: gbload_util.cpp 254643 2011-02-16 16:42:21Z vasilche $
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
*  Author: Michael Kimelman
*
*  File Description: GenBank Data loader
*
*/

#include <ncbi_pch.hpp>
#include <objtools/data_loaders/genbank/gbload_util.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>
#include <objtools/error_codes.hpp>
#include <objmgr/impl/handle_range.hpp>
#include <objmgr/objmgr_exception.hpp>


#define NCBI_USE_ERRCODE_X   Objtools_GB_Util

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

//============================================================================
// Support Classes
//


/* =========================================================================== */
//////////////////////////////////////////////////////////////////////////////
//
// CTimer 

CTimer::CTimer(void)
    : m_RequestsDevider(0), m_Requests(0)
{
    m_ReasonableRefreshDelay = 0;
    m_LastCalibrated = m_Time= time(0);
}


time_t CTimer::Time(void)
{
    if(--m_Requests>0)
        return m_Time;
    m_RequestsLock.Lock();
    if(m_Requests<=0) {
        time_t x = time(0);
        if(x==m_Time) {
            m_Requests += m_RequestsDevider + 1;
            m_RequestsDevider = m_RequestsDevider*2 + 1;
        } else {
            m_Requests = m_RequestsDevider / ( x - m_Time );
            m_Time=x;
        }
    }
    m_RequestsLock.Unlock();
    return m_Time;
}


void CTimer::Start(void)
{
    m_TimerLock.Lock();
    m_StartTime = Time();
}


void CTimer::Stop(void)
{
    time_t x = Time() - m_StartTime; // test request timing in seconds
    m_ReasonableRefreshDelay = 60 /*sec*/ * 
        (x==0 ? 5 /*min*/ : x*50 /* 50 min per sec of test request*/);
    m_LastCalibrated = m_Time;
    m_TimerLock.Unlock();
}


time_t CTimer::RetryTime(void)
{
    return Time() +
        (m_ReasonableRefreshDelay>0?m_ReasonableRefreshDelay:24*60*60);
    /* 24 hours */
}


bool CTimer::NeedCalibration(void)
{
    return
        (m_ReasonableRefreshDelay==0) ||
        (m_Time-m_LastCalibrated>100*m_ReasonableRefreshDelay);
}


#if 0
void CRefresher::Reset(CTimer &timer)
{
    m_RefreshTime = timer.RetryTime();
}


bool CRefresher::NeedRefresh(CTimer &timer) const
{
    return timer.Time() > m_RefreshTime;
}


// MutexPool
//

#if defined(NCBI_THREADS)
CMutexPool::CMutexPool()
{
    m_size =0;
    m_Locks=0;
    spread =0;
}


void CMutexPool::SetSize(int size)
{
    _VERIFY(m_size==0 && !m_Locks);
    m_size = size;
    m_Locks = new CMutex[m_size];
    spread  = new int[m_size];
    for ( int i = 0; i < m_size; ++i ) {
        spread[i]=0;
    }
}


CMutexPool::~CMutexPool(void)
{
    delete [] m_Locks;
    if ( spread )  {
        for ( int i = 0; i < m_size; ++i ) {
            GBLOG_POST_X(1, "PoolMutex " << i << " used "<< spread[i] << " times");
        }
    }
    delete [] spread;
}
#else
CMutex CMutexPool::sm_Lock;
#endif

/* =========================================================================== */
// CGBLGuard 
//
CGBLGuard::CGBLGuard(TLMutex& lm,EState orig,const char *loc,int select)
    : m_Locks(&lm),
      m_Loc(loc),
      m_orig(orig),
      m_current(orig),
      m_select(select)
{
}

CGBLGuard::CGBLGuard(TLMutex &lm,const char *loc)
    // assume orig=eNone, switch to e.Main in constructor
    : m_Locks(&lm),
      m_Loc(loc),
      m_orig(eNone),
      m_current(eNone),
      m_select(-1)
{
    Switch(eMain);
}

CGBLGuard::CGBLGuard(CGBLGuard &g,const char *loc)
    : m_Locks(g.m_Locks),
      m_Loc(g.m_Loc),
      m_orig(g.m_current),
      m_current(g.m_current),
      m_select(g.m_select)
{
    if ( loc ) {
        m_Loc = loc;
    }
    _VERIFY(m_Locks);
}

CGBLGuard::~CGBLGuard()
{
    Switch(m_orig);
}

#if defined(NCBI_THREADS)
void CGBLGuard::Select(int s)
{
    if ( m_current==eMain ) {
        m_select=s;
    }
    _ASSERT(m_select==s);
}

#define LOCK_POST(err_subcode, x) GBLOG_POST_X(err_subcode, x) 
//#define LOCK_POST(err_subcode, x) 
void CGBLGuard::MLock()
{
    LOCK_POST(2, &m_Locks << ":: MainLock tried   @ " << m_Loc);
    m_Locks->m_Lookup.Lock();
    LOCK_POST(3, &m_Locks << ":: MainLock locked  @ " << m_Loc);
}

void CGBLGuard::MUnlock()
{
    LOCK_POST(4, &m_Locks << ":: MainLock unlocked@ " << m_Loc);
    m_Locks->m_Lookup.Unlock();
}

void CGBLGuard::PLock()
{
    _ASSERT(m_select>=0);
    LOCK_POST(5, &m_Locks << ":: Pool["<< setw(2) << m_select << "] tried   @ "
                 << m_Loc);
    m_Locks->m_Pool.GetMutex(m_select).Lock();
    LOCK_POST(6, &m_Locks << ":: Pool["<< setw(2) << m_select << "] locked  @ "
                 << m_Loc);
}

void CGBLGuard::PUnlock()
{
    _ASSERT(m_select>=0);
    LOCK_POST(7, &m_Locks << ":: Pool["<< setw(2) << m_select << "] unlocked@ "
                 << m_Loc);
    m_Locks->m_Pool.GetMutex(m_select).Unlock();
}

void CGBLGuard::Switch(EState newstate)
{
    if(newstate==m_current) return;
    switch(newstate) {
    case eNone:
        if ( m_current!=eMain ) {
            Switch(eMain);
        }
        _ASSERT(m_current==eMain);
        //LOCK_POST(8, &m_Locks << ":: switch 'main' to 'none'");
        MUnlock();
        m_current=eNone;
        return;
      
    case eBoth:
        if ( m_current!=eMain ) {
            Switch(eMain);
        }
        _ASSERT(m_current==eMain);
        //LOCK_POST(9, &m_Locks << ":: switch 'main' to 'both'");
        if ( m_Locks->m_SlowTraverseMode>0 ) {
            PLock();
        }
        m_current=eBoth;
        return;
      
    case eLocal:
        if ( m_current!=eBoth ) {
            Switch(eBoth);
        }
        _ASSERT(m_current==eBoth);
        //LOCK_POST(10, &m_Locks << ":: switch 'both' to 'local'");
        if(m_Locks->m_SlowTraverseMode==0) {
            PLock();
        }
        try {
            m_Locks->m_SlowTraverseMode++;
            MUnlock();
        }
        catch( exception& ) {
            m_Locks->m_SlowTraverseMode--;
            if(m_Locks->m_SlowTraverseMode==0) {
                PUnlock();
            }
            throw;
        }
        m_current=eLocal;
        return;
    case eMain:
        switch(m_current) {
        case eNone:
            m_select=-1;
            //LOCK_POST(11, &m_Locks << ":: switch 'none' to 'main'");
            MLock();
            m_current=eMain;
            return;
        case eBoth:
            //LOCK_POST(12, &m_Locks << ":: switch 'both' to 'main'");
            if(m_Locks->m_SlowTraverseMode>0) {
                PUnlock();
            }
            m_select=-1;
            m_current=eMain;
            return;
        case eLocal:
            //LOCK_POST(13, &m_Locks << ":: switch 'local' to 'none2main'");
            PUnlock();
            m_current=eNoneToMain;
        case eNoneToMain:
            //LOCK_POST(14, &m_Locks << ":: switch 'none2main' to 'main'");
            MLock();
            m_Locks->m_SlowTraverseMode--;
            m_select=-1;
            m_current=eMain;
            return;
        default:
            break;
        }
    default:
        break;
    }
    NCBI_THROW(CLoaderException, eOtherError,
        "CGBLGuard::Switch - state desynchronized");
}
#endif // if(NCBI_THREADS)

#endif

END_SCOPE(objects)
END_NCBI_SCOPE
