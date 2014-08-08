/*  $Id: epost.cpp 119669 2008-02-12 19:40:05Z grichenk $
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
* Author: Aleksey Grichenko
*
* File Description:
*   EPost request
*
*/

#include <ncbi_pch.hpp>
#include <objtools/eutils/api/epost.hpp>
#include <cgi/cgi_util.hpp>


BEGIN_NCBI_SCOPE


CEPost_Request::CEPost_Request(const string& db,
                               CRef<CEUtils_ConnContext>& ctx)
    : CEUtils_Request(ctx, "epost.fcgi")
{
    SetDatabase(db);
}


CEPost_Request::~CEPost_Request(void)
{
}


string CEPost_Request::GetQueryString(void) const
{
    string args = TParent::GetQueryString();
    string ids = m_Id.AsQueryString();
    if ( !ids.empty() ) {
        args += "&" + ids;
    }
    return args;
}


ESerialDataFormat CEPost_Request::GetSerialDataFormat(void) const
{
    return eSerial_Xml;
}


CRef<epost::CEPostResult> CEPost_Request::GetEPostResult(void)
{
    CObjectIStream* is = GetObjectIStream();
    _ASSERT(is);
    CRef<epost::CEPostResult> res(new epost::CEPostResult);
    *is >> *res;
    Disconnect();
    // Save context data
    if ( res->IsSetHistory() ) {
        GetConnContext()->SetWebEnv(res->GetHistory().GetWebEnv());
        GetConnContext()->SetQueryKey(res->GetHistory().GetQueryKey());
    }
    return res;
}


END_NCBI_SCOPE
