#ifndef OBJECTS_SEQFEAT___SEQFEAT_MACROS__HPP
#define OBJECTS_SEQFEAT___SEQFEAT_MACROS__HPP

/*  $Id: seqfeat_macros.hpp 400350 2013-05-20 20:59:14Z rafanovi $
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
 * Authors:  Jonathan Kans, Michael Kornbluh, Colleen Bollin
 *
 */

/// @file seqfeat_macros.hpp
/// Utility macros and typedefs for exploring NCBI objects from seqfeat.asn.


#include <objects/misc/sequence_util_macros.hpp>
#include <objects/seqfeat/seqfeat__.hpp>


/// @NAME Convenience macros for NCBI objects
/// @{


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

/////////////////////////////////////////////////////////////////////////////
/// Macros and typedefs for object subtypes
/////////////////////////////////////////////////////////////////////////////


/// CBioSource definitions

#define NCBI_GENOME(Type) CBioSource::eGenome_##Type
typedef CBioSource::TGenome TBIOSOURCE_GENOME;

//   genomic              chloroplast       chromoplast
//   kinetoplast          mitochondrion     plastid
//   macronuclear         extrachrom        plasmid
//   transposon           insertion_seq     cyanelle
//   proviral             virion            nucleomorph
//   apicoplast           leucoplast        proplastid
//   endogenous_virus     hydrogenosome     chromosome
//   chromatophore

#define NCBI_ORIGIN(Type) CBioSource::eOrigin_##Type
typedef CBioSource::TOrigin TBIOSOURCE_ORIGIN;

//   natural       natmut     mut     artificial
//   synthetic     other


/// COrgName definitions

#define NCBI_ORGNAME(Type) COrgName::e_##Type
typedef COrgName::C_Name::E_Choice TORGNAME_CHOICE;

//   Binomial     Virus     Hybrid     Namedhybrid     Partial


/// CSubSource definitions

#define NCBI_SUBSOURCE(Type) CSubSource::eSubtype_##Type
typedef CSubSource::TSubtype TSUBSOURCE_SUBTYPE;

//   chromosome                map                 clone
//   subclone                  haplotype           genotype
//   sex                       cell_line           cell_type
//   tissue_type               clone_lib           dev_stage
//   frequency                 germline            rearranged
//   lab_host                  pop_variant         tissue_lib
//   plasmid_name              transposon_name     insertion_seq_name
//   plastid_name              country             segment
//   endogenous_virus_name     transgenic          environmental_sample
//   isolation_source          lat_lon             collection_date
//   collected_by              identified_by       fwd_primer_seq
//   rev_primer_seq            fwd_primer_name     rev_primer_name
//   metagenomic               mating_type         linkage_group
//   haplogroup                other


/// COrgMod definitions

#define NCBI_ORGMOD(Type) COrgMod::eSubtype_##Type
typedef COrgMod::TSubtype TORGMOD_SUBTYPE;

//   strain                 substrain        type
//   subtype                variety          serotype
//   serogroup              serovar          cultivar
//   pathovar               chemovar         biovar
//   biotype                group            subgroup
//   isolate                common           acronym
//   dosage                 nat_host         sub_species
//   specimen_voucher       authority        forma
//   forma_specialis        ecotype          synonym
//   anamorph               teleomorph       breed
//   gb_acronym             gb_anamorph      gb_synonym
//   culture_collection     bio_material     metagenome_source
//   old_lineage            old_name         other


/// CSeq_feat definitions

#define NCBI_SEQFEAT(Type) CSeqFeatData::e_##Type
typedef CSeqFeatData::E_Choice TSEQFEAT_CHOICE;

//   Gene         Org                 Cdregion     Prot
//   Rna          Pub                 Seq          Imp
//   Region       Comment             Bond         Site
//   Rsite        User                Txinit       Num
//   Psec_str     Non_std_residue     Het          Biosrc
//   Clone


/// CProt_ref definitions

#define NCBI_PROTREF(Type) CProt_ref::eProcessed_##Type
typedef CProt_ref::EProcessed TPROTREF_PROCESSED;

//   preprotein     mature     signal_peptide     transit_peptide


/// CRNA_ref definitions

#define NCBI_RNAREF(Type) CRNA_ref::eType_##Type
typedef CRNA_ref::EType TRNAREF_TYPE;

//   premsg     mRNA      tRNA      rRNA        snRNA     scRNA
//   snoRNA     ncRNA     tmRNA     miscRNA     other

#define NCBI_RNAEXT(Type) CRNA_ref::C_Ext::e_##Type
typedef CRNA_ref::C_Ext::E_Choice TRNAREF_EXT;

//   Name     TRNA      Gen


/// CCdregion definitions

#define NCBI_CDSFRAME(Type) CCdregion::eFrame_##Type
typedef CCdregion::EFrame TCDSFRAME_TYPE;

//   one       two     three



// "FOR_EACH_XXX_ON_YYY" does a linear const traversal of STL containers
// "EDIT_EACH_XXX_ON_YYY" does a linear non-const traversal of STL containers

// "SWITCH_ON_XXX_CHOICE" switches on the item subtype

// "ADD_XXX_TO_YYY" adds an element to a specified object
// "ERASE_XXX_ON_YYY" deletes a specified object within an iterator

// Miscellaneous macros for testing objects include
// "XXX_IS_YYY" or "XXX_HAS_YYY"
// "XXX_CHOICE_IS"


////
//// FEATID_ON_SEQFEAT macros
//// ( Warning: features also have an "Id" field (deprecated?) )

#define FEATID_ON_BIOSEQ_Type      CSeq_feat::TIds:
#define FEATID_ON_BIOSEQ_Test(Var) (Var).IsSetIds()
#define FEATID_ON_BIOSEQ_Get(Var)  (Var).GetIds()
#define FEATID_ON_BIOSEQ_Set(Var)  (Var).SetIds()

#define EDIT_EACH_FEATID_ON_SEQFEAT( Iter, Var ) \
    EDIT_EACH( FEATID_ON_BIOSEQ, Iter, Var )


///
/// CBioSource macros

/// BIOSOURCE_GENOME macros

#define BIOSOURCE_GENOME_Test(Var) (Var).IsSetGenome()
#define BIOSOURCE_GENOME_Chs(Var)  (Var).GetGenome()

/// BIOSOURCE_GENOME_IS

#define BIOSOURCE_GENOME_IS(Var, Chs) \
CHOICE_IS (BIOSOURCE_GENOME, Var, Chs)

/// SWITCH_ON_BIOSOURCE_GENOME

#define SWITCH_ON_BIOSOURCE_GENOME(Var) \
SWITCH_ON (BIOSOURCE_GENOME, Var)


/// BIOSOURCE_ORIGIN macros

#define BIOSOURCE_ORIGIN_Test(Var) (Var).IsSetOrigin()
#define BIOSOURCE_ORIGIN_Chs(Var)  (Var).GetOrigin()

/// BIOSOURCE_ORIGIN_IS

#define BIOSOURCE_ORIGIN_IS(Var, Chs) \
CHOICE_IS (BIOSOURCE_ORIGIN, Var, Chs)

/// SWITCH_ON_BIOSOURCE_ORIGIN

#define SWITCH_ON_BIOSOURCE_ORIGIN(Var) \
SWITCH_ON (BIOSOURCE_ORIGIN, Var)


/// ORGREF_ON_BIOSOURCE macros

#define ORGREF_ON_BIOSOURCE_Test(Var) (Var).IsSetOrg()

/// BIOSOURCE_HAS_ORGREF

#define BIOSOURCE_HAS_ORGREF(Var) \
ITEM_HAS (ORGREF_ON_BIOSOURCE, Var)


/// ORGNAME_ON_BIOSOURCE macros

#define ORGNAME_ON_BIOSOURCE_Test(Var) (Var).IsSetOrgname()

/// BIOSOURCE_HAS_ORGNAME

#define BIOSOURCE_HAS_ORGNAME(Var) \
ITEM_HAS (ORGNAME_ON_BIOSOURCE, Var)


/// SUBSOURCE_ON_BIOSOURCE macros

#define SUBSOURCE_ON_BIOSOURCE_Type       CBioSource::TSubtype
#define SUBSOURCE_ON_BIOSOURCE_Test(Var)  (Var).IsSetSubtype()
#define SUBSOURCE_ON_BIOSOURCE_Get(Var)   (Var).GetSubtype()
#define SUBSOURCE_ON_BIOSOURCE_Set(Var)   (Var).SetSubtype()
#define SUBSOURCE_ON_BIOSOURCE_Reset(Var) (Var).ResetSubtype()

/// BIOSOURCE_HAS_SUBSOURCE

#define BIOSOURCE_HAS_SUBSOURCE(Var) \
ITEM_HAS (SUBSOURCE_ON_BIOSOURCE, Var)

/// FOR_EACH_SUBSOURCE_ON_BIOSOURCE
/// EDIT_EACH_SUBSOURCE_ON_BIOSOURCE
// CBioSource& as input, dereference with [const] CSubSource& sbs = **itr

#define FOR_EACH_SUBSOURCE_ON_BIOSOURCE(Itr, Var) \
FOR_EACH (SUBSOURCE_ON_BIOSOURCE, Itr, Var)

#define EDIT_EACH_SUBSOURCE_ON_BIOSOURCE(Itr, Var) \
EDIT_EACH (SUBSOURCE_ON_BIOSOURCE, Itr, Var)

/// ADD_SUBSOURCE_TO_BIOSOURCE

#define ADD_SUBSOURCE_TO_BIOSOURCE(Var, Ref) \
ADD_ITEM (SUBSOURCE_ON_BIOSOURCE, Var, Ref)

/// ERASE_SUBSOURCE_ON_BIOSOURCE

#define ERASE_SUBSOURCE_ON_BIOSOURCE(Itr, Var) \
LIST_ERASE_ITEM (SUBSOURCE_ON_BIOSOURCE, Itr, Var)

/// SUBSOURCE_ON_BIOSOURCE_IS_SORTED

#define SUBSOURCE_ON_BIOSOURCE_IS_SORTED(Var, Func) \
IS_SORTED (SUBSOURCE_ON_BIOSOURCE, Var, Func)

/// SORT_SUBSOURCE_ON_BIOSOURCE

#define SORT_SUBSOURCE_ON_BIOSOURCE(Var, Func) \
DO_LIST_SORT (SUBSOURCE_ON_BIOSOURCE, Var, Func)

/// SUBSOURCE_ON_BIOSOURCE_IS_UNIQUE

#define SUBSOURCE_ON_BIOSOURCE_IS_UNIQUE(Var, Func) \
IS_UNIQUE (SUBSOURCE_ON_BIOSOURCE, Var, Func)

/// UNIQUE_SUBSOURCE_ON_BIOSOURCE

#define UNIQUE_SUBSOURCE_ON_BIOSOURCE(Var, Func) \
DO_UNIQUE (SUBSOURCE_ON_BIOSOURCE, Var, Func)

