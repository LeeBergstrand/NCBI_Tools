/* $Id: dbapi_driver_conn_mgr.cpp 212357 2010-11-19 16:59:00Z ivanovp $
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
 *  ===========================================================================
 *
 * Author:  Sergey Sikorskiy
 *
 * File Description:
 *
 */

#include <ncbi_pch.hpp>

#include <dbapi/driver/dbapi_driver_conn_mgr.hpp>
#include <dbapi/driver/dbapi_conn_factory.hpp>
#include <corelib/ncbi_safe_static.hpp>
#include <set>
#include <map>
#include <dbapi/driver/public.hpp>

BEGIN_NCBI_SCOPE

///////////////////////////////////////////////////////////////////////////////
class CDefaultConnectPolicy : public IDBConnectionFactory
{
public:
    virtual ~CDefaultConnectPolicy(void);

public:
    virtual void Configure(const IRegistry* registry = NULL);
    virtual CDB_Connection* MakeDBConnection(
        I_DriverContext& ctx,
        const CDBConnParams& params);
};

CDefaultConnectPolicy::~CDefaultConnectPolicy(void)
{
}

void
CDefaultConnectPolicy::Configure(const IRegistry*)
{
    // Do-nothing ...
}

CDB_Connection*
CDefaultConnectPolicy::MakeDBConnection(
    I_DriverContext& ctx,
    const CDBConnParams& params)
{
    auto_ptr<CDB_Connection> conn(CtxMakeConnection(ctx, params));

    if (conn.get()) {
        CTrivialConnValidator use_db_validator(
            params.GetDatabaseName(), 
            CTrivialConnValidator::eKeepModifiedConnection
            );
        CConnValidatorCoR validator;

        validator.Push(params.GetConnValidator());

        // Check "use <database>" command ...
        if (!params.GetDatabaseName().empty()) {
            validator.Push(CRef<IConnValidator>(&use_db_validator));
        }

        if (validator.Validate(*conn) != IConnValidator::eValidConn) {
            return NULL;
        }
        conn->FinishOpening();
    }
    return conn.release();
}

///////////////////////////////////////////////////////////////////////////////
IConnValidator::~IConnValidator(void)
{
}


IConnValidator::EConnStatus
IConnValidator::ValidateException(const CDB_Exception&)
{
    return eInvalidConn;
}


string
IConnValidator::GetName(void) const
{
    return typeid(this).name();
}


///////////////////////////////////////////////////////////////////////////////
IDBConnectionFactory::IDBConnectionFactory(void)
{
}

IDBConnectionFactory::~IDBConnectionFactory(void)
{
}

///////////////////////////////////////////////////////////////////////////////
CDbapiConnMgr::CDbapiConnMgr(void) : m_NumConnect(0)
{
    m_ConnectFactory.Reset( new CDefaultConnectPolicy() );
}

CDbapiConnMgr::~CDbapiConnMgr(void)
{
}

CDbapiConnMgr&
CDbapiConnMgr::Instance(void)
{
    static CSafeStaticPtr<CDbapiConnMgr> instance;

    return instance.Get();
}


NCBI_PARAM_DECL(unsigned int, dbapi, max_connection);
typedef NCBI_PARAM_TYPE(dbapi, max_connection) TDbapi_MaxConnection;
NCBI_PARAM_DEF_EX(unsigned int, dbapi, max_connection, 100, eParam_NoThread, NULL);


void CDbapiConnMgr::SetMaxConnect(unsigned int max_connect)
{
    TDbapi_MaxConnection::SetDefault(max_connect);
}

unsigned int CDbapiConnMgr::GetMaxConnect(void)
{
    return TDbapi_MaxConnection::GetDefault();
}

bool CDbapiConnMgr::AddConnect(void)
{
    CMutexGuard mg(m_Mutex);

    if (m_NumConnect >= GetMaxConnect())
        return false;

    ++m_NumConnect;
    return true;
}

void CDbapiConnMgr::DelConnect(void)
{
    CMutexGuard mg(m_Mutex);

    if (m_NumConnect > 0)
        --m_NumConnect;
}

END_NCBI_SCOPE

