#ifndef UTIL_CACHE_THREAD_CLEANER__HPP
#define UTIL_CACHE_THREAD_CLEANER__HPP

/*  $Id: icache_clean_thread.hpp 129052 2008-05-29 15:23:11Z lavr $
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
 * File Description: Cache cleaner (runs a thread, calls purge to remove 
 *                   obsolete thread elements.
 *
 */

#include <util/cache/icache.hpp>
#include <util/thread_nonstop.hpp>
#include <util/error_codes.hpp>


BEGIN_NCBI_SCOPE

/// Thread class, peridically calls ICache::Purge to remove obsolete
/// elements
///
class CCacheCleanerThread : public CThreadNonStop
{
public:
    CCacheCleanerThread(ICache* cache,
                        unsigned run_delay,
                        unsigned stop_request_poll = 10)
    : CThreadNonStop(run_delay, stop_request_poll),
      m_Cache(cache)
    {}

    virtual void DoJob(void)
    {
        try {
            int timeout = m_Cache->GetTimeout();
            m_Cache->Purge(timeout);
        } 
        catch(exception& ex)
        {
            RequestStop();
            LOG_POST_XX(Util_Cache, 3,
                        Error << "Error when cleaning cache: " 
                              << ex.what()
                              << " cleaning thread has been stopped.");
        }
    }

private:
    ICache*  m_Cache;
};


END_NCBI_SCOPE

#endif
