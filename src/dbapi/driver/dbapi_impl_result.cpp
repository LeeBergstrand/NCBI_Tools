/*  $Id: dbapi_impl_result.cpp 125986 2008-04-28 21:35:56Z ssikorsk $
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
*/

#include <ncbi_pch.hpp>

#include <dbapi/driver/impl/dbapi_impl_result.hpp>
#include <dbapi/error_codes.hpp>


#define NCBI_USE_ERRCODE_X   Dbapi_DrvrResult

BEGIN_NCBI_SCOPE

namespace impl
{
    
////////////////////////////////////////////////////////////////////////////////
CResult::CResult(void)
: m_CachedRowInfo(GetDefineParamsImpl())
{
    return;
}

CResult::~CResult(void)
{
    try {
        DetachInterface();
    }
    NCBI_CATCH_ALL_X( 1, NCBI_CURRENT_FUNCTION )
}

const CDBParams& 
CResult::GetDefineParams(void) const
{
    return m_CachedRowInfo;
}


} // namespace impl

END_NCBI_SCOPE


