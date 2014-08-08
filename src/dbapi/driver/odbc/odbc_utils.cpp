/* $Id: odbc_utils.cpp 103491 2007-05-04 17:18:18Z kazimird $
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
 * File Description:  Small utility classes common to the odbc driver.
 *
 */

#include <ncbi_pch.hpp>

#include "odbc_utils.hpp"


BEGIN_NCBI_SCOPE


CODBCString::CODBCString(SQLCHAR* str,
                         EEncoding enc) :
    CWString(reinterpret_cast<const char*>(const_cast<const SQLCHAR*>(str)), string::npos, enc)
{
}


CODBCString::CODBCString(SQLCHAR* str,
                         SQLINTEGER size,
                         EEncoding enc) :
    CWString(reinterpret_cast<const char*>(const_cast<const SQLCHAR*>(str)),
             static_cast<string::size_type>(size),
             enc)
{
}


CODBCString::CODBCString(const char* str,
                         string::size_type size,
                         EEncoding enc) :
    CWString(str, size, enc)
{
}


#ifdef HAVE_WSTRING
CODBCString::CODBCString(SQLWCHAR* str,
                         EEncoding /*enc*/) :
    CWString(reinterpret_cast<const wchar_t*>(const_cast<const SQLWCHAR*>(str)))
{
}


CODBCString::CODBCString(const wchar_t* str,
                         wstring::size_type size,
                         EEncoding /*enc*/) :
    CWString(str, size)
{
}


CODBCString::CODBCString(const wstring& str,
                         EEncoding /*enc*/) :
    CWString(str)
{
}
#endif


CODBCString::CODBCString(const string& str, EEncoding enc) :
    CWString(str, enc)
{
}


CODBCString::~CODBCString(void)
{
}


END_NCBI_SCOPE


