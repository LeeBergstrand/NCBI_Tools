#ifndef DBAPI_DRIVER_SAMPLE_BASE_HPP
#define DBAPI_DRIVER_SAMPLE_BASE_HPP

/*  $Id: dbapi_driver_sample_base.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author:  Sergey Sikorskiy
*
* File Description:
*
*/


#include <corelib/ncbiapp.hpp>


BEGIN_NCBI_SCOPE


/////////////////////////////////////////////////////////////////////////////
//  CDbapiSampleApp::
//

class CDbapiDriverSampleApp : public CNcbiApplication
{
public:
    CDbapiDriverSampleApp(const string& server_name,
                          int tds_version = 0);
    virtual ~CDbapiDriverSampleApp(void);

protected:
    virtual int  RunSample(void) = 0;

protected:
    enum EServerType {
        eUnknown,   //< Server type is not known
        eSybase,    //< Sybase server
        eMsSql      //< Microsoft SQL server
    };

protected:
    /// Return current server name
    const string& GetServerName(void) const
    {
        _ASSERT(!m_ServerName.empty());
        return m_ServerName;
    }
    /// Return current server type
    EServerType GetServerType(void) const;
    /// Return current user name
    const string& GetUserName(void) const
    {
        _ASSERT(!m_UserName.empty());
        return m_UserName;
    }
    /// Return current password
    const string& GetPassword(void) const
    {
        _ASSERT(!m_Password.empty());
        return m_Password;
    }
    /// Return TDS version.
    int GetTDSVersion(void) const
    {
        return m_TDSVersion;
    }

private:
    virtual void Init();
    virtual int  Run();

private:
    const string    m_DefaultServerName;
    const int       m_DefaultTDSVersion;

    string          m_ServerName;
    string          m_UserName;
    string          m_Password;
    int             m_TDSVersion;
};


END_NCBI_SCOPE


#endif // DBAPI_DRIVER_SAMPLE_BASE_HPP