/// REMOVE_IF_EMPTY_SUBSOURCE_ON_BIOSOURCE

#define REMOVE_IF_EMPTY_SUBSOURCE_ON_BIOSOURCE(Var) \
    REMOVE_IF_EMPTY_FIELD(SUBSOURCE_ON_BIOSOURCE, Var)

#define SUBSOURCE_ON_BIOSOURCE_IS_EMPTY(Var) \
    FIELD_IS_EMPTY( SUBSOURCE_ON_BIOSOURCE, Var )

/// ORGMOD_ON_BIOSOURCE macros

#define ORGMOD_ON_BIOSOURCE_Type      COrgName::TMod
#define ORGMOD_ON_BIOSOURCE_Test(Var) (Var).IsSetOrgMod()
#define ORGMOD_ON_BIOSOURCE_Get(Var)  (Var).GetOrgname().GetMod()
#define ORGMOD_ON_BIOSOURCE_Set(Var)  (Var).SetOrg().SetOrgname().SetMod()

/// BIOSOURCE_HAS_ORGMOD

#define BIOSOURCE_HAS_ORGMOD(Var) \
ITEM_HAS (ORGMOD_ON_BIOSOURCE, Var)

/// FOR_EACH_ORGMOD_ON_BIOSOURCE
/// EDIT_EACH_ORGMOD_ON_BIOSOURCE
// CBioSource& as input, dereference with [const] COrgMod& omd = **itr

#define FOR_EACH_ORGMOD_ON_BIOSOURCE(Itr, Var) \
FOR_EACH (ORGMOD_ON_BIOSOURCE, Itr, Var)

#define EDIT_EACH_ORGMOD_ON_BIOSOURCE(Itr, Var) \
EDIT_EACH (ORGMOD_ON_BIOSOURCE, Itr, Var)

/// ADD_ORGMOD_TO_BIOSOURCE

#define ADD_ORGMOD_TO_BIOSOURCE(Var, Ref) \
ADD_ITEM (ORGMOD_ON_BIOSOURCE, Var, Ref)

/// ERASE_ORGMOD_ON_BIOSOURCE

#define ERASE_ORGMOD_ON_BIOSOURCE(Itr, Var) \
LIST_ERASE_ITEM (ORGMOD_ON_BIOSOURCE, Itr, Var)

/// ORGMOD_ON_BIOSOURCE_IS_SORTED

#define ORGMOD_ON_BIOSOURCE_IS_SORTED(Var, Func) \
IS_SORTED (ORGMOD_ON_BIOSOURCE, Var, Func)

/// SORT_ORGMOD_ON_BIOSOURCE

#define SORT_ORGMOD_ON_BIOSOURCE(Var, Func) \
DO_LIST_SORT (ORGMOD_ON_BIOSOURCE, Var, Func)

/// ORGMOD_ON_BIOSOURCE_IS_UNIQUE

#define ORGMOD_ON_BIOSOURCE_IS_UNIQUE(Var, Func) \
IS_UNIQUE (ORGMOD_ON_BIOSOURCE, Var, Func)

/// UNIQUE_ORGMOD_ON_BIOSOURCE

#define UNIQUE_ORGMOD_ON_BIOSOURCE(Var, Func) \
DO_UNIQUE (ORGMOD_ON_BIOSOURCE, Var, Func)


///
/// COrg_ref macros

/// ORGMOD_ON_ORGREF macros

#define ORGMOD_ON_ORGREF_Type      COrgName::TMod
#define ORGMOD_ON_ORGREF_Test(Var) (Var).IsSetOrgMod()
#define ORGMOD_ON_ORGREF_Get(Var)  (Var).GetOrgname().GetMod()
#define ORGMOD_ON_ORGREF_Set(Var)  (Var).SetOrgname().SetMod()

/// ORGREF_HAS_ORGMOD

#define ORGREF_HAS_ORGMOD(Var) \
ITEM_HAS (ORGMOD_ON_ORGREF, Var)

/// FOR_EACH_ORGMOD_ON_ORGREF
/// EDIT_EACH_ORGMOD_ON_ORGREF
// COrg_ref& as input, dereference with [const] COrgMod& omd = **itr

#define FOR_EACH_ORGMOD_ON_ORGREF(Itr, Var) \
FOR_EACH (ORGMOD_ON_ORGREF, Itr, Var)

#define EDIT_EACH_ORGMOD_ON_ORGREF(Itr, Var) \
EDIT_EACH (ORGMOD_ON_ORGREF, Itr, Var)

/// ADD_ORGMOD_TO_ORGREF

#define ADD_ORGMOD_TO_ORGREF(Var, Ref) \
ADD_ITEM (ORGMOD_ON_ORGREF, Var, Ref)

/// ERASE_ORGMOD_ON_ORGREF

#define ERASE_ORGMOD_ON_ORGREF(Itr, Var) \
LIST_ERASE_ITEM (ORGMOD_ON_ORGREF, Itr, Var)

/// ORGMOD_ON_ORGREF_IS_SORTED

#define ORGMOD_ON_ORGREF_IS_SORTED(Var, Func) \
IS_SORTED (ORGMOD_ON_ORGREF, Var, Func)

/// SORT_ORGMOD_ON_ORGREF

#define SORT_ORGMOD_ON_ORGREF(Var, Func) \
DO_LIST_SORT (ORGMOD_ON_ORGREF, Var, Func)

/// ORGMOD_ON_ORGREF_IS_UNIQUE

#define ORGMOD_ON_ORGREF_IS_UNIQUE(Var, Func) \
IS_UNIQUE (ORGMOD_ON_ORGREF, Var, Func)

/// UNIQUE_ORGMOD_ON_ORGREF

#define UNIQUE_ORGMOD_ON_ORGREF(Var, Func) \
DO_UNIQUE (ORGMOD_ON_ORGREF, Var, Func)


/// DBXREF_ON_ORGREF macros

#define DBXREF_ON_ORGREF_Type      COrg_ref::TDb
#define DBXREF_ON_ORGREF_Test(Var) (Var).IsSetDb()
#define DBXREF_ON_ORGREF_Get(Var)  (Var).GetDb()
#define DBXREF_ON_ORGREF_Set(Var)  (Var).SetDb()

/// ORGREF_HAS_DBXREF

#define ORGREF_HAS_DBXREF(Var) \
ITEM_HAS (DBXREF_ON_ORGREF, Var)

/// FOR_EACH_DBXREF_ON_ORGREF
/// EDIT_EACH_DBXREF_ON_ORGREF
// COrg_ref& as input, dereference with [const] CDbtag& dbt = **itr

#define FOR_EACH_DBXREF_ON_ORGREF(Itr, Var) \
FOR_EACH (DBXREF_ON_ORGREF, Itr, Var)

#define EDIT_EACH_DBXREF_ON_ORGREF(Itr, Var) \
EDIT_EACH (DBXREF_ON_ORGREF, Itr, Var)

/// ADD_DBXREF_TO_ORGREF

#define ADD_DBXREF_TO_ORGREF(Var, Ref) \
ADD_ITEM (DBXREF_ON_ORGREF, Var, Ref)

/// ERASE_DBXREF_ON_ORGREF

#define ERASE_DBXREF_ON_ORGREF(Itr, Var) \
VECTOR_ERASE_ITEM (DBXREF_ON_ORGREF, Itr, Var)

/// DBXREF_ON_ORGREF_IS_SORTED

#define DBXREF_ON_ORGREF_IS_SORTED(Var, Func) \
IS_SORTED (DBXREF_ON_ORGREF, Var, Func)

/// SORT_DBXREF_ON_ORGREF

#define SORT_DBXREF_ON_ORGREF(Var, Func) \
DO_VECTOR_SORT (DBXREF_ON_ORGREF, Var, Func)

/// DBXREF_ON_ORGREF_IS_UNIQUE

#define DBXREF_ON_ORGREF_IS_UNIQUE(Var, Func) \
IS_UNIQUE (DBXREF_ON_ORGREF, Var, Func)

/// UNIQUE_DBXREF_ON_ORGREF

#define UNIQUE_DBXREF_ON_ORGREF(Var, Func) \
DO_UNIQUE (DBXREF_ON_ORGREF, Var, Func)


/// MOD_ON_ORGREF macros

#define MOD_ON_ORGREF_Type      COrg_ref::TMod
#define MOD_ON_ORGREF_Test(Var) (Var).IsSetMod()
#define MOD_ON_ORGREF_Get(Var)  (Var).GetMod()
#define MOD_ON_ORGREF_Set(Var)  (Var).SetMod()

/// ORGREF_HAS_MOD

#define ORGREF_HAS_MOD(Var) \
ITEM_HAS (MOD_ON_ORGREF, Var)

/// FOR_EACH_MOD_ON_ORGREF
/// EDIT_EACH_MOD_ON_ORGREF
// COrg_ref& as input, dereference with [const] string& str = *itr

#define FOR_EACH_MOD_ON_ORGREF(Itr, Var) \
FOR_EACH (MOD_ON_ORGREF, Itr, Var)

#define EDIT_EACH_MOD_ON_ORGREF(Itr, Var) \
EDIT_EACH (MOD_ON_ORGREF, Itr, Var)

/// ERASE_MOD_ON_ORGREF

#define ERASE_MOD_ON_ORGREF(Itr, Var) \
LIST_ERASE_ITEM (MOD_ON_ORGREF, Itr, Var)

/// MOD_ON_ORGREF_IS_EMPTY

#define MOD_ON_ORGREF_IS_EMPTY(Var) \
FIELD_IS_EMPTY( MOD_ON_ORGREF, Var )


/// SYN_ON_ORGREF macros

#define SYN_ON_ORGREF_Type      COrg_ref::TSyn
#define SYN_ON_ORGREF_Test(Var) (Var).IsSetSyn()
#define SYN_ON_ORGREF_Get(Var)  (Var).GetSyn()
#define SYN_ON_ORGREF_Set(Var)  (Var).SetSyn()

/// ORGREF_HAS_SYN

#define ORGREF_HAS_SYN(Var) \
ITEM_HAS (SYN_ON_ORGREF, Var)

/// FOR_EACH_SYN_ON_ORGREF
/// EDIT_EACH_SYN_ON_ORGREF
// COrg_ref& as input, dereference with [const] string& str = *itr

#define FOR_EACH_SYN_ON_ORGREF(Itr, Var) \
FOR_EACH (SYN_ON_ORGREF, Itr, Var)

#define EDIT_EACH_SYN_ON_ORGREF(Itr, Var) \
EDIT_EACH (SYN_ON_ORGREF, Itr, Var)

/// ERASE_SYN_ON_ORGREF

#define ERASE_SYN_ON_ORGREF(Itr, Var) \
LIST_ERASE_ITEM (SYN_ON_ORGREF, Itr, Var)

