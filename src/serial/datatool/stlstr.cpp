/*  $Id: stlstr.cpp 381068 2012-11-19 15:47:53Z gouriano $
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
*   Type info for class generation: includes, used classes, C code etc.
*
*/

#include <ncbi_pch.hpp>
#include "exceptions.hpp"
#include "stlstr.hpp"
#include "classctx.hpp"
#include "namespace.hpp"
#include "srcutil.hpp"

BEGIN_NCBI_SCOPE

CTemplate1TypeStrings::CTemplate1TypeStrings(const string& templateName,
                                             CTypeStrings* arg1Type,
                                             const string& namespaceName,
                                             const CDataType* dataType)
    : CParent(namespaceName, dataType),
      m_Arg1Type(arg1Type)
{
    x_ParseTemplateName(templateName);
}

CTemplate1TypeStrings::CTemplate1TypeStrings(const string& templateName,
                                             AutoPtr<CTypeStrings> arg1Type,
                                             const string& namespaceName,
                                             const CDataType* dataType)
    : CParent(namespaceName, dataType),
      m_TemplateName(templateName), m_Arg1Type(arg1Type)
{
    x_ParseTemplateName(templateName);
}

void CTemplate1TypeStrings::x_ParseTemplateName(const string& templateName)
{
    string s1, s2;
    NStr::SplitInTwo(templateName," ",s1,s2);
    m_TemplateName = s1;
    if (!s2.empty()) {
        m_ExtraParam = ", ";
        m_ExtraParam += s2;
    }
}

CTemplate1TypeStrings::~CTemplate1TypeStrings(void)
{
}

CTypeStrings::EKind CTemplate1TypeStrings::GetKind(void) const
{
    return eKindContainer;
}

string CTemplate1TypeStrings::GetCType(const CNamespace& ns) const
{
    string result(ns.GetNamespaceRef(GetTemplateNamespace()));
    result += GetTemplateName()+"< "+GetArg1Type()->GetCType(ns);
    result += GetTemplateExtraParam() + " >";
    return result;
}

string CTemplate1TypeStrings::GetPrefixedCType(const CNamespace& ns,
                                               const string& methodPrefix) const
{
    string result(ns.GetNamespaceRef(GetTemplateNamespace()));
    result += GetTemplateName()+"< "+GetArg1Type()->GetPrefixedCType(ns,methodPrefix);
    result += GetTemplateExtraParam() + " >";
    return result;
}

string CTemplate1TypeStrings::GetRef(const CNamespace& ns) const
{
    return "STL_"+GetRefTemplate()+", ("+GetArg1Type()->GetRef(ns)+
        GetTemplateExtraParam()+')';
}

string CTemplate1TypeStrings::GetRefTemplate(void) const
{
    // count extra params
    string extracount;
    const string& extra(GetTemplateExtraParam());
    if (!extra.empty()) {
        const CTemplate2TypeStrings* t2 =
            dynamic_cast<const CTemplate2TypeStrings*>(this);
        int c= t2 ? 2 : 1;
        string::size_type comma = extra.find(',');
        for (; comma != string::npos;) {
            ++c;
            comma = extra.find(',', ++comma);
        }
        extracount = NStr::NumericToString(c);
    }
    return GetTemplateName()+extracount;
}

string CTemplate1TypeStrings::GetIsSetCode(const string& var) const
{
    return "!("+var+").empty()";
}

void CTemplate1TypeStrings::AddTemplateInclude(CClassContext::TIncludes& hpp) const
{
    string header = GetTemplateName();
    if ( header == "multiset" )
        header = "<set>";
    else if ( header == "multimap" )
        header = "<map>";
    else if ( header == "AutoPtr" )
        header = "<corelib/ncbiutil.hpp>";
    else
        header = '<'+header+'>';
    hpp.insert(header);
}

