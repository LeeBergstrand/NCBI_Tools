/*  $Id: flat_file_generator.cpp 197676 2010-07-21 00:56:33Z dicuccio $
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
* Author:  Mati Shomrat
*
* File Description:
*   User interface for generating flat file reports from ASN.1
*   
*/
#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbiobj.hpp>
#include <connect/ncbi_conn_stream.hpp>

#include <objects/seqset/Seq_entry.hpp>
#include <objects/submit/Seq_submit.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/bioseq_handle.hpp>
#include <objmgr/seq_entry_handle.hpp>
#include <objmgr/util/sequence.hpp>

#include <objtools/format/flat_file_generator.hpp>
#include <objtools/format/text_ostream.hpp>
#include <objtools/format/item_formatter.hpp>
#include <objtools/format/ostream_text_ostream.hpp>
#include <objtools/format/format_item_ostream.hpp>
#include <objtools/format/gather_items.hpp>
#include <objtools/format/context.hpp>
#include <objtools/format/flat_expt.hpp>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)
USING_SCOPE(sequence);


//////////////////////////////////////////////////////////////////////////////
//
// PUBLIC

// constructor
CFlatFileGenerator::CFlatFileGenerator(const CFlatFileConfig& cfg) :
    m_Ctx(new CFlatFileContext(cfg))
{
     if ( !m_Ctx ) {
         NCBI_THROW(CFlatException, eInternal, "Unable to initialize context");
     }
}


CFlatFileGenerator::CFlatFileGenerator
(CFlatFileConfig::TFormat format,
 CFlatFileConfig::TMode   mode,
 CFlatFileConfig::TStyle  style,
 CFlatFileConfig::TFlags  flags,
 CFlatFileConfig::TView   view) :
    m_Ctx(new CFlatFileContext(CFlatFileConfig(format, mode, style, flags, view)))
{
    if ( !m_Ctx ) {
       NCBI_THROW(CFlatException, eInternal, "Unable to initialize context");
    }
}


// destructor

CFlatFileGenerator::~CFlatFileGenerator(void)
{
}


// Set annotation selector for feature gathering

SAnnotSelector& CFlatFileGenerator::SetAnnotSelector(void)
{
    return m_Ctx->SetAnnotSelector();
}


// Generate a flat-file report for a Seq-entry
void CFlatFileGenerator::Generate
(const CSeq_entry_Handle& entry,
 CFlatItemOStream& item_os)
{
    _ASSERT(entry  &&  entry.Which() != CSeq_entry::e_not_set);

    /// archive a copy of the annot selector before we generate!
    SAnnotSelector sel = m_Ctx->SetAnnotSelector();
    m_Ctx->SetEntry(entry);

    CFlatFileConfig::TFormat format = m_Ctx->GetConfig().GetFormat();
    CRef<CFlatItemFormatter> formatter(CFlatItemFormatter::New(format));
    if ( !formatter ) {
        NCBI_THROW(CFlatException, eInternal, "Unable to initialize formatter");
    }
    formatter->SetContext(*m_Ctx);
    item_os.SetFormatter(formatter);

    CRef<CFlatGatherer> gatherer(CFlatGatherer::New(format));
    if ( !gatherer ) {
        NCBI_THROW(CFlatException, eInternal, "Unable to initialize gatherer");
    }
    gatherer->Gather(*m_Ctx, item_os);

    /// reset the context, but preserve our selector
    /// we do this a bit oddly since resetting the context erases the selector;
    /// since the caller is reusing this object (most likely), we automatically
    /// restore the selector to its former glory
    m_Ctx->Reset();
    m_Ctx->SetAnnotSelector() = sel;
}


void CFlatFileGenerator::Generate
(CSeq_submit& submit,
 CScope& scope,
 CFlatItemOStream& item_os)
{
    _ASSERT(submit.CanGetData());
    _ASSERT(submit.CanGetSub());
    _ASSERT(submit.GetData().IsEntrys());
    _ASSERT(!submit.GetData().GetEntrys().empty());

    // NB: though the spec specifies a submission may contain multiple entries
    // this is not the case. A submission should only have a single Top-level
    // Seq-entry
    CRef<CSeq_entry> e(submit.SetData().SetEntrys().front());
    if (e.NotEmpty()) {
        // get Seq_entry_Handle from scope
        CSeq_entry_Handle entry;
        try {
            entry = scope.GetSeq_entryHandle(*e);
        } catch (CException&) {}

        if (!entry) {  // add to scope if not already in it
            entry = scope.AddTopLevelSeqEntry(*e);
        }
        // "remember" the submission block
        m_Ctx->SetSubmit(submit.GetSub());

        Generate(entry, item_os);
    }
}


