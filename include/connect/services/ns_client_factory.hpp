#ifndef CONNECT_SERVICES___NS_CLIENT_FACTORY__HPP
#define CONNECT_SERVICES___NS_CLIENT_FACTORY__HPP

/*  $Id: ns_client_factory.hpp 193444 2010-06-03 19:28:37Z kazimird $
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
 */

#include <connect/services/netschedule_api.hpp>

#include <corelib/ncbireg.hpp>
#include <corelib/plugin_manager.hpp>
#include <corelib/ncbiexpt.hpp>


BEGIN_NCBI_SCOPE


/// NetSchedule Client Factory interface
///
/// @sa CNetScheduleAPI
/// @internal
class NCBI_XCONNECT_EXPORT CNetScheduleClientFactory
{
public:
    CNetScheduleClientFactory(const IRegistry& reg);
    virtual ~CNetScheduleClientFactory() {}

    virtual CNetScheduleAPI CreateInstance();

private:
    typedef CPluginManager<SNetScheduleAPIImpl> TPMNetSchedule;
    TPMNetSchedule   m_PM_NetSchedule;
    const IRegistry& m_Registry;
};



/////////////////////////////////////////////////////////////////////////////
//
/// @internal
class CNSClientFactoryException : public CException
{
public:
    enum EErrCode {
        eNSClientIsNotCreated
    };

    virtual const char* GetErrCodeString(void) const
    {
        switch (GetErrCode()) {
        case eNSClientIsNotCreated:  return "eNSClientIsNotCreatedError";
        default:                     return CException::GetErrCodeString();
        }
    }

    NCBI_EXCEPTION_DEFAULT(CNSClientFactoryException, CException);
};


END_NCBI_SCOPE

#endif // CONNECT_SERVICES___NS_CLIENT_FACTORY__HPP
