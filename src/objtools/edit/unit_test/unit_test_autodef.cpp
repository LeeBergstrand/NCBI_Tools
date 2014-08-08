/*  $Id: unit_test_autodef.cpp 390007 2013-02-22 15:50:37Z bollin $
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
* Author:  Colleen Bollin, NCBI
*
* File Description:
*   Unit tests for the validator.
*
* ===========================================================================
*/

#include <ncbi_pch.hpp>

#include "unit_test_autodef.hpp"

#include <corelib/ncbi_system.hpp>

// This macro should be defined before inclusion of test_boost.hpp in all
// "*.cpp" files inside executable except one. It is like function main() for
// non-Boost.Test executables is defined only in one *.cpp file - other files
// should not include it. If NCBI_BOOST_NO_AUTO_TEST_MAIN will not be defined
// then test_boost.hpp will define such "main()" function for tests.
//
// Usually if your unit tests contain only one *.cpp file you should not
// care about this macro at all.
//
//#define NCBI_BOOST_NO_AUTO_TEST_MAIN


// This header must be included before all Boost.Test headers if there are any
#include <corelib/test_boost.hpp>

#include <objects/biblio/Id_pat.hpp>
#include <objects/biblio/Title.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/general/Dbtag.hpp>
#include <objects/general/User_object.hpp>
#include <objects/medline/Medline_entry.hpp>
#include <objects/misc/sequence_macros.hpp>
#include <objects/pub/Pub_equiv.hpp>
#include <objects/pub/Pub.hpp>
#include <objects/seqset/Seq_entry.hpp>
#include <objects/seq/GIBB_mol.hpp>
#include <objects/seq/Seq_ext.hpp>
#include <objects/seq/Delta_ext.hpp>
#include <objects/seq/Delta_seq.hpp>
#include <objects/seq/Seq_literal.hpp>
#include <objects/seq/Ref_ext.hpp>
#include <objects/seq/Map_ext.hpp>
#include <objects/seq/Seg_ext.hpp>
#include <objects/seq/Seq_gap.hpp>
#include <objects/seq/Seq_data.hpp>
#include <objects/seq/Seq_descr.hpp>
#include <objects/seq/Seqdesc.hpp>
#include <objects/seq/MolInfo.hpp>
#include <objects/seq/Pubdesc.hpp>
#include <objects/seq/Seq_hist.hpp>
#include <objects/seq/Seq_hist_rec.hpp>
#include <objects/seqalign/Dense_seg.hpp>
#include <objects/seqblock/GB_block.hpp>
#include <objects/seqblock/EMBL_block.hpp>
#include <objects/seqfeat/BioSource.hpp>
#include <objects/seqfeat/Org_ref.hpp>
#include <objects/seqfeat/OrgName.hpp>
#include <objects/seqfeat/SubSource.hpp>
#include <objects/seqfeat/Imp_feat.hpp>
#include <objects/seqfeat/Cdregion.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/PDB_seq_id.hpp>
#include <objects/seqloc/Giimport_id.hpp>
#include <objects/seqloc/Patent_seq_id.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/bioseq_ci.hpp>
#include <objmgr/feat_ci.hpp>
#include <objmgr/seq_vector.hpp>
#include <objmgr/util/sequence.hpp>
#include <objmgr/seqdesc_ci.hpp>
#include <objects/seq/seqport_util.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>
#include <corelib/ncbiapp.hpp>

#include <objtools/edit/autodef.hpp>


// for writing out tmp files
#include <serial/objostrasn.hpp>
#include <serial/objostrasnb.hpp>

extern const char* sc_TestEntryCollidingLocusTags;

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)





NCBITEST_INIT_TREE()
{
    if ( !CNcbiApplication::Instance()->GetConfig().HasEntry("NCBI", "Data") ) {
    }
}

static bool s_debugMode = false;

NCBITEST_INIT_CMDLINE(arg_desc)
{
    // Here we make descriptions of command line parameters that we are
    // going to use.

    arg_desc->AddFlag( "debug_mode",
        "Debugging mode writes errors seen for each test" );
}

NCBITEST_AUTO_INIT()
{
    // initialization function body

    const CArgs& args = CNcbiApplication::Instance()->GetArgs();
    if (args["debug_mode"]) {
        s_debugMode = true;
    }
}