void CFlatFileGenerator::Generate
(const CSeq_loc& loc,
 CScope& scope,
 CFlatItemOStream& item_os)
{
    CBioseq_Handle bsh = GetBioseqFromSeqLoc(loc, scope);
    if (!bsh) {
        NCBI_THROW(CFlatException, eInvalidParam, "location not in scope");
    }
    CSeq_entry_Handle entry = bsh.GetParentEntry();
    if (!entry) {
        NCBI_THROW(CFlatException, eInvalidParam, "Id not in scope");
    }
    CRef<CSeq_loc> location(new CSeq_loc);
    location->Assign(loc);
    m_Ctx->SetLocation(location);

    Generate(entry, item_os);
}


void CFlatFileGenerator::Generate
(const CSeq_id& id,
 const TRange& range,
 ENa_strand strand,
 CScope& scope,
 CFlatItemOStream& item_os)
{
    CRef<CSeq_id> id2(new CSeq_id);
    id2->Assign(id);
    CRef<CSeq_loc> loc;
    if ( range.IsWhole() ) {
        loc.Reset(new CSeq_loc);
        loc->SetWhole(*id2);
    } else {
        loc.Reset(new CSeq_loc(*id2, range.GetFrom(), range.GetTo(), strand));
    }
    if ( loc ) {
        Generate(*loc, scope, item_os);
    }
}


void CFlatFileGenerator::Generate
(CSeq_submit& submit,
 CScope& scope,
 CNcbiOstream& os)
{
    CRef<CFlatItemOStream> 
        item_os(new CFormatItemOStream(new COStreamTextOStream(os)));

    Generate(submit, scope, *item_os);
}


void CFlatFileGenerator::Generate
(const CSeq_entry_Handle& entry,
 CNcbiOstream& os)
{
    CRef<CFlatItemOStream> 
        item_os(new CFormatItemOStream(new COStreamTextOStream(os)));

    Generate(entry, *item_os);
}


void CFlatFileGenerator::Generate
(const CSeq_loc& loc,
 CScope& scope,
 CNcbiOstream& os)
{
    CRef<CFlatItemOStream> 
        item_os(new CFormatItemOStream(new COStreamTextOStream(os)));

    Generate(loc, scope, *item_os);
}


void CFlatFileGenerator::Generate
(const CSeq_id& id,
 const TRange& range, 
 ENa_strand strand,
 CScope& scope,
 CNcbiOstream& os)
{
    CRef<CFlatItemOStream> 
        item_os(new CFormatItemOStream(new COStreamTextOStream(os)));

    Generate(id, range, strand, scope, *item_os);
}


//void CFlatFileGenerator::Reset(void)
//{
//    m_Ctx->Reset();
//}


string CFlatFileGenerator::GetSeqFeatText
(const CMappedFeat& feat,
 CScope& scope,
 const CFlatFileConfig& cfg)
{
    CBioseq_Handle seq = sequence::GetBioseqFromSeqLoc(feat.GetLocation(), scope);
    if (!seq) {
        NCBI_THROW(CFlatException, eUnknown, "Bioseq not found for feature");
    }
    CRef<CFlatItemFormatter> formatter(CFlatItemFormatter::New(cfg.GetFormat()));
    CRef<CFlatFileContext> ctx(new CFlatFileContext(cfg));

    ctx->SetEntry(seq.GetParentEntry());
    formatter->SetContext(*ctx);

    CConn_MemoryStream os;
    CFormatItemOStream item_os(new COStreamTextOStream(os));
    item_os.SetFormatter(formatter);

    CBioseqContext bctx(seq, *ctx);
    CConstRef<IFlatItem> item;
    if (feat.GetData().IsBiosrc()) {
        item.Reset( new CSourceFeatureItem(feat, bctx, &feat.GetLocation()) );
        item_os << item;
    } else {
        item.Reset( new CFeatureItem(feat, bctx, &feat.GetLocation()) );
        item_os << item;
    }

    string text;
    os.ToString(&text);
    return text;
}


END_SCOPE(objects)
END_NCBI_SCOPE
