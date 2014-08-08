/*  $Id: seq_align_handle.cpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author: Aleksey Grichenko, Eugene Vasilchenko
*
* File Description:
*   Seq-align handle
*
*/


#include <ncbi_pch.hpp>
#include <objmgr/seq_align_handle.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/impl/seq_annot_info.hpp>
#include <objmgr/impl/scope_impl.hpp>

#include <objmgr/impl/seq_annot_edit_commands.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


CSeq_align_Handle::CSeq_align_Handle(const CSeq_annot_Handle& annot,
                                     TIndex index)
    : m_Annot(annot),
      m_AnnotIndex(index)
{
    _ASSERT(!IsRemoved());
    _ASSERT(annot.x_GetInfo().GetInfo(index).IsAlign());
}


void CSeq_align_Handle::Reset(void)
{
    m_AnnotIndex = eNull;
    m_Annot.Reset();
}


const CSeq_align& CSeq_align_Handle::x_GetSeq_align(void) const
{
    _ASSERT(m_Annot);
    const CAnnotObject_Info& info = m_Annot.x_GetInfo().GetInfo(m_AnnotIndex);
    if ( info.IsRemoved() ) {
        NCBI_THROW(CObjMgrException, eInvalidHandle,
                   "CSeq_align_Handle: removed");
    }
    return info.GetAlign();
}


CConstRef<CSeq_align> CSeq_align_Handle::GetSeq_align(void) const
{
    return ConstRef(&x_GetSeq_align());
}


bool CSeq_align_Handle::IsRemoved(void) const
{
    return m_Annot.x_GetInfo().GetInfo(m_AnnotIndex).IsRemoved();
}


void CSeq_align_Handle::Remove(void) const
{
    typedef CSeq_annot_Remove_EditCommand<CSeq_align_Handle> TCommand;
    CCommandProcessor processor(GetAnnot().x_GetScopeImpl());
    processor.run(new TCommand(*this));
}


void CSeq_align_Handle::Replace(const CSeq_align& new_obj) const
{
    typedef CSeq_annot_Replace_EditCommand<CSeq_align_Handle> TCommand;
    CCommandProcessor processor(GetAnnot().x_GetScopeImpl());
    processor.run(new TCommand(*this, new_obj));
}

void CSeq_align_Handle::Update(void) const
{
    GetAnnot().GetEditHandle().x_GetInfo().Update(m_AnnotIndex);
}

void CSeq_align_Handle::x_RealRemove(void) const
{
    GetAnnot().GetEditHandle().x_GetInfo().Remove(m_AnnotIndex);
    _ASSERT(IsRemoved());
}


void CSeq_align_Handle::x_RealReplace(const CSeq_align& new_obj) const
{
    GetAnnot().GetEditHandle().x_GetInfo().Replace(m_AnnotIndex, new_obj);
    _ASSERT(!IsRemoved());
}

END_SCOPE(objects)
END_NCBI_SCOPE