static CRef<CSeq_entry> BuildSequence()
{
    CRef<CSeq_entry> entry(new CSeq_entry());
    entry->SetSeq().SetInst().SetMol(CSeq_inst::eMol_dna);
    entry->SetSeq().SetInst().SetRepr(CSeq_inst::eRepr_raw);
    entry->SetSeq().SetInst().SetSeq_data().SetIupacna().Set("AATTGGCCAAAATTGGCCAAAATTGGCCAAAATTGGCCAAAATTGGCCAAAATTGGCCAA");
    entry->SetSeq().SetInst().SetLength(60);

    CRef<CSeq_id> id(new CSeq_id());
    id->SetLocal().SetStr ("good");
    entry->SetSeq().SetId().push_back(id);

    CRef<CSeqdesc> mdesc(new CSeqdesc());
    mdesc->SetMolinfo().SetBiomol(CMolInfo::eBiomol_genomic);    
    entry->SetSeq().SetDescr().Set().push_back(mdesc);
    return entry;
}


static CRef<CSeqdesc> AddSource (CRef<CSeq_entry> entry, string taxname)
{
    CRef<CSeqdesc> odesc(new CSeqdesc());
    odesc->SetSource().SetOrg().SetTaxname(taxname);

    if (entry->IsSeq()) {
        entry->SetSeq().SetDescr().Set().push_back(odesc);
    } else if (entry->IsSet()) {
        entry->SetSet().SetDescr().Set().push_back(odesc);
    }
    return odesc;
}


static void AddTitle (CRef<CSeq_entry> entry, string defline)
{
    CRef<CSeqdesc> odesc(new CSeqdesc());
    odesc->SetTitle(defline);

    if (entry->IsSeq()) {
        entry->SetSeq().SetDescr().Set().push_back(odesc);
    } else if (entry->IsSet()) {
        if (entry->GetSet().IsSetClass() && entry->GetSet().GetClass() == CBioseq_set::eClass_nuc_prot) {
            AddTitle (entry->SetSet().SetSeq_set().front(), defline);
        } else {
            entry->SetSet().SetDescr().Set().push_back(odesc);
        }
    }
}


static void CheckDeflineMatches(CRef<CSeq_entry> entry, bool use_best = false)
{
    CRef<CObjectManager> object_manager = CObjectManager::GetInstance();

    CRef<CScope> scope(new CScope(*object_manager));
    CSeq_entry_Handle seh = scope->AddTopLevelSeqEntry (*entry);

    objects::CAutoDef autodef;

    // add to autodef 
    autodef.AddSources (seh);

    CRef<CAutoDefModifierCombo> mod_combo;
    if (use_best) {
        mod_combo = autodef.FindBestModifierCombo();
    } else {
        mod_combo = new CAutoDefModifierCombo ();
    }
    CBioseq_CI seq_iter(seh, CSeq_inst::eMol_na);
    for ( ; seq_iter; ++seq_iter ) {
       CBioseq_Handle bh (*seq_iter);
       //Display ID of sequence
       CConstRef<CSeq_id> id = bh.GetSeqId();

       // original defline
       string orig_defline = "";
       CSeqdesc_CI desc_it(bh, CSeqdesc::e_Title, 1);
       if (desc_it) {
           orig_defline = desc_it->GetTitle();
       }
       
       string new_defline = autodef.GetOneDefLine(mod_combo, bh);

       BOOST_CHECK_EQUAL(orig_defline, new_defline);
    }
}


CRef<CSeq_entry> FindNucInSeqEntry(CRef<CSeq_entry> entry)
{
    CRef<CSeq_entry> empty(NULL);
    if (!entry) {
        return empty;
    } else if (entry->IsSeq() && entry->GetSeq().IsNa()) {
        return entry;
    } else if (entry->IsSet()) {
        ITERATE(CBioseq_set::TSeq_set, it, entry->GetSet().GetSeq_set()) {
            CRef<CSeq_entry> rval = FindNucInSeqEntry(*it);
            if (rval) {
                return rval;
            }
        }
    }
    return empty;
}


