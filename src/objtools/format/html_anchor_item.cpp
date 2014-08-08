/*  $Id: html_anchor_item.cpp 294826 2011-05-27 11:19:20Z kornbluh $
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
* Author:  Michael Kornbluh, NCBI
*
* File Description:
*   This provides an (invisible) HTML anchor item (<a name=...></a>)
*
*/
#include <ncbi_pch.hpp>

#include <objtools/format/items/html_anchor_item.hpp>

#include <objtools/format/context.hpp>
#include <objtools/format/formatter.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

CHtmlAnchorItem::CHtmlAnchorItem( CBioseqContext& ctx, const string &label_core )
    : CFlatItem(&ctx), m_LabelCore(label_core), m_GI(ctx.GetGI())
{
    x_GatherInfo(ctx);
}

void CHtmlAnchorItem::Format(IFormatter& formatter, IFlatTextOStream& text_os) const
{
    formatter.FormatHtmlAnchor(*this, text_os);
}

void CHtmlAnchorItem::x_GatherInfo(CBioseqContext& ctx)
{
    // check if we should skip this
    if( ! ctx.Config().DoHTML() || ! ctx.Config().IsModeEntrez() ) {
        x_SetSkip();
    }
}

END_SCOPE(objects)
END_NCBI_SCOPE
