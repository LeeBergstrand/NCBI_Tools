/*  $Id: ns_server_misc.cpp 381925 2012-11-30 14:28:54Z satskyse $
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
 * Authors:  Anatoliy Kuznetsov, Victor Joukov
 *
 * File Description: factories, hosts etc for the NS server
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/request_ctx.hpp>

#include "ns_server_misc.hpp"
#include "ns_handler.hpp"
#include "ns_server.hpp"


USING_NCBI_SCOPE;


//////////////////////////////////////////////////////////////////////////
/// CNetScheduleConnectionFactory implemetation
IServer_ConnectionHandler* CNetScheduleConnectionFactory::Create(void)
{
    return new CNetScheduleHandler(m_Server);
}


//////////////////////////////////////////////////////////////////////////
/// CNetScheduleBackgroundHost implementation
void CNetScheduleBackgroundHost::ReportError(ESeverity          severity,
                                             const std::string& what)
{
    if (severity == CBackgroundHost::eFatal)
        m_Server->SetShutdownFlag();
}


bool CNetScheduleBackgroundHost::ShouldRun()
{
    return true;
}


bool CNetScheduleBackgroundHost::IsLog() const
{
    return m_Server->IsLog();
}


//////////////////////////////////////////////////////////////////////////
/// CNetScheduleRequestExecutor implementation
void
CNetScheduleRequestExecutor::SubmitRequest(const CRef<CStdRequest>& request)
{
    m_Server->SubmitRequest(request);
}