/// SYN_ON_ORGREF_IS_SORTED

#define SYN_ON_ORGREF_IS_SORTED(Var, Func) \
IS_SORTED (SYN_ON_ORGREF, Var, Func)

/// SORT_SYN_ON_ORGREF

#define SORT_SYN_ON_ORGREF(Var, Func) \
DO_LIST_SORT (SYN_ON_ORGREF, Var, Func)

/// SYN_ON_ORGREF_IS_UNIQUE

#define SYN_ON_ORGREF_IS_UNIQUE(Var, Func) \
IS_UNIQUE (SYN_ON_ORGREF, Var, Func)

/// UNIQUE_SYN_ON_ORGREF

#define UNIQUE_SYN_ON_ORGREF(Var, Func) \
DO_UNIQUE (SYN_ON_ORGREF, Var, Func)

///
/// COrgName macros

/// ORGNAME_CHOICE macros

#define ORGNAME_CHOICE_Test(Var) (Var).IsSetName() && \
                                     (Var).GetName().Which() != COrgName::e_not_set
#define ORGNAME_CHOICE_Chs(Var)  (Var).GetName().Which()

/// ORGNAME_CHOICE_IS

#define ORGNAME_CHOICE_IS(Var, Chs) \
CHOICE_IS (ORGNAME_CHOICE, Var, Chs)

/// SWITCH_ON_ORGNAME_CHOICE

#define SWITCH_ON_ORGNAME_CHOICE(Var) \
SWITCH_ON (ORGNAME_CHOICE, Var)


/// ORGMOD_ON_ORGNAME macros

#define ORGMOD_ON_ORGNAME_Type       COrgName::TMod
#define ORGMOD_ON_ORGNAME_Test(Var)  (Var).IsSetMod()
#define ORGMOD_ON_ORGNAME_Get(Var)   (Var).GetMod()
#define ORGMOD_ON_ORGNAME_Set(Var)   (Var).SetMod()
#define ORGMOD_ON_ORGNAME_Reset(Var) (Var).ResetMod()

/// ORGNAME_HAS_ORGMOD

#define ORGNAME_HAS_ORGMOD(Var) \
ITEM_HAS (ORGMOD_ON_ORGNAME, Var)

/// FOR_EACH_ORGMOD_ON_ORGNAME
/// EDIT_EACH_ORGMOD_ON_ORGNAME
// COrgName& as input, dereference with [const] COrgMod& omd = **itr

#define FOR_EACH_ORGMOD_ON_ORGNAME(Itr, Var) \
FOR_EACH (ORGMOD_ON_ORGNAME, Itr, Var)

#define EDIT_EACH_ORGMOD_ON_ORGNAME(Itr, Var) \
EDIT_EACH (ORGMOD_ON_ORGNAME, Itr, Var)

/// ADD_ORGMOD_TO_ORGNAME

#define ADD_ORGMOD_TO_ORGNAME(Var, Ref) \
ADD_ITEM (ORGMOD_ON_ORGNAME, Var, Ref)

/// ERASE_ORGMOD_ON_ORGNAME

#define ERASE_ORGMOD_ON_ORGNAME(Itr, Var) \
LIST_ERASE_ITEM (ORGMOD_ON_ORGNAME, Itr, Var)

/// ORGMOD_ON_ORGNAME_IS_SORTED

#define ORGMOD_ON_ORGNAME_IS_SORTED(Var, Func) \
IS_SORTED (ORGMOD_ON_ORGNAME, Var, Func)

/// SORT_ORGMOD_ON_ORGNAME

#define SORT_ORGMOD_ON_ORGNAME(Var, Func) \
DO_LIST_SORT (ORGMOD_ON_ORGNAME, Var, Func)

/// ORGMOD_ON_ORGNAME_IS_UNIQUE

#define ORGMOD_ON_ORGNAME_IS_UNIQUE(Var, Func) \
IS_UNIQUE (ORGMOD_ON_ORGNAME, Var, Func)

/// UNIQUE_ORGMOD_ON_ORGNAME

#define UNIQUE_ORGMOD_ON_ORGNAME(Var, Func) \
DO_UNIQUE (ORGMOD_ON_ORGNAME, Var, Func)

#define REMOVE_IF_EMPTY_ORGMOD_ON_ORGNAME(Var) \
REMOVE_IF_EMPTY_FIELD(ORGMOD_ON_ORGNAME, Var)

///
/// CSubSource macros

/// SUBSOURCE_CHOICE macros

#define SUBSOURCE_CHOICE_Test(Var) (Var).IsSetSubtype()
#define SUBSOURCE_CHOICE_Chs(Var)  (Var).GetSubtype()

/// SUBSOURCE_CHOICE_IS

#define SUBSOURCE_CHOICE_IS(Var, Chs) \
CHOICE_IS (SUBSOURCE_CHOICE, Var, Chs)

/// SWITCH_ON_SUBSOURCE_CHOICE

#define SWITCH_ON_SUBSOURCE_CHOICE(Var) \
SWITCH_ON (SUBSOURCE_CHOICE, Var)


///
/// COrgMod macros

/// ORGMOD_CHOICE macros

#define ORGMOD_CHOICE_Test(Var) (Var).IsSetSubtype()
#define ORGMOD_CHOICE_Chs(Var)  (Var).GetSubtype()

/// ORGMOD_CHOICE_IS

#define ORGMOD_CHOICE_IS(Var, Chs) \
CHOICE_IS (ORGMOD_CHOICE, Var, Chs)

/// SWITCH_ON_ORGMOD_CHOICE

#define SWITCH_ON_ORGMOD_CHOICE(Var) \
SWITCH_ON (ORGMOD_CHOICE, Var)

/// ATTRIB_ON_ORGMOD macros

#define ATTRIB_ON_ORGMOD_Test(Var) (Var).IsSetAttrib()
#define ATTRIB_ON_ORGMOD_Get(Var)  (Var).GetAttrib()

/// 
#define GET_ATTRIB_OR_BLANK(Var) \
    GET_STRING_OR_BLANK( ATTRIB_ON_ORGMOD, Var )

///
/// CSeq_feat macros

/// SEQFEAT_CHOICE macros

#define SEQFEAT_CHOICE_Test(Var) (Var).IsSetData()
#define SEQFEAT_CHOICE_Chs(Var)  (Var).GetData().Which()

/// SEQFEAT_CHOICE_IS

#define SEQFEAT_CHOICE_IS(Var, Chs) \
CHOICE_IS (SEQFEAT_CHOICE, Var, Chs)

/// SWITCH_ON_SEQFEAT_CHOICE

#define SWITCH_ON_SEQFEAT_CHOICE(Var) \
SWITCH_ON (SEQFEAT_CHOICE, Var)

/// FEATURE_CHOICE_IS
/// SWITCH_ON_FEATURE_CHOICE

#define FEATURE_CHOICE_IS SEQFEAT_CHOICE_IS
#define SWITCH_ON_FEATURE_CHOICE SWITCH_ON_SEQFEAT_CHOICE


/// GBQUAL_ON_SEQFEAT macros

#define GBQUAL_ON_SEQFEAT_Type       CSeq_feat::TQual
#define GBQUAL_ON_SEQFEAT_Test(Var)  (Var).IsSetQual()
#define GBQUAL_ON_SEQFEAT_Get(Var)   (Var).GetQual()
#define GBQUAL_ON_SEQFEAT_Set(Var)   (Var).SetQual()
#define GBQUAL_ON_SEQFEAT_Reset(Var) (Var).ResetQual()

/// SEQFEAT_HAS_GBQUAL

#define SEQFEAT_HAS_GBQUAL(Var) \
ITEM_HAS (GBQUAL_ON_SEQFEAT, Var)

/// FOR_EACH_GBQUAL_ON_SEQFEAT
/// EDIT_EACH_GBQUAL_ON_SEQFEAT
// CSeq_feat& as input, dereference with [const] CGb_qual& gbq = **itr;

#define FOR_EACH_GBQUAL_ON_SEQFEAT(Itr, Var) \
FOR_EACH (GBQUAL_ON_SEQFEAT, Itr, Var)

#define EDIT_EACH_GBQUAL_ON_SEQFEAT(Itr, Var) \
EDIT_EACH (GBQUAL_ON_SEQFEAT, Itr, Var)

/// ADD_GBQUAL_TO_SEQFEAT

#define ADD_GBQUAL_TO_SEQFEAT(Var, Ref) \
ADD_ITEM (GBQUAL_ON_SEQFEAT, Var, Ref)

/// ERASE_GBQUAL_ON_SEQFEAT

#define ERASE_GBQUAL_ON_SEQFEAT(Itr, Var) \
VECTOR_ERASE_ITEM (GBQUAL_ON_SEQFEAT, Itr, Var)

/// GBQUAL_ON_SEQFEAT_IS_SORTED

#define GBQUAL_ON_SEQFEAT_IS_SORTED(Var, Func) \
IS_SORTED (GBQUAL_ON_SEQFEAT, Var, Func)

/// SORT_GBQUAL_ON_SEQFEAT

#define SORT_GBQUAL_ON_SEQFEAT(Var, Func) \
DO_VECTOR_SORT (GBQUAL_ON_SEQFEAT, Var, Func)

/// GBQUAL_ON_SEQFEAT_IS_UNIQUE

#define GBQUAL_ON_SEQFEAT_IS_UNIQUE(Var, Func) \
IS_UNIQUE (GBQUAL_ON_SEQFEAT, Var, Func)

/// UNIQUE_GBQUAL_ON_SEQFEAT

#define UNIQUE_GBQUAL_ON_SEQFEAT(Var, Func) \
DO_UNIQUE (GBQUAL_ON_SEQFEAT, Var, Func)

/// RESET_GBQUAL_ON_SEQFEAT_IF_EMPTY
#define REMOVE_IF_EMPTY_GBQUAL_ON_SEQFEAT(Var) \
    REMOVE_IF_EMPTY_FIELD(GBQUAL_ON_SEQFEAT, Var)

/// FEATURE_HAS_GBQUAL
/// FOR_EACH_GBQUAL_ON_FEATURE
/// EDIT_EACH_GBQUAL_ON_FEATURE
/// ADD_GBQUAL_TO_FEATURE
/// ERASE_GBQUAL_ON_FEATURE
/// GBQUAL_ON_FEATURE_IS_SORTED
/// SORT_GBQUAL_ON_FEATURE
/// GBQUAL_ON_FEATURE_IS_UNIQUE
/// UNIQUE_GBQUAL_ON_FEATURE

