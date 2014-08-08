#ifndef OBJTOOLS_FORMAT_ITEMS___HTML_ANCHOR_ITEM__HPP
#define OBJTOOLS_FORMAT_ITEMS___HTML_ANCHOR_ITEM__HPP

/*  $Id: html_anchor_item.hpp 338787 2011-09-22 15:16:57Z kornbluh $
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

#include <objtools/format/items/item_base.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CHtmlAnchorItem : public CFlatItem
{
public:
    CHtmlAnchorItem( CBioseqContext& ctx, const string &label_core );
    void Format(IFormatter& formatter, IFlatTextOStream& text_os) const;

    const string &GetLabelCore(void) const { return m_LabelCore; }
    int           GetGI(void)        const { return m_GI; }

private:
    void x_GatherInfo(CBioseqContext& ctx);

    // data
    const string  m_LabelCore;
    const int     m_GI;
};

END_SCOPE(objects)
END_NCBI_SCOPE

#endif  /* OBJTOOLS_FORMAT_ITEMS___HTML_ANCHOR_ITEM__HPP */
