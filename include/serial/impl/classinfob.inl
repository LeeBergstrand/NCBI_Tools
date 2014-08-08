#if defined(CLASSINFOB__HPP)  &&  !defined(CLASSINFOB__INL)
#define CLASSINFOB__INL

/*  $Id: classinfob.inl 103491 2007-05-04 17:18:18Z kazimird $
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

inline
const CItemsInfo& CClassTypeInfoBase::GetItems(void) const
{
    return m_Items;
}

inline
CItemsInfo& CClassTypeInfoBase::GetItems(void)
{
    return m_Items;
}

inline
const CItemInfo* CClassTypeInfoBase::GetItemInfo(const string& name) const
{
    return GetItems().GetItemInfo(GetItems().Find(name));
}

inline
const type_info& CClassTypeInfoBase::GetId(void) const
{
    _ASSERT(m_Id);
    return *m_Id;
}

inline
CClassTypeInfoBase::CIterator::CIterator(const CClassTypeInfoBase* type)
    : CParent(type->GetItems())
{
}

inline
CClassTypeInfoBase::CIterator::CIterator(const CClassTypeInfoBase* type,
                                         TMemberIndex index)
    : CParent(type->GetItems(), index)
{
}

#endif /* def CLASSINFOB__HPP  &&  ndef CLASSINFOB__INL */