#define FEATURE_HAS_GBQUAL SEQFEAT_HAS_GBQUAL
#define FOR_EACH_GBQUAL_ON_FEATURE FOR_EACH_GBQUAL_ON_SEQFEAT
#define EDIT_EACH_GBQUAL_ON_FEATURE EDIT_EACH_GBQUAL_ON_SEQFEAT
#define ADD_GBQUAL_TO_FEATURE ADD_GBQUAL_TO_SEQFEAT
#define ERASE_GBQUAL_ON_FEATURE ERASE_GBQUAL_ON_SEQFEAT
#define GBQUAL_ON_FEATURE_IS_SORTED GBQUAL_ON_SEQFEAT_IS_SORTED
#define SORT_GBQUAL_ON_FEATURE SORT_GBQUAL_ON_SEQFEAT
#define GBQUAL_ON_FEATURE_IS_UNIQUE GBQUAL_ON_SEQFEAT_IS_UNIQUE
#define UNIQUE_GBQUAL_ON_FEATURE UNIQUE_GBQUAL_ON_SEQFEAT


/// SEQFEATXREF_ON_SEQFEAT macros

#define SEQFEATXREF_ON_SEQFEAT_Type       CSeq_feat::TXref
#define SEQFEATXREF_ON_SEQFEAT_Test(Var)  (Var).IsSetXref()
#define SEQFEATXREF_ON_SEQFEAT_Get(Var)   (Var).GetXref()
#define SEQFEATXREF_ON_SEQFEAT_Set(Var)   (Var).SetXref()
#define SEQFEATXREF_ON_SEQFEAT_Reset(Var) (Var).ResetXref()

/// SEQFEAT_HAS_SEQFEATXREF

#define SEQFEAT_HAS_SEQFEATXREF(Var) \
ITEM_HAS (SEQFEATXREF_ON_SEQFEAT, Var)

/// FOR_EACH_SEQFEATXREF_ON_SEQFEAT
/// EDIT_EACH_SEQFEATXREF_ON_SEQFEAT
// CSeq_feat& as input, dereference with [const] CSeqFeatXref& sfx = **itr;

#define FOR_EACH_SEQFEATXREF_ON_SEQFEAT(Itr, Var) \
FOR_EACH (SEQFEATXREF_ON_SEQFEAT, Itr, Var)

#define EDIT_EACH_SEQFEATXREF_ON_SEQFEAT(Itr, Var) \
EDIT_EACH (SEQFEATXREF_ON_SEQFEAT, Itr, Var)

/// ADD_SEQFEATXREF_TO_SEQFEAT

#define ADD_SEQFEATXREF_TO_SEQFEAT(Var, Ref) \
ADD_ITEM (SEQFEATXREF_ON_SEQFEAT, Var, Ref)

/// ERASE_SEQFEATXREF_ON_SEQFEAT

#define ERASE_SEQFEATXREF_ON_SEQFEAT(Itr, Var) \
VECTOR_ERASE_ITEM (SEQFEATXREF_ON_SEQFEAT, Itr, Var)

/// SEQFEATXREF_ON_SEQFEAT_IS_SORTED

#define SEQFEATXREF_ON_SEQFEAT_IS_SORTED(Var, Func) \
IS_SORTED (SEQFEATXREF_ON_SEQFEAT, Var, Func)

/// SORT_SEQFEATXREF_ON_SEQFEAT

#define SORT_SEQFEATXREF_ON_SEQFEAT(Var, Func) \
DO_VECTOR_SORT (SEQFEATXREF_ON_SEQFEAT, Var, Func)

/// SEQFEATXREF_ON_SEQFEAT_IS_UNIQUE

#define SEQFEATXREF_ON_SEQFEAT_IS_UNIQUE(Var, Func) \
IS_UNIQUE (SEQFEATXREF_ON_SEQFEAT, Var, Func)

/// UNIQUE_SEQFEATXREF_ON_SEQFEAT

#define UNIQUE_SEQFEATXREF_ON_SEQFEAT(Var, Func) \
DO_UNIQUE (SEQFEATXREF_ON_SEQFEAT, Var, Func)

/// REMOVE_IF_EMPTY_GBQUAL_ON_SEQFEAT

#define REMOVE_IF_EMPTY_SEQFEATXREF_ON_SEQFEAT(Var) \
    REMOVE_IF_EMPTY_FIELD(SEQFEATXREF_ON_SEQFEAT, Var)

/// FEATURE_HAS_SEQFEATXREF
/// FOR_EACH_SEQFEATXREF_ON_FEATURE
/// EDIT_EACH_SEQFEATXREF_ON_FEATURE
/// ADD_SEQFEATXREF_TO_FEATURE
/// ERASE_SEQFEATXREF_ON_FEATURE
/// SEQFEATXREF_ON_FEATURE_IS_SORTED
/// SORT_SEQFEATXREF_ON_FEATURE
/// SEQFEATXREF_ON_FEATURE_IS_UNIQUE
/// UNIQUE_SEQFEATXREF_ON_FEATURE

#define FEATURE_HAS_SEQFEATXREF SEQFEAT_HAS_SEQFEATXREF
#define FOR_EACH_SEQFEATXREF_ON_FEATURE FOR_EACH_SEQFEATXREF_ON_SEQFEAT
#define EDIT_EACH_SEQFEATXREF_ON_FEATURE EDIT_EACH_SEQFEATXREF_ON_SEQFEAT
#define ADD_SEQFEATXREF_TO_FEATURE ADD_SEQFEATXREF_TO_SEQFEAT
#define ERASE_SEQFEATXREF_ON_FEATURE ERASE_SEQFEATXREF_ON_SEQFEAT
#define SEQFEATXREF_ON_FEATURE_IS_SORTED SEQFEATXREF_ON_SEQFEAT_IS_SORTED
#define SORT_SEQFEATXREF_ON_FEATURE SORT_SEQFEATXREF_ON_SEQFEAT
#define SEQFEATXREF_ON_FEATURE_IS_UNIQUE SEQFEATXREF_ON_SEQFEAT_IS_UNIQUE
#define UNIQUE_SEQFEATXREF_ON_FEATURE UNIQUE_SEQFEATXREF_ON_SEQFEAT

/// XREF_ON_SEQFEAT macros

#define XREF_ON_SEQFEAT_Type       CSeq_feat::TXref
#define XREF_ON_SEQFEAT_Test(Var)  (Var).IsSetXref()
#define XREF_ON_SEQFEAT_Get(Var)   (Var).GetXref()
#define XREF_ON_SEQFEAT_Set(Var)   (Var).SetXref()
#define XREF_ON_SEQFEAT_Reset(Var) (Var).ResetXref()

/// SEQFEAT_HAS_XREF

#define SEQFEAT_HAS_XREF(Var) \
ITEM_HAS (XREF_ON_SEQFEAT, Var)

/// FOR_EACH_XREF_ON_SEQFEAT
/// EDIT_EACH_XREF_ON_SEQFEAT
// CSeq_feat& as input, dereference with [const] CDbtag& dbt = **itr;

#define FOR_EACH_XREF_ON_SEQFEAT(Itr, Var) \
FOR_EACH (XREF_ON_SEQFEAT, Itr, Var)

#define EDIT_EACH_XREF_ON_SEQFEAT(Itr, Var) \
EDIT_EACH (XREF_ON_SEQFEAT, Itr, Var)

/// ADD_XREF_TO_SEQFEAT

#define ADD_XREF_TO_SEQFEAT(Var, Ref) \
ADD_ITEM (XREF_ON_SEQFEAT, Var, Ref)

/// ERASE_XREF_ON_SEQFEAT

#define ERASE_XREF_ON_SEQFEAT(Itr, Var) \
VECTOR_ERASE_ITEM (XREF_ON_SEQFEAT, Itr, Var)

/// XREF_ON_SEQFEAT_IS_SORTED

#define XREF_ON_SEQFEAT_IS_SORTED(Var, Func) \
IS_SORTED (XREF_ON_SEQFEAT, Var, Func)

/// SORT_XREF_ON_SEQFEAT

#define SORT_XREF_ON_SEQFEAT(Var, Func) \
DO_VECTOR_SORT (XREF_ON_SEQFEAT, Var, Func)

/// XREF_ON_SEQFEAT_IS_UNIQUE

#define XREF_ON_SEQFEAT_IS_UNIQUE(Var, Func) \
IS_UNIQUE (XREF_ON_SEQFEAT, Var, Func)

/// UNIQUE_XREF_ON_SEQFEAT

#define UNIQUE_XREF_ON_SEQFEAT(Var, Func) \
DO_UNIQUE (XREF_ON_SEQFEAT, Var, Func)

/// REMOVE_IF_EMPTY_XREF_ON_SEQFEAT

#define REMOVE_IF_EMPTY_XREF_ON_SEQFEAT(Var) \
REMOVE_IF_EMPTY_FIELD(XREF_ON_SEQFEAT, Var)

/// DBXREF_ON_SEQFEAT macros

#define DBXREF_ON_SEQFEAT_Type       CSeq_feat::TDbxref
#define DBXREF_ON_SEQFEAT_Test(Var)  (Var).IsSetDbxref()
#define DBXREF_ON_SEQFEAT_Get(Var)   (Var).GetDbxref()
#define DBXREF_ON_SEQFEAT_Set(Var)   (Var).SetDbxref()
#define DBXREF_ON_SEQFEAT_Reset(Var) (Var).ResetDbxref()

/// SEQFEAT_HAS_DBXREF

#define SEQFEAT_HAS_DBXREF(Var) \
ITEM_HAS (DBXREF_ON_SEQFEAT, Var)

/// FOR_EACH_DBXREF_ON_SEQFEAT
/// EDIT_EACH_DBXREF_ON_SEQFEAT
// CSeq_feat& as input, dereference with [const] CDbtag& dbt = **itr;

#define FOR_EACH_DBXREF_ON_SEQFEAT(Itr, Var) \
FOR_EACH (DBXREF_ON_SEQFEAT, Itr, Var)

#define EDIT_EACH_DBXREF_ON_SEQFEAT(Itr, Var) \
EDIT_EACH (DBXREF_ON_SEQFEAT, Itr, Var)

/// ADD_DBXREF_TO_SEQFEAT

#define ADD_DBXREF_TO_SEQFEAT(Var, Ref) \
ADD_ITEM (DBXREF_ON_SEQFEAT, Var, Ref)

/// ERASE_DBXREF_ON_SEQFEAT

#define ERASE_DBXREF_ON_SEQFEAT(Itr, Var) \
VECTOR_ERASE_ITEM (DBXREF_ON_SEQFEAT, Itr, Var)

/// DBXREF_ON_SEQFEAT_IS_SORTED

#define DBXREF_ON_SEQFEAT_IS_SORTED(Var, Func) \
IS_SORTED (DBXREF_ON_SEQFEAT, Var, Func)

/// SORT_DBXREF_ON_SEQFEAT

#define SORT_DBXREF_ON_SEQFEAT(Var, Func) \
DO_VECTOR_SORT (DBXREF_ON_SEQFEAT, Var, Func)

/// DBXREF_ON_SEQFEAT_IS_UNIQUE