static void AddFeat (CRef<CSeq_feat> feat, CRef<CSeq_entry> entry)
{
    CRef<CSeq_annot> annot;

    if (entry->IsSeq()) {
        if (!entry->GetSeq().IsSetAnnot() 
            || !entry->GetSeq().GetAnnot().front()->IsFtable()) {
            CRef<CSeq_annot> new_annot(new CSeq_annot());
            entry->SetSeq().SetAnnot().push_back(new_annot);
            annot = new_annot;
        } else {
            annot = entry->SetSeq().SetAnnot().front();
        }
    } else if (entry->IsSet()) {
        if (!entry->GetSet().IsSetAnnot() 
            || !entry->GetSet().GetAnnot().front()->IsFtable()) {
            CRef<CSeq_annot> new_annot(new CSeq_annot());
            entry->SetSet().SetAnnot().push_back(new_annot);
            annot = new_annot;
        } else {
            annot = entry->SetSet().SetAnnot().front();
        }
    }

    if (!feat->IsSetLocation() || feat->GetLocation().Which() == CSeq_loc::e_not_set) {
        CRef<CSeq_entry> nuc_entry = FindNucInSeqEntry(entry);
        if (nuc_entry) {
            CRef<CSeq_id> id(new CSeq_id());
            id->Assign(*(nuc_entry->GetSeq().GetId().front()));
            feat->SetLocation().SetInt().SetId(*id);
            feat->SetLocation().SetInt().SetFrom(0);
            feat->SetLocation().SetInt().SetTo(entry->GetSeq().GetLength() - 1);
        }
    }

    annot->SetData().SetFtable().push_back(feat);
}


static CRef<CSeq_entry> MakeProteinForNucProtSet (string id, string protein_name)
{
    // make protein
    CRef<CBioseq> pseq(new CBioseq());
    pseq->SetInst().SetMol(CSeq_inst::eMol_aa);
    pseq->SetInst().SetRepr(CSeq_inst::eRepr_raw);
    pseq->SetInst().SetSeq_data().SetIupacaa().Set("MPRKTEIN");
    pseq->SetInst().SetLength(8);

    CRef<CSeq_id> pid(new CSeq_id());
    pid->SetLocal().SetStr (id);
    pseq->SetId().push_back(pid);

    CRef<CSeqdesc> mpdesc(new CSeqdesc());
    mpdesc->SetMolinfo().SetBiomol(CMolInfo::eBiomol_peptide);    
    pseq->SetDescr().Set().push_back(mpdesc);

    CRef<CSeq_entry> pentry(new CSeq_entry());
    pentry->SetSeq(*pseq);

    CRef<CSeq_feat> feat (new CSeq_feat());
    feat->SetData().SetProt().SetName().push_back(protein_name);
    feat->SetLocation().SetInt().SetId().SetLocal().SetStr(id);
    feat->SetLocation().SetInt().SetFrom(0);
    feat->SetLocation().SetInt().SetTo(7);
    AddFeat (feat, pentry);

    return pentry;
}


static CRef<CSeq_feat> MakeCDSForNucProtSet (string nuc_id, string prot_id)
{
    CRef<CSeq_feat> cds (new CSeq_feat());
    cds->SetData().SetCdregion();
    cds->SetProduct().SetWhole().SetLocal().SetStr(prot_id);
    cds->SetLocation().SetInt().SetId().SetLocal().SetStr(nuc_id);
    cds->SetLocation().SetInt().SetFrom(0);
    cds->SetLocation().SetInt().SetTo(26);
    return cds;
}


