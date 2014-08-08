#ifndef PREFETCH_MANAGER_IMPL__HPP
#define PREFETCH_MANAGER_IMPL__HPP

/*  $Id: prefetch_manager_impl.hpp 133984 2008-07-15 19:35:22Z vasilche $
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
*   Prefetch manager
*
*/

#include <corelib/ncbistd.hpp>
#include <corelib/ncbiobj.hpp>
#include <corelib/ncbimisc.hpp>
#include <objmgr/prefetch_manager.hpp>
#include <set>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CScope;
class IPrefetchAction;
class IPrefetchListener;
class CPrefetchManager_Impl;

/** @addtogroup ObjectManagerCore
 *
 * @{
 */


class NCBI_XOBJMGR_EXPORT CPrefetchManager_Impl
    : public CObject,
      public CThreadPool,
      public SPrefetchTypes
{
public:
    CPrefetchManager_Impl(unsigned max_threads,
                          CThread::TRunMode threads_mode);
    ~CPrefetchManager_Impl(void);

    typedef unsigned int TPriority;

    CRef<CPrefetchRequest> AddAction(TPriority priority,
                                     IPrefetchAction* action,
                                     IPrefetchListener* listener);

protected:
    friend class CPrefetchRequest;
    friend class CPrefetchManager;

private:
    CRef<CObjectFor<CMutex> > m_StateMutex;

private:
    CPrefetchManager_Impl(const CPrefetchManager_Impl&);
    void operator=(const CPrefetchManager_Impl&);
};


/* @} */


END_SCOPE(objects)
END_NCBI_SCOPE

#endif  // PREFETCH_MANAGER_IMPL__HPP