const CNamespace& CTemplate1TypeStrings::GetTemplateNamespace(void) const
{
    if ( GetTemplateName() == "AutoPtr" )
        return CNamespace::KNCBINamespace;
    return CNamespace::KSTDNamespace;
}

void CTemplate1TypeStrings::GenerateTypeCode(CClassContext& ctx) const
{
    AddTemplateInclude(ctx.HPPIncludes());
    GetArg1Type()->GenerateTypeCode(ctx);
}

CTemplate2TypeStrings::CTemplate2TypeStrings(const string& templateName,
                                             AutoPtr<CTypeStrings> arg1Type,
                                             AutoPtr<CTypeStrings> arg2Type,
                                             const string& namespaceName,
                                             const CDataType* dataType)
    : CParent(templateName, arg1Type, namespaceName, dataType), m_Arg2Type(arg2Type)
{
}

CTemplate2TypeStrings::~CTemplate2TypeStrings(void)
{
}

string CTemplate2TypeStrings::GetCType(const CNamespace& ns) const
{
    string result(ns.GetNamespaceRef(GetTemplateNamespace()));
    result += GetTemplateName()+"< ";
    result += GetArg1Type()->GetCType(ns)+", "+GetArg2Type()->GetCType(ns);
    result += GetTemplateExtraParam() + " >";
    return result;
}

string CTemplate2TypeStrings::GetPrefixedCType(const CNamespace& ns,
                                               const string& methodPrefix) const
{
    string result(ns.GetNamespaceRef(GetTemplateNamespace()));
    result += GetTemplateName()+"< ";
    result += GetArg1Type()->GetPrefixedCType(ns,methodPrefix)+", "
            + GetArg2Type()->GetPrefixedCType(ns,methodPrefix);
    result += GetTemplateExtraParam() + " >";
    return result;
}

string CTemplate2TypeStrings::GetRef(const CNamespace& ns) const
{
    return "STL_"+GetRefTemplate()+
        ", ("+GetArg1Type()->GetRef(ns)+", "+GetArg2Type()->GetRef(ns)+
        GetTemplateExtraParam()+')';
}

void CTemplate2TypeStrings::GenerateTypeCode(CClassContext& ctx) const
{
    CParent::GenerateTypeCode(ctx);
    GetArg2Type()->GenerateTypeCode(ctx);
}

CSetTypeStrings::CSetTypeStrings(const string& templateName,
                                 AutoPtr<CTypeStrings> type,
                                 const string& namespaceName,
                                 const CDataType* dataType)
    : CParent(templateName, type, namespaceName, dataType)
{
}

CSetTypeStrings::~CSetTypeStrings(void)
{
}

string CSetTypeStrings::GetDestructionCode(const string& expr) const
{
    string code;
    string iter;
    static int level = 0;
    try {
        level++;
        iter = "setIter"+NStr::IntToString(level);
        code = Tabbed(GetArg1Type()->GetDestructionCode('*'+iter), "        ");
    }
    catch (CDatatoolException& exp) {
        level--;
        NCBI_RETHROW_SAME(exp,"CSetTypeStrings::GetDestructionCode: failed");
    }
    catch (...) {
        level--;
        throw;
    }
    level--;
    if ( code.empty() )
        return string();
    return
        "{\n"
        "    for ( "+GetCType(CNamespace::KEmptyNamespace)+"::iterator "+iter+" = ("+expr+").begin(); "+iter+" != ("+expr+").end(); ++"+iter+" ) {\n"
        +code+
        "    }\n"
        "}\n";
}

string CSetTypeStrings::GetResetCode(const string& var) const
{
    return var+".clear();\n";
}

CListTypeStrings::CListTypeStrings(const string& templateName,
                                   AutoPtr<CTypeStrings> type,
                                   const string& namespaceName,
                                   const CDataType* dataType,
                                   bool externalSet)
    : CParent(templateName, type, namespaceName, dataType), m_ExternalSet(externalSet)
{
}

CListTypeStrings::~CListTypeStrings(void)
{
}