static CRef<CSeq_entry> BuildNucProtSet(string protein_name)
{
    CRef<CBioseq_set> set(new CBioseq_set());
    set->SetClass(CBioseq_set::eClass_nuc_prot);

    // make nucleotide
    CRef<CBioseq> nseq(new CBioseq());
    nseq->SetInst().SetMol(CSeq_inst::eMol_dna);
    nseq->SetInst().SetRepr(CSeq_inst::eRepr_raw);
    nseq->SetInst().SetSeq_data().SetIupacna().Set("ATGCCCAGAAAAACAGAGATAAACTAAGGGATGCCCAGAAAAACAGAGATAAACTAAGGG");
    nseq->SetInst().SetLength(60);

    CRef<CSeq_id> id(new CSeq_id());
    id->SetLocal().SetStr ("nuc");
    nseq->SetId().push_back(id);

    CRef<CSeqdesc> mdesc(new CSeqdesc());
    mdesc->SetMolinfo().SetBiomol(CMolInfo::eBiomol_genomic);    
    nseq->SetDescr().Set().push_back(mdesc);

    CRef<CSeq_entry> nentry(new CSeq_entry());
    nentry->SetSeq(*nseq);

    set->SetSeq_set().push_back(nentry);

    // make protein
    CRef<CSeq_entry> pentry = MakeProteinForNucProtSet("prot", protein_name);

    set->SetSeq_set().push_back(pentry);

    CRef<CSeq_entry> set_entry(new CSeq_entry());
    set_entry->SetSet(*set);

    CRef<CSeq_feat> cds = MakeCDSForNucProtSet("nuc", "prot");
    AddFeat (cds, set_entry);

    return set_entry;
}


// tests

BOOST_AUTO_TEST_CASE(Test_SimpleAutodef)
{
    // prepare entry
    CRef<CSeq_entry> entry = BuildSequence();
    AddSource (entry, "Homo sapiens");
    AddTitle(entry, "Homo sapiens.");

    CheckDeflineMatches(entry);
}


BOOST_AUTO_TEST_CASE(Test_UnnamedPlasmid)
{
    // prepare entry
    CRef<CSeq_entry> entry = BuildSequence();
    CRef<CSeqdesc> desc = AddSource (entry, "Alcanivorax sp. HA03");
    desc->SetSource().SetGenome(CBioSource::eGenome_plasmid);
    CRef<CSubSource> sub(new CSubSource("plasmid-name", "unnamed"));
    desc->SetSource().SetSubtype().push_back(sub);
    AddTitle(entry, "Alcanivorax sp. HA03 plasmid.");

    CheckDeflineMatches(entry);
}


BOOST_AUTO_TEST_CASE(Test_SQD_476)
{
    CRef<CSeq_entry> entry = BuildNucProtSet("chlorocatechol 1,2-dioxygenase");
    CRef<CSeqdesc> desc = AddSource (entry, "Alcanivorax sp. HA03");
    desc->SetSource().SetGenome(CBioSource::eGenome_plasmid);
    CRef<CSubSource> sub(new CSubSource("plasmid-name", "unnamed"));
    desc->SetSource().SetSubtype().push_back(sub);
    AddTitle(entry, "Alcanivorax sp. HA03 plasmid chlorocatechol 1,2-dioxygenase gene, complete cds.");

    CheckDeflineMatches(entry);
}


BOOST_AUTO_TEST_CASE(Test_SQD_630)
{
    CRef<CSeq_entry> entry = BuildSequence();
    CRef<CSeqdesc> desc = AddSource (entry, "Clathrina aurea");
    CRef<CSubSource> sub(new CSubSource("clone", "Cau_E6"));
    desc->SetSource().SetSubtype().push_back(sub);
    CRef<CSeq_feat> feat(new CSeq_feat());
    feat->SetData().SetImp().SetKey("repeat_region");
    CRef<CGb_qual> qual(new CGb_qual("satellite", "microsatellite"));
    feat->SetQual().push_back(qual);
    AddFeat(feat, entry);

    AddTitle(entry, "Clathrina aurea microsatellite sequence.");

    CheckDeflineMatches(entry);

    feat->SetComment("dinucleotide");
    CheckDeflineMatches(entry);
}


BOOST_AUTO_TEST_CASE(Test_SQD_169)
{
    CRef<CSeq_entry> entry = BuildSequence();
    CRef<CSeqdesc> desc = AddSource (entry, "Clathrina aurea");
    CRef<CSeq_feat> feat(new CSeq_feat());
    feat->SetData().SetImp().SetKey("misc_feature");
    feat->SetComment("contains 5S ribosomal RNA and nontranscribed spacer");
    AddFeat(feat, entry);

    AddTitle(entry, "Clathrina aurea 5S ribosomal RNA gene region.");

    CheckDeflineMatches(entry);
}


END_SCOPE(objects)
END_NCBI_SCOPE