#define DBXREF_ON_SEQFEAT_IS_UNIQUE(Var, Func) \
IS_UNIQUE (DBXREF_ON_SEQFEAT, Var, Func)

/// UNIQUE_DBXREF_ON_SEQFEAT

#define UNIQUE_DBXREF_ON_SEQFEAT(Var, Func) \
DO_UNIQUE (DBXREF_ON_SEQFEAT, Var, Func)

/// REMOVE_IF_EMPTY_DBXREF_ON_SEQFEAT

#define REMOVE_IF_EMPTY_DBXREF_ON_SEQFEAT(Var) \
REMOVE_IF_EMPTY_FIELD(DBXREF_ON_SEQFEAT, Var)

/// FEATURE_HAS_DBXREF
/// FOR_EACH_DBXREF_ON_FEATURE
/// EDIT_EACH_DBXREF_ON_FEATURE
/// ADD_DBXREF_TO_FEATURE
/// ERASE_DBXREF_ON_FEATURE
/// DBXREF_ON_FEATURE_IS_SORTED
/// SORT_DBXREF_ON_FEATURE
/// DBXREF_ON_FEATURE_IS_UNIQUE
/// UNIQUE_DBXREF_ON_FEATURE

#define FEATURE_HAS_DBXREF SEQFEAT_HAS_DBXREF
#define FOR_EACH_DBXREF_ON_FEATURE FOR_EACH_DBXREF_ON_SEQFEAT
#define EDIT_EACH_DBXREF_ON_FEATURE EDIT_EACH_DBXREF_ON_SEQFEAT
#define ADD_DBXREF_TO_FEATURE ADD_DBXREF_TO_SEQFEAT
#define ERASE_DBXREF_ON_FEATURE ERASE_DBXREF_ON_SEQFEAT
#define DBXREF_ON_FEATURE_IS_SORTED DBXREF_ON_SEQFEAT_IS_SORTED
#define SORT_DBXREF_ON_FEATURE SORT_DBXREF_ON_SEQFEAT
#define DBXREF_ON_FEATURE_IS_UNIQUE DBXREF_ON_SEQFEAT_IS_UNIQUE
#define UNIQUE_DBXREF_ON_FEATURE UNIQUE_DBXREF_ON_SEQFEAT


///
/// CSeqFeatData macros

/// SEQFEATDATA_CHOICE macros

#define SEQFEATDATA_CHOICE_Test(Var) (Var).Which() != CSeqFeatData::e_not_set
#define SEQFEATDATA_CHOICE_Chs(Var)  (Var).Which()

/// SEQFEATDATA_CHOICE_IS

#define SEQFEATDATA_CHOICE_IS(Var, Chs) \
CHOICE_IS (SEQFEATDATA_CHOICE, Var, Chs)

/// SWITCH_ON_SEQFEATDATA_CHOICE

#define SWITCH_ON_SEQFEATDATA_CHOICE(Var) \
SWITCH_ON (SEQFEATDATA_CHOICE, Var)


///
/// CSeqFeatXref macros

/// SEQFEATXREF_CHOICE macros

#define SEQFEATXREF_CHOICE_Test(Var) (Var).IsSetData()
#define SEQFEATXREF_CHOICE_Chs(Var)  (Var).GetData().Which()

/// SEQFEATXREF_CHOICE_IS

#define SEQFEATXREF_CHOICE_IS(Var, Chs) \
CHOICE_IS (SEQFEATXREF_CHOICE, Var, Chs)

/// SWITCH_ON_SEQFEATXREF_CHOICE

#define SWITCH_ON_SEQFEATXREF_CHOICE(Var) \
SWITCH_ON (SEQFEATXREF_CHOICE, Var)


///
/// CGene_ref macros

/// SYNONYM_ON_GENEREF macros

#define SYNONYM_ON_GENEREF_Type      CGene_ref::TSyn
#define SYNONYM_ON_GENEREF_Test(Var) (Var).IsSetSyn()
#define SYNONYM_ON_GENEREF_Get(Var)  (Var).GetSyn()
#define SYNONYM_ON_GENEREF_Set(Var)  (Var).SetSyn()

/// GENEREF_HAS_SYNONYM

#define GENEREF_HAS_SYNONYM(Var) \
ITEM_HAS (SYNONYM_ON_GENEREF, Var)

/// FOR_EACH_SYNONYM_ON_GENEREF
/// EDIT_EACH_SYNONYM_ON_GENEREF
// CGene_ref& as input, dereference with [const] string& str = *itr;

#define FOR_EACH_SYNONYM_ON_GENEREF(Itr, Var) \
FOR_EACH (SYNONYM_ON_GENEREF, Itr, Var)

#define EDIT_EACH_SYNONYM_ON_GENEREF(Itr, Var) \
EDIT_EACH (SYNONYM_ON_GENEREF, Itr, Var)

/// ADD_SYNONYM_TO_GENEREF

#define ADD_SYNONYM_TO_GENEREF(Var, Ref) \
ADD_ITEM (SYNONYM_ON_GENEREF, Var, Ref)

/// ERASE_SYNONYM_ON_GENEREF

#define ERASE_SYNONYM_ON_GENEREF(Itr, Var) \
LIST_ERASE_ITEM (SYNONYM_ON_GENEREF, Itr, Var)

/// SYNONYM_ON_GENEREF_IS_SORTED

#define SYNONYM_ON_GENEREF_IS_SORTED(Var, Func) \
IS_SORTED (SYNONYM_ON_GENEREF, Var, Func)

/// SORT_SYNONYM_ON_GENEREF

#define SORT_SYNONYM_ON_GENEREF(Var, Func) \
DO_LIST_SORT (SYNONYM_ON_GENEREF, Var, Func)

/// SYNONYM_ON_GENEREF_IS_UNIQUE

#define SYNONYM_ON_GENEREF_IS_UNIQUE(Var, Func) \
IS_UNIQUE (SYNONYM_ON_GENEREF, Var, Func)

/// UNIQUE_SYNONYM_ON_GENEREF

#define UNIQUE_SYNONYM_ON_GENEREF(Var, Func) \
DO_UNIQUE (SYNONYM_ON_GENEREF, Var, Func)

/// GENE_HAS_SYNONYM
/// FOR_EACH_SYNONYM_ON_GENE
/// EDIT_EACH_SYNONYM_ON_GENE
/// ADD_SYNONYM_TO_GENE
/// ERASE_SYNONYM_ON_GENE
/// SYNONYM_ON_GENE_IS_SORTED
/// SORT_SYNONYM_ON_GENE
/// SYNONYM_ON_GENE_IS_UNIQUE
/// UNIQUE_SYNONYM_ON_GENE

#define GENE_HAS_SYNONYM GENEREF_HAS_SYNONYM
#define FOR_EACH_SYNONYM_ON_GENE FOR_EACH_SYNONYM_ON_GENEREF
#define EDIT_EACH_SYNONYM_ON_GENE EDIT_EACH_SYNONYM_ON_GENEREF
#define ADD_SYNONYM_TO_GENE ADD_SYNONYM_TO_GENEREF
#define ERASE_SYNONYM_ON_GENE ERASE_SYNONYM_ON_GENEREF
#define SYNONYM_ON_GENE_IS_SORTED SYNONYM_ON_GENEREF_IS_SORTED
#define SORT_SYNONYM_ON_GENE SORT_SYNONYM_ON_GENEREF
#define SYNONYM_ON_GENE_IS_UNIQUE SYNONYM_ON_GENEREF_IS_UNIQUE
#define UNIQUE_SYNONYM_ON_GENE UNIQUE_SYNONYM_ON_GENEREF


/// DBXREF_ON_GENEREF macros

#define DBXREF_ON_GENEREF_Type      CGene_ref::TDb
#define DBXREF_ON_GENEREF_Test(Var) (Var).IsSetDb()
#define DBXREF_ON_GENEREF_Get(Var)  (Var).GetDb()
#define DBXREF_ON_GENEREF_Set(Var)  (Var).SetDb()

/// GENEREF_HAS_DBXREF

#define GENEREF_HAS_DBXREF(Var) \
ITEM_HAS (DBXREF_ON_GENEREF, Var)

/// FOR_EACH_DBXREF_ON_GENEREF
/// EDIT_EACH_DBXREF_ON_GENEREF
// CGene_ref& as input, dereference with [const] CDbtag& dbt = **itr;

#define FOR_EACH_DBXREF_ON_GENEREF(Itr, Var) \
FOR_EACH (DBXREF_ON_GENEREF, Itr, Var)

#define EDIT_EACH_DBXREF_ON_GENEREF(Itr, Var) \
EDIT_EACH (DBXREF_ON_GENEREF, Itr, Var)

/// ADD_DBXREF_TO_GENEREF

#define ADD_DBXREF_TO_GENEREF(Var, Ref) \
ADD_ITEM (DBXREF_ON_GENEREF, Var, Ref)

/// ERASE_DBXREF_ON_GENEREF

#define ERASE_DBXREF_ON_GENEREF(Itr, Var) \
VECTOR_ERASE_ITEM (DBXREF_ON_GENEREF, Itr, Var)

/// DBXREF_ON_GENEREF_IS_SORTED

#define DBXREF_ON_GENEREF_IS_SORTED(Var, Func) \
IS_SORTED (DBXREF_ON_GENEREF, Var, Func)

/// SORT_DBXREF_ON_GENEREF

#define SORT_DBXREF_ON_GENEREF(Var, Func) \
DO_VECTOR_SORT (DBXREF_ON_GENEREF, Var, Func)

/// DBXREF_ON_GENEREF_IS_UNIQUE

#define DBXREF_ON_GENEREF_IS_UNIQUE(Var, Func) \
IS_UNIQUE (DBXREF_ON_GENEREF, Var, Func)

/// UNIQUE_DBXREF_ON_GENEREF

#define UNIQUE_DBXREF_ON_GENEREF(Var, Func) \
DO_UNIQUE (DBXREF_ON_GENEREF, Var, Func)

/// GENE_HAS_DBXREF
/// FOR_EACH_DBXREF_ON_GENE
/// EDIT_EACH_DBXREF_ON_GENE
/// ADD_DBXREF_TO_GENE
/// ERASE_DBXREF_ON_GENE
/// DBXREF_ON_GENE_IS_SORTED
/// SORT_DBXREF_ON_GENE
/// DBXREF_ON_GENE_IS_UNIQUE
/// UNIQUE_DBXREF_ON_GENE

#define GENE_HAS_DBXREF GENEREF_HAS_DBXREF
#define FOR_EACH_DBXREF_ON_GENE FOR_EACH_DBXREF_ON_GENEREF
#define EDIT_EACH_DBXREF_ON_GENE EDIT_EACH_DBXREF_ON_GENEREF
#define ADD_DBXREF_TO_GENE ADD_DBXREF_TO_GENEREF
#define ERASE_DBXREF_ON_GENE ERASE_DBXREF_ON_GENEREF
#define DBXREF_ON_GENE_IS_SORTED DBXREF_ON_GENEREF_IS_SORTED
#define SORT_DBXREF_ON_GENE SORT_DBXREF_ON_GENEREF
#define DBXREF_ON_GENE_IS_UNIQUE DBXREF_ON_GENEREF_IS_UNIQUE
#define UNIQUE_DBXREF_ON_GENE UNIQUE_DBXREF_ON_GENEREF


