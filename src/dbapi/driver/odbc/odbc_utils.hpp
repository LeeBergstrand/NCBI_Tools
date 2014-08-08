#ifndef DBAPI_DRIVER_DBLIB___DBAPI_DRIVER_ODBC_UTILS__HPP
#define DBAPI_DRIVER_DBLIB___DBAPI_DRIVER_ODBC_UTILS__HPP

/* $Id: odbc_utils.hpp 399404 2013-05-14 15:05:30Z ucko $
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

#include <dbapi/driver/impl/dbapi_driver_utils.hpp>
#include <dbapi/driver/odbc/interfaces.hpp>

BEGIN_NCBI_SCOPE

/////////////////////////////////////////////////////////////////////////////
class CODBCString : public CWString
{
public:
    explicit CODBCString(SQLCHAR* str,
                         EEncoding enc = eEncoding_Unknown);
    explicit CODBCString(SQLCHAR* str,
                         SQLINTEGER size,
                         EEncoding enc = eEncoding_Unknown);
    explicit CODBCString(const char* str,
                         string::size_type size = string::npos,
                         EEncoding enc = eEncoding_Unknown);
    explicit CODBCString(const string& str, EEncoding enc = eEncoding_Unknown);
#ifdef HAVE_WSTRING
    // Seconnd parameter is redundant and will be ignored,
    // but we need it as syntactical sugar.
    explicit CODBCString(SQLWCHAR* str,
                         EEncoding enc = eEncoding_Unknown);
    // Seconnd parameter is redundant and will be ignored,
    // but we need it as syntactical sugar.
    explicit CODBCString(const wchar_t* str,
                         wstring::size_type size = wstring::npos,
                         EEncoding enc = eEncoding_Unknown);
    // Seconnd parameter is redundant and will be ignored,
    // but we need it as syntactical sugar.
    explicit CODBCString(const wstring& str,
                         EEncoding enc = eEncoding_Unknown);
#endif
    ~CODBCString(void);

public:
    operator LPCSTR(void) const
    {
        if (!(GetAvailableValueType() & eChar)) {
            x_MakeString();
        }

        return reinterpret_cast<LPCSTR>(m_Char);
    }
    operator SQLCHAR*(void) const
    {
        if (!(GetAvailableValueType() & eChar)) {
            x_MakeString();
        }

        return const_cast<SQLCHAR*>(reinterpret_cast<const SQLCHAR*>(m_Char));
    }
    operator const SQLCHAR*(void) const
    {
        if (!(GetAvailableValueType() & eChar)) {
            x_MakeString();
        }

        return reinterpret_cast<const SQLCHAR*>(m_Char);
    }
};

/////////////////////////////////////////////////////////////////////////////
#if defined(UNICODE)
#  define _T_NCBI_ODBC(x) L ## x
#else
#  define _T_NCBI_ODBC(x) x
#endif

using odbc::TSqlChar;
typedef CGenericSqlString<TSqlChar> TSqlString;
inline
TSqlString x_MakeTSqlString(const CTempString& s, EEncoding enc)
{
#if defined(UNICODE)  ||  defined(_UNICODE)  ||  defined(UCS2)
    return CUtf8::AsBasicString<TSqlChar>(CUtf8::AsUTF8(s, enc));
#else
    if (enc == eEncoding_Unknown  ||  enc == eEncoding_UTF8) {
        return CUtf8::AsSingleByteString(CUtf8::AsUTF8(s, enc),
                                         eEncoding_ISO8859_1);
    } else {
        return CSqlString(s.data(), s.size());
    }
#endif
}


#ifdef HAVE_WSTRING
inline
wstring operator+(const wstring& str1, const string& str2)
{
    return str1 + CUtf8::AsBasicString<wchar_t>( CUtf8::AsUTF8(str2, eEncoding_ISO8859_1));
}
#endif

namespace util
{
    inline
    int strncmp(const char* str1, const char* str2, size_t count)
    {
        return ::strncmp(str1, str2, count);
    }

#ifdef HAVE_WSTRING
    inline
    int strncmp(const wchar_t* str1, const wchar_t* str2, size_t count)
    {
        return ::wcsncmp(str1, str2, count);
    }
#endif

    inline
    int strncmp(const SQLCHAR* str1, const char* str2, size_t count)
    {
        return strncmp((const char*)str1, str2, count);
    }

    inline
    int strncmp(const char* str1, const SQLCHAR* str2, size_t count)
    {
        return strncmp(str1, (const char*)str2, count);
    }

    ///////////////////////////////////////////////////////////////////////////
    inline
    int strcmp(const char* str1, const char* str2)
    {
        return ::strcmp(str1, str2);
    }

#ifdef HAVE_WSTRING
    inline
    int strcmp(const wchar_t* str1, const wchar_t* str2)
    {
        return ::wcscmp(str1, str2);
    }
#endif

}


extern "C"
{

NCBI_DBAPIDRIVER_ODBC_EXPORT
void
NCBI_EntryPoint_xdbapi_odbc(
    CPluginManager<I_DriverContext>::TDriverInfoList&   info_list,
    CPluginManager<I_DriverContext>::EEntryPointRequest method);

} // extern C


END_NCBI_SCOPE


#endif // DBAPI_DRIVER_DBLIB___DBAPI_DRIVER_ODBC_UTILS__HPP


