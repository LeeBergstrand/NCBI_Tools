#ifndef __MISC__GRID_CGI__REMOTE_CGIAPP_HPP
#define __MISC__GRID_CGI__REMOTE_CGIAPP_HPP

/*  $Id: remote_cgiapp.hpp 383181 2012-12-12 16:51:50Z kazimird $
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
 * Author:  Maxim Didenko
 *
 * File Description:
 *
 */

#include <corelib/ncbimisc.hpp>
#include <cgi/cgictx.hpp>

#include <connect/services/grid_worker.hpp>

/// @file grid_worker_cgiapp.hpp
/// NetSchedule Framework specs. 
///


BEGIN_NCBI_SCOPE

/** @addtogroup NetScheduleClient
 *
 * @{
 */

class CCgiWorkerNodeJob;

class NCBI_XGRIDCGI_EXPORT CRemoteCgiApp : public CCgiApplication
{
public:
    CRemoteCgiApp();

    virtual void Init(void);
    virtual int Run(void);

    void RequestShutdown(void);

    virtual string GetJobVersion() const;

protected:

    virtual void SetupArgDescriptions(CArgDescriptions* arg_desc);

    void         PutProgressMessage(const string& msg, 
                                    bool send_immediately = false);

private:
    CWorkerNodeJobContext* m_JobContext;
    auto_ptr<CGridWorkerNode> m_AppImpl;
    friend class CCgiWorkerNodeJob;
    int RunJob(CNcbiIstream& is, CNcbiOstream& os, CWorkerNodeJobContext&);
};


/* @} */

/////////////////////////////////////////////////////////////////////////////

END_NCBI_SCOPE

#endif //__MISC__GRID_CGI__REMOTE_CGIAPP_HPP
