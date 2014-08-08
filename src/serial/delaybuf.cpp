/*  $Id: delaybuf.cpp 380536 2012-11-14 14:51:26Z vasilche $
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
#include <corelib/ncbistd.hpp>
#include <serial/delaybuf.hpp>
#include <serial/objostr.hpp>
#include <serial/objistr.hpp>
#include <util/bytesrc.hpp>
#include <serial/impl/item.hpp>
#include <serial/impl/stdtypes.hpp>

BEGIN_NCBI_SCOPE

CDelayBuffer::~CDelayBuffer(void)
{
}

void CDelayBuffer::SetData(const CItemInfo* itemInfo, TObjectPtr object,
                           ESerialDataFormat dataFormat, TFormatFlags flags,
                           CByteSource& data)
{
    _ASSERT(!Delayed());

    m_Info.reset(new SInfo(itemInfo, object, dataFormat, flags, data));
}

void CDelayBuffer::Forget(void)
{
    m_Info.reset(0);
}

void CDelayBuffer::DoUpdate(void)
{
    _ASSERT(m_Info.get() != 0);
    SInfo& info = *m_Info;

    {
        auto_ptr<CObjectIStream> in(CObjectIStream::Create(info.m_DataFormat,
                                                           *info.m_Source));
        in->SetFlags(info.m_Flags);
        info.m_ItemInfo->UpdateDelayedBuffer(*in, info.m_Object);
        _VERIFY(in->EndOfData());
    }

    m_Info.reset(0);
}

TMemberIndex CDelayBuffer::GetIndex(void) const
{
    const SInfo* info = m_Info.get();
    if ( !info )
        return kInvalidMember;
    else
        return info->m_ItemInfo->GetIndex();
}

CDelayBuffer::SInfo::SInfo(const CItemInfo* itemInfo, TObjectPtr object,
                           ESerialDataFormat format, TFormatFlags flags,
                           CByteSource& source)
    : m_ItemInfo(itemInfo), m_Object(object),
      m_DataFormat(format), m_Flags(flags),
      m_Source(&source)
{
}

CDelayBuffer::SInfo::~SInfo(void)
{
}

END_NCBI_SCOPE
