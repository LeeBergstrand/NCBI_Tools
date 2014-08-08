#ifndef PTRSTR_HPP
#define PTRSTR_HPP

/*  $Id: ptrstr.hpp 282780 2011-05-16 16:02:27Z gouriano $
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
* Author: Eugene Vasilchenko
*
* File Description:
*   C++ class info: includes, used classes, C++ code etc.
*
*/

#include "typestr.hpp"
#include <corelib/ncbiutil.hpp>

BEGIN_NCBI_SCOPE

class CPointerTypeStrings : public CTypeStrings
{
    typedef CTypeStrings CParent;
public:
    CPointerTypeStrings(CTypeStrings* type);
    CPointerTypeStrings(AutoPtr<CTypeStrings> type);
    ~CPointerTypeStrings(void);

    const CTypeStrings* GetDataTypeStr(void) const
        {
            return m_DataTypeStr.get();
        }

    void GenerateTypeCode(CClassContext& ctx) const;

    EKind GetKind(void) const;

    string GetCType(const CNamespace& ns) const;
    string GetPrefixedCType(const CNamespace& ns,
                            const string& methodPrefix) const;
    string GetRef(const CNamespace& ns) const;

    string GetInitializer(void) const;
    string GetDestructionCode(const string& expr) const;
    string GetIsSetCode(const string& var) const;
    string GetResetCode(const string& var) const;

private:
    AutoPtr<CTypeStrings> m_DataTypeStr;
};

class CRefTypeStrings : public CPointerTypeStrings
{
    typedef CPointerTypeStrings CParent;
public:
    CRefTypeStrings(CTypeStrings* type);
    CRefTypeStrings(AutoPtr<CTypeStrings> type);
    ~CRefTypeStrings(void);

    EKind GetKind(void) const;

    string GetCType(const CNamespace& ns) const;
    string GetPrefixedCType(const CNamespace& ns,
                            const string& methodPrefix) const;
    string GetRef(const CNamespace& ns) const;

    string GetInitializer(void) const;
    string GetDestructionCode(const string& expr) const;
    string GetIsSetCode(const string& var) const;
    string GetResetCode(const string& var) const;
};

END_NCBI_SCOPE

#endif
