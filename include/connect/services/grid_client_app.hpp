#ifndef CONNECT_SERVICES_GRID__GRID_CLIENT_APP__HPP
#define CONNECT_SERVICES_GRID__GRID_CLIENT_APP__HPP

/*  $Id: grid_client_app.hpp 196780 2010-07-08 16:50:11Z kazimird $
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
 * Authors:  Maxim Didenko
 *
 * File Description:
 *   NetSchedule Client Framework Application.
 *
 */

#include <connect/services/grid_client.hpp>

#include <corelib/ncbimisc.hpp>
#include <corelib/ncbiapp.hpp>


/// @file grid_client_app.hpp
/// NetSchedule Framework specs.
///

BEGIN_NCBI_SCOPE

/** @addtogroup NetScheduleClient
 *
 * @{
 */

/// Grid Client Application
///
class NCBI_XCONNECT_EXPORT CGridClientApp : public CNcbiApplication
{
public:
    /// If you override this method, do call CGridClientApp::Init()
    /// from inside your overriding method.
    virtual void Init(void);

    /// Get a grid client
    ///
    CGridClient& GetGridClient() { return *m_GridClient; }

    /// Get program version (like: MyProgram v. 1.2.3)
    ///
    /// Program version is passed to NetSchedule queue so queue
    /// controls versions and does not allow obsolete clients
    /// to connect and submit or execute jobs
    ///
    virtual string GetProgramVersion(void) const = 0;

protected:
    virtual bool UseProgressMessage() const;
    virtual bool UseAutomaticCleanup() const;

    auto_ptr<CGridClient>        m_GridClient;
};

/////////////////////////////////////////////////////////////////////////////

END_NCBI_SCOPE

#endif // CONNECT_SERVICES_GRID__GRID_CLIENT_APP__HPP
