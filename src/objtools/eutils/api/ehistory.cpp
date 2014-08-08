/*  $Id: ehistory.cpp 119843 2008-02-14 19:08:48Z grichenk $
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
*   EHistory request
*
*/

#include <ncbi_pch.hpp>
#include <objtools/eutils/api/ehistory.hpp>


BEGIN_NCBI_SCOPE


CEHistory_Request::CEHistory_Request(const string& db,
                                     CRef<CEUtils_ConnContext>& ctx)
    : CEUtils_Request(ctx, "ehistory.fcgi")
{
    SetDatabase(db);
}


CEHistory_Request::~CEHistory_Request(void)
{
}


ESerialDataFormat CEHistory_Request::GetSerialDataFormat(void) const
{
    return eSerial_Xml;
}


CRef<ehistory::CEHistoryResult> CEHistory_Request::GetEHistoryResult(void)
{
    CObjectIStream* is = GetObjectIStream();
    _ASSERT(is);
    CRef<ehistory::CEHistoryResult> res(new ehistory::CEHistoryResult);
    *is >> *res;
    Disconnect();
    return res;
}


END_NCBI_SCOPE
