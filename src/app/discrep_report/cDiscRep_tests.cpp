/*  $Id: cDiscRep_tests.cpp 389991 2013-02-22 14:31:43Z gouriano $
 * =========================================================================
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
 * =========================================================================
 *
 * Author:  Jie Chen
 *
 * File Description:
 *   Tests and report data collection for Cpp Discrepany Report
 *
 * $Log:$ 
 */

#include <ncbi_pch.hpp>
#include <serial/enumvalues.hpp>
#include <objtools/format/items/source_item.hpp>
#include <objtools/format/items/feature_item.hpp>
#include <objects/submit/Contact_info.hpp>
#include <objects/seq/Seq_ext.hpp>
#include <objects/seq/Seg_ext.hpp>
#include <objects/seq/seqport_util.hpp>
#include <objects/seq/Seq_data.hpp>
#include <objects/seq/Seqdesc.hpp>
#include <objects/seq/Pubdesc.hpp>
#include <objects/seq/Seq_ext.hpp>
#include <objects/seq/Delta_ext.hpp>
#include <objects/seq/Delta_seq.hpp>
#include <objects/seq/Seq_literal.hpp>
#include <objects/seq/MolInfo.hpp>
#include <objects/seqfeat/Cdregion.hpp>
#include <objects/seqfeat/Code_break.hpp>
#include <objects/seqfeat/Feat_id.hpp>
#include <objects/seqfeat/PCRReaction.hpp>
#include <objects/seqfeat/PCRPrimer.hpp>
#include <objects/seqfeat/PCRReactionSet.hpp>
#include <objects/seqfeat/PCRPrimerSet.hpp>
#include <objects/seqfeat/RNA_gen.hpp>
#include <objects/seqfeat/RNA_qual.hpp>
#include <objects/seqfeat/RNA_qual_set.hpp>
#include <objects/seqfeat/RNA_ref.hpp>
#include <objects/seqfeat/SeqFeatXref.hpp>
#include <objects/seqblock/GB_block.hpp>
#include <objects/macro/Feat_qual_legal_.hpp>
#include <objects/macro/Feature_field.hpp>
#include <objects/macro/Search_func.hpp>
#include <objects/macro/Constraint_choice_set.hpp>
#include <objects/macro/Field_type.hpp>
#include <objects/general/Name_std.hpp>
#include <objects/general/Date.hpp>
#include <objects/general/Date_std.hpp>
#include <objects/general/Dbtag.hpp>
#include <objects/general/User_object.hpp>
#include <objects/general/User_field.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/biblio/Affil.hpp>
#include <objects/biblio/Author.hpp>
#include <objects/biblio/Cit_gen.hpp>
#include <objects/biblio/Cit_sub.hpp>
#include <objects/biblio/Cit_art.hpp>
#include <objects/biblio/Cit_book.hpp>
#include <objects/biblio/Cit_proc.hpp>
#include <objects/biblio/Cit_let.hpp>
#include <objects/biblio/Cit_pat.hpp>
#include <objects/biblio/Cit_jour.hpp>
#include <objects/biblio/citation_base.hpp>
#include <objects/biblio/Title.hpp>
#include <objects/biblio/Imprint.hpp>
#include <objects/pub/Pub_equiv.hpp>
#include <objmgr/annot_selector.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/util/sequence.hpp>
#include <objmgr/util/seq_loc_util.hpp>
#include <objmgr/util/feature.hpp>
#include <objmgr/seq_vector_ci.hpp>
#include <objmgr/seq_vector.hpp>
#include <objmgr/seq_map.hpp>
#include <objmgr/seq_feat_handle.hpp>
#include <util/xregexp/regexp.hpp>

#include <algo/blast/core/ncbi_std.h>

#include "hDiscRep_app.hpp"
#include "hDiscRep_config.hpp"
#include "hDiscRep_tests.hpp"
#include "hchecking_class.hpp"

#include "hUtilib.hpp"

USING_NCBI_SCOPE;
using namespace objects;
using namespace sequence;
using namespace DiscRepNmSpc;
using namespace feature;
using namespace validator;

vector <bool>   CRuleProperties :: srch_func_empty;
vector <bool>   CRuleProperties :: except_empty;

static CDiscRepInfo thisInfo;
static CRef <CRuleProperties> rule_prop;
static string strtmp(kEmptyStr);
static CDiscTestInfo thisTest;

// ini. CDiscTestInfo
bool CDiscTestInfo :: is_Aa_run;
bool CDiscTestInfo :: is_AllAnnot_run;
bool CDiscTestInfo :: is_BacPartial_run;
bool CDiscTestInfo :: is_BASES_N_run;
bool CDiscTestInfo :: is_BioSet_run;
bool CDiscTestInfo :: is_BIOSRC_run;
bool CDiscTestInfo :: is_BIOSRC1_run;
bool CDiscTestInfo :: is_Biosrc_Orgmod_run;
bool CDiscTestInfo :: is_CDs_run;
bool CDiscTestInfo :: is_CdTransl_run;
bool CDiscTestInfo :: is_Comment_run;
bool CDiscTestInfo :: is_Defl_run;
bool CDiscTestInfo :: is_DESC_user_run;
bool CDiscTestInfo :: is_GP_Set_run;
bool CDiscTestInfo :: is_IncnstUser_run;
bool CDiscTestInfo :: is_MolInfo_run;
bool CDiscTestInfo :: is_MRNA_run;  // need modification, has to be is_mRNACDs_run
bool CDiscTestInfo :: is_mRNA_run;
bool CDiscTestInfo :: is_Prot_run;
bool CDiscTestInfo :: is_ProtFeat_run;
bool CDiscTestInfo :: is_Pub_run;
bool CDiscTestInfo :: is_Quals_run;
bool CDiscTestInfo :: is_TRRna_run;
bool CDiscTestInfo :: is_RRna_run;
bool CDiscTestInfo :: is_Subsrc_run;
bool CDiscTestInfo :: is_SusPhrase_run;
bool CDiscTestInfo :: is_TaxCflts_run;
bool CDiscTestInfo :: is_TaxDef_run;

typedef map <string, CRef < GeneralDiscSubDt > > Str2SubDt;
typedef map <int, vector <string> > Int2Strs;

static Str2SubDt GENE_PRODUCT_CONFLICT_cds;
static set <string> DUP_LOCUS_TAGS_adj_genes;
static vector <CRef < GeneralDiscSubDt > > INCONSISTENT_BIOSOURCE_biosrc;
static vector < CRef < CClickableItem > > DUPLICATE_GENE_LOCUS_locuss;
static vector < CRef < GeneralDiscSubDt > > SUSPECT_PROD_NAMES_feat;
static unsigned DISC_QUALITY_SCORES_graph=0;
static Int2Strs PROJECT_ID_prot;
static Int2Strs PROJECT_ID_nuc;
static vector <CRef < GeneralDiscSubDt > > DISC_FEATURE_COUNT_list;
static vector <string> DISC_FEATURE_COUNT_protfeat_prot_list;
static vector <string> DISC_FEATURE_COUNT_protfeat_nul_list;
static Str2Strs JOINED_FEATURES_sfs;

static Str2MapStr2Strs biosrc2qualvlu_nm;
Str2QualVlus qual_nm2qual_vlus;

// CRuleProperties
bool CRuleProperties :: IsSearchFuncEmpty (const CSearch_func& func)
{
  if (func.IsString_constraint()) {
     const CString_constraint& str_cons = func.GetString_constraint();
     if (str_cons.GetIs_all_lower() || str_cons.GetIs_all_lower() || str_cons.GetIs_all_punct())
           return false;
     else if (!(str_cons.CanGetMatch_text()) || str_cons.GetMatch_text().empty()) return true;
     else return false;
  }
  else if (func.IsPrefix_and_numbers()) {
      if (func.GetPrefix_and_numbers().empty()) return true;
      else return false;
  }
  else return false;
};


// CBioseq
void CBioseq_DISC_GAPS :: TestOnObj(const CBioseq& bioseq)
{
   string desc(GetDiscItemText(bioseq));
   if (bioseq.GetInst().GetRepr() == CSeq_inst::eRepr_delta) {
      const CSeq_inst& inst = bioseq.GetInst();
      if (inst.CanGetExt() && inst.GetExt().IsDelta()) {
        ITERATE (list <CRef <CDelta_seq> >, it, inst.GetExt().GetDelta().Get()) {
          if ( (*it)->IsLiteral() 
                 && (!(*it)->GetLiteral().CanGetSeq_data() 
                            || (*it)->GetLiteral().GetSeq_data().IsGap())) {
             thisInfo.test_item_list[GetName()].push_back(desc);
             return;
          } 
        }
      }
   }

   if (!gap_feat.empty()) thisInfo.test_item_list[GetName()].push_back(desc);
};

void CBioseq_DISC_GAPS :: GetReport (CRef <CClickableItem>& c_item)
{
   c_item->description 
         = GetContainsComment(c_item->item_list.size(), "sequence") + "gaps";
};

static const CEnumeratedTypeValues* molinfo_tech = CMolInfo::GetTypeInfo_enum_ETech();
void CBioseq_DISC_INCONSISTENT_MOLINFO_TECH :: TestOnObj(const CBioseq& bioseq)
{
  ITERATE (vector <const CSeqdesc*>, it, bioseq_molinfo) {
    strtmp =  
       molinfo_tech->FindName((CMolInfo::ETech)(*it)->GetMolinfo().GetTech(), false)+"$";
    thisInfo.test_item_list[GetName()].push_back(strtmp + GetDiscItemText(bioseq));
  }
};

void CBioseq_DISC_INCONSISTENT_MOLINFO_TECH :: GetReport(CRef <CClickableItem>& c_item)
{
  Str2Strs tech2seqs;
  GetTestItemList(c_item->item_list, tech2seqs);
  c_item->item_list.clear();
  bool has_missing = false;
  if (tech2seqs.find("unknown") != tech2seqs.end()) {
    has_missing = true;
    AddSubcategories(c_item, GetName(), tech2seqs["unknown"], "Molinfo", 
                                                      "missing field technique");
  }
  unsigned cnt = tech2seqs.size();
  if (cnt == 1) {
     if (has_missing) strtmp = "(all missing)";
     else {
        Str2Strs::iterator it = tech2seqs.begin();
        AddSubcategories(c_item, GetName(), it->second, "All Molinfo",
              "field technique value '" + it->first, e_HasComment);
        strtmp = "(all present, all same)";
     }
  }
  else if (cnt == 2 && has_missing) {
      strtmp = "(some missing, all same)";
      ITERATE (Str2Strs, it, tech2seqs) {
         if (it->first != "unknowns")
             AddSubcategories(c_item, GetName(), it->second, "Molinfo", 
               "field technique value '" + it->first, e_HasComment);
      }
  }
  else {
    strtmp = "(all present, inconsistent)";
    ITERATE (Str2Strs, it, tech2seqs) {
       AddSubcategories(c_item, GetName(), it->second, "Molinfo",
               "field technique value '" + it->first, e_HasComment); 
    }
  }

  c_item->description = "Molinfo Technique Report " + strtmp;
};


void CBioseq_TEST_MRNA_OVERLAPPING_PSEUDO_GENE :: TestOnObj(const CBioseq& bioseq)
{
  ITERATE (vector <const CSeq_feat*>, it, mrna_feat) {
     CConstRef <CSeq_feat> gene = GetGeneForFeature(**it);
     if (gene.NotEmpty() && gene->CanGetPseudo() && gene->GetPseudo())
        thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));  
  }
};

void CBioseq_TEST_MRNA_OVERLAPPING_PSEUDO_GENE :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
     = GetHasComment(c_item->item_list.size(), "Pseudogene") + "overlapping mRNAs.";
};


void CBioseq_TEST_UNWANTED_SPACER :: TestOnObj(const CBioseq& bioseq)
{
   ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
     const CBioSource& biosrc = (*it)->GetSource();
     switch ( biosrc.GetGenome() ) {
        case CBioSource::eGenome_chloroplast:
        case CBioSource::eGenome_plastid:
            return;
        default: break; 
     }
     /* shouldn't be uncultured non-organelle */
     if (biosrc.IsSetTaxname() && !biosrc.GetTaxname().empty() 
             && HasUnculturedNonOrganelleName(biosrc.GetTaxname())) return;
   }

   /* look for misc_features */
   ITERATE (vector <const CSeq_feat*>, it, miscfeat_feat) {
     if ( (*it)->CanGetComment() && !(*it)->GetComment().empty()
             && HasIntergenicSpacerName( (*it)->GetComment()))
        thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
   }
};


bool CBioseqTestAndRepData :: HasUnculturedNonOrganelleName (const string& tax_nm)
{
  if (tax_nm == "uncultured organism" || tax_nm == "uncultured microorganism"
           || tax_nm == "uncultured bacterium" || tax_nm == "uncultured archaeon")
    return true;
  else return false;
};


bool CBioseq_TEST_UNWANTED_SPACER :: HasIntergenicSpacerName (const string& comm)
{
  ITERATE (vector <string>, it, thisInfo.kIntergenicSpacerNames) {
     if (NStr::FindNoCase(comm, *it) != string::npos) return true;
  }
  return false;
};


void CBioseq_TEST_UNWANTED_SPACER :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description
    = GetIsComment(c_item->item_list.size(),"suspect intergenic spacer note") +"not organelle";
};


// new comb 
void CBioseq_on_Aa :: TestOnObj(const CBioseq& bioseq) 
{
   if (thisTest.is_Aa_run) return;

   string bioseq_desc(GetDiscItemText(bioseq));
   unsigned len = bioseq.GetLength();
   CConstRef <CBioseq_set> bioseq_set = bioseq.GetParentSet();
   if (bioseq.IsAa()) {
      // SHORT_PROT_SEQUENCES
      if (len >= 50) return;
      if (bioseq_set.Empty() || bioseq_set->GetClass() != CBioseq_set::eClass_parts) { 
         ITERATE (vector <const CSeqdesc*>, it, bioseq_molinfo) {
            if ( (*it)->GetMolinfo().GetCompleteness()
                                    != CMolInfo::eCompleteness_complete) continue;
            else {
               thisInfo.test_item_list[GetName_shtprt()].push_back(bioseq_desc);
               break;
            }
         }
      }       
      return;
   }
 
    bool is_dna = (bioseq.GetInst().GetMol() == CSeq_inst::eMol_dna);
    bool is_rna = (bioseq.GetInst().GetMol() == CSeq_inst::eMol_rna);

    if (is_dna) {

      // DISC_RETROVIRIDAE_DNA, NON_RETROVIRIDAE_PROVIRAL
      bool is_retro;
      string src_desc;
      ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
         const CBioSource& biosrc = (*it)->GetSource();
         const int genome = biosrc.GetGenome();
         src_desc = GetDiscItemText(**it, bioseq);
         is_retro = HasLineage(biosrc, "Retroviridae");
         if ( genome != (int)CBioSource :: eGenome_proviral && is_retro)
            thisInfo.test_item_list[GetName_retro()].push_back(src_desc);
         else if (genome == CBioSource :: eGenome_proviral && !is_retro)
            thisInfo.test_item_list[GetName_nonr()].push_back(src_desc);
      }
   }
   else {
      // EUKARYOTE_SHOULD_HAVE_MRNA
      bool has_CDs = false, has_mrna = false;
      if (m_check_eu_mrna) {
         if (IsBioseqHasLineage(bioseq, "Eukaryota")) {
            ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
              if (!IsLocationOrganelle((*it)->GetSource().GetGenome())) {
                 ITERATE (vector <const CSeqdesc*>, jt, bioseq_molinfo) {
                    if ( (*it)->GetMolinfo().GetBiomol() == CMolInfo::eBiomol_genomic) {
                       ITERATE (vector <const CSeq_feat*>, kt, cd_feat) {
                         if (IsPseudoSeqFeatOrXrefGene(*kt)) continue;
                         has_CDs = true;
                         CConstRef <CSeq_feat> mRNA = GetBestMrnaForCds(**kt, *thisInfo.scope, 
                                                                       fBestFeat_NoExpensive);
                         if (mRNA.NotEmpty()) { 
                            thisInfo.test_item_list[GetName_eu_mrna()].push_back("yes");
                            m_check_eu_mrna = false;
                            has_mrna = true;
                            break;
                         }
                       }
                    }
                    if (has_mrna) break;
                 }
              }
              if (has_mrna) break; 
            }
            if (has_CDs && !has_mrna)
               thisInfo.test_item_list[GetName_eu_mrna()].push_back("no");
         }
      }

      if (is_rna) {  // RNA_PROVIRAL
        ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
           if ( (*it)->GetSource().GetGenome() == CBioSource :: eGenome_proviral ) {
               thisInfo.test_item_list[GetName_rnapro()].push_back(bioseq_desc);
               break;
           }
        }
      }
   }
       
   // TEST_ORGANELLE_NOT_GENOMIC
   bool run_test = false;
   ITERATE (vector <const CSeqdesc*>, it, bioseq_molinfo) {
     int biomol = (*it)->GetMolinfo().GetBiomol();
     if ( ( biomol == CMolInfo::eBiomol_genomic || biomol == CMolInfo::eBiomol_unknown)
            && is_dna) {
          run_test = true;
          break;
     }
   }
   if (run_test) {  // RNA_PROVIRAL
     ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
       if (IsLocationOrganelle( (*it)->GetSource().GetGenome() ))
           thisInfo.test_item_list[GetName_orgl()].push_back(GetDiscItemText(**it,bioseq));
     }
   }

   // TEST_UNNECESSARY_VIRUS_GENE
   bool has_virus = false;
   ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
      const CBioSource& biosrc = (*it)->GetSource();
      ITERATE (vector <string>, jt, thisInfo.virus_lineage) {
         if (HasLineage(biosrc, *jt)) {
             ITERATE (vector <const CSeq_feat*>, kt, gene_feat)
                 thisInfo.test_item_list[GetName_vir()].push_back(GetDiscItemText(**kt));
             has_virus = true;
             break;
         }
      }
      if (has_virus) break;
   }

   // DISC_RBS_WITHOUT_GENE
   if (!gene_feat.empty()) {
       ITERATE (vector <const CSeq_feat*>, it, rbs_feat) {
         CConstRef <CSeq_feat>
             rbs_gene = GetBestOverlappingFeat((*it)->GetLocation(),
                                      CSeqFeatData::e_Gene, eOverlap_Contained,
                                      *thisInfo.scope);
         if (rbs_gene.NotEmpty())
             thisInfo.test_item_list[GetName_rbs()].push_back(GetDiscItemText(**it));
       }
   }

   // DISC_EXON_INTRON_CONFLICT
   string seq_id_desc(BioseqToBestSeqIdString(bioseq, CSeq_id::e_Genbank)+"$");
   unsigned i=0;
   if (gene_feat.empty()) {
       /* no genes - just do all exons and introns present */
       if (!exon_feat.empty() && !intron_feat.empty()) {
            for (i=0; i< exon_feat.size(); i++) m_e_exist.push_back(1);
            for (i=0; i< intron_feat.size(); i++) m_i_exist.push_back(1);
            CompareIntronExonList(seq_id_desc, exon_feat, intron_feat);
       }
   }
   else {
      ITERATE (vector <const CSeq_feat*>, it, gene_feat) {
         if ((*it)->CanGetExcept_text() && (*it)->GetExcept_text() != "trans-splicing"){
            GetFeatureList4Gene( *it, exon_feat, m_e_exist);
            GetFeatureList4Gene( *it, intron_feat, m_i_exist);
            if (!exon_feat.empty() && !intron_feat.empty())
              CompareIntronExonList(seq_id_desc, exon_feat, intron_feat);
         }
      }
   }
   
   // DISC_GENE_PARTIAL_CONFLICT
   // ReportPartialConflictsForFeatureType (bsp, 0, FEATDEF_intron, "intron")
   ReportPartialConflictsForFeatureType (intron_feat, (string)"intron");
   ReportPartialConflictsForFeatureType (exon_feat, (string)"exon");
   ReportPartialConflictsForFeatureType (promoter_feat, (string)"promoter");
   ReportPartialConflictsForFeatureType (rna_feat, (string)"RNA");
   ReportPartialConflictsForFeatureType (utr3_feat, (string)"3' URT");
   ReportPartialConflictsForFeatureType (utr5_feat, (string)"5' URT");
   if (!IsBioseqHasLineage(bioseq, "Eukaryota") || IsMrnaSequence ())
       ReportPartialConflictsForFeatureType (cd_feat, (string)"coding region", true);
   ReportPartialConflictsForFeatureType (miscfeat_feat, (string)"misc_feature");

   if (!IsmRNASequenceInGenProdSet(bioseq)) {
      // SHORT_CONTIG
      if (len < 200) thisInfo.test_item_list[GetName_contig()].push_back(bioseq_desc);

      // SHORT_SEQUENCES
      if (bioseq_set.Empty() || bioseq_set->GetClass() != CBioseq_set::eClass_parts) {
          if (len < 200) 
              thisInfo.test_item_list[GetName_200seq()].push_back(bioseq_desc);
          else if (len < 50) 
              thisInfo.test_item_list[GetName_50seq()].push_back(bioseq_desc);
      }
   }

   // TEST_DUP_GENES_OPPOSITE_STRANDS
   vector <const CSeq_feat*>::const_iterator jt;
   bool prev_dup = false;
   ITERATE (vector <const CSeq_feat*>, it, gene_feat) {
      jt = it + 1;
      if (jt != gene_feat.end()) {
         sequence::ECompare
           ovp = Compare( (*it)->GetLocation(), (*jt)->GetLocation(), thisInfo.scope);
         if (ovp == sequence::eSame
                && (*it)->GetLocation().GetStrand() != (*jt)->GetLocation().GetStrand()){
            thisInfo.test_item_list[GetName_dupg()].push_back(GetDiscItemText(**jt));
            if (!prev_dup)
               thisInfo.test_item_list[GetName_dupg()].push_back(GetDiscItemText(**it));
            prev_dup = true;
         }
         else prev_dup = false;
      }
   }

   // TEST_COUNT_UNVERIFIED
   if (BioseqHasKeyword (bioseq, "UNVERIFIED"))
      thisInfo.test_item_list[GetName_unv()].push_back(bioseq_desc);

   thisTest.is_Aa_run = true;
};

void CBioseq_EUKARYOTE_SHOULD_HAVE_MRNA :: GetReport(CRef <CClickableItem>& c_item)
{
  strtmp = NStr::Join(c_item->item_list, ",");
  c_item->item_list.clear();
  if (strtmp.find("yes") == string::npos) 
     c_item->description = "No mRNA present.";
};

void CBioseq_RNA_PROVIRAL :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
     = GetIsComment(c_item->item_list.size(), "RNA bioseq") + "proviral";
};

void CBioseq_DISC_RETROVIRIDAE_DNA :: GetReport(CRef <CClickableItem>& c_item)
{   
   c_item->description
     = GetOtherComment(c_item->item_list.size(), "Retroviridae biosource on DNA sequences is",
                          "Retroviridae biosourceson DNA sequences are") + " not proviral";
};

void CBioseq_NON_RETROVIRIDAE_PROVIRAL :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = 
       GetIsComment(c_item->item_list.size(), "non-Retroviridae biosource") + "proviral.";
};

void CBioseq_TEST_COUNT_UNVERIFIED :: GetReport(CRef <CClickableItem>& c_item) 
{
  c_item->description = GetIsComment(c_item->item_list.size(), "sequence") + "unverified.";
};

bool CBioseqTestAndRepData :: BioseqHasKeyword(const CBioseq& bioseq, const string& keywd)
{
  if (NStr::EqualNocase(keywd, "UNVERIFIED"))
  {
    /* special case for unverified */
    ITERATE (vector <const CSeqdesc*>, it, bioseq_user) {
       const CUser_object& user_obj = (*it)->GetUser();
       if (user_obj.GetType().IsStr() 
             && NStr::EqualNocase(user_obj.GetType().GetStr(), "Unverified"))
          return true;
    }

  }

  ITERATE (vector <const CSeqdesc*>, it, bioseq_genbank) {
     const CGB_block& gb = (*it)->GetGenbank();
     if (gb.CanGetKeywords()) {
        ITERATE (list <string>, jt, gb.GetKeywords()) {
           if (NStr::EqualNocase(*jt, keywd)) return true;
        }
     }
  }

  return false;
};


void CBioseq_TEST_DUP_GENES_OPPOSITE_STRANDS :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
      = GetOtherComment(c_item->item_list.size(), "gene matches", "genes match")
         + " other genes in the same location, but on the opposite strand";
};

void CBioseq_SHORT_CONTIG :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
       = GetIsComment(c_item->item_list.size(), "contig") + "shorter than 200 nt.";
};


void CBioseq_SHORT_SEQUENCES :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description
    = GetIsComment(c_item->item_list.size(), "sequence") + "shorter than 50 nt.";
};


void CBioseq_SHORT_SEQUENCES_200 :: GetReport(CRef <CClickableItem>& c_item) 
{ 
   c_item->description 
       = GetIsComment(c_item->item_list.size(), "sequence") + "shorter than 200 nt."; 
}; 


void CBioseq_SHORT_PROT_SEQUENCES :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
       = GetIsComment(c_item->item_list.size(), "protein sequence") + "shorter than 50 aa.";
};


void CBioseq_DISC_GENE_PARTIAL_CONFLICT :: GetReport(CRef <CClickableItem>& c_item)
{
  Str2Strs label2lists;
  GetTestItemList(c_item->item_list, label2lists);
  c_item->item_list.clear();
  bool has_other = false;
  ITERATE (Str2Strs, it, label2lists) {
    strtmp = kEmptyStr;
    if (it->first.find("coding region") != string::npos) strtmp = "coding region";
    else if (it->first.find("misc_feature") != string::npos) strtmp = "misc_feature";
    if (!strtmp.empty()) {
        AddSubcategories(c_item, GetName(), it->second, strtmp + " location conflicts ",
                   strtmp + " locations conflict ", e_OtherComment, true,
                   "with partialness of overlapping gene.", true);
    }
    else {
      if (!has_other) {
        CRef <CClickableItem> citem_other (new CClickableItem);
        ITERATE (Str2Strs, jt, label2lists) {
           if (jt->first.find("coding region") == string::npos
                && jt->first.find("misc_feature") == string::npos)
              AddSubcategories(citem_other, GetName(), jt->second, jt->first, jt->first,
                       e_OtherComment, true, "", true);
        }
        AddSubcategories(c_item, GetName(), citem_other->item_list,
              "feature that is not coding regions or misc_feature conflicts",
              "features that are not coding regions or misc_features conflict", e_OtherComment,
              true, " with partialness of overlapping gene.", true);
        has_other = true;
      }
    }
  }
  c_item->description
       = GetOtherComment(c_item->item_list.size()/2,
                        "feature location conflicts", "feature locations conflict")
          + " with partialness of overlapping gene.";
};


void CBioseq_DISC_EXON_INTRON_CONFLICT :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs seq2cflts;
   GetTestItemList(c_item->item_list, seq2cflts);
   c_item->item_list.clear();
   ITERATE (Str2Strs, it, seq2cflts) {
     AddSubcategories(c_item, GetName(), it->second, "introns and exon",
                      "location conflicts on " + it->first, e_HasComment);
   }
   c_item->description = GetIsComment(c_item->item_list.size(), "introns and exon")
                          + "incorrectly positioned";
};


void CBioseq_DISC_RBS_WITHOUT_GENE :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
      = GetDoesComment(c_item->item_list.size(), "RBS feature") + "not have overlapping genes";
};


void CBioseq_TEST_UNNECESSARY_VIRUS_GENE :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
      = GetOtherComment(c_item->item_list.size(), "virus gene needs", "virus genes need") 
        + " to be removed";
};


void CBioseq_TEST_ORGANELLE_NOT_GENOMIC :: GetReport(CRef <CClickableItem>& c_item)
{
   unsigned cnt = c_item->item_list.size();
   c_item->description 
         = GetIsComment(cnt, "non-genomic sequence") + GetNoun(cnt, "organelle");
};


// new comb
void CBioseq_on_mRNA :: TestOnObj(const CBioseq& bioseq)
{
  if (thisTest.is_mRNA_run) return;
  if (!IsMrnaSequence()) return;

  string desc(GetDiscItemText(bioseq));
  // TEST_EXON_ON_MRNA
  if (!exon_feat.empty()) 
         thisInfo.test_item_list[GetName_exon()].push_back(desc);

  // TEST_BAD_MRNA_QUAL
  ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
    const CBioSource& biosrc = (*it)->GetSource();
    if ( biosrc.CanGetSubtype()) {
      if (IsSubSrcPresent(biosrc, CSubSource::eSubtype_germline) 
               || IsSubSrcPresent(biosrc, CSubSource::eSubtype_rearranged))
        thisInfo.test_item_list[GetName_qual()].push_back(GetDiscItemText(**it, bioseq));
    }
  }

  // TEST_MRNA_SEQUENCE_MINUS_ST
  ITERATE (vector <const CSeq_feat*>, it, all_feat) {
     if ( (*it)->GetData().GetSubtype() != CSeqFeatData::eSubtype_primer_bind
             && (*it)->GetLocation().GetStrand() == eNa_strand_minus)
        thisInfo.test_item_list[GetName_str()].push_back(desc);
  }

  // MULTIPLE_CDS_ON_MRNA
  strtmp = "coding region disrupted by sequencing gap";
  unsigned cnt=0;
  ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
     if ( (!(*it)->CanGetPseudo() || !(*it)->GetPseudo())
            && ( !(*it)->CanGetComment() || (*it)->GetComment().find(strtmp) == string::npos)){
        if (cnt) thisInfo.test_item_list[GetName_mcds()].push_back(desc);
        else cnt++;
     }
  }

  thisTest.is_mRNA_run = true;
};

void CBioseq_MULTIPLE_CDS_ON_MRNA :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetHasComment(c_item->item_list.size(), "mRNA bioseq")
                           + "multiple CDS features";
};

void CBioseq_TEST_MRNA_SEQUENCE_MINUS_ST :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description
    = GetHasComment(c_item->item_list.size(), "mRNA sequence") 
       + "features on the complement strand.";
};


void CBioseq_TEST_BAD_MRNA_QUAL :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetHasComment(c_item->item_list.size(), "mRNA sequence") 
                   + "germline or rearranged qualifier";
};


void CBioseq_TEST_EXON_ON_MRNA :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
       = GetHasComment(c_item->item_list.size(), "mRNA bioseq") + "exon features";
};


void CBioseq_ONCALLER_HIV_RNA_INCONSISTENT :: TestOnObj(const CBioseq& bioseq)
{
   if (bioseq.GetInst().GetMol() != CSeq_inst::eMol_rna) return;
   ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
     const CBioSource& biosrc = (*it)->GetSource();
     if ( biosrc.GetGenome() == CBioSource::eGenome_unknown) return;
     if ( biosrc.IsSetTaxname() || (strtmp = biosrc.GetTaxname()).empty()) return;
     if (!NStr::EqualNocase(strtmp, "Human immunodeficiency virus")
             && (!NStr::EqualNocase(strtmp, "Human immunodeficiency virus"))
             && (!NStr::EqualNocase(strtmp, "Human immunodeficiency virus"))) 
         return;
     if ( biosrc.GetGenome() == CBioSource::eGenome_genomic ){
        ITERATE (vector <const CSeqdesc*>, jt, bioseq_molinfo)
           if ( (*jt)->GetMolinfo().GetBiomol() != CMolInfo::eBiomol_genomic) return;
     }
   }

   thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(bioseq));
};

void CBioseq_ONCALLER_HIV_RNA_INCONSISTENT :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "HIV RNA bioseq") 
                            + "inconsistent location/moltype";
};


// new comb
void CBioseq_on_cd_feat :: TestOnObj(const CBioseq& bisoeq)
{
  if (thisTest.is_CDs_run) return;

  string desc;
  ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
    desc = GetDiscItemText(**it);

    // DISC_CDS_HAS_NEW_EXCEPTION
    if ( (*it)->CanGetExcept_text()) {
       ITERATE (vector <string>, jt, thisInfo.new_exceptions) {
          if (NStr::FindNoCase( (*it)->GetExcept_text(), *jt ) != string::npos) {
            thisInfo.test_item_list[GetName()].push_back(desc);
            break;
          }
       }
    }

    // TEST_CDS_HAS_CDD_XREF
    if ((*it)->CanGetDbxref()) {
       ITERATE (vector < CRef< CDbtag > >, jt, (*it)->GetDbxref()) {
         if ( NStr::EqualNocase( (*jt)->GetDb(), "CDD")) {
            thisInfo.test_item_list[GetName_cdd()].push_back(desc);
            break;
         }
       }
    }
  }
 
  thisTest.is_CDs_run = true;
};


void CBioseq_TEST_CDS_HAS_CDD_XREF :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "feature") + "CDD Xrefs";

};


void CBioseq_DISC_CDS_HAS_NEW_EXCEPTION :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
     = GetHasComment(c_item->item_list.size(), "coding region") + "new exceptions";
};


void CBioseq_DISC_MITOCHONDRION_REQUIRED :: TestOnObj(const CBioseq& bioseq)
{
   bool needs_mitochondrial = false;
   string comm;
   if (D_loop_feat.empty()) {
      ITERATE (vector <const CSeq_feat*>, it, miscfeat_feat) {
         if ((*it)->CanGetComment() && !( (comm = (*it)->GetComment()).empty())
                 && NStr::FindNoCase(comm, "control region") != string::npos) {
            needs_mitochondrial = true; break;
         }
      }
   }
   else needs_mitochondrial = true; 
  
   if (needs_mitochondrial) {
     string desc = GetDiscItemText(bioseq);
     if (bioseq_biosrc_seqdesc.empty()) thisInfo.test_item_list[GetName()].push_back(desc);
     ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
         if ( (*it)->GetSource().GetGenome() != CBioSource::eGenome_mitochondrion) {
             thisInfo.test_item_list[GetName()].push_back(desc); break;
         }
     }
   }
};


void CBioseq_DISC_MITOCHONDRION_REQUIRED :: GetReport(CRef <CClickableItem>& c_item)
{
   unsigned cnt = c_item->item_list.size();
   c_item->description
     = GetHasComment(cnt, "bioseq") + "D-loop or control region misc_feature, but "
        + ( (cnt ==1)? "does" : "do" ) + " not have mitochondrial source";
};


void CBioseq_DISC_MICROSATELLITE_REPEAT_TYPE :: TestOnObj(const CBioseq& bioseq)
{
   bool is_microsatellite = false, is_tandem = false;
   string qual_qual, qual_val;
   ITERATE (vector <const CSeq_feat*>, it, repeat_region_feat) {
     if ( (*it)->CanGetQual()) {
        ITERATE (vector <CRef <CGb_qual> >, jt, (*it)->GetQual()) {
          qual_qual = (*jt)->GetQual();
          qual_val = (*jt)->GetVal(); 
          if ( qual_qual == "satellite"
                && (NStr::EqualNocase(qual_val, "microsatellite") //please check all FindNoCase
                     || NStr::EqualNocase( qual_val, 0, 15, "microsatellite:")) ) {
              is_microsatellite = true;
          }
          else if ( qual_qual == "rpt_type" && qual_val == "tandem") is_tandem = true;
          if (is_microsatellite && is_tandem) break;
        }
     }
     if (is_microsatellite && !is_tandem)
        thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
   }
};


void CBioseq_DISC_MICROSATELLITE_REPEAT_TYPE :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetDoesComment(c_item->item_list.size(), "microsatellite") 
                             + "not have a repeat type of tandem";
};



//new comb
void CBioseq_test_on_suspect_phrase :: TestOnObj(const CBioseq& bioseq) 
{
   if (thisTest.is_SusPhrase_run) return;

   // DISC_CHECK_RNA_PRODUCTS_AND_COMMENTS
   ITERATE (vector <const CSeq_feat*>, it, rrna_feat) CheckForProdAndComment(**it);
   ITERATE (vector <const CSeq_feat*>, it, trna_feat) CheckForProdAndComment(**it);

   // DISC_SUSPECT_MISC_FEATURES
   ITERATE (vector <const CSeq_feat*>, it, miscfeat_feat) FindBadMiscFeatures(**it);

   thisTest.is_SusPhrase_run = true;
}


void CBioseq_test_on_suspect_phrase :: FindBadMiscFeatures(const CSeq_feat& seq_feat)
{
  string bad_misc_comment_phrases("catalytic intron");
  if (seq_feat.CanGetComment() 
      && DoesStringContainPhrase(seq_feat.GetComment(), bad_misc_comment_phrases, false,false))
    thisInfo.test_item_list[GetName_misc()].push_back(
                        bad_misc_comment_phrases + "$" + GetDiscItemText(seq_feat));
};


void CBioseq_DISC_SUSPECT_MISC_FEATURES :: GetReport(CRef <CClickableItem>& c_item)
{
   GetRepOfSuspPhrase(c_item, GetName(), "misc_feature comment", "misc_feature comments");
};


void CBioseq_test_on_suspect_phrase :: GetRepOfSuspPhrase(CRef <CClickableItem>& c_item, const string& setting_name, const string& phrase_loc_4_1, const string& phrase_loc_4_mul)
{
   Str2Strs tp2feats;
   GetTestItemList(c_item->item_list, tp2feats);
   c_item->item_list.clear();
   string s_desc(phrase_loc_4_1 + " contains "), p_desc(phrase_loc_4_mul + " contain ");
   ITERATE (Str2Strs, it, tp2feats) {
     AddSubcategories(c_item, setting_name, it->second, s_desc, p_desc,
         e_OtherComment, true, "'" + it->first + "'");
   }
   c_item->description
     = GetOtherComment(c_item->item_list.size(), s_desc, p_desc) + "suspect phrase";
};


void CBioseq_test_on_suspect_phrase :: CheckForProdAndComment(const CSeq_feat& seq_feat)
{
   string prod_str = GetRNAProductString(seq_feat);                   
   string desc(GetDiscItemText(seq_feat));
   string comm;
   ITERATE (vector <string>, it, thisInfo.suspect_rna_product_names) {
      if (DoesStringContainPhrase(prod_str, *it, false)) 
            thisInfo.test_item_list[GetName_rna_comm()].push_back(*it + "$" + desc);
      else if (seq_feat.CanGetComment() 
                   && DoesStringContainPhrase(seq_feat.GetComment(), *it, false))
         thisInfo.test_item_list[GetName_rna_comm()].push_back(*it + "$" + desc);
   }
};


void CBioseq_DISC_CHECK_RNA_PRODUCTS_AND_COMMENTS :: GetReport(CRef <CClickableItem>& c_item)
{
   GetRepOfSuspPhrase(c_item, GetName(), "RNA product_name or comment", 
                                                         "RNA product_names or comments");
};
// new comb


void CBioseq_DISC_mRNA_ON_WRONG_SEQUENCE_TYPE :: TestOnObj(const CBioseq& bioseq)
{
  if (bioseq.GetInst().GetMol() != CSeq_inst :: eMol_dna) return;
  if (!IsBioseqHasLineage(bioseq, "Eukaryota")) return;
  bool is_genomic = false;
  ITERATE (vector <const CSeqdesc*>, it, bioseq_molinfo) {
     if ( (*it)->GetMolinfo().GetBiomol() == CMolInfo::eBiomol_genomic) {
       is_genomic = true; break;
     }
  }
  if (!is_genomic) return;
  ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
     CBioSource :: EGenome genome = (CBioSource::EGenome)(*it)->GetSource().GetGenome();
     if ( genome == CBioSource :: eGenome_unknown || genome == CBioSource :: eGenome_genomic
              || genome == CBioSource :: eGenome_macronuclear) 
        return; 
  }
  ITERATE (vector <const CSeq_feat*>, it, mrna_feat) 
     thisInfo.test_item_list[GetName()].push_back( GetDiscItemText((**it)));
};


void CBioseq_DISC_mRNA_ON_WRONG_SEQUENCE_TYPE :: GetReport(CRef <CClickableItem>& c_item)
{
   unsigned cnt = c_item->item_list.size();
   c_item->description = GetIsComment(cnt, "mRNA")
       + "located on eukaryotic sequences that do not have genomic or plasmid " 
       + GetNoun(cnt, "source");
};


/*
void CBioseq_DISC_RBS_WITHOUT_GENE :: TestOnObj(const CBioseq& bioseq)
{
   if (bioseq.IsAa() || gene_feat.empty()) return;
   ITERATE (vector <const CSeq_feat*>, it, rbs_feat) {
     CConstRef <CSeq_feat> 
         rbs_gene = GetBestOverlappingFeat((*it)->GetLocation(),
                                      CSeqFeatData::e_Gene, eOverlap_Contained,
                                      *thisInfo.scope);
     if (rbs_gene.NotEmpty()) 
         thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
   }
};
 

void CBioseq_DISC_RBS_WITHOUT_GENE :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
      = GetDoesComment(c_item->item_list.size(), "RBS feature") + "not have overlapping genes";
};
 
*/


void CBioseq_on_Aa :: CompareIntronExonList(const string& seq_id_desc, const vector <const CSeq_feat*>& exon_ls, const vector <const CSeq_feat*>& intron_ls)
{
   string setting_name = GetName_ei();
   unsigned e_idx=0, i_idx=0, exon_start, exon_stop, intron_start, intron_stop;
   unsigned e_sz = exon_ls.size(), i_sz = intron_ls.size();
   bool has_exon = true, has_intron = true;
   while (e_idx < e_sz && !m_e_exist[e_idx]) e_idx++;
   if (e_idx < e_sz) {
      exon_start = exon_ls[e_idx]->GetLocation().GetStart(eExtreme_Biological);
      exon_stop =  exon_ls[e_idx]->GetLocation().GetStop(eExtreme_Biological);
   }
   else has_exon = false;
   while (i_idx < i_sz && !m_i_exist[i_idx]) i_idx ++;
   if (i_idx < e_sz) {
     intron_start = intron_ls[i_idx]->GetLocation().GetStart(eExtreme_Biological);
     intron_stop =  intron_ls[i_idx]->GetLocation().GetStop(eExtreme_Biological);
   }
   else has_intron = false;

   if (!has_exon || !has_intron) return;

   if (intron_start < exon_start) {
      if (intron_stop != exon_start - 1) {
        thisInfo.test_item_list[setting_name].push_back(
                              seq_id_desc + GetDiscItemText(*exon_ls[e_idx]));
        thisInfo.test_item_list[setting_name].push_back(
                              seq_id_desc + GetDiscItemText(*intron_ls[e_idx]));
      }
      while (i_idx < i_sz && !m_i_exist[i_idx]) i_idx ++;
      if (i_idx < i_sz) {
        intron_start = intron_ls[i_idx]->GetLocation().GetStart(eExtreme_Biological);
        intron_stop = intron_ls[i_idx]->GetLocation().GetStop(eExtreme_Biological);
      }
  };

  unsigned next_exon_start, next_exon_stop;
  while (e_idx < e_sz && i_idx < i_sz) {
      while ( ++ e_idx < e_sz && !m_e_exist[e_idx]);
      next_exon_start = exon_ls[e_idx]->GetLocation().GetStart(eExtreme_Biological);
      next_exon_stop = exon_ls[e_idx]->GetLocation().GetStop(eExtreme_Biological);
      while (i_idx < i_sz && intron_start < next_exon_start) {
        if (intron_start != exon_stop + 1 || intron_stop != next_exon_start - 1) {
          if (intron_start != exon_stop + 1) 
              thisInfo.test_item_list[setting_name].push_back(
                           seq_id_desc + GetDiscItemText(*exon_ls[e_idx]));
          thisInfo.test_item_list[setting_name].push_back(
                           seq_id_desc + GetDiscItemText(*intron_ls[i_idx]));
          if (intron_stop != next_exon_start - 1)
              thisInfo.test_item_list[setting_name].push_back(
                           seq_id_desc + GetDiscItemText(*exon_ls[e_idx]));
        }
        while ( ++ i_idx < i_sz && !m_i_exist[i_idx]);
        if ( i_idx < i_sz) {
          intron_start = intron_ls[i_idx]->GetLocation().GetStart(eExtreme_Biological);
          intron_stop = intron_ls[i_idx]->GetLocation().GetStop(eExtreme_Biological);
        }
      }
      exon_start = next_exon_start;
      exon_stop = next_exon_stop;
  }
  if (i_idx < i_sz)
      if (intron_start != exon_stop + 1) {
          thisInfo.test_item_list[setting_name].push_back(
                           seq_id_desc + GetDiscItemText(*exon_ls[e_idx]));
          thisInfo.test_item_list[setting_name].push_back(
                           seq_id_desc + GetDiscItemText(*intron_ls[i_idx]));
      }
};


void CBioseq_on_Aa :: GetFeatureList4Gene(const CSeq_feat* gene, const vector <const CSeq_feat*> feats, vector <unsigned> exist_ls)
{
   unsigned i=0;
   ITERATE (vector <const CSeq_feat*>, it, feats) {
      const CGene_ref* feat_gene_ref = (*it)->GetGeneXref();
      if (feat_gene_ref) {
         CConstRef <CSeq_feat> 
            feat_gene =GetBestOverlappingFeat((*it)->GetLocation(), CSeqFeatData::e_Gene,
                                       eOverlap_Contained, *thisInfo.scope);
         if (feat_gene.GetPointer() == *it) // ? need to check the pointers
            exist_ls[i] = 1; 
         else exist_ls[i] = 0;
      }
   }
};


void CBioseq_DISC_FEATURE_MOLTYPE_MISMATCH :: TestOnObj(const CBioseq& bioseq)
{
   bool is_genomic = false;
   ITERATE (vector <const CSeqdesc*>, it, bioseq_molinfo) {
      if ( (*it)->GetMolinfo().GetBiomol() == CMolInfo::eBiomol_genomic) is_genomic = true;
   }
   if ( (bioseq.GetInst().GetMol() == CSeq_inst :: eMol_dna) && is_genomic ) return;

   string desc = GetDiscItemText(bioseq);
   if (!rrna_feat.empty()) thisInfo.test_item_list[GetName()].push_back(desc);
   else if (!otherRna_feat.empty()) thisInfo.test_item_list[GetName()].push_back(desc);
};



void CBioseq_DISC_FEATURE_MOLTYPE_MISMATCH :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "sequence") 
        + "rRNA or misc_RNA features but are not genomic DNA.";
};



string CBioseq_ADJACENT_PSEUDOGENES :: GetGeneStringMatch (const string& str1, const string& str2)
{
   string rval(kEmptyStr);

   if (str1.find("hypothetical") != string::npos 
        || str1.find("transposase") != string::npos
        || str2.find("hypothetical") != string::npos
        || str2.find("transposase") != string::npos) 
     return kEmptyStr;

   Str2Int txts;
   vector <string> arr;
   arr = NStr::Tokenize(str1, ";", arr);
   ITERATE (vector <string>, it, arr) txts[*it] = 1;
   
   arr.clear();
   arr = NStr::Tokenize(str2, ";", arr);
   ITERATE (vector <string>, it, arr)
    if (txts.find(*it) != txts.end()) txts[*it] ++;
   
   ITERATE (Str2Int, it, txts)
     if (it->second > 1) rval += (it->first + ";");
   rval = rval.substr(0, rval.size()-1);
   return rval;
};


void CBioseq_ADJACENT_PSEUDOGENES :: TestOnObj(const CBioseq& bioseq)
{
   vector <const CSeq_feat*>::const_iterator jt;   
   ENa_strand this_strand;
   string match_txt;
   ITERATE (vector <const CSeq_feat*>, it, gene_feat) {
     if (!(*it)->CanGetPseudo() || !(*it)->GetPseudo()) continue;
     this_strand = (*it)->GetLocation().GetStrand();
     jt = it + 1; 
     if (!(*jt)->CanGetPseudo() || !(*jt)->GetPseudo()) continue;
     if ((*it)->GetLocation().GetStrand() != (*jt)->GetLocation().GetStrand()) 
               continue;
     if (!(*it)->CanGetComment() || (*it)->GetComment().empty()
           || !(*jt)->CanGetComment() || (*jt)->GetComment().empty())
         continue;
     match_txt = GetGeneStringMatch((*it)->GetComment(), (*jt)->GetComment());
     if (match_txt.empty()) {
       const CGene_ref& this_gene = (*it)->GetData().GetGene();
       const CGene_ref& next_gene = (*jt)->GetData().GetGene();
       if (this_gene.CanGetLocus() && !this_gene.GetLocus().empty()
            && next_gene.CanGetLocus() && !next_gene.GetLocus().empty()) {
         match_txt = GetGeneStringMatch(this_gene.GetLocus(), next_gene.GetLocus());
         if (match_txt.empty()) {
            if (this_gene.CanGetDesc() && !this_gene.GetDesc().empty()
                  && next_gene.CanGetDesc() && !next_gene.GetDesc().empty())
               match_txt = GetGeneStringMatch(this_gene.GetDesc(), next_gene.GetDesc());
         } 
       }
     }    
     if (!match_txt.empty()) {
       thisInfo.test_item_list[GetName()].push_back(match_txt + "$" + GetDiscItemText(**it));
       thisInfo.test_item_list[GetName()].push_back(match_txt + "$" + GetDiscItemText(**jt));
     }
   }
};


void CBioseq_ADJACENT_PSEUDOGENES :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs txt2feats;
   GetTestItemList(c_item->item_list, txt2feats);
   c_item->item_list.clear();
   ITERATE (Str2Strs, it, txt2feats) {
     strtmp = "genes: Adjacent pseudogenes have the same text: " + it->first;
     AddSubcategories(c_item, GetName(), it->second, strtmp, strtmp, e_OtherComment);
   }
   c_item->description
     = NStr::UIntToString((unsigned)c_item->item_list.size()) + " pseudogenes match an adjacent pseudogene's text.";
};


void CBioseq_DISC_FEAT_OVERLAP_SRCFEAT :: TestOnObj(const CBioseq& bioseq)
{
  string src_desc, feat_desc;
  unsigned src_left, src_right, feat_left, feat_right;
  ITERATE (vector <const CSeq_feat*>, it, bioseq_biosrc_feat) {
    src_desc = GetDiscItemText(**it);
    src_left = (*it)->GetLocation().GetStart(eExtreme_Positional);
    src_right = (*it)->GetLocation().GetStop(eExtreme_Positional);
    ITERATE (vector <const CSeq_feat*>, jt, all_feat) {
      feat_desc = GetDiscItemText(**jt); 
      if (src_desc == feat_desc) continue;
      feat_left = (*jt)->GetLocation().GetStart(eExtreme_Positional);
      feat_right = (*jt)->GetLocation().GetStop(eExtreme_Positional);
      if (feat_right < src_left) continue;
      if (feat_left > src_right) break;
      sequence::ECompare 
         ovp = Compare((*it)->GetLocation(), (*jt)->GetLocation(), thisInfo.scope);
      if (ovp != eNoOverlap && ovp != eContained && ovp != eSame)
         thisInfo.test_item_list[GetName()].push_back(src_desc + "$" + feat_desc);
    }
  }
};


void CBioseq_DISC_FEAT_OVERLAP_SRCFEAT :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs biosrc2feats;
   GetTestItemList(c_item->item_list, biosrc2feats);
   c_item->item_list.clear();
   unsigned i=1;
   ITERATE (Str2Strs, it, biosrc2feats) {
     c_item->item_list.push_back(it->first);
     c_item->item_list.insert(
         c_item->item_list.end(), it->second.begin(), it->second.end()); 
     c_item->description 
       = GetOtherComment(c_item->item_list.size(), "feature overlaps", "features overlap") 
          + " a source feature.";
     if (i > 1) thisInfo.disc_report_data.push_back(c_item);
     if (i < biosrc2feats.size()) {
        c_item = CRef <CClickableItem> (new CClickableItem);
        c_item->setting_name = GetName();
     }
   };
};


void CBioseq_CDS_TRNA_OVERLAP :: TestOnObj(const CBioseq& bioseq)
{
   string bioseq_cd_desc, bioseq_desc(GetDiscItemText(bioseq));
   ENa_strand cd_str, trna_str;
   ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
     bioseq_cd_desc = bioseq_desc + "$" + GetDiscItemText(**it);
     cd_str = (*it)->GetLocation().GetStrand();
     ITERATE (vector <const CSeq_feat*>, jt, trna_feat) {
       trna_str = (*jt)->GetLocation().GetStrand();
       sequence::ECompare 
         ovlp = sequence::Compare((*it)->GetLocation(), (*jt)->GetLocation(), thisInfo.scope);
       if ( ((cd_str == eNa_strand_minus && trna_str == eNa_strand_minus )
                        || (cd_str != eNa_strand_minus && trna_str != eNa_strand_minus)) 
             && ovlp != eNoOverlap) {
          thisInfo.test_item_list[GetName()].push_back(
                          bioseq_cd_desc + "#" + GetDiscItemText(**jt));
       }
     }
  }
};


void CBioseq_CDS_TRNA_OVERLAP :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs bioseq_cd2trnas, cd2trnas;
   GetTestItemList(c_item->item_list, bioseq_cd2trnas);
   c_item->item_list.clear();
   ITERATE (Str2Strs, it, bioseq_cd2trnas) {
      CRef <CClickableItem> c_sub (new CClickableItem);
      c_sub->setting_name = GetName();
      cd2trnas.clear();
      GetTestItemList(it->second, cd2trnas, "#");
      ITERATE (Str2Strs, jt, cd2trnas) {
          c_sub->item_list.push_back(jt->first);
          AddSubcategories(c_sub, GetName(), jt->second, "Coding region overlaps tRNAs", 
                                    "Coding region overlaps tRNAs", e_OtherComment); 
      }
      c_sub->description 
                 = GetHasComment(cd2trnas.size(), "coding region") + "overlapping tRNAs";
      c_item->item_list.insert(c_item->item_list.end(), c_sub->item_list.begin(), 
                                                                  c_sub->item_list.end());
      c_item->subcategories.push_back(c_sub);
   }
   c_item->description 
      = GetHasComment(bioseq_cd2trnas.size(), "Bioseq") + "coding region that overlap tRNAs";
};


// new comb
void CBioseq_on_tax_def :: TestOnObj(const CBioseq& bioseq)
{
   if (thisTest.is_TaxDef_run) return;

   string taxnm(kEmptyStr), title(kEmptyStr), tax_desc;
   ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
      if ((*it)->GetSource().GetOrg().CanGetTaxname()) {
        taxnm = (*it)->GetSource().GetOrg().GetTaxname();
        tax_desc = GetDiscItemText(**it, bioseq);
        break;
      }
   }

   string lookfor, desc;
   bool add = false;
   if (!taxnm.empty()) {
      desc = GetDiscItemText(*(bioseq_title[0]), bioseq);
      if (!bioseq_title.empty()) title = bioseq_title[0]->GetTitle();

      // INCONSISTENT_SOURCE_DEFLINE
      if (!title.empty()) {
         if (title.find(taxnm) == string::npos) {
            thisInfo.test_item_list[GetName_inc()].push_back(taxnm + "$" + tax_desc);
            thisInfo.test_item_list[GetName_inc()].push_back(taxnm + "$" + desc);
         }
      }
      
      // TEST_TAXNAME_NOT_IN_DEFLINE
      if (NStr::EqualNocase(taxnm, "Human immunodeficiency virus 1"))
             lookfor = "HIV-1";
      else if (NStr::EqualNocase(taxnm, "Human immunodeficiency virus 2"))
             lookfor = "HIV-2";
      else lookfor = taxnm;
      string pattern("\\b" + CRegexp::Escape(lookfor) + "\\b");
      CRegexp :: ECompile comp_flag = CRegexp::fCompile_ignore_case;
      CRegexp rx(pattern, comp_flag);
      if (rx.IsMatch(title)) {
          // capitalization must match for all but the first letter */
          size_t pos = NStr::FindNoCase(title, lookfor);
          if (title.substr(pos, lookfor.size()) != lookfor.substr(1)) add = true;
      }
      else add = true; // missing in defline
      if (add) thisInfo.test_item_list[GetName_missing()].push_back(desc);
   }

   thisTest.is_TaxDef_run = true;
};


void CBioseq_TEST_TAXNAME_NOT_IN_DEFLINE :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetDoesComment(c_item->item_list.size(), "defline")
            + "not contain the complete taxname.";
};



/*
void CBioseq_INCONSISTENT_SOURCE_DEFLINE :: TestOnObj(const CBioseq& bioseq)
{
   string taxnm(kEmptyStr), title(kEmptyStr), tax_desc;
   ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
      if ((*it)->GetSource().GetOrg().CanGetTaxname()) {
        taxnm = (*it)->GetSource().GetOrg().GetTaxname(); 
        tax_desc = GetDiscItemText(**it, bioseq);
        break;
      } 
   }

   if (!taxnm.empty()) {
      if (!bioseq_title.empty()) title = bioseq_title[0]->GetTitle();
      if (!title.empty() && title.find(taxnm) == string::npos) {
         thisInfo.test_item_list[GetName()].push_back(taxnm + "$" + tax_desc);
         thisInfo.test_item_list[GetName()].push_back(
              taxnm + "$" + GetDiscItemText(*(bioseq_title[0]), bioseq));
      }
   }
};
*/


void CBioseq_INCONSISTENT_SOURCE_DEFLINE :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs tax2list;
   GetTestItemList(c_item->item_list, tax2list);
   c_item->item_list.clear();
   if (tax2list.size() == 1) {
     c_item->item_list = tax2list.begin()->second;
     c_item->description 
       = "Organism description not found in definition line: " + tax2list.begin()->first;
   }
   else {
      ITERATE (Str2Strs, it, tax2list) {
         AddSubcategories(c_item, GetName(), it->second,
              "Organism description not found in definition line: " + it->first, "",
              e_OtherComment, false);
      }
      c_item->description
        = GetDoesComment(tax2list.size(), "source") + + "not match definition lines.";
   }
};


void CBioseq_CONTAINED_CDS :: TestOnObj(const CBioseq& bioseq)
{
   set <string> contained_this_strand, last_this_strand;
   set <string> contained_other_strand, last_other_strand;
   ENa_strand str1, str2;
   unsigned i=0, j=0;
   string desc1, desc2;
   for (i=0; (int)i< (int)(cd_feat.size()-1); i++) {
     const CSeq_loc& loc1 = cd_feat[i]->GetLocation();
     str1 = loc1.GetStrand();
     for (j=i+1; j< cd_feat.size(); j++) {
        const CSeq_loc& loc2 = cd_feat[j]->GetLocation();
        str2 = loc2.GetStrand();
        sequence:ECompare loc_cmp = Compare(loc1, loc2, thisInfo.scope);
        if (loc_cmp == eSame || loc_cmp == eContained || loc_cmp == eContains) {
          desc1 = GetDiscItemText(*cd_feat[i]);
          desc2 = GetDiscItemText(*cd_feat[j]);
          if (StrandOk(str1, str2)) {
             if (contained_this_strand.find(desc1) == contained_this_strand.end()) {
                thisInfo.test_item_list[GetName()].push_back("same$" + desc1);
                last_this_strand.insert(desc1); 
                if (contained_this_strand.empty()) 
                       contained_this_strand = last_this_strand;
             }
             if (contained_this_strand.find(desc2) == contained_this_strand.end()) {
                thisInfo.test_item_list[GetName()].push_back("same$" + desc2);
                last_this_strand.insert(desc2);
                if (contained_this_strand.empty()) 
                     contained_this_strand = last_this_strand;
             }
          } 
          else {
             if (contained_other_strand.find(desc1) == contained_other_strand.end()) {
                 thisInfo.test_item_list[GetName()].push_back("opposite$" + desc1);
                 last_other_strand.insert(desc1);
                 if (contained_other_strand.empty())
                      contained_other_strand = last_other_strand;
             }
             if (contained_other_strand.find(desc2) == contained_other_strand.end()) {
                thisInfo.test_item_list[GetName()].push_back("opposite$" + desc2);
                last_other_strand.insert(desc2);
                if (contained_other_strand.empty())
                     contained_other_strand = last_other_strand;
             }
          }
        }
     }
   }
   contained_this_strand.clear();
   contained_other_strand.clear();
   last_this_strand.clear();
   last_other_strand.clear();
};


void CBioseq_CONTAINED_CDS :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs list_tp2items;
   GetTestItemList(c_item->item_list, list_tp2items);
   c_item->item_list.clear();
   if (list_tp2items.size() == 1) {
      c_item->item_list = list_tp2items.begin()->second;
      c_item->description 
          = GetIsComment(c_item->item_list.size(), "coding region")
                + ", completely contained in another coding region on the " 
                + list_tp2items.begin()->first + " strand.";
   }
   else {
      ITERATE (Str2Strs, it, list_tp2items) {
        AddSubcategories(c_item, GetName(), it->second, "coding region", 
  "completely contained in another coding region on the " + it->first + " strand.");
           
      }
      c_item->description = GetIsComment(c_item->item_list.size(), "coding region") 
                              + "completely contained in another coding region.";
   }
};


void CBioseq_PSEUDO_MISMATCH :: FindPseudoDiscrepancies(const CSeq_feat& seq_feat)
{
    const CGene_ref* xref_gene = seq_feat.GetGeneXref();
    if ( xref_gene ) return;//not empty
    else {
      CConstRef <CSeq_feat> gene_olp= GetBestOverlappingFeat(seq_feat.GetLocation(),
                                                             CSeqFeatData::e_Gene,
                                                             eOverlap_Contained,
                                                             *thisInfo.scope);
      if (gene_olp.Empty()) return;
      if (seq_feat.CanGetPseudo() && seq_feat.GetPseudo() 
                      && !(gene_olp->CanGetPseudo() && gene_olp->GetPseudo())) {
         thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(seq_feat));
         thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(*gene_olp));
      }
    }
};



void CBioseq_PSEUDO_MISMATCH :: TestOnObj(const CBioseq& bioseq)
{
  ITERATE (vector <const CSeq_feat*>, it, cd_feat)
    FindPseudoDiscrepancies(**it);

  ITERATE (vector <const CSeq_feat*>, it, rna_feat) 
    FindPseudoDiscrepancies(**it);
};


void CBioseq_PSEUDO_MISMATCH :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = NStr::UIntToString((unsigned)c_item->item_list.size()) 
                             + " CDSs, RNAs, and genes have mismatching pseudos.";
};


void CBioseq_EC_NUMBER_NOTE :: TestOnObj(const CBioseq& bioseq) 
{
  string desc;
  bool done;
  ITERATE (vector <const CSeq_feat*>, it, all_feat) {
    done = false;

    if ( (*it)->CanGetComment() && !((*it)->GetComment().empty())) {
      desc = GetDiscItemText(**it); 
      if (CProt_ref::IsValidECNumberFormat((*it)->GetComment())) 
         thisInfo.test_item_list[GetName()].push_back(desc);
      else if ( (*it)->GetData().IsCdregion() && (*it)->CanGetProduct()) {
        CConstRef <CProt_ref> prot_ref (GetProtRefForFeature(**it, false));
        if (prot_ref.NotEmpty() && prot_ref->CanGetName() 
                                         && !(prot_ref->GetName().empty())) {
          ITERATE (list <string>, jt, prot_ref->GetName()) {
            if (CProt_ref::IsValidECNumberFormat(*jt)) {
               thisInfo.test_item_list[GetName()].push_back(desc);
               done = true;
               break;
            }
          }
          if (!done && prot_ref->CanGetDesc() && !(prot_ref->GetDesc().empty())
                 && CProt_ref::IsValidECNumberFormat(prot_ref->GetDesc())) {
               thisInfo.test_item_list[GetName()].push_back(desc);
          }
        }
      }
    }
  }
};


void CBioseq_EC_NUMBER_NOTE :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description 
    = GetHasComment(c_item->item_list.size(), "feature") + "EC numbers in notes or products.";
};


void CBioseq_NON_GENE_LOCUS_TAG :: TestOnObj(const CBioseq& bioseq)
{
   ITERATE (vector <const CSeq_feat*>, it, all_feat) {
     if (!(*it)->GetData().IsGene()) {
       if ( (*it)->IsSetQual())
         ITERATE (vector <CRef <CGb_qual> >, jt, (*it)->GetQual()) {
            if ( NStr::EqualNocase((*jt)->GetQual(), "locus_tag")) {
               thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
               break;
            }
         }
     }
   }
};


void CBioseq_NON_GENE_LOCUS_TAG :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
     = GetHasComment(c_item->item_list.size(), "non_gene feature") + "locus tags.";
};



void CBioseq_SHOW_TRANSL_EXCEPT :: TestOnObj(const CBioseq& bioseq)
{
  ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
     if ( (*it)->GetData().GetCdregion().IsSetCode_break()) // don;t use CanGet...() for list.
       thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
  }
};


void CBioseq_SHOW_TRANSL_EXCEPT :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description 
     = GetHasComment(c_item->item_list.size(), "coding region") + "a translation exception.";
};


bool CBioseqTestAndRepData :: IsUnknown(const string& known_items, const unsigned idx)
{
   if (known_items.find("|" + NStr::UIntToString(idx) + "|") == string::npos) return true;
   else return false;
};


// equal = IsPesudo(sfp) in C
bool CBioseqTestAndRepData :: IsPseudoSeqFeatOrXrefGene(const CSeq_feat* seq_feat)
{
   bool rvl;
   if (seq_feat->CanGetPseudo()) rvl = seq_feat->GetPseudo();
   else {
     const CSeqFeatData& seq_feat_dt = seq_feat->GetData();
     if (seq_feat_dt.IsGene()) rvl = seq_feat_dt.GetGene().GetPseudo();
     else {
        CConstRef <CSeq_feat> gene_olp= GetBestOverlappingFeat(seq_feat->GetLocation(),
                                                             CSeqFeatData::e_Gene,
                                                             eOverlap_Contained,
                                                             *thisInfo.scope);
        if (gene_olp.Empty()) rvl = false;
        else rvl = IsPseudoSeqFeatOrXrefGene(gene_olp.GetPointer());
     }
   }

   return rvl;
};



string CBioseqTestAndRepData :: GetRNAProductString(const CSeq_feat& seq_feat)
{
  const CRNA_ref& rna = seq_feat.GetData().GetRna();
  string rna_str(kEmptyStr);
  if (!rna.CanGetExt()) rna_str = seq_feat.GetNamedQual("product");
  else {
     const CRNA_ref::C_Ext& ext = rna.GetExt();
     switch (ext.Which()) {
       case CRNA_ref::C_Ext::e_Name:
             rna_str = ext.GetName();
             if (rna_str.empty()|| rna_str== "ncRNA"|| rna_str== "tmRNA"||rna_str== "misc_RNA")
                 rna_str = seq_feat.GetNamedQual("product");
             break;
       case CRNA_ref::C_Ext::e_TRNA:
              GetSeqFeatLabel(seq_feat, &rna_str);
              break;
       case CRNA_ref::C_Ext::e_Gen:
              if (ext.GetGen().CanGetProduct()) rna_str = ext.GetGen().GetProduct();
       default: break;
     }
  }
  return rna_str;
};


bool CBioseq_on_Aa :: Is5EndInUTRList(const unsigned& start)
{
   ITERATE (vector <const CSeq_feat*>, it, utr5_feat) {
     if (start == (*it)->GetLocation().GetStart(eExtreme_Positional)) return true;
   }
   return false;
};


bool CBioseq_on_Aa :: Is3EndInUTRList(const unsigned& stop)
{
  ITERATE (vector <const CSeq_feat*>, it, utr3_feat) {
     if (stop == (*it)->GetLocation().GetStop(eExtreme_Positional)) return true;
  }
  return false;
};



void CBioseq_on_Aa :: ReportPartialConflictsForFeatureType(vector <const CSeq_feat*>& seq_feats, string label, bool check_for_utrs)
{
  CRef <CSeq_loc> gene_loc, feat_loc;
  ENa_strand feat_strand, gene_strand;
  unsigned feat_start, feat_stop, gene_start, gene_stop;
  bool conflict5, conflict3, feat_partial5, feat_partial3, gene_partial5, gene_partial3;
  CConstRef <CSeq_feat> gene_feat_4_feat;
  ITERATE (vector <const CSeq_feat*>, it, seq_feats) {
    gene_feat_4_feat = CConstRef <CSeq_feat> (GetGeneForFeature(**it));
    if (gene_feat_4_feat.NotEmpty()) {
      feat_loc = Seq_loc_Merge((*it)->GetLocation(), 
                     CSeq_loc::fMerge_Overlapping | CSeq_loc::fSort, thisInfo.scope);
      gene_loc = Seq_loc_Merge(gene_feat_4_feat->GetLocation(),
                      CSeq_loc::fMerge_Overlapping | CSeq_loc::fSort, thisInfo.scope); 
      feat_strand = feat_loc->GetStrand();
      if (feat_strand == eNa_strand_minus) {
         feat_start = feat_loc->GetStop(eExtreme_Positional);
         feat_stop = feat_loc->GetStart(eExtreme_Positional); 
      }    
      else {
         feat_start = feat_loc->GetStart(eExtreme_Positional);
         feat_stop = feat_loc->GetStop(eExtreme_Positional);
      }
      gene_strand = gene_loc->GetStrand();
      if (gene_strand  == eNa_strand_minus) {
         gene_start = gene_loc->GetStop(eExtreme_Positional);
         gene_stop = gene_loc->GetStart(eExtreme_Positional);
      } 
      else {
         gene_start = gene_loc->GetStart(eExtreme_Positional);
         gene_stop = gene_loc->GetStop(eExtreme_Positional);
      }
      feat_partial5 = feat_partial3 = gene_partial5 = gene_partial3 = false;
      feat_partial5 = feat_loc->IsPartialStart(eExtreme_Biological);
      feat_partial3 = feat_loc->IsPartialStop(eExtreme_Biological);
      gene_partial5 = gene_loc->IsPartialStart(eExtreme_Biological);
      gene_partial3 = gene_loc->IsPartialStop(eExtreme_Biological);

      conflict5 = false;
      if ( (feat_partial5 && !gene_partial5) || (!feat_partial5 && gene_partial5) ) {
        if (feat_start == gene_start) conflict5 = true;
        else if (check_for_utrs && !Is5EndInUTRList(gene_start)) conflict5 = true;
      }

      conflict3 = false;
      if ( (feat_partial3 && !gene_partial3) || (!feat_partial3 && gene_partial3) ) {
        if (feat_stop == gene_stop) conflict3 = true;
        else if (check_for_utrs && !Is3EndInUTRList(gene_stop)) conflict3 = true;
      }

      strtmp = label;
      if (conflict5 || conflict3) {
        if (conflict5 && conflict3) 
             strtmp += " feature partialness conflicts with gene on both ends.";
        else if (conflict5) 
              strtmp += " feature partialness conflicts with gene on 5' end.";
        else strtmp += " feature partialness conflicts with gene on 3' end.";

        thisInfo.test_item_list[GetName_pgene()].push_back(
                               strtmp + "$" + GetDiscItemText(**it));
        thisInfo.test_item_list[GetName_pgene()].push_back(
                               strtmp + "$" +GetDiscItemText(*gene_feat_4_feat));
      }
    }
  }
};

/*

bool CBioseqTestAndRepData :: IsMrnaSequence()
{
  ITERATE (vector <const CSeqdesc*>, it, bioseq_molinfo) {
     if ( (*it)->GetMolinfo().GetBiomol() == CMolInfo :: eBiomol_mRNA) return true;
  }
  return false;
};


void CBioseq_DISC_BACTERIA_SHOULD_NOT_HAVE_MRNA :: TestOnObj(const CBioseq& bioseq)
{
   if (IsBioseqHasLineage(bioseq, "Bacteria") && !mrna_feat.empty())
       thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(bioseq));
};


void CBioseq_DISC_BACTERIA_SHOULD_NOT_HAVE_MRNA :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
     = GetHasComment(c_item->item_list.size(), "bacterial sequence") + "mRNA features.";
};

*/


bool CBioseqTestAndRepData :: IsMrnaSequence()
{
  ITERATE (vector <const CSeqdesc*>, it, bioseq_molinfo) {
     if ( (*it)->GetMolinfo().GetBiomol() == CMolInfo :: eBiomol_mRNA) return true;
  }
  return false;
};


void CBioseq_DISC_BACTERIA_SHOULD_NOT_HAVE_MRNA :: TestOnObj(const CBioseq& bioseq)
{
   if (IsBioseqHasLineage(bioseq, "Bacteria") && !mrna_feat.empty())
       thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(bioseq));
};


void CBioseq_DISC_BACTERIA_SHOULD_NOT_HAVE_MRNA :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
     = GetHasComment(c_item->item_list.size(), "bacterial sequence") + "mRNA features.";
};



// new comb
bool CBioseq_test_on_rrna :: NameNotStandard(const string& nm)
{
  unsigned i=0;
  bool is_equal = false, is_no_case_equal = false;
  ITERATE (vector <string>, it, thisInfo.rrna_standard_name) {
    if ( (*it) == nm ) { is_equal = true; break;}
    else if ( NStr::EqualNocase((*it), nm) ) {
        is_no_case_equal = true; break;
    }
    else i++;
  }

  vector <string> arr;
  string nm_new;
  if (is_equal) return false;
  else if (is_no_case_equal) {
    if (!DoesStringContainPhrase(nm, "RNA")) return true;
    arr = NStr::Tokenize(nm, " ", arr); 
    NON_CONST_ITERATE (vector <string>, it, arr) 
     if (*it != "RNA" && isalpha( (*it)[0] ) && isupper( (*it)[0] ))
       (*it)[0] = tolower( (*it)[0]);
    nm_new = NStr::Join(arr, " ");
    if (nm_new == thisInfo.rrna_standard_name[i]) return false;
    else return true; 
  }
  else return TRUE;
};



void CBioseq_test_on_rrna :: TestOnObj(const CBioseq& bioseq)
{
   if (thisTest.is_RRna_run) return;

   string desc, rrna_name, prod_str;
   unsigned len;
   ITERATE (vector <const CSeq_feat*>, it, rrna_feat) {
     desc = GetDiscItemText(**it); 
     const CRNA_ref& rrna = (*it)->GetData().GetRna();

     // RRNA_NAME_CONFLICTS
     if ( rrna.CanGetExt() && rrna.GetExt().IsName()
                                               && NameNotStandard(rrna.GetExt().GetName())) {
        thisInfo.test_item_list[GetName_nm()].push_back(desc);
     }

     // DISC_INTERNAL_TRANSCRIBED_SPACER_RRNA
     prod_str = GetRNAProductString(**it); 
     if (NStr::FindNoCase(prod_str, "internal") != string::npos
             || NStr::FindNoCase(prod_str, "transcribed") != string::npos
             || NStr::FindNoCase(prod_str, "spacer") != string::npos) 
        thisInfo.test_item_list[GetName_its()].push_back(desc);

     // DISC_SHORT_RRNA
     if ((*it)->CanGetPartial() && (*it)->GetPartial()) continue;  // ??? with ITERATE
     len = sequence::GetCoverage((*it)->GetLocation(), thisInfo.scope);
     rrna_name = GetRNAProductString(**it);
     ITERATE (Str2UInt, jt, thisInfo.rRNATerms) {
        if (NStr::FindNoCase(rrna_name, jt->first) != string::npos 
                  && len < jt->second
                  && (!thisInfo.rRNATerms_partial[jt->first] 
                       || !((*it)->CanGetPartial() && (*it)->GetPartial()))  ) {
             thisInfo.test_item_list[GetName_short()].push_back(desc);
             break;
        }
     }
   }

   thisTest.is_RRna_run = true;
};


void CBioseq_DISC_INTERNAL_TRANSCRIBED_SPACER_RRNA :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetOtherComment(c_item->item_list.size(), 
                             "rRNA feature product contain", "rRNA feature products contain")
         + " 'internal', 'transcribed', or 'spacer'";
};


void CBioseq_DISC_SHORT_RRNA :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
      = GetIsComment(c_item->item_list.size(), "rRNA feature") + "too short.";
};

/*
void CBioseq_RRNA_NAME_CONFLICTS :: TestOnObj(const CBioseq& bioseq)
{
  ITERATE (vector <const CSeq_feat*>, it, rrna_feat) {
     const CRNA_ref& rrna = (*it)->GetData().GetRna();
     if ( rrna.CanGetExt() && rrna.GetExt().IsName() 
                                               && NameNotStandard(rrna.GetExt().GetName())) {
        thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
     }
  }
};
*/

void CBioseq_RRNA_NAME_CONFLICTS :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
        = GetIsComment(c_item->item_list.size(), "rRNA product name") + "not standard.";
};



CConstRef <CSeq_feat> CBioseqTestAndRepData :: GetmRNAforCDS(const CSeq_feat& cd_feat, const CSeq_entry& seq_entry)
{
  /* first, check for mRNA identified by feature xref */
  if (cd_feat.CanGetXref()) {
    ITERATE (vector <CRef <CSeqFeatXref> >, it, cd_feat.GetXref()) {
      if ( (*it)->CanGetId() ) {
        const CFeat_id& id = (*it)->GetId();
        if (id.IsLocal()) {
          const CObject_id& obj_id = id.GetLocal();
          if (obj_id.IsId()) {
            int local_id = obj_id.GetId();
            CSeq_feat_Handle 
                seq_feat_hl = thisInfo.scope->GetTSE_Handle(seq_entry).GetFeatureWithId(
                                                       CSeqFeatData::eSubtype_mRNA, local_id); 
            if (seq_feat_hl) return (seq_feat_hl.GetSeq_feat());
          }
        }
      }
    }
  }

  /* try by location if not by xref */
  CConstRef <CSeq_feat> mRNA = GetBestOverlappingFeat(cd_feat.GetLocation(),
                                  CSeqFeatData::eSubtype_mRNA,
                                  eOverlap_Subset,
                                  *thisInfo.scope);
  if (mRNA.Empty())
      mRNA = GetBestOverlappingFeat(cd_feat.GetLocation(), CSeqFeatData::eSubtype_mRNA,
                                                           eOverlap_Contained,
                                                           *thisInfo.scope);
  if (mRNA.Empty()) {
      CConstRef <CBioseq> 
          bioseq = 
             GetBioseqFromSeqLoc(cd_feat.GetLocation(), *thisInfo.scope).GetCompleteBioseq();
      if (bioseq.NotEmpty() && IsmRNASequenceInGenProdSet(*bioseq) )
          mRNA =CConstRef <CSeq_feat> (sequence :: GetmRNAForProduct(*bioseq, thisInfo.scope));
  }

  return mRNA;
}



CConstRef <CProt_ref> CBioseqTestAndRepData :: GetProtRefForFeature(const CSeq_feat& seq_feat, bool look_xref)
{
  CConstRef <CProt_ref> prot_ref;
  if (seq_feat.GetData().IsProt()) 
       prot_ref = CConstRef <CProt_ref> (&(seq_feat.GetData().GetProt()));
  else if (seq_feat.GetData().IsCdregion()) {
    if (look_xref) {
      if (seq_feat.CanGetXref()) {
        ITERATE (vector <CRef <CSeqFeatXref> >, it, seq_feat.GetXref()) {
          if ( (*it)->CanGetData() && (*it)->GetData().IsProt())
            prot_ref = CConstRef <CProt_ref> (&( (*it)->GetData().GetProt() ));
        }
      }
    }

    if (prot_ref.Empty() && seq_feat.CanGetProduct()) {
        CBioseq_Handle bioseq_h = GetBioseqFromSeqLoc(seq_feat.GetProduct(), *thisInfo.scope);
        for (CFeat_CI prot_ci(bioseq_h, CSeqFeatData::e_Prot); prot_ci; ++prot_ci) {
          prot_ref 
              = CConstRef <CProt_ref> (&(prot_ci->GetOriginalFeature().GetData().GetProt())); 
          if (prot_ref->GetProcessed() == CProt_ref :: eProcessed_not_set) break;
	}
    }
  }

  return prot_ref;
};

/*
GetGBQualFromFeatQual(EFeat_qual_legal feat_legal_dt, int gb_qual, int& subfield)
{
};

string GetFirstGBQualMatch()

string GetFirstGBQualMatchConstraintName(
*/

string CBioseqTestAndRepData :: GetFieldValueForObject(const CSeq_feat& seq_feat, const CFeat_qual_choice& feat_qual)
{
  string ret_str(kEmptyStr);

  // for protein fields
  CConstRef <CProt_ref> prot = GetProtRefForFeature (seq_feat);

  /* fields common to some features */
  /* product */
  if (ret_str.empty()
      && ((feat_qual.IsLegal_qual() && feat_qual.GetLegal_qual() == eFeat_qual_legal_product) 
//          || (feat_qual.IsIllegal_qual() && DoesStringMatchConstraint ("product", field->data.ptrvalue))
))
  {
    if (prot.NotEmpty()) {
      if (prot->CanGetName()) {
        ret_str = *(prot->GetName().begin()); //str = GetFirstValNodeStringMatch (prp->name, scp);
      }
    } 
    else if (seq_feat.GetData().IsRna()) {
      ret_str = CBioseqTestAndRepData :: GetRNAProductString (seq_feat);
   //   str = GetRNAProductString (seq_feat, scp);
    }
  }

  /* actual GenBank qualifiers */
/*
  if (ret_str.empty()) {
    if (feat_qual.IsLegal_qual()) {
      gbqual = GetGBQualFromFeatQual (field->data.intvalue, &subfield);
      if (gbqual > -1) 
        str = GetFirstGBQualMatch (sfp->qual, ParFlat_GBQual_names [gbqual].name, subfield, scp);
    } else {
      str = GetFirstGBQualMatchConstraintName (sfp->qual, field->data.ptrvalue, scp);
    }
  }
*/
  return ret_str;
}


CSeqFeatData::ESubtype CBioseqTestAndRepData :: GetFeatdefFromFeatureType(EMacro_feature_type feat_field_type)
{
   switch (feat_field_type) {
     case eMacro_feature_type_any:
         return (CSeqFeatData :: eSubtype_any); break;
     default: return (CSeqFeatData :: eSubtype_bad);
   }
};


string CBioseqTestAndRepData :: GetQualFromFeature(const CSeq_feat& seq_feat, const CField_type& field_type)
{
  if (field_type.IsFeature_field()) {
     const CFeature_field& feat_field = field_type.GetFeature_field();
     EMacro_feature_type feat_field_type = feat_field.GetType();
     if (eMacro_feature_type_any == feat_field_type
             || seq_feat.GetData().GetSubtype() == GetFeatdefFromFeatureType(feat_field_type))
        return (GetFieldValueForObject(seq_feat, feat_field.GetField()));
  }
  return (kEmptyStr);
};


bool CBioseqTestAndRepData :: IsLocationOrganelle(int genome)
{
    if (genome == (int)CBioSource :: eGenome_chloroplast
      || genome == (int)CBioSource :: eGenome_chromoplast
      || genome == (int)CBioSource :: eGenome_kinetoplast
      || genome == (int)CBioSource :: eGenome_mitochondrion
      || genome == (int)CBioSource :: eGenome_cyanelle
      || genome == (int)CBioSource :: eGenome_nucleomorph
      || genome == (int)CBioSource :: eGenome_apicoplast
      || genome == (int)CBioSource :: eGenome_leucoplast
      || genome == (int)CBioSource :: eGenome_proplastid
      || genome == (int)CBioSource :: eGenome_hydrogenosome
      || genome == (int)CBioSource :: eGenome_plastid
      || genome == (int)CBioSource :: eGenome_chromatophore) {
    return true;
  } 
  else return false;
};


static string kCDSVariant(", isoform ");
static string kmRNAVariant(", transcript variant ");

bool CBioseqTestAndRepData :: ProductsMatchForRefSeq(const string& feat_prod, const string& mRNA_prod)
{
  if (feat_prod.empty() || mRNA_prod.empty()) return false;

  size_t pos_m, pos_f;
  if ( (pos_m = mRNA_prod.find(kmRNAVariant)) == string::npos) return false;

  if ( (pos_f = feat_prod.find(kCDSVariant)) == string::npos) return false;

  if (pos_m != pos_f) return false;
  else if (mRNA_prod.substr(0, pos_m) != feat_prod.substr(0, pos_f)) return false;

  if ( feat_prod.substr(pos_f, kCDSVariant.size())
                                 == mRNA_prod.substr(pos_m, kmRNAVariant.size()))
       return true;
  else return false;
};



void CBioseqTestAndRepData :: TestOnMRna(const CBioseq& bioseq)
{
  bool has_qual_ids = false;
  if (bioseq.GetInst().GetMol() != CSeq_inst::eMol_dna 
           || !IsBioseqHasLineage(bioseq, "Eukaryota", false)) return;
  ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
    if (IsLocationOrganelle((*it)->GetSource().GetGenome())) {
           return;
    }
  }
  
  ITERATE (vector <const CSeqdesc*>, it, bioseq_molinfo) {
    if ( (*it)->GetMolinfo().GetBiomol() == CMolInfo::eBiomol_genomic) {
        return;
    }
  }
 
  string feat_prod, mRNA_prod;
  CSeq_feat_Handle cd_hl;
  CSeq_entry* seq_entry = bioseq.GetParentEntry();

  CRef <CFeature_field> feat_field (new CFeature_field);
  feat_field->SetType(eMacro_feature_type_any);
  CRef <CFeat_qual_choice> feat_qual (new CFeat_qual_choice);
  feat_qual->SetLegal_qual(eFeat_qual_legal_product);
  feat_field->SetField(*feat_qual);
  CRef <CField_type> field_type (new CField_type);
  field_type->SetFeature_field(*feat_field);

  ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
    if (IsPseudoSeqFeatOrXrefGene(*it)) continue;
    CConstRef <CSeq_feat> mRNA = GetmRNAforCDS(**it, *seq_entry);
    if (mRNA.Empty())
      thisInfo.test_item_list[GetName_no_mrna()].push_back(GetDiscItemText(**it));
    else {
      feat_prod = GetQualFromFeature(**it, *field_type);
      mRNA_prod = GetQualFromFeature(*mRNA, *field_type);
      if (feat_prod != mRNA_prod && !ProductsMatchForRefSeq(feat_prod, mRNA_prod))
         thisInfo.test_item_list[GetName_no_mrna()].push_back(GetDiscItemText(**it));
      if (!has_qual_ids) {
         if (!(mRNA->GetNamedQual("orig_protein_id").empty()) 
                                && !(mRNA->GetNamedQual("orig_transcript_id").empty())) 
           has_qual_ids = true;
      }
    }     
  }

  strtmp = has_qual_ids ? "yes" : "no";
  thisInfo.test_item_list[GetName_pid_tid()].push_back(strtmp);
  thisTest.is_MRNA_run = true;
};


void CBioseq_DISC_CDS_WITHOUT_MRNA :: TestOnObj(const CBioseq& bioseq)
{
   if (!thisTest.is_MRNA_run) TestOnMRna(bioseq);
};


void CBioseq_DISC_CDS_WITHOUT_MRNA :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description 
     = GetDoesComment(c_item->item_list.size(), "coding region") + "not have an mRNA.";
};


void CBioseq_MRNA_SHOULD_HAVE_PROTEIN_TRANSCRIPT_IDS :: TestOnObj(const CBioseq& bioseq)
{
  if (!thisTest.is_MRNA_run) TestOnMRna(bioseq);
};


void CBioseq_MRNA_SHOULD_HAVE_PROTEIN_TRANSCRIPT_IDS :: GetReport(CRef <CClickableItem>& c_item)
{
  bool has_ids = false;
  ITERATE (vector <string>, it, c_item->item_list) 
    if ( (*it).find("yes") != string::npos) {
       has_ids = true;
       break;
    }
  if (!has_ids) {
    c_item->item_list.clear();
    c_item->description = "no protein_id and transcript_id present.";
  }
};


void CBioseq_GENE_PRODUCT_CONFLICT :: TestOnObj(const CBioseq& bioseq)
{  
   string gene_label(kEmptyStr), prod_nm, desc;
   ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
     gene_label = kEmptyStr;
     // GetGeneLabel
     CGene_ref* gene = const_cast <CGene_ref*> ( (*it)->GetGeneXref());
     if (!gene) {
        CConstRef<CSeq_feat> g_olp= 
                                GetBestOverlappingFeat( (*it)->GetLocation(),
                                                   CSeqFeatData::e_Gene,
                                                   eOverlap_Contained,
                                                   *thisInfo.scope);
        if (g_olp.NotEmpty()) 
            gene = const_cast< CGene_ref* > (&(g_olp->GetData().GetGene()));
     } 
     if (gene && gene->CanGetLocus()) gene_label = gene->SetLocus();

     if (!(gene_label.empty())) {
       prod_nm = GetProdNmForCD(**it);
       desc = GetDiscItemText(**it);
       Str2SubDt::iterator it = GENE_PRODUCT_CONFLICT_cds.find(gene_label);
       if (it == GENE_PRODUCT_CONFLICT_cds.end())
           GENE_PRODUCT_CONFLICT_cds[gene_label] 
                = CRef < GeneralDiscSubDt > (new GeneralDiscSubDt(desc, prod_nm));
       else {
          if (it->second->str != prod_nm) { // conflict
            if ( !( it->second->sf0_added )) {
                thisInfo.test_item_list[GetName()].push_back(it->second->obj_text[0]);
                it->second->sf0_added = true;
            }
            thisInfo.test_item_list[GetName()].push_back(desc);
            it->second->obj_text.push_back(strtmp);
          }
       }
     }
   }
};


void CBioseq_GENE_PRODUCT_CONFLICT :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetHasComment(c_item->item_list.size(), "coding region")
      + "the same gene name as another coding region but a different product.";

  ITERATE ( Str2SubDt, it, GENE_PRODUCT_CONFLICT_cds) {
    if ( it->second->obj_text.size() > 1) {
      CRef <CClickableItem> c_sub (new CClickableItem);
      c_sub->setting_name = GetName();
      c_sub->item_list = it->second->obj_text;
      strtmp = (it->second->str.empty())? kEmptyStr: ("(" + it->second->str + ") ");
      c_sub->description = GetHasComment(c_sub->item_list.size(), "coding region")
         + "the same gene name " + strtmp
         + (string)"as another coding region but a different product.";
      c_item->subcategories.push_back(c_sub);
    }
  }

};



string CBioseq_TEST_UNUSUAL_MISC_RNA :: GetTrnaProductString(const CTrna_ext& trna_ext)
{
  int            aa = 0;
  unsigned       bb = 0, idx;
  ESeq_code_type from;
  string         str(kEmptyStr);
 
  const CTrna_ext::C_Aa& aa_dt = trna_ext.GetAa();
  switch (aa_dt.Which()) {
      case CTrna_ext::C_Aa::e_Iupacaa:
        from = eSeq_code_type_iupacaa;
        aa = aa_dt.GetIupacaa();
        break;
      case CTrna_ext::C_Aa::e_Ncbieaa :
        from = eSeq_code_type_ncbieaa;
        bb = aa_dt.GetNcbieaa();
        break;
      case CTrna_ext::C_Aa::e_Ncbi8aa :
        from = eSeq_code_type_ncbi8aa;
        aa = aa_dt.GetNcbi8aa();
        break;
      case CTrna_ext::C_Aa::e_Ncbistdaa :
        from = eSeq_code_type_ncbistdaa;
        aa = aa_dt.GetNcbistdaa();
        break;
      default:
        NCBI_THROW(CException, eUnknown, "GetTrnaProductString: can't get from.");
        break;
  }

  if (from != eSeq_code_type_ncbieaa) {
    bb = CSeqportUtil::GetMapToIndex(from, eSeq_code_type_ncbieaa, aa);
    if (bb == 255 && from == eSeq_code_type_iupacaa) {
        if (aa == 'U') bb = 'U';
        else if (aa == 'O') bb = 'O';
    }
  }
  if (bb > 0 && bb != 255) {
    if (bb != '*') idx = bb - 64;
    else idx = 25;
    if (idx > 0 && idx < 28) str = thisInfo.trna_list [idx];
  }
  return str;
};


string CBioseq_TEST_UNUSUAL_MISC_RNA :: GetRnaRefProductString(const CRNA_ref& rna_ref)
{
  string str(kEmptyStr);

  const CRNA_ref::C_Ext& ext = rna_ref.GetExt();
  switch (ext.Which()) {
    case CRNA_ref::C_Ext::e_Name:
        str = ext.GetName();  break;
    case CRNA_ref::C_Ext::e_TRNA:
        if (ext.GetTRNA().CanGetAa()) str = GetTrnaProductString(ext.GetTRNA()); 
        break;
    case CRNA_ref::C_Ext::e_Gen:
        if (ext.GetGen().CanGetProduct())
          str = ext.GetGen().GetProduct(); break;
    default: break;
  }

  return str;
};


void CBioseq_TEST_UNUSUAL_MISC_RNA :: TestOnObj(const CBioseq& bioseq)
{
   string product;
   ITERATE (vector <const CSeq_feat*>, it, otherRna_feat) {
      const CRNA_ref& rna_ref = (*it)->GetData().GetRna();
      if ( rna_ref.CanGetExt() && rna_ref.GetExt().Which() != CRNA_ref::C_Ext::e_not_set ) {
         product = GetRnaRefProductString( (*it)->GetData().GetRna());
         if (product.find("ITS") == string::npos 
                          && product.find("internal transcribed spacer") == string::npos) { 
            thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
            break;
         }
      }
   }
};


void CBioseq_TEST_UNUSUAL_MISC_RNA :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description
    = GetOtherComment(c_item->item_list.size(), "unexpected misc_RNA feature", "unexpected misc_RNA features")
       + " found. misc_RNAs are unusual in a genome, consider using ncRNA, misc_binding, or misc_feature as appropriate.";
};



int CBioseqTestAndRepData :: DistanceToUpstreamGap(const unsigned& pos, const CBioseq& bioseq)
{
   int offset = 0, last_gap = -1;
   ITERATE (list <CRef <CDelta_seq> >, it, bioseq.GetInst().GetExt().GetDelta().Get()) {
     if ( (*it)->IsLoc()) offset += sequence::GetCoverage((*it)->GetLoc(), thisInfo.scope);
     else if ((*it)->IsLiteral()) {
            const CSeq_literal& seq_lit = (*it)->GetLiteral();
            offset += seq_lit.GetLength();
            if (!seq_lit.CanGetSeq_data() || seq_lit.GetSeq_data().IsGap()) // IsDeltaSeqGap()
                last_gap = offset;
     }
     if (offset > (int)pos) {
       if (last_gap > -1) return ((int)pos - last_gap);
       else return -1;
     }
   }
   return -1;
};



bool CBioseq_DISC_PARTIAL_PROBLEMS :: CouldExtendLeft(const CBioseq& bioseq, const unsigned& pos)
{
  int dist;
  if (!pos) return false;
  else if (pos < 3) return true;
  else if (bioseq.GetInst().GetRepr() == CSeq_inst::eRepr_delta) {
    dist = DistanceToUpstreamGap(pos, bioseq); 
    if (dist > 0 && dist < 3) return true;
  }
  return false;
};


int CBioseqTestAndRepData :: DistanceToDownstreamGap (const int& pos, const CBioseq& bioseq)
{
  int offset = 0;

  if (pos < 0) return -1;

  ITERATE (list <CRef <CDelta_seq> >, it, bioseq.GetInst().GetExt().GetDelta().Get()) {
     if ( (*it)->IsLoc()) offset += sequence::GetCoverage((*it)->GetLoc(), thisInfo.scope);
     else if ((*it)->IsLiteral()) {
            const CSeq_literal& seq_lit = (*it)->GetLiteral();
            if ( (!seq_lit.CanGetSeq_data() || seq_lit.GetSeq_data().IsGap()) //IsDeltaSeqGap()
                     && (offset > pos))
                return (offset - pos -1);
            else offset += seq_lit.GetLength();
     }
   }
   return -1;
};



bool CBioseq_DISC_PARTIAL_PROBLEMS :: CouldExtendRight(const CBioseq& bioseq, const int& pos)
{
   int dist;
   if (pos == (int)bioseq.GetLength() -1 ) return false;
   else if (pos > (int)bioseq.GetLength() - 4) return true;
   else if (bioseq.GetInst().GetRepr() == CSeq_inst::eRepr_delta) {
     dist = DistanceToDownstreamGap(pos, bioseq);
     if (dist > 0 && dist < 3) return true;
   }
   return false;
};


void CBioseq_DISC_PARTIAL_PROBLEMS :: TestOnObj(const CBioseq& bioseq)
{
  bool partial5, partial3, partialL, partialR;
  ENa_strand strand;
  ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
    const CSeq_loc& seq_loc = (*it)->GetLocation();
    strand = seq_loc.GetStrand();
    partial5 = partial3 = false;
    partial5 = seq_loc.IsPartialStart(eExtreme_Biological);
    partial3 = seq_loc.IsPartialStop(eExtreme_Biological);
    if (strand == eNa_strand_minus) {
        partialL = partial3;
        partialR = partial5;
    }
    else {
        partialL = partial5;
        partialR = partial3;
    }
    if ( (partialL && CouldExtendLeft(bioseq, seq_loc.GetTotalRange().GetFrom()))
          || (partialR && CouldExtendRight(bioseq,(int)seq_loc.GetTotalRange().GetTo())))
        thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
  }
};


void CBioseq_DISC_PARTIAL_PROBLEMS :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description 
    = GetHasComment(c_item->item_list.size(), "feature") 
        + "partial ends that do not abut the end of the sequence or a gap, but could be extended by 3 or fewer nucleotides to do so"; 
};



// new comb

bool CBioseq_test_on_bac_partial :: IsNonExtendableLeft(const CBioseq& bioseq, const unsigned& pos)
{
  int  dist;
  if (pos < 3) return false; /* is either at the end or is within extending distance */
  else if (bioseq.GetInst().GetRepr() == CSeq_inst::eRepr_delta) {
    /* wasn't close to the sequence end, but perhaps it is close to a gap */
    dist = DistanceToUpstreamGap (pos, bioseq);
    if (dist > -1 && dist < 3) return false;
  }
  return true;
};


bool CBioseq_test_on_bac_partial :: IsNonExtendableRight(const CBioseq& bioseq, const unsigned& pos)
{
  int  dist;
  unsigned  len;

  len = bioseq.IsSetLength()? bioseq.GetLength() : 0;
  if ((int)pos > (int)(len - 4)) /* is either at the end or is within extending distance */
       return false; 
  else if (bioseq.GetInst().GetRepr() == CSeq_inst::eRepr_delta) {
    /* wasn't close to the sequence end, but perhaps it is close to a gap */
    dist = DistanceToDownstreamGap (pos, bioseq);
    if (dist > -1 && dist < 3) return false;
  }

  return true;
};



void CBioseq_test_on_bac_partial :: TestOnObj(const CBioseq& bioseq)
{
  if (thisTest.is_BacPartial_run) return;

  if (bioseq.IsAa()) return;
  string desc;
  if (bioseq_biosrc_seqdesc.empty() || !IsBioseqHasLineage(bioseq, "Eukaryota")) {
     bool partial5, partial3, partialL, partialR;
     ENa_strand strand;
     ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
       const CSeq_loc& seq_loc = (*it)->GetLocation();
       strand = seq_loc.GetStrand();
       partial5 = partial3 = false;
       partial5 = seq_loc.IsPartialStart(eExtreme_Biological);
       partial3 = seq_loc.IsPartialStop(eExtreme_Biological);
       if (!partial3 && !partial5) continue;
       if (strand == eNa_strand_minus) {
           partialL = partial3;
           partialR = partial5;
       }
       else {
           partialL = partial5;
           partialR = partial3;
       }
       if ( (partialL && IsNonExtendableLeft(bioseq, seq_loc.GetTotalRange().GetFrom()))
            || (partialR && IsNonExtendableRight(bioseq, seq_loc.GetTotalRange().GetTo()))) {
          desc = GetDiscItemText(**it);
          // DISC_BACTERIAL_PARTIAL_NONEXTENDABLE_PROBLEMS
          if ((*it)->CanGetExcept_text() && NStr::FindNoCase( 
                                 (*it)->GetExcept_text(), thisInfo.kNonExtendableException)
                                                                == string::npos)
              thisInfo.test_item_list[GetName_noexc()].push_back(desc);
          else  // DISC_BACTERIAL_PARTIAL_NONEXTENDABLE_EXCEPTION 
             thisInfo.test_item_list[GetName_exc()].push_back(desc);
       }
     }
  }

  thisTest.is_BacPartial_run = true;
};


void CBioseq_DISC_BACTERIAL_PARTIAL_NONEXTENDABLE_PROBLEMS :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetHasComment(c_item->item_list.size(), "feature")
        + "partial ends that do not abut the end of the sequence or a gap, and cannot be extended by 3 or fewer nucleotides to do so";
};


void CBioseq_DISC_BACTERIAL_PARTIAL_NONEXTENDABLE_EXCEPTION :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetHasComment(c_item->item_list.size(), "feature")
        + "partial ends that do not abut the end of the sequence or a gap, and cannot be extended by 3 or fewer nucleotides to do so, but have the correct exception";
};

// new comb



bool CBioseq_DISC_SUSPICIOUS_NOTE_TEXT :: HasSuspiciousStr(const string& str, string& sus_str)
{
  sus_str = kEmptyStr;
  ITERATE (vector <string>, it, thisInfo.suspicious_notes) {
    if (NStr::FindNoCase(str, *it) != string::npos) { sus_str = *it; return true;}
  }
  return false;
};


void CBioseq_DISC_SUSPICIOUS_NOTE_TEXT :: TestOnObj(const CBioseq& bioseq)
{
  string sus_str, sus_str2, desc;
  ITERATE (vector <const CSeq_feat*>, it, gene_feat) {
    desc = GetDiscItemText(**it);
    if ( (*it)->CanGetComment() && HasSuspiciousStr((*it)->GetComment(), sus_str))
        thisInfo.test_item_list[GetName()].push_back(sus_str + "$" + desc);
    if ( (*it)->GetData().GetGene().CanGetDesc()
                  && HasSuspiciousStr((*it)->GetData().GetGene().GetDesc(), sus_str2)
                  && sus_str != sus_str2) 
        thisInfo.test_item_list[GetName()].push_back(sus_str + "$" + desc);
  }

  ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
    if ( (*it)->CanGetComment() && HasSuspiciousStr((*it)->GetComment(), sus_str))
          thisInfo.test_item_list[GetName()].push_back(sus_str + "$" + GetDiscItemText(**it));
  }
  
  ITERATE (vector <const CSeq_feat*>, it, prot_feat) {
    if ( (*it)->GetData().GetSubtype() != CSeqFeatData::eSubtype_prot) continue; 
    // FEATDEF_PROT
    if ( (*it)->GetData().GetProt().CanGetDesc() 
                    && HasSuspiciousStr((*it)->GetData().GetProt().GetDesc(), sus_str))
          thisInfo.test_item_list[GetName()].push_back(sus_str + "$" + GetDiscItemText(**it));

  }
  
  ITERATE (vector <const CSeq_feat*>, it, miscfeat_feat) {
    if ( (*it)->CanGetComment() && HasSuspiciousStr((*it)->GetComment(), sus_str) )
          thisInfo.test_item_list[GetName()].push_back(sus_str + "$" + GetDiscItemText(**it));
  }
};



void CBioseq_DISC_SUSPICIOUS_NOTE_TEXT :: GetReport(CRef <CClickableItem>& c_item)
{
  Str2Strs sus_str2list;
  GetTestItemList(c_item->item_list, sus_str2list);
  c_item->item_list.clear();
  ITERATE (Str2Strs, it, sus_str2list) {
    AddSubcategories(c_item, GetName(), it->second, "note text", it->first, e_ContainsComment);
  }

  c_item->description 
    = GetContainsComment(c_item->item_list.size(),"note text") + "suspicious phrases";
};



void CBioseq_HYPOTHETICAL_CDS_HAVING_GENE_NAME :: TestOnObj(const CBioseq& bioseq)
{
  SAnnotSelector sel_seqfeat;
  sel_seqfeat.IncludeFeatType(CSeqFeatData::e_Prot);
  string locus;
  ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
/*
    const CGene_ref* gene = (*it)->GetGeneXref();
    if (gene && gene->IsSuppressed()) continue;
    else if (gene) {
       // look for overlap_gene = SeqMgrGetGeneByLocusTag (bsp, grp->locus_tag, &fcontext);
       // overlap_gene = SeqMgrGetFeatureByLabel (bsp, grp->locus, SEQFEAT_GENE, 0, &fcontext);
    }
    else {
         CConstRef <CSeq_feat> gene_olp= GetBestOverlappingFeat((*it)->GetLocation(),
                                                             CSeqFeatData::e_Gene,
                                                             eOverlap_Contained,
                                                             *thisInfo.scope);
         if (gene_olp.Empty() || !gene_olp->GetData().GetGene().CanGetLocus()
                 || gene_olp->GetData().GetGene().GetLocus().empty()) continue;
    }
*/
    CConstRef <CSeq_feat> gene_feat_4_feat (GetGeneForFeature(**it));
    if (gene_feat_4_feat.Empty() || !(gene_feat_4_feat->GetData().GetGene().CanGetLocus())
                || !(gene_feat_4_feat->GetData().GetGene().GetLocus().empty()) ) 
        continue;  // no gene name
    if ((*it)->CanGetProduct()) {
      CBioseq_Handle bioseq_prot = GetBioseqFromSeqLoc((*it)->GetProduct(),*thisInfo.scope);
      CFeat_CI feat_it(bioseq_prot, sel_seqfeat);
      if (!feat_it) continue;
      const CProt_ref& prot_ref = feat_it->GetOriginalFeature().GetData().GetProt();
      if (prot_ref.CanGetName()) {
          ITERATE (list <string>, jt, prot_ref.GetName()) {
            if (NStr::FindNoCase(*jt, "hypothetical protein") != string::npos)
                thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
          }
      }
    }
  }
};


void CBioseq_HYPOTHETICAL_CDS_HAVING_GENE_NAME :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
          = GetHasComment(c_item->item_list.size(), "hypothetical coding region")
                              + "a gene name";
};



void CBioseqTestAndRepData :: TestOverlapping_ed_Feats(const vector <const CSeq_feat*>& feat, const string& setting_name, bool isGene, bool isOverlapped)
{
   string overlapping_ed_feats("|");
   unsigned i, j;
   for (i=0; (int)i< (int)(feat.size()-1); i++) {
      if (isOverlapped && !IsUnknown(overlapping_ed_feats, i)) continue;
      for (j=i+1; j< feat.size(); j++) {
           if (!isOverlapped && !IsUnknown(overlapping_ed_feats, j)) continue;
           const CSeq_loc& loc_i = feat[i]->GetLocation();
           const CSeq_loc& loc_j = feat[j]->GetLocation();

           // all feats more distant than the loc_j will have no chance of overlapping
           if (loc_i.GetStop(eExtreme_Positional) < loc_j.GetStart(eExtreme_Positional))
               break;

           if (isGene && loc_i.GetStrand() != loc_j.GetStrand()) continue;
           sequence::ECompare ovlp = Compare(loc_i, loc_j, thisInfo.scope);
           if (isOverlapped) {
               if (ovlp == sequence::eContained || ovlp == sequence::eSame) {
                  thisInfo.test_item_list[setting_name].push_back(
                                                            GetDiscItemText(*feat[i]));
                  overlapping_ed_feats += NStr::UIntToString(i) + "|";
                  break;
               }
               else if (ovlp == sequence::eContains 
                                          && IsUnknown(overlapping_ed_feats, j)) {
                    thisInfo.test_item_list[setting_name].push_back(
                                                            GetDiscItemText(*feat[j]));
                    overlapping_ed_feats += NStr::UIntToString(j) + "|";
               }
           }
           else if (ovlp != sequence :: eNoOverlap) {
               if (IsUnknown(overlapping_ed_feats, i)) {
                  thisInfo.test_item_list[setting_name].push_back(
                                                          GetDiscItemText(*feat[i]));
                  overlapping_ed_feats += NStr::UIntToString(i) + "|";
               }
               thisInfo.test_item_list[setting_name].push_back(
                                                            GetDiscItemText(*feat[j]));
               overlapping_ed_feats += NStr::UIntToString(j) + "|";
           }
      }
   }
};



void CBioseq_TEST_OVERLAPPING_RRNAS :: TestOnObj(const CBioseq& bioseq)
{
    TestOverlapping_ed_Feats(rrna_feat, GetName(), false, false);
};



void CBioseq_TEST_OVERLAPPING_RRNAS :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description 
     = GetOtherComment(c_item->item_list.size(), "rRNA feature overlaps", "rRNA features overlap")
      + " another rRNA feature.";
};


void CBioseq_OVERLAPPING_GENES :: TestOnObj(const CBioseq& bioseq)
{
    TestOverlapping_ed_Feats(gene_feat, GetName(), true, false);
};



void CBioseq_OVERLAPPING_GENES :: GetReport(CRef <CClickableItem>& c_item)
{
    c_item->description 
      = GetOtherComment(c_item->item_list.size(), "gene overlaps", "genes overlap")
                           + " another gene on the same strand.";
};


void CBioseq_FIND_OVERLAPPED_GENES :: TestOnObj(const CBioseq& bioseq)
{
    TestOverlapping_ed_Feats(gene_feat, GetName());
};



void CBioseq_FIND_OVERLAPPED_GENES :: GetReport(CRef <CClickableItem>& c_item)
{
    c_item->description = GetOtherComment(c_item->item_list.size(), "gene", "genes")
                           + " completely overlapped by other genes";
};


void CBioseq_TEST_LOW_QUALITY_REGION :: TestOnObj(const CBioseq& bioseq)
{
    int start = -1;
    float min_pct = 0.25;
    double min_length = 30;
    unsigned i=0, num_ns, len;
    bool found_interval = false;
    CBioseq_Handle bioseq_h = thisInfo.scope->GetBioseqHandle(bioseq);
    CSeqVector seq_vec = bioseq_h.GetSeqVector(CBioseq_Handle::eCoding_Iupac);
    for (CSeqVector_CI it =seq_vec.begin(); it && !found_interval; ++it, i++) {
      if ( it.IsInGap()) continue;

      if ( (*it) != 'A' && (*it) != 'T' && (*it) != 'G' && (*it) != 'C') {
         if (start == -1) { /* start new interval if we aren't already in one */
             start = i;
             num_ns = 1;
         } 
         else num_ns++;   /* add to number of ns in this interval */
      } 
      else {
         if (start > -1) { /* if we are already in an interval, see if we should continue to be */
           len = i - start;
           if ( ((float)num_ns / (float)len) < min_pct) {
              /* is the interval long enough to qualify? */
              if (len >= min_length) {
                 thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(bioseq));
                 found_interval = true;
              }
              else { /* reset for next interval */
                 start = -1;
                 num_ns = 0;
              }
           }
         }
      }
    }
    
    // last interval
    len = i-start;
    if (!found_interval && start > -1 && len >= min_length)
         thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(bioseq));
};


void CBioseq_TEST_LOW_QUALITY_REGION :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description 
      = GetContainsComment(c_item->item_list.size(), "sequence") + "low quality region";
};



bool CBioseq_TEST_BAD_GENE_NAME :: GeneNameHas4Numbers(const string& locus)
{
  bool num_digits=0;
  for (unsigned i=0; i< locus.size() && num_digits < 4; i++)
    if (isdigit(locus[i])) num_digits++;
    else num_digits = 0;

  if (num_digits >=4) return true;
  else return false; 
};


void CBioseq_TEST_BAD_GENE_NAME :: TestOnObj(const CBioseq& bioseq)
{
  string desc, locus;
  bool is_euka;
  ITERATE (vector <const CSeq_feat*>, it, gene_feat) {  
    const CSeqFeatData& seq_feat_dt = (*it)->GetData();
    if (seq_feat_dt.GetGene().CanGetLocus()) {
      locus = seq_feat_dt.GetGene().GetLocus();
      if (!locus.empty()) {
        desc = GetDiscItemText(**it); 
        if (locus.size() > 10)
           thisInfo.test_item_list[GetName()].push_back("more than 10 characters$" + desc);
        ITERATE (vector <string>, jt, thisInfo.bad_gene_names_contained)
          if (NStr::FindNoCase(locus, *jt) != string::npos)
              thisInfo.test_item_list[GetName()].push_back(*jt + "$" + desc);
        if (GeneNameHas4Numbers(locus))
           thisInfo.test_item_list[GetName()].push_back("4 or more consecutive numbers$"+desc);
        is_euka = false;
 
        // bad_bacterial_gene_name
        ITERATE (vector <const CSeqdesc*>, jt, bioseq_biosrc_seqdesc)
          if (HasLineage((*jt)->GetSource(), "Eukaryota")) {is_euka = true; break;}
        if (!is_euka && (!isalpha(locus[0]) || !islower(locus[0]))) 
           thisInfo.test_item_list[GetName()].push_back("bad$" + desc);      
      }
    }
  }
};


void CBioseq_TEST_BAD_GENE_NAME :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->item_list.clear();
  Str2Strs bad_name_genes;
  vector <string> bad_bacterial, rep_arr;
  ITERATE (vector <string>, it, thisInfo.test_item_list[GetName()]) {
    rep_arr = NStr::Tokenize(*it, "$", rep_arr);
    if (rep_arr[0] == "bad") c_item->item_list.push_back(*it);
    else bad_name_genes[rep_arr[0]].push_back(rep_arr[1]);
    rep_arr.clear();
  }

  bool has_bad_bacterial = false;
  if (!c_item->item_list.empty()) {
    c_item->setting_name = GetName_bad();
    has_bad_bacterial = true;
    if (!bad_name_genes.empty()) 
         c_item = CRef <CClickableItem> (new CClickableItem);
  } 
  if (!bad_name_genes.empty()) {
    c_item->setting_name = GetName();
  
    if (bad_name_genes.size() == 1) {
       c_item->item_list = bad_name_genes.begin()->second;
       c_item->description
         =GetContainsComment(c_item->item_list.size(), "gene") + bad_name_genes.begin()->first;
    }
    else {
      ITERATE (Str2Strs, it, bad_name_genes) {
        CRef <CClickableItem> c_sub (new CClickableItem);
        c_sub->setting_name = c_item->setting_name;
        c_sub->item_list = it->second;
        c_sub->description = GetContainsComment(c_sub->item_list.size(),"gene") + it->first;
        c_item->subcategories.push_back(c_sub);
        c_item->item_list.insert(c_item->item_list.end(), c_sub->item_list.begin(), 
                                                                      c_sub->item_list.end());
      }
      c_item->description
        = GetContainsComment(c_item->item_list.size(),"gene") + "suspect phrase or characters";
    }
    if (has_bad_bacterial) thisInfo.disc_report_data.push_back(c_item);
  }
};


/*

void CBioseq_DISC_SHORT_RRNA :: TestOnObj(const CBioseq& bioseq)
{
  string rrna_name;
  unsigned len;
  ITERATE (vector <const CSeq_feat*>, it, rrna_feat) {
    if ((*it)->CanGetPartial() && (*it)->GetPartial()) continue; 
    len = sequence::GetCoverage((*it)->GetLocation(), thisInfo.scope);
    rrna_name = GetRNAProductString(**it);
    ITERATE (Str2UInt, jt, thisInfo.rRNATerms) {
      if (NStr::FindNoCase(rrna_name, jt->first) != string::npos && len < jt->second) {
           thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
           break;
      }
    }
  }
};



void CBioseq_DISC_SHORT_RRNA :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
      = GetIsComment(c_item->item_list.size(), "rRNA feature") + "too short.";
};

*/


bool CBioseqTestAndRepData :: StrandOk(ENa_strand strand1, ENa_strand strand2)
{
  if (strand1 == eNa_strand_minus && strand2 != eNa_strand_minus) return false;
  else if (strand1 != eNa_strand_minus && strand2 == eNa_strand_minus) return false;
  else return true;
};



bool CBioseq_DISC_BAD_GENE_STRAND :: AreIntervalStrandsOk(const CSeq_loc& g_loc, const CSeq_loc& f_loc) 
{
  bool   found_match;
  bool   found_bad = FALSE;
  ENa_strand     feat_strand, gene_strand;

  for (CSeq_loc_CI f_loc_it(f_loc); f_loc_it && !found_bad; ++ f_loc_it) {
     found_match = false;
     for (CSeq_loc_CI g_loc_it(g_loc); g_loc_it && !found_match; ++ g_loc_it) {
        sequence::ECompare ovlp = Compare(f_loc_it.GetEmbeddingSeq_loc(), 
                                        g_loc_it.GetEmbeddingSeq_loc(), thisInfo.scope);
        if (ovlp == sequence::eContained || ovlp == sequence::eSame) {
          found_match = true;
          feat_strand = f_loc_it.GetStrand();
          gene_strand = g_loc_it.GetStrand();
          if (!StrandOk(feat_strand, gene_strand)) found_bad = true;
        }
     }
  }

  return !found_bad;
};



void CBioseq_DISC_BAD_GENE_STRAND :: TestOnObj(const CBioseq& bioseq)
{
  if (bioseq.IsAa()) return;
  bool is_error, g_mixed_strand;

  unsigned f_left, f_right, g_left, g_right;
  ITERATE (vector <const CSeq_feat*>, it, gene_feat) {
    const CSeq_loc& g_loc = (*it)->GetLocation();
    ITERATE (vector <const CSeq_feat*>, jt, all_feat) {
       const CSeq_loc& f_loc = (*jt)->GetLocation();
       const CSeqFeatData& seq_feat_dt = (*jt)->GetData();
       if ( seq_feat_dt.IsGene() 
              || seq_feat_dt.GetSubtype() == CSeqFeatData::eSubtype_primer_bind) continue;
       f_left = f_loc.GetStart(eExtreme_Positional);
       f_right = f_loc.GetStop(eExtreme_Positional);
       g_left = g_loc.GetStart(eExtreme_Positional);
       g_right = g_loc.GetStop(eExtreme_Positional);
       if (f_left == g_left || f_right == g_right) {
          g_mixed_strand = is_error = false;
          if (g_loc.IsMix()) {
             bool has_strand_plus = false, has_strand_minus = false;
             ITERATE (list <CRef <CSeq_loc> >, it, g_loc.GetMix().Get()) {
                if ( (*it)->GetStrand() == eNa_strand_plus) has_strand_plus = true;
                else if ( (*it)->GetStrand() == eNa_strand_minus) has_strand_minus = true;
                if (has_strand_plus && has_strand_minus) break; 
             }
             if (has_strand_plus && has_strand_minus) g_mixed_strand = true;
          }
          if (g_mixed_strand) is_error = !AreIntervalStrandsOk(g_loc, f_loc);
          else if (!StrandOk(f_loc.GetStrand(), g_loc.GetStrand())) is_error = true;
          if (is_error)
            thisInfo.test_item_list[GetName()].push_back(
                                         GetDiscItemText(**it)+"$" + GetDiscItemText(**jt));
       } 
       if (f_left > g_right) break;
    }
  }
};


void CBioseq_DISC_BAD_GENE_STRAND :: GetReport(CRef <CClickableItem>& c_item)
{
   vector <string> rep_arr;
   c_item->item_list.clear();
   ITERATE (vector <string>, it, thisInfo.test_item_list[GetName()]) {
     rep_arr = NStr::Tokenize(*it, "$", rep_arr);
     CRef <CClickableItem> c_sub (new CClickableItem);
     c_sub->setting_name = GetName();
     c_sub->item_list.insert(c_sub->item_list.end(), rep_arr.begin(), rep_arr.end());
     c_sub->description = "Gene and feature strands conflict";
     c_item->subcategories.push_back(c_sub); 
     c_item->item_list.insert(c_item->item_list.end(), rep_arr.begin(), rep_arr.end());
     rep_arr.clear(); 
   }

   c_item->description 
      = GetOtherComment(c_item->item_list.size()/2, 
                           "feature location conflicts", "feature locations conflict") 
          + "with gene location strands";
};



bool CBioseq_DISC_SHORT_INTRON :: PosIsAt5End(unsigned pos, CConstRef <CBioseq>& bioseq)
{
  unsigned seq_end = 0;

  if (!pos) return true;
  else {
    const CSeq_inst& seq_inst = bioseq->GetInst();
    if (seq_inst.GetRepr() != CSeq_inst::eRepr_seg) return false;
    else {
      if (seq_inst.CanGetExt()) {
         const CSeq_ext& seq_ext = seq_inst.GetExt();
         if (seq_ext.Which() != CSeq_ext :: e_Seg) return false;
         else {
           ITERATE (list <CRef <CSeq_loc> >, it, seq_ext.GetSeg().Get()) {
             seq_end += sequence::GetCoverage(**it, thisInfo.scope);
             if (pos == seq_end) return true;
           }
        }
      }
    }
  }
  return false;
};



bool CBioseq_DISC_SHORT_INTRON :: PosIsAt3End(const unsigned pos, CConstRef <CBioseq>& bioseq)
{
  unsigned seq_end = 0;

  if (pos == bioseq->GetLength() - 1) return true;
  else {
   const CSeq_inst& seq_inst = bioseq->GetInst();
   if (seq_inst.GetRepr() != CSeq_inst::eRepr_seg) return false;
   else {
     if (seq_inst.CanGetExt()) {
        const CSeq_ext& seq_ext = seq_inst.GetExt();
        if (seq_ext.Which() != CSeq_ext :: e_Seg) return false;
        else {
          ITERATE (list <CRef <CSeq_loc> >, it, seq_ext.GetSeg().Get()) {
            seq_end += sequence::GetCoverage(**it, thisInfo.scope);
            if (pos == seq_end -1) return true;
          }
        }
    }
   }
  }
  return false;
};



void CBioseq_DISC_SHORT_INTRON :: TestOnObj(const CBioseq& bioseq)
{
  ENa_strand strand;
  bool partial5, partial3, excpt;
  unsigned start, stop;
  string desc_txt;
  ITERATE (vector <const CSeq_feat*>, it, intron_feat) {
    if (IsPseudoSeqFeatOrXrefGene(*it)) continue;
    const CSeq_loc& seq_loc = (*it)->GetLocation();
    if (sequence::GetCoverage(seq_loc, thisInfo.scope) >= 11) continue;
    strand = seq_loc.GetStrand();
    partial5 = partial3 = false;
    start = seq_loc.GetTotalRange().GetFrom();
    stop = seq_loc.GetTotalRange().GetTo();
    partial5 = seq_loc.IsPartialStart(eExtreme_Biological);
    partial3 = seq_loc.IsPartialStop(eExtreme_Biological);
    CConstRef <CBioseq> 
         bioseq_new = GetBioseqFromSeqLoc(seq_loc, *thisInfo.scope).GetCompleteBioseq();
    if ( (*it)->CanGetExcept() && (*it)->GetExcept()) excpt = true;
    else excpt = false;
    desc_txt = (excpt)? GetDiscItemText(**it) : "except$" + GetDiscItemText(**it);      
    if (bioseq_new.Empty()) thisInfo.test_item_list[GetName()].push_back(desc_txt);
    else if ( (partial5 && strand!=eNa_strand_minus && PosIsAt5End(start, bioseq_new))
             || (partial3 && strand==eNa_strand_minus && PosIsAt5End(stop, bioseq_new) )
             || (partial5 && strand==eNa_strand_minus && PosIsAt3End(start, bioseq_new))
             || (partial3 && strand!=eNa_strand_minus && PosIsAt3End(stop, bioseq_new)) )
           continue;
    else thisInfo.test_item_list[GetName()].push_back(desc_txt);
  }  

  bool found_short;
  unsigned last_start, last_stop;
  ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
    found_short = false;
    if ( (*it)->CanGetExcept() && (*it)->GetExcept() ) continue;
    CSeq_loc_CI loc_it( (*it)->GetLocation() );
    last_start = loc_it.GetRange().GetFrom();
    last_stop = loc_it.GetRange().GetTo();
    for (++loc_it; loc_it; ++loc_it) {
      start = loc_it.GetRange().GetFrom();
      stop = loc_it.GetRange().GetTo();
      if (ABS((int)(start - last_stop)) < 11 || ABS ((int)(stop - last_start)) < 11) {
               found_short = true; break;
      }
      last_start = start;
      last_stop = stop; 
    }
    if (found_short) 
         thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
  }
};



void CBioseq_DISC_SHORT_INTRON :: GetReport(CRef <CClickableItem>& c_item)
{
  vector <string> rep_arr;
  vector <string> with_exception;
  bool any_no_exception = false;
  c_item->item_list.clear();
  ITERATE (vector <string>, it, thisInfo.test_item_list[GetName()]) {
    rep_arr = NStr::Tokenize(*it, "$", rep_arr);
    if (rep_arr.size() == 1) {
          any_no_exception = true;
          c_item->item_list.push_back(*it);
    }
    else {
      with_exception.push_back(rep_arr[1]);
      c_item->item_list.push_back(rep_arr[1]);
    }
    rep_arr.clear(); 
  }

  if (any_no_exception) {
     c_item->description
         = GetIsComment(c_item->item_list.size(), "intron") + "shorter than 11 nt.";
     if (!with_exception.empty()) {
       CRef <CClickableItem> c_sub (new CClickableItem);
       c_sub->setting_name = GetName();
       c_sub->item_list = with_exception;
       c_sub->description = GetIsComment(c_sub->item_list.size(), "intron")
                                       + "shorter than 11 nt and have an exception";
       c_item->subcategories.push_back(c_sub);
     }
  }
  else {
    c_item->description= GetIsComment(c_item->item_list.size(), "intron")
                                       + "shorter than 11 nt and have an exception";
  }
};



bool CBioseqTestAndRepData :: IsDeltaSeqWithFarpointers(const CBioseq& bioseq)
{
  bool rval = false;

  if (bioseq.GetInst().GetRepr() != CSeq_inst :: eRepr_delta) return false;
  if (bioseq.GetInst().CanGetExt()) {
    ITERATE (list <CRef <CDelta_seq> >, it, bioseq.GetInst().GetExt().GetDelta().Get()) 
       if ( (*it)->IsLoc()) {rval = true; break;}
  }
  return rval;
};




void CBioseqTestAndRepData :: TestOnBasesN(const CBioseq& bioseq)
{
    if (IsDeltaSeqWithFarpointers (bioseq)) return;
   
    unsigned cnt_a, cnt_t, cnt_g, cnt_c, cnt_n, tot_n, cnt_non_nt;
    cnt_a = cnt_t = cnt_g = cnt_c = cnt_n = tot_n = cnt_non_nt = 0;
    unsigned start_n, i=0;
    string n10_intvls(kEmptyStr), n14_intvls(kEmptyStr);
    CBioseq_Handle bioseq_h = thisInfo.scope->GetBioseqHandle(bioseq);
    CSeqVector seq_vec = bioseq_h.GetSeqVector(CBioseq_Handle::eCoding_Iupac);
    for (CSeqVector_CI it =seq_vec.begin(); it; ++it, i++) {
// if ( it.IsInGap()) cerr << "is in gap " << (*it) << endl;// gaps represented as 'N's
      if ( (*it) == 'N') {
        if (!cnt_n) start_n = i+1;
        cnt_n++;
        tot_n++;
      }
      else {
         if (cnt_n >= 10) 
            n10_intvls += ", " + NStr::UIntToString(start_n) + "-" + NStr::UIntToString(i);
         if (cnt_n >= 14) 
            n14_intvls += ", " + NStr::UIntToString(start_n) + "-" + NStr::UIntToString(i);
         cnt_n = 0;
         if (!cnt_a && (*it) == 'A') cnt_a++;
         else if (!cnt_t && (*it) == 'T') cnt_t++;
         else if (!cnt_c && (*it) == 'C') cnt_c++;
         else if (!cnt_g && (*it) == 'G') cnt_g++;
         else if ( (*it) != 'A' && (*it) != 'T' && (*it) != 'C' && (*it) != 'G') cnt_non_nt++;
      }
    }
    // last count
    if (cnt_n >= 10)   
          n10_intvls += ", " + NStr::UIntToString(start_n) + "-" + NStr::UIntToString(i);
    if (cnt_n >= 14)
          n14_intvls += ", " + NStr::UIntToString(start_n) + "-" + NStr::UIntToString(i);
   
    const CSeq_id& new_seq_id =
                       BioseqToBestSeqId(*(bioseq_h.GetCompleteBioseq()), CSeq_id::e_Genbank);
    string id_str = new_seq_id.AsFastaString();
    id_str = id_str.substr(id_str.find("|")+1);

    string desc = GetDiscItemText(bioseq);
    if (!n10_intvls.empty())  
       thisInfo.test_item_list[GetName_n10()].push_back(
                                   desc + "$" + id_str + "#" + n10_intvls.substr(2));
    if (!n14_intvls.empty())   // just for test, should be in the TSA report          
       thisInfo.test_item_list[GetName_n14()].push_back(
                                  desc + "$" + id_str + "#" + n14_intvls.substr(2));
    if (!cnt_a) thisInfo.test_item_list[GetName_0()].push_back("A$" + desc);
    if (!cnt_t) thisInfo.test_item_list[GetName_0()].push_back("T$" + desc);
    if (!cnt_c) thisInfo.test_item_list[GetName_0()].push_back("C$" + desc);
    if (!cnt_g) thisInfo.test_item_list[GetName_0()].push_back("G$" + desc);
    if (tot_n && (float)tot_n/(float)bioseq.GetLength() > 0.05)
          thisInfo.test_item_list[GetName_5perc()].push_back(desc);
    if (tot_n && (float)tot_n/(float)bioseq.GetLength() > 0.10)
          thisInfo.test_item_list[GetName_10perc()].push_back(desc);
    if (cnt_non_nt) thisInfo.test_item_list[GetName_non_nt()].push_back(desc);

    thisTest.is_BASES_N_run = true;
};


void CBioseqTestAndRepData :: AddNsReport(CRef <CClickableItem>& c_item, bool is_n10)
{
   Str2Strs desc2intvls;
   GetTestItemList(c_item->item_list, desc2intvls);
   c_item->item_list.clear();

   vector <string> rep_dt;
   ITERATE (Str2Strs, it, desc2intvls) {
     c_item->item_list.push_back(it->first);
     CRef <CClickableItem> c_sub (new CClickableItem);
     c_sub->setting_name = GetName();
     c_sub->item_list.push_back(it->first);
     rep_dt = NStr::Tokenize(it->second[0], "#", rep_dt);
     c_sub->description =
               rep_dt[0] + " has runs of Ns at the following locations:\n" + rep_dt[1];
     c_item->subcategories.push_back(c_sub);
     rep_dt.clear();
   }
   c_item->description = GetHasComment(c_item->item_list.size(), "sequence") 
                           + "runs of " + (is_n10 ? "10":"14") + " or more Ns.";
};



void CBioseq_N_RUNS :: TestOnObj(const CBioseq& bioseq)
{
   if (!thisTest.is_BASES_N_run) TestOnBasesN(bioseq);
};
 

void CBioseq_N_RUNS :: GetReport(CRef <CClickableItem>& c_item) 
{
      AddNsReport(c_item);
};


void CBioseq_N_RUNS_14 :: TestOnObj(const CBioseq& bioseq)
{
   if (!thisTest.is_BASES_N_run) TestOnBasesN(bioseq);
};


void CBioseq_N_RUNS_14 :: GetReport(CRef <CClickableItem>& c_item)
{
     AddNsReport(c_item, false);
};


void CBioseq_ZERO_BASECOUNT :: TestOnObj(const CBioseq& bioseq)
{
   if (!thisTest.is_BASES_N_run) TestOnBasesN(bioseq);
};


void CBioseq_ZERO_BASECOUNT :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs bases2list;
   GetTestItemList(c_item->item_list, bases2list);
   c_item->item_list.clear();

   string base_tp;
   ITERATE (Str2Strs, it, bases2list) {
     CRef <CClickableItem> c_sub (new CClickableItem);
     c_sub->setting_name = GetName();
     c_sub->item_list = it->second;
     c_sub->description 
        = GetHasComment(c_sub->item_list.size(), "sequence") + "no " + it->first + "s.";
     c_item->subcategories.push_back(c_sub);
     c_item->item_list.insert(c_item->item_list.end(), it->second.begin(), it->second.end());
   }
};


void CBioseq_DISC_PERCENT_N :: TestOnObj(const CBioseq& bioseq)
{
   if (!thisTest.is_BASES_N_run) TestOnBasesN(bioseq);
};



void CBioseq_DISC_PERCENT_N :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetHasComment(c_item->item_list.size(), "sequence") + "> 5% Ns.";
};



void CBioseq_DISC_10_PERCENTN :: TestOnObj(const CBioseq& bioseq)
{
   if (!thisTest.is_BASES_N_run) TestOnBasesN(bioseq);
};


void CBioseq_DISC_10_PERCENTN :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetHasComment(c_item->item_list.size(), "sequence") + "> 10% Ns.";
};


void CBioseq_TEST_UNUSUAL_NT :: TestOnObj(const CBioseq& bioseq)
{
   if (!thisTest.is_BASES_N_run) TestOnBasesN(bioseq);
};


void CBioseq_TEST_UNUSUAL_NT :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetContainsComment(c_item->item_list.size(), "sequence")
                          + " nucleotides that are not ATCG or N.";
};



void CBioseq_RNA_NO_PRODUCT :: TestOnObj(const CBioseq& bioseq)
{
  string pseudo;
  ITERATE (vector <const CSeq_feat*>, it, rna_feat) {
     CSeqFeatData :: ESubtype subtype = (*it)->GetData().GetSubtype();
     if (IsPseudoSeqFeatOrXrefGene(*it)) pseudo = "pseudo";
     else pseudo = "not pseudo";
     if (subtype == CSeqFeatData :: eSubtype_otherRNA) {
        if ((*it)->CanGetComment()) {
          strtmp = (*it)->GetComment();
          if ((NStr::FindNoCase(strtmp.substr(0, 9), "contains ") != string::npos)
              || (NStr::FindNoCase(strtmp.substr(0, 11), "may contain") != string::npos))
             continue;
        }
     }
     else if (subtype == CSeqFeatData :: eSubtype_tmRNA) continue;
     
     if (GetRNAProductString(**it).empty())
        thisInfo.test_item_list[GetName()].push_back(pseudo + "$" +GetDiscItemText(**it));
  }
};


void CBioseq_RNA_NO_PRODUCT :: GetReport(CRef <CClickableItem>& c_item)
{
  Str2Strs sub_ls;
  vector <string> rep_arr;
  string setting_name = GetName();
  ITERATE (vector <string>, it, thisInfo.test_item_list[setting_name]) {
    rep_arr = NStr::Tokenize(*it, "$", rep_arr);
    sub_ls[rep_arr[0]].push_back(rep_arr[1]); 
    rep_arr.clear();
  }

  unsigned cnt;
  if (sub_ls.size() == 1) {
    c_item->item_list.clear();
    c_item->item_list = sub_ls.begin()->second;
    cnt = c_item->item_list.size();
    c_item->description = GetHasComment(cnt, "RNA feature") 
           + "no product and " + ( (cnt>1) ? "are " : "is ") + sub_ls.begin()->first + ".";
  }
  else {
    ITERATE (Str2Strs, it, sub_ls) {
      CRef <CClickableItem> c_sub (new CClickableItem);
      c_sub->setting_name = setting_name;
      c_sub->item_list = it->second;
      cnt = c_sub->item_list.size();
      c_sub->description = GetHasComment(cnt, "RNA feature") 
             + "no product and " + ( (cnt >1) ? "are " : "is ") + it->first + "."; 
      c_item->subcategories.push_back(c_sub);
    }
  }
};


// new comb
bool CBioseq_test_on_protfeat :: EndsWithPattern(const string& pattern, const list <string>& strs)
{
  unsigned len_patt = pattern.size(), len_str;
  ITERATE (list <string>, it, strs) {
     len_str = (*it).size(); 
     if ( len_str >= len_patt && (*it).substr(len_str - len_patt) == pattern) return true;
  }
  return false;
};


bool CBioseq_test_on_protfeat :: ContainsPseudo(const string& pattern, const list <string>& strs)
{
  bool right_pseudo;
  size_t pos;
  ITERATE (list <string>, it, strs) {
    strtmp = *it;
    while ( !strtmp.empty() && (pos = strtmp.find("pseudo")) != string::npos) {
        right_pseudo = true;
        ITERATE (vector <string>, jt, thisInfo.s_pseudoweasels) {
           if ( strtmp.substr(pos, (*jt).size()) == *jt) {
               right_pseudo = false;
               strtmp = strtmp.substr(pos + (*jt).size());      
               break;
           }
        }
        if (right_pseudo) return true;
    }  
  };
  return false;
};


bool CBioseq_test_on_protfeat :: ContainsWholeWord(const string& pattern, const list <string>& strs)
{
   ITERATE (list <string>, it, strs) 
      if (DoesStringContainPhrase(*it, pattern, false, true)) return true;
   return false;
};


void CBioseq_test_on_protfeat :: TestOnObj(const CBioseq& bioseq) 
{
   if (thisTest.is_ProtFeat_run) return;

   string desc, head_desc, func;
   ITERATE (vector <const CSeq_feat*>, it, prot_feat) {
     const CProt_ref& prot = (*it)->GetData().GetProt();
     const list <string>& names = prot.GetName();
     desc = GetDiscItemText(**it);

     // EC_NUMBER_ON_UNKNOWN_PROTEIN
     if (prot.CanGetName() && prot.CanGetEc() && !prot.GetEc().empty()) {
        strtmp = *(names.begin());
        if ((NStr::FindNoCase(strtmp, "hypothetical protein") != string::npos)
                 || (NStr::FindNoCase(strtmp, "unknown protein") != string::npos))
             thisInfo.test_item_list[GetName_ec()].push_back(desc);
     }

     // DISC_CDS_PRODUCT_FIND
     if ( (*it)->GetData().GetSubtype() == CSeqFeatData::eSubtype_prot) {
          const CSeq_feat* 
             cds =GetCDSForProduct( GetBioseqFromSeqLoc((*it)->GetLocation(),*thisInfo.scope));
          if (cds) desc = GetDiscItemText(*cds);
     }
     ITERATE (Str2Str, jt, thisInfo.cds_prod_find) {
       func = jt->second;
       head_desc = kEmptyStr;
       if (func == "EndsWithPattern") {
         if (EndsWithPattern(jt->first, names)) head_desc = "end# with " + jt->first;
       }
       else if (func == "ContainsPseudo") {
         if (ContainsPseudo(jt->first,names)) head_desc = "contain# '" + jt->first + "'";
       }
       else if (func == "ContainsWholeWord") {
         if ( ContainsWholeWord(jt->first, names) ) 
                  head_desc = "contain# '" + jt->first + "'";
       }
       if (!head_desc.empty())
         thisInfo.test_item_list[GetName_cds()].push_back(head_desc + "$" + desc);
     }

     // DISC_PROTEIN_NAMES
     if ( (*it)->GetData().GetSubtype() == CSeqFeatData::eSubtype_prot) { // FEATDEF_PROT
        if ( prot.CanGetName() && !prot.GetName().empty()) {
           thisInfo.test_item_list[GetName_pnm()].push_back(
               *(prot.GetName().begin()) + "$" + desc);
        }
     }
   }

   thisTest.is_ProtFeat_run = true;
};

void CBioseq_DISC_PROTEIN_NAMES :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs pnm2feats;
   GetTestItemList(c_item->item_list, pnm2feats);
   unsigned min_num = c_item->item_list.size(), cnt=0;
   min_num = min_num > 100 ? 100 : min_num;
   c_item->item_list.clear();
   
   ITERATE (Str2Strs, it, pnm2feats) {
     if (it->second.size() < min_num) continue;  // erase?
     cnt++;
   } 
   ITERATE (Str2Strs, it, pnm2feats) {
     if (it->second.size() < min_num) continue;
     if (cnt >1 ) {
        AddSubcategories(c_item, GetName(), it->second, 
                      "protein", "name '" + it->first + "'.", e_HasComment, false);
     }
     else {
         c_item->item_list = it->second;
         c_item->description 
            = GetHasComment(it->second.size(), "protein") + "name '" + it->first + "'.";
     }
   }
   if (cnt > 1) c_item->description = "Many proteins have the same name";
};

void CBioseq_DISC_CDS_PRODUCT_FIND :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs cd2feats;
   GetTestItemList(c_item->item_list, cd2feats);
   c_item->item_list.size();
   string desc1, desc2;
   vector <string> arr;
   ITERATE (Str2Strs, it, cd2feats) {
      arr.clear();
      arr = NStr::Tokenize(it->first, "#", arr);
      AddSubcategories(c_item, GetName(), it->second, "coding region product " + arr[0] + "s", 
         "coding region products " + arr[0], e_OtherComment, true, arr[1]); 
   } 
   c_item->description = GetContainsComment(c_item->item_list.size(), "coding region product")
                          + " suspect phrase or characters";
};


/*
void CBioseq_EC_NUMBER_ON_UNKNOWN_PROTEIN :: TestOnObj(const CBioseq& bioseq)
{
   ITERATE (vector <const CSeq_feat*>, it, prot_feat) {
     const CProt_ref& prot = (*it)->GetData().GetProt();
     if (prot.CanGetName() && prot.CanGetEc() && !prot.GetEc().empty()) {
        const list <string>& names = prot.GetName();
        strtmp = *(names.begin());
        if ((NStr::FindNoCase(strtmp, "hypothetical protein") != string::npos)
                 || (NStr::FindNoCase(strtmp, "unknown protein") != string::npos))
             thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
     }
   }
};
*/


void CBioseq_EC_NUMBER_ON_UNKNOWN_PROTEIN :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description 
     = GetHasComment(c_item->item_list.size(), "protein feature")
       + "an EC number and a protein name of 'unknown protein' or 'hypothetical protein'";
};

// new comb


bool CBioseq_FEATURE_LOCATION_CONFLICT :: IsMixStrand(const CSeq_feat* seq_feat)
{
  CSeq_loc_CI loc_it(seq_feat->GetLocation());
  unsigned cnt = 0;
  for (CSeq_loc_CI it = loc_it; it; ++it, cnt++);
  bool same_strand = true;
  if (cnt > 1) {
    ENa_strand pre_strand, this_strand;
    pre_strand = loc_it.GetStrand();
    for (CSeq_loc_CI it = ++loc_it; it && same_strand; ++it) {
       this_strand = it.GetStrand();
       if (this_strand != pre_strand) same_strand = false;
    }
  }
  return (!same_strand);  
};


bool CBioseq_FEATURE_LOCATION_CONFLICT :: StrandOk(const int& strand1, const int& strand2)
{
  if (strand1 == (int)eNa_strand_minus && strand2 != (int)eNa_strand_minus)
       return false;
  else if (strand1 != (int)eNa_strand_minus && strand2 == eNa_strand_minus) return false;
  else return true;
};



bool CBioseq_FEATURE_LOCATION_CONFLICT :: IsMixedStrandGeneLocationOk(const CSeq_loc& feat_loc, const CSeq_loc& gene_loc)
{
  CSeq_loc_CI f_loc_it(feat_loc);
  CSeq_loc_CI g_loc_it(gene_loc);
  int gene_start, gene_stop, feat_start, feat_stop;

  ENa_strand gene_strand, feat_strand;
  while (f_loc_it && g_loc_it) {
    gene_strand = g_loc_it.GetStrand();
    feat_strand = f_loc_it.GetStrand();

    if (!StrandOk(gene_strand, feat_strand)) return false;
    
    gene_start = g_loc_it.GetRange().GetFrom(); 
    gene_stop = g_loc_it.GetRange().GetTo();
    feat_start = f_loc_it.GetRange().GetFrom();
    feat_stop = f_loc_it.GetRange().GetTo();

    if (gene_strand ==  eNa_strand_minus) {
      if (gene_stop != feat_stop) return false;
      while ( (gene_start != feat_start) && f_loc_it) {
        while (++f_loc_it) {
          feat_strand = f_loc_it.GetStrand();
          if (!StrandOk (gene_strand, feat_strand)) return false;
          feat_start = f_loc_it.GetRange().GetFrom();
          if (feat_start < gene_start) return false;
          else if (feat_start == gene_start) break;
        }
      }
    }
    else {
      if (gene_start != feat_start) return false;
      while (gene_stop != feat_stop && f_loc_it) {
        while (++f_loc_it) {
          feat_strand = f_loc_it.GetStrand();
          if (!StrandOk (gene_strand, feat_strand)) return false;
          feat_stop = f_loc_it.GetRange().GetTo();
          if (feat_stop > gene_stop) return false;
          else if (feat_stop == gene_stop)  break;
        }
      }
    }
    if (!f_loc_it) return false;
    ++g_loc_it;
    ++f_loc_it;
  }

  if (g_loc_it || f_loc_it) return false;
  else return true;
};


bool CBioseq_FEATURE_LOCATION_CONFLICT :: IsGeneLocationOk(const CSeq_feat* seq_feat, const CSeq_feat* gene)
{
  if (IsMixStrand(seq_feat) || IsMixStrand(gene))
     return (IsMixedStrandGeneLocationOk(seq_feat->GetLocation(), gene->GetLocation()));
  else {
    const CSeq_loc& feat_loc = seq_feat->GetLocation();
    const CSeq_loc& gene_loc = gene->GetLocation();
    ENa_strand feat_strand =  feat_loc.GetStrand();
    ENa_strand gene_strand = gene_loc.GetStrand();
    int gene_left = gene_loc.GetStart(eExtreme_Biological);
    int gene_right = gene_loc.GetStop(eExtreme_Biological);
    int feat_left = feat_loc.GetStart(eExtreme_Biological);
    int feat_right = feat_loc.GetStop(eExtreme_Biological);
    
    if ( (feat_strand == eNa_strand_minus && gene_strand != eNa_strand_minus)
                || (feat_strand != eNa_strand_minus && gene_strand == eNa_strand_minus))
         return false;
    else if (gene_left == feat_left && gene_right == feat_right) return true; 
    else if ((gene_strand == eNa_strand_minus && gene_left == feat_left)
                    || (gene_strand != eNa_strand_minus && gene_right == feat_right)) {
       ITERATE (vector <const CSeq_feat*>, it, rbs_feat) {
         const CSeq_loc& rbs_loc = (*it)->GetLocation();
         ENa_strand rbs_strand = rbs_loc.GetStrand();
         int rbs_left = rbs_loc.GetStart(eExtreme_Biological);
         int rbs_right = rbs_loc.GetStop(eExtreme_Biological);
         if (rbs_strand != gene_strand) continue;
         if (rbs_strand == eNa_strand_minus) {
            if (rbs_right == gene_right && rbs_left >= feat_right) return true;
         }
         else if (rbs_left == gene_left && rbs_right <= feat_left) return true;
       } 
       return false;
    }
    else return false;
  }
}
 


bool CBioseq_FEATURE_LOCATION_CONFLICT :: GeneRefMatch(const CGene_ref& gene1, const CGene_ref& gene2)
{
   if (gene1.GetPseudo() != gene2.GetPseudo()) return false;

   string g1_locus = gene1.CanGetLocus() ? gene1.GetLocus() : kEmptyStr;
   string g2_locus = gene2.CanGetLocus() ? gene2.GetLocus() : kEmptyStr;
   if (g1_locus != g2_locus) return false;

   string g1_allele = gene1.CanGetAllele() ? gene1.GetAllele() : kEmptyStr;
   string g2_allele = gene2.CanGetAllele() ? gene2.GetAllele() : kEmptyStr;
   if (g1_allele != g2_allele) return false;

   string g1_desc = gene1.CanGetDesc() ? gene1.GetDesc() : kEmptyStr;
   string g2_desc = gene2.CanGetDesc() ? gene2.GetDesc() : kEmptyStr;
   if (g1_desc != g2_desc) return false;

   string g1_maploc = gene1.CanGetMaploc() ? gene1.GetMaploc() : kEmptyStr;
   string g2_maploc = gene2.CanGetMaploc() ? gene2.GetMaploc() : kEmptyStr;
   if (g1_maploc != g2_maploc) return false;

   string g1_locus_tag = gene1.CanGetLocus_tag() ? gene1.GetLocus_tag() : kEmptyStr;
   string g2_locus_tag = gene2.CanGetLocus_tag() ? gene2.GetLocus_tag() : kEmptyStr;
   if (g1_locus_tag != g2_locus_tag) return false;

   unsigned i;
   bool g1_getdb = gene1.CanGetDb() ? true : false;
   bool g2_getdb = gene1.CanGetDb() ? true : false;
   if (g1_getdb && g2_getdb) {
      const vector <CRef <CDbtag> >& g1_db = gene1.GetDb(), g2_db = gene2.GetDb();
      if (g1_db.size() != g2_db.size()) return false;
      for (i=0; i< g1_db.size(); i++) {
        if (g1_db[i]->GetDb() != g2_db[i]->GetDb()) return false;
      }
      
   }
   else if ( g1_getdb || g2_getdb ) return false; 
 
   bool g1_getsyn = gene1.CanGetSyn() ? true : false;
   bool g2_getsyn = gene2.CanGetSyn() ? true : false;
   if (g1_getsyn && g2_getsyn) {
      const list <string>& g1_syn = gene1.GetSyn(), g2_syn = gene2.GetSyn();
      if (g1_syn.size() != g2_syn.size()) return false;
      list <string> ::const_iterator it, jt;
      for (it=g1_syn.begin(), jt = g2_syn.begin(); it != g1_syn.end(); it++, jt++) 
        if (*it != *jt) return false;
   }
   else if (g1_getsyn || g2_getsyn) return false;

   return true;
};


void CBioseq_FEATURE_LOCATION_CONFLICT :: CheckFeatureTypeForLocationDiscrepancies(const vector <const CSeq_feat*>& seq_feat, const string& feat_type)
{
   ITERATE (vector <const CSeq_feat*>, it, seq_feat) {
     const CGene_ref* gene_ref = (*it)->GetGeneXref();
     const CSeq_loc& feat_loc = (*it)->GetLocation();
     if (gene_ref && !gene_ref->IsSuppressed()) {
        bool found_match = false;
        ITERATE (vector <const CSeq_feat*>, jt, gene_feat) {
          if (GeneRefMatch(*gene_ref, (*jt)->GetData().GetGene()) 
                       && (*jt)->GetLocation().GetStrand() == feat_loc.GetStrand()) {
             if (IsGeneLocationOk(*it, *jt)) found_match = true;
             else if ( (*it)->GetData().GetSubtype()) {
                thisInfo.test_item_list[GetName()].push_back(
                                                      feat_type +"$"+GetDiscItemText(**it));
                thisInfo.test_item_list[GetName()].push_back( GetDiscItemText(**jt));
             }
          }
        }
        if (!found_match)
            thisInfo.test_item_list[GetName()].push_back(
                                          feat_type + "_missing$" + GetDiscItemText(**it));
     }
     else {
         CConstRef <CSeq_feat> gene_olp= GetBestOverlappingFeat(feat_loc,
                                                             CSeqFeatData::e_Gene,
                                                             eOverlap_Contained,
                                                             *thisInfo.scope);

//cerr << "start stop " << feat_loc.GetStart(eExtreme_Biological) << "  " 
  //<< feat_loc.GetStop(eExtreme_Biological) <<  endl;
         if (gene_olp.NotEmpty() && !IsGeneLocationOk(*it, gene_olp.GetPointer())) {
            thisInfo.test_item_list[GetName()].push_back(feat_type +"$"+GetDiscItemText(**it));
            thisInfo.test_item_list[GetName()].push_back(
                                                      GetDiscItemText(*gene_olp.GetPointer()));
         }
     }
   }
};



void CBioseq_FEATURE_LOCATION_CONFLICT :: TestOnObj(const CBioseq& bioseq)
{
  if (bioseq.IsAa()) return;
  if (!IsBioseqHasLineage(bioseq, "Eukaryota", false))
           CheckFeatureTypeForLocationDiscrepancies(cd_feat, "Coding region");        
  CheckFeatureTypeForLocationDiscrepancies(rna_feat, "RNA feature");
};


void CBioseq_FEATURE_LOCATION_CONFLICT :: GetReport(CRef <CClickableItem>& c_item)
{
  vector <string> rep_arr;
  c_item->item_list.clear();
  unsigned cnt=0;
  ITERATE (vector <string>, it, thisInfo.test_item_list[GetName()]) {
    CRef <CClickableItem> c_sub (new CClickableItem);
    rep_arr = NStr::Tokenize(*it, "$", rep_arr);
    size_t pos;
    if ( (pos = rep_arr[0].find("_missing")) != string::npos) {
      c_sub->description = rep_arr[0].substr(0, pos) + " xref gene does not exist";
      c_sub->item_list.push_back(rep_arr[1]);
      c_item->item_list.push_back(rep_arr[1]);
    }
    else {
      c_sub->description = rep_arr[0] + " location does not match gene location";
      c_sub->item_list.push_back(rep_arr[1]);
      c_sub->item_list.push_back( *(++it) );
      c_item->item_list.push_back(rep_arr[1]);
      c_item->item_list.push_back( *it );
    }
     
    c_item->subcategories.push_back(c_sub);
    cnt++;
    rep_arr.clear();
  }
  c_item->description 
        = GetHasComment(cnt, "feature") + "inconsistent gene locations";
};




bool CBioseq_LOCUS_TAGS :: IsLocationDirSub(const CSeq_loc& seq_location)  // not actually used
{   
   const CSeq_id* seq_id  = seq_location.GetId();
   if (!seq_id) return false;
   if (seq_id->Which() == CSeq_id::e_Other) return false;
   CBioseq_Handle bioseq_h = thisInfo.scope->GetBioseqHandle(*seq_id);
   CConstRef <CBioseq> bioseq = bioseq_h.GetCompleteBioseq();
   if (bioseq.NotEmpty()) {
      bool ret = true, is_complete = false;

      ITERATE (list <CRef <CSeq_id> >, it, bioseq->GetId()) {
         if ( (*it)->Which() == CSeq_id :: e_Other) {
           ret = false; break;
         }
      }
      // look for WGS keyword
      for (CSeqdesc_CI seqdesc_it(bioseq_h, CSeqdesc :: e_Genbank);
                                                            ret && seqdesc_it; ++seqdesc_it) {
         const CGB_block& gb = seqdesc_it->GetGenbank();
         if (gb.CanGetKeywords()) {
             ITERATE (list <string>, it, gb.GetKeywords()) {
               if (string::npos != NStr::FindNoCase(*it, "WGS")) {
                     ret = false; break;
               }
             }
         }
      }
      for (CSeqdesc_CI seqdesc_it(bioseq_h, CSeqdesc :: e_Molinfo);
                                                            ret && seqdesc_it; ++seqdesc_it) {
           const CMolInfo& mol = seqdesc_it->GetMolinfo();
           ret = (mol.GetTech() == CMolInfo::eTech_wgs) ? false : ret;
           is_complete =
             (mol.GetCompleteness() == CMolInfo::eCompleteness_complete) ? true : is_complete;
      }
      // is genome? (complete and bacterial)?
      if (is_complete) {
        for (CSeqdesc_CI seqdesc_it(bioseq_h, CSeqdesc :: e_Source);
                                                         ret && seqdesc_it; ++seqdesc_it) {
          ret = HasLineage(seqdesc_it->GetSource(), "Bacteria")? false : ret;
        }
      }
      return ret;
   }
   else return true;  // bioseq is empty
};


void CBioseq_LOCUS_TAGS :: TestOnObj(const CBioseq& bioseq) 
{
  string locus_tag, locus_tag_adj;
  string test_desc, test_desc_adj;
  ITERATE (vector <const CSeq_feat*>, jt, gene_feat) {
     const CGene_ref& gene = (*jt)->GetData().GetGene();
     if (gene.GetPseudo()) continue;  
     test_desc = GetDiscItemText(**jt);
     if (gene.CanGetLocus_tag()) {
        locus_tag = gene.GetLocus_tag();
        if (!locus_tag.empty()) {
           thisInfo.test_item_list[GetName()].push_back( locus_tag + "$" + test_desc);
           // if adjacent gene has the same locus;
           if ((jt+1) != gene_feat.end()) {
             const CGene_ref& gene_adj = (*(jt+1))->GetData().GetGene();
             if (gene_adj.CanGetLocus_tag() && !gene_adj.GetPseudo() 
                                                        && !gene_adj.GetLocus_tag().empty() ) {
                locus_tag_adj = gene_adj.GetLocus_tag();
                if (locus_tag_adj == locus_tag 
                   && ( (*jt)->GetLocation().GetStop(eExtreme_Positional) 
                                   < (*(jt+1))->GetLocation().GetStart(eExtreme_Positional))){
                   test_desc_adj = GetDiscItemText( **(jt+1) );
                   if (DUP_LOCUS_TAGS_adj_genes.find(test_desc) 
                                                           == DUP_LOCUS_TAGS_adj_genes.end())
                       DUP_LOCUS_TAGS_adj_genes.insert(test_desc);
                   if (DUP_LOCUS_TAGS_adj_genes.find(test_desc_adj)
                                                            ==DUP_LOCUS_TAGS_adj_genes.end())
                       DUP_LOCUS_TAGS_adj_genes.insert(test_desc_adj);
                }
             }
           }
        }
        else thisInfo.test_item_list[GetName_missing()].push_back(test_desc);
     }
     else thisInfo.test_item_list[GetName_missing()].push_back(test_desc);
  }
};


void CBioseq_LOCUS_TAGS :: GetReport(CRef <CClickableItem>& c_item)
{
   bool used_citem1 = false;
   // MISSING_LOCUS_TAGS
   if (thisInfo.test_item_list.find(GetName_missing()) != thisInfo.test_item_list.end()) {
     c_item->setting_name = GetName_missing();
     c_item->item_list = thisInfo.test_item_list[GetName_missing()];
     c_item->description = GetHasComment(c_item->item_list.size(), "gene") + "no locus tags.";
     thisInfo.disc_report_data.push_back(c_item);
     used_citem1 = true;
   }

// DUPLICATE_LOCUS_TAGS && Dup_prefix && bad_tag_feats;
   Str2Strs tag2dups, prefix2dups;
   vector <string> dt_arr, bad_tag_feats;
   size_t pos;
   unsigned i;
   bool bad_tag;
   dt_arr.clear();
   ITERATE (vector <string>, it, thisInfo.test_item_list[GetName()]) {
     bad_tag = false;
     dt_arr = NStr::Tokenize(*it, "$", dt_arr);
     tag2dups[dt_arr[0]].push_back(dt_arr[1]);
     if ( (pos = dt_arr[0].find("_")) != string::npos)
          prefix2dups[dt_arr[0].substr(0, pos)].push_back(dt_arr[1]);
     else bad_tag = true;
     if (!isalpha(dt_arr[0][0])) bad_tag = true;
     else {
        dt_arr[0] = dt_arr[0].substr(0, pos) + dt_arr[0].substr(pos+1);
        for (i=0; (i< dt_arr[0].size()) && !bad_tag; i++)
             if ( !isalnum(dt_arr[0][i]) ) bad_tag = true;
     }
     if (bad_tag) bad_tag_feats.push_back(dt_arr[1]);
     dt_arr.clear();
   }

// DUPLICATE_LOCUS_TAGS
   unsigned dup_cnt = 0;
   string dup_setting_name(GetName_dup());
   ITERATE (Str2Strs, it, tag2dups) {
     if (it->second.size() > 1) {
       CRef <CClickableItem> c_sub (new CClickableItem);
       c_sub->item_list = it->second;
       c_sub->setting_name = dup_setting_name;
       c_sub->description
              = GetHasComment(c_sub->item_list.size(), "gene") + "locus tag " + it->first;
       if (thisInfo.report == "Discrepancy") thisInfo.disc_report_data.push_back(c_sub);
       else {
          if (used_citem1) c_item = CRef <CClickableItem> (new CClickableItem);
          c_item->setting_name = dup_setting_name;
          c_item->subcategories.push_back(c_sub);
       }
       dup_cnt += it->second.size();
     }
   }

   if (!DUP_LOCUS_TAGS_adj_genes.empty()) {
       CRef <CClickableItem> c_sub (new CClickableItem);
       c_sub->setting_name = dup_setting_name;
       c_sub->item_list.insert(c_sub->item_list.end(), 
                            DUP_LOCUS_TAGS_adj_genes.begin(), DUP_LOCUS_TAGS_adj_genes.end());
       c_sub->description = GetIsComment(c_sub->item_list.size(), "gene") 
                               + "adjacent to another gene with the same locus tag.";
       if (thisInfo.report == "Discrepancy") thisInfo.disc_report_data.push_back(c_sub);
       else {
            c_item->subcategories.push_back(c_sub);
            if (used_citem1) thisInfo.disc_report_data.push_back(c_item);
            used_citem1 = true;
       }
   }

// inconsistent locus tags: 
   ITERATE (Str2Strs, it, prefix2dups) {
      GetProperCItem(c_item, &used_citem1);
      c_item->setting_name = GetName_incons();
      c_item->item_list = it->second;
      c_item->description 
          = GetHasComment(c_item->item_list.size(),"feature") +"locus tag prefix " + it->first;
   } 

// bad formats: BAD_LOCUS_TAG_FORMAT
   if (!bad_tag_feats.empty()) {
     GetProperCItem(c_item, &used_citem1);
     c_item->setting_name = GetName_badtag();
     c_item->item_list = bad_tag_feats;
     c_item->description 
           = GetIsComment(c_item->item_list.size(), "locus tag") + "incorrectly formatted.";
   }
};


void CBioseq_DISC_FEATURE_COUNT :: TestOnObj(const CBioseq& bioseq)
{
   CBioseq_Handle bioseq_hl = thisInfo.scope->GetBioseqHandle(bioseq);
   SAnnotSelector sel;
   sel.ExcludeFeatSubtype(CSeqFeatData::eSubtype_prot);
   
   string bioseq_text = GetDiscItemText(bioseq);
   CRef <GeneralDiscSubDt> bioseq_feat_count (new GeneralDiscSubDt(kEmptyStr, bioseq_text));

   Str2Int :: iterator it;
   for (CFeat_CI feat_it(bioseq_hl, sel); feat_it; ++feat_it) {  // use non_prot_feat instead
      const CSeqFeatData& seq_feat_dt = (feat_it->GetOriginalFeature()).GetData();
      strtmp = seq_feat_dt.GetKey(CSeqFeatData::eVocabulary_genbank);
      it = bioseq_feat_count->feat_count_list.find(strtmp);
      if (it == bioseq_feat_count->feat_count_list.end())  
                bioseq_feat_count->feat_count_list[strtmp] = 1;
      else it->second ++;
   }
 
   if ( !(bioseq_feat_count->feat_count_list.empty()) ) {
     if (bioseq.IsAa()) bioseq_feat_count->isAa = true;
     else bioseq_feat_count->isAa = false;
     DISC_FEATURE_COUNT_list.push_back(bioseq_feat_count);
     if (thisInfo.test_item_list.find(GetName()) == thisInfo.test_item_list.end())
           thisInfo.test_item_list[GetName()].push_back("Feaure Counts"); 
   }
   else {
      if (bioseq.IsAa()) DISC_FEATURE_COUNT_protfeat_prot_list.push_back(bioseq_text);
      else DISC_FEATURE_COUNT_protfeat_nul_list.push_back(bioseq_text);
   }
};


void CBioseq_DISC_FEATURE_COUNT :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Int feat_bioseqcnt;

   // get all feattype
   ITERATE (vector <CRef <GeneralDiscSubDt> >, it, DISC_FEATURE_COUNT_list) {
      ITERATE (Str2Int, jt, (*it)->feat_count_list) {
         if (feat_bioseqcnt.find( jt->first ) == feat_bioseqcnt.end()) 
                  feat_bioseqcnt[jt->first] = 1;
         else feat_bioseqcnt[jt->first]++;
      }    
   }

   // missing feature
   NON_CONST_ITERATE (vector <CRef <GeneralDiscSubDt> >, it, DISC_FEATURE_COUNT_list) {
     ITERATE (Str2Int, jt, feat_bioseqcnt) {
       if ( (*it)->feat_count_list.find(jt->first) == (*it)->feat_count_list.end()) {
         if ( (jt->first == CSeqFeatData::SelectionName(CSeqFeatData::e_Prot)
                 && (*it)->isAa)
              || (jt->first != CSeqFeatData::SelectionName(CSeqFeatData::e_Prot)
                    && !((*it)->isAa)))
          (*it)->feat_count_list[jt->first] = 0;   
       }
     }
   }

   // subcategories
   unsigned current_num = 0, i, j;
   string current_feat(kEmptyStr);
   typedef map <int, vector <string>, std::greater<int> > Int2Strs;
   Int2Strs featnum_bioseqs;
   unsigned total_feat;
   Str2Int :: iterator jt;
   Int2Strs :: iterator jjt;
   for (i=0; i< DISC_FEATURE_COUNT_list.size(); i++) {
     ITERATE (Str2Int, it, DISC_FEATURE_COUNT_list[i]->feat_count_list) {
        if (it->second >= 0) {
           featnum_bioseqs.clear();
           current_feat = it->first;
           total_feat = current_num = it->second;
           featnum_bioseqs[current_num].push_back(DISC_FEATURE_COUNT_list[i]->str);//bioseq

           for (j = i+1; j < DISC_FEATURE_COUNT_list.size(); j++) {
              jt = DISC_FEATURE_COUNT_list[j]->feat_count_list.find(current_feat);
              if (jt != (DISC_FEATURE_COUNT_list[j]->feat_count_list).end()) {
                jjt = featnum_bioseqs.find(jt->second);
                if (jjt != featnum_bioseqs.end())
                      jjt->second.push_back(DISC_FEATURE_COUNT_list[j]->str); 
                else 
                 featnum_bioseqs[jt->second].push_back(DISC_FEATURE_COUNT_list[j]->str);
                total_feat += jt->second;
                if (jt->second >0) jt->second *= -1;
                else jt->second = -1;
              } 
           } 

           if (current_feat == CSeqFeatData::SelectionName(CSeqFeatData::e_Prot))
               ITERATE (vector <string>, p_it, DISC_FEATURE_COUNT_protfeat_prot_list)
                 featnum_bioseqs[0].push_back(*p_it);
           else ITERATE (vector <string>, p_it, DISC_FEATURE_COUNT_protfeat_nul_list)
                 featnum_bioseqs[0].push_back(*p_it);

           CRef <CClickableItem> c_type_sub (new CClickableItem);  // type_list
           c_type_sub->description = current_feat + ": " + NStr::UIntToString(total_feat)
                              + ((total_feat > 1)? " present" : " presents")
                              + ((featnum_bioseqs.size() > 1)? " (inconsistent)" : kEmptyStr);
           c_type_sub->setting_name = GetName();

           ITERATE (Int2Strs, kt, featnum_bioseqs) {
              CRef <CClickableItem> c_sub (new CClickableItem);
              c_sub->setting_name = GetName();
              c_sub->item_list = kt->second;
              c_sub->description = GetHasComment(c_sub->item_list.size(), "bioseq")
                                     + NStr::IntToString(kt->first) + " " + current_feat
                                     + ( (kt->first > 1)? " features" : " feature");
              c_type_sub->subcategories.push_back(c_sub);
           } 
  
           c_item->subcategories.push_back(c_type_sub);
        }
     }
    
   } 
   c_item->description = "Feature Counts";
   c_item->item_list.clear();

   DISC_FEATURE_COUNT_protfeat_prot_list.clear();
   DISC_FEATURE_COUNT_protfeat_nul_list.clear();
   DISC_FEATURE_COUNT_list.clear();
};


void CSeqEntryTestAndRepData :: TestOnBiosrc(const CSeq_entry& seq_entry)
{
   string desc;
   ITERATE (vector <const CSeq_feat*>, it, biosrc_feat) {
     const CBioSource& biosrc = (*it)->GetData().GetBiosrc();
     desc = GetDiscItemText(**it);
     if (IsBacterialIsolate(biosrc))
               thisInfo.test_item_list[GetName_iso()].push_back(desc); 
   };

   unsigned i=0;
   ITERATE (vector <const CSeqdesc*>, it, biosrc_seqdesc) {
     const CBioSource& biosrc = (*it)->GetSource();
     desc = GetDiscItemText( **it, *(biosrc_seqdesc_seqentry[i]));
     if (IsBacterialIsolate(biosrc)) 
                 thisInfo.test_item_list[GetName_iso()].push_back(desc);
     i++;
   };

  thisTest.is_BIOSRC_run = true;
};


// change parent name to CSeqEntry_on_biosrc ::
bool CSeqEntryTestAndRepData :: HasAmplifiedWithSpeciesSpecificPrimerNote(const CBioSource& biosrc)
{
  if (biosrc.CanGetSubtype()) {
    ITERATE (list <CRef <CSubSource> >, it, biosrc.GetSubtype()) {
      if ( (*it)->GetSubtype() == CSubSource::eSubtype_other
            && (*it)->GetName() == "amplified with species-specific primers")
        return true;
    }
  }

  ITERATE (list <CRef <COrgMod> >, it, biosrc.GetOrg().GetOrgname().GetMod()) {
     if ( (*it)->GetSubtype() == COrgMod::eSubtype_other
                  && (*it)->GetSubname() == "amplified with species-specific primers")
        return true;
  }
  return false;
};


bool CSeqEntryTestAndRepData :: IsBacterialIsolate(const CBioSource& biosrc)
{
   if (!HasLineage(biosrc, "Bacteria")
         || !biosrc.IsSetOrgMod()
         || HasAmplifiedWithSpeciesSpecificPrimerNote(biosrc)) return false;

   ITERATE (list <CRef <COrgMod> >, it, biosrc.GetOrg().GetOrgname().GetMod()) {
     strtmp = (*it)->GetSubname();
     if ( (*it)->GetSubtype() == COrgMod::eSubtype_isolate
             && !NStr::EqualNocase(strtmp, 0, 13, "DGGE gel band")
             && !NStr::EqualNocase(strtmp, 0, 13, "TGGE gel band")
             && !NStr::EqualNocase(strtmp, 0, 13, "SSCP gel band"))
        return true;
   }
   return false; 
};



void CSeqEntry_DISC_BACTERIA_SHOULD_NOT_HAVE_ISOLATE :: TestOnObj(const CSeq_entry& seq_entry)
{
   if (!thisTest.is_BIOSRC_run) TestOnBiosrc(seq_entry);
};


void CSeqEntry_DISC_BACTERIA_SHOULD_NOT_HAVE_ISOLATE :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "bacterial biosrouce")
                           + "isolate.";
};



bool CSeqEntry_INCONSISTENT_BIOSOURCE :: SynonymsMatch(const COrg_ref& org1, const COrg_ref& org2)
{
     bool has_syn1 = org1.CanGetSyn(), has_syn2 = org2.CanGetSyn();

     if ( !has_syn1 && !has_syn2 ) return true;
     if ( has_syn1 && has_syn2 ) {
        list <string>& syn1 = const_cast<COrg_ref&>(org1).SetSyn();
        list <string>& syn2 = const_cast<COrg_ref&>(org2).SetSyn();

        if (syn1.size() != syn2.size()) return false;

        syn1.sort();
        syn2.sort();

        list <string> :: const_iterator it, jt;
        for (it = syn1.begin(), jt = syn2.begin(); it != syn1.end(); it++, jt++) {
             if ( (*it) != (*jt)) return false;
        }
        return true;
     }
     else return false;

};  // SynonymsMatch



bool CSeqEntry_INCONSISTENT_BIOSOURCE :: DbtagMatch(const COrg_ref& org1, const COrg_ref& org2)
{
    bool has_db1 = org1.CanGetDb(), has_db2 = org2.CanGetDb();

    if ( !has_db1 && !has_db2) return true;
    else if ( has_db1 && has_db2)  {
      const vector < CRef < CDbtag > >& db1 = org1.GetDb();
      const vector < CRef < CDbtag > >& db2 = org2.GetDb();

      if (db1.size() != db2.size()) return false;
      for (unsigned i=0; i< db1.size(); i++) {   // already sorted
           if ( !db1[i]->Match(*db2[i]) ) return false;
      };
      return true;
    }
    else return false;
}; // DbtagMatch



bool CSeqEntry_INCONSISTENT_BIOSOURCE :: OrgModSetMatch(const COrgName& nm1, const COrgName& nm2) 
{
  bool has_mod1 = nm1.CanGetMod(), has_mod2 = nm2.CanGetMod();

  if (!has_mod1 && !has_mod2) return true;
  if (has_mod1 && has_mod2) {
     const list < CRef <COrgMod> >& mod1= nm1.GetMod();
     const list < CRef <COrgMod> >& mod2 = nm2.GetMod();

     if (mod1.size() != mod2.size()) return false;
     list < CRef < COrgMod > > ::const_iterator it, jt;
     // already sorted
     for (it = mod1.begin(), jt = mod2.begin(); it != mod1.end(); it++, jt++) {
       string att_i = (*it)->CanGetAttrib()? (*it)->GetAttrib() : kEmptyStr;
       string att_j = (*jt)->CanGetAttrib()? (*jt)->GetAttrib(): kEmptyStr;
       if ( (*it)->GetSubtype() != (*jt)->GetSubtype()
                  || (*it)->GetSubname() != (*jt)->GetSubname() || att_i != att_j)
            return false;
     }
     return true;
  }
  else return false;
};  // OrgModSetMatch



bool CSeqEntry_INCONSISTENT_BIOSOURCE :: OrgNameMatch(const COrg_ref& org1, const COrg_ref& org2)
{
   bool has_orgnm1 = org1.CanGetOrgname();
   bool has_orgnm2 = org2.CanGetOrgname();
   
   if (!has_orgnm1 && !has_orgnm2) return true;
   else if (has_orgnm1 && has_orgnm2) {
     const COrgName& nm1 = org1.GetOrgname();
     const COrgName& nm2 = org2.GetOrgname();

     COrgName::C_Name::E_Choice choc1 = nm1.CanGetName()? 
                                          nm1.GetName().Which(): COrgName::C_Name::e_not_set;
     COrgName::C_Name::E_Choice choc2 = nm2.CanGetName()? 
                                          nm1.GetName().Which(): COrgName::C_Name::e_not_set;
     int gcode1 = nm1.CanGetGcode()? nm1.GetGcode(): 0;
     int gcode2 = nm2.CanGetGcode()? nm2.GetGcode(): 0;
     int mgcode1 = nm1.CanGetMgcode()? nm1.GetMgcode() : 0;
     int mgcode2 = nm2.CanGetMgcode()? nm2.GetMgcode() : 0;
     string att1 = nm1.CanGetAttrib()? nm1.GetAttrib() : kEmptyStr;
     string att2 = nm2.CanGetAttrib()? nm2.GetAttrib() : kEmptyStr;
     string lin1 = nm1.CanGetLineage()? nm1.GetLineage() : kEmptyStr;
     string lin2 = nm2.CanGetLineage()? nm2.GetLineage() : kEmptyStr;
     string div1 = nm1.CanGetDiv()? nm1.GetDiv() : kEmptyStr;
     string div2 = nm2.CanGetDiv()? nm2.GetDiv() : kEmptyStr;

     if (choc1 != choc2 || gcode1 != gcode2 || mgcode1 != mgcode2 || att1 != att2 
               || lin1 != lin2 || div1 != div2 || !OrgModSetMatch(nm1, nm2)) 
           return false;
     else return true;
   }
   else return false;
}; // OrgNameMatch 



bool CSeqEntry_INCONSISTENT_BIOSOURCE :: OrgRefMatch(const COrg_ref& org1, const COrg_ref& org2)
{
  string tax1 = org1.CanGetTaxname()? org1.GetTaxname() : kEmptyStr;
  string tax2 = org2.CanGetTaxname()? org2.GetTaxname() : kEmptyStr;
  string comm1 = org1.CanGetCommon()? org1.GetCommon() : kEmptyStr;
  string comm2 = org2.CanGetCommon()? org2.GetCommon() : kEmptyStr;
 
  if ( tax1 != tax2 || comm1 != comm2) return false;
  else if ( !SynonymsMatch(org1, org2) || !DbtagMatch(org1, org2)) return false;
  else if (!OrgNameMatch(org1, org2)) return false;
  else return true;
};  // OrgRefMatch


bool CSeqEntry_INCONSISTENT_BIOSOURCE :: SubSourceSetMatch(const CBioSource& biosrc1, const CBioSource& biosrc2)
{
  bool has_subtype1 = biosrc1.CanGetSubtype();
  bool has_subtype2 = biosrc2.CanGetSubtype();

  if (!has_subtype1 && !has_subtype2) return true;
  else if (has_subtype1 && has_subtype2) {
    const list < CRef < CSubSource > >& subsrc1 = biosrc1.GetSubtype();
    const list < CRef < CSubSource > >& subsrc2 = biosrc2.GetSubtype();

    if (subsrc1.size() != subsrc2.size()) return false;
    list < CRef < CSubSource > >::const_iterator it, jt;
    // already sorted
    for (it = subsrc1.begin(), jt = subsrc2.begin(); it != subsrc1.end(); it++, jt++) { 
         if ( (*it)->GetSubtype() != (*jt)->GetSubtype() 
              || (*it)->GetName() != (*jt)->GetName()  
              || ( (*it)->CanGetAttrib()? (*it)->GetAttrib(): kEmptyStr )
                   != ( (*jt)->CanGetAttrib()? (*jt)->GetAttrib(): kEmptyStr ) ) {
             return false;
         }
     }
     return true;
  }  
  else return false;
}; // SubSourceSetMatch


bool CSeqEntry_INCONSISTENT_BIOSOURCE :: BioSourceMatch( const CBioSource& biosrc1, const CBioSource& biosrc2)
{
  if ( (biosrc1.GetOrigin() != biosrc2.GetOrigin()) 
          || (biosrc1.CanGetIs_focus() != biosrc2.CanGetIs_focus())
          || !OrgRefMatch(biosrc1.GetOrg(), biosrc2.GetOrg()) 
          || !SubSourceSetMatch(biosrc1, biosrc2) )
      return false;

  int genome1 = biosrc1.GetGenome();
  int genome2 = biosrc2.GetGenome();
  if (genome1 == genome2) return true;
  if ( (genome1 = CBioSource::eGenome_unknown && genome2 == CBioSource::eGenome_genomic)
        || (genome1 == CBioSource::eGenome_genomic && genome2 == CBioSource::eGenome_unknown) )
       return true;
 
  else return false;
};



string CSeqEntry_INCONSISTENT_BIOSOURCE :: OrgModSetDifferences(const COrgName& nm1, const COrgName& nm2) 
{
  bool has_mod1 = nm1.CanGetMod(), has_mod2 = nm2.CanGetMod();
  string org_mod_diff_str(kEmptyStr);
  
  if (!has_mod1 && !has_mod2) return (kEmptyStr);
  else if (has_mod1 && has_mod2) {
     const list < CRef <COrgMod> >& mod1= nm1.GetMod();
     const list < CRef <COrgMod> >& mod2 = nm2.GetMod();

     string att_i, att_j, qual;
     unsigned sz = min(mod1.size(), mod2.size());
     list < CRef < COrgMod > > :: const_iterator it, jt;
     unsigned i;
     for (i=0, it = mod1.begin(), jt = mod2.begin(); i < sz; i++, it++, jt++) {
       att_i = (*it)->CanGetAttrib()? (*it)->GetAttrib(): kEmptyStr;
       att_j = (*jt)->CanGetAttrib()? (*jt)->GetAttrib(): kEmptyStr;
       qual = (*it)->GetSubtypeName((*it)->GetSubtype(), COrgMod::eVocabulary_insdc);
       if ( (*it)->GetSubtype() != (*jt)->GetSubtype() ) 
          org_mod_diff_str += "Missing " + qual + " modifier, ";
       else if (att_i != att_j) 
          org_mod_diff_str += qual + " modifier attrib values differ, ";
       else if ( (*it)->GetSubname() != (*jt)->GetSubname() )
          org_mod_diff_str += "Different " + qual + " values, ";
     } 
     if (sz < mod1.size()) {
        // pick up one of the remain
        org_mod_diff_str 
           += "Missing " 
              + (*it)->GetSubtypeName((*it)->GetSubtype(), COrgMod::eVocabulary_insdc)
              +" modifier, ";
     }
     else if (sz < mod2.size()) {
        // pick up one of the remain
        org_mod_diff_str 
           += "Missing " 
              + (*jt)->GetSubtypeName((*jt)->GetSubtype(), COrgMod::eVocabulary_insdc)
              + " modifier, ";
     }
  }
  else if (has_mod1) {
     list < CRef < COrgMod > >::const_iterator it = nm1.GetMod().begin();
     org_mod_diff_str 
         += "Missing " 
            + (*it)->GetSubtypeName((*it)->GetSubtype(), COrgMod::eVocabulary_insdc) 
            + " modifier, ";
  }
  else {    // has_mod2
     list < CRef < COrgMod > >::const_iterator it = nm2.GetMod().begin();
     org_mod_diff_str 
       += "Missing " 
           + (*it)->GetSubtypeName((*it)->GetSubtype(), COrgMod::eVocabulary_insdc) 
           + " modifier, ";
  }
  org_mod_diff_str = org_mod_diff_str.substr(0, org_mod_diff_str.size()-2);
  return (org_mod_diff_str);
};




string CSeqEntry_INCONSISTENT_BIOSOURCE :: DescribeOrgNameDifferences(const COrg_ref& org1, const COrg_ref& org2)
{
   bool has_orgnm1 = org1.CanGetOrgname();
   bool has_orgnm2 = org2.CanGetOrgname();

   if (!has_orgnm1 && !has_orgnm2) return(kEmptyStr);
   else if (has_orgnm1 && has_orgnm2) {
     string org_nm_diff_str(kEmptyStr);
     const COrgName& nm1 = org1.GetOrgname();
     const COrgName& nm2 = org2.GetOrgname();

     COrgName::C_Name::E_Choice choc1 = nm1.CanGetName()?
                                          nm1.GetName().Which(): COrgName::C_Name :: e_not_set;
     COrgName::C_Name::E_Choice choc2 = nm2.CanGetName()?
                                          nm2.GetName().Which(): COrgName::C_Name :: e_not_set;
     int gcode1 = nm1.CanGetGcode()? nm1.GetGcode(): 0;
     int gcode2 = nm2.CanGetGcode()? nm2.GetGcode(): 0;
     int mgcode1 = nm1.CanGetMgcode()? nm1.GetMgcode() : 0;
     int mgcode2 = nm2.CanGetMgcode()? nm2.GetMgcode() : 0;
     string att1 = nm1.CanGetAttrib()? nm1.GetAttrib() : kEmptyStr;
     string att2 = nm2.CanGetAttrib()? nm2.GetAttrib() : kEmptyStr;
     string lin1 = nm1.CanGetLineage()? nm1.GetLineage() : kEmptyStr;
     string lin2 = nm2.CanGetLineage()? nm2.GetLineage() : kEmptyStr;
     string div1 = nm1.CanGetDiv()? nm1.GetDiv() : kEmptyStr;
     string div2 = nm2.CanGetDiv()? nm2.GetDiv() : kEmptyStr;

     if (choc1 != choc2) org_nm_diff_str = "orgname choices differ, ";
     if (gcode1 != gcode2) org_nm_diff_str += "genetic codes differ, ";
     if (mgcode1 != mgcode2) org_nm_diff_str += "mitochondrial genetic codes differ, ";
     if (att1 != att2) org_nm_diff_str += "attributes differ, ";
     if (lin1 != lin2) org_nm_diff_str += "lineages differ, ";
     if (div1 != div2) org_nm_diff_str += "divisions differ, ";
     string tmp = OrgModSetDifferences(nm1, nm2);
     if (!tmp.empty()) org_nm_diff_str += tmp + ", ";
     org_nm_diff_str = org_nm_diff_str.substr(0, org_nm_diff_str.size()-2);
     return (org_nm_diff_str); 
   }
   else return ("One Orgname is missing, ");
}



string CSeqEntry_INCONSISTENT_BIOSOURCE :: DescribeOrgRefDifferences(const COrg_ref& org1, const COrg_ref& org2)
{
  string org_ref_diff_str(kEmptyStr);

  string tax1 = org1.CanGetTaxname()? org1.GetTaxname() : kEmptyStr;
  string tax2 = org2.CanGetTaxname()? org2.GetTaxname() : kEmptyStr;
  string comm1 = org1.CanGetCommon()? org1.GetCommon() : kEmptyStr;
  string comm2 = org2.CanGetCommon()? org2.GetCommon() : kEmptyStr;

  if ( tax1 != tax2 ) org_ref_diff_str = "taxnames differ, ";
  if ( comm1 != comm2 ) org_ref_diff_str += "common names differ, ";
  if ( !SynonymsMatch(org1, org2) ) org_ref_diff_str += "synonyms differ, ";
  if ( !DbtagMatch(org1, org2)) org_ref_diff_str += "dbxrefs differ, ";
  string tmp = DescribeOrgNameDifferences(org1, org2);
  if (!tmp.empty()) org_ref_diff_str += tmp + ", ";
  org_ref_diff_str = org_ref_diff_str.substr(0, org_ref_diff_str.size()-2);
  return (org_ref_diff_str);
}



string CSeqEntry_INCONSISTENT_BIOSOURCE :: DescribeBioSourceDifferences(const CBioSource& biosrc1, const CBioSource& biosrc2)
{
    string diff_str(kEmptyStr);
    if (biosrc1.GetOrigin() != biosrc2.GetOrigin()) diff_str = "origins differ, ";
    if (biosrc1.CanGetIs_focus() != biosrc2.CanGetIs_focus()) diff_str += "focus differs, ";
    int genome1 = biosrc1.GetGenome();
    int genome2 = biosrc2.GetGenome();
    if (genome1 != genome2
        && !(genome1 = CBioSource::eGenome_unknown && genome2 == CBioSource::eGenome_genomic)
        && !(genome1 == CBioSource::eGenome_genomic && genome2 ==CBioSource::eGenome_unknown) )
       diff_str += "locations differ, ";
    if (!SubSourceSetMatch(biosrc1,biosrc2)) diff_str += "subsource qualifiers differ, ";
    string tmp = DescribeOrgRefDifferences (biosrc1.GetOrg(), biosrc2.GetOrg());
    if (!tmp.empty()) diff_str += tmp + ", ";
    diff_str = diff_str.substr(0, diff_str.size()-2);
    return (diff_str);
};


void CBioseqTestAndRepData :: TestProteinID(const CBioseq& bioseq)
{
   bool has_pid;
   ITERATE (list <CRef <CSeq_id> >, it, bioseq.GetId()) {
     if ((*it)->IsGeneral()) {
       const CDbtag& dbtag = (*it)->GetGeneral();
       if (!dbtag.IsSkippable()) {
          has_pid = true;
          strtmp = dbtag.GetDb();
          if (!strtmp.empty())
                thisInfo.test_item_list[GetName_prefix()].push_back(
                                             dbtag.GetDb() + "$" + GetDiscItemText(bioseq));
          break;
       }
       else has_pid = false; 
     }
   }

   if (!has_pid) 
       thisInfo.test_item_list[GetName_pid()].push_back(GetDiscItemText(bioseq));
};



// new comb.
void CBioseq_test_on_genprod_set :: TestOnObj(const CBioseq& bioseq)
{
   if (thisTest.is_GP_Set_run) return;
   if (!bioseq.IsNa()) {thisTest.is_GP_Set_run = true; return; }
   CConstRef <CBioseq_set> set = bioseq.GetParentSet();
   if (set.Empty()) { thisTest.is_GP_Set_run = true; return;}

   string prod_id, desc;
   /* look for missing protein IDs and duplicate protein IDs on coding regions */
   ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
     desc = GetDiscItemText(**it);
     if (!(*it)->CanGetProduct()) {
        if (!(*it)->CanGetPseudo() || !(*it)->GetPseudo())
          thisInfo.test_item_list[GetName_mprot()].push_back(desc);
     }
     else {
        const CSeq_id& seq_id = sequence::GetId((*it)->GetProduct(), thisInfo.scope); 
        seq_id.GetLabel(&prod_id); 
        thisInfo.test_item_list[GetName_dprot()].push_back(prod_id + "$" + desc);
     }
   } 

   /* look for missing transcript IDs and duplicate transcript IDs on mRNAs */
   ITERATE (vector <const CSeq_feat*>, it, mrna_feat) {
     desc = GetDiscItemText(**it);
     if (!(*it)->CanGetProduct()) {
        if (!(*it)->CanGetPseudo() || !(*it)->GetPseudo())
          thisInfo.test_item_list[GetName_mtid()].push_back(desc);
     }
     else {
        const CSeq_id& seq_id = sequence::GetId((*it)->GetProduct(), thisInfo.scope);
        seq_id.GetLabel(&prod_id); 
        thisInfo.test_item_list[GetName_dtid()].push_back(prod_id + "$" + desc);
     }
   }
   
   thisTest.is_GP_Set_run = true;
};


void CBioseq_test_on_genprod_set :: GetReport_dup(CRef <CClickableItem>& c_item, const string& setting_name, const string& desc1, const string& desc2, const string& desc3)
{
   Str2Strs label2feats;
   GetTestItemList(c_item->item_list, label2feats);
   c_item->item_list.clear();
   unsigned cnt;
   if (label2feats.size() == 1) {
       cnt = (label2feats.begin()->second).size(); 
       if (cnt > 1) {
          c_item->item_list = label2feats.begin()->second;
          c_item->description 
             = GetHasComment(cnt, desc1) + desc2 + label2feats.begin()->first;
       }
   }
   else {
      ITERATE (Str2Strs, it, label2feats) {
         if (it->second.size() > 1) {
            AddSubcategories(c_item, setting_name, it->second, desc1,
                              desc2 + it->first, e_HasComment); 
         } 
      }
      c_item->description 
         = GetHasComment(c_item->item_list.size(), desc1) + desc3;
   };
};


void CBioseq_MISSING_GENPRODSET_PROTEIN :: GetReport(CRef <CClickableItem>& c_item)
{
   unsigned cnt = c_item->item_list.size();
   c_item->description = GetHasComment(cnt, "coding region") 
                           + "missing protein " + ((cnt > 1) ? "IDs." : "ID.");
};


void CBioseq_DUP_GENPRODSET_PROTEIN :: GetReport(CRef <CClickableItem>& c_item)
{
   GetReport_dup(c_item, GetName(), "coding region", "protein ID ", 
                                           "non-unique protein IDs.");
};


void CBioseq_MISSING_GENPRODSET_TRANSCRIPT_ID :: GetReport(CRef <CClickableItem>& c_item)
{
   unsigned cnt = c_item->item_list.size();
   c_item->description = GetHasComment(cnt, "mRNA") 
                           + "missing transcript " + ((cnt > 1) ? "IDs." : "ID.");
};


void CBioseq_DISC_DUP_GENPRODSET_TRANSCRIPT_ID :: GetReport(CRef <CClickableItem>& c_item)
{
   GetReport_dup(c_item, GetName(), "mRNA", "non-unique transcript ID ", 
                                              "non-unique transcript IDs.");
};


void CBioseq_test_on_prot :: TestOnObj(const CBioseq& bioseq)
{
   if (thisTest.is_Prot_run) return;

   bool has_pid;
   string desc(GetDiscItemText(bioseq));

   // COUNT_PROTEINS
   thisInfo.test_item_list[GetName_cnt()].push_back(desc); 

   // INCONSISTENT_PROTEIN_ID_PREFIX1, MISSING_PROTEIN_ID1
   ITERATE (list <CRef <CSeq_id> >, it, bioseq.GetId()) {
     if ((*it)->IsGeneral()) {
       const CDbtag& dbtag = (*it)->GetGeneral();
       if (!dbtag.IsSkippable()) {
          has_pid = true;
          strtmp = dbtag.GetDb();
          if (!strtmp.empty())
             thisInfo.test_item_list[GetName_prefix()].push_back(dbtag.GetDb()+"$"+desc);
          break;
       }
       else has_pid = false;
     }
   }

   if (!has_pid)
       thisInfo.test_item_list[GetName_id()].push_back(desc);

   thisTest.is_Prot_run = true;
};


void CBioseq_COUNT_PROTEINS :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
     = NStr::UIntToString((unsigned)c_item->item_list.size()) + " protein sequences in record";
};


void CBioseq_MISSING_PROTEIN_ID1 :: GetReport(CRef <CClickableItem>& c_item) 
{
  unsigned cnt = c_item->item_list.size();
  c_item->description = GetHasComment(cnt, "protein")
                         + "invalid " + ( (cnt > 1)? "IDs." : "ID." );
};


void CBioseq_INCONSISTENT_PROTEIN_ID_PREFIX1 :: MakeRep(const Str2Strs& item_map, const string& desc1, const string& desc2)
{
   string setting_name = GetName();
   ITERATE (Str2Strs, it, item_map) {
     if (it->second.size() > 1) {
       CRef <CClickableItem> c_item (new CClickableItem);
       c_item->setting_name = setting_name;
       c_item->item_list = it->second;
       c_item->description
          = GetHasComment(c_item->item_list.size(), desc1) + desc2 + it->first + ".";
       thisInfo.disc_report_data.push_back(c_item);
     }
   }
};


void CBioseq_INCONSISTENT_PROTEIN_ID_PREFIX1 :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs db2list, prefix2list;
   GetTestItemList(c_item->item_list, db2list);

   GetTestItemList(c_item->item_list, prefix2list);
   c_item->item_list.clear();

   // ReportInconsistentGlobalDiscrepancyStrings
   if (db2list.size() > 1) MakeRep(db2list, "sequence", "protein ID prefix ");

   // ReportInconsistentGlobalDiscrepancyPrefixes
   if (prefix2list.size() > 1) MakeRep(prefix2list, "feature", "locus tag prefix ");
};



void CBioseq_test_on_cd_4_transl :: TranslExceptOfCDs (const CSeq_feat& cd, bool& has_transl, bool& too_long)
{
  has_transl = too_long = false;
  unsigned len, tmp_len;
  tmp_len = len = sequence::GetCoverage(cd.GetLocation(), thisInfo.scope);
  switch ( cd.GetData().GetCdregion().GetFrame() ) {
     case CCdregion::eFrame_two: tmp_len -=1; break;
     case CCdregion::eFrame_three: tmp_len -= 2; break;
     default: break;
  }

  unsigned codon_start, codon_stop, pos, i, codon_len;
  ITERATE (list <CRef <CCode_break> >, it, cd.GetData().GetCdregion().GetCode_break()) {
    if ( !((*it)->GetAa().IsNcbieaa()) || (*it)->GetAa().GetNcbieaa() != 42) continue;
    if (!too_long && sequence::GetCoverage((*it)->GetLoc(), thisInfo.scope) > 3) 
         too_long = true;
      
    if (!has_transl && (tmp_len % 3)) {
       i=0;
       for (CSeq_loc_CI loc_ci( (*it)->GetLoc() ); loc_ci && !has_transl; ++loc_ci, i++) {
          pos = sequence::LocationOffset(cd.GetLocation(), loc_ci.GetEmbeddingSeq_loc(),
                                eOffset_FromLeft, thisInfo.scope);
          
          if ( (int)pos > -1 &&  (!i || pos <= codon_start)) {
            codon_start = pos;
            codon_len = loc_ci.GetRange().GetLength() - 1;// codon_len=codon_stop - codon_start
            codon_stop = codon_len + codon_start;
            if (codon_len >= 0 && codon_len <= 1 && codon_stop == len - 1) 
               has_transl = true;
          } 
       }
    }
    
    if (too_long && has_transl) return;
  }
};


void CBioseq_test_on_cd_4_transl :: TestOnObj(const CBioseq& bioseq)
{ 
   if (thisTest.is_CdTransl_run) return;
  
   string desc, comment;
   string note_txt("TAA stop codon is completed by the addition of 3' A residues to the mRNA");
   bool has_transl, too_long;
   ITERATE (vector <const CSeq_feat*>, it, cd_feat) {
     desc = GetDiscItemText(**it); 
     comment = (*it)->CanGetComment()? (*it)->GetComment() : kEmptyStr;
     if ( (*it)->GetData().GetCdregion().CanGetCode_break() ) {
        TranslExceptOfCDs(**it, has_transl, too_long); 
        if (has_transl) {
            if (comment.find(note_txt.substr(0, 3)) == string::npos) 
                 thisInfo.test_item_list[GetName_note()].push_back(desc);
        }
        else if (comment.find(note_txt) != string::npos)
                    thisInfo.test_item_list[GetName_transl()].push_back(desc);
        if (too_long) thisInfo.test_item_list[GetName_long()].push_back(desc);
     }
     else if (comment.find(note_txt) != string::npos)
                    thisInfo.test_item_list[GetName_transl()].push_back(desc);
   }  

   thisTest.is_CdTransl_run = true;
};

void CBioseq_TRANSL_NO_NOTE :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "feature") 
                             + "a translation exception but no note";
};

void CBioseq_NOTE_NO_TRANSL :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "feature")
                            + "a note but not translation exception";
};

void CBioseq_TRANSL_TOO_LONG :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "feature")
                             + "translation exceptions longer than 3 bp";
};


void CBioseq_test_on_rna :: FindMissingRNAsInList()
{
  vector <unsigned>  num_present;
  Str2Strs aa_seqfeat;
  num_present.reserve(thisInfo.desired_aaList.size());
  for (unsigned i=0; i< thisInfo.desired_aaList.size(); i++) num_present.push_back(0);

  string context_label;
  unsigned i; 
  ITERATE (vector <const CSeq_feat*>, it, trna_feat) {
     GetSeqFeatLabel(**it, &context_label);
     i=0;
     ITERATE (Str2UInt, jt, thisInfo.desired_aaList) {
        if (context_label.find(jt->first) != string::npos) {
              num_present[i]++;
              aa_seqfeat[jt->first].push_back(GetDiscItemText(**it));
              break;
        }
        i++;
     } 
  }

  i=0; 
  ITERATE (Str2UInt, it, thisInfo.desired_aaList) {
    if (num_present[i] < it->second) 
       thisInfo.test_item_list[GetName_tcnt()].push_back(
           "Sequence " + m_best_id_str + " is missing trna-" + it->first + "$" + m_bioseq_desc);
                            //   it->first + "_missing$" + m_bioseq_desc);
    else if (num_present[i] > it->second) {
       strtmp = "Sequence " + m_best_id_str + " has " + NStr::UIntToString(num_present[i]) 
                     + " trna-" + it->first + (num_present[i]==1 ? " feature." : " features.") + "$";
       thisInfo.test_item_list[GetName_tcnt()].push_back(
                                               strtmp + m_bioseq_desc);
       ITERATE (vector <string>, jt, aa_seqfeat[it->first])
           thisInfo.test_item_list[GetName_tcnt()].push_back(strtmp + *jt);
    }
    i++;    
  }
};


bool CBioseq_test_on_rna :: RRnaMatch(const CRNA_ref& rna1, const CRNA_ref& rna2)
{
  if (!rna1.CanGetExt() && !rna2.CanGetExt()) return true;
  else if (rna1.CanGetExt() && rna1.CanGetExt()) {
    const CRNA_ref::C_Ext& ext1 = rna1.GetExt();
    const CRNA_ref::C_Ext& ext2 = rna2.GetExt();
    if (ext1.Which() != ext2.Which()) return false;
    else {
      if (ext1.IsName()) {
          if (ext1.GetName() == ext2.GetName()) return true;
          else return false;
      }
      else if (ext1.IsTRNA()) {
         const CTrna_ext& trna1 = ext1.GetTRNA();
         const CTrna_ext& trna2 = ext2.GetTRNA();
         bool has_aa1 = trna1.CanGetAa();
         bool has_aa2 = trna2.CanGetAa();
         if (has_aa1 != has_aa2) return false;
         else {
            const CTrna_ext::C_Aa& aa1 = trna1.GetAa();
            const CTrna_ext::C_Aa& aa2 = trna2.GetAa();
            if (aa1.Which() != aa2.Which()) return false;
            else {
               int iaa1, iaa2; 
               if (aa1.IsIupacaa()) {
                   iaa1 = aa1.GetIupacaa();
                   iaa2 = aa2.GetIupacaa();
               }
               else if (aa1.IsNcbieaa()) {
                   iaa1 = aa1.GetNcbieaa();
                   iaa2 = aa2.GetNcbieaa();
               }
               else if (aa1.IsNcbi8aa()) {
                   iaa1 = aa1.GetNcbi8aa();
                   iaa2 = aa2.GetNcbi8aa();
               }
               else if (aa1.IsNcbistdaa()) {
                   iaa1 = aa1.GetNcbistdaa();
                   iaa2 = aa2.GetNcbistdaa();
               }
               else return true;
               if (iaa1 == iaa2) return true;
               else return false;
            }
         }
      }
      else if (ext1.IsGen()) {
        const CRNA_gen& gen1 = ext1.GetGen();
        const CRNA_gen& gen2 = ext2.GetGen();
        string class1 = gen1.CanGetClass() ? gen1.GetClass() : kEmptyStr;
        string class2 = gen2.CanGetClass() ? gen2.GetClass() : kEmptyStr;
        if (class1 != class2) return false;
        string prod1 = gen1.CanGetProduct() ? gen1.GetProduct() : kEmptyStr;
        string prod2 = gen2.CanGetProduct() ? gen2.GetProduct() : kEmptyStr;
        if (prod1 != prod2) return false;
        bool has_quals1 = gen1.CanGetQuals();
        bool has_quals2 = gen2.CanGetQuals();
        if (has_quals1 != has_quals2) return false;
        else if (has_quals1) { 
          unsigned sz1 = gen1.GetQuals().Get().size();
          unsigned sz2 = gen2.GetQuals().Get().size(); 
          if (sz1 != sz2) return false;
          else if (!sz1) return true;
          else {
             Str2Str qual1_vlu;
             ITERATE (list <CRef <CRNA_qual> >, it, gen1.GetQuals().Get()) {
                qual1_vlu[(*it)->GetQual()] = (*it)->GetVal();
             }
             ITERATE (list <CRef <CRNA_qual> >, it, gen2.GetQuals().Get()) {
                if (qual1_vlu.find((*it)->GetQual()) == qual1_vlu.end()) return false;
                else if (qual1_vlu[(*it)->GetQual()] != (*it)->GetVal()) return false;
             }
             return true;
          } 
        }
        else return true;
      }
      else return true;  // e_not_set
    }   
  }
  else return false;

};

void CBioseq_test_on_rna :: FindDupRNAsInList()
{
  unsigned i, j;
  vector <unsigned> is_dup;
  is_dup.reserve(rrna_feat.size());
  for (i=0; i< rrna_feat.size(); i++) is_dup.push_back(0);
  Str2Strs label2dup_rrna;
  string label;

  for (i=0; (int) i < (int)rrna_feat.size() -1; i++) {
    if (is_dup[i]) continue;
    GetSeqFeatLabel(*rrna_feat[i], &label);  // context_label in C
    label2dup_rrna[label].push_back(GetDiscItemText(*rrna_feat[i]));
    is_dup[i] = 1;
    for (j=i+1; j< rrna_feat.size(); j++) {
      if (!is_dup[j] 
           && RRnaMatch(rrna_feat[i]->GetData().GetRna(), rrna_feat[j]->GetData().GetRna())) {
         label2dup_rrna[label].push_back(GetDiscItemText(*rrna_feat[j]));
         is_dup[j] = 1;
      }      
    }
  }

  unsigned cnt;
  ITERATE (Str2Strs, it, label2dup_rrna) 
    ITERATE (vector <string>, jt, it->second) {
      cnt = it->second.size();
      if (cnt > 1) {
         strtmp = NStr::UIntToString(cnt) + " rRNA features on " + m_best_id_str 
                    + " have the same name (" + it->first + ").";
         thisInfo.test_item_list[GetName_rdup()].push_back(strtmp + "$" + *jt);
      }
    }
};


void CBioseq_test_on_rna :: FindtRNAsOnSameStrand()
{
   ENa_strand str0 = trna_feat[0]->GetLocation().GetStrand();
   ENa_strand this_str; 
   vector <string> trnas_same_str;
   bool mixed_strand = false;

   for (unsigned i=1; !mixed_strand && i< trna_feat.size(); i++) {
      this_str = trna_feat[i]->GetLocation().GetStrand();
      if ((str0 == eNa_strand_minus && this_str != eNa_strand_minus)
          || (str0 != eNa_strand_minus && this_str == eNa_strand_minus)) {
          mixed_strand = true;
      }
      else trnas_same_str.push_back(GetDiscItemText(*trna_feat[i]));
   }
  
   string setting_name = GetName_strand();
   if (!mixed_strand) {
     string str_tp = (str0 == eNa_strand_minus) ? "minus" : "plus";
     thisInfo.test_item_list[setting_name].push_back(
                            str_tp + "$" +GetDiscItemText( *trna_feat[0]));
     thisInfo.test_item_list[setting_name].insert( thisInfo.test_item_list[setting_name].end(),
                                trnas_same_str.begin(), trnas_same_str.end()); 
   }
};


void CBioseq_test_on_rna :: TestOnObj(const CBioseq& bioseq)
{
  if (thisTest.is_TRRna_run) return;
 
  // FIND_BADLEN_TRNAS
  unsigned len;
  string trna_desc;
  ITERATE (vector <const CSeq_feat*>, it, trna_feat) {
     len = sequence::GetCoverage((*it)->GetLocation(), thisInfo.scope);
     trna_desc = GetDiscItemText(**it); 
     if ( len > 90) 
        thisInfo.test_item_list[GetName_len()].push_back("long$" + trna_desc);
     else if (len < 50) {
       if ( (*it)->CanGetPartial() && !(*it)->GetPartial())
          thisInfo.test_item_list[GetName_len()].push_back("short$" + trna_desc);
     }
  }

  // below covers COUNT_TRNAS, COUNT_RRNAS, FIND_DUP_RRNAS, FUND_DUP_TRNAS
  if (bioseq.IsAa()) return;
  m_best_id_str = BioseqToBestSeqIdString(bioseq, CSeq_id::e_Genbank);
  m_bioseq_desc = GetDiscItemText(bioseq);

  bool run_test = false;
  int src_genome;
  ITERATE (vector <const CSeqdesc*>, it, bioseq_biosrc_seqdesc) {
    src_genome = (*it)->GetSource().GetGenome(); 
    if ( src_genome == CBioSource ::eGenome_plastid
           || src_genome == CBioSource ::eGenome_mitochondrion
           || src_genome == CBioSource ::eGenome_chloroplast ) {
        run_test = true;
        break;
    }
  }

  string label;
  unsigned cnt;
  if (run_test) {
    cnt = trna_feat.size();
    if (cnt) {
       thisInfo.test_item_list[GetName_tcnt()].push_back(
                                NStr::UIntToString(cnt) + "$" + m_bioseq_desc);
       FindMissingRNAsInList();
       FindtRNAsOnSameStrand();
    }

    cnt = rrna_feat.size();
    if (cnt) {
       thisInfo.test_item_list[GetName_rcnt()].push_back(
                                NStr::UIntToString(cnt) + "$" + m_bioseq_desc);
       FindDupRNAsInList();
    }
  }

  thisTest.is_TRRna_run = true;
};


void CBioseq_FIND_STRAND_TRNAS :: GetReport(CRef <CClickableItem>& c_item)
{
  size_t pos = c_item->item_list[0].find("$");
  strtmp = c_item->item_list[0].substr(0, pos);
  c_item->item_list[0] = c_item->item_list[0].substr(pos+1);
  c_item->description 
      = NStr::UIntToString((unsigned)c_item->item_list.size()) + " tRNAs on " + strtmp + " strand.";
};


void CBioseq_FIND_BADLEN_TRNAS :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs len2trnas;
   GetTestItemList(c_item->item_list, len2trnas);
   c_item->item_list.clear();
   unsigned i=0;
   ITERATE (Str2Strs, it, len2trnas) {
     c_item->item_list = it->second;
     c_item->description =GetIsComment(c_item->item_list.size(), "tRNAs") + "too " + it->first;
     if (i) thisInfo.disc_report_data.push_back(c_item);
     else {
        c_item = CRef <CClickableItem> (new CClickableItem);
        c_item->setting_name = GetName();
     }
     i++;
   }
};


void CBioseq_FIND_DUP_RRNAS :: GetReport(CRef <CClickableItem>& c_item)
{
  Str2Strs label2rrna;
  GetTestItemList(c_item->item_list, label2rrna);
  c_item->item_list.clear();
  unsigned i=1;
  ITERATE (Str2Strs, it, label2rrna) {
    c_item->item_list = it->second;
    c_item->description = it->first;
    if (i > 1) thisInfo.disc_report_data.push_back(c_item);
    if (i != label2rrna.size()) {
         c_item = CRef <CClickableItem> (new CClickableItem);
         c_item->setting_name = GetName();
    }
    i++;
  }
};


void CBioseq_test_on_rna :: GetReport_trna(CRef <CClickableItem>& c_item)
{
   Str2Strs cnt2bioseq;
   GetTestItemList(c_item->item_list, cnt2bioseq);
   c_item->item_list.size();
   unsigned i=1;
   ITERATE (Str2Strs, it, cnt2bioseq) {
     c_item->item_list = it->second;
     if (isInt(it->first)) {
         c_item->description = GetHasComment(c_item->item_list.size(), "sequence") + it->first 
                               + " tRNA " + ((it->first== "1")? "feature." : "features.");
     }
     else c_item->description = it->first;
     if (i > 1) thisInfo.disc_report_data.push_back(c_item);
     if (i != cnt2bioseq.size()) {
         c_item = CRef <CClickableItem> (new CClickableItem);
         c_item->setting_name = GetName_tcnt(); 
     }
     i++;
   } 
};
void CBioseq_COUNT_TRNAS :: GetReport(CRef <CClickableItem>& c_item)
{
    GetReport_trna(c_item);
};
void CBioseq_FIND_DUP_TRNAS :: GetReport(CRef <CClickableItem>& c_item)
{
    GetReport_trna(c_item);
};



void CBioseq_COUNT_RRNAS :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs cnt2bioseq;
   GetTestItemList(c_item->item_list, cnt2bioseq);
   c_item->item_list.size();
   unsigned i=1;
   ITERATE (Str2Strs, it, cnt2bioseq) {
     c_item->item_list = it->second;
     if (isInt(it->first)) {
         c_item->description = GetHasComment(c_item->item_list.size(), "sequence") + it->first
                               + " rRNA " + ((it->first== "1")? "feature." : "features.");
     }
     else c_item->description = it->first;
     if (i > 1) thisInfo.disc_report_data.push_back(c_item);
     if (i != cnt2bioseq.size()) {
         c_item = CRef <CClickableItem> (new CClickableItem);
         c_item->setting_name = GetName();
     }
     i++;
   }
};


void CBioseq_test_on_molinfo :: TestOnObj(const CBioseq& bioseq)
{
   if (thisTest.is_MolInfo_run) return;

   ITERATE (vector <const CSeqdesc*>, it, bioseq_molinfo) {
      const CMolInfo& molinfo = (*it)->GetMolinfo();     
      // PARTIAL_CDS_COMPLETE_SEQUENCE
      if (molinfo.GetCompleteness() == CMolInfo::eCompleteness_complete) {
         ITERATE (vector <const CSeq_feat*>, jt, cd_feat) {
           const CSeq_loc& seq_loc = (*jt)->GetLocation();
           if ( ((*jt)->CanGetPartial() && (*jt)->GetPartial())
                  || seq_loc.IsPartialStart(eExtreme_Biological)
                  || seq_loc.IsPartialStop(eExtreme_Biological) ) {
              thisInfo.test_item_list[GetName_part()].push_back(GetDiscItemText(**jt));
           }
         }
      }
   }

   string desc(GetDiscItemText(bioseq));
   if (bioseq.IsNa()) {
     ITERATE (vector <const CSeqdesc*>, it, bioseq_molinfo) {
        const CMolInfo& molinfo = (*it)->GetMolinfo();
       // MOLTYPE_NOT_MRNA
       if ( (molinfo.GetBiomol() != CMolInfo :: eBiomol_mRNA)) 
           thisInfo.test_item_list[GetName_mrna()].push_back(desc);

       // TECHNIQUE_NOT_TSA
       if ( molinfo.GetTech() != CMolInfo :: eTech_tsa)
           thisInfo.test_item_list[GetName_tsa()].push_back(desc);
     }
   }
   else if (bioseq.GetInst().GetMol() == CSeq_inst::eMol_rna) {
      // DISC_POSSIBLE_LINKER
      if (!IsMrnaSequence()
            || (bioseq.IsSetLength() && bioseq.GetLength() < 30) ) return;
      else {
        CSeqVector seq_vec = thisInfo.scope->GetBioseqHandle(bioseq).GetSeqVector(
                                             CBioseq_Handle::eCoding_Iupac, eNa_strand_plus);
        unsigned tail_len = 0;
        bool found_linker = false;
        for (CSeqVector_CI seq_ci(seq_vec.end());  
                               !found_linker && (seq_ci > seq_vec.end() - 30); -- seq_ci) {
           if (*seq_ci == 'A') tail_len++;
           else if (tail_len > 20) found_linker = true;
           else tail_len = 0;
        }
        if (!found_linker)
           thisInfo.test_item_list[GetName_link()].push_back(desc);
      }
   }

   thisTest.is_MolInfo_run = true;
};


void CBioseq_DISC_POSSIBLE_LINKER :: GetReport(CRef <CClickableItem>& c_item)
{ 
  c_item->description = GetHasComment(c_item->item_list.size(), "bioseq") 
                         + "linker sequence after the poly-A tail";
};


void CBioseq_PARTIAL_CDS_COMPLETE_SEQUENCE :: GetReport(CRef <CClickableItem>& c_item)
{
   unsigned cnt = c_item->item_list.size();
   c_item->description 
      = GetIsComment(cnt, "partial CDS") + "in complete " + GetNoun(cnt, "sequence");
};


void CBioseq_MOLTYPE_NOT_MRNA :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
      = GetIsComment(c_item->item_list.size(), "molecule type") + "not set as mRNA.";
};


void CBioseq_TECHNIQUE_NOT_TSA :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description
      = GetIsComment(c_item->item_list.size(), "technique") + "not set as TSA.";
};


void CBioseq_test_on_all_annot :: TestOnObj(const CBioseq& bioseq)
{
  if (thisTest.is_AllAnnot_run) return;
  
  bool excpt;
  string excpt_txt(kEmptyStr);
  string desc( GetDiscItemText(bioseq));

  if (all_feat.empty()) {
    thisInfo.test_item_list[GetName_no()].push_back(desc);
    if (bioseq.IsNa() && bioseq.GetLength() >= 5000)
      thisInfo.test_item_list[GetName_long_no()].push_back(desc);
  }
  else {
     string label, desc;
     ITERATE (vector <const CSeq_feat*>, it, all_feat) {
        desc = GetDiscItemText(**it);
        // DISC_FEATURE_LIST
        // CD replaces Prot
        if ( (*it)->GetData().IsProt() 
             || (*it)->GetData().GetSubtype() == CSeqFeatData::eSubtype_gap) continue; 
        label = (*it)->GetData().GetKey();
        label = label.empty()? "unknown$" : label + "$";
        thisInfo.test_item_list[GetName_ls()].push_back(label + desc);

        // ONCALLER_ORDERED_LOCATION
        if (LocationHasNullsBetween(*it))
           thisInfo.test_item_list[GetName_loc()].push_back(desc);

        // ONCALLER_HAS_STANDARD_NAME
        if ( !((*it)->CanGetQual()) ) continue;
        ITERATE (vector <CRef< CGb_qual > >, jt, (*it)->GetQual()) {
          if ( (*jt)->GetQual() == "standard_name" )
             thisInfo.test_item_list[GetName_std()].push_back(desc);
        }
     }
   

    // JOINED_FEATURES
    if (IsBioseqHasLineage(bioseq, "Eukaryota", false)) return;
    ITERATE (vector <const CSeq_feat*>, it, all_feat) {
       if ( (*it)->GetLocation().IsMix() || (*it)->GetLocation().IsPacked_int()) {
          if ((*it)->CanGetExcept()) {
              if ((*it)->GetExcept()) excpt = true;
              else excpt = false;
              if ((*it)->CanGetExcept_text()) excpt_txt = (*it)->GetExcept_text();
              excpt_txt = excpt_txt.empty()? "blank" : excpt_txt;
          }
          else excpt = false;
          desc = GetDiscItemText(**it);
          if (excpt) JOINED_FEATURES_sfs[excpt_txt].push_back(desc);
          else JOINED_FEATURES_sfs["no"].push_back(desc);
          thisInfo.test_item_list[GetName_joined()].push_back(desc);
       }
    }
  }
  thisTest.is_AllAnnot_run = true;
};


void CBioseq_ONCALLER_HAS_STANDARD_NAME :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
        = GetHasComment(c_item->item_list.size(), "feature") + "standard_name qualifier";
};


void CBioseq_DISC_FEATURE_LIST :: GetReport(CRef <CClickableItem>& c_item)
{
  Str2Strs feat2ls;
  GetTestItemList(c_item->item_list, feat2ls);
  c_item->item_list.clear();
  ITERATE (Str2Strs, it, feat2ls) {
     strtmp = it->first + " feature";
     AddSubcategories(c_item,GetName(),it->second,strtmp, strtmp + "s", e_OtherComment, false);
  }
  c_item->description = "Feature List";
};


bool CBioseq_test_on_all_annot :: LocationHasNullsBetween(const CSeq_feat* seq_feat)
{
  for (CSeq_loc_CI loc_it(seq_feat->GetLocation()); loc_it; ++ loc_it) {
     if (loc_it.GetEmbeddingSeq_loc().IsNull()) return true;
 // is it = loc_it.IsEmpty()?
  }
  return false;
};


void CBioseq_ONCALLER_ORDERED_LOCATION :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description 
    = GetHasComment(c_item->item_list.size(), "feature") + "ordered locations";
};


void CBioseq_JOINED_FEATURES :: GetReport(CRef <CClickableItem>& c_item)
{
   string cmt_txt;
   ITERATE (Str2Strs, it, JOINED_FEATURES_sfs) {
      CRef <CClickableItem> c_sub (new CClickableItem);
      if (it->first == "no") cmt_txt = "but no exception";
      else if (it->first == "empty") cmt_txt = "but a blank exception";
      else cmt_txt = "but exception " + it->first;
      c_sub->setting_name = GetName();
      c_sub->item_list = it->second;
      c_sub->description = GetHasComment(c_sub->item_list.size(), "feature")
                                + "joined location " + cmt_txt;
      c_item->subcategories.push_back(c_sub);
   }

   c_item->description
        = GetHasComment(c_item->item_list.size(), "feature") + "joined location.";
};


void CBioseq_NO_ANNOTATION :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "bioseq") + "no features";
};

void CBioseq_DISC_LONG_NO_ANNOTATION :: GetReport(CRef <CClickableItem>& c_item)
{
  unsigned len = c_item->item_list.size();
  c_item->description = GetIsComment(len, "bioseq") + "longer than 5000nt and " 
                               + ( (len == 1)? "has" : "have") + " no features.";
};

//


void CBioseq_MISSING_PROTEIN_ID :: TestOnObj(const CBioseq& bioseq)
{
   if (thisInfo.test_item_list.find(GetName_prefix()) == thisInfo.test_item_list.end())
     TestProteinID(bioseq);
};


void CBioseq_MISSING_PROTEIN_ID :: GetReport(CRef <CClickableItem>& c_item)
{
  unsigned cnt = c_item->item_list.size();
  c_item->description = GetHasComment(cnt, "protein") 
                         + "invalid " + ( (cnt > 1)? "IDs." : "ID." ); 
};


//not tested yet
void CBioseq_INCONSISTENT_PROTEIN_ID_PREFIX :: TestOnObj(const CBioseq& bioseq)
{
   if (thisInfo.test_item_list.find(GetName_pid()) == thisInfo.test_item_list.end())
     TestProteinID(bioseq);
};


void CBioseq_INCONSISTENT_PROTEIN_ID_PREFIX :: MakeRep(const Str2Strs& item_map, const string& desc1, const string& desc2) 
{
   string setting_name = GetName();
   ITERATE (Str2Strs, it, item_map) {
     if (it->second.size() > 1) {
       CRef <CClickableItem> c_item (new CClickableItem);
       c_item->setting_name = setting_name;
       c_item->item_list = it->second;
       c_item->description 
          = GetHasComment(c_item->item_list.size(), desc1) + desc2 + it->first + ".";
       thisInfo.disc_report_data.push_back(c_item);
     }
   }
};


void CBioseq_INCONSISTENT_PROTEIN_ID_PREFIX :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs db2list, prefix2list;
   GetTestItemList(c_item->item_list, db2list);

   GetTestItemList(c_item->item_list, prefix2list);
   c_item->item_list.clear();

   // ReportInconsistentGlobalDiscrepancyStrings
   if (db2list.size() > 1) MakeRep(db2list, "sequence", "protein ID prefix ");

   // ReportInconsistentGlobalDiscrepancyPrefixes
   if (prefix2list.size() > 1) MakeRep(prefix2list, "feature", "locus tag prefix ");
};




void CBioseq_TEST_DEFLINE_PRESENT :: TestOnObj(const CBioseq& bioseq)
{
  CConstRef <CSeqdesc> seqdesc_title = bioseq.GetClosestDescriptor(CSeqdesc::e_Title);
  if (seqdesc_title.NotEmpty()) {
     thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(bioseq));
     ITERATE(vector < CRef < CTestAndRepData > >, it, CRepConfig :: tests_on_Bioseq_na) {
       if ( (*it)->GetName() == GetName()) {
             CRepConfig :: tests_4_once.push_back(*it);
             break;
       }
     }
  } 
};


void CBioseq_TEST_DEFLINE_PRESENT :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "Bioseq") + "definition line";
}



void CBioseq_DISC_QUALITY_SCORES :: TestOnObj(const CBioseq& bioseq)
{
  if (bioseq.IsNa() && bioseq.CanGetAnnot()) {
    unsigned cnt = 0;
    ITERATE (list <CRef <CSeq_annot> >, it, bioseq.GetAnnot()) {
       if ( (*it)->GetData().IsGraph() ) { DISC_QUALITY_SCORES_graph |= 1; break; }
       cnt ++;
    }

    if (cnt == bioseq.GetAnnot().size()) DISC_QUALITY_SCORES_graph |= 2; 
  }
};


void CBioseq_DISC_QUALITY_SCORES ::  GetReport(CRef <CClickableItem>& c_item)
{
  if (DISC_QUALITY_SCORES_graph == 1) 
    c_item->description = "Quality scores are present on all sequences.";
  else if (DISC_QUALITY_SCORES_graph == 2) 
    c_item->description = "Quality scores are missing on all sequences.";
  else if (DISC_QUALITY_SCORES_graph == 3) 
    c_item->description = "Quality scores are missing on some sequences.";
};



void CBioseq_RNA_CDS_OVERLAP :: TestOnObj(const CBioseq& bioseq)
{
  string text_i, text_j, subcat_tp;
  ENa_strand strand_i, strand_j;
  ITERATE (vector <const CSeq_feat*>, it, rna_not_mrna_feat) {
    ITERATE (vector <const CSeq_feat*>, jt, cd_feat) {
      const CSeq_loc& loc_i = (*it)->GetLocation();
      const CSeq_loc& loc_j = (*jt)->GetLocation();
      sequence::ECompare ovlp_com = Compare(loc_j, loc_i, thisInfo.scope);
      subcat_tp = kEmptyStr;
      switch (ovlp_com) {
        case eSame: subcat_tp = "extrc$"; break;
        case eContained: subcat_tp = "cds_in_rna$"; break;
        case eContains: subcat_tp = "rna_in_cds$"; break;
        case eOverlap: {
           strand_i = loc_i.GetStrand();
           strand_j = loc_j.GetStrand();
           if ((strand_i == eNa_strand_minus && strand_j != eNa_strand_minus)
                       || (strand_i != eNa_strand_minus && strand_j == eNa_strand_minus))
                subcat_tp = "overlap_opp_strand$";
           else subcat_tp = "overlap_same_strand$";
           break;
        }
        default:;
      }
      if (!subcat_tp.empty()) {
          thisInfo.test_item_list[GetName()].push_back(subcat_tp + GetDiscItemText(**jt));
          thisInfo.test_item_list[GetName()].push_back(subcat_tp + GetDiscItemText(**it));
      }
    }
  }
}; //  CBioseq_RNA_CDS_OVERLAP :: TestOnObj



void CBioseq_RNA_CDS_OVERLAP :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs subcat2list;
   GetTestItemList(c_item->item_list, subcat2list);
   c_item->item_list.clear();

   string desc1, desc2, desc3;
   CRef <CClickableItem> ovp_subcat (new CClickableItem);
   ITERATE (Str2Strs, it, subcat2list) {
     if (it->first.find("overlap") != string::npos) {
       if (it->first == "exact") {
         desc1 = "coding region exactly matches";
         desc2 = "coding regions exactly match";
         desc3 = " an RNA location";
       }
       else if (it->first == "cds_in_rna") {
         desc1 = "coding region is";
         desc2 = "coding regions are";
         desc3 = " completely contained in RNA location";
       }
       else if (it->first == "rna_in_cds" ) {
         desc1 = "coding region completely contains";
         desc2 = "coding regions completely contain";
         desc3 = " RNAs";
       }
       AddSubcategories(c_item, GetName(), it->second, desc1, desc2, e_OtherComment, true, 
                           desc3, true); 
     }
     else {
       desc1 = "coding region overlaps";
       desc2 = "coding regions overlap";
       if (it->first == "overlap_opp_strand") 
             desc3 = " RNAs on the opposite strand (no containment)";
       else desc3 = " RNAs on the same strand (no containment)";
       AddSubcategories(ovp_subcat, GetName(), it->second, desc1, desc2, e_OtherComment, true,
                        desc3, true); 
     } 
   }
   if (!ovp_subcat->item_list.empty()) {
      ovp_subcat->setting_name = GetName();
      ovp_subcat->description 
         = GetOtherComment(ovp_subcat->item_list.size(), "coding region exactly matches",
                             "coding regions exactly match") 
             + " an RNA location";
      c_item->item_list.insert(
         c_item->item_list.end(), ovp_subcat->item_list.begin(), ovp_subcat->item_list.end());
      c_item->subcategories.push_back(ovp_subcat);
   }
    
   c_item->description 
        = GetOtherComment(c_item->item_list.size()/2, "coding region overlapps", 
                           "coding regions overlap") 
           + " RNA features";
}; // CBioseq_RNA_CDS_OVERLAP :: GetReport



bool CBioseq_OVERLAPPING_CDS :: OverlappingProdNmSimilar(const string& prod_nm1, const string& prod_nm2)
{
  bool str1_has_similarity_word = false, str2_has_similarity_word = false;
  vector < string > simi_prod_wds;
  simi_prod_wds.push_back("transposase");
  simi_prod_wds.push_back("integrase");  

  vector < string > ignore_simi_prod_wds;
  ignore_simi_prod_wds.push_back("hypothetical protein");
  ignore_simi_prod_wds.push_back("phage");
  ignore_simi_prod_wds.push_back("predicted protein");

  unsigned i;
  /* if both product names contain one of the special case similarity words,
   * the product names are similar. */
  
  for (i = 0; i < simi_prod_wds.size(); i++)
  {
    if (!str1_has_similarity_word 
          && string::npos != NStr::FindNoCase(prod_nm1, simi_prod_wds[i]))
         str1_has_similarity_word = true;

    if (!str2_has_similarity_word
           && string::npos != NStr::FindNoCase(prod_nm2, simi_prod_wds[i]))
        str2_has_similarity_word = true;
  }
  if (str1_has_similarity_word && str2_has_similarity_word) return true;
  
  /* otherwise, if one of the product names contains one of special ignore similarity
   * words, the product names are not similar.
   */
  for (i = 0; i < ignore_simi_prod_wds.size(); i++)
     if (string::npos != NStr::FindNoCase(prod_nm1, ignore_simi_prod_wds[i])
           || string::npos != NStr::FindNoCase(prod_nm2, ignore_simi_prod_wds[i]))
         return false;

  if (!(NStr::CompareNocase(prod_nm1, prod_nm2))) return true;
  else return false;

}; // OverlappingProdNmSimilar



void CBioseq_OVERLAPPING_CDS :: AddToDiscRep(const CSeq_feat* seq_feat)
{
     string desc = GetDiscItemText(*seq_feat);
     if (seq_feat->CanGetComment()) {
         const string& comm = seq_feat->GetComment();
         if ( string::npos != NStr::FindNoCase(comm, "overlap")
              || string::npos != NStr::FindNoCase(comm, "frameshift")
              || string::npos != NStr::FindNoCase(comm, "frame shift")
              || string::npos != NStr::FindNoCase(comm, "extend") ) 
         thisInfo.test_item_list[GetName()].push_back("comment$" + desc);
     }
     else thisInfo.test_item_list[GetName()].push_back("no_comment$" + desc);
}; // AddToDiscRep



bool CBioseq_OVERLAPPING_CDS :: HasNoSuppressionWords(const CSeq_feat* seq_feat)
{
  string prod_nm;
  if (seq_feat->GetData().GetSubtype() == CSeqFeatData::eSubtype_cdregion) {
     prod_nm = GetProdNmForCD(*seq_feat);
     if ( IsWholeWord(prod_nm, "ABC")
            || string::npos != NStr::FindNoCase(prod_nm, "transposon")
            || string::npos != NStr::FindNoCase(prod_nm, "transposase"))
          return false;
     else return true;
  }
  else return false;
}; // HasNoSuppressionWords()



void CBioseq_OVERLAPPING_CDS :: TestOnObj(const CBioseq& bioseq)
{
  bool sf_i_processed = false;
  int i, j;
  for (i=0; (int)i< (int)(cd_feat.size() - 1); i++) {
    sf_i_processed = false;
    for (j = i+1; j < (int)cd_feat.size(); j++) {
      const CSeq_loc& loc_i = cd_feat[i]->GetLocation();
      const CSeq_loc& loc_j = cd_feat[j]->GetLocation();
      if (loc_i.GetStop(eExtreme_Positional) < loc_j.GetStart(eExtreme_Positional)) break;
      if (!OverlappingProdNmSimilar(GetProdNmForCD(*(cd_feat[i])), 
                                    GetProdNmForCD(*(cd_feat[j])))) 
          continue;
      ENa_strand strand1 = loc_i.GetStrand();
      ENa_strand strand2 = loc_j.GetStrand();
      if ((strand1 == eNa_strand_minus && strand2 != eNa_strand_minus)
          || (strand1 != eNa_strand_minus && strand2 == eNa_strand_minus)) continue;
      if ( eNoOverlap != Compare(loc_i, loc_j, thisInfo.scope)) {
          if (!sf_i_processed) {
              if ( HasNoSuppressionWords(cd_feat[i])) AddToDiscRep(cd_feat[i]);
              sf_i_processed = true;
          }
          if ( HasNoSuppressionWords(cd_feat[j]) ) AddToDiscRep(cd_feat[j]);
      }
    }  
  }
}


void CBioseq_OVERLAPPING_CDS :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs subcat2list;
   string desc1, desc2, desc3;
   GetTestItemList(c_item->item_list, subcat2list);
   c_item->item_list.clear();
   ITERATE (Str2Strs, it, subcat2list) {
     if (it->first == "comment") {
       desc1 = "coding region overlapps";
       desc2 = "coding regions overlap";
       desc3 = " another coding region with a similar or identical name that do not have the appropriate note text";
     }
     else {
       desc1 = "coding region overlapps";
       desc2 = "coding regions overlap";
       desc3 = " another coding region with a similar or identical name but have the appropriate note text";
     }
     AddSubcategories(c_item, GetName(), it->second, desc1, desc2, e_OtherComment, true,desc3);
   }
   c_item->description 
     = GetOtherComment(c_item->item_list.size(), "coding region overlapps", 
                                                                "coding regions overlap")
        + " another coding region with a similar or identical name.";
}; // OVERLAPPING_CDS



// new comb
void CBioseq_test_on_missing_genes :: CheckGenesForFeatureType (const vector <const CSeq_feat*>& feats, bool makes_gene_not_superfluous)
{
  unsigned i=0, j;
  ITERATE (vector <const CSeq_feat*>, it, feats) {
    if ((*it)->GetData().IsGene()) continue;
    const CGene_ref* xref_gene = (*it)->GetGeneXref();
    if (xref_gene && xref_gene->IsSuppressed()) {
         if (m_no_genes[i].empty()) m_no_genes[i] = GetDiscItemText(**it);
    }
    else {
        if (xref_gene) {
           if (m_super_cnt) {
             j = 0;
             ITERATE (vector <const CSeq_feat*>, jt, gene_feat) {
               if (m_super_idx[j] &&
                     GeneRefMatchForSuperfluousCheck((*jt)->GetData().GetGene(), xref_gene)) {
                   m_super_cnt --;
                   m_super_idx[j] = 0;
               }
               j++;
             }
           }
        } 
        else {
          // GetBest... can't deal with the mix strand now,
          // so for File1fordiscrep.sqn, it needs to check the EXTRA_GENES
          // and MISSING_GENES after the bug is fixed.
          CConstRef < CSeq_feat>
             gene_olp= GetBestOverlappingFeat( (*it)->GetLocation(),
                                               CSeqFeatData::e_Gene,
                                               eOverlap_Contained,
                                               *thisInfo.scope);

          if (gene_olp.Empty()) {
             if (m_no_genes[i].empty()) m_no_genes[i] = GetDiscItemText(**it);
          }
          else if ( m_super_cnt && makes_gene_not_superfluous) {
             j=0;
             ITERATE (vector <const CSeq_feat*>, jt, gene_feat) {
               if (m_super_idx[j] && (*jt == gene_olp.GetPointer())) {
                    m_super_idx[j] = 0;
                    m_super_cnt --;
               }
               j++;
             }
          }
        }
    }
    i++;
  } 
};


void CBioseq_missing_genes_regular :: TestOnObj(const CBioseq& bioseq)
{
  unsigned i;
  if (IsmRNASequenceInGenProdSet(bioseq)) m_super_cnt = 0;
  else {
     m_super_cnt = gene_feat.size();
     m_no_genes.reserve(m_super_cnt);
     m_super_idx.reserve(m_super_cnt);
     for (i=0; i< m_super_cnt; i++) m_super_idx[i] = 1;
  }
  CheckGenesForFeatureType(cd_feat, true); 
  CheckGenesForFeatureType(rna_feat, true);
  CheckGenesForFeatureType(rbs_feat);
  CheckGenesForFeatureType(exon_feat);
  CheckGenesForFeatureType(intron_feat);

  ITERATE (vector <string>, it, m_no_genes) {
     if (!(*it).empty())
       thisInfo.test_item_list[GetName_missing()].push_back(*it);
  } 

  //GetPseudoAndNonPseudoGeneList;
  if (!m_super_cnt) return;
  i=0;
  string desc, nm_desc;
  ITERATE (vector <const CSeq_feat* >, it, gene_feat) {
     if (m_super_idx[i++]) {
       desc = GetDiscItemText(**it);
       thisInfo.test_item_list[GetName_extra()].push_back("extra$" + desc);
       nm_desc = kEmptyStr;
       const CSeqFeatData& seq_feat_dt = (*it)->GetData();
       if ((*it)->CanGetPseudo() && (*it)->GetPseudo()) nm_desc = "pseudo$" + desc;
       else if ( seq_feat_dt.GetGene().GetPseudo() ) nm_desc = "pseudo$" + desc;
       else {
             //GetFrameshiftAndNonFrameshiftGeneList
             if ((*it)->CanGetComment()) {
               const string& comment = (*it)->GetComment();
               if (string::npos != NStr::FindNoCase(comment, "frameshift")
                    || string::npos!= NStr::FindNoCase(comment, "frame shift"))
                   nm_desc = "frameshift$" + desc;
               else nm_desc = "nonshift$" + desc;
             }
       }
       if (!nm_desc.empty()) thisInfo.test_item_list[GetName_extra()].push_back(nm_desc);
     }
  }
};


void CBioseq_MISSING_GENES :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description
    = GetHasComment(c_item->item_list.size(), "feature") + "no genes.";
};


void CBioseq_EXTRA_GENES :: GetReport(CRef <CClickableItem>& c_item)
{
  string desc1, desc2;
  Str2Strs tp2ls;
  GetTestItemList(c_item->item_list, tp2ls);
  c_item->item_list.clear();
  ITERATE (Str2Strs, it, tp2ls) {
    if (it->first == "extra") c_item->item_list = it->second;
    else {
      if (it->first == "pseudo") {
         desc1 = "pseudo gene feature";
         desc2 = "not associated with a CDS or RNA feature.";
      }
      else if (it->first == "frameshift") {
         desc1 = "non-pseudo gene feature";
         desc2
         ="not associated with a CDS or RNA feature and have frameshift in the comment.";
      }
      else if (it->first == "nonshift") {
            desc1 = "non-pseudo gene feature";
            strtmp = (it->second.size() > 1) ? "do" : "does";
            desc2 = "not associated with a CDS or RNA feature and "
                              + strtmp + " not have frameshift in the comment.";
      }
      AddSubcategories(c_item,  GetName(), it->second, desc1, desc2, e_IsComment, false);
    }
  };
  c_item->description = GetIsComment(c_item->item_list.size(), "gene feature") 
                        + "not associated with a CDS or RNA feature.";
};


void CBioseq_missing_genes_oncaller :: TestOnObj(const CBioseq& bioseq)
{
  if (bioseq.IsAa()) return;
  m_super_cnt = 0;
  unsigned i=0;
  m_no_genes.reserve(gene_feat.size());
  m_super_idx.reserve(gene_feat.size());
  if (!IsmRNASequenceInGenProdSet(bioseq)) {
     ITERATE (vector <const CSeq_feat*>, it, gene_feat) {
       if ( (*it)->CanGetPseudo() && (*it)->GetPseudo()) m_super_idx[i] = 0;
       else {
          m_super_cnt ++;
          m_super_idx[i] = 1;
       }
       i++; 
     }
  }

  CheckGenesForFeatureType(cd_feat, true);
  CheckGenesForFeatureType(mrna_feat, true);
  CheckGenesForFeatureType(trna_feat, true);

  ITERATE (vector <string>, it, m_no_genes) 
    if ( !(*it).empty())
        thisInfo.test_item_list[GetName_missing()].push_back(*it);

  m_no_genes.clear();
  CheckGenesForFeatureType(all_feat, true);
  m_no_genes.clear();
  if (!m_super_cnt) return;
  i=0;
  ITERATE (vector <const CSeq_feat*>, it, gene_feat) {
    if (!m_super_idx[i]) continue;
    /* remove genes with explanatory comments/descriptions */
    if (!IsOkSuperfluousGene( *it )) 
       thisInfo.test_item_list[GetName_extra()].push_back(GetDiscItemText(**it));
    i++;
  } 
};


bool CBioseq_missing_genes_oncaller :: IsOkSuperfluousGene (const CSeq_feat* seq_feat)
{
  string phrase = "coding region not determined";
  if (seq_feat->CanGetComment() && CommentHasPhrase(seq_feat->GetComment(), phrase))
     return true;
  else if (seq_feat->GetData().GetGene().CanGetDesc()
             && CommentHasPhrase(seq_feat->GetData().GetGene().GetDesc(), phrase))
     return true;
  else return false;
};


void CBioseq_ONCALLER_GENE_MISSING :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description
    = GetHasComment(c_item->item_list.size(), "feature") + "no genes.";
};


void CBioseq_ONCALLER_SUPERFLUOUS_GENE :: GetReport(CRef <CClickableItem>& c_item)
{
  unsigned cnt = c_item->item_list.size();
  c_item->description
    = GetIsComment(cnt, "gene feature") + "not associated with any feature and " 
          + ((cnt > 1) ? "are" : "is") + " not pseudo.";
}

// new comb

/*
void CBioseq_EXTRA_MISSING_GENES :: TestOnObj(const CBioseq& bioseq)
{
  vector <int> super_idx;
  unsigned super_cnt = gene_feat.size();
  unsigned i;
  for (i=0; i< super_cnt; i++) super_idx.push_back(1);
  set <const CSeq_feat*> features_without_genes;
  ITERATE ( vector < const CSeq_feat* > , it, mix_feat) {
     const CGene_ref* xref_gene = (*it)->GetGeneXref();
     if (xref_gene && xref_gene->IsSuppressed() 
                && features_without_genes.find(*it) == features_without_genes.end())
          features_without_genes.insert(*it);
     else {
        if (xref_gene) {
           if (super_cnt) {
             i = 0;
             ITERATE (vector <const CSeq_feat*>, jt, gene_feat) {
               if (super_idx[i] &&
                      GeneRefMatchForSuperfluousCheck((*jt)->GetData().GetGene(), xref_gene)) {
                   super_cnt --;
                   super_idx[i] = 0;
               }
               i++;
             } 
           }
        }
        else {
          // GetBest... can't deal with the mix strand now, 
          // so for File1fordiscrep.sqn, it needs to check the EXTRA_GENES
          // and MISSING_GENES after the bug is fixed.
          CConstRef < CSeq_feat> 
             gene_olp= GetBestOverlappingFeat( (*it)->GetLocation(), 
                                               CSeqFeatData::e_Gene, 
                                               eOverlap_Contained, 
                                               *thisInfo.scope);

          if (gene_olp.Empty() && features_without_genes.find(*it) == features_without_genes.end()) 
               features_without_genes.insert(*it);
          else if ( super_cnt && ((*it)->GetData().IsCdregion() || (*it)->GetData().IsRna()) ){
             i=0;
             ITERATE (vector <const CSeq_feat*>, jt, gene_feat) {
               if (super_idx[i] && (*jt == gene_olp.GetPointer())) {
                    super_idx[i] = 0;
                    super_cnt --;
               }
               i++;
             } 
          }
        }
     }
  }

  ITERATE (set <const CSeq_feat* >, it, features_without_genes) 
     thisInfo.test_item_list[GetName()].push_back(GetName_missing() + "$" + GetDiscItemText(*(*it)));

//GetPseudoAndNonPseudoGeneList;
  if (!super_cnt) return;
  vector < CSeq_feat* > pseudo_ls, frameshift_ls, non_frameshift_lframeshift_ls, non_frameshift_ls;
  i = 0;
  string desc, nm_desc;
  ITERATE (vector <const CSeq_feat* >, it, gene_feat) {
     if (super_idx[i++]) {
       desc = GetDiscItemText(**it);
       thisInfo.test_item_list[GetName()].push_back(GetName_extra() + "$" + desc);
       nm_desc = kEmptyStr;
       const CSeqFeatData& seq_feat_dt = (*it)->GetData();
       if ((*it)->CanGetPseudo() && (*it)->GetPseudo())
              nm_desc = GetName_extra_pseudodp() + "$" + desc;
       else if ( seq_feat_dt.GetGene().GetPseudo() ) 
                 nm_desc = GetName_extra_pseudodp() + "$" + desc;
       else {
             //GetFrameshiftAndNonFrameshiftGeneList
             if ((*it)->CanGetComment()) {
               const string& comment = (*it)->GetComment();
               if (string::npos != NStr::FindNoCase(comment, "frameshift")
                    || string::npos!= NStr::FindNoCase(comment, "frame shift"))
                   nm_desc = GetName_extra_frameshift() + "$" + desc;
               else nm_desc = GetName_extra_nonshift() + "$" + desc;
             } 
       }
       if (!nm_desc.empty()) thisInfo.test_item_list[GetName()].push_back(nm_desc);
     }
  }
};  // EXTRA_MISSING_GENES :: TestOnObj

*/


//bool CBioseq_EXTRA_MISSING_GENES :: GeneRefMatchForSuperfluousCheck (const CGene_ref& gene, const CGene_ref* g_xref)
bool CBioseq_test_on_missing_genes :: GeneRefMatchForSuperfluousCheck (const CGene_ref& gene, const CGene_ref* g_xref)
{
  const string& locus1 = (gene.CanGetLocus()) ? gene.GetLocus() : kEmptyStr;
  const string& locus_tag1 = (gene.CanGetLocus_tag()) ? gene.GetLocus_tag() : kEmptyStr;

  const string& locus2 = (g_xref->CanGetLocus()) ? g_xref->GetLocus() : kEmptyStr;
  const string& locus_tag2 = (g_xref->CanGetLocus_tag()) ? g_xref->GetLocus_tag() : kEmptyStr;

  if (locus1 != locus2 || locus_tag1 != locus_tag2 
          || (gene.GetPseudo() != g_xref->GetPseudo()) ) return false;
  else {
    const string& allele1 = (gene.CanGetAllele()) ? gene.GetAllele(): kEmptyStr;
    const string& allele2 = (g_xref->CanGetAllele()) ? g_xref->GetAllele(): kEmptyStr;
    if (allele1.empty() || allele2.empty()) return true;
    else if (allele1 != allele2) return false;
    else {
      const string& desc1 = (gene.CanGetDesc())? gene.GetDesc() : kEmptyStr;
      const string& desc2 = (g_xref->CanGetDesc())? g_xref->GetDesc() : kEmptyStr;
      if (desc1.empty() || desc2.empty()) return true;
      else if (desc1 != desc2) return false;
      else {
        const string& maploc1 = (gene.CanGetMaploc())? gene.GetMaploc(): kEmptyStr;
        const string& maploc2= (g_xref->CanGetMaploc())? g_xref->GetMaploc(): kEmptyStr; 
        if (!maploc1.empty() && !maploc2.empty() && maploc1 != maploc2) 
             return false; 
        else return true;
      }      
    } 
  }
};


/*
void CBioseq_EXTRA_MISSING_GENES :: GetReport(CRef <CClickableItem>& c_item)
{
    bool has_missing = false;
    Str2Strs setting2list;
    string desc1(kEmptyStr), desc2(kEmptyStr);
    GetTestItemList(c_item->item_list, setting2list);
    if (setting2list.find(GetName_missing()) != setting2list.end()) has_missing = true;
    c_item->item_list.clear();
  
    CRef <CClickableItem> c_extra;
    if (has_missing) c_extra = CRef <CClickableItem> (new CClickableItem);
    else c_extra = c_item;
    ITERATE (Str2Strs, it, setting2list) {
      if (it->first == GetName_missing()) {
        c_item->setting_name = GetName_missing();
        c_item->item_list = it->second;
        c_item->description = GetHasComment(c_item->item_list.size(), "feature") + "no gene"; 
      }
      else {
        if (it->first.find("pseudodp") != string::npos) {
          desc1 = "pseudo gene feature"; 
          desc2 = "not associated with a CDS or RNA feature.";
          AddSubcategories(c_extra,  GetName(), it->second, desc1, desc2);
        }
        else if (it->first.find("frameshift") != string::npos) {
            desc1 = "non-pseudo gene feature";
            desc2
         ="not associated with a CDS or RNA feature and have frameshift in the comment.";
            AddSubcategories(c_extra,  GetName(), it->second, desc1, desc2);
        }
        else if (it->first.find("nonshift") != string::npos) {
            desc1 = "non-pseudo gene feature";
            strtmp = (it->second.size() > 1) ? "do" : "does";
            desc2 = "not associated with a CDS or RNA feature and " 
                              + strtmp + " not have frameshift in the comment.";
            AddSubcategories(c_extra,  GetName(), it->second, desc1, desc2);
        }
        else c_extra->item_list.insert(c_extra->item_list.end(), 
                                                      it->second.begin(), it->second.end());
      }
    }

    c_extra->setting_name = GetName_extra();
    c_extra->description = GetIsComment(c_extra->item_list.size(), "gene feature")
                          + "not associated with a CDS or RNA feature.";
    if (has_missing && !c_extra->item_list.empty()) thisInfo.disc_report_data.push_back(c_extra);
}; // EXTRA_MISSING_GENES
*/



void CBioseq_DISC_COUNT_NUCLEOTIDES :: TestOnObj(const CBioseq& bioseq)
{
   if (bioseq.IsNa())
        thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(bioseq));

}; // DISC_COUNT_NUCLEOTIDES :: TestOnObj


void CBioseq_DISC_COUNT_NUCLEOTIDES :: GetReport(CRef <CClickableItem>& c_item)
{
    c_item->setting_name = GetName();
    c_item->item_list = thisInfo.test_item_list[GetName()];
    c_item->description = GetIsComment(c_item->item_list.size(), "nucleotide Bioseq")
                           + "present.";
}; // DISC_COUNT_NUCLEOTIDES :: GetReport()



CRef <GeneralDiscSubDt> CBioseq_DUPLICATE_GENE_LOCUS :: AddLocus(const CSeq_feat& seq_feat)
{
    CRef < GeneralDiscSubDt > gene_locus
              (new GeneralDiscSubDt (GetDiscItemText(seq_feat), kEmptyStr));
    return (gene_locus);
};


void CBioseq_DUPLICATE_GENE_LOCUS :: TestOnObj(const CBioseq& bioseq)
{
  Str2SubDt locus_list;
  Str2SubDt :: iterator it;
  ITERATE (vector <const CSeq_feat*>, jt, gene_feat) {
     if ((*jt)->GetData().GetGene().CanGetLocus()) {
        string this_locus = (*jt)->GetData().GetGene().GetLocus();
        if (locus_list.empty()) locus_list[this_locus] = AddLocus(**jt);
        else {
          it = locus_list.find(this_locus);
          if (locus_list.end() != it) { // duplicates
              if ( !(it->second->sf0_added) ) {
                   thisInfo.test_item_list[GetName()].push_back(it->second->obj_text[0]);
                   it->second->sf0_added = true;
              }
              strtmp = GetDiscItemText(**jt);
              thisInfo.test_item_list[GetName()].push_back(strtmp);
              it->second->obj_text.push_back(strtmp);
          }
          else locus_list[this_locus] = AddLocus(**jt);
        }
     }
  }   
  
  if (!locus_list.empty()) {

      const CSeq_id& seq_id = BioseqToBestSeqId(bioseq, CSeq_id::e_Genbank);
      string location = seq_id.AsFastaString();
      CRef < CClickableItem > c_ls_item (new CClickableItem);

      unsigned cnt=0;
      ITERATE (Str2SubDt, it, locus_list) {
         if (it->second->obj_text.size() > 1) {
            CRef < CClickableItem > c_item (new CClickableItem);
            c_item->item_list = it->second->obj_text;            
            cnt += it->second->obj_text.size();
            c_item->description = GetHasComment(c_item->item_list.size(), "gene")
                                   + "locus " + it->second->str;
            c_item->setting_name = "DUPLICATE_GENE_LOCUS";
            c_ls_item->subcategories.push_back(c_item);
         }
      }
      c_ls_item->item_list.clear();
      c_ls_item->item_list.reserve(cnt);
      ITERATE (Str2SubDt, it, locus_list) {
        if (it->second->obj_text.size() > 1) {
           c_ls_item->item_list.insert(c_ls_item->item_list.end(), 
                     it->second->obj_text.begin(), it->second->obj_text.end()); 
        }
      };
      c_ls_item->setting_name = GetName();
      c_ls_item->description = GetHasComment(c_ls_item->item_list.size(), "gene")
                                   + "the same locus as another gene on " + location;

      DUPLICATE_GENE_LOCUS_locuss.push_back(c_ls_item);
  }
};


void CBioseq_DUPLICATE_GENE_LOCUS :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "gene")
                          + "the same locus as another gene on the same Bioseq.";
}; // DUPLICATE_GENE_LOCUS


/*
bool CBioseq_SUSPECT_PRODUCT_NAMES :: IsSearchFuncEmpty(const CSearch_func& func)
{
  if (func.IsString_constraint()) {
     const CString_constraint& str_cons = func.GetString_constraint();
     if (str_cons.GetIs_all_lower() || str_cons.GetIs_all_lower() || str_conf.GetIs_all_punct())
           return false;
     else if (!(str_cons.CanGetMatch_text()) return true;
     else return false;
  }
  else if (func.IsPrefix_and numbers()) {
      if (func.GetPrefix_and_numbers().empty()) return true;
      else return false;
  }
  else return false;
};



string CBioseq_SUSPECT_PRODUCT_NAMES :: SkipWeasel(string& str)
{
  vector <string> words;
  words = NStr::Tokenize(str, " ", words);
  
  vector <string> weasels;
  weasels.reserve(11);
  weasels.push_back("candidate");
  weasels.push_back("hypothetical");
  weasels.push_back("novel");
  weasels.push_back("possible");
  weasels.push_back("potential");
  weasels.push_back("predicted");
  weasels.push_back("probable");
  weasels.push_back("putative");
  weasels.push_back("uncharacterized");
  weasels.push_back("unique");

  NON_CONST_ITERATE (vector <string>, it, words) {
     if (!(*it).empty()) {
         ITERATE (vector <string>, jt, weasels) {
            if ( !(*it).empty() && NStr::FindNoCase(*it, *jt)) {
                *it = kEmptyStr;
                break;
            }
         }
     }
  }
 
**
  str = kEmptyStr;
  ITERATE (vector <string>, it, words) {
    if (!(*it).empty()) str += (*it) + " ";
  }
  return(NStr::TruncateSpaces(str));
**
  return (NStr::Join(words, " "));
};



bool CBioseq_SUSPECT_PRODUCT_NAMES :: DoesStringMatchConstraint(string& str, const CString_constrain& str_cons)
{
  if (str_cons.GetIgnore_weasel()) str = SkipWeasel(str);

  if (str_cons.GetIs_all_caps() && !IsAllCaps(str)) return false;
  if (str_cons.GetIs_all_lower && !IsAllLowerCase(str)) return false;
  if (str_cons.GetIs_all_punct && !IsAllPunctuation(str)) return false;
  if (!(str_conf.CanGetMatch_text())) return true; 

  string tmp_match = str_conf.GetMatch_text();
  if (str_cons.GetIgnore_weasel) {
    str_conf.SetMatch_text(SkipWeasel (scp->match_text));
  }

  if ((str_conf.GetMatch_location != eString_location_inlist) 
                                                      && !str_conf.CanGetIgnore_words()){
    str_conf.SetMatch_text(tmp_match);
    return AdvancedStringMatch(str, scp);
  }

  string search, pattern;
  if (str_conf.GetMatch_location() != eString_location_inlist 
           && (str_conf.GetIgnore_space() || str_cons.GetIgnore_punct())) {
    search = str;
    StripUnimportantCharacters (search, scp->ignore_space, scp->ignore_punct);
    pattern = str_confs.GetMatch_text();
    StripUnimportantCharacters (pattern, scp->ignore_space, scp->ignore_punct);
  } else {
    search = str;
    pattern = scp->match_text;
  }

  bool rval;
  if (str_conf.GetCase_sensitive()) pFound = search.find(pattern);
  else pFound = NStr::FindNoCase(search, pattern);
  switch (str_conf.GetMatch_location()) 
  {
    case eString_location_contains:
      if (string::npos == pFound) rval = FALSE;
      else if (str_conf.GetWhole_word()) 
      {
          rval = IsWholeWordMatch (search, pFound, pattern.size());
          while (!rval && pFound != string::npos) 
          {
	     if (str_conf.GetCase_sensitive()) pFound = search.substr(pFound+1).find(patten);
	     else pFound = NStr::FindNoCase(search.substr(pFound+1), pattern);
             if (pFound != string::npos)
                 rval = IsWholeWordMatch (search, pFound, pattern.size());
          }
      }
      else rval = TRUE;
      break;
    case eString_location_starts:
      if (pFound == search)
      {
        if (str_conf.GetWhole_word()) 
            rval = IsWholeWordMatch (search, pFound, pattern.size());
        else rval = TRUE;
      }
      break;
    case eString_location_ends:
      while (pFound != string::npos && !rval) {
  	    char_after = *(pFound + StringLen (pattern));
        if ( (pFound + pattern.size()) == search.size())
        {
          if (str_conf.GetWhole_word()) 
              rval = IsWholeWordMatch (search, pFound, pattern.size());
          else rval = true;
          * stop the search, we're at the end of the string *
          pFound = string::npos;
        }
        else
        {
	   if (str_conf.GetCase_sensitive()) pFound = search.substr(pFound+1).find(pattern);
	   else pFound = NStr::FindNoCase(search.substr(pFound + 1), pattern);
        }
      }
      break;
    case eString_location_equals:
      if (str_conf.GetCase_sensitive() && (search==pattern) ) rval= true; 
      else if (!str_conf.GetCase_sensitive() 
                  && !NStr::CompareNocase(search, pattern) ) rval = true;
      break;
    case eString_location_inlist:
      if (str_conf.GetCase_sensitive()) pFound = pattern.find(search);
      else pFound = NStr::FinNoCase(pattern, search);
      if (pFound == string::npos) rval = FALSE;
      else
      {
        rval = IsWholeWordMatchEx (pattern, pFound, search.size(), true);
        while (!rval && pFound != string::npos) 
        {
          if (str_conf.GetCase_sensitive())
               pFound = pattern.substr(pFound + 1).find(search);
          else pFound = NStr::FindNoCase(pattern.substr(pFound + 1), search);
          if (pFound != string::npos)
              rval = IsWholeWordMatchEx (pattern, pFound, str.size(), TRUE);
        }
      }
      if (!rval) {
        * look for spans *
        rval = IsStringInSpanInList (search, pattern);
      }
      break;
  }

  str_conf.SetMatch_text(tmp_match);
  return rval;
};
*/



/*
void CBioseq_SUSPECT_PRODUCT_NAMES :: MatchesSearchFunc(const string& str, const CSearch_func& func)
{
    if (func.IsString_constrain()) {
        bool rval = DoesSingleStringMatchConstraint(str, func.GetString_constrain());
        rval = (func.GetString_constrain().GetNot_present()) ? !rval : rval;
        return rval;
    }
    else if (func.IsContains_plural()) return (StringMayContainPlural (str));
    else if (func.IsN_or_more_brackets_or_parentheses())
           return( ContainsNorMoreSetsOfBracketsOrParentheses (str, search->data.intvalue));
    else if (func.IsThree_numbers()) return(ContainsThreeOrMoreNumbersTogether (str));
    else if (func.IsUnderscore()) return(StringContainsUnderscore (str));
    else if (func.IsPrefix_and_numbers()) 
              return( IsPrefixPlusNumbers (func.Getprefix_and_number(), str));
    else if (func.IsAll_caps()) return (IsAllCaps (str));
    else if (func.IsUnbalanced_paren()) return (StringContainsUnbalancedParentheses (str));
    else if (func.IsToo_long()) {
      if ((string::npos != str.fin("bifunctional")) 
             && (string::npos == str.find("multifunctional"))
             && (str.size() > func.GetToo_long()) ) 
          return true;
      else return false;
    }
    else if (func.IsHas_term()) return (ProductContainsTerm (search->data.ptrvalue, str));
    else return false;
};
*/




/*
string CBioseq_SUSPECT_PRODUCT_NAMES :: SummarizeStringConstraint(const CString_constraint& constraint)
{

   return("not set yet");
};




string CBioseq_SUSPECT_PRODUCT_NAMES :: SummarizeSuspectRule(const CSuspect_rule& rule)
{
   string summ; 

   if (rule.CanGetDescription()) return (rule.GetDescription());
   else {
     const CSearch_func& func = rule.GetFind();
     if (func.IsString_constraint()) 
            summ = SummarizeStringConstraint(func.GetString_constraint());
     else if (func.IsContains_plural()) 
             summ = "May contain plural";
     else if (func.IsN_or_more_brackets_or_parentheses())
             summ = "Contains " 
                     + NStr::IntToString(func.GetN_or_more_brackets_or_parentheses())
                     + " or more brackets or parentheses";
     else if (func.IsThree_numbers()) summ = "Three or more numbers together";
     else if (func.IsUnderscore()) summ = "Contains underscore";
     else if (func.IsPrefix_and_numbers())
              summ = "Is '" + func.GetPrefix_and_numbers() + "' followed by numbers";
     else if (func.IsAll_caps()) summ = "Is all capital letters";
     else if (func.IsUnbalanced_paren()) summ = "Contains unbalanced brackets or parentheses";
     else if (func.IsToo_long()) 
              summ = "Is longer than " + NStr::IntToString(func.GetToo_long()) + " characters";
     else if (func.IsHas_term())
              summ = "Contains '" + func.GetHas_term() + "'";
     else summ = "Unknown search function";
   } 
   return summ;
}




void CBioseq_SUSPECT_PRODUCT_NAMES :: DoesStringMatchConstraint(vector <string>& prot_nm, vector <bool>& match_ls, const CString_constraint& str_cst)
{
   if ( (!(str_cst.CanGetMatch_text()) || str_cst.GetMatch_text().empty()) ) {

       NON_CONST_ITERATE(vector <bool>, it, match_ls) *(it) = true;
       return;
   }

   unsigned i=0, cnt = 0;
   ITERATE(vector <string>, it, prot_nm) {
       if ((*it).empty()) { match_ls[i++] = false; cnt++;};
   }
   if (cnt == prot_nm.size()) return;

   if (str_cst.GetIgnore_weasel()) 
         NON_CONST_ITERATE (vector <string>, it, prot_nm)
             *it = SkipWeasel(*it);
   
   i=0;
   if (str_cst.GetIs_all_caps()) 
     NON_CONST_ITERATE (vector <bool>, it, match_ls)
        if (!(*it)) i++;
        else if (!IsAllCaps(prot_nm[i++])) { (*it) = false; cnt++;};
   if (cnt == prot_nm.size()) return;

   i = 0;
   if (str_cst.GetIs_all_lower()) 
       NON_CONST_ITERATE (vector <bool>, it, match_ls) 
          if (!(*it)) i++;
          else if (!IsAllLowerCase(prot_nm[i++])) { (*it) = false; cnt++; }
   if (cnt == prot_nm.size()) return;

   if (!(str_cst.CanGetMatch_text())) {
       NON_CONST_ITERATE (vector <bool>, it, match_ls) {
            (*it) = true;
       }
       return;
   }

   if (str_cst.GetMatch_location() !=eString_location_inlist 
                 && !(str_cst.CanGetIgnore_words())){
//         AdVancedStringMatch(prot_nm, str_cst);
         return;
   } 

   string tmp_match = str_cst.GetMatch_text();
   if (str_cst.GetIgnore_weasel()) {
       strtmp = SkipWeasel(str_cst.GetMatch_text());
       str_cst.SetMatch_text("!!!");
   }
   
   string pattern( str_cst.GetMatch_text() ); 
   if (eString_location_inlist != str_cst.GetMatch_Location()
        && ( str_cst.GetIgnore_space() || str_cst.GetIgnore_punct()) ) {
      StripUnimportantCharacters(prot_nm, str_cst.GetIgnore_space(), str_cst.GetIgnore_punct());
   }

   vector <size_t> pFound;
   pFound.reserve(prot_nm.size());
   i=0;
   if (str_cst.GetMatch_location != eString_location_inlist) {
     if (str_cst.GetCase_sensitive()) {
         NON_CONST_ITERATE (vector <size_t>, it, pFound) {
           if (match_ls[i] < 0) (*it) = prot_nm[i].find(pattern);
           else (*it) = string::npos; 
           i++;
         }
     }
     else {
         NON_CONST_ITERATE (vector <size_t>, it, pFound) {
           if (match_ls[i] < 0) (*it) = NStr::FindNoCase(prot_nm[i], pattern);
           else (*it) = string::npos;
           i++;
         }
     }
   }
   else {
     if (str_cst.GetCase_sensitive()) {
         NON_CONST_ITERATE (vector <size_t>, it, pFound) {
           if (match_ls[i]) (*it) = pattern.find(prot_nm[i]);
           else (*it) = string::npos;
           i++;
         }
     }
     else {
         NON_CONST_ITERATE (vector <size_t>, it, pFound) {
           if (match_ls[i]) (*it) = NStr::FindNoCase(pattern, prot_nm[i]);
           else (*it) = string::npos;
           i++;
         }
     }

   }
 
   i=0;
   bool if_match;
   switch (str_cst.GetMatch_location()) {
      case eString_location_contains:
         NON_CONST_ITERATE (vector <int>, it, match_ls) {
            if (*it < 0) {
                if (string::npos == pFound[i])  {
                      (*it) = 0;
                      cnt++;
                }
            }
            i++;
         }
         if (cnt < prot_nm.size()) {
             if (str_cst.GetWhole_word()) {
                i=0;
                bool if_found, if_match;
                NON_CONST_ITERATE (vector <int>, it, match_ls) {
                  if (*it < 0) {
                    strtmp = prot_nm[i];
                    if_found = pFound[i];
                    if_match = IsWholeWordMatch(strtmp, if_found, pattern.size());
                    while (string::npos != if_found && !if_match) {
                        if (str_cst.GetCase_sensitive()) if_found = strtmp.find(pattern);
                        else if_found = NStr::FinNoCase(strtmp, pattern);
                    } 
                    *it = if_match ? 1 : 0; 
                  }
                  i++;
                }
             };
             else NON_CONST_ITERATE (vector <int>, it, match_ls) if (*it < 0) *it = 1;
          }
          break;
      case eString_location_starts:
          NON_CONST_ITERATE (vector <bool>, it, match_ls) {
             if (*it) {
                if (!pFound[i]) {
                   if (str_cst.GetWhole_word())
                        *it = true; //IsWholeWordMatch(prot_nm[i], pFound[i], pattern.size());
                   else *it = true;
                }
             }
             i++;
          }; 
          break;
      case eString_location_ends:
          i = 0;
          string char_after;
          bool if_found;
          NON_CONST_ITERATE(vector <bool>, it, match_ls) {
            if (*it < 0) {
              bool rval = false;
              if_found = pFound[i]; 
              while (string::npos != if_found && !rval) {
                 if ( (if_found + pattern.size()) > prot_nm.size()) {
                   if (str_cst.GetWhole_word())
                      match_ls[i] = (IsWholeWordMatch(prot_nm, if_found, pattern.size()))? 1: 0;
                   else match_ls[i] = 1;
                   if_found = string::npos;
                 }
                 else {
                   if (str_cst.GetCase_sensitive())
                        if_found = prot_nm.substr(if_found+1).find(pattern);
                   else if_found = NStr::FindNoCase(pro_nm.substr(if_found+1), pattern);
                 }
              }
           }
           i++;  
          };
          break;
      case eString_location_equals:
          if (str_cst.GetCase_sensitive()) {
             NON_CONST_ITERATE (vector <bool>, it, match_ls) {
                if (*it) {
                   if (prot_nm[i] == pattern) *it = true;
                   else *it = false;
                }
                i++;
             }
          }
          else {
             NON_CONST_ITERATE (vector <bool>, it, match_ls) {
                if (*it) {
                    if (!NStr::CompareNocase(prot_nm[i], pattern)) *it = true;
                    else *it = false;
                }
                i++;
             }
          }
          break;
      case eString_location_inlist:
          i=0; 
          NON_CONST_ITERATE (vector <bool>, it, match_ls) {
            if (*it) {
               if (pFound[i] == string::npos) {
                   *it = false;
                   cnt ++;
               }
            } 
          }
          if (cnt < prot_nm.size()) {
             i = 0;
             NON_CONST_ITERATE (vector <bool>, it, match_ls) {
                if (*it) {
                   strtmp = prot_nm[i];
                   is_found = pFound[i];
                   is_match = IsWholeWordMatch(pattern, is_found, strtmp.size())
                   while (!is_match && string::npos != is_found) {
                      if (str_cst.GetCase_sensitive()) 
                            is_found = pattern.substr(is_found+1,strtmp);
                      else is_found = NStr::FindNoCase(pattern.substr(is_found+1), strtmp);
                      if (string::npos != is_found)
                        is_match = IsWholeWordMatch(pattern, is_found, strtmp.size());
                   }
                  if (!is_match) {
                      match_ls = IsStringInSpanInList(strtmp, pattern);
                  };
                }
                i++; 
             }
          }
          break;
   }

   str_cst.SetMatch_text(tmp_match);

   if (str_cst.GetNot_present()) 
        NON_CONST_ITERATE (vector <bool>, it, match_ls) *it = !(*it);
};






void CBioseq_SUSPECT_PRODUCT_NAMES :: MatchesSrchFunc(vector <string>& prot_nm, vector <bool>& match_ls, const CSearch_func& func)
{
  if (func.IsString_constraint())
        DoesStringMatchConstraint(prot_nm, func->GetString_constraint());
  
};



void CBioseq_SUSPECT_PRODUCT_NAMES :: DoesObjectMatchStringConstraint (vector <CSeq_feat*> seq_feats, vector <bool> match_ls, const CString_constraint& str_cst)
{
};


void CBioseq_SUSPECT_PRODUCT_NAMES :: DoesObjectMatchConstraint(const CRef < CConstraint_choice>& cst, vector <const CSeq_feat*> seq_feats, vector <bool> match_ls) 
{
  if (cst->IsString()) {
      DoesObjectMatchStringConstraint(seq_feats, match_ls, cst->GetString());
  }
};



void CBioseq_SUSPECT_PRODUCT_NAMES :: DoesObjectMatchConstraintChoiceSet(const CConstraint_choice_set& feat_cst, vector <const CSeq_feat*>& seq_feats, vector <bool>& match_ls)
{
   const list < CRef < CConstraint_choice > >& cst_ls = feat_cst.Get();
   ITERATE (CConstraint_choice_set::Tdata, it, cst_ls) {
      DoesObjectMatchConstraint(*it, seq_feats, match_ls);
   }
};






void CBioseq_SUSPECT_PRODUCT_NAMES :: TestOnObj(const CBioseq& bioseq)  // with rules
{
  const CSuspect_rule_set::Tdata& rules = thisInfo.suspect_rules->Get();
  vector < vector <CSeq_feat*> > feature_ls;

  unsigned k=0;
  CSeq_feat* seq_feat;
  vector <string> prot_nm;
  vector <const CSeq_feat*> seq_feat_ls;

  ITERATE (vector <const CSeq_feat*>, it, prot_feat) {
     seq_feat = const_cast <CSeq_feat*> (*it);
     const CProt_ref& prot = seq_feat->GetData().GetProt();
     if (prot.CanGetName()) {
       if ((*it)->GetData().GetSubtype() == CSeqFeatData::eSubtype_cdregion) {
          const CSeq_feat* cds = GetCDSForProduct(bioseq, thisInfo.scope);
          if (cds) seq_feat = const_cast <CSeq_feat*> (cds);
       }
       prot_nm.push_back(*(prot.GetName().begin()));
       seq_feat_ls.push_back(const_cast <const CSeq_feat*>(seq_feat));
     }
  }
 
  vector <bool> match_ls;
  match_ls.reserve(prot_nm.size());

  k = 0; 
  ITERATE (CSuspect_rule_set::Tdata, it, rules) {
     match_ls.clear();
     if (!(rule_prop->srch_func_empty[k])) MatchesSrchFunc(prot_nm, match_ls, (*it)->GetFind());
     else if (!(rule_prop->except_empty[k])) MatchesSrchFunc(prot_nm, match_ls,(*it)->GetExcept());
     else NON_CONST_ITERATE (vector <bool>, jt, match_ls) *jt = true;
     
     if ( (*it)->CanGetFeat_constraint() )
         DoesObjectMatchConstraintChoiceSet((*it)->GetFeat_constraint(), seq_feat_ls, match_ls);

     //ITERATE (match_ls) -> suspect_prod_name_feat[k]
  }
};

*/




/*  previous version
void CBioseq_SUSPECT_PRODUCT_NAMES :: TestOnObj(const CBioseq& bioseq)  // with rules
{
  const CSuspect_rule_set::Tdata& rules = thisInfo.suspect_rules->Get();
  vector < vector <CSeq_feat*> > feature_ls; 
  
  unsigned k;
  CSeq_feat* seq_feat;
  ITERATE (vector <const CSeq_feat*>, it, prot_feat) {
    seq_feat = const_cast <CSeq_feat*> (*it);
    const CProt_ref& prot = seq_feat->GetData().GetProt();
    if (prot.CanGetName()) {
       if ((*it)->GetData().GetSubtype() == CSeqFeatData::eSubtype_cdregion) {
          const CSeq_feat* cds = GetCDSForProduct(bioseq, thisInfo.scope);
          if (cds) seq_feat = const_cast <CSeq_feat*> (cds);
       }
       k = 0;
       ITERATE (CSuspect_rule_set::Tdata, jt, rules) {
         if (DoesStringMatchSuspectRule ( prot.GetName(), seq_feat, *jt, k)) {
            strtmp = GetDiscItemText(*seq_feat);
            thisInfo.test_item_list[GetName()].push_back(strtmp);  // master_list
            if (!SUSPECT_PROD_NAMES_feat[k]) {
               CRef <GeneralDiscSubDt> feat (new GeneralDiscSubDt (strtmp, 
                                                           SummarizeSuspectRule(*(*jt)))); 
               SUSPECT_PROD_NAMES_feat[k] = feat;
            }
            else SUSPECT_PROD_NAMES_feat[k]->seq_feat_text.push_back(strtmp);
         }
         k++;
       }
    }
  }
};
*/



/*
void CBioseq_SUSPECT_PRODUCT_NAMES :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->setting_name = GetName();
   c_item->item_list = thisInfo.test_item_list[GetName()];

   typedef map <EFix_type, CRef < CClickableItem > > RuleTp2Citem;
   RuleTp2Citem rule_tp_cate;
   map <EFix_type, vector < unsigned > > rule_feat_idx;

   unsigned i = 0;
   ITERATE (CSuspect_rule_set::Tdata, it, thisInfo.suspect_rules->Get()) {
      CRef <CClickableItem> c_feat (new CClickableItem);

      EFix_type rule_tp = (*it)->GetRule_type();
      switch ( rule_tp ) {
         case eFix_type_typo:
                   strtmp = "DISC_PRODUCT_NAME_TYPO"; break;
         case eFix_type_quickfix:
                   strtmp = "DISC_PRODUCT_NAME_QUICKFIX"; break;
         default:
                   strtmp = GetName();
      };        
      c_feat->setting_name = strtmp;
      c_feat->item_list = SUSPECT_PROD_NAMES_feat[i]->seq_feat_text;
      c_feat->description = SUSPECT_PROD_NAMES_feat[i]->str;
**
      if ((*it)->replace) {
...
      }
**
      if (rule_tp_cate.end() == rule_tp_cate.find(rule_tp)) {
          CRef < CClickableItem > c_rule (new CClickableItem);
          c_rule->setting_name = GetName();
          c_rule->description = "!!!";
          c_rule->item_list.insert(c_rule->item_list.end(), 
                        c_feat->item_list.begin(), c_feat->item_list.end());
          c_rule->expanded = true;
          c_rule->subcategories.push_back(c_feat);
      }
      else  {
          rule_tp_cate[rule_tp]->subcategories.push_back(c_feat);
          rule_tp_cate[rule_tp]->item_list.insert( rule_tp_cate[rule_tp]->item_list.end(), 
                       c_feat->item_list.begin(), c_feat->item_list.end());
      }
      rule_feat_idx[rule_tp].push_back(i);
   }

   unsigned tot_cnt;
   NON_CONST_ITERATE ( RuleTp2Citem, it, rule_tp_cate) {
      tot_cnt = 0;
      ITERATE ( vector < unsigned >, jt, rule_feat_idx[it->first])
            tot_cnt += SUSPECT_PROD_NAMES_feat[*jt]->seq_feat_text.size();
      it->second->item_list.reserve (tot_cnt);
      ITERATE ( vector < unsigned >, jt, rule_feat_idx[it->first]) 
           it->second->item_list.insert(it->second->item_list.end(),
                     SUSPECT_PROD_NAMES_feat[*jt]->seq_feat_text.begin(), 
                     SUSPECT_PROD_NAMES_feat[*jt]->seq_feat_text.end());    
      c_item->subcategories.push_back(it->second);
   }   

   c_item->expanded = true;
};
*/


// CSeqFeat


// CBioseq_set
// new comb
bool CBioseqSet_on_class :: IsMicrosatelliteRepeatRegion (const CSeq_feat& seq_feat)
{
   if (seq_feat.GetData().GetSubtype() != CSeqFeatData::eSubtype_repeat_region) return false;
   if (seq_feat.CanGetQual()) {
      ITERATE (vector <CRef < CGb_qual > >, it, seq_feat.GetQual()) {
        if ( NStr::EqualNocase((*it)->GetQual(), "satellite")
                         && NStr::EqualNocase( (*it)->GetVal(), "microsatellite"))
           return true;
      }
   }
   return false;
};


void CBioseqSet_on_class :: FindUnwantedSetWrappers (const CBioseq_set& set)
{
  ITERATE (list < CRef < CSeq_entry > >, it, set.GetSeq_set()) {
    if ( (*it)->IsSeq() ) {
       const CBioseq& bioseq = (*it)->GetSeq();
       CBioseq_Handle bioseq_hl = thisInfo.scope->GetBioseqHandle(bioseq);
       for (CSeqdesc_CI ci(bioseq_hl, CSeqdesc::e_Source); ci && !m_has_rearranged; ++ci) {
         m_has_rearranged = IsSubSrcPresent( ci->GetSource(), CSubSource::eSubtype_rearranged);
       }
       for (CFeat_CI feat_it(bioseq_hl); feat_it && (!m_has_sat_feat || !m_has_non_sat_feat); 
                                                                                 ++feat_it) {
          if (IsMicrosatelliteRepeatRegion(feat_it->GetOriginalFeature())) 
                  m_has_sat_feat = true;
          else m_has_non_sat_feat = true; 
       }
    } 
    else if (!m_has_rearranged || !m_has_sat_feat || !m_has_non_sat_feat)
            FindUnwantedSetWrappers( (*it)->GetSet() );
  }
};


void CBioseqSet_on_class :: TestOnObj(const CBioseq_set& bioseq_set)
{
   if (thisTest.is_BioSet_run) return;

   string desc = GetDiscItemText(bioseq_set);
   CBioseq_set::EClass e_cls = bioseq_set.GetClass();
   if ( e_cls == CBioseq_set::eClass_eco_set
        || e_cls == CBioseq_set :: eClass_mut_set
        || e_cls == CBioseq_set :: eClass_phy_set
        || e_cls == CBioseq_set :: eClass_pop_set) {

      // DISC_NONWGS_SETS_PRESENT
      thisInfo.test_item_list[GetName_nonwgs()].push_back(desc);
 
      // TEST_UNWANTED_SET_WRAPPER
      FindUnwantedSetWrappers(bioseq_set);
      if (m_has_rearranged || (m_has_sat_feat && !m_has_non_sat_feat) )
         thisInfo.test_item_list[GetName_wrap()].push_back(desc);
   }
   else {
      // DISC_SEGSETS_PRESENT
      if ( e_cls == CBioseq_set :: eClass_segset)
        thisInfo.test_item_list[GetName_segset()].push_back(desc);
   }

   thisTest.is_BioSet_run = true;
};


void CBioseqSet_TEST_UNWANTED_SET_WRAPPER :: GetReport(CRef <CClickableItem>& c_item)
{
   unsigned cnt = c_item->item_list.size();
   c_item->description = NStr :: UIntToString(cnt) + GetNoun(cnt, "unwanted set wrapper");
};


void CBioseqSet_DISC_SEGSETS_PRESENT :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetIsComment(c_item->item_list.size(), "segset") + "present.";
};


void CBioseqSet_DISC_NONWGS_SETS_PRESENT :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
        = GetIsComment(c_item->item_list.size(), "set") + "of type eco, mut, phy or pop.";
};
// new comb


// CSeqEntryTestAndRepData
// new method
bool CSeqEntry_test_on_tax_cflts :: s_StringHasVoucherSN(const string& vou_nm)
{
  vector <string> spec_vou;
  spec_vou.push_back("s.n.");
  spec_vou.push_back("sn");
  return (DoesStringContainPhrase(vou_nm, spec_vou));
};


void CSeqEntry_test_on_tax_cflts :: RunTests(const CBioSource& biosrc, const string& desc)
{
   string qual_vlu, tax_nm;
   tax_nm = biosrc.IsSetTaxname() ? biosrc.GetTaxname() : kEmptyStr;
   if (tax_nm.empty()) return; 
   else tax_nm = "$" + tax_nm + "#";
   string tax_desc = tax_nm + desc;

   ITERATE (list <CRef <COrgMod> >, it, biosrc.GetOrg().GetOrgname().GetMod()) {
     switch ( (*it)->GetSubtype() ) {

       // DISC_SPECVOUCHER_TAXNAME_MISMATCH
       case COrgMod::eSubtype_specimen_voucher: 
             if (!s_StringHasVoucherSN( qual_vlu = (*it)->GetSubname())) 
                 thisInfo.test_item_list[GetName_vou()].push_back(qual_vlu + tax_desc);
             break;
       // DISC_STRAIN_TAXNAME_MISMATCH
       case COrgMod::eSubtype_strain:
             thisInfo.test_item_list[GetName_str()].push_back((*it)->GetSubname() + tax_desc);
             break;
       // DISC_CULTURE_TAXNAME_MISMATCH
       case COrgMod::eSubtype_culture_collection:
             thisInfo.test_item_list[GetName_cul()].push_back((*it)->GetSubname() + tax_desc);
             break;
       // DISC_BIOMATERIAL_TAXNAME_MISMATCH
       case COrgMod::eSubtype_bio_material:
             thisInfo.test_item_list[GetName_biom()].push_back((*it)->GetSubname() + tax_desc);
             break;
       default: break;
     }
   }
};


void CSeqEntry_test_on_tax_cflts :: TestOnObj(const CSeq_entry& seq_entry)
{
   if (thisTest.is_TaxCflts_run) return;

   string desc;
   ITERATE (vector <const CSeq_feat*>, it, biosrc_orgmod_feat)
      RunTests( (*it)->GetData().GetBiosrc(), GetDiscItemText(**it));
  
   unsigned i=0;
   ITERATE (vector <const CSeqdesc*>, it, biosrc_orgmod_seqdesc) {
      desc = GetDiscItemText( **it, *(biosrc_orgmod_seqdesc_seqentry[i]));
      RunTests((*it)->GetSource(), desc);
      i++;
   }

   thisTest.is_TaxCflts_run = true;
};


void CSeqEntry_test_on_tax_cflts :: GetReport_cflts(CRef <CClickableItem>& c_item, const string& setting_name, const string& qual_nm)
{
   Str2Strs qual2tax_seqs, tax2seqs;
   GetTestItemList(c_item->item_list, qual2tax_seqs);
   c_item->item_list.clear();
   if (qual2tax_seqs.size() == 1) {
       tax2seqs.clear();
       GetTestItemList(qual2tax_seqs.begin()->second, tax2seqs, "#");
       if (tax2seqs.size() > 1) {
          ITERATE (Str2Strs, jt, tax2seqs) 
             c_item->item_list.insert(c_item->item_list.end(), 
                                        jt->second.begin(), jt->second.end());
          c_item->description 
            = GetHasComment(c_item->item_list.size(), "biosource") + qual_nm + " "
              + qual2tax_seqs.begin()->first + " but do not have the same taxnames.";
       }
   }
   else {
     ITERATE (Str2Strs, it, qual2tax_seqs) {
       tax2seqs.clear();
       GetTestItemList(it->second, tax2seqs, "#");
       if (tax2seqs.size() > 1) {
           CRef <CClickableItem> c_sub (new CClickableItem);
           ITERATE (Str2Strs, jt, tax2seqs)
             c_sub->item_list.insert(c_sub->item_list.end(),
                                        jt->second.begin(), jt->second.end());
           c_sub->description
              = GetHasComment(c_sub->item_list.size(), "biosource") + qual_nm + " "
                + it->first + " but do not have the same taxnames.";    
           c_item->item_list.insert(c_item->item_list.end(), 
                                     c_sub->item_list.begin(), c_sub->item_list.end());
           c_item->subcategories.push_back(c_sub);
       }   
     }
     c_item->description = GetHasComment(c_item->item_list.size(), "BioSource") 
                           + qual_nm + "/taxname conflicts.";
   }
};




void CSeqEntry_test_on_quals :: GetMultiSubSrcVlus(const CBioSource& biosrc, const string& type_name, vector <string>& multi_vlus)
{
  ITERATE (list <CRef <CSubSource> >, it, biosrc.GetSubtype()) {
     int type = (*it)->GetSubtype();
     if ( (*it)->GetSubtypeName( type ) == type_name ) {
        strtmp = (*it)->GetName();
        if (strtmp.empty() && (type == CSubSource::eSubtype_germline
                                 || type == CSubSource::eSubtype_transgenic
                                 || type == CSubSource::eSubtype_metagenomic
                                 || type == CSubSource::eSubtype_environmental_sample
                                 || type == CSubSource::eSubtype_rearranged)) {
           multi_vlus.push_back("TRUE");
        }
        else multi_vlus.push_back(strtmp);
     }
  }
};


void CSeqEntry_test_on_quals :: GetMultiOrgModVlus(const CBioSource& biosrc, const string& type_name, vector <string>& multi_vlus)
{
   ITERATE (list <CRef <COrgMod> >, it, biosrc.GetOrgname().GetMod() )
      if ((*it)->GetSubtypeName((*it)->GetSubtype()) == type_name)
          multi_vlus.push_back((*it)->GetSubname());
};


void CSeqEntry_test_on_quals :: GetMultiPrimerVlus(const CBioSource& biosrc, const string& qual_name, vector <string>& multi_vlus)
{
   ITERATE (list <CRef <CPCRReaction> >, jt, biosrc.GetPcr_primers().Get()) {
     if (qual_name == "fwd_primer_name" || qual_name == "fwd_primer_seq") {
       ITERATE (list <CRef <CPCRPrimer> >, kt, (*jt)->GetForward().Get()) {
          if ( qual_name == "fwd_primer_name" && (*kt)->CanGetName() ) {
              strtmp = (*kt)->GetName();
              if (!strtmp.empty()) multi_vlus.push_back(strtmp);
          }
          if ( qual_name == "fwd_primer_seq" && (*kt)->CanGetSeq() ) {
              strtmp = (*kt)->GetSeq();
              if (!strtmp.empty()) multi_vlus.push_back(strtmp);
          }
       }
     }
     else {
        ITERATE (list <CRef <CPCRPrimer> >, kt, (*jt)->GetReverse().Get()) {
           if ( qual_name == "rev_primer_name" && (*kt)->CanGetName() ) {
               strtmp = (*kt)->GetName();
               if (!strtmp.empty()) multi_vlus.push_back(strtmp);
           }
           if ( qual_name == "rev_primer_seq" && (*kt)->CanGetSeq() ) {
               strtmp = (*kt)->GetSeq();
               if (!strtmp.empty()) multi_vlus.push_back(strtmp);
           }
        }
     }
   }
};


CRef <CClickableItem> CSeqEntry_test_on_quals :: MultiItem(const string& qual_name, const vector <string>& multi_list, const string& ext_desc, const string& setting_name)
{
    CRef <CClickableItem> c_sub (new CClickableItem);
    c_sub->setting_name = setting_name;
    c_sub->description = "  " + GetHasComment(multi_list.size(), "source") + "multiple "
                         + qual_name + " qualifiers" + ext_desc;
    if (setting_name == GetName_asn1_oncall()) c_sub->item_list = multi_list;
    return (c_sub);
};


void CSeqEntry_test_on_quals :: CheckForMultiQual(const string& qual_name, const CBioSource& biosrc, eMultiQual& multi_type, bool is_subsrc)
{
   vector <string> multi_qual_vlus;
   Str2Int  vlu_cnt;
   GetMultiQualVlus(qual_name, biosrc, multi_qual_vlus, is_subsrc);
   if (multi_qual_vlus.size() <= 1) multi_type = e_no_multi;
   else {
     ITERATE (vector <string>, it, multi_qual_vlus) {
       if (vlu_cnt.find(*it) != vlu_cnt.end()) vlu_cnt[*it]++;
       else vlu_cnt[*it] = 1;
     }
     if (vlu_cnt.size() == 1) multi_type = e_same;
     else {
        bool has_dup = false;
        ITERATE (Str2Int, it, vlu_cnt) {
          if (it->second > 1) {has_dup = true; break;}
   }
        multi_type = has_dup ? e_dup : e_all_dif;
     }
   }
};



void CSeqEntry_test_on_quals :: GetMultiQualVlus(const string& qual_name, const CBioSource& biosrc, vector <string>& multi_vlus, bool is_subsrc)
{
   ITERATE (vector <string>, it, thisInfo.no_multi_qual) if (qual_name == *it) return;

   if (is_subsrc) GetMultiSubSrcVlus(biosrc, qual_name, multi_vlus);
   if (qual_name == "fwd_primer_name" || qual_name == "fwd_primer_seq"
             || qual_name == "rev_primer_name" || qual_name == "rev_primer_seq" )
      GetMultiPrimerVlus(biosrc, qual_name, multi_vlus);
   else GetMultiOrgModVlus(biosrc, qual_name, multi_vlus);
};


// C: GetSourceQualFromBioSource
string CTestAndRepData :: GetSrcQualValue(const CBioSource& biosrc, const string& qual_name, bool is_subsrc)
{
 string ret_str(kEmptyStr);
 // DoesStringMatchConstraint missing
 if (is_subsrc) ret_str = Get1SubSrcValue(biosrc, qual_name);
 else if (qual_name == "location") 
            ret_str = biosrc.GetOrganelleByGenome(biosrc.GetGenome());
 else if (qual_name == "taxname") 
            ret_str = biosrc.IsSetTaxname() ? biosrc.GetTaxname() : kEmptyStr;
 else if (qual_name == "common_name") 
        ret_str = biosrc.IsSetCommon() ? biosrc.GetCommon() : kEmptyStr;
 else if (qual_name == "lineage") 
        ret_str = biosrc.IsSetLineage() ? biosrc.GetLineage() : kEmptyStr;
 else if (qual_name == "div") 
        ret_str = biosrc.IsSetDivision() ? biosrc.GetDivision() : kEmptyStr;
 else if (qual_name == "dbxref") ret_str = "no ready yet";
 else if (qual_name == "taxid") {
   int tid = biosrc.GetOrg().GetTaxId();
   if (tid > 0) ret_str = NStr::IntToString(tid);
   else {
     if (biosrc.GetOrg().CanGetDb()) {
       ITERATE (vector <CRef <CDbtag> >, it, biosrc.GetOrg().GetDb()) {
         strtmp = (*it)->GetDb();
         if (NStr::FindNoCase(strtmp, "taxdb") != string::npos
               && strtmp.size() == ((string)"taxdb").size()
               && (*it)->GetTag().IsStr()) 
             ret_str = (*it)->GetTag().GetStr();
         }       
       }
   }
 }
 else if ( qual_name == "fwd_primer_name" || qual_name == "fwd_primer_seq"
           || qual_name == "rev_primer_name" || qual_name == "rev_primer_seq" )
 {
   ITERATE (list <CRef <CPCRReaction> >, jt, biosrc.GetPcr_primers().Get()) {
     if (qual_name == "fwd_primer_name" || qual_name == "fwd_primer_seq") {
       ITERATE (list <CRef <CPCRPrimer> >, kt, (*jt)->GetForward().Get()) {
          if ( qual_name == "fwd_primer_name" && (*kt)->CanGetName() ) {
              strtmp = (*kt)->GetName();
              if (!strtmp.empty()) { ret_str = strtmp; break; }
          }
          if ( qual_name == "fwd_primer_seq" && (*kt)->CanGetSeq() ) {
              strtmp = (*kt)->GetSeq();
              if (!strtmp.empty()) { ret_str = strtmp; break; }
          }
          if (!ret_str.empty()) break;
       }
     }
     else {
        ITERATE (list <CRef <CPCRPrimer> >, kt, (*jt)->GetReverse().Get()) {
           if ( qual_name == "rev_primer_name" && (*kt)->CanGetName() ) {
               strtmp = (*kt)->GetName();
               if (!strtmp.empty()) {ret_str = strtmp; break;}
           }
           if ( qual_name == "rev_primer_seq" && (*kt)->CanGetSeq() ) {
               strtmp = (*kt)->GetSeq();
               if (!strtmp.empty()) { ret_str = strtmp; break;}
           }
           if ( !ret_str.empty() ) break;
        }
     }
     if ( !ret_str.empty() ) break;
   }
 }
 else ret_str = Get1OrgModValue(biosrc, qual_name);
 
 return (ret_str);
};



void CSeqEntry_test_on_quals :: GetQualDistribute(Str2Ints& qual2src_idx, const vector <string>& desc_ls, const vector <CConstRef <CBioSource> >& src_ls, const string& setting_name)
{
  int pre_idx, cur_idx, i;
  string qual_name, src_qual_vlu, src_txt;
  bool is_subsrc;
  size_t pos;

  ITERATE (Str2Ints, it, qual2src_idx) {
     qual_name = it->first;
     is_subsrc = false;
     if ( (pos = qual_name.find("$subsrc")) != string::npos) {
        is_subsrc = true;
        qual_name = qual_name.substr(0, pos);
     }
     pre_idx = -1;
     ITERATE (vector <int>, jt, it->second) {
       cur_idx = *jt;
       if (pre_idx +1 != cur_idx) {  // have missing
    //      strtmp = (qual_name == "location") ? "$genomic#" : "$missing#";
          strtmp = "$missing#";
          for ( i = pre_idx+1; i < cur_idx; i++) 
               thisInfo.test_item_list[setting_name].push_back(qual_name+ strtmp +desc_ls[i]);
                //qual_nm2qual_vlus[qual_name].qual_vlu2src["genomic"].push_back(desc_ls[i]);
       }
       src_qual_vlu= GetSrcQualValue(*src_ls[cur_idx], qual_name, is_subsrc);
       src_txt = desc_ls[cur_idx];
       thisInfo.test_item_list[setting_name].push_back(
                                          qual_name+"$"+ src_qual_vlu + "#" + src_txt);
       //qual_nm2qual_vlus[qual_name].qual_vlu2src[src_qual_vlu].push_back(src_txt);

       // have multiple qualifiers? 
       eMultiQual multi_type;
       CheckForMultiQual(qual_name, *src_ls[cur_idx], multi_type, is_subsrc);
       strtmp = kEmptyStr;
       switch (multi_type) {
          case e_same: strtmp = "$multi_same#"; break;
          case e_dup:  strtmp = "$multi_dup#"; break;
          case e_all_dif: strtmp = "$multi_all_dif#"; break;
          default: break;
       }
       if (!strtmp.empty()) 
          thisInfo.test_item_list[setting_name].push_back(qual_name + strtmp + src_txt);

       // biosrc_val_qualnm:
       strtmp = src_txt + "$" + src_qual_vlu + "#" + qual_name;
       thisInfo.test_item_list[setting_name + "_src"].push_back(strtmp);

       pre_idx = cur_idx;
     }

   //  strtmp = (qual_name == "location") ? "$genomic#" : "$missing#";
     strtmp = "$missing#";
     for (i = cur_idx +1; i < (int)src_ls.size(); i++)
           thisInfo.test_item_list[setting_name].push_back(qual_name + strtmp +desc_ls[i]);
  }
};


void CSeqEntry_test_on_quals :: GetReport_quals(CRef <CClickableItem>& c_item, const string& setting_name)
{
   Str2Strs qnm2qvlu_src;
   GetTestItemList(c_item->item_list, qnm2qvlu_src);
   c_item->item_list.clear();

   bool all_same, all_unique, all_present;
   bool multi_same, multi_dup, multi_all_dif;
   unsigned multi_type_cnt;
   string qual_nm;

   Str2Strs qvlu2src;
   CRef <CClickableItem> c_main (new CClickableItem);
   if (setting_name == GetName_asn1_oncall()) {
     c_main = c_item;
     c_main->setting_name = setting_name;
     c_main->description = "Source Qualifer Report";
     c_main->item_list.clear();
     c_item = CRef <CClickableItem> (new CClickableItem);
   }

   unsigned i = 0;
   ITERATE (Str2Strs, it, qnm2qvlu_src) {
     qual_nm = it->first;
     qvlu2src.clear();
     GetTestItemList(it->second, qvlu2src, "#");
     if (qvlu2src.size() == 1 && qvlu2src.begin()->second.size() > 1) all_same = true;
     else {
        all_same = false;
        all_unique = true;
        ITERATE (Str2Strs, jt, qvlu2src) {
          if ( jt->first != "missing"
                   && jt->first != "multi_same"
                   && jt->first != "multi_dup"
                   && jt->first != "multi_all_dif"
                   && jt->second.size() > 1) {
            all_unique = false; break;
          }
        }
     } 
     
     all_present = (qvlu2src.find("missing") == qvlu2src.end())? true : false;

     multi_same = (qvlu2src.find("multi_same") == qvlu2src.end()) ? false : true;
     multi_dup = (qvlu2src.find("multi_dup") == qvlu2src.end()) ? false : true;
     multi_all_dif= (qvlu2src.find("multi_all_dif") == qvlu2src.end()) ? false : true;

     multi_type_cnt = 0;
     multi_type_cnt += multi_same ? 1 : 0;
     multi_type_cnt += multi_dup ? 1 : 0;
     multi_type_cnt += multi_all_dif? 1 : 0;

     if (qual_nm == "note") qual_nm += "-subsrc";
     c_item->description = qual_nm;
     if (all_present) {
         c_item->description += " (all present, ";
         if (all_same) {
            c_item->description += "all same";
            CRef <CClickableItem> c_sub (new CClickableItem);
            c_sub->setting_name = setting_name;
            c_sub->description 
                = GetHasComment( (qvlu2src.begin()->second).size(), "source")
                   + " '" + qvlu2src.begin()->first + "' for " + qual_nm;
            c_item->subcategories.push_back(c_sub);
            if (setting_name == GetName_asn1_oncall()) 
                    c_sub->item_list = qvlu2src.begin()->second;
         }
         else if (all_unique) {
                 c_item->description += "all_unique";
                 if (setting_name == GetName_asn1_oncall()) {
                    ITERATE (Str2Strs, jt, qvlu2src) { 
                       if ( jt->first != "missing" && jt->first != "multi_same"
                                   && jt->first != "multi_dup" && jt->first != "multi_all_dif")
                           c_item->item_list.push_back((jt->second)[0]);
                    }
                 }
         }
         else {
           c_item->description += "some duplicate";
           unsigned uni_cnt=0;
           vector <string> uni_sub;
           ITERATE (Str2Strs, jt, qvlu2src) {
             if ( jt->first == "missing" || jt->first == "multi_same"
                         || jt->first == "multi_dup" || jt->first == "multi_all_dif")
                    continue;
             unsigned sz = (jt->second).size();
             if (sz > 1) {
               CRef <CClickableItem> c_sub (new CClickableItem);
               c_sub->setting_name = setting_name;
               c_sub->description =
                  GetHasComment(sz, "source") + "'" + jt->first + "' for " + qual_nm;
               c_item->subcategories.push_back(c_sub);
               if (setting_name == GetName_asn1_oncall()) c_sub->item_list = jt->second; 
             }
             else  {
               uni_cnt ++;
               if (setting_name == GetName_asn1_oncall()) uni_sub.push_back((jt->second)[0]);
             }
           }
           if (uni_cnt) {
             CRef <CClickableItem> c_sub (new CClickableItem);
             c_sub->setting_name = GetName();
             c_sub->description = GetHasComment(uni_cnt, "source") + "unique "
                                    + (uni_cnt >1 ? "values" : "value") + " for " + qual_nm;
             c_item->subcategories.push_back(c_sub);
             if (setting_name == GetName_asn1_oncall()) c_sub->item_list = uni_sub;
          }
         }
     }
     else {
        c_item->description += " (some missing, ";
        CRef <CClickableItem> c_sub (new CClickableItem);
        c_sub->setting_name = setting_name;
        c_sub->description = 
             GetIsComment(qvlu2src["missing"].size(), "source") + "missing " + qual_nm;
        c_item->subcategories.push_back(c_sub);
        if (setting_name == GetName_asn1_oncall()) c_sub->item_list = qvlu2src["missing"];

        if (all_same) {
            c_item->description += "all same";
            CRef <CClickableItem> c_sub (new CClickableItem);
            c_sub->setting_name = GetName();
            c_sub->description 
                = GetHasComment( (qvlu2src.begin()->second).size(), "source")
                   + " '" + qvlu2src.begin()->first + "' for " + qual_nm;
            c_item->subcategories.push_back(c_sub);
            if (setting_name == GetName_asn1_oncall()) 
                     c_sub->item_list = qvlu2src.begin()->second;
        }
        else if (all_unique) {
           c_item->description += "all unique"; 
           CRef <CClickableItem> c_sub (new CClickableItem);
           unsigned uni_cnt = 0;
           ITERATE (Str2Strs, jt, qvlu2src) {
              if ( jt->first == "missing" || jt->first == "multi_same"
                         || jt->first == "multi_dup" || jt->first == "multi_all_dif")
                    continue;
              if (jt->second.size() == 1) {
                 uni_cnt ++;
                 if (setting_name == GetName_asn1_oncall()) 
                                     c_sub->item_list.push_back((jt->second)[0]);
              }
           }
           c_sub->setting_name = setting_name;
           c_sub->description = GetHasComment(uni_cnt, "source") + "unique " 
                                    + (uni_cnt >1 ? "values" : "value") + " for " + qual_nm;
           c_item->subcategories.push_back(c_sub);
        }
        else {
           c_item->description += "some duplicate";
           unsigned uni_cnt=0, sz;
           vector <string> uni_sub;
           ITERATE (Str2Strs, jt, qvlu2src) {
             if ( jt->first == "missing" || jt->first == "multi_same"
                         || jt->first == "multi_dup" || jt->first == "multi_all_dif")
                    continue;
             sz = (jt->second).size();
             if (sz > 1) {
               CRef <CClickableItem> c_sub (new CClickableItem);
               c_sub->setting_name = GetName();
               c_sub->description =
                  GetHasComment(sz, "source") + "'" + jt->first + "' for " + qual_nm;
               c_item->subcategories.push_back(c_sub);
               if (setting_name == GetName_asn1_oncall()) c_sub->item_list = jt->second;
             }
             else  {
                 uni_cnt ++;
                 uni_sub.push_back((jt->second)[0]);
             }
           }
           if (uni_cnt) {
             CRef <CClickableItem> c_sub (new CClickableItem);
             c_sub->setting_name = GetName();
             c_sub->description = GetHasComment(uni_cnt, "source") + "unique "
                                    + (uni_cnt >1 ? "values" : "value") + " for " + qual_nm;
             c_item->subcategories.push_back(c_sub);
             if (setting_name == GetName_asn1_oncall()) c_sub->item_list = uni_sub;
          }
        }
     }

     //multiple
     unsigned sz = qvlu2src["multi_same"].size() 
                        + qvlu2src["multi_dup"].size() + qvlu2src["multi_all_dif"].size();
     string ext_desc;
     if ( multi_type_cnt) {
          c_item->description += ", some multi";
          CRef <CClickableItem> c_sub (new CClickableItem);
          c_sub->setting_name = setting_name;
          if (multi_type_cnt == 1) 
              ext_desc = multi_same ? ", same_value" 
                                       : (multi_dup ? ", some dupplicates" : kEmptyStr);
          else {
              ext_desc = kEmptyStr;
              if (multi_same) {
                 c_sub->subcategories.push_back(MultiItem(
                               qual_nm, qvlu2src["multi_same"], ", same value", setting_name));
              }
              if (multi_dup) 
                 c_sub->subcategories.push_back(MultiItem(
                           qual_nm, qvlu2src["multi_dup"], ", some duplicates", setting_name));
              if (multi_all_dif) 
                 c_sub->subcategories.push_back(MultiItem(
                             qual_nm, qvlu2src["multi_all_dif"], kEmptyStr, setting_name));
          }
          c_sub->description = GetHasComment(sz, "source") 
                                   + "multiple " + qual_nm + " qualifiers" + ext_desc;
          c_item->subcategories.push_back(c_sub);
     }
     
     c_item->description += ")";
     if (setting_name == GetName_asn1_oncall()) 
           c_main->subcategories.push_back(c_item);
     else {
        c_item->item_list.clear();
        if (i) thisInfo.disc_report_data.push_back(c_item);
        if (i < qnm2qvlu_src.size() - 1 ) {
             c_item = CRef <CClickableItem> (new CClickableItem);
             c_item->setting_name = setting_name;
        }
     }
     i++;
   }  // it

 // biosrc has 2 qual with same value: biosrc2qualvlu_nm
    string biosrc_nm, quals;
    unsigned cnt = 0;
    vector < CRef <CClickableItem> > sub;
    Str2Strs biosrc2qualvlu_nm, vlu2nms;
    GetTestItemList(thisInfo.test_item_list[setting_name + "_src"], biosrc2qualvlu_nm);
    thisInfo.test_item_list[setting_name + "_src"].clear();
    ITERATE (Str2Strs, it, biosrc2qualvlu_nm) {
         biosrc_nm = it->first;
         vlu2nms.clear();
         GetTestItemList(it->second, vlu2nms);
         ITERATE (Str2Strs, jt, vlu2nms) {
           if (jt->second.size() > 1) {
              cnt ++;
              quals = NStr::Join(jt->second, ", ");
              CRef <CClickableItem> c_sub (new CClickableItem);
              c_sub->setting_name = setting_name;
              c_sub->description = 
                "BioSource has value '" + jt->first + "' for these qualifiers: " + quals;
              sub.push_back(c_sub);
              if (setting_name == GetName_asn1_oncall()) c_sub->item_list.push_back(biosrc_nm);
           }
         }
    }
 
    if (!sub.empty()) {
         c_item = CRef <CClickableItem> (new CClickableItem);
         c_item->setting_name = setting_name;
         c_item->description = GetHasComment(cnt, "source") 
                                      + "two or more qualifiers with the same value";
         c_item->item_list.push_back(biosrc_nm);
         c_item->subcategories = sub;
         thisInfo.disc_report_data.push_back(c_item);
    }
};


void CSeqEntry_test_on_quals :: GetQual2SrcIdx(const vector <CConstRef <CBioSource> >& src_ls, const vector <string>& desc_ls, Str2Ints& qual2src_idx)
{
   bool fwd_nm, fwd_seq, rev_nm, rev_seq;
   unsigned i=0;
   ITERATE (vector <CConstRef <CBioSource> >, it, src_ls) {
    if ( (*it)->IsSetTaxname() ) qual2src_idx["taxname"].push_back(i);
    if ( (*it)->GetOrg().GetTaxId() ) qual2src_idx["taxid"].push_back(i);

    // add subtypes & orgmods
    if ( (*it)->CanGetSubtype() ) {
       ITERATE (list <CRef <CSubSource> >, jt, (*it)->GetSubtype()) {
         strtmp = (*jt)->GetSubtypeName((*jt)->GetSubtype(), CSubSource::eVocabulary_insdc);
         qual2src_idx[strtmp + "$subsrc"].push_back(i);
       }
    }

    if ( (*it)->IsSetOrgname() && (*it)->GetOrgname().CanGetMod() ) {
       ITERATE (list <CRef <COrgMod> >, jt, (*it)->GetOrgname().GetMod() ) {
          strtmp = (*jt)->GetSubtypeName((*jt)->GetSubtype(), COrgMod::eVocabulary_insdc);
          if (strtmp != "old_name" && strtmp != "old_lineage" && strtmp != "gb_acronym"
                         && strtmp != "gb_anamorph" && strtmp != "gb_synonym")
              qual2src_idx[strtmp].push_back(i);
       }
    }
   
    // add PCR primers
    if ( (*it)->CanGetPcr_primers() ) {
       fwd_nm = fwd_seq = rev_nm = rev_seq = false;
       ITERATE (list <CRef <CPCRReaction> >, jt, (*it)->GetPcr_primers().Get()) {
          if ( !fwd_nm && !fwd_seq && (*jt)->CanGetForward() ) {
            ITERATE (list <CRef <CPCRPrimer> >, kt, (*jt)->GetForward().Get()) {
               if ( !fwd_nm && (*kt)->CanGetName() ) {
                  strtmp = (*kt)->GetName();
                  if (!strtmp.empty()) {
                      qual2src_idx["fwd_primer_name"].push_back(i);
                      fwd_nm = true;
                  }
               }
               if ( !fwd_seq && (*kt)->CanGetSeq() ) {
                  strtmp = (*kt)->GetSeq();
                  if (!strtmp.empty()) {
                     fwd_seq = true;
                     qual2src_idx["fwd_primer_seq"].push_back(i);
                  }
               }
               if (fwd_nm && fwd_seq) break;
            }
          }
          if ( !rev_nm && !rev_seq && (*jt)->CanGetReverse() ) {
            ITERATE (list <CRef <CPCRPrimer> >, kt, (*jt)->GetReverse().Get()) {
               if (!rev_nm && (*kt)->CanGetName() ) {
                  strtmp = (*kt)->GetName();
                  if (!strtmp.empty()) {
                      rev_nm = true;
                      qual2src_idx["rev_primer_name"].push_back(i);
                  }
               }
               if (!rev_seq && (*kt)->CanGetSeq() ) {
                  strtmp = (*kt)->GetSeq();
                  if (!strtmp.empty()) {
                      rev_seq = true;
                      qual2src_idx["rev_primer_seq"].push_back(i);
                  }
               }
               if (rev_nm && rev_seq) break;
            }
          }
          if (fwd_nm && fwd_seq && rev_nm && rev_seq) break;
       }
    }
    
    // genomic
    if ((*it)->GetGenome() != CBioSource::eGenome_unknown) 
             qual2src_idx["location"].push_back(i);
    
    i++;
  } 
};


static vector <string> comb_desc_ls;
static vector <CConstRef <CBioSource> > comb_src_ls;
static Str2Ints comb_qual2src_idx;
void CSeqEntry_test_on_quals :: TestOnObj(const CSeq_entry& seq_entry)
{
   if (thisTest.is_Quals_run) return;
   vector <string> this_desc_ls;
   vector <CConstRef <CBioSource> > this_src_ls;
   Str2Ints this_qual2src_idx;

   string desc;
   ITERATE (vector <const CSeq_feat*>, it, biosrc_feat) {
     desc = GetDiscItemText(**it);
     comb_desc_ls.push_back(desc); // for combine_seqentry_reports
     const CBioSource& biosrc = (*it)->GetData().GetBiosrc();
     comb_src_ls.push_back(CConstRef <CBioSource> (&biosrc));  // for combine_seqentry_reports
     this_desc_ls.push_back(desc);
     this_src_ls.push_back(CConstRef <CBioSource> (&biosrc));
   }

   unsigned i=0;
   ITERATE (vector <const CSeqdesc*>, it, biosrc_seqdesc) {
      desc = GetDiscItemText(**it, *(biosrc_seqdesc_seqentry[i]));
      comb_desc_ls.push_back(desc);
      this_desc_ls.push_back(desc);
      const CBioSource& biosrc = (*it)->GetSource();
      comb_src_ls.push_back(CConstRef <CBioSource> (&biosrc));
      this_src_ls.push_back(CConstRef <CBioSource> (&biosrc));
      i++;
   }

   GetQual2SrcIdx(this_src_ls, this_desc_ls, this_qual2src_idx);
   GetQualDistribute(this_qual2src_idx, this_desc_ls, this_src_ls, GetName_bad());

   if (!this_qual2src_idx.empty()
         && thisInfo.test_item_list.find(GetName_asn1()) == thisInfo.test_item_list.end()) {
       thisInfo.test_item_list[GetName_asn1()].push_back("yes");
       thisInfo.test_item_list[GetName_asn1_oncall()].push_back("yes");
   }

   thisTest.is_Quals_run = true;
};


void CSeqEntry_DISC_SRC_QUAL_PROBLEM :: GetReport(CRef <CClickableItem>& c_item)
{
   GetReport_quals(c_item, GetName());
};


void CSeqEntry_DISC_SOURCE_QUALS_ASNDISC :: GetReport(CRef <CClickableItem>& c_item)
{
   thisInfo.test_item_list[GetName()].clear();
   GetQual2SrcIdx(comb_src_ls, comb_desc_ls, comb_qual2src_idx);
   GetQualDistribute(comb_qual2src_idx, comb_desc_ls, comb_src_ls, GetName());
   c_item->item_list = thisInfo.test_item_list[GetName()];
   GetReport_quals(c_item, GetName());
};


void CSeqEntry_DISC_SOURCE_QUALS_ASNDISC_oncaller :: GetReport(CRef <CClickableItem>& c_item)
{
   thisInfo.test_item_list.clear();
   GetQual2SrcIdx(comb_src_ls, comb_desc_ls, comb_qual2src_idx);
   GetQualDistribute(comb_qual2src_idx, comb_desc_ls, comb_src_ls, GetName());
   c_item->item_list = thisInfo.test_item_list[GetName()];
   GetReport_quals(c_item, GetName());
};



bool CSeqEntry_test_on_biosrc_orgmod :: HasConflict(const list <CRef <COrgMod> >& mods, const string& subname_rest, const COrgMod::ESubtype& check_type, const string& check_head)
{
  size_t pos;
  unsigned h_size = check_head.size();
  ITERATE (list <CRef <COrgMod> >, it, mods) {
     strtmp = (*it)->GetSubname();
     if ( (*it)->GetSubtype() == check_type && strtmp.substr(0, h_size) == check_head ) {
        if ( (pos = strtmp.find(';')) != string::npos) {
           if ( strtmp.substr(h_size, pos-h_size) == subname_rest ) return false;  // need to test
        }
        else if (strtmp.substr(h_size) == subname_rest) return false;
     }
  }
  return true;
};


bool CSeqEntry_test_on_biosrc_orgmod :: IsStrainInCultureCollectionForBioSource(const CBioSource& biosrc, const string& strain_head, const string& culture_head)
{
  COrgMod::ESubtype check_type;
  string check_head(kEmptyStr), subname_rest(kEmptyStr);
  const list < CRef < COrgMod > >& mods = biosrc.GetOrgname().GetMod();
  ITERATE (list <CRef <COrgMod> >, it, mods) {
    if ( (*it)->GetSubtype() == COrgMod::eSubtype_strain
                  && (*it)->GetSubname().substr(0, strain_head.size()) == strain_head) {
       check_type = COrgMod::eSubtype_culture_collection;
       check_head = culture_head;
       subname_rest = (*it)->GetSubname().substr(strain_head.size());
    }
    else if ( (*it)->GetSubtype() == COrgMod::eSubtype_culture_collection
                && (*it)->GetSubname().substr(0, culture_head.size()) == culture_head) {
       check_type = COrgMod::eSubtype_strain;
       check_head = strain_head;
       subname_rest = (*it)->GetSubname().substr(culture_head.size());
    }

    if (!check_head.empty()&& !subname_rest.empty() 
                          && HasConflict(mods, subname_rest, check_type, check_head) )
         return true;
  }

  return false;
};


void CSeqEntry_test_on_biosrc_orgmod :: BacterialTaxShouldEndWithStrain(const CBioSource& biosrc, const string& desc)
{
    if ( HasLineage(biosrc, "Bacteria") && biosrc.GetOrg().CanGetTaxname()) {
      string taxname = biosrc.GetOrg().GetTaxname();
      if (taxname.empty()) return;
      string subname;
      ITERATE (list <CRef <COrgMod> >, it, biosrc.GetOrgname().GetMod()) {
        if ( (*it)->GetSubtype() == COrgMod::eSubtype_strain) {
           subname = (*it)->GetSubname();
           size_t pos = taxname.find(subname);
           if (pos == string::npos || subname.size() != (taxname.size() - pos + 1))
                 thisInfo.test_item_list[GetName_mism()].push_back(desc);
        }
      }
    }
};


void CSeqEntry_test_on_biosrc_orgmod :: BiosrcHasConflictingStrainAndCultureCollectionValues(const CBioSource& biosrc, const string& desc)
{
   bool has_match = false, has_conflict = false;
   string extra_str(kEmptyStr), subnm1, subnm2;

   const list < CRef < COrgMod > >& mods = biosrc.GetOrgname().GetMod();
   ITERATE (list < CRef < COrgMod > >, it, mods) {
     if ((*it)->GetSubtype() == COrgMod :: eSubtype_strain) {
       ITERATE (list < CRef < COrgMod > >, jt, mods) {
         if ( (*jt)->GetSubtype() == COrgMod :: eSubtype_culture_collection) {
            subnm1 = (*it)->GetSubname();
            subnm2 = (*jt)->GetSubname();
            RmvChar(subnm1, ": ");
            RmvChar(subnm2, ": ");
            if (subnm1 == subnm2) {
                   has_match = true;
                   break;
            }
            else {
               extra_str = "        " + (*it)->GetSubtypeName( (*it)->GetSubtype(),
                                                     COrgMod::eVocabulary_insdc ) + ": "
                                + (*it)->GetSubname() + "\n        "
                                + (*jt)->GetSubtypeName( (*jt)->GetSubtype(),
                                                     COrgMod::eVocabulary_insdc ) + ": "
                                + (*jt)->GetSubname();
               has_conflict = true;
            }
         }
       };
       if (has_match) break;
     }
   }
   if (has_conflict && !has_match)
       thisInfo.test_item_list[GetName_cul()].push_back(desc + "\n" + extra_str); 
};


bool CSeqEntry_test_on_biosrc_orgmod :: IsMissingRequiredStrain(const CBioSource& biosrc)
{
  if (!HasLineage(biosrc, "Bacteria")) return false;
  ITERATE (list <CRef <COrgMod> >, it, biosrc.GetOrgname().GetMod())
       if ( (*it)->GetSubtype() == COrgMod :: eSubtype_strain) return false;

  return true;
};



void CSeqEntry_test_on_biosrc_orgmod :: RunTests(const CBioSource& biosrc, const string& desc)
{
  // DUP_DISC_ATCC_CULTURE_CONFLICT
  if (IsStrainInCultureCollectionForBioSource(biosrc, "ATCC ", "ATCC:"))
        thisInfo.test_item_list[GetName_atcc()].push_back(desc);
         
  // DUP_DISC_CBS_CULTURE_CONFLICT
  if (IsStrainInCultureCollectionForBioSource(biosrc, "CBS ", "CBS:"))
          thisInfo.test_item_list[GetName_cbs()].push_back(desc);

  // DISC_BACTERIAL_TAX_STRAIN_MISMATCH
  BacterialTaxShouldEndWithStrain(biosrc, desc);

  // ONCALLER_STRAIN_CULTURE_COLLECTION_MISMATCH
  BiosrcHasConflictingStrainAndCultureCollectionValues(biosrc, desc);

  if (IsMissingRequiredStrain(biosrc)) {
      // DISC_REQUIRED_STRAIN
      thisInfo.test_item_list[GetName_strain()].push_back(desc);
      
      // DISC_BACTERIA_MISSING_STRAIN
      thisInfo.test_item_list[GetName_sp_strain()].push_back(desc);
  }
  else { // DISC_BACTERIA_MISSING_STRAIN
     if (HasMissingBacteriaStrain(biosrc))
       thisInfo.test_item_list[GetName_sp_strain()].push_back(desc);
  }

  // DISC_METAGENOME_SOURCE
  if (IsOrgModPresent(biosrc, COrgMod::eSubtype_metagenome_source)) 
      thisInfo.test_item_list[GetName_meta()].push_back(desc); 

  // ONCALLER_CHECK_AUTHORITY
  bool has_tax = false;
  if (biosrc.IsSetTaxname() && !biosrc.GetTaxname().empty()) {
      has_tax = true;
      if (DoAuthorityAndTaxnameConflict(biosrc))
        thisInfo.test_item_list[GetName_auth()].push_back(desc);
  }

  // ONCALLER_MULTIPLE_CULTURE_COLLECTION
  string cul_vlus;
  if (HasMultipleCultureCollection(biosrc, cul_vlus))
     thisInfo.test_item_list[GetName_mcul()].push_back(desc + " " + cul_vlus);

  // DISC_HUMAN_HOST
  vector <string> arr;
  GetOrgModValues(biosrc, COrgMod::eSubtype_nat_host, arr);
  ITERATE (vector <string>, it, arr) {
    if ( DoesStringContainPhrase(*it, "human", false, true) ) {
         thisInfo.test_item_list[GetName_human()].push_back(desc); 
         break;
    }
  }
  arr.clear();

  // TEST_UNNECESSARY_ENVIRONMENTAL
  if (has_tax && HasUnnecessaryEnvironmental(biosrc))
      thisInfo.test_item_list[GetName_env()].push_back(desc);

  // TEST_AMPLIFIED_PRIMERS_NO_ENVIRONMENTAL_SAMPLE
  if (AmpPrimersNoEnvSample(biosrc))
     thisInfo.test_item_list[GetName_amp()].push_back(desc);
};


bool CSeqEntry_test_on_biosrc_orgmod :: AmpPrimersNoEnvSample(const CBioSource& biosrc)
{
   string note("amplified with species-specific primers");
   vector <string> arr;
   if (IsSubSrcPresent(biosrc, CSubSource::eSubtype_environmental_sample)) return false;
   GetSubSrcValues(biosrc, CSubSource::eSubtype_other, arr);
   GetOrgModValues(biosrc, COrgMod::eSubtype_other, arr);
   ITERATE (vector <string>, it, arr)
      if (NStr::FindNoCase(*it, note) != string::npos) return true;

   return false;                     
};


void CSeqEntry_TEST_AMPLIFIED_PRIMERS_NO_ENVIRONMENTAL_SAMPLE :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "biosource") 
     + "'amplified with species-specific primers' note but no environmental-sample qualifier.";
};


bool CSeqEntry_test_on_biosrc_orgmod :: HasUnnecessaryEnvironmental(const CBioSource& biosrc)
{
   bool found = false, has_note = false, has_metagenomic = false;

   if (!biosrc.CanGetSubtype()) return false;
   found = IsSubSrcPresent(biosrc, CSubSource::eSubtype_environmental_sample);
   vector <string> arr;
   GetSubSrcValues(biosrc, CSubSource::eSubtype_other, arr);
   ITERATE (vector <string>, it, arr)
     if (NStr::FindNoCase(*it, "amplified with species-specific primers")!=string::npos){
          has_note = true; break;
     }
   has_metagenomic = IsSubSrcPresent(biosrc, CSubSource::eSubtype_metagenomic);

   if (!found || has_note) return false;
   strtmp = biosrc.GetTaxname();
   ITERATE (vector <string>, it, thisInfo.taxnm_env) 
       if (NStr::FindNoCase(strtmp, *it) != string::npos) return false;
   arr.clear();
   GetOrgModValues(biosrc, COrgMod::eSubtype_other, arr);
   ITERATE (vector <string>, it, arr)
     if ( NStr::FindNoCase(*it, "amplified with species-specific primers")!=string::npos)
             return false;
   return true;
};


void CSeqEntry_TEST_UNNECESSARY_ENVIRONMENTAL :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetHasComment(c_item->item_list.size(), "biosource") 
                        + "unnecessary environmental qualifier";
};


void CSeqEntry_DISC_HUMAN_HOST :: GetReport(CRef <CClickableItem>& c_item)
{
   unsigned cnt = c_item->item_list.size();
   c_item->description
     = GetHasComment(cnt, "organism") + "'human' host qualifier" + (cnt >1 ? "s" : "");
};


bool CSeqEntry_test_on_biosrc_orgmod :: HasMultipleCultureCollection (const CBioSource& biosrc, string& cul_vlus)
{
  bool has_one = false;
  cul_vlus = kEmptyStr; 
  strtmp 
    =COrgMod::ENUM_METHOD_NAME(ESubtype)()->FindName(COrgMod::eSubtype_culture_collection,true)
      + ": ";
  ITERATE (list <CRef <COrgMod> >, it, biosrc.GetOrgname().GetMod() ) {  
    if ( (*it)->GetSubtype() == COrgMod::eSubtype_culture_collection) {
       if (!has_one) has_one = true;
       if ( !(*it)->GetSubname().empty() ) {
         cul_vlus += strtmp + (*it)->GetSubname() + " ";
       }
    }
  }
  if (has_one) return true;
  else return false;
};


void CSeqEntry_ONCALLER_MULTIPLE_CULTURE_COLLECTION :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetHasComment(c_item->item_list.size(), "organism") 
                       + "multiple culture-collection qualifiers.";
};


bool CSeqEntry_test_on_biosrc_orgmod :: DoAuthorityAndTaxnameConflict(const CBioSource& biosrc)
{
  vector <string> auty_strs;
  GetOrgModValues(biosrc, COrgMod::eSubtype_authority, auty_strs);
  if (auty_strs.empty()) return false;

  strtmp = biosrc.GetTaxname();
  size_t pos = strtmp.find(' ');
  unsigned len;
  if (pos != string::npos) pos == strtmp.substr(pos).find(' ');
  if (pos == string::npos) len = strtmp.size();
  else len = pos;
  ITERATE (vector <string>, it, auty_strs) {
    if ( (*it).size() < len || (*it).substr(0, len) == strtmp) return true;
  }
  return false; 
};


void CSeqEntry_ONCALLER_CHECK_AUTHORITY :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description 
     = GetHasComment(c_item->item_list.size(), "biosource") + "taxname/authority conflict";
};


void CSeqEntry_DISC_METAGENOME_SOURCE :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description
    = GetHasComment(c_item->item_list.size(), "biosource") + "metagenome_source qualifier";
};


bool CSeqEntry_test_on_biosrc_orgmod :: HasMissingBacteriaStrain(const CBioSource& biosrc)
{
   if (!HasLineage(biosrc, "Bacteria")) return false;
   string tax_nm(biosrc.IsSetTaxname() ? biosrc.GetTaxname() : kEmptyStr);
   size_t pos;
   if (tax_nm.empty() 
           || NStr::FindNoCase(tax_nm, "enrichment culture clone") != string::npos
           || (pos = tax_nm.find(" sp. ")) == string::npos) return false;
   tax_nm = tax_nm.substr(pos+5);
   if (tax_nm.empty() 
           || (tax_nm.find('(') != string::npos && tax_nm[tax_nm.size()-1] == ')')) 
            return false;
   if (NStr::FindNoCase(tax_nm, "enrichment culture clone") != string::npos) return false;
   ITERATE (list <CRef <COrgMod> >, it, biosrc.GetOrgname().GetMod()) {
       if ( (*it)->GetSubtype() == COrgMod :: eSubtype_strain && (*it)->GetSubname() == tax_nm)
            return false;
   }
   return true;
};


void CSeqEntry_DISC_BACTERIA_MISSING_STRAIN :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description
     = GetHasComment(c_item->item_list.size(), "bacterial biosource")
       + "taxname 'Genus sp. strain' but no strain";
};


void CSeqEntry_test_on_biosrc_orgmod :: TestOnObj(const CSeq_entry& seq_entry)
{
   if (thisTest.is_Biosrc_Orgmod_run) return;

   string desc;
   ITERATE (vector <const CSeq_feat*>, it, biosrc_orgmod_feat)
     RunTests((*it)->GetData().GetBiosrc(), GetDiscItemText(**it));

   unsigned i=0;
   ITERATE (vector <const CSeqdesc*>, it, biosrc_orgmod_seqdesc) {
      desc = GetDiscItemText(**it, *(biosrc_orgmod_seqdesc_seqentry[i]));
      RunTests((*it)->GetSource(), desc);
      i++;
   }

   thisTest.is_Biosrc_Orgmod_run = true;
};


void CSeqEntry_DISC_REQUIRED_STRAIN :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetIsComment(c_item->item_list.size(), "biosource")
                                                         + "missing required strain value";
};


void CSeqEntry_DUP_DISC_ATCC_CULTURE_CONFLICT :: GetReport(CRef <CClickableItem>& c_item)
{
  RmvRedundancy(c_item->item_list); 
  c_item->description = GetHasComment(c_item->item_list.size(), "biosource")
                          + "conflicting ATCC strain and culture collection values.";
};


void CSeqEntry_DUP_DISC_CBS_CULTURE_CONFLICT :: GetReport(CRef <CClickableItem>& c_item)
{
   RmvRedundancy(c_item->item_list); 
   c_item->description = GetHasComment(c_item->item_list.size(), "biosource")
                          + "conflicting CBS strain and culture collection values.";
};


void CSeqEntry_DISC_BACTERIAL_TAX_STRAIN_MISMATCH :: GetReport(CRef <CClickableItem>& c_item)
{
   RmvRedundancy(c_item->item_list); 
   c_item->description = GetHasComment(c_item->item_list.size(), "biosource") 
                           + "tax name/strain mismatch.";
};


void CSeqEntry_ONCALLER_STRAIN_CULTURE_COLLECTION_MISMATCH :: GetReport(CRef <CClickableItem>& c_item) 
{
   RmvRedundancy(c_item->item_list); 
   c_item->description = GetHasComment(c_item->item_list.size(), "organism")
                          + "conflicting strain and culture-collection values";
};



bool CSeqEntry_test_on_biosrc :: HasMulSrc(const CBioSource& biosrc)
{
  if (biosrc.IsSetOrgMod()) {
    ITERATE (list <CRef <COrgMod> >, it, biosrc.GetOrgname().GetMod()) {
      const int subtype = (*it)->GetSubtype();
      const string& subname = (*it)->GetSubname();
      if ( (subtype == COrgMod::eSubtype_strain || subtype == COrgMod::eSubtype_isolate)
             && ( subname.find(",") !=string::npos || subname.find(";") !=string::npos) )
         return true;
    }
    return false;
  }
  else return false;
};


bool CSeqEntry_test_on_biosrc :: IsBacterialIsolate(const CBioSource& biosrc)
{
   if (!HasLineage(biosrc, "Bacteria")
         || !biosrc.IsSetOrgMod()
         || HasAmplifiedWithSpeciesSpecificPrimerNote(biosrc)) return false;

   ITERATE (list <CRef <COrgMod> >, it, biosrc.GetOrg().GetOrgname().GetMod()) {
     strtmp = (*it)->GetSubname();
     if ( (*it)->GetSubtype() == COrgMod::eSubtype_isolate
             && !NStr::EqualNocase(strtmp, 0, 13, "DGGE gel band")
             && !NStr::EqualNocase(strtmp, 0, 13, "TGGE gel band")
             && !NStr::EqualNocase(strtmp, 0, 13, "SSCP gel band"))
        return true;
   }
   return false;
};

bool CSeqEntry_test_on_biosrc :: DoTaxonIdsMatch(const COrg_ref& org1, const COrg_ref& org2)
{
   if (!org1.CanGetDb() || org1.GetDb().empty()
             || !org2.CanGetDb() || org2.GetDb().empty())
      return false;

   ITERATE (vector <CRef <CDbtag> >, it, org1.GetDb()) {
       if ((*it)->GetType() == CDbtag::eDbtagType_taxon) {
          ITERATE (vector <CRef <CDbtag> >, jt, org2.GetDb()) {
             if ((*jt)->GetType() == CDbtag::eDbtagType_taxon) {
                if ((*it)->Match(**jt)) return true;
                else return false;
             }
          }
          return false;
       }
   }
   return false;
};


bool CSeqEntry_test_on_biosrc :: DoInfluenzaStrainAndCollectionDateMisMatch(const CBioSource& biosrc)
{
   string digits("1234567890");
   string flu_substr, tax_name, year, coll_year;
   size_t pos;
   if (!biosrc.IsSetTaxname() || (tax_name=biosrc.GetTaxname()).empty()) return false;
//   tax_name = biosrc.GetTaxname();
   flu_substr = tax_name.substr(0, 18);
   if (flu_substr == "Influenza A virus ") {
     if ( (pos = tax_name.find("(")) != string::npos) {
       if ( (pos = tax_name.substr(pos+1).find("(")) != string::npos) {
          strtmp = tax_name.substr(0, pos);
          if ( (pos = strtmp.find_last_not_of(" ")) != string::npos) {
             if (isdigit(strtmp[pos])) {
               pos = strtmp.substr(0, pos).find_last_not_of(digits); 
               year = NStr::StringToUInt(strtmp.substr(pos+1));
             }
          };
       }
     }
   }
   else if (flu_substr == "Influenza B virus ") {
     if ( (pos = tax_name.find("/")) != string::npos) {
       if ( (pos = tax_name.substr(pos+1).find_first_not_of(" ")) != string::npos) {
         if (isdigit(tax_name[pos])) 
           year = tax_name.substr(pos);
       }
     }
   }
   else return false;

   if (!year.empty()) {
     if (biosrc.CanGetSubtype()) {
        ITERATE (list< CRef< CSubSource > >, it, biosrc.GetSubtype()) {
           if ( (*it)->GetSubtype() == CSubSource::eSubtype_collection_date) {
              strtmp = (*it)->GetName();
              if ( (pos = strtmp.find("-")) == string::npos) {
                 coll_year = strtmp;
              }
              else if (!isdigit(strtmp[pos+1])) return true;
              else coll_year = strtmp.substr(pos+1);
              if (year == coll_year) return true;
              else return false;
           }
        }
     }
   }
   return true;
};


void CSeqEntry_test_on_biosrc :: AddMissingViralQualsDiscrepancies(const CBioSource& biosrc, const string& desc)
{
  bool has_collection_date = false, has_country = false, has_specific_host = false;
  if (biosrc.CanGetSubtype()) {
    ITERATE (list <CRef <CSubSource> >, it, biosrc.GetSubtype()) {
      if (!has_collection_date 
            && (*it)->GetSubtype() == CSubSource :: eSubtype_collection_date) 
        has_collection_date = true;
      if (!has_country 
                && (*it)->GetSubtype() == CSubSource :: eSubtype_country)
         has_country = true;
      if (has_collection_date && has_country) break;
    }
  }

  if (biosrc.IsSetOrgname() && biosrc.GetOrgname().CanGetMod()) {
    ITERATE (list< CRef< COrgMod > >, it, biosrc.GetOrgname().GetMod()) {
      if ( (*it)->GetSubtype() == COrgMod :: eSubtype_nat_host) {
        has_specific_host = true;
        break;
      }
    }
  }

  if (!has_collection_date) 
    thisInfo.test_item_list[GetName_quals()].push_back("collection date$" + desc);
  if (!has_country)
    thisInfo.test_item_list[GetName_quals()].push_back("country$" + desc);
  if (!has_specific_host)
    thisInfo.test_item_list[GetName_quals()].push_back("specific-host$" + desc);
};


bool CSeqEntry_test_on_biosrc :: HasMoreOrSpecNames(const CBioSource& biosrc, CSubSource::ESubtype subtype, bool check_mul_nm_only)
{
   string name;
   vector <string> name_ls;
   if (biosrc.CanGetSubtype()) {
     ITERATE (list <CRef <CSubSource> >, it, biosrc.GetSubtype()) {
       if ( (*it)->GetSubtype() == subtype) {
          name = (*it)->GetName();
          name_ls = NStr::Tokenize(name, ",;", name_ls, NStr::eMergeDelims);
          if (name_ls.size() >= 3) return true;
          else if (!check_mul_nm_only) {
            ITERATE (vector <string>, jt, thisInfo.spec_words_biosrc)
              if (NStr::FindNoCase(name, *jt) != string::npos) return true;
          }
          if (!check_mul_nm_only) {
              ITERATE (vector <string>, it, m_submit_text)
                  if ( (*it) == name) return true;
          }
       }
     }
   }
   return false;
};


bool CSeqEntry_test_on_biosrc :: IsMissingRequiredClone (const CBioSource& biosrc)
{
   bool has_clone = false, needs_clone = false;
   if (HasAmplifiedWithSpeciesSpecificPrimerNote(biosrc)) return false;
   if (biosrc.IsSetTaxname()
         && NStr::FindNoCase(biosrc.GetTaxname(), "uncultured") != string::npos)
      needs_clone = true;
   if (IsSubSrcPresent(biosrc, CSubSource :: eSubtype_environmental_sample)
                     || IsSubSrcPresent(biosrc, CSubSource :: eSubtype_clone))
         has_clone = true;
   if (needs_clone && !has_clone) {  // look for gel band isolate
     if (biosrc.IsSetOrgMod()) {
       ITERATE (list <CRef <COrgMod> >, it, biosrc.GetOrgname().GetMod()) {
          if ( (*it)->GetSubtype() == COrgMod :: eSubtype_isolate 
                && NStr::FindNoCase((*it)->GetSubname(), "gel band") != string::npos) {
                   needs_clone = false;  break;
          }
       }
     }
   }
   if (needs_clone && !has_clone) return true;
   return false;
};


void CSeqEntry_test_on_biosrc ::RunTests(const CBioSource& biosrc, const string& desc, int idx)
{
  // DISC_REQUIRED_CLONE
  if (IsMissingRequiredClone(biosrc))
      thisInfo.test_item_list[GetName_clone()].push_back(desc);

  // ONCALLER_MULTISRC
  if (HasMulSrc( biosrc ))   thisInfo.test_item_list[GetName_mult()].push_back(desc);

  // DISC_BACTERIA_SHOULD_NOT_HAVE_ISOLATE
  if (IsBacterialIsolate(biosrc))   
               thisInfo.test_item_list[GetName_iso()].push_back(desc);

  // TAX_LOOKUP_MISSING
  string org_tax, db_tax;
  CRef <CTaxon2_data> lookup_tax = thisInfo.tax_db_conn.Lookup(biosrc.GetOrg());
  if (lookup_tax.Empty() || !(lookup_tax->CanGetOrg()))
      thisInfo.test_item_list[GetName_tmiss()].push_back(desc); 
  else {
    org_tax = biosrc.GetOrg().CanGetTaxname()? biosrc.GetOrg().GetTaxname() : kEmptyStr;
    db_tax =lookup_tax->GetOrg().CanGetTaxname() ?
                                  lookup_tax->GetOrg().GetTaxname() : kEmptyStr;
    if (org_tax != db_tax) thisInfo.test_item_list[GetName_tbad()].push_back(desc);
    else {
      if (!DoTaxonIdsMatch(biosrc.GetOrg(), lookup_tax->GetOrg()))
          thisInfo.test_item_list[GetName_tbad()].push_back(desc);
    }
  }
 
  // DISC_INFLUENZA_DATE_MISMATCH
  if (DoInfluenzaStrainAndCollectionDateMisMatch(biosrc))
     thisInfo.test_item_list[GetName_flu()].push_back(desc);

  // DISC_MISSING_VIRAL_QUALS
  AddMissingViralQualsDiscrepancies(biosrc, desc);

  // MORE_OR_SPEC_NAMES_IDENTIFIED_BY
  if ( HasMoreOrSpecNames( biosrc, CSubSource::eSubtype_identified_by))
        thisInfo.test_item_list[GetName_iden()].push_back(desc);

  // MORE_NAMES_COLLECTED_BY
  if ( HasMoreOrSpecNames( biosrc, CSubSource::eSubtype_collected_by, true))
        thisInfo.test_item_list[GetName_col()].push_back(desc);

  // DIVISION_CODE_CONFLICTS
  string div_code;
  if (biosrc.IsSetDivision() && !(div_code= biosrc.GetDivision()).empty())
     thisInfo.test_item_list[GetName_div()].push_back(div_code + "$" + desc);

  // DISC_MAP_CHROMOSOME_CONFLICT
  bool has_map = false, has_chrom = false;
  if (idx >=0 && biosrc.CanGetSubtype()) {
    has_map = IsSubSrcPresent(biosrc, CSubSource :: eSubtype_map);
    has_chrom = IsSubSrcPresent(biosrc, CSubSource :: eSubtype_chromosome);
    if (has_map && !has_chrom) {
      if (biosrc_seqdesc_seqentry[idx]->IsSeq()) {
          const CBioseq& bioseq = biosrc_seqdesc_seqentry[idx]->GetSeq();
          if (IsBioseqHasLineage(bioseq, "Eukaryota", false) && !bioseq.IsAa()) 
              thisInfo.test_item_list[GetName_map()].push_back(GetDiscItemText(bioseq)); 
      }
      else AddEukaryoticBioseqsToReport(biosrc_seqdesc_seqentry[idx]->GetSet());
    }
  }

  // DISC_TRINOMIAL_SHOULD_HAVE_QUALIFIER
  if (FindTrinomialWithoutQualifier(biosrc))
      thisInfo.test_item_list[GetName_trin()].push_back(desc);

  // DISC_METAGENOMIC
  if (IsSubSrcPresent(biosrc, CSubSource::eSubtype_metagenomic))
      thisInfo.test_item_list[GetName_meta()].push_back(desc); 

  // TEST_SP_NOT_UNCULTURED
  unsigned len;
  if (!org_tax.empty()) {
     len = org_tax.size();
     if (len >= 4 && org_tax.substr(len-4) == " sp."
           && !NStr::EqualNocase(org_tax.substr(0, 11), "uncultured "))
        thisInfo.test_item_list[GetName_sp()].push_back(desc);
  }

  // TEST_MISSING_PRIMER
  if (MissingPrimerValue(biosrc)) 
       thisInfo.test_item_list[GetName_prim()].push_back(desc);
 
  // ONCALLER_COUNTRY_COLON
  vector <string> src_strs, arr;
  GetSubSrcValues(biosrc, CSubSource::eSubtype_country, src_strs);
  if (!src_strs.empty()) {
      ITERATE (vector <string>, it, src_strs) {
        arr.clear();
        arr = NStr::Tokenize(*it, ":", arr);
        if (arr.size() > 1) {
            thisInfo.test_item_list[GetName_cty()].push_back(desc); break;
        }
      }
  }
  arr.clear();
 
  // ONCALLER_DUPLICATE_PRIMER_SET
  if (biosrc.CanGetPcr_primers()) {
     const list <CRef <CPCRReaction> > pcr_ls = biosrc.GetPcr_primers().Get();
     list <CRef <CPCRReaction> > :: const_iterator it, jt;
     it = jt = pcr_ls.begin();
     for (jt; jt != pcr_ls.end(); jt++) {
        if ( (it != jt) && SamePCRReaction(**it, **jt))
           thisInfo.test_item_list[GetName_pcr()].push_back(desc);
     }
  }
};

void CSeqEntry_test_on_biosrc :: IniMap(const list <CRef <CPCRPrimer> >& ls, Str2Int& map)
{
   string pcr_str;
   ITERATE (list <CRef <CPCRPrimer> >, it, ls) {
     pcr_str = (*it)->CanGetSeq() ? (*it)->GetSeq() : kEmptyStr;
     pcr_str = NStr::ToUpper(pcr_str) + "$";
     strtmp = (*it)->CanGetName() ? (*it)->GetName() : kEmptyStr;
     pcr_str += NStr::ToUpper(strtmp);
     if (map.find(pcr_str) == map.end()) map[pcr_str] = 1;
     else map[pcr_str] ++;
   }
};

bool CSeqEntry_test_on_biosrc :: SamePrimerList(const list <CRef <CPCRPrimer> >& ls1,
const list <CRef <CPCRPrimer> >& ls2)
{
   if ( (ls1.size() != ls2.size()) || ls1.empty()) return false;
   Str2Int map1, map2;
   IniMap(ls1, map1);
   IniMap(ls2, map1);
   if (map1.size() != map2.size()) return false;
   if (map1 == map2) return true;
   else return false;
};

bool CSeqEntry_test_on_biosrc :: SamePCRReaction(const CPCRReaction& pcr1, const CPCRReaction& pcr2)
{
  bool has_fwd1 = pcr1.CanGetForward()? true : false;
  bool has_rev1 = pcr1.CanGetReverse()? true : false;
  bool has_fwd2 = pcr1.CanGetForward()? true : false;
  bool has_rev2 = pcr1.CanGetReverse()? true : false;

  if ((has_fwd1 != has_fwd2) || (has_rev1 != has_rev2) || (!has_fwd1 && !has_rev1)) 
            return false;
  if (has_fwd1) { 
    if (SamePrimerList(pcr1.GetForward().Get(), pcr2.GetForward().Get()))
        return (SamePrimerList(pcr1.GetReverse().Get(), pcr2.GetReverse().Get()));
  }
  else if (has_rev1)
       return (SamePrimerList(pcr1.GetReverse().Get(), pcr2.GetReverse().Get()));
  else return false;
};

void CSeqEntry_ONCALLER_DUPLICATE_PRIMER_SET :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
     = GetHasComment(c_item->item_list.size(), "BioSource") + "duplicate primer pairs.";
};

void CSeqEntry_ONCALLER_COUNTRY_COLON :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
     = GetHasComment(c_item->item_list.size(), "country source") + "more than 1 colon.";
};

bool CSeqEntry_test_on_biosrc :: MissingPrimerValue(const CBioSource& biosrc)
{
  bool has_fwd, has_rev, seq_f, seq_r, name_f, name_r;
  list <CRef <CPCRPrimer> >::const_iterator fwd_it, rev_it;
  if (biosrc.CanGetPcr_primers()) {
     ITERATE (list <CRef <CPCRReaction> >, it, biosrc.GetPcr_primers().Get()) {
        has_fwd = (*it)->CanGetForward();
        has_rev = (*it)->CanGetReverse();
        if (!has_fwd && !has_rev) continue;
        if ( has_fwd != has_rev) return true;
        else {
           const list <CRef <CPCRPrimer> >& fwd = (*it)->GetForward().Get();
           const list <CRef <CPCRPrimer> >& rev = (*it)->GetReverse().Get();
           if (fwd.size() != rev.size() ) return true;
           for (fwd_it = fwd.begin(), rev_it = rev.begin(); fwd_it != fwd.end(); 
                                                                     fwd_it++, rev_it++) {
              IsFwdRevDataPresent(*fwd_it, seq_f, name_f);
              IsFwdRevDataPresent(*rev_it, seq_r, name_r);
              if ((seq_f != seq_r) || (name_f != name_r)) return true;
           }
        }
     }
  }
  return false;
};


void CSeqEntry_test_on_biosrc :: IsFwdRevDataPresent(const CRef <CPCRPrimer>& primer, bool& has_seq, bool& has_name)
{
   has_seq = (primer->CanGetSeq() && !primer->GetSeq().Get().empty()) ? true : false;
   has_name = (primer->CanGetName() && !primer->GetName().Get().empty()) ? true : false;
};


void CSeqEntry_TEST_MISSING_PRIMER :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
      = GetHasComment(c_item->item_list.size(),"biosource") +"primer sets with missing values";
};


void CSeqEntry_TEST_SP_NOT_UNCULTURED :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "biosource") 
                     + "taxnames that end with ' sp.' but do not start with 'uncultured'";
};


void CSeqEntry_DISC_METAGENOMIC :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
      = GetHasComment(c_item->item_list.size(), "biosource") + "metagenomic qualifier";
};


bool CSeqEntry_test_on_biosrc :: FindTrinomialWithoutQualifier(const CBioSource& biosrc)
{
  if (!biosrc.IsSetTaxname() || biosrc.GetTaxname().empty() 
         || NStr::FindNoCase(biosrc.GetTaxname(), " x ") != string::npos) return false;
  if (HasLineage(biosrc, "Viruses")) return false;
  string tax_nm = biosrc.GetTaxname();
  size_t pos;
  ITERATE (Str2Str, it, thisInfo.srcqual_keywords) {
     if ( (pos = NStr::FindNoCase(tax_nm, it->second)) != string::npos) {
        tax_nm 
           = NStr::TruncateSpaces(tax_nm.substr(pos+ (it->second).size()), NStr::eTrunc_Begin);
        if (!tax_nm.empty()) {
           vector <string> arr;
           GetOrgModValues(biosrc, it->first, arr);
           ITERATE (vector <string>, jt, arr)
             if (tax_nm.substr(0, (*jt).size()) == (*jt)) return true;
        } 
        break;
     } 
  } 
  return false;
};


void CSeqEntry_DISC_TRINOMIAL_SHOULD_HAVE_QUALIFIER :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
     = GetOtherComment(c_item->item_list.size(), "trinomial source lacks", 
                                                              "trinomial sources lack")
       + "  corresponding qualifier";

};


void CSeqEntry_test_on_biosrc :: AddEukaryoticBioseqsToReport(const CBioseq_set& set)
{
   ITERATE (list < CRef < CSeq_entry > >, it, set.GetSeq_set()) {
      if ((*it)->IsSeq()) {
         const CBioseq& bioseq = (*it)->GetSeq();
         if ( IsBioseqHasLineage(bioseq, "Eukaryota", false) && !bioseq.IsAa())
           thisInfo.test_item_list[GetName_map()].push_back(GetDiscItemText(bioseq));
         else AddEukaryoticBioseqsToReport( (*it)->GetSet() );
     }
   }
};



void CSeqEntry_test_on_biosrc :: GetSubmitText(const CAuth_list& authors)
{
   if (authors.CanGetAffil()) {
     const CAffil& affil = authors.GetAffil();
     if (affil.IsStd()) {
        if (affil.GetStd().CanGetAffil()) {
             strtmp = affil.GetStd().GetAffil();
             if (!strtmp.empty()) m_submit_text.push_back(strtmp);
        }
        if (affil.GetStd().CanGetDiv()) {
             strtmp = affil.GetStd().GetDiv();
             if (!strtmp.empty()) m_submit_text.push_back(strtmp);
        }
     }
   }
};


void CSeqEntry_test_on_biosrc :: FindSpecSubmitText()
{
  string inst(kEmptyStr), dept(kEmptyStr);
  if (thisInfo.seq_submit.NotEmpty()) {
    GetSubmitText(thisInfo.seq_submit->GetSub().GetCit().GetAuthors());
  }

  ITERATE (vector <const CSeq_feat*>, it, pub_feat) {
     ITERATE (list < CRef < CPub > >, jt, (*it)->GetData().GetPub().GetPub().Get())
       if ( (*jt)->IsSetAuthors()) GetSubmitText((*jt)->GetAuthors());
  }

  ITERATE (vector <const CSeqdesc*>, it, pub_seqdesc) {
     ITERATE (list < CRef < CPub > >, jt, (*it)->GetPub().GetPub().Get())
       if ( (*jt)->IsSetAuthors()) GetSubmitText((*jt)->GetAuthors());
  }

};

void CSeqEntry_test_on_biosrc :: TestOnObj(const CSeq_entry& seq_entry)
{
   if (thisTest.is_BIOSRC1_run) return;

   m_submit_text.clear(); 
   FindSpecSubmitText();

   string desc;
   ITERATE (vector <const CSeq_feat*>, it, biosrc_feat)
     RunTests((*it)->GetData().GetBiosrc(), GetDiscItemText(**it));

   unsigned i=0;
   ITERATE (vector <const CSeqdesc*>, it, biosrc_seqdesc) {
     desc = GetDiscItemText( **it, *(biosrc_seqdesc_seqentry[i]));
     RunTests((*it)->GetSource(), desc, (int)i);
     i++;
   };

  thisTest.is_BIOSRC1_run = true;
};


void CSeqEntry_DISC_REQUIRED_CLONE :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetIsComment(c_item->item_list.size(), "biosource")
                          + "missing required clone value";
};


void CSeqEntry_DISC_MAP_CHROMOSOME_CONFLICT :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetOtherComment(c_item->item_list.size(), 
                "source on eukaryotic sequence has", "sources on eukaryotic sequences have") 
         + " map but not chromosome.";
};
  


void CSeqEntry_DIVISION_CODE_CONFLICTS :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs div2item;
   GetTestItemList(c_item->item_list, div2item);
   c_item->item_list.clear();

   if (div2item.size() > 1) {
     ITERATE (Str2Strs, it, div2item) {
       AddSubcategories(c_item, GetName(), it->second, "bioseq", "division code " + it->first,
                           e_HasComment, false);
     }
     c_item->description = "Division code conflicts found.";
   }
};



void CSeqEntry_MORE_NAMES_COLLECTED_BY :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetHasComment(c_item->item_list.size(), "biosource")
                          + "3 or more names in collected-by.";
};


void CSeqEntry_MORE_OR_SPEC_NAMES_IDENTIFIED_BY :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "biosource")
                          + "3 or more names or suspect text in identified-by.";
};


void CSeqEntry_DISC_MISSING_VIRAL_QUALS :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs qual2src;
   GetTestItemList(c_item->item_list, qual2src);
   c_item->item_list.clear();
   ITERATE (Str2Strs, it, qual2src) {
      AddSubcategories(c_item, GetName(), it->second, "virus organism", 
                         "missing suggested qualifier " + it->first);
   } 
   RmvRedundancy(c_item->item_list);
   unsigned cnt = c_item->item_list.size();
   c_item->description 
       = GetIsComment(cnt, "virus organism") + GetNoun(cnt, "missing required qualifier");
};


void CSeqEntry_DISC_INFLUENZA_DATE_MISMATCH :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
     = GetOtherComment(c_item->item_list.size(), "influenza strain conflicts", "influenza strains conflict")
        + " with collection date.";
};


void CSeqEntry_TAX_LOOKUP_MISSING :: GetReport(CRef <CClickableItem>& c_item)
{
   RmvRedundancy(c_item->item_list);  // maybe unnecessary?
   c_item->description
     = GetIsComment(c_item->item_list.size(), "organism") + "not found in taxonomy lookup.";
};


void CSeqEntry_TAX_LOOKUP_MISMATCH :: GetReport(CRef <CClickableItem>& c_item)
{
   RmvRedundancy(c_item->item_list); 
   c_item->description
     = GetDoesComment(c_item->item_list.size(), "tax name") + "not match taxonomy lookup.";
};


void CSeqEntry_DISC_BACTERIA_SHOULD_NOT_HAVE_ISOLATE1 :: GetReport(CRef <CClickableItem>& c_item)
{
   RmvRedundancy(c_item->item_list); 
   c_item->description = GetHasComment(c_item->item_list.size(), "bacterial biosrouce")
                           + "isolate.";
};


void CSeqEntry_ONCALLER_MULTISRC :: GetReport(CRef <CClickableItem>& c_item)
{
  RmvRedundancy(c_item->item_list); 
  c_item->description = GetHasComment(c_item->item_list.size(), "organism")
                                      + "commma or semicolon in strain or isolate.";
};


void CSeqEntryTestAndRepData :: GetIncnstTestReport (CRef <CClickableItem>& c_item, const string& setting_name, const string& title, const string& item_type)
{ 
  Str2Strs key2ls, field2ls, vlu2ls;
  RmvRedundancy(c_item->item_list);
  GetTestItemList(c_item->item_list, key2ls);
  c_item->item_list.clear();
  
  bool has_missing, any_missing = false, any_inconsistent = false;
  vector <string> missing_ls;
  string key_str, field;
  unsigned cnt;
  CRef <CClickableItem> c_missing (new CClickableItem);
  CRef <CClickableItem> c_field (new CClickableItem);

  ITERATE (Str2Strs, it, key2ls) {
    key_str = it->first + " " + item_type; //" structured comment";
    GetTestItemList(it->second, field2ls, "#");
    has_missing = (field2ls.find("missing") == field2ls.end()) ? false : true;
    if (has_missing) {
        GetTestItemList(field2ls["missing"], vlu2ls, "@");
        missing_ls = vlu2ls.begin()->second;
        c_missing.Reset(CRef <CClickableItem> (new CClickableItem)); 
        c_missing->setting_name = setting_name;
        c_missing->item_list = missing_ls;
        c_missing->description = GetIsComment(missing_ls.size(), "Bioseq") 
              + "missing " + it->first + " " + item_type; //"structured comment";
        c_item->subcategories.insert(c_item->subcategories.begin(), c_missing);
    }
    ITERATE (Str2Strs, fid, field2ls) {
      c_field.Reset(CRef <CClickableItem> (new CClickableItem));
      c_field->setting_name = setting_name;
      if (fid->first != "missing") {
        GetTestItemList(fid->second, vlu2ls, "@");
        if (has_missing) {
           if (vlu2ls.find("missing") != vlu2ls.end())
              missing_ls.insert(missing_ls.end(), 
                            vlu2ls["missing"].begin(), vlu2ls["missing"].end());
           AddSubcategories(c_field, setting_name, missing_ls, key_str,
                 " missing field " + fid->first);
        } 
        if (has_missing || vlu2ls.size() > 1) {
            ITERATE (Str2Strs, vid, vlu2ls) {
              if (vid->first != "missing")
                  AddSubcategories(c_field, setting_name, vid->second, key_str,
                    "field " + fid->first + " value '" + vid->first + "'", e_HasComment);
            }
        }
        else if (!has_missing) {
           AddSubcategories(c_field, setting_name, vlu2ls.begin()->second,
              "All " + key_str, 
              "field " + fid->first + " value '" + vlu2ls.begin()->first + "'", 
              e_HasComment); 
        }
        cnt = c_field->subcategories.size();
        strtmp = key_str + " field " + fid->first;
        if (has_missing) {
          if (cnt == 1) strtmp += "  (all missing)";
          else if (cnt == 2) strtmp += " (some missing, all same)";
          else strtmp += " (some missing, inconsistent)";
        }
        else {
          if (cnt == 1) strtmp += " (all present, all same)";
          else strtmp += " (all present, inconsistent)"; 
        }
        c_field->description = strtmp;
        c_item->subcategories.push_back(c_field);
        if (!any_missing && strtmp.find("missing") != string::npos) any_missing = true;
        if (!any_inconsistent && strtmp.find("inconsistent") != string::npos)
            any_inconsistent = true;
      }
    }    
  }

  c_item->description = title + (any_missing ? " (some missing, " : " (all present, ")
            + (string)(any_inconsistent ? "inconsistent)" : "all same)");
};

void CSeqEntry_on_incnst_user :: TestOnObj(const CSeq_entry& seq_entry)
{
   unsigned i=0;
   CConstRef <CBioseq> seq_ref;
   CConstRef <CBioseq_set> set_ref;
   bool entry_is_seq, not_empty_fields;
   string desc, type_str, prefix, seq_desc;

   ITERATE (vector <const CSeqdesc*>, it, user_seqdesc) {
     desc = GetDiscItemText(**it, *(user_seqdesc_seqentry[i]));
     const CUser_object& user_obj = (*it)->GetUser();
     entry_is_seq = user_seqdesc_seqentry[i]->IsSeq() ? true : false;

     if (entry_is_seq) {
         seq_ref = CConstRef <CBioseq> (&(user_seqdesc_seqentry[i]->GetSeq()));
         seq_desc = GetDiscItemText(*seq_ref);
     }
     else set_ref = CConstRef <CBioseq_set> (&(user_seqdesc_seqentry[i]->GetSet()));
    
     type_str = user_obj.GetType().IsStr() ? user_obj.GetType().GetStr() : kEmptyStr;
     
     // DISC_INCONSISTENT_STRUCTURED_COMMENTS
     not_empty_fields = false;
     if (type_str == "StructuredComment") {
        prefix 
           = user_obj.HasField("StructuredCommentPrefix") ?
           (user_obj.GetField("StructuredCommentPrefix").GetData().IsStr()?
                   user_obj.GetField("StructuredCommentPrefix").GetData().GetStr()
                   : "unnamed")
             : "unnamed";
        if (m_prefix_set.find(prefix) == m_prefix_set.end()) m_prefix_set.insert(prefix);
        if (entry_is_seq) {
           if (!seq_ref->IsAa()) {
              m_seq2prefix[seq_desc] += ("$" + prefix);
              not_empty_fields = true;
           }
        }
        else not_empty_fields = SetHasStrCommFields(*set_ref, prefix);
        if (not_empty_fields) AddStrCommFieldValues(user_obj, prefix, desc);
        else AddStrCommFieldEmptyValues(user_obj, prefix, desc);
     } 
     else if (type_str == "DBLink") { // DISC_INCONSISTENT_DBLINK
        if (entry_is_seq) {
           if (!seq_ref->IsAa()) {
               m_seq_has_dblink.insert(seq_desc);
               not_empty_fields = true;
           }
        } 
        else not_empty_fields = SetHasDbLinkFields(*set_ref);
        if (not_empty_fields) AddDbLinkFieldValues(user_obj, desc);
        else AddDbLinkFieldEmptyValues(user_obj, desc);
     }

     i++;
   }

   // Look for bioseqs missing keys
   if (seq_entry.IsSeq()) CollectSeqMissingKeys(seq_entry.GetSeq());
   else FindSeqsMissingKeys(seq_entry.GetSet());

   // DISC_INCONSISTENT_STRUCTURED_COMMENTS
   ITERATE (set <string>, pit, m_prefix_set) {
     ITERATE (Str2Str, bit, m_seq2prefix) {
       if ( bit->second.find(*pit) == string::npos )
          thisInfo.test_item_list[GetName_comm()].push_back(
                     *pit + "$missing#empty@" + bit->first);                
     } 
   }   
   m_seq2prefix.clear();
   m_prefix_set.clear();
   m_seq_has_dblink.clear();
   m_seq_no_dblink.clear();

   // DISC_INCONSISTENT_DBLink
   ITERATE (set <string>, it, m_seq_no_dblink)
     thisInfo.test_item_list[GetName_db()].push_back("DBLink$missing$empty@" + *it);
   
};

bool CSeqEntry_on_incnst_user :: SetHasStrCommFields(const CBioseq_set& set, const string& prefix)
{
   bool rval = false;
   ITERATE (list <CRef <CSeq_entry> >, it, set.GetSeq_set()) {
     if ( (*it)->IsSeq() ) {
        if (!(*it)->GetSeq().IsAa()) {
            m_seq2prefix[GetDiscItemText( (*it)->GetSeq())] += ("$" + prefix);
            rval = true;
        }
     }
     else if (SetHasStrCommFields( (*it)->GetSet(), prefix )) rval = true;
   }
   return rval;
};

void CSeqEntry_on_incnst_user :: AddStrCommFieldValues(const CUser_object& user_obj, const string& prefix, const string& desc)
{
    string field_str;
    ITERATE (vector <CRef <CUser_field> >, uit, user_obj.GetData()) {
       if (!(*uit)->GetLabel().IsStr()) continue;
       field_str = (*uit)->GetLabel().GetStr();
       if (NStr::EqualNocase(field_str, "StructuredCommentPrefix")
                     || NStr::EqualNocase(field_str, "StructuredCommentPrefix"))
            continue;
       strtmp = prefix + "$" + field_str + "#";
       if ( (*uit)->GetData().IsStr())    // check with Colleen
            strtmp += (*uit)->GetData().GetStr() + "@" + desc;
       else strtmp += ("missing@" + desc);
       thisInfo.test_item_list[GetName_comm()].push_back(strtmp);
    }
};

void CSeqEntry_on_incnst_user :: AddStrCommFieldEmptyValues(const CUser_object& user_obj, const string& prefix, const string& desc)
{
    string field_str;
    ITERATE (vector <CRef <CUser_field> >, uit, user_obj.GetData()) { 
       if (!(*uit)->GetLabel().IsStr()) continue;
       field_str = (*uit)->GetLabel().GetStr();
       if (NStr::EqualNocase(field_str, "StructuredCommentPrefix")
                     || NStr::EqualNocase(field_str, "StructuredCommentPrefix"))
            continue;
       strtmp = prefix + "$" + field_str + "#missing@" + desc;
       thisInfo.test_item_list[GetName_comm()].push_back(strtmp);
    }
};

bool CSeqEntry_on_incnst_user :: SetHasDbLinkFields(const CBioseq_set& set)
{
   bool rval = false;
   ITERATE (list <CRef <CSeq_entry> >, it, set.GetSeq_set()) {
     if ( (*it)->IsSeq() ) {
        if (!(*it)->GetSeq().IsAa()) {
            m_seq_has_dblink.insert(GetDiscItemText( (*it)->GetSeq())); 
            rval = true;
        }
     }
     else if (SetHasDbLinkFields( (*it)->GetSet())) rval = true;
   }
   return rval;
};

void CSeqEntry_on_incnst_user :: AddDbLinkFieldValues(const CUser_object& user_obj, const string& desc)
{
    string field_str;
    ITERATE (vector <CRef <CUser_field> >, uit, user_obj.GetData()) {
       if (!(*uit)->GetLabel().IsStr()) continue;
       field_str = (*uit)->GetLabel().GetStr();
       if (field_str == "Trace Assembly Archive" || field_str == "BioSample"
                 || field_str == "ProbeDB" || field_str == "Sequence Read Archive"
                 || field_str == "BioProject") {
          strtmp = "DBLink$" + field_str + "#";
          if ( (*uit)->GetData().IsStrs()) {
              ITERATE (CUser_field::C_Data::TStrs, sit, (*uit)->GetData().GetStrs()) 
                 thisInfo.test_item_list[GetName_db()].push_back(strtmp+(*sit)+"@"+desc);
          }
          else if ( (*uit)->GetData().IsInts()) {
              ITERATE (vector <int>, iit, (*uit)->GetData().GetInts())
                thisInfo.test_item_list[GetName_db()].push_back(
                   strtmp + NStr::IntToString(*iit) + "@" + desc);
          }
          else 
             thisInfo.test_item_list[GetName_db()].push_back(strtmp + "missing@" + desc);
       }
    }
};

void CSeqEntry_on_incnst_user :: AddDbLinkFieldEmptyValues(const CUser_object& user_obj, const string& desc)
{
    string field_str;
    ITERATE (vector <CRef <CUser_field> >, uit, user_obj.GetData()) {
       if (!(*uit)->GetLabel().IsStr()) continue;
       field_str = (*uit)->GetLabel().GetStr();
       if (field_str == "Trace Assembly Archive" || field_str == "BioSample"
                 || field_str == "ProbeDB" || field_str == "Sequence Read Archive"
                 || field_str == "BioProject") {
          strtmp = "DBLink$" + field_str + "#missing@" + desc;
          thisInfo.test_item_list[GetName_db()].push_back(strtmp);
       }
    }
};

void CSeqEntry_on_incnst_user :: CollectSeqMissingKeys(const CBioseq& bioseq)
{
   if (!bioseq.IsAa()) {
      string  desc = GetDiscItemText(bioseq);
      if (m_seq2prefix.find(desc) == m_seq2prefix.end()) m_seq2prefix[desc] = "";
      if (m_seq_has_dblink.find(desc) == m_seq_has_dblink.end())
            m_seq_no_dblink.insert(desc);     
   }
};
 
void CSeqEntry_on_incnst_user :: FindSeqsMissingKeys(const CBioseq_set& set)
{
   ITERATE (list <CRef <CSeq_entry> >, it, set.GetSeq_set()) {
      if ( (*it)->IsSeq()) CollectSeqMissingKeys((*it)->GetSeq());
      else FindSeqsMissingKeys((*it)->GetSet());
   }
};


void CSeqEntry_test_on_user :: GroupAllBioseqs(const CBioseq_set& bioseq_set, const int& id)
{
  string desc;
  ITERATE (list < CRef < CSeq_entry > >, it, bioseq_set.GetSeq_set()) {
     if ((*it)->IsSeq()) {
         const CBioseq& bioseq = (*it)->GetSeq();
         desc = GetDiscItemText(bioseq);
         if ( bioseq.IsAa() ) {
              PROJECT_ID_prot[id].push_back(desc);
              thisInfo.test_item_list[GetName_proj()].push_back("prot$" + desc);
         }
         else {
              PROJECT_ID_nuc[id].push_back(desc);
              thisInfo.test_item_list[GetName_proj()].push_back("nuc$" + desc);
         }
     }
     else GroupAllBioseqs((*it)->GetSet(), id);
  }
};


void CSeqEntry_test_on_user :: CheckCommentCountForSet(const CBioseq_set& set, const unsigned& cnt, Str2Int& bioseq2cnt)
{
   string desc;
   ITERATE (list < CRef < CSeq_entry > >, it, set.GetSeq_set()) {
     if ((*it)->IsSeq()) {
          const CBioseq& bioseq = (*it)->GetSeq();
          if (!bioseq.IsAa()) {
              desc =  GetDiscItemText( (*it)->GetSeq());
              if (bioseq2cnt.find(desc) != bioseq2cnt.end()) {
                   if (cnt) 
                       bioseq2cnt[desc] = bioseq2cnt[desc] ? bioseq2cnt[desc]++ : cnt;
              }
              else bioseq2cnt[desc] = cnt;
          }
     }
     else CheckCommentCountForSet((*it)->GetSet(), cnt, bioseq2cnt);
   }
};


void CSeqEntry_test_on_user :: AddBioseqsOfSet2Map(const CBioseq_set& bioseq_set)
{
   ITERATE (list < CRef < CSeq_entry > >, it, bioseq_set.GetSeq_set()) {
     if ((*it)->IsSeq()) {
          if ((*it)->GetSeq().IsNa())
            m_bioseq2geno_comm[GetDiscItemText((*it)->GetSeq())] = 1;
     }
     else AddBioseqsOfSet2Map((*it)->GetSet());
   }
};


const CBioseq& CSeqEntry_test_on_user :: Get1stBioseqOfSet(const CBioseq_set& bioseq_set)
{
   const list < CRef < CSeq_entry > >& seq_entrys = bioseq_set.GetSeq_set();
   if ( (*(seq_entrys.begin()))->IsSeq()) {
       if ( (*(seq_entrys.begin()))->GetSeq().IsNa() )
           return ((*(seq_entrys.begin()))->GetSeq());
   }
   return (Get1stBioseqOfSet((*(seq_entrys.begin()))->GetSet()));
};


void CSeqEntry_test_on_user :: RmvBioseqsOfSetOutMap(const CBioseq_set& bioseq_set)
{
   const CBioseq& bioseq = Get1stBioseqOfSet(bioseq_set);
   string desc = GetDiscItemText(bioseq);
   if (m_bioseq2geno_comm.find(desc) != m_bioseq2geno_comm.end()
               && !m_bioseq2geno_comm[desc]) 
      return;
   ITERATE (list < CRef < CSeq_entry > >, it, bioseq_set.GetSeq_set()) {
     if ((*it)->IsSeq()) {
        if ((*it)->GetSeq().IsNa()) {
           desc = GetDiscItemText( (*it)->GetSeq() ); 
           if (m_bioseq2geno_comm.find(desc) != m_bioseq2geno_comm.end())
              m_bioseq2geno_comm[desc] = 0;
        }
     }
     else RmvBioseqsOfSetOutMap( (*it)->GetSet() );
   }
};


void CSeqEntry_test_on_user :: TestOnObj(const CSeq_entry& seq_entry)
{
  if (thisTest.is_DESC_user_run) return;

  CValidError errors;
  CValidError_imp imp(thisInfo.scope->GetObjectManager(),&errors);
  CValidError_desc validator(imp);

  unsigned i=0, cnt;
  int id;
  string bioseq_desc, type_str, user_desc;
  Str2Int bioseq2cnt;

  if (seq_entry.IsSeq()) {
     if (seq_entry.GetSeq().IsNa())
         m_bioseq2geno_comm[GetDiscItemText(seq_entry.GetSeq())] = 1;
  }
  else AddBioseqsOfSet2Map(seq_entry.GetSet());

  bioseq2cnt.clear();
  bool entry_is_seq;
  CConstRef <CBioseq> seq_ref;
  CConstRef <CBioseq_set> set_ref;

  ITERATE (vector <const CSeqdesc*>, it, user_seqdesc) {
    const CUser_object& user_obj = (*it)->GetUser();
    if ( !(user_obj.GetType().IsStr()) ) { 
         i++; continue;
    }
    user_desc = GetDiscItemText(**it, *(user_seqdesc_seqentry[i]));

    entry_is_seq = false;
    if (user_seqdesc_seqentry[i]->IsSeq()) {
          seq_ref = CConstRef <CBioseq> (&(user_seqdesc_seqentry[i]->GetSeq()));
          bioseq_desc = GetDiscItemText(*seq_ref);
          entry_is_seq = true;
    }
    else set_ref = CConstRef <CBioseq_set> (&(user_seqdesc_seqentry[i]->GetSet()));
    type_str = user_obj.GetType().GetStr();

    // MISSING_GENOMEASSEMBLY_COMMENTS
    if (type_str == "StructuredComment"
          && user_obj.HasField("StructuredCommentPrefix")
          && user_obj.GetField("StructuredCommentPrefix").GetData().IsStr()
          && user_obj.GetField("StructuredCommentPrefix").GetData().GetStr()
                      == "##Genome-Assembly-Data-START##") {
            // has comment, rm bioseqs from map
            if (entry_is_seq) {
                if (seq_ref->IsNa()
                      && m_bioseq2geno_comm.find(bioseq_desc)== m_bioseq2geno_comm.end())
                m_bioseq2geno_comm[bioseq_desc] = 0;
            }
            else RmvBioseqsOfSetOutMap(*set_ref);
    }

    // TEST_HAS_PROJECT_ID
    if (type_str == "GenomeProjectsDB" && user_obj.HasField("ProjectID")
         && user_obj.GetField("ProjectID").GetData().IsInt()) {
      id = user_obj.GetField("ProjectID").GetData().GetInt();
      if (entry_is_seq) {
          if ( seq_ref->IsAa() ) {
              PROJECT_ID_prot[id].push_back(bioseq_desc);
              thisInfo.test_item_list[GetName_proj()].push_back("prot$" + bioseq_desc);
          }
          else {
              PROJECT_ID_nuc[id].push_back(bioseq_desc);
              thisInfo.test_item_list[GetName_proj()].push_back("nuc$" + bioseq_desc);
          }
      }
      else GroupAllBioseqs(*set_ref, id);
    }

    // ONCALLER_MISSING_STRUCTURED_COMMENTS, MISSING_STRUCTURED_COMMENT
    if (type_str == "StructuredComment") cnt = 1;
    else cnt = 0;
    if (entry_is_seq) {
      if (!seq_ref->IsAa()) {
         if (bioseq2cnt.find(bioseq_desc) != bioseq2cnt.end()) {
              if (cnt) bioseq2cnt[bioseq_desc] 
                         = bioseq2cnt[bioseq_desc] ? bioseq2cnt[bioseq_desc]++ : cnt;
         }
         else bioseq2cnt[bioseq_desc] = cnt;
      }
    }
    else CheckCommentCountForSet(*set_ref, cnt, bioseq2cnt);

    // MISSING_PROJECT
    if (type_str != "GenomeProjectsDB"
            && (type_str != "DBLink" || !user_obj.HasField("BioProject")) ) 
       AddBioseqsInSeqentryToReport(user_seqdesc_seqentry[i], GetName_mproj());

    // ONCALLER_BIOPROJECT_ID
    if (type_str == "DBLink" && user_obj.HasField("BioProject")
         && user_obj.GetField("BioProject").GetData().IsStrs()) {
       const CUser_field::C_Data::TStrs& ids =user_obj.GetField("BioProject").GetData().GetStrs();
       if (!ids.empty() && !ids[0].empty()) {
         if (entry_is_seq) 
              thisInfo.test_item_list[GetName_bproj()].push_back(bioseq_desc);
         else 
            AddBioseqsOfSetToReport(*set_ref, GetName_bproj());
       }
    }

    // ONCALLER_SWITCH_STRUCTURED_COMMENT_PREFIX
    if (validator.ValidateStructuredComment(user_obj, **it))
       thisInfo.test_item_list[GetName_prefix()].push_back(user_desc); 

    i++;
  }

// MISSING_GENOMEASSEMBLY_COMMENTS
   ITERATE (Str2Int, it, m_bioseq2geno_comm) {
      if (it->second)
         thisInfo.test_item_list[GetName_gcomm()].push_back(it->first);
   }

// ONCALLER_MISSING_STRUCTURED_COMMENTS, MISSING_STRUCTURED_COMMENT
  ITERATE (Str2Int, it, bioseq2cnt) {
    thisInfo.test_item_list[GetName_oncall_scomm()].push_back(
                                NStr::IntToString(it->second) + "$" + it->first);
    if (!it->second)
      thisInfo.test_item_list[GetName_scomm()].push_back(it->first);
  }

  thisTest.is_DESC_user_run = true;
};

void CSeqEntry_ONCALLER_SWITCH_STRUCTURED_COMMENT_PREFIX :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetIsComment(c_item->item_list.size(), "structured comment")
                       + "invalid but would be valid with a different prefix.";
};

void CSeqEntry_MISSING_STRUCTURED_COMMENT :: GetReport(CRef <CClickableItem>& c_item)
{
  RmvRedundancy(c_item->item_list);   // all CSeqEntry_Feat_desc tests need this.

  c_item->description = GetDoesComment(c_item->item_list.size(), "sequence") 
                           + "not include structured comments.";
};

void CSeqEntry_ONCALLER_BIOPROJECT_ID :: GetReport(CRef <CClickableItem>& c_item)
{
  RmvRedundancy(c_item->item_list);   // all CSeqEntry_Feat_desc tests need this.

  c_item->description 
        = GetContainsComment(c_item->item_list.size(), "sequence") + "BioProject IDs.";
};

void CSeqEntry_MISSING_PROJECT :: GetReport(CRef <CClickableItem>& c_item)
{
  RmvRedundancy(c_item->item_list);   // all CSeqEntry_Feat_desc tests need this.

  c_item->description
    = GetDoesComment(c_item->item_list.size(), "sequence") + " not include project.";
};

void CSeqEntry_ONCALLER_MISSING_STRUCTURED_COMMENTS :: GetReport(CRef <CClickableItem>& c_item)
{
  RmvRedundancy(c_item->item_list);   // all CSeqEntry_Feat_desc tests need this.

  Str2Strs bioseq2cnt;
  GetTestItemList(c_item->item_list, bioseq2cnt); //bioseq2cnt Globle in 1 or multi file?
  c_item->item_list.clear();
  string desc;
  if (bioseq2cnt.size() > 1) {
    ITERATE (Str2Strs, it, bioseq2cnt) {
       desc =
         (it->first=="1"||it->first=="0")? " structured comment":" structured comments";
       AddSubcategories(c_item,  GetName(), it->second, "sequence", it->first + desc,
                          e_HasComment, false);
    }
    c_item->description = "Sequences have different numbers of structured comments.";
  }
};

void CSeqEntry_MISSING_GENOMEASSEMBLY_COMMENTS :: GetReport(CRef <CClickableItem>& c_item)
{
  RmvRedundancy(c_item->item_list);   // all CSeqEntry_Feat_desc tests need this.

  c_item->description = GetIsComment(c_item->item_list.size(), "bioseq")
                            + "missing GenomeAssembly structured comments.";
};


void CSeqEntry_TEST_HAS_PROJECT_ID :: GetReport(CRef <CClickableItem>& c_item)
{
   RmvRedundancy(c_item->item_list);   // all CSeqEntry_Feat_desc tests need this.

   unsigned prot_cnt = 0, nuc_cnt = 0, prot_id1 = 0, nuc_id1 = 0;
   Str2Strs mol2list;
   GetTestItemList(c_item->item_list, mol2list);
   c_item->item_list.clear();
   ITERATE (Str2Strs, it, mol2list) {
     if (it->first == "prot") prot_cnt = it->second.size();
     else nuc_cnt = it->second.size();
   }

   string tot_add_desc, prot_add_desc, nuc_add_desc;
   if (PROJECT_ID_prot.size() <= 1 && PROJECT_ID_nuc.size() <= 1) {
      if (prot_cnt)  prot_id1 = PROJECT_ID_prot.begin()->first;
      if (nuc_cnt) nuc_id1 = PROJECT_ID_nuc.begin()->first;
      if ( (prot_id1 == nuc_id1) ||  !(prot_id1 * nuc_id1)) tot_add_desc = "(all same)";
      else tot_add_desc = " (some different)";
   }

   if (prot_cnt && PROJECT_ID_prot.size() > 1) prot_add_desc = "(some different)";
   else prot_add_desc = "(all same)";

   if (nuc_cnt && PROJECT_ID_nuc.size() > 1) nuc_add_desc = "(some different)";
   else nuc_add_desc = "(all same)";

   if (prot_cnt && !nuc_cnt) {
       c_item->item_list = mol2list.begin()->second;
       c_item->description = GetOtherComment(c_item->item_list.size(),
                                 "protein sequence has project ID ",
                                 "protein sequences have project IDs " + tot_add_desc);
   }
   else if (!prot_cnt && nuc_cnt) {
       c_item->item_list = mol2list.begin()->second;
       c_item->description = GetOtherComment(c_item->item_list.size(),
                                "nucleotide sequence has project ID ",
                                "nucleotide sequences have project IDs " + tot_add_desc);
   }
   else {
       c_item->description 
               = GetOtherComment(prot_cnt + nuc_cnt, "sequence has project ID ",
                                "sequencs have project IDs " + tot_add_desc);
       AddSubcategories(c_item, GetName(), mol2list["nuc"],
           "nucleotide sequence has project ID", "nucleotide sequences have project IDs",
           e_OtherComment, false);
       AddSubcategories(c_item, GetName(), mol2list["prot"], "protein sequence has project ID",
                       "protein sequences have project IDs", e_OtherComment, false);

       //c_item->expanded = true;  what does the expanded work for?
   }

   PROJECT_ID_prot.clear();
   PROJECT_ID_nuc.clear();
};

// new method


CFlatFileConfig::CGenbankBlockCallback::EBioseqSkip CFlatfileTextFind::notify_bioseq(const CBioseqContext& ctx )
{
   m_taxname = ctx.GetTaxname();
// wait for new build:   CConstRef <CBioseq> m_bioseq_ref = ctx.GetHandle().GetCompleteObject();
   m_bioseq_desc = "!!!"; //GetDiscItemText(*m_bioseq_ref);

   m_src_desc = m_mol_desc = m_tlt_desc = m_pub_desc = m_gbk_desc = kEmptyStr;
/*
   for (CSeqdesc_CI it(ctx.GetHandle(), m_seqdesc_sel); it; ++it) {
     switch (it->Which()) {
        case CSeqdesc::e_Source:
            m_src_desc = GetDiscItemText(it->GetSource(), *m_bioseq_ref);
            break;
        case CSeqdesc::e_Molinfo:
            m_mol_desc = GetDiscItemText(it->GetMolinfo(), *m_bioseq_ref);
            break;
        case CSeqdesc::e_Title:
            m_tlt_desc = GetDiscItemText(it->GetTitle(), *m_bioseq_ref);
            break;
        case CSeqdesc::e_Pub:
            m_pub_desc = GetDiscItemText(it->GetPub(), *m_bioseq_ref);
            break;
        case CSeqdesc::e_Genbank:
            m_gbk_desc = GetDiscItemText(it->GetGenbank(), *m_bioseq_ref);
            break;
        default: break;
     }

   }

*/
   return (eBioseqSkip_No); 
};

CFlatFileConfig::CGenbankBlockCallback::EAction   CFlatfileTextFind::notify(string& block_text, const CBioseqContext& ctx, const CSourceItem& source_item)
{
  block_text = block_text.substr(0, block_text.find("ORGANISM")); // double check  
  return unified_notify(block_text, ctx, source_item, 
                                          CFlatFileConfig::fGenbankBlocks_Source);
};

CFlatFileConfig::CGenbankBlockCallback::EAction CFlatfileTextFind::notify(string& block_text, const CBioseqContext& ctx, const CFeatureItem& feature_item)
{
   size_t pos = block_text.find("/translation=\"");
   string trans_str;
   if (pos != string::npos) {
     strtmp = block_text.substr(pos); 
     block_text = block_text.substr(0, pos-1);
     pos = strtmp.substr(14).find("\"");
     if (pos != string::npos) {
       if (pos < strtmp.size() - 1) block_text += strtmp.substr(pos+1);
     }
   }
   return unified_notify(block_text, ctx, feature_item, 
                                            CFlatFileConfig::fGenbankBlocks_FeatAndGap);
};


CFlatFileConfig::CGenbankBlockCallback::EAction CFlatfileTextFind::unified_notify( string & block_text, const CBioseqContext& ctx, const IFlatItem & flat_item, CFlatFileConfig::FGenbankBlocks which_block )
{
  block_text =CTestAndRepData::FindReplaceString(block_text, m_taxname, "", false, true);
  ITERATE (Str2UInt, it, thisInfo.spell_data) {
     if (CTestAndRepData 
           :: DoesStringContainPhrase(block_text, it->first, false,  (bool)it->second)) {
       switch (which_block) {
         case CFlatFileConfig::fGenbankBlocks_Locus: strtmp = m_mol_desc; break;
         case CFlatFileConfig::fGenbankBlocks_Defline: strtmp = m_tlt_desc; break;
         case CFlatFileConfig::fGenbankBlocks_Keywords: strtmp = m_gbk_desc; break;
         case CFlatFileConfig::fGenbankBlocks_Reference: strtmp = m_pub_desc; break;
         case CFlatFileConfig::fGenbankBlocks_Source: strtmp = m_src_desc; break;
         case CFlatFileConfig::fGenbankBlocks_Sourcefeat: 
            {
              const CFeatureItemBase& feat_item 
                             = dynamic_cast<const CFeatureItemBase&> (flat_item);
              strtmp = GetDiscItemText(
                                             feat_item.GetFeat().GetOriginalFeature());
            }
            break;
         case CFlatFileConfig::fGenbankBlocks_FeatAndGap:
            {
              const CFeatureItemBase& feat_item 
                             = dynamic_cast<const CFeatureItemBase&> (flat_item);
              strtmp = GetDiscItemText(
                                             feat_item.GetFeat().GetOriginalFeature());
            }
            break;
         default: strtmp = m_bioseq_desc;
       };
       thisInfo.test_item_list[m_setting_name].push_back(
            thisInfo.fix_data[it->first] + "$" + it->first + "#" + strtmp);
     }
  } 

  return eAction_Skip;  // flat file gen. won't print out blocks
};

void CSeqEntry_DISC_FLATFILE_FIND_ONCALLER :: TestOnObj(const CSeq_entry& seq_entry)
{
   CFlatfileTextFind flatfile_find(GetName());
   unsigned blocks = CFlatFileConfig::fGenbankBlocks_All 
                             & ~CFlatFileConfig::fGenbankBlocks_Sequence;
   CFlatFileConfig cfg(CFlatFileConfig::eFormat_GenBank,
                       CFlatFileConfig::eMode_GBench,
                       CFlatFileConfig::eStyle_Normal,
                       CFlatFileConfig::fShowContigFeatures,
                       CFlatFileConfig::fViewAll,
                       CFlatFileConfig::fGffGTFCompat,
                       (CFlatFileConfig::FGenbankBlocks)blocks,
                       &flatfile_find);
   CFlatFileGenerator gen(cfg);
   CSeq_entry_Handle entry_hl = thisInfo.scope->GetSeq_entryHandle(seq_entry);
   gen.Generate( entry_hl, cout);
   
cerr << "block_text " << flatfile_find.m_block_text << endl;
};

void CSeqEntry_DISC_FLATFILE_FIND_ONCALLER :: AddCItemToReport(const string& ls_type, const string& setting_name, CRef <CClickableItem>& c_item)
{
   Str2Strs wrds2ls;
   ITERATE (Str2Strs, it, m_fixable2ls) {
     if (it->first == ls_type) {
        wrds2ls.clear();
        GetTestItemList(it->second, wrds2ls);
        ITERATE (Str2Strs, jt, wrds2ls) {
           if (m_citem1) m_citem1 = false;
           else {
              c_item.Reset(new CClickableItem);
              c_item->setting_name = setting_name;
              thisInfo.disc_report_data.push_back(c_item);
           }
           c_item->item_list = jt->second;
           c_item->description
                   = GetContainsComment(c_item->item_list.size(), "object") + jt->first;
        }
     }
   }
   
};

void CSeqEntry_DISC_FLATFILE_FIND_ONCALLER :: GetReport(CRef <CClickableItem>& c_item)
{
   m_fixable2ls.clear();
   GetTestItemList(c_item->item_list, m_fixable2ls); 
   c_item->item_list.clear();
   m_citem1 = true;
   AddCItemToReport("no", GetName_nofix(), c_item);
   AddCItemToReport("yes", GetName_fix(), c_item); 
};

bool CSeqEntry_ONCALLER_STRAIN_TAXNAME_CONFLICT :: StrainConflictsTaxname(const COrg_ref& org)
{
   vector <string> arr;
   GetOrgModValues(org, COrgMod::eSubtype_other, arr);
   unsigned len;
   string tax_nm = org.GetTaxname();
   if (!arr.empty()) {
      ITERATE (vector <string>, iit, arr) {   
        ITERATE (vector <string>, it, thisInfo.strain_tax) {
          len = (*it).size();
          if (NStr::EqualNocase((*iit).substr(0, len), *it)) {
             strtmp = NStr::TruncateSpaces( (*iit).substr(len) );
             if (strtmp != tax_nm) return true;
             break;
          }
        }
      }
   }

   return false;
};


void CSeqEntry_ONCALLER_STRAIN_TAXNAME_CONFLICT :: TestOnObj(const CSeq_entry& seq_entry)
{
   bool has_tax;
   ITERATE (vector <const CSeq_feat*>, it, biosrc_orgmod_feat) {
     const CBioSource& biosrc = (*it)->GetData().GetBiosrc();
     has_tax = (biosrc.IsSetTaxname() && !biosrc.GetTaxname().empty()) ? true : false;
     if (has_tax && StrainConflictsTaxname(biosrc.GetOrg()))
        thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
   }

   ITERATE (vector <const CSeq_feat*>, it, org_orgmod_feat) {
      const COrg_ref& org = (*it)->GetData().GetOrg();
      has_tax = (org.IsSetTaxname() && !org.GetTaxname().empty()) ? true : false;
      if (has_tax && StrainConflictsTaxname(org))
        thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
   }

   unsigned i=0;
   ITERATE (vector <const CSeqdesc*>, it, biosrc_orgmod_seqdesc) {
     const CBioSource& biosrc = (*it)->GetSource();
     has_tax = (biosrc.IsSetTaxname() && !biosrc.GetTaxname().empty()) ? true : false;
     if ( has_tax && StrainConflictsTaxname(biosrc.GetOrg()))
         thisInfo.test_item_list[GetName()].push_back(
                             GetDiscItemText(**it, *(biosrc_orgmod_seqdesc_seqentry[i])));
     i++;
   };
   
   i=0;
   ITERATE (vector <const CSeqdesc*>, it, org_orgmod_seqdesc) {
     const COrg_ref& org = (*it)->GetOrg();
     has_tax = (org.IsSetTaxname() && !org.GetTaxname().empty()) ? true : false;
     if (StrainConflictsTaxname( org ))
         thisInfo.test_item_list[GetName()].push_back(
                             GetDiscItemText(**it, *(org_orgmod_seqdesc_seqentry[i])));
     i++;
   }
};

void CSeqEntry_ONCALLER_STRAIN_TAXNAME_CONFLICT :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "sequence") 
                           + "conflicts between type strain and organism name.";
};

bool CSeqEntry_TEST_SMALL_GENOME_SET_PROBLEM :: HasSmallSeqset(const CSeq_entry& seq_entry)
{
   if (seq_entry.IsSet()) {
      if (seq_entry.GetSet().GetClass() == CBioseq_set::eClass_small_genome_set)
          return true;
      else {
         ITERATE (list <CRef <CSeq_entry> >, it, seq_entry.GetSet().GetSeq_set()) 
            if (HasSmallSeqset(**it)) return true;
      }
   } 

   return false;
};

void CSeqEntry_TEST_SMALL_GENOME_SET_PROBLEM :: TestOnObj(const CSeq_entry& seq_entry)
{
   if (!HasSmallSeqset(seq_entry)) return;
   string pre_taxname, pre_isolate, pre_strain;
   pre_taxname = pre_isolate = pre_strain = kEmptyStr;
   bool all_taxnames_same, all_isolates_same, all_strains_same;
   all_taxnames_same = all_isolates_same = all_strains_same = true;
   unsigned i=0;
   string desc, desc_entry(GetDiscItemText(seq_entry));
   vector <string> arr;
   ITERATE (vector <const CSeqdesc*>, it, biosrc_seqdesc) {
      const CBioSource& biosrc = (*it)->GetSource();
      desc = GetDiscItemText(**it, *biosrc_seqdesc_seqentry[i]);
      if (HasLineage(biosrc, "Viruses")) {
         if (!IsSubSrcPresent(biosrc, CSubSource::eSubtype_segment))
           thisInfo.test_item_list[GetName()].push_back("missing$" + desc);
      }
      if (all_taxnames_same) {
         desc = GetSrcQualValue(biosrc, "taxname");
         if (pre_taxname.empty() && !desc.empty()) pre_taxname = desc;
         else if (desc != pre_taxname) all_taxnames_same = false;  
      }
      if (all_isolates_same) {
         arr.clear();
         GetOrgModValues(biosrc, COrgMod::eSubtype_isolate, arr);
         all_isolates_same = AllVecElesSame(arr);
         if (all_isolates_same) {
           if (pre_isolate.empty() && !arr[0].empty()) pre_isolate = arr[0];
           else if (arr[0] != pre_isolate) all_isolates_same = false;
         }
      }
      if (all_strains_same) {
         arr.clear();
         GetOrgModValues(biosrc, COrgMod::eSubtype_strain, arr);
         all_strains_same = AllVecElesSame(arr);
         if (all_strains_same) {
            if (pre_strain.empty() && !arr[0].empty()) pre_strain = arr[0];
            else if (arr[0] != pre_strain) all_strains_same = false;
         }
      }
   }
 
   if (!all_taxnames_same) strtmp = "taxname$" + desc_entry;
   if (!all_isolates_same) strtmp = "isolate$" + desc_entry;
   if (!all_strains_same) strtmp = "strain$" + desc_entry;
   thisInfo.test_item_list[GetName()].push_back(strtmp);
};

void CSeqEntry_TEST_SMALL_GENOME_SET_PROBLEM :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs qual2ls;
   GetTestItemList(c_item->item_list, qual2ls);
   c_item->item_list.size();
   unsigned cnt;
   ITERATE (Str2Strs, it, qual2ls) {
     if (it != qual2ls.begin() ) { 
             c_item = CRef <CClickableItem> (new CClickableItem);
             c_item->setting_name = GetName();
             thisInfo.disc_report_data.push_back(c_item);
     } 
     c_item->item_list = it->second;
     if (it->first == "missing") {
        cnt = it->second.size(); 
        c_item->description = GetOtherComment(cnt, "biosourc", "biosource")
          + " should have segment qualifier but " + ( cnt>1? "do not" : "does not"); 
     }
     else c_item->description = "Not all biosources have same " + it->first;
   }
};


bool CSeqEntry_DISC_SUBMITBLOCK_CONFLICT :: CitSubMatchExceptDate(const CCit_sub& cit1, const CCit_sub& cit2)
{
   if (Blob2Str(cit1.GetAuthors()) != Blob2Str(cit2.GetAuthors())) return false; 
   if (cit1.CanGetImp() != cit2.CanGetImp()) return false;
   else if (cit1.CanGetImp() && Blob2Str(cit1.GetImp()) != Blob2Str(cit2.GetImp())) 
         return false;
   string desc1 = cit1.CanGetDescr()? cit1.GetDescr() : kEmptyStr;
   string desc2 = cit2.CanGetDescr()? cit2.GetDescr() : kEmptyStr;
   if (desc1 != desc2) return false;
   int med1 = cit1.CanGetMedium()? cit2.GetMedium() : 0;
   int med2 = cit2.CanGetMedium() ? cit2.GetMedium() : 0; 
   if (med1 != med2) return false;
   return true; 
};


bool CSeqEntry_DISC_SUBMITBLOCK_CONFLICT :: DateMatch(const CSubmit_block& blk1, const CSubmit_block& blk2) 
{
   if (blk1.CanGetReldate() != blk2.CanGetReldate()) return false;
   else if (blk1.CanGetReldate()
            && CDate::eCompare_same != blk1.GetReldate().Compare(blk2.GetReldate()))
      return false;

   return true;
};


static vector <string> SUBMIT_BLKs;
string CSeqEntry_DISC_SUBMITBLOCK_CONFLICT :: SubmitBlockMatchExceptDate(const CSubmit_block& this_blk)
{
   CSubmit_block blk_in_ls;
   bool match = true;
   string ctat, this_ctat, str1, str2;
   this_ctat = Blob2Str(this_blk.GetContact());
   int subtp1, subtp2;
   unsigned i=0;
   ITERATE (vector <string>, it, SUBMIT_BLKs) {
      blk_in_ls.Reset();
      Str2Blob(*it, blk_in_ls);
      ctat = Blob2Str(blk_in_ls.GetContact());
      if (this_ctat != ctat) match = false;
      else if (!CitSubMatchExceptDate(this_blk.GetCit(), blk_in_ls.GetCit())) match = false;
      else if ( this_blk.GetHup() != blk_in_ls.GetHup()) match = false;
      else if ( this_blk.GetHup() && !DateMatch(this_blk, blk_in_ls)) match = false;
      else {
         subtp1 = blk_in_ls.CanGetSubtype() ? blk_in_ls.GetSubtype() : 0;
         subtp2 = this_blk.CanGetSubtype() ? this_blk.GetSubtype() : 0;
         if (subtp1 != subtp2) match = false;
         else {
            str1 = blk_in_ls.CanGetTool() ? blk_in_ls.GetTool() : kEmptyStr;
            str2 = this_blk.CanGetTool() ? this_blk.GetTool() : kEmptyStr;
            if (str1 != str2) match = false;
            else {
              str1 = blk_in_ls.CanGetUser_tag() ? blk_in_ls.GetUser_tag() : kEmptyStr;
              str2 = this_blk.CanGetUser_tag() ? this_blk.GetUser_tag() : kEmptyStr;
              if (str1 != str2) match = false;
              else {
                 str1 = blk_in_ls.CanGetComment() ? blk_in_ls.GetComment() : kEmptyStr;
                 str2 = this_blk.CanGetComment() ? this_blk.GetComment() : kEmptyStr;
                 if (str1 != str2) match = false;
              }
            } 
         }
      }

      if (match) return (NStr::UIntToString(i) + "$");
      i++;
      match = true;
   }
   SUBMIT_BLKs.push_back(Blob2Str(this_blk));
   return (NStr::UIntToString((unsigned)SUBMIT_BLKs.size()) + "$");
};



void CSeqEntry_DISC_SUBMITBLOCK_CONFLICT :: TestOnObj(const CSeq_entry& seq_entry)
{
   string desc;
   if (thisInfo.seq_submit.Empty()) desc = "0$";
   else {
     if (SUBMIT_BLKs.empty()) SUBMIT_BLKs.push_back(Blob2Str(thisInfo.seq_submit->GetSub()));
     else desc = SubmitBlockMatchExceptDate(thisInfo.seq_submit->GetSub());
   }
   if (seq_entry.IsSeq()) desc += GetDiscItemText(seq_entry.GetSeq());
   else desc += GetDiscItemText(seq_entry.GetSet());
   thisInfo.test_item_list[GetName()].push_back(desc);
};


void CSeqEntry_DISC_SUBMITBLOCK_CONFLICT :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs blk2ords;
   GetTestItemList(c_item->item_list, blk2ords);
   c_item->item_list.clear();
   if (blk2ords.size() > 1) {
      ITERATE (Str2Strs, it, blk2ords) {
        strtmp = (it->first == "0") ? "no submit-block" : "identical submit-blocks";
        AddSubcategories(c_item, GetName(), it->second, "record", strtmp, e_HasComment, false);
      }
      c_item->description = "SubmitBlock Conflicts";
   }
};


void CSeqEntry_DISC_INCONSISTENT_MOLTYPES :: AddMolinfoToBioseqsOfSet(const CBioseq_set& set, const string& desc)
{
   const CEnumeratedTypeValues* mol_vlu = CSeq_inst::GetTypeInfo_enum_EMol(); 
   ITERATE (list < CRef < CSeq_entry > >, it, set.GetSeq_set()) {
     if ((*it)->IsSeq()) {
         const CBioseq& bioseq = (*it)->GetSeq();
         if ( !bioseq.IsAa() ) {
              thisInfo.test_item_list[GetName()].push_back( 
                         desc + mol_vlu->FindName(bioseq.GetInst().GetMol(), false) + "#" 
                         + GetDiscItemText(bioseq));
         }
     }
     else AddMolinfoToBioseqsOfSet((*it)->GetSet(), desc);
  }
};



void CSeqEntry_DISC_INCONSISTENT_MOLTYPES :: TestOnObj(const CSeq_entry& seq_entry)
{
   string desc;
   const CEnumeratedTypeValues* biomol_vlu = CMolInfo::GetTypeInfo_enum_EBiomol();
   const CEnumeratedTypeValues* mol_vlu = CSeq_inst::GetTypeInfo_enum_EMol(); 
   unsigned i=0;
   ITERATE (vector <const CSeqdesc*>, it, molinfo_seqdesc) {
      desc = NStr::UIntToString(m_entry_no) + "$" 
              + biomol_vlu->FindName((*it)->GetMolinfo().GetBiomol(), false) + " ";
      if ( molinfo_seqdesc_seqentry[i]->IsSeq()) {
        const CBioseq& bioseq = molinfo_seqdesc_seqentry[i]->GetSeq();
        if (!bioseq.IsAa()) {
              thisInfo.test_item_list[GetName()].push_back( 
                         desc + mol_vlu->FindName(bioseq.GetInst().GetMol(), false) + "#" 
                         + GetDiscItemText(bioseq));
        }
      } 
      else AddMolinfoToBioseqsOfSet(molinfo_seqdesc_seqentry[i]->GetSet(), desc);
      i++;
   }

   m_entry_no ++;
};


void CSeqEntry_DISC_INCONSISTENT_MOLTYPES :: GetReport(CRef <CClickableItem>& c_item)
{
  RmvRedundancy(c_item->item_list);
  Str2Strs entry2moltps, moltp2seqs;  
  GetTestItemList(c_item->item_list, entry2moltps);
  c_item->item_list.clear();
  ITERATE (Str2Strs, it, entry2moltps) {
    moltp2seqs.clear();
    GetTestItemList(it->second, moltp2seqs);
    if (moltp2seqs.size() > 1) {
       ITERATE (Str2Strs, jt, moltp2seqs) {
         AddSubcategories(c_item, GetName(), jt->second, "sequence", "moltype" + jt->first,
                          e_HasComment); 
       } 
    }
  } 
  if (c_item->subcategories.empty()) c_item->description = "Moltypes are consistent";
  else c_item->description 
        = NStr::UIntToString((unsigned)c_item->item_list.size()) 
             + "sequences have inconsistent moltypes"; 
};


void CSeqEntry_DISC_HAPLOTYPE_MISMATCH :: ExtractNonAaBioseqsOfSet(const string& tax_hap, const CBioseq_set& set)
{
   ITERATE (list < CRef < CSeq_entry > >, it, set.GetSeq_set()) {
      if ((*it)->IsSeq() && ! ((*it)->GetSeq().IsAa()) ) 
           m_tax_hap2seqs[tax_hap].push_back(CConstRef <CBioseq> (&(*it)->GetSeq()));
      else ExtractNonAaBioseqsOfSet(tax_hap, (*it)->GetSet());
   }
};



bool CSeqEntry_DISC_HAPLOTYPE_MISMATCH :: SeqMatch(const CConstRef <CBioseq>& seq1, int beg1, const CConstRef <CBioseq>& seq2, int beg2, unsigned& len, bool Ndiff)
{
   CBioseq_Handle hdl1 = thisInfo.scope->GetBioseqHandle(*seq1);
   CBioseq_Handle hdl2 = thisInfo.scope->GetBioseqHandle(*seq2);
   CSeqVector seq_vec1 = hdl1.GetSeqVector(CBioseq_Handle::eCoding_Iupac, eNa_strand_plus); 
   CSeqVector seq_vec2 = hdl2.GetSeqVector(CBioseq_Handle::eCoding_Iupac, eNa_strand_plus); 
   unsigned sz=0;
   CSeqVector_CI it1 = seq_vec1.begin();
   CSeqVector_CI it2 = seq_vec2.begin();
   for (;   it1 && it2 && (sz < len); ++it1, ++it2, sz++) {
      if (Ndiff && ( *it1 == 'N' || *it2 == 'N' )) continue;
      else if ( *it1 == *it2 ) continue;
      else return false;
   }
   return true;
};



bool CSeqEntry_DISC_HAPLOTYPE_MISMATCH :: SeqsMatch(const vector <CConstRef <CBioseq> >& seqs, bool Ndiff)
{
  unsigned len1, len2;
  bool rvlu = true;
  int diff, i, j;
  for (i=0; i < (int)seqs.size() -1 && !rvlu; i++)
     for (j=0; (j< (int)seqs.size()) && rvlu; j++) {
       len1 = seqs[i]->IsSetLength() ? seqs[i]->GetLength() : 0;
       len2 = seqs[j]->IsSetLength() ? seqs[j]->GetLength() : 0;
       diff = len1 - len2;
       if (diff > 0) {
         while (!rvlu && diff >= 0) {
            rvlu = SeqMatch(seqs[i], diff, seqs[j], 0, len2, Ndiff);
            diff --;
         }
       }
       else if (diff < 0) {
           diff = -diff;
           while (!rvlu && diff >=0) {
             rvlu = SeqMatch(seqs[i], 0, seqs[j], diff, len1, Ndiff);
             diff --;
           }
       }
       else rvlu = SeqMatch(seqs[i], 0, seqs[j], 0, len1, Ndiff);
     }

  return rvlu;
};


void CSeqEntry_DISC_HAPLOTYPE_MISMATCH :: ReportOneHaplotypeSequenceMismatch(Str2Seqs::const_iterator& iter, bool Ndiff)
{
   strtmp = (string)"seq_diff_" + (Ndiff ? "N" : "strict") 
                                 + "_" + NStr::UIntToString(m_entry_cnt) + "$";
   string tax_hap(iter->first);
   ITERATE (vector <CConstRef <CBioseq> >, it, iter->second) {
     thisInfo.test_item_list[GetName()].push_back(
                strtmp + iter->first + "@" + GetDiscItemText(**it));
   }
};


void CSeqEntry_DISC_HAPLOTYPE_MISMATCH :: ReportHaplotypeSequenceMismatchForList()
{
   /* first, look for same taxname, same haplotype, different sequence */
   bool Ndiff_mismatch, strict_mismatch;
   ITERATE (Str2Seqs, it, m_tax_hap2seqs) {
      Ndiff_mismatch = strict_mismatch = false;
      Ndiff_mismatch = SeqsMatch(it->second);
      strict_mismatch = SeqsMatch(it->second, false);
      if (Ndiff_mismatch) ReportOneHaplotypeSequenceMismatch(it);
      if (strict_mismatch) ReportOneHaplotypeSequenceMismatch(it, false);
   }

   /* now look for sequence that match but have different haplotypes */
   vector <CConstRef <CBioseq> > seqs;
   vector <string> hap_tps;
   vector <unsigned> hap_idx;
   unsigned i;
   ITERATE (Str2Seqs, it, m_tax_hap2seqs) {
     hap_tps.push_back(it->first.substr(it->first.find("$") + 1));
     seqs.insert(seqs.end(), it->second.begin(), it->second.end());
     for (i=0; i< it->second.size(); i++) hap_idx.push_back(hap_tps.size()-1);
   }

   unsigned j, len1, len2;
   vector <string> seqs_Ndiff, seqs_strict;
   string desc1, desc2;
   bool mismatch;
   for (i=0; (int)i < (int)(seqs.size()-1); i++) {
      if (seqs[i].Empty()) continue;
      len1 = seqs[i]->IsSetLength() ? seqs[i]->GetLength() : 0; 
      desc1 = GetDiscItemText(*seqs[i]) + ": " + hap_tps[hap_idx[i]];
      mismatch = false;
      for (j=i+1; j < seqs.size(); j++) {
         if (seqs[j].Empty()) continue;
         len2 = seqs[j]->IsSetLength() ? seqs[j]->GetLength() : 0;
         if (len1 == len2) {
           desc2 = GetDiscItemText(*seqs[j]) + ": " + hap_tps[hap_idx[j]];
           if (SeqMatch(seqs[i], 0, seqs[j], 0, len1) ) {
               if (seqs_Ndiff.empty()) seqs_Ndiff.push_back(desc1);
               seqs_Ndiff.push_back(desc2);
               seqs[j].Reset();
           }
           if (SeqMatch(seqs[1], 0, seqs[j], 0, len1, false)) {
               if (seqs_strict.empty()) seqs_strict.push_back(desc1);
               seqs_Ndiff.push_back(desc2);
               seqs[j].Reset();
           }
           if (hap_tps[hap_idx[j]] != hap_tps[hap_idx[i]]) mismatch = true;
         } 
      }
      if (mismatch) {
          strtmp = "seq_same_N_" + NStr::UIntToString(m_entry_cnt) + "$";
          ITERATE (vector <string>, it, seqs_Ndiff) 
             thisInfo.test_item_list[GetName()].push_back(strtmp + *it); 
          strtmp = "seq_same_strict_" + NStr::UIntToString(m_entry_cnt) + "$" 
                                                     + NStr::UIntToString(i) + "#";
          ITERATE (vector <string>, it, seqs_strict)
             thisInfo.test_item_list[GetName()].push_back(strtmp + *it);
      }
   }
};



// new comb 
void CSeqEntry_on_biosrc_subsrc :: TestOnObj(const CSeq_entry& seq_entry)
{
   if (thisTest.is_Subsrc_run) return;

   string desc;
   ITERATE (vector <const CSeq_feat*>, it, biosrc_subsrc_feat) {
      RunTests( (*it)->GetData().GetBiosrc(), GetDiscItemText(**it));
   }

   unsigned i=0;
   ITERATE (vector <const CSeqdesc*>, it, biosrc_subsrc_seqdesc) {
     desc = GetDiscItemText(**it, *(biosrc_subsrc_seqdesc_seqentry[i]));
     i++;
   }
   
   thisTest.is_Subsrc_run = true;
};

bool CSeqEntry_on_biosrc_subsrc :: Has3Names(const vector <string> arr)
{
   vector <string> names_arr;
   unsigned cnt=0;
   ITERATE (vector <string>, it, arr) {
      names_arr = NStr::Tokenize(*it, ",; ", names_arr, NStr::eMergeDelims);
      ITERATE (vector <string>, jt, names_arr)
         if ( (*it) == "and") cnt++;
      if ((int)(names_arr.size() - cnt) > 2) return true;
   }
   return false;
};

void CSeqEntry_on_biosrc_subsrc :: RunTests(const CBioSource& biosrc, const string& desc)
{
   vector <string> arr;
   // ONCALLER_MORE_NAMES_COLLECTED_BY
   GetSubSrcValues(biosrc, CSubSource::eSubtype_collected_by, arr);
   if (Has3Names(arr)) thisInfo.test_item_list[GetName_col()].push_back(desc);

   string tax_nm;
   if (biosrc.IsSetTaxname() && !(tax_nm = biosrc.GetTaxname()).empty()) {

      // ONCALLER_SUSPECTED_ORG_IDENTIFIED
      if (IsSubSrcPresent(biosrc, CSubSource::eSubtype_identified_by)
            && (tax_nm == "Homo sapiens" || tax_nm.find("uncultured") != string::npos))
         thisInfo.test_item_list[GetName_itax()].push_back(desc);

      // ONCALLER_SUSPECTED_ORG_COLLECTED
      if (IsSubSrcPresent(biosrc, CSubSource::eSubtype_collected_by)
             && tax_nm == "Homo sapiens")
          thisInfo.test_item_list[GetName_ctax()].push_back(desc);
   }
   
   // END_COLON_IN_COUNTRY
   arr.clear();
   GetSubSrcValues(biosrc, CSubSource::eSubtype_country, arr);
   size_t pos;
   ITERATE (vector <string>, it, arr) {
     if ( ((pos = (*it).find_last_of(":")) != string::npos)
             && pos == (*it).size()-1)
        thisInfo.test_item_list[GetName_end()].push_back(desc);
   }
};

void CSeqEntry_END_COLON_IN_COUNTRY :: GetReport(CRef <CClickableItem>& c_item)
{
   
   c_item->description 
      = GetOtherComment(c_item->item_list.size(), "country source ends", 
                                                       "country sources end")
        + " with a colon.";
};


void CSeqEntry_ONCALLER_SUSPECTED_ORG_COLLECTED :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "biosource")
                         + "collected-by and suspect organism";
};

void CSeqEntry_ONCALLER_SUSPECTED_ORG_IDENTIFIED :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "biosource")
                         + "identified-by and suspect organism";
};

void CSeqEntry_ONCALLER_MORE_NAMES_COLLECTED_BY :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description = GetHasComment(c_item->item_list.size(), "biosource") 
                       + "3 or more names in collected-by.";
};

// new comb


void CSeqEntry_DISC_HAPLOTYPE_MISMATCH :: TestOnObj(const CSeq_entry& seq_entry)
{
   if (thisInfo.test_item_list.find(GetName()) == thisInfo.test_item_list.end()) 
        m_entry_cnt = 0;
   else m_entry_cnt ++;
   string tax_nm, hap_tp;
   unsigned i=0;
   ITERATE (vector <const CSeqdesc*>, it, biosrc_subsrc_seqdesc) {
     const CBioSource& biosrc = (*it)->GetSource();
     if (biosrc.IsSetTaxname() && !(tax_nm = biosrc.GetTaxname()).empty()) {
        ITERATE (list <CRef <CSubSource> >, it, biosrc.GetSubtype()) {
           if ((*it)->GetSubtype() == CSubSource :: eSubtype_haplotype 
                 && !(hap_tp = (*it)->GetName()).empty()) {
               if (biosrc_subsrc_seqdesc_seqentry[i]->IsSet()) {
                  ExtractNonAaBioseqsOfSet(tax_nm +"#" + hap_tp, 
                                            biosrc_subsrc_seqdesc_seqentry[i]->GetSet());
               }
               break;
           }
        }
     }
   }

   ReportHaplotypeSequenceMismatchForList();
};


void CSeqEntry_DISC_HAPLOTYPE_MISMATCH :: MakeCitem4DiffSeqs(CRef <CClickableItem>& c_item, const vector <string> tax_hap_seqs, bool Ndiff)
{
   Str2Strs tax_hap2seqs;
   GetTestItemList(tax_hap_seqs, tax_hap2seqs, "@");
   size_t pos;
   string tax_nm, hap_tp;
   ITERATE (Str2Strs, it, tax_hap2seqs){
      pos = it->first.find("#");
      tax_nm = it->first.substr(0, pos);
      hap_tp = it->first.substr(pos+1);
      strtmp = "organism " + tax_nm + " haplotype " + hap_tp 
          + " but the sequences do not match " 
          + (Ndiff ? "(allowing N to match any)." : "(strict match).");
      AddSubcategories(c_item, GetName(), it->second, "sequence", strtmp, e_HasComment);
   }

};


void CSeqEntry_DISC_HAPLOTYPE_MISMATCH :: MakeCitem4SameSeqs(CRef <CClickableItem>& c_item, const vector <string>& idx_seqs, bool Ndiff)
{
   Str2Strs idx2seqs;
   GetTestItemList(idx_seqs, idx2seqs, "#");
   strtmp = (string)"identical " + (Ndiff ? "(allowing N to match any)" : "(strict match)")
            + " but have different haplotypes.";
   ITERATE (Str2Strs, it, idx2seqs)
      AddSubcategories(c_item, GetName(), it->second, "sequences", strtmp);

};


void CSeqEntry_DISC_HAPLOTYPE_MISMATCH :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs match_tp2ls;
   GetTestItemList(c_item->item_list, match_tp2ls);
   c_item->item_list.clear();
   CRef <CClickableItem> csub_N (new CClickableItem);
   CRef <CClickableItem> csub_strict(new CClickableItem);
   
   string tax_nm, hap_tp;
   ITERATE (Str2Strs, it, match_tp2ls) {
     if (it->first.find("seq_diff_N") != string::npos) MakeCitem4DiffSeqs(csub_N, it->second);
     else if (it->first.find("seq_same_N") != string::npos)
               MakeCitem4SameSeqs(csub_N, it->second);
     else if (it->first.find("seq_diff_strict") != string::npos)
                   MakeCitem4DiffSeqs(csub_strict, it->second,true);
     else MakeCitem4DiffSeqs(csub_strict, it->second, true);  // "seq_same_strict"
   }

   if (csub_N.NotEmpty()) {
     csub_N->description 
       = "There are " + NStr::UIntToString((unsigned)csub_N->item_list.size())
         + "haplotype problems (loose match, allowing Ns to differ)";
     csub_N->setting_name = GetName();
     c_item->subcategories.push_back(csub_N);
     c_item->item_list.insert(
              c_item->item_list.end(), csub_N->item_list.begin(), csub_N->item_list.end());
   }
   if (csub_strict.NotEmpty()) {
      csub_strict->description
        = "There are " + NStr::UIntToString((unsigned)csub_strict->item_list.size())
            + "haplotype problems (strict match)";
      csub_strict->setting_name = GetName();
      c_item->subcategories.push_back(csub_strict);
      c_item->item_list.insert(
        c_item->item_list.end(), csub_strict->item_list.begin(), csub_strict->item_list.end());
   }
   c_item->description = "Haplotype Problem Report";
};


CConstRef <CCit_sub> CSeqEntry_test_on_pub :: CitSubFromPubEquiv(const list <CRef <CPub> >& pubs)
{
   ITERATE (list <CRef <CPub> >, it, pubs) {
      if ((*it)->IsSub())
        return (CConstRef <CCit_sub> ( &( (*it)->GetSub() )));
      else if ( (*it)->IsEquiv())
        return (CitSubFromPubEquiv( (*it)->GetEquiv().Get()));
   }
   return (CConstRef <CCit_sub>());
};



static string uni_str("University of");
bool CSeqEntry_test_on_pub :: AffilStreetEndsWith(const string& street, const string& end_str)
{
  unsigned street_sz = street.size();
  unsigned end_sz = end_str.size();
  unsigned diff_sz = street_sz - end_sz;
  unsigned uni_sz = uni_str.size();
  if (!end_sz || street_sz < end_sz) return false;
  else if (NStr::EqualNocase(street, diff_sz, street_sz-1, end_str)
            && ( (street_sz == end_sz 
                        || isspace(street[diff_sz-1]) || ispunct(street[diff_sz-1])))) {
     if (diff_sz >= uni_sz 
            && NStr::EqualNocase(street, diff_sz - uni_sz -1, diff_sz-1, uni_str))
          return false;
     else return true;
  }
  else return false;
};


bool CSeqEntry_test_on_pub :: AffilStreetContainsDuplicateText(const CAffil& affil)
{
  if (affil.IsStd() && affil.GetStd().CanGetStreet() 
                              && !(affil.GetStd().GetStreet().empty())) {
    const CAffil :: C_Std& std = affil.GetStd();
    const string& street = std.GetStreet();
    if (std.CanGetCountry() && AffilStreetEndsWith(street, std.GetCountry()))
         return true;
    else if (std.CanGetPostal_code()
                            && AffilStreetEndsWith (street, std.GetPostal_code()))
            return true;
    else if (std.CanGetSub() && AffilStreetEndsWith (street, std.GetSub()))
            return true;
    else if (std.CanGetCity() && AffilStreetEndsWith (street, std.GetCity()))
           return true;
    else return false;
  }
  else return false;
};



// new comb
bool CSeqEntry_test_on_pub :: CorrectUSAStates(CConstRef <CCit_sub>& cit_sub)
{
   string country, state;
   if (cit_sub->GetAuthors().CanGetAffil()) {
       const CAffil& affil = cit_sub->GetAuthors().GetAffil();
       if (affil.IsStd()) {
           country =(affil.GetStd().CanGetCountry()) ? affil.GetStd().GetCountry() : kEmptyStr;
           state = (affil.GetStd().CanGetSub()) ? affil.GetStd().GetSub() : kEmptyStr;
           if (country == "USA") {
               if (state != "Washington DC") {
                   if (state.empty() || state.size() > 2 
                                   || !isupper(state[0]) || !isupper(state[1]))
                          return false;
                   else {
                        ITERATE (list <string>, it, thisInfo.state_abbrev) 
                              if (state == *it) return true;
                        return false;
                   }
               }
           } 
       }
   }
   return true;
};


string CSeqEntry_test_on_pub :: GetAuthNameList(const CAuthor& auth, bool use_initials)
{
  string str;
  const CPerson_id& pid = auth.GetName();
  switch (pid.Which()) {
    case CPerson_id::e_Dbtag:
          pid.GetDbtag().GetLabel(&str);
          break;
    case CPerson_id::e_Name:
         { const CName_std& nm_std = pid.GetName();
          if (use_initials)
              str =(nm_std.CanGetInitials()? nm_std.GetInitials() + " " :"") +nm_std.GetLast();
          else {
            str = (nm_std.CanGetFirst()? nm_std.GetFirst() + " " : "")
                    + ( (nm_std.CanGetInitials() && nm_std.GetInitials().size()>2)? 
                          nm_std.GetInitials().substr(2) + " " : "")
                    + nm_std.GetLast();
          }
          }
          break;
    case CPerson_id::e_Ml:
          str = pid.GetMl(); break;
    case CPerson_id::e_Str:
          str = pid.GetStr(); break;
    case CPerson_id::e_Consortium: 
          str = pid.GetConsortium(); break;
    default:   break;
  }
  return str;
};


void CSeqEntry_test_on_pub :: CheckTitleAndAuths(CConstRef <CCit_sub>& cit_sub, const string& desc)
{
   string title = cit_sub->CanGetDescr()? cit_sub->GetDescr() : kEmptyStr;
   string authors(kEmptyStr);
   switch (cit_sub->GetAuthors().GetNames().Which()) {
        case CAuth_list::C_Names::e_Std:
                ITERATE (list <CRef <CAuthor> >, it, cit_sub->GetAuthors().GetNames().GetStd())
                      authors += ", " + GetAuthNameList(**it);
                break;
        case CAuth_list::C_Names::e_Ml:
                ITERATE (list <string>, it, cit_sub->GetAuthors().GetNames().GetMl())
                      authors += ", " + *it; 
                break;
        case CAuth_list::C_Names::e_Str:
                ITERATE (list <string>, it, cit_sub->GetAuthors().GetNames().GetStr())
                      authors += ", " + *it; 
                break;
        default: break;
   };

   if (!authors.empty())  authors = authors.substr(2);
   thisInfo.test_item_list[GetName_tlt()].push_back(title + "$" + authors + "#" + desc);
};


void CSeqEntry_test_on_pub :: RunTests(const list <CRef <CPub> >& pubs, const string& desc)
{
   CConstRef <CCit_sub> cit_sub = CitSubFromPubEquiv(pubs);
   if (cit_sub.NotEmpty()) {
       // DISC_CITSUB_AFFIL_DUP_TEXT
       if (cit_sub->GetAuthors().CanGetAffil()) {
          const CAffil& affil = cit_sub->GetAuthors().GetAffil();
          if (affil.IsStd() && affil.GetStd().CanGetStreet()
                 && !(affil.GetStd().GetStreet().empty())
                 && AffilStreetContainsDuplicateText(cit_sub->GetAuthors().GetAffil())) {
             thisInfo.test_item_list[GetName_dup()].push_back(desc);
          }
       }

      // DISC_USA_STATE
      if (!CorrectUSAStates(cit_sub))
         thisInfo.test_item_list[GetName_usa()].push_back(desc);
  
      // DISC_TITLE_AUTHOR_CONFLICT;
      CheckTitleAndAuths(cit_sub, desc);


      // DISC_CITSUBAFFIL_CONFLICT
      string affil_str, grp_str;
      GetGroupedAffilString(cit_sub->GetAuthors(), affil_str, grp_str);
      thisInfo.test_item_list[GetName_aff()].push_back(grp_str + "$" + desc);
      m_has_cit = true;

      // DISC_MISSING_AFFIL
      if (cit_sub->GetAuthors().CanGetAffil()) {
         const CAffil& affil = cit_sub->GetAuthors().GetAffil();
         if (IsMissingAffil(cit_sub->GetAuthors().GetAffil()))
            thisInfo.test_item_list[GetName_noaff()].push_back(desc);
      } 
   }

   // DISC_CHECK_AUTH_CAPS, DISC_CHECK_AUTH_NAME
   CheckBadAuthCapsOrNoFirstLastNamesInPubdesc(pubs, desc);

   // DISC_UNPUB_PUB_WITHOUT_TITLE
   if (DoesPubdescContainUnpubPubWithoutTitle(pubs))
      thisInfo.test_item_list[GetName_unp()].push_back(desc);

   // ONCALLER_CONSORTIUM
   if (PubHasConsortium(pubs)) 
      thisInfo.test_item_list[GetName_cons()].push_back(desc);
};


bool CSeqEntry_test_on_pub :: AuthorHasConsortium(const CAuthor& author)
{
   if (author.GetName().IsConsortium() && !author.GetName().GetConsortium().empty()) 
     return true;
   else return false;
};


bool CSeqEntry_test_on_pub :: AuthListHasConsortium(const CAuth_list& auth_ls)
{
   if ( auth_ls.GetNames().IsStd()) {
      ITERATE (list <CRef <CAuthor> >, it, auth_ls.GetNames().GetStd()) {
         if (AuthorHasConsortium (**it)) return true;
      }
   }
   return false;
};


bool CSeqEntry_test_on_pub :: PubHasConsortium(const list <CRef <CPub> >& pubs)
{
  ITERATE (list <CRef <CPub> >, it, pubs) {
    if ( (*it)->IsSetAuthors() ) 
       if (AuthListHasConsortium( (*it)->GetAuthors())) return true;
  }
  return false;
};
 

void CSeqEntry_ONCALLER_CONSORTIUM :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description
    = GetOtherComment(c_item->item_list.size(), "publication/submitter block has", 
                                         "publications/submitter blocks have") 
       + " consortium";
};


bool CSeqEntry_test_on_pub :: IsMissingAffil(const CAffil& affil)
{
   if ( (affil.IsStd() && affil.GetStd().CanGetAffil() && affil.GetStd().GetAffil().empty())
         || (affil.IsStr() && affil.GetStr().empty()) ) 
       return true;
   return false;
};


CSeqEntry_test_on_pub::E_Status CSeqEntry_test_on_pub :: GetPubMLStatus (const CPub& pub)
{
  switch (pub.Which()) {
    case CPub::e_Gen:
        if (pub.GetGen().CanGetCit() 
              && NStr::FindNoCase(pub.GetGen().GetCit(), "unpublished") != string::npos)
           return e_unpublished;
        else return e_published;
    case CPub::e_Sub: return e_submitter_block;
    case CPub::e_Article:
       switch (pub.GetArticle().GetFrom().Which() ) {
         case CCit_art::C_From::e_Journal:
              return ImpStatus(pub.GetArticle().GetFrom().GetJournal().GetImp());
         case CCit_art::C_From::e_Book:
              return ImpStatus(pub.GetArticle().GetFrom().GetBook().GetImp());
         case CCit_art::C_From::e_Proc:
              return ImpStatus(pub.GetArticle().GetFrom().GetProc().GetBook().GetImp());
         default: return e_any;
       }
    case CPub::e_Journal: return ImpStatus(pub.GetJournal().GetImp()); 
    case CPub::e_Book: return ImpStatus(pub.GetBook().GetImp());
    case CPub::e_Man: return ImpStatus(pub.GetMan().GetCit().GetImp());
    case CPub::e_Patent: return e_published;
    default: return e_any;
  }
};


CSeqEntry_test_on_pub::E_Status CSeqEntry_test_on_pub :: ImpStatus(const CImprint& imp, bool is_pub_sub)
{ 
  if (imp.CanGetPrepub()) {
     switch (imp.GetPrepub()) {
       case CImprint::ePrepub_submitted:
          if (is_pub_sub) return e_submitter_block;
          return e_unpublished;
       case CImprint::ePrepub_in_press: return e_in_press;
       case CImprint::ePrepub_other: return e_unpublished;
     }
  }
  return e_published;
};


string CSeqEntry_test_on_pub :: Get1stTitle(const CTitle::C_E& title)
{
   switch (title.Which()) {
     case CTitle::C_E::e_Name:    return title.GetName();
     case CTitle::C_E::e_Tsub:    return title.GetTsub();
     case CTitle::C_E::e_Trans:   return title.GetTrans();
     case CTitle::C_E::e_Jta:     return title.GetJta();
     case CTitle::C_E::e_Iso_jta: return title.GetIso_jta();
     case CTitle::C_E::e_Ml_jta:  return title.GetMl_jta();
     case CTitle::C_E::e_Coden:   return title.GetCoden();
     case CTitle::C_E::e_Issn:    return title.GetIssn();
     case CTitle::C_E::e_Abr:     return title.GetAbr();
     case CTitle::C_E::e_Isbn:    return title.GetIsbn();
     default: return kEmptyStr;
   }
};

string CSeqEntry_test_on_pub :: GetTitleFromPub(const CPub& pub)
{
  switch (pub.Which()) {
    case CPub::e_Gen:
      return (pub.GetGen().CanGetTitle() ? pub.GetGen().GetTitle() : kEmptyStr);
    case CPub::e_Sub:
      return (pub.GetSub().CanGetDescr() ? pub.GetSub().GetDescr() : kEmptyStr);
    case CPub::e_Article:
      if (pub.GetArticle().CanGetTitle()) {
         Get1stTitle(**(pub.GetArticle().GetTitle().Get().begin()));  
      }  
      else return kEmptyStr;
    case CPub::e_Book:
      return Get1stTitle(**(pub.GetBook().GetTitle().Get().begin()));
    case CPub::e_Man:
      return Get1stTitle(**(pub.GetMan().GetCit().GetTitle().Get().begin()));
    case CPub::e_Patent:
      return (pub.GetPatent().GetTitle());
    default: return kEmptyStr;
  }
};


bool CSeqEntry_test_on_pub :: DoesPubdescContainUnpubPubWithoutTitle(const list <CRef <CPub> >& pubs)
{
  string title;
  ITERATE (list <CRef <CPub> >, it, pubs) {
     if (GetPubMLStatus( **it ) == e_unpublished) {
       title = GetTitleFromPub(**it);
       if (title.empty() || NStr::FindNoCase(title, "Direct Submission")!= string::npos)
         return true;
     }
  }
  return false;
};


void CSeqEntry_DISC_UNPUB_PUB_WITHOUT_TITLE :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
       = GetHasComment(c_item->item_list.size(), "unpublished pub") + "no title";
};


void CSeqEntry_DISC_CITSUBAFFIL_CONFLICT :: GetReport(CRef <CClickableItem>& c_item)
{
   if (c_item->item_list[0] == "no cit") {
     c_item->item_list.clear();
     c_item->description = "No citsubs were found!";
     return;
   }
    
   Str2Strs affil2pubs;
   GetTestItemList(c_item->item_list, affil2pubs);
   c_item->item_list.clear();
   if (affil2pubs.size() == 1) {
      if ( affil2pubs.begin()->first != "no affil" )
            c_item->description = "All citsub affiliations match";
      else c_item->description = "All citsub have no affiliations";
   }
   else {
      vector <string> subcat_pub, no_affil_pubs;
      ITERATE (Str2Strs, it, affil2pubs) {
         if (it->first == "no affil") no_affil_pubs = it->second;
         else {
            subcat_pub = NStr::Tokenize(it->first, ",", subcat_pub);  
            NON_CONST_ITERATE (vector <string>, jt, subcat_pub)
               (*jt) = (*jt) + "$" + it->second[0];
         }
      }
      if (!no_affil_pubs.empty()) 
          AddSubcategories(c_item,GetName(),no_affil_pubs,"Cit-sub", "no affiliation", 
                                                                          e_HasComment, false);
      Str2Strs subcat2pubs, subvlu2pubs;
      GetTestItemList(subcat_pub, subcat2pubs, "@");
      subcat_pub.clear();
      ITERATE (Str2Strs, it, subcat2pubs) {
         subvlu2pubs.clear();
         GetTestItemList(it->second, subvlu2pubs);
         if (subvlu2pubs.size() > 1) {
            CRef <CClickableItem> c_sub (new CClickableItem);
            c_sub->setting_name = GetName();
            ITERATE (Str2Strs, jt, subvlu2pubs)
               AddSubcategories(c_sub, GetName(), jt->second, "affiliation", 
                                       it->first + " value '" + jt->first + "'", e_HasComment);
            c_sub->description = "Affiliations have different values for " + it->first;
            c_item->subcategories.push_back(c_sub);
         } 
      }
      c_item->description = "Citsub affiliation conflicts found";
   }
};


void CSeqEntry_test_on_pub :: GetGroupedAffilString(const CAuth_list& authors, string& affil_str, string& grp_str)
{
   affil_str = grp_str = kEmptyStr;
   if (authors.CanGetAffil()) {
      const CAffil& affil = authors.GetAffil();
      switch (affil.Which()) {
      case CAffil :: e_not_set: return;
      case CAffil :: e_Str:
          affil_str = NStr::Replace(affil.GetStr(), "\"", "'");
          grp_str  = "no group";
          return;
      case CAffil :: e_Std:
        {
 #define ADD_AFFIL_FIELD(X, Y) \
         if (affil.GetStd().CanGet##X()  &&  !(affil.GetStd().Get##X().empty())) { \
             string value = NStr::Replace(affil.GetStd().Get##X(), "\"", "'"); \
             affil_str += "," + value; \
             grp_str += "," #Y "@" + value; \
         }
         ADD_AFFIL_FIELD(Div, department)
         ADD_AFFIL_FIELD(Affil, institution)
         ADD_AFFIL_FIELD(Street, street)
         ADD_AFFIL_FIELD(City, city)
         ADD_AFFIL_FIELD(Sub, state/province)
         ADD_AFFIL_FIELD(Postal_code, postal_code)
         ADD_AFFIL_FIELD(Country, country)
         affil_str = affil_str.empty() ? "no affil" : affil_str.substr(1);
         grp_str = grp_str.empty() ? "no affil" : grp_str.substr(1);
 #undef ADD_AFFIL_FIELD
         return;
       }
     }    
   }
};


void CSeqEntry_test_on_pub :: TestOnObj(const CSeq_entry& seq_entry)
{
   if (thisTest.is_Pub_run) return;
   m_has_cit = false;

   string desc;
   ITERATE (vector <const CSeq_feat*>, it, pub_feat) {
      const list <CRef <CPub> >& pubs = (*it)->GetData().GetPub().GetPub().Get();
      RunTests(pubs, GetDiscItemText(**it));
   };

   unsigned i=0;
   ITERATE (vector <const CSeqdesc*>, it, pub_seqdesc) {
      desc = GetDiscItemText(**it, *(pub_seqdesc_seqentry[i]));
      const list <CRef <CPub> >& pubs = (*it)->GetPub().GetPub().Get();
      RunTests(pubs, desc);
      i++;
   };

   CSubmit_block this_submit_blk;
   if (thisInfo.seq_submit.NotEmpty()) {
      if (seq_entry.IsSeq()) desc = GetDiscItemText(seq_entry.GetSeq());
      else desc = GetDiscItemText(seq_entry.GetSet());

      const CAuth_list& auths = thisInfo.seq_submit->GetSub().GetCit().GetAuthors();
      // DISC_CITSUB_AFFIL_DUP_TEXT
      if (auths.CanGetAffil() && AffilStreetContainsDuplicateText(auths.GetAffil())) {
          size_t pos = desc.find(": ");
          strtmp = desc.substr(0, pos + 2) + "Cit-sub for " + desc.substr(pos+3);
          thisInfo.test_item_list[GetName_dup()].push_back(strtmp);
      }

      // DISC_CHECK_AUTH_CAPS
      if ( HasBadAuthorName(auths)) thisInfo.test_item_list[GetName_cap()].push_back(desc);

      // DISC_CITSUBAFFIL_CONFLICT
      string affil_str, grp_str;
      GetGroupedAffilString(auths, affil_str, grp_str);
      thisInfo.test_item_list[GetName_aff()].push_back(grp_str + "$" + desc);
      m_has_cit = true;

      // DISC_MISSING_AFFIL
      const CContact_info& contact = thisInfo.seq_submit->GetSub().GetContact();
      desc = GetDiscItemText(*thisInfo.seq_submit);
      if (!contact.CanGetContact()
            || !contact.GetContact().CanGetAffil()
            || IsMissingAffil(contact.GetContact().GetAffil())) {
          thisInfo.test_item_list[GetName_noaff()].push_back(desc);
      }
      else if (!auths.CanGetAffil() 
                 || IsMissingAffil(auths.GetAffil())) {
           thisInfo.test_item_list[GetName_noaff()].push_back(desc);
      }

     // ONCALLER_CONSORTIUM
     if ( (contact.CanGetContact() && AuthorHasConsortium(contact.GetContact()))
           || AuthListHasConsortium(auths))
        thisInfo.test_item_list[GetName_cons()].push_back(desc);
   }
 
   if (!m_has_cit) 
       thisInfo.test_item_list[GetName_aff()].push_back("no cit");
   thisTest.is_Pub_run = true;
};


void CSeqEntry_DISC_MISSING_AFFIL :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
          = GetIsComment(c_item->item_list.size(), "citsub") + "missing affiliation";
};


void CSeqEntry_DISC_TITLE_AUTHOR_CONFLICT :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs tlt2auths, auth2feats;
   GetTestItemList(c_item->item_list, tlt2auths);
   c_item->item_list.clear();
   ITERATE (Str2Strs, it, tlt2auths) {
      auth2feats.clear();
      GetTestItemList(it->second, auth2feats);
      if (auth2feats.size() > 1) {
         CRef <CClickableItem> c_tlt (new CClickableItem);
         c_tlt->setting_name = GetName();
         ITERATE (Str2Strs, jt, auth2feats) {
           AddSubcategories(c_tlt, GetName(), jt->second, "article", 
                         "title '" + it->first + "' and author list '" + jt->first + "'", 
                          e_HasComment, true);
         }
         c_tlt->description = NStr::UIntToString((unsigned)c_tlt->item_list.size()) 
             + " articles have title '" + it->first + " but do not have the same author list";
         c_item->item_list.insert(c_item->item_list.end(),
                                   c_tlt->item_list.begin(), c_tlt->item_list.end());
         c_item->subcategories.push_back(c_tlt);
      } 
   };
   if (!c_item->item_list.empty())
      c_item->description = "Publication Title/Author Inconsistencies";
};


void CSeqEntry_DISC_USA_STATE :: GetReport(CRef <CClickableItem>& c_item)
{
   sort(c_item->item_list.begin(), c_item->item_list.end());
   c_item->description 
       = GetIsComment(c_item->item_list.size(), "cit-sub") + "missing state abbreviations.";
};


void CSeqEntry_DISC_CITSUB_AFFIL_DUP_TEXT :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description
     = GetHasComment(c_item->item_list.size(), "Cit-sub pub") + "duplicate affil text.";
};
// new comb



void CSeqEntryTestAndRepData :: AddBioseqsOfSetToReport(const CBioseq_set& bioseq_set, const string& setting_name, bool be_na, bool be_aa)
{
   ITERATE (list < CRef < CSeq_entry > >, it, bioseq_set.GetSeq_set()) {
     if ((*it)->IsSeq()) {
         const CBioseq& bioseq = (*it)->GetSeq();
         if ( (be_na && bioseq.IsNa()) || (be_aa && bioseq.IsAa()) ) 
           thisInfo.test_item_list[setting_name].push_back(GetDiscItemText(bioseq));
     }
     else AddBioseqsOfSetToReport( (*it)->GetSet(), setting_name, be_na, be_aa);
   }
};



void CSeqEntryTestAndRepData :: AddBioseqsInSeqentryToReport(const CSeq_entry* seq_entry, const string& setting_name, bool be_na, bool be_aa)
{
  if (seq_entry->IsSeq()) 
    thisInfo.test_item_list[setting_name].push_back(
                            GetDiscItemText( seq_entry->GetSeq() ));
  else AddBioseqsOfSetToReport(seq_entry->GetSet(), setting_name, be_na, be_aa);
};



void CSeqEntry_INCONSISTENT_BIOSOURCE :: AddAllBioseqToList(const CBioseq_set& bioseq_set, CRef <GeneralDiscSubDt>& item_list, const string& biosrc_txt)
{
  ITERATE (list < CRef < CSeq_entry > >, it, bioseq_set.GetSeq_set()) {
     if ((*it)->IsSeq()) { 
         if ((*it)->GetSeq().IsNa()) {
              item_list->obj_text.push_back( biosrc_txt);
              item_list->obj_text.push_back( GetDiscItemText((*it)->GetSeq()));
         }
     }
     else AddAllBioseqToList( (*it)->GetSet(), item_list, biosrc_txt);
  }
};


bool CSeqEntry_INCONSISTENT_BIOSOURCE :: HasNaInSet(const CBioseq_set& bioseq_set)
{
  bool rvl = false;
  ITERATE (list < CRef < CSeq_entry > >, it, bioseq_set.GetSeq_set()) {
     if ((*it)->IsSeq()) {
        if ( (*it)->GetSeq().IsNa()) return true;
        else return false;
     }
     else rvl = HasNaInSet( (*it)->GetSet() );
  }

  return rvl;
};



CRef <GeneralDiscSubDt> CSeqEntry_INCONSISTENT_BIOSOURCE :: AddSeqEntry(const CSeqdesc& seqdesc, const CSeq_entry& seq_entry)
{
    CRef < GeneralDiscSubDt > new_biosrc_ls (new GeneralDiscSubDt (kEmptyStr, kEmptyStr));
    new_biosrc_ls->obj_text.clear();

    string biosrc_txt =  GetDiscItemText(seqdesc, seq_entry);
    if (seq_entry.IsSeq()) {
            new_biosrc_ls->obj_text.push_back(biosrc_txt);
            new_biosrc_ls->obj_text.push_back(GetDiscItemText(seq_entry.GetSeq()));
    }
    else AddAllBioseqToList(seq_entry.GetSet(), new_biosrc_ls, biosrc_txt);

    new_biosrc_ls->biosrc = &(seqdesc.GetSource());
    return (new_biosrc_ls);
};



void CSeqEntry_INCONSISTENT_BIOSOURCE :: TestOnObj(const CSeq_entry& seq_entry)
{
   bool found, has_na, is_seq = false;;
   unsigned i=0;
   string biosrc_txt;
   ITERATE (vector <const CSeqdesc*>, it, biosrc_seqdesc) {
      found = has_na = is_seq = false;
      const CSeq_entry* this_seq = biosrc_seqdesc_seqentry[i];
      if (this_seq->IsSeq() && this_seq->GetSeq().IsNa()) {
                has_na = true;
                is_seq = true;
      }
      else has_na = HasNaInSet(this_seq->GetSet());
      if (!has_na) continue;
      NON_CONST_ITERATE(vector <CRef < GeneralDiscSubDt > >,jt,INCONSISTENT_BIOSOURCE_biosrc) {
         if ( BioSourceMatch((*it)->GetSource(), *((*jt)->biosrc)) ) {
            biosrc_txt = GetDiscItemText(**it, *this_seq);
            if (is_seq) {
                  (*jt)->obj_text.push_back(biosrc_txt);
                  (*jt)->obj_text.push_back( GetDiscItemText(this_seq->GetSeq()) );
            }
            else {
              const CBioseq_set& bioseq_set = this_seq->GetSet();
              AddAllBioseqToList(bioseq_set, *jt, biosrc_txt);
            }
            found = true;
            break;
         }
     }
     if (!found) {
          INCONSISTENT_BIOSOURCE_biosrc.push_back(AddSeqEntry(**it, *this_seq));
          if (thisInfo.test_item_list.find(GetName()) == thisInfo.test_item_list.end()
                                   && INCONSISTENT_BIOSOURCE_biosrc.size() > 1)
             thisInfo.test_item_list[GetName()].push_back("inconsistance");
     }
     i++;
   }
};


void CSeqEntry_INCONSISTENT_BIOSOURCE :: GetReport(CRef <CClickableItem>& c_item)
{
   string diff =
            (INCONSISTENT_BIOSOURCE_biosrc.size() > 1)?
              " (" + DescribeBioSourceDifferences(*(INCONSISTENT_BIOSOURCE_biosrc[0]->biosrc),
                                           *(INCONSISTENT_BIOSOURCE_biosrc[1]->biosrc)) + ")."
              : ".";
   c_item->item_list.clear();
   ITERATE (vector <CRef < GeneralDiscSubDt > >, it, INCONSISTENT_BIOSOURCE_biosrc) {
     CRef <CClickableItem> sub_c_item (new CClickableItem);
     sub_c_item->setting_name = GetName();
     sub_c_item->item_list = (*it)->obj_text;
     c_item->item_list.insert(
                 c_item->item_list.end(), (*it)->obj_text.begin(), (*it)->obj_text.end());
     
     sub_c_item->description = GetHasComment(sub_c_item->item_list.size()/2, "contig")
                             + "identical sources that do not match another contig source.";
     c_item->subcategories.push_back(sub_c_item);
   }
   c_item->description =
       GetOtherComment(c_item->item_list.size()/2, "inconsistent contig source.",
                                                         "inconsistent contig sources") + diff;

   INCONSISTENT_BIOSOURCE_biosrc.clear();
};


//new comb
void CSeqEntry_test_on_defline :: AddBioseqsOfSet(const CBioseq_set& set)
{
   ITERATE (list <CRef <CSeq_entry> >, it, set.GetSeq_set()) {
     if ( (*it)->IsSeq() ) {
        if ( !((*it)->GetSeq().IsAa() ))
           m_bioseqs.push_back(GetDiscItemText( (*it)->GetSeq() ));
     }
     else AddBioseqsOfSet( (*it)->GetSet());
   }
};


void CSeqEntry_test_on_defline :: RmvBioseqsOfSet(const CBioseq_set& set)
{
   ITERATE (list <CRef <CSeq_entry> >, it, set.GetSeq_set()) {
     if ( (*it)->IsSeq() ) {
        if ( !((*it)->GetSeq().IsAa() ))
           m_bioseqs.remove(GetDiscItemText( (*it)->GetSeq() ));
     }
     else RmvBioseqsOfSet( (*it)->GetSet());
   }
};


void CSeqEntry_test_on_defline :: TestOnObj(const CSeq_entry& seq_entry)
{
    if (thisTest.is_Defl_run) return;
    // DISC_MISSING_DEFLINES
    if (seq_entry.IsSeq()) {
        if (!seq_entry.GetSeq().IsAa()) 
               m_bioseqs.push_back(GetDiscItemText(seq_entry.GetSeq()));
    }
    else AddBioseqsOfSet(seq_entry.GetSet());
    m_bioseqs.sort();

    unsigned i=0, end, cnt; 
    string desc, title;
    ITERATE ( vector <const CSeqdesc*>, it, title_seqdesc) {
       if ((title = (*it)->GetTitle()).empty()) continue; 
       desc = GetDiscItemText(**it, *(title_seqdesc_seqentry[i]));
       // ONCALLER_DEFLINE_ON_SET
       if (title_seqdesc_seqentry[i]->IsSet()) 
            thisInfo.test_item_list[GetName_set()].push_back(desc);
     
       // DISC_DUP_DEFLINE
       title = NStr::ToUpper(title); 
       thisInfo.test_item_list[GetName_dup()].push_back(title + "$" + desc);

       // DISC_MISSING_DEFLINES
       if (title_seqdesc_seqentry[i]->IsSeq()) {
         if (!title_seqdesc_seqentry[i]->GetSeq().IsAa())
             m_bioseqs.remove(GetDiscItemText(title_seqdesc_seqentry[i]->GetSeq()));
       }
       else RmvBioseqsOfSet(title_seqdesc_seqentry[i]->GetSet());

       // DISC_TITLE_ENDS_WITH_SEQUENCE
       end = title.size()-1;
       strtmp = title[end];
       while (end && (cnt < 19)
                 && (strtmp == "A" || strtmp == "C" || strtmp == "T" || strtmp == "G")){
           end--;
           cnt ++;
           strtmp = title[end];
       }
       if (cnt >= 19) thisInfo.test_item_list[GetName_seqch()].push_back(desc);
 
       i++;
    }

    // DISC_MISSING_DEFLINES
    ITERATE (list <string>, it, m_bioseqs)
      thisInfo.test_item_list[GetName_notlt()].push_back(*it);
 
    thisTest.is_Defl_run = true;
};

void CSeqEntry_DISC_TITLE_ENDS_WITH_SEQUENCE :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description
     = GetOtherComment(c_item->item_list.size(), "defline appears", "deflines appear")
       + " to end with sequence characters";
};

void CSeqEntry_DISC_MISSING_DEFLINES :: GetReport(CRef <CClickableItem>& c_item)
{
  c_item->description 
        = GetHasComment(c_item->item_list.size(), "bioseq") + "no definition line";
};

void CSeqEntry_DISC_DUP_DEFLINE :: GetReport(CRef <CClickableItem>& c_item)
{
   Str2Strs def2ls;
   GetTestItemList(c_item->item_list, def2ls);
   c_item->item_list.clear();

   vector <string> unique;
   unsigned dup_cnt = 0;
   Str2Strs::const_iterator iit;
   ITERATE (Str2Strs, it, def2ls) {
      if (it->second.size() > 1) {
              dup_cnt++;
              iit = it;
      }
      else unique.push_back(it->second[0]);
   }  
   if (!dup_cnt) c_item->description = "All deflines are unique";
   else {
      if (dup_cnt == 1) { 
          if ( unique.empty()) {
             c_item->item_list = iit->second;
             c_item->description 
                   = GetIsComment(c_item->item_list.size(), "definition line") 
                      + "identical";
          }
          else {
              AddSubcategories(c_item, GetName(), iit->second, "definition line",
                                                    "identical", e_IsComment, false);
              AddSubcategories(c_item, GetName(), unique, "definition line", 
                                                    "unique", e_IsComment, false);
              c_item->description = "Defline Problem Report";
          }
      }
      else {
         ITERATE (Str2Strs, it, def2ls) {
            if (it->second.size() > 1)
              AddSubcategories(c_item, GetName(), it->second, "definition line",
                                                    "identical", e_IsComment, false);
         }
         AddSubcategories(c_item, GetName(), unique, "definition line",
                                                    "unique", e_IsComment, false);
         c_item->description = "Defline Problem Report";
      }
   }
};


void CSeqEntry_ONCALLER_DEFLINE_ON_SET :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description 
     = GetOtherComment(c_item->item_list.size(), "title on sets was", "titles on sets were") 
          + " found";

   ITERATE (vector <string>, it, thisInfo.test_item_list[GetName()]) {   // for oncaller left window display
      CRef <CClickableItem> c_sub (new CClickableItem);
      c_sub->description = *it;
      c_sub->item_list.push_back(*it);
      c_item->subcategories.push_back(c_sub); 
   }

   if (thisInfo.expand_defline_on_set == "true" 
                              || thisInfo.expand_defline_on_set == "TRUE")
     c_item->expanded = true;
};

//new comb


void CSeqEntry_DISC_FEATURE_COUNT :: TestOnObj(const CSeq_entry& seq_entry)
{  
   typedef map <string, int > Str2Int;
   Str2Int feat_count_list;
   Str2Int :: iterator it;
   CSeq_entry_Handle seq_entry_hl = thisInfo.scope->GetSeq_entryHandle(seq_entry);
   SAnnotSelector sel;
   sel.ExcludeFeatSubtype(CSeqFeatData::eSubtype_prot);
   for (CFeat_CI feat_it(seq_entry_hl, sel); feat_it; ++feat_it) { // try non_prot_feat
      const CSeq_feat& seq_feat = (feat_it->GetOriginalFeature());
      const CSeqFeatData& seq_feat_dt = seq_feat.GetData();
      strtmp = seq_feat_dt.GetKey(CSeqFeatData::eVocabulary_genbank);
      it = feat_count_list.find(strtmp);
      if (it == feat_count_list.end())  feat_count_list[strtmp] = 1;
      else it->second ++;
   }

   ITERATE (Str2Int, it, feat_count_list) 
     thisInfo.test_item_list[GetName()+"$" + it->first].push_back(NStr::UIntToString(it->second));
};



void CSeqEntry_DISC_FEATURE_COUNT :: GetReport(CRef <CClickableItem>& c_item)
{
   size_t pos;
   unsigned cnt;
   strtmp = GetName() + "$";
   ITERATE (Str2Strs, it, thisInfo.test_item_list) {
     if ( it->first.find(strtmp) != string::npos) {
        pos = it->first.find("$");
        c_item->setting_name = GetName();
        c_item->item_list.clear();
        cnt = NStr::StringToUInt(((it->second)[0]));
        c_item->description = 
             it->first.substr(pos+1) + ": " + NStr::UIntToString(cnt) + ((cnt>1)? " present" : " presents");
        thisInfo.disc_report_data.push_back(c_item);
        c_item = CRef <CClickableItem> (new CClickableItem);
     }
   }
};


// new comb
void CSeqEntry_on_comment :: TestOnObj(const CSeq_entry& seq_entry)
{
  if (thisTest.is_Comment_run) return;

  unsigned i=0;
  string desc;
  ITERATE (vector <const CSeqdesc*>, it, comm_seqdesc) {
     desc = GetDiscItemText(**it, *(comm_seqdesc_seqentry[i])); 
     // ONCALLER_COMMENT_PRESENT
     if (m_all_same && comm_seqdesc[0]->GetComment() != (*it)->GetComment()) 
          m_all_same  = false; 
     thisInfo.test_item_list[GetName_has()].push_back(desc);

     // DISC_MISMATCHED_COMMENTS
     strtmp = (*it)->GetComment();
     strtmp = strtmp.empty() ? "empty$" : strtmp + "$";
     thisInfo.test_item_list[GetName_mix()].push_back(strtmp + desc);
  }

  thisTest.is_Comment_run = true;
};


void CSeqEntry_DISC_MISMATCHED_COMMENTS :: GetReport(CRef <CClickableItem>& c_item)
{
  Str2Strs comm2ls;
  GetTestItemList(c_item->item_list, comm2ls);
  c_item->item_list.clear();
  if (comm2ls.size() > 1) {
     ITERATE (Str2Strs, it, comm2ls) 
        AddSubcategories(c_item,GetName(), it->second, "comment", it->first,e_ContainsComment);
     c_item->description = "Mismatched comments were found";
  }
};


/*
void CSeqEntry_ONCALLER_COMMENT_PRESENT :: TestOnObj(const CSeq_entry& seq_entry)
{
   unsigned i=0;
   m_all_same = true;
   ITERATE (vector <const CSeqdesc*>, it, comm_seqdesc) {
     if (m_all_same && comm_seqdesc[0]->GetComment() != (*it)->GetComment())
          m_all_same  = false;
     thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it, *(comm_seqdesc_seqentry[i++])));
   }
};
*/


void CSeqEntry_ONCALLER_COMMENT_PRESENT :: GetReport(CRef <CClickableItem>& c_item)
{
    c_item->description = GetOtherComment(c_item->item_list.size(), 
                                          "comment descriptor was found", 
                                          "comment descriptors were found") 
      + ((c_item->item_list.size() > 1 && m_all_same)? " (all same)." : " (some different). ");
};



//bool CSeqEntry_DISC_CHECK_AUTH_CAPS :: IsNameCapitalizationOk(const string& name)
bool CSeqEntry_test_on_pub :: IsNameCapitalizationOk(const string& name)
{
  if (name.empty()) return (true);

  unsigned    len;
  bool need_cap = true, rval = true, found;
  bool needed_lower = false, found_lower = false;

  size_t pos;
  pos = 0;
  while (pos < name.size() && rval) {
    if (isalpha (name[pos])) {
      if (!need_cap) {
          needed_lower = true;
          if (!isupper(name[pos])) found_lower = true;
      }

      /* check to see if this is a short name */
      if (need_cap && !isupper (name[pos])) {
        if (!pos || name[pos-1] == ' ') {
          found = false;
          ITERATE (vector <string>, it, thisInfo.short_auth_nms) {
            len = (*it).size();
            if (name.size() > len && ( (pos = name.find(*it)) != string::npos) && name[len] == ' ') {
                 found = true;
                 pos += len -1; //in order to set need_cap correctly
                 break;
            }
          }
          if (!found) rval = false;
        } 
        else rval = false;
      }

      need_cap = false;
    } 
    else need_cap = true;
    pos++;
  }
  if (needed_lower && !found_lower) rval = false;

  return rval;

}; //IsNameCapitalizationOK


//bool CSeqEntry_DISC_CHECK_AUTH_CAPS :: IsAuthorInitialsCapitalizationOk(const string& nm_init)
bool CSeqEntry_test_on_pub :: IsAuthorInitialsCapitalizationOk(const string& nm_init)
{
   strtmp = nm_init;
   size_t pos = 0;
   while (pos != nm_init.size()) {
      if (isalpha(nm_init[pos]) && !isupper(nm_init[pos])) return false;
      else pos ++;
   }
   return true;
};  // IsAuthorInitialsCapitalizationOk



bool CSeqEntry_test_on_pub :: NameIsBad(const CRef <CAuthor>& auth_nm) 
{
   const CPerson_id& pid = auth_nm->GetName();
   if (pid.IsName()) {
       const CName_std& name_std = pid.GetName();
       if ( !IsNameCapitalizationOk(name_std.GetLast()) ) return true;
       else if ( name_std.CanGetFirst() && !IsNameCapitalizationOk(name_std.GetFirst()))
              return true;
       else if ( name_std.CanGetInitials() &&
                   !IsAuthorInitialsCapitalizationOk(name_std.GetInitials()))
               return true;
   }

   return false;
}

bool CSeqEntry_test_on_pub :: HasBadAuthorName(const CAuth_list& auths)
{
  if (auths.GetNames().IsStd()) 
    ITERATE (list <CRef <CAuthor> >, it, auths.GetNames().GetStd()) {
      if (NameIsBad(*it)) return true;
    }

  return false;
 
}; // HasBadAuthorName 

bool CSeqEntry_test_on_pub ::  AuthNoFirstLastNames(const CAuth_list& auths)
{
   if (auths.GetNames().IsStd()) {
      ITERATE (list <CRef <CAuthor> >, it, auths.GetNames().GetStd()) {
        const CPerson_id& pid = (*it)->GetName();
        if (!pid.IsName()) return true;
        const CName_std& name = pid.GetName();
        if (name.GetLast().empty()) return true;
        if (!name.CanGetFirst() || name.GetFirst().empty()) return true;
        if (!name.CanGetInitials() || name.GetInitials().empty()) return true;
      }
   }
   return true;
};

void CSeqEntry_test_on_pub :: CheckBadAuthCapsOrNoFirstLastNamesInPubdesc(const list <CRef <CPub> >& pubs, const string& desc)
{
  bool isBadCap = false, isMissing = false;
  bool has_pmid = false;
  ITERATE (list <CRef <CPub> >, it, pubs)
    if ( (*it)->IsPmid() ) has_pmid = true;
  
  ITERATE (list <CRef <CPub> >, it, pubs) {
    if ( !(*it)->IsProc() && (*it)->IsSetAuthors() ) {
      if (!isBadCap && (isBadCap = HasBadAuthorName( (*it)->GetAuthors() ) ))
          thisInfo.test_item_list[GetName_cap()].push_back(desc);
      if (!isMissing && !has_pmid && (isMissing = AuthNoFirstLastNames( (*it)->GetAuthors() )))
          thisInfo.test_item_list[GetName_missing()].push_back(desc);
      if (isBadCap && isMissing) return; 
    }
  }

}; //CheckBadAuthCapsOrNoFirstLastNamesInPubdesc

void CSeqEntry_DISC_CHECK_AUTH_NAME :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetOtherComment(c_item->item_list.size(), "pub", "pubs") 
                            + "missing author's first or last name";
};


/*

bool CSeqEntry_DISC_CHECK_AUTH_CAPS :: AreAuthCapsOkInSubmitBlock(const CSubmit_block& submit_block)
{
   return ( !(HasBadAuthorName(submit_block.GetCit().GetAuthors())) );
 
}; // AreAuthCapsOkInSubmitBlock()


void CSeqEntry_DISC_CHECK_AUTH_CAPS :: TestOnObj(const CSeq_entry& seq_entry)
{
   ITERATE (vector < const CSeq_feat* >, it, pub_feat) {
      if (AreBadAuthCapsInPubdesc((*it)->GetData().GetPub())) {
         thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(**it));
      }
   }

   unsigned i = 0;
   ITERATE (vector < const CSeqdesc* >, it, pub_seqdesc) {
      if (AreBadAuthCapsInPubdesc( (*it)->GetPub())) {
          thisInfo.test_item_list[GetName()].push_back(
                        GetDiscItemText(**it, *(pub_seqdesc_seqentry[i])));
      }
      i++;
   }

   if (thisInfo.submit_block.NotEmpty()) { 
       if ( !AreAuthCapsOkInSubmitBlock(*thisInfo.submit_block) ) {
         if (seq_entry.IsSeq())
            thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(seq_entry.GetSeq()));
         else if (seq_entry.IsSet()) {
            thisInfo.test_item_list[GetName()].push_back(GetDiscItemText(seq_entry.GetSet()));
         }
       }
   }
};
*/


void CSeqEntry_DISC_CHECK_AUTH_CAPS :: GetReport(CRef <CClickableItem>& c_item)
{
   c_item->description = GetHasComment(c_item->item_list.size(), "pub") + 
                                                      "incorrect author capitalization.";
};