string CListTypeStrings::GetRefTemplate(void) const
{
    string templ = CParent::GetRefTemplate();
    if ( m_ExternalSet )
        templ += "_set";
    return templ;
}

string CListTypeStrings::GetDestructionCode(const string& expr) const
{
    string code;
    string iter;
    static int level = 0;
    try {
        level++;
        iter = "listIter"+NStr::IntToString(level);
        code = Tabbed(GetArg1Type()->GetDestructionCode('*'+iter), "        ");
    }
    catch (CDatatoolException& exp) {
        level--;
        NCBI_RETHROW_SAME(exp,"CListTypeStrings::GetDestructionCode: failed");
    }
    catch (...) {
        level--;
        throw;
    }
    level--;
    if ( code.empty() )
        return string();
    return
        "{\n"
        "    for ( "+GetCType(CNamespace::KEmptyNamespace)+"::iterator "+iter+" = ("+expr+").begin(); "+iter+" != ("+expr+").end(); ++"+iter+" ) {\n"
        +code+
        "    }\n"
        "}\n";
}

string CListTypeStrings::GetResetCode(const string& var) const
{
    return var+".clear();\n";
}

CMapTypeStrings::CMapTypeStrings(const string& templateName,
                                 AutoPtr<CTypeStrings> keyType,
                                 AutoPtr<CTypeStrings> valueType,
                                 const string& namespaceName,
                                 const CDataType* dataType)
    : CParent(templateName, keyType, valueType, namespaceName, dataType)
{
}

CMapTypeStrings::~CMapTypeStrings(void)
{
}

string CMapTypeStrings::GetDestructionCode(const string& expr) const
{
    string code;
    string iter;
    static int level = 0;
    try {
        level++;
        iter = "mapIter"+NStr::IntToString(level);
        code = Tabbed(GetArg1Type()->GetDestructionCode(iter+"->first")+
                      GetArg2Type()->GetDestructionCode(iter+"->second"),
                      "        ");
    }
    catch (CDatatoolException& exp) {
        level--;
        NCBI_RETHROW_SAME(exp,"CMapTypeStrings::GetDestructionCode: failed");
    }
    catch (...) {
        level--;
        throw;
    }
    level--;
    if ( code.empty() )
        return string();
    return
        "{\n"
        "    for ( "+GetCType(CNamespace::KEmptyNamespace)+"::iterator "+iter+" = ("+expr+").begin(); "+iter+" != ("+expr+").end(); ++"+iter+" ) {\n"
        +code+
        "    }\n"
        "}\n";
}

string CMapTypeStrings::GetResetCode(const string& var) const
{
    return var+".clear();\n";
}

CVectorTypeStrings::CVectorTypeStrings(const string& charType,
                                       const string& namespaceName,
                                       const CDataType* dataType,
                                       const CComments& comments)
    : CParent(namespaceName, dataType, comments), m_CharType(charType)
{
}

CVectorTypeStrings::~CVectorTypeStrings(void)
{
}

CTypeStrings::EKind CVectorTypeStrings::GetKind(void) const
{
    return eKindOther;
}

void CVectorTypeStrings::GenerateTypeCode(CClassContext& ctx) const
{
    ctx.HPPIncludes().insert("<vector>");
}

string CVectorTypeStrings::GetCType(const CNamespace& ns) const
{
    return ns.GetNamespaceRef(CNamespace::KSTDNamespace)+"vector< " + m_CharType + " >";
}

string CVectorTypeStrings::GetPrefixedCType(const CNamespace& ns,
                                            const string& /*methodPrefix*/) const
{
    return GetCType(ns);
}

string CVectorTypeStrings::GetRef(const CNamespace& /*ns*/) const
{
    return "STL_CHAR_vector, ("+m_CharType+')';
}

string CVectorTypeStrings::GetResetCode(const string& var) const
{
    return var+".clear();\n";
}

END_NCBI_SCOPE