///
/// CCdregion macros

/// CODEBREAK_ON_CDREGION macros

#define CODEBREAK_ON_CDREGION_Type      CCdregion::TCode_break
#define CODEBREAK_ON_CDREGION_Test(Var) (Var).IsSetCode_break()
#define CODEBREAK_ON_CDREGION_Get(Var)  (Var).GetCode_break()
#define CODEBREAK_ON_CDREGION_Set(Var)  (Var).SetCode_break()

/// CDREGION_HAS_CODEBREAK

#define CDREGION_HAS_CODEBREAK(Var) \
ITEM_HAS (CODEBREAK_ON_CDREGION, Var)

/// FOR_EACH_CODEBREAK_ON_CDREGION
/// EDIT_EACH_CODEBREAK_ON_CDREGION
// CCdregion& as input, dereference with [const] CCode_break& cbk = **itr;

#define FOR_EACH_CODEBREAK_ON_CDREGION(Itr, Var) \
FOR_EACH (CODEBREAK_ON_CDREGION, Itr, Var)

#define EDIT_EACH_CODEBREAK_ON_CDREGION(Itr, Var) \
EDIT_EACH (CODEBREAK_ON_CDREGION, Itr, Var)

/// ADD_CODEBREAK_TO_CDREGION

#define ADD_CODEBREAK_TO_CDREGION(Var, Ref) \
ADD_ITEM (CODEBREAK_ON_CDREGION, Var, Ref)

/// ERASE_CODEBREAK_ON_CDREGION

#define ERASE_CODEBREAK_ON_CDREGION(Itr, Var) \
LIST_ERASE_ITEM (CODEBREAK_ON_CDREGION, Itr, Var)

/// CODEBREAK_ON_CDREGION_IS_SORTED

#define CODEBREAK_ON_CDREGION_IS_SORTED(Var, Func) \
IS_SORTED (CODEBREAK_ON_CDREGION, Var, Func)

/// SORT_CODEBREAK_ON_CDREGION

#define SORT_CODEBREAK_ON_CDREGION(Var, Func) \
DO_LIST_SORT_HACK(CODEBREAK_ON_CDREGION, Var, Func)

/// CODEBREAK_ON_CDREGION_IS_UNIQUE

#define CODEBREAK_ON_CDREGION_IS_UNIQUE(Var, Func) \
IS_UNIQUE (CODEBREAK_ON_CDREGION, Var, Func)

/// UNIQUE_CODEBREAK_ON_CDREGION

#define UNIQUE_CODEBREAK_ON_CDREGION(Var, Func) \
DO_UNIQUE (CODEBREAK_ON_CDREGION, Var, Func)


///
/// CProt_ref macros

/// NAME_ON_PROTREF macros

#define NAME_ON_PROTREF_Type       CProt_ref::TName
#define NAME_ON_PROTREF_Test(Var)  (Var).IsSetName()
#define NAME_ON_PROTREF_Get(Var)   (Var).GetName()
#define NAME_ON_PROTREF_Set(Var)   (Var).SetName()
#define NAME_ON_PROTREF_Reset(Var) (Var).ResetName()

/// PROTREF_HAS_NAME

#define PROTREF_HAS_NAME(Var) \
ITEM_HAS (NAME_ON_PROTREF, Var)

/// FOR_EACH_NAME_ON_PROTREF
/// EDIT_EACH_NAME_ON_PROTREF
// CProt_ref& as input, dereference with [const] string& str = *itr;

#define FOR_EACH_NAME_ON_PROTREF(Itr, Var) \
FOR_EACH (NAME_ON_PROTREF, Itr, Var)

#define EDIT_EACH_NAME_ON_PROTREF(Itr, Var) \
EDIT_EACH (NAME_ON_PROTREF, Itr, Var)

/// ADD_NAME_TO_PROTREF

#define ADD_NAME_TO_PROTREF(Var, Ref) \
ADD_ITEM (NAME_ON_PROTREF, Var, Ref)

/// ERASE_NAME_ON_PROTREF

#define ERASE_NAME_ON_PROTREF(Itr, Var) \
LIST_ERASE_ITEM (NAME_ON_PROTREF, Itr, Var)

/// NAME_ON_PROTREF_IS_SORTED

#define NAME_ON_PROTREF_IS_SORTED(Var, Func) \
IS_SORTED (NAME_ON_PROTREF, Var, Func)

/// SORT_NAME_ON_PROTREF

#define SORT_NAME_ON_PROTREF(Var, Func) \
DO_LIST_SORT (NAME_ON_PROTREF, Var, Func)

/// NAME_ON_PROTREF_IS_UNIQUE

#define NAME_ON_PROTREF_IS_UNIQUE(Var, Func) \
IS_UNIQUE (NAME_ON_PROTREF, Var, Func)

/// UNIQUE_NAME_ON_PROTREF

#define UNIQUE_NAME_ON_PROTREF(Var, Func) \
DO_UNIQUE (NAME_ON_PROTREF, Var, Func)

#define REMOVE_IF_EMPTY_NAME_ON_PROTREF(Var) \
    REMOVE_IF_EMPTY_FIELD(NAME_ON_PROTREF, Var)

#define NAME_ON_PROTREF_IS_EMPTY(Var) \
    FIELD_IS_EMPTY( NAME_ON_PROTREF, Var )    

/// PROT_HAS_NAME
/// FOR_EACH_NAME_ON_PROT
/// EDIT_EACH_NAME_ON_PROT
/// ADD_NAME_TO_PROT
/// ERASE_NAME_ON_PROT
/// NAME_ON_PROT_IS_SORTED
/// SORT_NAME_ON_PROT
/// NAME_ON_PROT_IS_UNIQUE
/// UNIQUE_NAME_ON_PROT

#define PROT_HAS_NAME PROTREF_HAS_NAME
#define FOR_EACH_NAME_ON_PROT FOR_EACH_NAME_ON_PROTREF
#define EDIT_EACH_NAME_ON_PROT EDIT_EACH_NAME_ON_PROTREF
#define ADD_NAME_TO_PROT ADD_NAME_TO_PROTREF
#define ERASE_NAME_ON_PROT ERASE_NAME_ON_PROTREF
#define NAME_ON_PROT_IS_SORTED NAME_ON_PROTREF_IS_SORTED
#define SORT_NAME_ON_PROT SORT_NAME_ON_PROTREF
#define NAME_ON_PROT_IS_UNIQUE NAME_ON_PROTREF_IS_UNIQUE
#define UNIQUE_NAME_ON_PROT UNIQUE_NAME_ON_PROTREF


/// ECNUMBER_ON_PROTREF macros

#define ECNUMBER_ON_PROTREF_Type      CProt_ref::TEc
#define ECNUMBER_ON_PROTREF_Test(Var) (Var).IsSetEc()
#define ECNUMBER_ON_PROTREF_Get(Var)  (Var).GetEc()
#define ECNUMBER_ON_PROTREF_Set(Var)  (Var).SetEc()

/// PROTREF_HAS_ECNUMBER

#define PROTREF_HAS_ECNUMBER(Var) \
ITEM_HAS (ECNUMBER_ON_PROTREF, Var)

/// FOR_EACH_ECNUMBER_ON_PROTREF
/// EDIT_EACH_ECNUMBER_ON_PROTREF
// CProt_ref& as input, dereference with [const] string& str = *itr;

#define FOR_EACH_ECNUMBER_ON_PROTREF(Itr, Var) \
FOR_EACH (ECNUMBER_ON_PROTREF, Itr, Var)

#define EDIT_EACH_ECNUMBER_ON_PROTREF(Itr, Var) \
EDIT_EACH (ECNUMBER_ON_PROTREF, Itr, Var)

/// ADD_ECNUMBER_TO_PROTREF

#define ADD_ECNUMBER_TO_PROTREF(Var, Ref) \
ADD_ITEM (ECNUMBER_ON_PROTREF, Var, Ref)

/// ERASE_ECNUMBER_ON_PROTREF

#define ERASE_ECNUMBER_ON_PROTREF(Itr, Var) \
LIST_ERASE_ITEM (ECNUMBER_ON_PROTREF, Itr, Var)

/// ECNUMBER_ON_PROTREF_IS_SORTED

#define ECNUMBER_ON_PROTREF_IS_SORTED(Var, Func) \
IS_SORTED (ECNUMBER_ON_PROTREF, Var, Func)

/// SORT_ECNUMBER_ON_PROTREF

#define SORT_ECNUMBER_ON_PROTREF(Var, Func) \
DO_LIST_SORT (ECNUMBER_ON_PROTREF, Var, Func)

/// ECNUMBER_ON_PROTREF_IS_UNIQUE

#define ECNUMBER_ON_PROTREF_IS_UNIQUE(Var, Func) \
IS_UNIQUE (ECNUMBER_ON_PROTREF, Var, Func)

/// UNIQUE_ECNUMBER_ON_PROTREF

#define UNIQUE_ECNUMBER_ON_PROTREF(Var, Func) \
DO_UNIQUE (ECNUMBER_ON_PROTREF, Var, Func)

/// PROT_HAS_ECNUMBER
/// FOR_EACH_ECNUMBER_ON_PROT
/// EDIT_EACH_ECNUMBER_ON_PROT
/// ADD_ECNUMBER_TO_PROT
/// ERASE_ECNUMBER_ON_PROT
/// ECNUMBER_ON_PROT_IS_SORTED
/// SORT_ECNUMBER_ON_PROT
/// ECNUMBER_ON_PROT_IS_UNIQUE
/// UNIQUE_ECNUMBER_ON_PROT

#define PROT_HAS_ECNUMBER PROTREF_HAS_ECNUMBER
#define FOR_EACH_ECNUMBER_ON_PROT FOR_EACH_ECNUMBER_ON_PROTREF
#define EDIT_EACH_ECNUMBER_ON_PROT EDIT_EACH_ECNUMBER_ON_PROTREF
#define ADD_ECNUMBER_TO_PROT ADD_ECNUMBER_TO_PROTREF
#define ERASE_ECNUMBER_ON_PROT ERASE_ECNUMBER_ON_PROTREF
#define ECNUMBER_ON_PROT_IS_SORTED ECNUMBER_ON_PROTREF_IS_SORTED
#define SORT_ECNUMBER_ON_PROT SORT_ECNUMBER_ON_PROTREF
#define ECNUMBER_ON_PROT_IS_UNIQUE ECNUMBER_ON_PROTREF_IS_UNIQUE
#define UNIQUE_ECNUMBER_ON_PROT UNIQUE_ECNUMBER_ON_PROTREF


