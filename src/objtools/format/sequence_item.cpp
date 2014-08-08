/*  $Id: sequence_item.cpp 103491 2007-05-04 17:18:18Z kazimird $
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
*   flat-file generator -- origin item implementation
*
*/
#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>

#include <objects/seq/Bioseq.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <objmgr/util/sequence.hpp>

#include <objtools/format/formatter.hpp>
#include <objtools/format/text_ostream.hpp>
#include <objtools/format/items/sequence_item.hpp>
#include <objtools/format/context.hpp>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


CSequenceItem::CSequenceItem
(TSeqPos from,
 TSeqPos to,
 bool first, 
 CBioseqContext& ctx) :
    CFlatItem(&ctx),
    m_From(from), m_To(to), m_First(first)
{
    x_GatherInfo(ctx);
}


void CSequenceItem::Format
(IFormatter& formatter,
 IFlatTextOStream& text_os) const

{
    formatter.FormatSequence(*this, text_os);
}


/***************************************************************************/
/*                                  PRIVATE                                */
/***************************************************************************/


void CSequenceItem::x_GatherInfo(CBioseqContext& ctx)
{
    x_SetObject(*ctx.GetHandle().GetBioseqCore());

    const CSeq_loc& loc = ctx.GetLocation();
    m_Sequence = CSeqVector(loc, ctx.GetScope());

    CSeqVector::TCoding coding = CSeq_data::e_Iupacna;
    if (ctx.IsProt()) {
        coding = ctx.Config().IsModeRelease() ?
            CSeq_data::e_Iupacaa : CSeq_data::e_Ncbieaa;
    }
    m_Sequence.SetCoding(coding);
}


END_SCOPE(objects)
END_NCBI_SCOPE
