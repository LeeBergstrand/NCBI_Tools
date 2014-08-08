/*  $Id: version_item.cpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author:  Mati Shomrat, NCBI
*
* File Description:
*   flat-file generator -- version item implementation
*
*/
#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>

#include <objects/seq/Bioseq.hpp>

#include <objtools/format/formatter.hpp>
#include <objtools/format/text_ostream.hpp>
#include <objtools/format/items/version_item.hpp>
#include <objtools/format/context.hpp>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


CVersionItem::CVersionItem(CBioseqContext& ctx) :
    CFlatItem(&ctx),
    m_Gi(0)
{
    x_GatherInfo(ctx);
}


void CVersionItem::Format
(IFormatter& formatter,
 IFlatTextOStream& text_os) const

{
    formatter.FormatVersion(*this, text_os);
}


int CVersionItem::GetGi(void) const
{
    return m_Gi;
}


const string& CVersionItem::GetAccession(void) const
{
    return m_Accession;
}


/***************************************************************************/
/*                                  PRIVATE                                */
/***************************************************************************/


void CVersionItem::x_GatherInfo(CBioseqContext& ctx)
{
    if ( ctx.GetPrimaryId() != 0 ) {
        const CSeq_id& id = *ctx.GetPrimaryId();
        switch ( id.Which() ) {
        case CSeq_id::e_Genbank:
        case CSeq_id::e_Embl:
        case CSeq_id::e_Ddbj:
        case CSeq_id::e_Other:
        case CSeq_id::e_Pir:
        case CSeq_id::e_Swissprot:
        case CSeq_id::e_Prf:
        case CSeq_id::e_Pdb:
        case CSeq_id::e_Tpg:
        case CSeq_id::e_Tpe:
        case CSeq_id::e_Tpd:
            m_Accession = ctx.GetAccession();
            break;
        default:
            break;
        }        
    }

    ITERATE (CBioseq::TId, id, ctx.GetBioseqIds()) {
        if ( (*id)->IsGi() ) { 
            m_Gi = (*id)->GetGi();
            break;
        }
    }
}


END_SCOPE(objects)
END_NCBI_SCOPE