/// ACTIVITY_ON_PROTREF macros

#define ACTIVITY_ON_PROTREF_Type       CProt_ref::TActivity
#define ACTIVITY_ON_PROTREF_Test(Var)  (Var).IsSetActivity()
#define ACTIVITY_ON_PROTREF_Get(Var)   (Var).GetActivity()
#define ACTIVITY_ON_PROTREF_Set(Var)   (Var).SetActivity()
#define ACTIVITY_ON_PROTREF_Reset(Var) (Var).ResetActivity()

/// PROTREF_HAS_ACTIVITY

#define PROTREF_HAS_ACTIVITY(Var) \
ITEM_HAS (ACTIVITY_ON_PROTREF, Var)

/// FOR_EACH_ACTIVITY_ON_PROTREF
/// EDIT_EACH_ACTIVITY_ON_PROTREF
// CProt_ref& as input, dereference with [const] string& str = *itr;

#define FOR_EACH_ACTIVITY_ON_PROTREF(Itr, Var) \
FOR_EACH (ACTIVITY_ON_PROTREF, Itr, Var)

#define EDIT_EACH_ACTIVITY_ON_PROTREF(Itr, Var) \
EDIT_EACH (ACTIVITY_ON_PROTREF, Itr, Var)

/// ADD_ACTIVITY_TO_PROTREF

#define ADD_ACTIVITY_TO_PROTREF(Var, Ref) \
ADD_ITEM (ACTIVITY_ON_PROTREF, Var, Ref)

/// ERASE_ACTIVITY_ON_PROTREF

#define ERASE_ACTIVITY_ON_PROTREF(Itr, Var) \
LIST_ERASE_ITEM (ACTIVITY_ON_PROTREF, Itr, Var)

/// ACTIVITY_ON_PROTREF_IS_SORTED

#define ACTIVITY_ON_PROTREF_IS_SORTED(Var, Func) \
IS_SORTED (ACTIVITY_ON_PROTREF, Var, Func)

/// SORT_ACTIVITY_ON_PROTREF

#define SORT_ACTIVITY_ON_PROTREF(Var, Func) \
DO_LIST_SORT (ACTIVITY_ON_PROTREF, Var, Func)

/// ACTIVITY_ON_PROTREF_IS_UNIQUE

#define ACTIVITY_ON_PROTREF_IS_UNIQUE(Var, Func) \
IS_UNIQUE (ACTIVITY_ON_PROTREF, Var, Func)

/// UNIQUE_ACTIVITY_ON_PROTREF

#define UNIQUE_ACTIVITY_ON_PROTREF(Var, Func) \
DO_UNIQUE (ACTIVITY_ON_PROTREF, Var, Func)

/// UNIQUE_WITHOUT_SORT_ACTIVITY_ON_PROTREF(Var, Func)

#define UNIQUE_WITHOUT_SORT_ACTIVITY_ON_PROTREF(Var, FuncType ) \
UNIQUE_WITHOUT_SORT( ACTIVITY_ON_PROTREF, Var, FuncType, \
    CCleanupChange::eChangeProtActivities)

/// REMOVE_IF_EMPTY_ACTIVITY_ON_PROTREF

#define REMOVE_IF_EMPTY_ACTIVITY_ON_PROTREF(Var) \
    REMOVE_IF_EMPTY_FIELD( ACTIVITY_ON_PROTREF, Var )

/// PROT_HAS_ACTIVITY
/// FOR_EACH_ACTIVITY_ON_PROT
/// EDIT_EACH_ACTIVITY_ON_PROT
/// ADD_ACTIVITY_TO_PROT
/// ERASE_ACTIVITY_ON_PROT
/// ACTIVITY_ON_PROT_IS_SORTED
/// SORT_ACTIVITY_ON_PROT
/// ACTIVITY_ON_PROT_IS_UNIQUE
/// UNIQUE_ACTIVITY_ON_PROT

#define PROT_HAS_ACTIVITY PROTREF_HAS_ACTIVITY
#define FOR_EACH_ACTIVITY_ON_PROT FOR_EACH_ACTIVITY_ON_PROTREF
#define EDIT_EACH_ACTIVITY_ON_PROT EDIT_EACH_ACTIVITY_ON_PROTREF
#define ADD_ACTIVITY_TO_PROT ADD_ACTIVITY_TO_PROTREF
#define ERASE_ACTIVITY_ON_PROT ERASE_ACTIVITY_ON_PROTREF
#define ACTIVITY_ON_PROT_IS_SORTED ACTIVITY_ON_PROTREF_IS_SORTED
#define SORT_ACTIVITY_ON_PROT SORT_ACTIVITY_ON_PROTREF
#define ACTIVITY_ON_PROT_IS_UNIQUE ACTIVITY_ON_PROTREF_IS_UNIQUE
#define UNIQUE_ACTIVITY_ON_PROT UNIQUE_ACTIVITY_ON_PROTREF


/// DBXREF_ON_PROTREF macros

#define DBXREF_ON_PROTREF_Type      CProt_ref::TDb
#define DBXREF_ON_PROTREF_Test(Var) (Var).IsSetDb()
#define DBXREF_ON_PROTREF_Get(Var)  (Var).GetDb()
#define DBXREF_ON_PROTREF_Set(Var)  (Var).SetDb()

/// PROTREF_HAS_DBXREF

#define PROTREF_HAS_DBXREF(Var) \
ITEM_HAS (DBXREF_ON_PROTREF, Var)

/// FOR_EACH_DBXREF_ON_PROTREF
/// EDIT_EACH_DBXREF_ON_PROTREF
// CProt_ref& as input, dereference with [const] CDbtag& dbt = *itr;

#define FOR_EACH_DBXREF_ON_PROTREF(Itr, Var) \
FOR_EACH (DBXREF_ON_PROTREF, Itr, Var)

#define EDIT_EACH_DBXREF_ON_PROTREF(Itr, Var) \
EDIT_EACH (DBXREF_ON_PROTREF, Itr, Var)

/// ADD_DBXREF_TO_PROTREF

#define ADD_DBXREF_TO_PROTREF(Var, Ref) \
ADD_ITEM (DBXREF_ON_PROTREF, Var, Ref)

/// ERASE_DBXREF_ON_PROTREF

#define ERASE_DBXREF_ON_PROTREF(Itr, Var) \
VECTOR_ERASE_ITEM (DBXREF_ON_PROTREF, Itr, Var)

/// DBXREF_ON_PROTREF_IS_SORTED

#define DBXREF_ON_PROTREF_IS_SORTED(Var, Func) \
IS_SORTED (DBXREF_ON_PROTREF, Var, Func)

/// SORT_DBXREF_ON_PROTREF

#define SORT_DBXREF_ON_PROTREF(Var, Func) \
DO_VECTOR_SORT (DBXREF_ON_PROTREF, Var, Func)

/// DBXREF_ON_PROTREF_IS_UNIQUE

#define DBXREF_ON_PROTREF_IS_UNIQUE(Var, Func) \
IS_UNIQUE (DBXREF_ON_PROTREF, Var, Func)

/// UNIQUE_DBXREF_ON_PROTREF

#define UNIQUE_DBXREF_ON_PROTREF(Var, Func) \
DO_UNIQUE (DBXREF_ON_PROTREF, Var, Func)

/// PROT_HAS_DBXREF
/// FOR_EACH_DBXREF_ON_PROT
/// EDIT_EACH_DBXREF_ON_PROT
/// ADD_DBXREF_TO_PROT
/// ERASE_DBXREF_ON_PROT
/// DBXREF_ON_PROT_IS_SORTED
/// SORT_DBXREF_ON_PROT
/// DBXREF_ON_PROT_IS_UNIQUE
/// UNIQUE_DBXREF_ON_PROT

#define PROT_HAS_DBXREF PROTREF_HAS_DBXREF
#define FOR_EACH_DBXREF_ON_PROT FOR_EACH_DBXREF_ON_PROTREF
#define EDIT_EACH_DBXREF_ON_PROT EDIT_EACH_DBXREF_ON_PROTREF
#define ADD_DBXREF_TO_PROT ADD_DBXREF_TO_PROTREF
#define ERASE_DBXREF_ON_PROT ERASE_DBXREF_ON_PROTREF
#define DBXREF_ON_PROT_IS_SORTED DBXREF_ON_PROTREF_IS_SORTED
#define SORT_DBXREF_ON_PROT SORT_DBXREF_ON_PROTREF
#define DBXREF_ON_PROT_IS_UNIQUE DBXREF_ON_PROTREF_IS_UNIQUE
#define UNIQUE_DBXREF_ON_PROT UNIQUE_DBXREF_ON_PROTREF


///
/// CRNA_gen macros

/// QUAL_ON_RNAGEN macros

#define QUAL_ON_RNAGEN_Type      CRNA_gen::TQuals::Tdata
#define QUAL_ON_RNAGEN_Test(Var) (Var).IsSetQuals() && (Var).GetQuals().IsSet()
#define QUAL_ON_RNAGEN_Get(Var)  (Var).GetQuals().Get()
#define QUAL_ON_RNAGEN_Set(Var)  (Var).SetQuals().Set()

/// RNAGEN_HAS_QUAL

#define RNAGEN_HAS_QUAL(Var) \
ITEM_HAS (QUAL_ON_RNAGEN, Var)

/// FOR_EACH_QUAL_ON_RNAGEN
/// EDIT_EACH_QUAL_ON_RNAGEN
// CRNA_gen& as input, dereference with [const] CRNA_qual& qual = **itr;

#define FOR_EACH_QUAL_ON_RNAGEN(Itr, Var) \
FOR_EACH (QUAL_ON_RNAGEN, Itr, Var)

#define EDIT_EACH_QUAL_ON_RNAGEN(Itr, Var) \
EDIT_EACH (QUAL_ON_RNAGEN, Itr, Var)

/// ADD_QUAL_TO_RNAGEN

#define ADD_QUAL_TO_RNAGEN(Var, Ref) \
ADD_ITEM (QUAL_ON_RNAGEN, Var, Ref)

/// ERASE_QUAL_ON_RNAGEN

#define ERASE_QUAL_ON_RNAGEN(Itr, Var) \
LIST_ERASE_ITEM (QUAL_ON_RNAGEN, Itr, Var)

/// QUAL_ON_RNAGEN_IS_SORTED

#define QUAL_ON_RNAGEN_IS_SORTED(Var, Func) \
IS_SORTED (QUAL_ON_RNAGEN, Var, Func)

/// SORT_QUAL_ON_RNAGEN

#define SORT_QUAL_ON_RNAGEN(Var, Func) \
DO_LIST_SORT (QUAL_ON_RNAGEN, Var, Func)
 
/// QUAL_ON_RNAGEN_IS_UNIQUE

