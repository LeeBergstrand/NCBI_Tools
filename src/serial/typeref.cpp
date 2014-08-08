/*  $Id: typeref.cpp 152541 2009-02-17 20:40:02Z grichenk $
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
*   !!! PUT YOUR DESCRIPTION HERE !!!
*/

#include <ncbi_pch.hpp>
#include <serial/impl/typeref.hpp>
#include <serial/typeinfo.hpp>
#include <serial/exception.hpp>
#include <corelib/ncbithr.hpp>
#include <serial/serialimpl.hpp>

BEGIN_NCBI_SCOPE


//DEFINE_STATIC_MUTEX(s_TypeRefMutex);


CTypeRef::CTypeRef(TGet1Proc getter, const CTypeRef& arg)
    : m_Getter(sx_GetResolve), m_ReturnData(0)
{
    m_ResolveData = new CGet1TypeInfoSource(getter, arg);
}


CTypeRef::CTypeRef(TGet2Proc getter,
                   const CTypeRef& arg1, const CTypeRef& arg2)
    : m_Getter(sx_GetResolve), m_ReturnData(0)
{
    m_ResolveData = new CGet2TypeInfoSource(getter, arg1, arg2);
}


CTypeRef::CTypeRef(TGet2Proc getter,
                  const CTypeRef& arg1,
                  TGet1Proc getter2, const CTypeRef& arg2)
    : m_Getter(sx_GetResolve), m_ReturnData(0)
{
    m_ResolveData = new CGet2TypeInfoSource(getter,
                                            arg1,
                                            CTypeRef(getter2, arg2));
}


CTypeRef::CTypeRef(TGet2Proc getter,
                   TGet1Proc getter1, const CTypeRef& arg1,
                   const CTypeRef& arg2)
    : m_Getter(sx_GetResolve), m_ReturnData(0)
{
    m_ResolveData = new CGet2TypeInfoSource(getter,
                                            CTypeRef(getter1, arg1),
                                            arg2);
}


CTypeRef::CTypeRef(TGet2Proc getter,
                   TGet1Proc getter1, const CTypeRef& arg1,
                   TGet1Proc getter2, const CTypeRef& arg2)
    : m_Getter(sx_GetResolve), m_ReturnData(0)
{
    m_ResolveData = new CGet2TypeInfoSource(getter,
                                            CTypeRef(getter1, arg1),
                                            CTypeRef(getter2, arg2));
}


CTypeRef& CTypeRef::operator=(const CTypeRef& typeRef)
{
    if ( this != &typeRef ) {
        Unref();
        Assign(typeRef);
    }
    return *this;
}

void CTypeRef::Unref(void)
{
    if ( m_Getter == sx_GetResolve ) {
        CMutexGuard guard(GetTypeInfoMutex()/*s_TypeRefMutex*/);
        if ( m_Getter == sx_GetResolve ) {
            m_Getter = sx_GetAbort;
            if ( m_ResolveData->m_RefCount.Add(-1) <= 0 ) {
                delete m_ResolveData;
                m_ResolveData = 0;
            }
        }
    }
    m_Getter = sx_GetAbort;
    m_ReturnData = 0;
}

void CTypeRef::Assign(const CTypeRef& typeRef)
{
    if ( typeRef.m_ReturnData ) {
        m_ReturnData = typeRef.m_ReturnData;
        m_Getter = sx_GetReturn;
    }
    else {
        CMutexGuard guard(GetTypeInfoMutex()/*s_TypeRefMutex*/);
        m_ReturnData = typeRef.m_ReturnData;
        m_Getter = typeRef.m_Getter;
        if ( m_Getter == sx_GetProc ) {
            m_GetProcData = typeRef.m_GetProcData;
        }
        else if ( m_Getter == sx_GetResolve ) {
            (m_ResolveData = typeRef.m_ResolveData)->m_RefCount.Add(1);
        }
    }
}

TTypeInfo CTypeRef::sx_GetAbort(const CTypeRef& typeRef)
{
    CMutexGuard guard(GetTypeInfoMutex()/*s_TypeRefMutex*/);
    if (typeRef.m_Getter != sx_GetAbort)
        return typeRef.m_Getter(typeRef);
    NCBI_THROW(CSerialException,eFail, "uninitialized type ref");
}

TTypeInfo CTypeRef::sx_GetReturn(const CTypeRef& typeRef)
{
    return typeRef.m_ReturnData;
}

TTypeInfo CTypeRef::sx_GetProc(const CTypeRef& typeRef)
{
    CMutexGuard guard(GetTypeInfoMutex()/*s_TypeRefMutex*/);
    if (typeRef.m_Getter != sx_GetProc)
        return typeRef.m_Getter(typeRef);
    TTypeInfo typeInfo = typeRef.m_GetProcData();
    if ( !typeInfo )
        NCBI_THROW(CSerialException,eFail, "cannot resolve type ref");
    const_cast<CTypeRef&>(typeRef).m_ReturnData = typeInfo;
    const_cast<CTypeRef&>(typeRef).m_Getter = sx_GetReturn;
    return typeInfo;
}

TTypeInfo CTypeRef::sx_GetResolve(const CTypeRef& typeRef)
{
    CMutexGuard guard(GetTypeInfoMutex()/*s_TypeRefMutex*/);
    if (typeRef.m_Getter != sx_GetResolve)
        return typeRef.m_Getter(typeRef);
    TTypeInfo typeInfo = typeRef.m_ResolveData->GetTypeInfo();
    if ( !typeInfo )
        NCBI_THROW(CSerialException,eFail, "cannot resolve type ref");
    if ( typeRef.m_ResolveData->m_RefCount.Add(-1) <= 0 ) {
        delete typeRef.m_ResolveData;
        const_cast<CTypeRef&>(typeRef).m_ResolveData = 0;
    }
    const_cast<CTypeRef&>(typeRef).m_ReturnData = typeInfo;
    const_cast<CTypeRef&>(typeRef).m_Getter = sx_GetReturn;
    return typeInfo;
}

CTypeInfoSource::CTypeInfoSource(void)
    : m_RefCount(1)
{
}

CTypeInfoSource::~CTypeInfoSource(void)
{
    _ASSERT(m_RefCount.Get() == 0);
}

CGet1TypeInfoSource::CGet1TypeInfoSource(CTypeRef::TGet1Proc getter,
                                         const CTypeRef& arg)
    : m_Getter(getter), m_Argument(arg)
{
}

CGet1TypeInfoSource::~CGet1TypeInfoSource(void)
{
}

TTypeInfo CGet1TypeInfoSource::GetTypeInfo(void)
{
    return m_Getter(m_Argument.Get());
}

CGet2TypeInfoSource::CGet2TypeInfoSource(CTypeRef::TGet2Proc getter,
                                         const CTypeRef& arg1,
                                         const CTypeRef& arg2)
    : m_Getter(getter), m_Argument1(arg1), m_Argument2(arg2)
{
}

CGet2TypeInfoSource::~CGet2TypeInfoSource(void)
{
}

TTypeInfo CGet2TypeInfoSource::GetTypeInfo(void)
{
    return m_Getter(m_Argument1.Get(), m_Argument2.Get());
}

END_NCBI_SCOPE