#define QUAL_ON_RNAGEN_IS_UNIQUE(Var, Func) \
IS_UNIQUE (QUAL_ON_RNAGEN, Var, Func)

/// UNIQUE_QUAL_ON_RNAGEN

#define UNIQUE_QUAL_ON_RNAGEN(Var, Func) \
DO_UNIQUE (QUAL_ON_RNAGEN, Var, Func)

/// REMOVE_IF_EMPTY_QUAL_ON_RNAGEN

#define REMOVE_IF_EMPTY_QUAL_ON_RNAGEN(Var) \
    REMOVE_IF_EMPTY_FIELD(QUAL_ON_RNAGEN, Var)

/// QUAL_ON_RNAGEN_IS_EMPTY

#define QUAL_ON_RNAGEN_IS_EMPTY(Var) \
    FIELD_IS_EMPTY(QUAL_ON_RNAGEN, Var, Func)


///
/// CRNA_qual_set macros

/// QUAL_ON_RNAQSET macros

#define QUAL_ON_RNAQSET_Type       CRNA_qual_set::Tdata
#define QUAL_ON_RNAQSET_Test(Var)  (Var).IsSet()
#define QUAL_ON_RNAQSET_Get(Var)   (Var).Get()
#define QUAL_ON_RNAQSET_Set(Var)   (Var).Set()
#define QUAL_ON_RNAQSET_Reset(Var) (Var).Reset()

/// RNAQSET_HAS_QUAL

#define RNAQSET_HAS_QUAL(Var) \
ITEM_HAS (QUAL_ON_RNAQSET, Var)

/// FOR_EACH_QUAL_ON_RNAQSET
/// EDIT_EACH_QUAL_ON_RNAQSET
// CRNA_qual_set& as input, dereference with [const] CRNA_qual& qual = **itr;

#define FOR_EACH_QUAL_ON_RNAQSET(Itr, Var) \
FOR_EACH (QUAL_ON_RNAQSET, Itr, Var)

#define EDIT_EACH_QUAL_ON_RNAQSET(Itr, Var) \
EDIT_EACH (QUAL_ON_RNAQSET, Itr, Var)

/// ADD_QUAL_TO_RNAQSET

#define ADD_QUAL_TO_RNAQSET(Var, Ref) \
ADD_ITEM (QUAL_ON_RNAQSET, Var, Ref)

/// ERASE_QUAL_ON_RNAQSET

#define ERASE_QUAL_ON_RNAQSET(Itr, Var) \
LIST_ERASE_ITEM (QUAL_ON_RNAQSET, Itr, Var)

/// QUAL_ON_RNAQSET_IS_SORTED

#define QUAL_ON_RNAQSET_IS_SORTED(Var, Func) \
IS_SORTED (QUAL_ON_RNAQSET, Var, Func)

/// SORT_QUAL_ON_RNAQSET

#define SORT_QUAL_ON_RNAQSET(Var, Func) \
DO_LIST_SORT (QUAL_ON_RNAQSET, Var, Func)

/// QUAL_ON_RNAQSET_IS_UNIQUE

#define QUAL_ON_RNAQSET_IS_UNIQUE(Var, Func) \
IS_UNIQUE (QUAL_ON_RNAQSET, Var, Func)

/// UNIQUE_QUAL_ON_RNAQSET

#define UNIQUE_QUAL_ON_RNAQSET(Var, Func) \
DO_UNIQUE (QUAL_ON_RNAQSET, Var, Func)

/// QUAL_ON_RNAQSET_IS_EMPTY

#define QUAL_ON_RNAQSET_IS_EMPTY(Var) \
    FIELD_IS_EMPTY(QUAL_ON_RNAQSET, Var)

/// REMOVE_IF_EMPTY_QUAL_ON_RNAQSET
#define REMOVE_IF_EMPTY_QUAL_ON_RNAQSET(Var) \
    REMOVE_IF_EMPTY_FIELD(QUAL_ON_RNAQSET, Var)

///
/// CTrna_ext macros

#define CODON_ON_TRNAEXT_Type       CTrna_ext::TCodon
#define CODON_ON_TRNAEXT_Test(Var)  (Var).IsSetCodon()
#define CODON_ON_TRNAEXT_Get(Var)   (Var).GetCodon()
#define CODON_ON_TRNAEXT_Set(Var)   (Var).SetCodon()
#define CODON_ON_TRNAEXT_Reset(Var) (Var).ResetCodon()

/// CODON_ON_TRNAEXT_IS_SORTED

#define CODON_ON_TRNAEXT_IS_SORTED(Var, Func) \
IS_SORTED (CODON_ON_TRNAEXT, Var, Func)

/// SORT_CODON_ON_TRNAEXT

#define SORT_CODON_ON_TRNAEXT(Var, Func) \
DO_LIST_SORT (CODON_ON_TRNAEXT, Var, Func)

/// CODON_ON_TRNAEXT_IS_UNIQUE

#define CODON_ON_TRNAEXT_IS_UNIQUE(Var, Func) \
IS_UNIQUE (CODON_ON_TRNAEXT, Var, Func)

/// UNIQUE_CODON_ON_TRNAEXT

#define UNIQUE_CODON_ON_TRNAEXT(Var, Func) \
DO_UNIQUE (CODON_ON_TRNAEXT, Var, Func)

/// CODON_ON_TRNAEXT_IS_EMPTY_OR_UNSET

#define CODON_ON_TRNAEXT_IS_EMPTY_OR_UNSET(Var) \
    FIELD_IS_EMPTY_OR_UNSET(CODON_ON_TRNAEXT, Var)

/// REMOVE_IF_EMPTY_CODON_ON_TRNAEXT

#define REMOVE_IF_EMPTY_CODON_ON_TRNAEXT(Var) \
    REMOVE_IF_EMPTY_FIELD(CODON_ON_TRNAEXT, Var)

///
/// CPCRParsedSet macros

#define PCRPARSEDSET_IN_LIST_Type       list<CPCRParsedSet>
#define PCRPARSEDSET_IN_LIST_Test(Var)  (! (Var).empty())
#define PCRPARSEDSET_IN_LIST_Get(Var)   (Var)
#define PCRPARSEDSET_IN_LIST_Set(Var)   (Var)
#define PCRPARSEDSET_IN_LIST_Reset(Var) (Var).clear()

#define FOR_EACH_PCRPARSEDSET_IN_LIST(Itr, Var) \
    FOR_EACH (PCRPARSEDSET_IN_LIST, Itr, Var)

///
/// CPCRReactionSet macros

#define PCRREACTION_IN_PCRREACTIONSET_Type       CPCRReactionSet::Tdata
#define PCRREACTION_IN_PCRREACTIONSET_Test(Var)  ( (Var).IsSet() && ! (Var).Get().empty() )
#define PCRREACTION_IN_PCRREACTIONSET_Get(Var)   (Var).Get()
#define PCRREACTION_IN_PCRREACTIONSET_Set(Var)   (Var).Set()
#define PCRREACTION_IN_PCRREACTIONSET_Reset(Var) (Var).Reset()

/// FOR_EACH_PCRREACTION_IN_PCRREACTIONSET

#define FOR_EACH_PCRREACTION_IN_PCRREACTIONSET(Itr, Var) \
    FOR_EACH (PCRREACTION_IN_PCRREACTIONSET, Itr, Var)

/// EDIT_EACH_PCRREACTION_IN_PCRREACTIONSET

#define EDIT_EACH_PCRREACTION_IN_PCRREACTIONSET(Itr, Var) \
    EDIT_EACH (PCRREACTION_IN_PCRREACTIONSET, Itr, Var)

/// ERASE_PCRREACTION_IN_PCRREACTIONSET

#define ERASE_PCRREACTION_IN_PCRREACTIONSET(Itr, Var) \
    LIST_ERASE_ITEM (PCRREACTION_IN_PCRREACTIONSET, Itr, Var)

/// REMOVE_IF_EMPTY_PCRREACTION_IN_PCRREACTIONSET

#define REMOVE_IF_EMPTY_PCRREACTION_IN_PCRREACTIONSET(Var) \
    REMOVE_IF_EMPTY_FIELD(PCRREACTION_IN_PCRREACTIONSET, Var)

/// UNIQUE_WITHOUT_SORT_PCRREACTION_IN_PCRREACTIONSET

#define UNIQUE_WITHOUT_SORT_PCRREACTION_IN_PCRREACTIONSET(Var, FuncType) \
UNIQUE_WITHOUT_SORT( PCRREACTION_IN_PCRREACTIONSET, Var, FuncType, \
    CCleanupChange::eChangePCRPrimers )

///
/// CPCRReaction macros

#define PCRPRIMER_IN_PCRPRIMERSET_Type       CPCRPrimerSet::Tdata
#define PCRPRIMER_IN_PCRPRIMERSET_Test(Var)  ( (Var).IsSet() && ! (Var).Get().empty() )
#define PCRPRIMER_IN_PCRPRIMERSET_Get(Var)   (Var).Get()
#define PCRPRIMER_IN_PCRPRIMERSET_Set(Var)   (Var).Set()
#define PCRPRIMER_IN_PCRPRIMERSET_Reset(Var) (Var).Reset()

/// FOR_EACH_PCRPRIMER_IN_PCRPRIMERSET

#define FOR_EACH_PCRPRIMER_IN_PCRPRIMERSET(Itr, Var) \
    FOR_EACH (PCRPRIMER_IN_PCRPRIMERSET, Itr, Var)

/// EDIT_EACH_PCRPRIMER_IN_PCRPRIMERSET

#define EDIT_EACH_PCRPRIMER_IN_PCRPRIMERSET(Itr, Var) \
    EDIT_EACH (PCRPRIMER_IN_PCRPRIMERSET, Itr, Var)

/// ERASE_PCRPRIMER_IN_PCRPRIMERSET

#define ERASE_PCRPRIMER_IN_PCRPRIMERSET(Itr, Var) \
    LIST_ERASE_ITEM (PCRPRIMER_IN_PCRPRIMERSET, Itr, Var)

/// UNIQUE_WITHOUT_SORT_PCRREACTION_IN_PCRREACTIONSET

#define UNIQUE_WITHOUT_SORT_PCRPRIMER_IN_PCRPRIMERSET(Var, FuncType) \
UNIQUE_WITHOUT_SORT( PCRPRIMER_IN_PCRPRIMERSET, Var, FuncType, \
    CCleanupChange::eChangePCRPrimers )

/// REMOVE_IF_EMPTY_PCRPRIMER_IN_PCRPRIMERSET

#define REMOVE_IF_EMPTY_PCRPRIMER_IN_PCRPRIMERSET(Var) \
    REMOVE_IF_EMPTY_FIELD(PCRPRIMER_IN_PCRPRIMERSET, Var)


END_SCOPE(objects)
END_NCBI_SCOPE


/* @} */

#endif  /* OBJECTS_SEQFEAT___SEQFEAT_MACROS__HPP */
