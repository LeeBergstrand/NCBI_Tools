/*
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
 * Author:  Jonathan Kans
 *
 * File Description:
 *   Feature table reader
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbithr.hpp>

#include <util/static_map.hpp>

#include <serial/iterator.hpp>
#include <serial/objistrasn.hpp>

// Objects includes
#include <objects/general/Int_fuzz.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/general/User_object.hpp>
#include <objects/general/User_field.hpp>
#include <objects/general/Dbtag.hpp>

#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <objects/seqloc/Seq_point.hpp>

#include <objects/seq/Seq_annot.hpp>
#include <objects/seq/Annotdesc.hpp>
#include <objects/seq/Annot_descr.hpp>
#include <objects/pub/Pub.hpp>
#include <objects/pub/Pub_equiv.hpp>
#include <objects/seq/Pubdesc.hpp>
#include <objects/seqfeat/SeqFeatData.hpp>
#include <objects/seq/seq_loc_from_string.hpp>

#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqfeat/BioSource.hpp>
#include <objects/seqfeat/Org_ref.hpp>
#include <objects/seqfeat/OrgName.hpp>
#include <objects/seqfeat/SubSource.hpp>
#include <objects/seqfeat/OrgMod.hpp>
#include <objects/seqfeat/Gene_ref.hpp>
#include <objects/seqfeat/Cdregion.hpp>
#include <objects/seqfeat/Code_break.hpp>
#include <objects/seqfeat/Genetic_code.hpp>
#include <objects/seqfeat/Genetic_code_table.hpp>
#include <objects/seqfeat/RNA_ref.hpp>
#include <objects/seqfeat/Trna_ext.hpp>
#include <objects/seqfeat/RNA_gen.hpp>
#include <objects/seqfeat/RNA_qual_set.hpp>
#include <objects/seqfeat/RNA_qual.hpp>
#include <objects/seqfeat/Imp_feat.hpp>
#include <objects/seqfeat/Gb_qual.hpp>

#include <objects/seqset/Bioseq_set.hpp>
#include <objects/seqset/Seq_entry.hpp>

#include <objtools/readers/readfeat.hpp>
#include <objtools/readers/table_filter.hpp>
#include <objtools/error_codes.hpp>

#include <algorithm>

#include <objtools/readers/error_container.hpp>

#include "best_feat_finder.hpp"

#define NCBI_USE_ERRCODE_X   Objtools_Rd_Feature

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class /* NCBI_XOBJREAD_EXPORT */ CFeature_table_reader_imp
{
public:
    enum EQual {
        eQual_allele,
        eQual_anticodon,
        eQual_bac_ends,
        eQual_bond_type,
        eQual_bound_moiety,
        eQual_chrcnt,
        eQual_citation,
        eQual_clone,
        eQual_clone_id,
        eQual_codon_recognized,
        eQual_codon_start,
        eQual_compare,
        eQual_cons_splice,
        eQual_ctgcnt,
        eQual_cyt_map,
        eQual_db_xref,
        eQual_direction,
        eQual_EC_number,
        eQual_estimated_length,
        eQual_evidence,
        eQual_exception,
        eQual_experiment,
        eQual_frequency,
        eQual_function,
        eQual_gap_type,
        eQual_gen_map,
        eQual_gene,
        eQual_gene_desc,
        eQual_gene_syn,
        eQual_go_component,
        eQual_go_function,
        eQual_go_process,
        eQual_heterogen,
        eQual_inference,
        eQual_insertion_seq,
        eQual_label,
        eQual_linkage_evidence,
        eQual_loccnt,
        eQual_locus_tag,
        eQual_macronuclear,
        eQual_map,
        eQual_MEDLINE,
        eQual_method,
        eQual_mobile_element_type,
        eQual_mod_base,
        eQual_muid,
        eQual_ncRNA_class,
        eQual_nomenclature,
        eQual_note,
        eQual_number,
        eQual_old_locus_tag,
        eQual_operon,
        eQual_organism,
        eQual_partial,
        eQual_PCR_conditions,
        eQual_phenotype,
        eQual_pmid,
        eQual_product,
        eQual_prot_desc,
        eQual_prot_note,
        eQual_protein_id,
        eQual_pseudo,
        eQual_pseudogene,
        eQual_PubMed,
        eQual_rad_map,
        eQual_region_name,
        eQual_replace,
        eQual_ribosomal_slippage,
        eQual_rpt_family,
        eQual_rpt_type,
        eQual_rpt_unit,
        eQual_rpt_unit_range,
        eQual_rpt_unit_seq,
        eQual_satellite,
        eQual_sec_str_type,
        eQual_secondary_accession,
        eQual_sequence,
        eQual_site_type,
        eQual_snp_class,
        eQual_snp_gtype,
        eQual_snp_het,
        eQual_snp_het_se,
        eQual_snp_linkout,
        eQual_snp_maxrate,
        eQual_snp_valid,
        eQual_standard_name,
        eQual_STS,
        eQual_sts_aliases,
        eQual_sts_dsegs,
        eQual_tag_peptide,
        eQual_trans_splicing,
        eQual_transcript_id,
        eQual_transcription,
        eQual_transl_except,
        eQual_transl_table,
        eQual_translation,
        eQual_transposon,
        eQual_usedin,
        eQual_weight
    };

    enum EOrgRef {
        eOrgRef_organism,
        eOrgRef_organelle,
        eOrgRef_div,
        eOrgRef_lineage,
        eOrgRef_gcode,
        eOrgRef_mgcode
    };

    // constructor
    CFeature_table_reader_imp(void);
    // destructor
    ~CFeature_table_reader_imp(void);

    // read 5-column feature table and return Seq-annot
    CRef<CSeq_annot> ReadSequinFeatureTable (ILineReader& reader,
                                             const string& seqid,
                                             const string& annotname,
                                             const CFeature_table_reader::TFlags flags, 
                                             IErrorContainer* container,
                                             ITableFilter *filter);

    // create single feature from key
    CRef<CSeq_feat> CreateSeqFeat (const string& feat,
                                   CSeq_loc& location,
                                   const CFeature_table_reader::TFlags flags, 
                                   IErrorContainer* container,
                                   unsigned int line,
                                   const string &seq_id,
                                   ITableFilter *filter);

    // add single qualifier to feature
    void AddFeatQual (CRef<CSeq_feat> sfp,
                      const string& feat_name,
                      const string& qual,
                      const string& val,
                      const CFeature_table_reader::TFlags flags,
                      IErrorContainer* container,
                      int line,
                      const string &seq_id );

private:

    // Prohibit copy constructor and assignment operator
    CFeature_table_reader_imp(const CFeature_table_reader_imp& value);
    CFeature_table_reader_imp& operator=(const CFeature_table_reader_imp& value);

    bool x_ParseFeatureTableLine (const string& line, Int4* startP, Int4* stopP,
                                  bool* partial5P, bool* partial3P, bool* ispointP, bool* isminusP,
                                  string& featP, string& qualP, string& valP, Int4 offset,
                                  IErrorContainer *container, int line_num, const string &seq_id );

    bool x_IsWebComment(CTempString line);

    bool x_AddIntervalToFeature (CTempString strFeatureName, CRef<CSeq_feat> sfp, CSeq_loc_mix& mix,
                                 Int4 start, Int4 stop,
                                 bool partial5, bool partial3, bool ispoint, bool isminus,
                                 IErrorContainer *container, int line_num, const string& seqid);

    bool x_AddQualifierToFeature (CRef<CSeq_feat> sfp,
        const string &feat_name,
        const string& qual, const string& val,
        IErrorContainer *container, int line_num, const string &seq_id );

    bool x_AddQualifierToGene     (CSeqFeatData& sfdata,
                                   EQual qtype, const string& val);
    bool x_AddQualifierToCdregion (CRef<CSeq_feat> sfp, CSeqFeatData& sfdata,
                                   EQual qtype, const string& val,
                                   IErrorContainer *container, int line_num, const string &seq_id );
    bool x_AddQualifierToRna      (CSeqFeatData& sfdata,
                                   EQual qtype, const string& val,
                                   IErrorContainer *container, int line_num, const string &seq_id );
    bool x_AddQualifierToImp      (CRef<CSeq_feat> sfp, CSeqFeatData& sfdata,
                                   EQual qtype, const string& qual, const string& val);
    bool x_AddQualifierToBioSrc   (CSeqFeatData& sfdata,
                                   const string &feat_name,
                                   EOrgRef rtype, const string& val,
                                   IErrorContainer *container, int line, const string &seq_id );
    bool x_AddQualifierToBioSrc   (CSeqFeatData& sfdata,
                                   CSubSource::ESubtype stype, const string& val);
    bool x_AddQualifierToBioSrc   (CSeqFeatData& sfdata,
                                   COrgMod::ESubtype mtype, const string& val);

    bool x_AddGBQualToFeature    (CRef<CSeq_feat> sfp,
                                  const string& qual, const string& val);

    bool x_AddGeneOntologyToFeature (CRef<CSeq_feat> sfp, 
                                     const string& qual, const string& val);

    bool x_StringIsJustQuotes (const string& str);

    int x_ParseTrnaString (const string& val);

    bool x_ParseTrnaExtString(
        CTrna_ext & ext_trna, const string & str, const CSeq_id *seq_id );
    SIZE_TYPE x_MatchingParenPos( const string &str, SIZE_TYPE open_paren_pos );

    long x_StringToLongNoThrow (
        CTempString strToConvert,
        IErrorContainer *container, 
        const std::string& strSeqId,
        unsigned int uLine,
        CTempString strFeatureName,
        CTempString strQualifierName,
        // user can override the default problem types that are set on error
        ILineError::EProblem eProblem = ILineError::eProblem_Unset
    );

    bool x_SetupSeqFeat (CRef<CSeq_feat> sfp, const string& feat,
                         const CFeature_table_reader::TFlags flags, 
                         unsigned int line,
                         const string &seq_id,
                         IErrorContainer* container,
                         ITableFilter *filter);

    void  x_ProcessMsg (
        IErrorContainer* container,
        ILineError::EProblem eProblem,
        EDiagSev eSeverity,
        const std::string& strSeqId,
        unsigned int uLine,
        const std::string & strFeatureName = string(""),
        const std::string & strQualifierName = string(""),
        const std::string & strQualifierValue = string("")  );

    void x_TokenizeStrict( const string &line, vector<string> &out_tokens );
    void x_TokenizeLenient( const string &line, vector<string> &out_tokens );
};

auto_ptr<CFeature_table_reader_imp> CFeature_table_reader::sm_Implementation;

void CFeature_table_reader::x_InitImplementation()
{
    DEFINE_STATIC_FAST_MUTEX(s_Implementation_mutex);

    CFastMutexGuard   LOCK(s_Implementation_mutex);
    if ( !sm_Implementation.get() ) {
        sm_Implementation.reset(new CFeature_table_reader_imp());
    }
}


typedef SStaticPair<const char *, CSeqFeatData::ESubtype> TFeatKey;

static const TFeatKey feat_key_to_subtype [] = {
    {  "-10_signal",         CSeqFeatData::eSubtype_10_signal           },
    {  "-35_signal",         CSeqFeatData::eSubtype_35_signal           },
    {  "3'UTR",              CSeqFeatData::eSubtype_3UTR                },
    {  "3'clip",             CSeqFeatData::eSubtype_3clip               },
    {  "5'UTR",              CSeqFeatData::eSubtype_5UTR                },
    {  "5'clip",             CSeqFeatData::eSubtype_5clip               },
    {  "Bond",               CSeqFeatData::eSubtype_bond                },
    {  "CAAT_signal",        CSeqFeatData::eSubtype_CAAT_signal         },
    {  "CDS",                CSeqFeatData::eSubtype_cdregion            },
    {  "C_region",           CSeqFeatData::eSubtype_C_region            },
    {  "Cit",                CSeqFeatData::eSubtype_pub                 },
    {  "CloneRef",           CSeqFeatData::eSubtype_clone               },
    {  "Comment",            CSeqFeatData::eSubtype_comment             },
    {  "D-loop",             CSeqFeatData::eSubtype_D_loop              },
    {  "D_segment",          CSeqFeatData::eSubtype_D_segment           },
    {  "GC_signal",          CSeqFeatData::eSubtype_GC_signal           },
    {  "Het",                CSeqFeatData::eSubtype_het                 },
    {  "J_segment",          CSeqFeatData::eSubtype_J_segment           },
    {  "LTR",                CSeqFeatData::eSubtype_LTR                 },
    {  "N_region",           CSeqFeatData::eSubtype_N_region            },
    {  "NonStdRes",          CSeqFeatData::eSubtype_non_std_residue     },
    {  "Num",                CSeqFeatData::eSubtype_num                 },
    {  "Protein",            CSeqFeatData::eSubtype_prot                },
    {  "RBS",                CSeqFeatData::eSubtype_RBS                 },
    {  "REFERENCE",          CSeqFeatData::eSubtype_pub                 },
    {  "Region",             CSeqFeatData::eSubtype_region              },
    {  "Rsite",              CSeqFeatData::eSubtype_rsite               },
    {  "STS",                CSeqFeatData::eSubtype_STS                 },
    {  "S_region",           CSeqFeatData::eSubtype_S_region            },
    {  "SecStr",             CSeqFeatData::eSubtype_psec_str            },
    {  "Site",               CSeqFeatData::eSubtype_site                },
    {  "Site-ref",           CSeqFeatData::eSubtype_site_ref            },
    {  "Src",                CSeqFeatData::eSubtype_biosrc              },
    {  "TATA_signal",        CSeqFeatData::eSubtype_TATA_signal         },
    {  "TxInit",             CSeqFeatData::eSubtype_txinit              },
    {  "User",               CSeqFeatData::eSubtype_user                },
    {  "V_region",           CSeqFeatData::eSubtype_V_region            },
    {  "V_segment",          CSeqFeatData::eSubtype_V_segment           },
    {  "VariationRef",       CSeqFeatData::eSubtype_variation_ref       },
    {  "Xref",               CSeqFeatData::eSubtype_seq                 },
    {  "assembly_gap",       CSeqFeatData::eSubtype_assembly_gap        },
    {  "attenuator",         CSeqFeatData::eSubtype_attenuator          },
    {  "centromere",         CSeqFeatData::eSubtype_centromere          },
    {  "conflict",           CSeqFeatData::eSubtype_conflict            },
    {  "enhancer",           CSeqFeatData::eSubtype_enhancer            },
    {  "exon",               CSeqFeatData::eSubtype_exon                },
    {  "gap",                CSeqFeatData::eSubtype_gap                 },
    {  "gene",               CSeqFeatData::eSubtype_gene                },
    {  "iDNA",               CSeqFeatData::eSubtype_iDNA                },
    {  "intron",             CSeqFeatData::eSubtype_intron              },
    {  "mRNA",               CSeqFeatData::eSubtype_mRNA                },
    {  "mat_peptide",        CSeqFeatData::eSubtype_mat_peptide_aa      },
    {  "mat_peptide_nt",     CSeqFeatData::eSubtype_mat_peptide         },
    {  "misc_RNA",           CSeqFeatData::eSubtype_otherRNA            },
    {  "misc_binding",       CSeqFeatData::eSubtype_misc_binding        },
    {  "misc_difference",    CSeqFeatData::eSubtype_misc_difference     },
    {  "misc_feature",       CSeqFeatData::eSubtype_misc_feature        },
    {  "misc_recomb",        CSeqFeatData::eSubtype_misc_recomb         },
    {  "misc_signal",        CSeqFeatData::eSubtype_misc_signal         },
    {  "misc_structure",     CSeqFeatData::eSubtype_misc_structure      },
    {  "mobile_element",     CSeqFeatData::eSubtype_mobile_element      },
    {  "modified_base",      CSeqFeatData::eSubtype_modified_base       },
    {  "ncRNA",              CSeqFeatData::eSubtype_ncRNA               },
    {  "old_sequence",       CSeqFeatData::eSubtype_old_sequence        },
    {  "operon",             CSeqFeatData::eSubtype_operon              },
    {  "oriT",               CSeqFeatData::eSubtype_oriT                },
    {  "polyA_signal",       CSeqFeatData::eSubtype_polyA_signal        },
    {  "polyA_site",         CSeqFeatData::eSubtype_polyA_site          },
    {  "pre_RNA",            CSeqFeatData::eSubtype_preRNA              },
    {  "precursor_RNA",      CSeqFeatData::eSubtype_preRNA              },
    {  "preprotein",         CSeqFeatData::eSubtype_preprotein          },
    {  "prim_transcript",    CSeqFeatData::eSubtype_prim_transcript     },
    {  "primer_bind",        CSeqFeatData::eSubtype_primer_bind         },
    {  "promoter",           CSeqFeatData::eSubtype_promoter            },
    {  "protein_bind",       CSeqFeatData::eSubtype_protein_bind        },
    {  "rRNA",               CSeqFeatData::eSubtype_rRNA                },
    {  "rep_origin",         CSeqFeatData::eSubtype_rep_origin          },
    {  "repeat_region",      CSeqFeatData::eSubtype_repeat_region       },
    {  "repeat_unit",        CSeqFeatData::eSubtype_repeat_unit         },
    {  "satellite",          CSeqFeatData::eSubtype_satellite           },
    {  "scRNA",              CSeqFeatData::eSubtype_scRNA               },
    {  "sig_peptide",        CSeqFeatData::eSubtype_sig_peptide_aa      },
    {  "sig_peptide_nt",     CSeqFeatData::eSubtype_sig_peptide         },
    {  "snRNA",              CSeqFeatData::eSubtype_snRNA               },
    {  "snoRNA",             CSeqFeatData::eSubtype_snoRNA              },
    {  "source",             CSeqFeatData::eSubtype_biosrc              },
    {  "stem_loop",          CSeqFeatData::eSubtype_stem_loop           },
    {  "tRNA",               CSeqFeatData::eSubtype_tRNA                },
    {  "telomere",           CSeqFeatData::eSubtype_telomere            },
    {  "terminator",         CSeqFeatData::eSubtype_terminator          },
    {  "tmRNA",              CSeqFeatData::eSubtype_tmRNA               },
    {  "transit_peptide",    CSeqFeatData::eSubtype_transit_peptide_aa  },
    {  "transit_peptide_nt", CSeqFeatData::eSubtype_transit_peptide     },
    {  "unsure",             CSeqFeatData::eSubtype_unsure              },
    {  "variation",          CSeqFeatData::eSubtype_variation           },
    {  "virion",             CSeqFeatData::eSubtype_virion              }
};

typedef CStaticPairArrayMap <const char*, CSeqFeatData::ESubtype, PCase_CStr> TFeatMap;
DEFINE_STATIC_ARRAY_MAP(TFeatMap, sm_FeatKeys, feat_key_to_subtype);


typedef SStaticPair<const char *, CFeature_table_reader_imp::EQual> TQualKey;

static const TQualKey qual_key_to_subtype [] = {
    {  "EC_number",            CFeature_table_reader_imp::eQual_EC_number             },
    {  "PCR_conditions",       CFeature_table_reader_imp::eQual_PCR_conditions        },
    {  "PubMed",               CFeature_table_reader_imp::eQual_PubMed                },
    {  "STS",                  CFeature_table_reader_imp::eQual_STS                   },
    {  "allele",               CFeature_table_reader_imp::eQual_allele                },
    {  "anticodon",            CFeature_table_reader_imp::eQual_anticodon             },
    {  "bac_ends",             CFeature_table_reader_imp::eQual_bac_ends              },
    {  "bond_type",            CFeature_table_reader_imp::eQual_bond_type             },
    {  "bound_moiety",         CFeature_table_reader_imp::eQual_bound_moiety          },
    {  "chrcnt",               CFeature_table_reader_imp::eQual_chrcnt                },
    {  "citation",             CFeature_table_reader_imp::eQual_citation              },
    {  "clone",                CFeature_table_reader_imp::eQual_clone                 },
    {  "clone_id",             CFeature_table_reader_imp::eQual_clone_id              },
    {  "codon_recognized",     CFeature_table_reader_imp::eQual_codon_recognized      },
    {  "codon_start",          CFeature_table_reader_imp::eQual_codon_start           },
    {  "codons_recognized",    CFeature_table_reader_imp::eQual_codon_recognized      },
    {  "compare",              CFeature_table_reader_imp::eQual_compare               },
    {  "cons_splice",          CFeature_table_reader_imp::eQual_cons_splice           },
    {  "ctgcnt",               CFeature_table_reader_imp::eQual_ctgcnt                },
    {  "cyt_map",              CFeature_table_reader_imp::eQual_cyt_map               },
    {  "db_xref",              CFeature_table_reader_imp::eQual_db_xref               },
    {  "direction",            CFeature_table_reader_imp::eQual_direction             },
    {  "estimated_length",     CFeature_table_reader_imp::eQual_estimated_length      },
    {  "evidence",             CFeature_table_reader_imp::eQual_evidence              },
    {  "exception",            CFeature_table_reader_imp::eQual_exception             },
    {  "experiment",           CFeature_table_reader_imp::eQual_experiment            },
    {  "frequency",            CFeature_table_reader_imp::eQual_frequency             },
    {  "function",             CFeature_table_reader_imp::eQual_function              },
    {  "gap_type",             CFeature_table_reader_imp::eQual_gap_type              },
    {  "gen_map",              CFeature_table_reader_imp::eQual_gen_map               },
    {  "gene",                 CFeature_table_reader_imp::eQual_gene                  },
    {  "gene_desc",            CFeature_table_reader_imp::eQual_gene_desc             },
    {  "gene_syn",             CFeature_table_reader_imp::eQual_gene_syn              },
    {  "gene_synonym",         CFeature_table_reader_imp::eQual_gene_syn              },
    {  "go_component",         CFeature_table_reader_imp::eQual_go_component          },
    {  "go_function",          CFeature_table_reader_imp::eQual_go_function           },
    {  "go_process",           CFeature_table_reader_imp::eQual_go_process            },
    {  "heterogen",            CFeature_table_reader_imp::eQual_heterogen             },
    {  "inference",            CFeature_table_reader_imp::eQual_inference             },
    {  "insertion_seq",        CFeature_table_reader_imp::eQual_insertion_seq         },
    {  "label",                CFeature_table_reader_imp::eQual_label                 },
    {  "linkage_evidence",     CFeature_table_reader_imp::eQual_linkage_evidence      },
    {  "loccnt",               CFeature_table_reader_imp::eQual_loccnt                },
    {  "locus_tag",            CFeature_table_reader_imp::eQual_locus_tag             },
    {  "macronuclear",         CFeature_table_reader_imp::eQual_macronuclear          },
    {  "map",                  CFeature_table_reader_imp::eQual_map                   },
    {  "method",               CFeature_table_reader_imp::eQual_method                },
    {  "mobile_element_type",  CFeature_table_reader_imp::eQual_mobile_element_type   },
    {  "mod_base",             CFeature_table_reader_imp::eQual_mod_base              },
    {  "ncRNA_class",          CFeature_table_reader_imp::eQual_ncRNA_class           },
    {  "nomenclature",         CFeature_table_reader_imp::eQual_nomenclature          },
    {  "note",                 CFeature_table_reader_imp::eQual_note                  },
    {  "number",               CFeature_table_reader_imp::eQual_number                },
    {  "old_locus_tag",        CFeature_table_reader_imp::eQual_old_locus_tag         },
    {  "operon",               CFeature_table_reader_imp::eQual_operon                },
    {  "organism",             CFeature_table_reader_imp::eQual_organism              },
    {  "partial",              CFeature_table_reader_imp::eQual_partial               },
    {  "phenotype",            CFeature_table_reader_imp::eQual_phenotype             },
    {  "product",              CFeature_table_reader_imp::eQual_product               },
    {  "prot_desc",            CFeature_table_reader_imp::eQual_prot_desc             },
    {  "prot_note",            CFeature_table_reader_imp::eQual_prot_note             },
    {  "protein_id",           CFeature_table_reader_imp::eQual_protein_id            },
    {  "pseudo",               CFeature_table_reader_imp::eQual_pseudo                },
    {  "pseudogene",           CFeature_table_reader_imp::eQual_pseudogene            },
    {  "rad_map",              CFeature_table_reader_imp::eQual_rad_map               },
    {  "replace",              CFeature_table_reader_imp::eQual_replace               },
    {  "ribosomal_slippage",   CFeature_table_reader_imp::eQual_ribosomal_slippage    },
    {  "rpt_family",           CFeature_table_reader_imp::eQual_rpt_family            },
    {  "rpt_type",             CFeature_table_reader_imp::eQual_rpt_type              },
    {  "rpt_unit",             CFeature_table_reader_imp::eQual_rpt_unit              },
    {  "rpt_unit_range",       CFeature_table_reader_imp::eQual_rpt_unit_range        },
    {  "rpt_unit_seq",         CFeature_table_reader_imp::eQual_rpt_unit_seq          },
    {  "satellite",            CFeature_table_reader_imp::eQual_satellite             },
    {  "sec_str_type",         CFeature_table_reader_imp::eQual_sec_str_type          },
    {  "secondary_accession",  CFeature_table_reader_imp::eQual_secondary_accession   },
    {  "secondary_accessions", CFeature_table_reader_imp::eQual_secondary_accession   },
    {  "sequence",             CFeature_table_reader_imp::eQual_sequence              },
    {  "site_type",            CFeature_table_reader_imp::eQual_site_type             },
    {  "snp_class",            CFeature_table_reader_imp::eQual_snp_class             },
    {  "snp_gtype",            CFeature_table_reader_imp::eQual_snp_gtype             },
    {  "snp_het",              CFeature_table_reader_imp::eQual_snp_het               },
    {  "snp_het_se",           CFeature_table_reader_imp::eQual_snp_het_se            },
    {  "snp_linkout",          CFeature_table_reader_imp::eQual_snp_linkout           },
    {  "snp_maxrate",          CFeature_table_reader_imp::eQual_snp_maxrate           },
    {  "snp_valid",            CFeature_table_reader_imp::eQual_snp_valid             },
    {  "standard_name",        CFeature_table_reader_imp::eQual_standard_name         },
    {  "sts_aliases",          CFeature_table_reader_imp::eQual_sts_aliases           },
    {  "sts_dsegs",            CFeature_table_reader_imp::eQual_sts_dsegs             },
    {  "tag_peptide",          CFeature_table_reader_imp::eQual_tag_peptide           },
    {  "trans_splicing",       CFeature_table_reader_imp::eQual_trans_splicing        },
    {  "transcript_id",        CFeature_table_reader_imp::eQual_transcript_id         },
    {  "transcription",        CFeature_table_reader_imp::eQual_transcription         },
    {  "transl_except",        CFeature_table_reader_imp::eQual_transl_except         },
    {  "transl_table",         CFeature_table_reader_imp::eQual_transl_table          },
    {  "translation",          CFeature_table_reader_imp::eQual_translation           },
    {  "transposon",           CFeature_table_reader_imp::eQual_transposon            },
    {  "usedin",               CFeature_table_reader_imp::eQual_usedin                },
    {  "weight",               CFeature_table_reader_imp::eQual_weight                }
};

typedef CStaticPairArrayMap <const char*, CFeature_table_reader_imp::EQual, PCase_CStr> TQualMap;
DEFINE_STATIC_ARRAY_MAP(TQualMap, sm_QualKeys, qual_key_to_subtype);


typedef SStaticPair<const char *, CFeature_table_reader_imp::EOrgRef> TOrgRefKey;

static const TOrgRefKey orgref_key_to_subtype [] = {
    {  "div",        CFeature_table_reader_imp::eOrgRef_div        },
    {  "gcode",      CFeature_table_reader_imp::eOrgRef_gcode      },
    {  "lineage",    CFeature_table_reader_imp::eOrgRef_lineage    },
    {  "mgcode",     CFeature_table_reader_imp::eOrgRef_mgcode     },
    {  "organelle",  CFeature_table_reader_imp::eOrgRef_organelle  },
    {  "organism",   CFeature_table_reader_imp::eOrgRef_organism   }
};

typedef CStaticPairArrayMap <const char*, CFeature_table_reader_imp::EOrgRef, PCase_CStr> TOrgRefMap;
DEFINE_STATIC_ARRAY_MAP(TOrgRefMap, sm_OrgRefKeys, orgref_key_to_subtype);


typedef SStaticPair<const char *, CBioSource::EGenome> TGenomeKey;

static const TGenomeKey genome_key_to_subtype [] = {
    {  "apicoplast",                CBioSource::eGenome_apicoplast        },
    {  "chloroplast",               CBioSource::eGenome_chloroplast       },
    {  "chromatophore",             CBioSource::eGenome_chromatophore     },
    {  "chromoplast",               CBioSource::eGenome_chromoplast       },
    {  "chromosome",                CBioSource::eGenome_chromosome        },
    {  "cyanelle",                  CBioSource::eGenome_cyanelle          },
    {  "endogenous_virus",          CBioSource::eGenome_endogenous_virus  },
    {  "extrachrom",                CBioSource::eGenome_extrachrom        },
    {  "genomic",                   CBioSource::eGenome_genomic           },
    {  "hydrogenosome",             CBioSource::eGenome_hydrogenosome     },
    {  "insertion_seq",             CBioSource::eGenome_insertion_seq     },
    {  "kinetoplast",               CBioSource::eGenome_kinetoplast       },
    {  "leucoplast",                CBioSource::eGenome_leucoplast        },
    {  "macronuclear",              CBioSource::eGenome_macronuclear      },
    {  "mitochondrion",             CBioSource::eGenome_mitochondrion     },
    {  "mitochondrion:kinetoplast", CBioSource::eGenome_kinetoplast       },
    {  "nucleomorph",               CBioSource::eGenome_nucleomorph       },
    {  "plasmid",                   CBioSource::eGenome_plasmid           },
    {  "plastid",                   CBioSource::eGenome_plastid           },
    {  "plastid:apicoplast",        CBioSource::eGenome_apicoplast        },
    {  "plastid:chloroplast",       CBioSource::eGenome_chloroplast       },
    {  "plastid:chromoplast",       CBioSource::eGenome_chromoplast       },
    {  "plastid:cyanelle",          CBioSource::eGenome_cyanelle          },
    {  "plastid:leucoplast",        CBioSource::eGenome_leucoplast        },
    {  "plastid:proplastid",        CBioSource::eGenome_proplastid        },
    {  "proplastid",                CBioSource::eGenome_proplastid        },
    {  "proviral",                  CBioSource::eGenome_proviral          },
    {  "transposon",                CBioSource::eGenome_transposon        },
    {  "unknown",                   CBioSource::eGenome_unknown           },
    {  "virion",                    CBioSource::eGenome_virion            }
};

typedef CStaticPairArrayMap <const char*, CBioSource::EGenome, PCase_CStr> TGenomeMap;
DEFINE_STATIC_ARRAY_MAP(TGenomeMap, sm_GenomeKeys, genome_key_to_subtype);


typedef SStaticPair<const char *, CSubSource::ESubtype> TSubSrcKey;

static const TSubSrcKey subsrc_key_to_subtype [] = {
    {  "altitude",             CSubSource::eSubtype_altitude               },
    {  "cell_line",            CSubSource::eSubtype_cell_line              },
    {  "cell_type",            CSubSource::eSubtype_cell_type              },
    {  "chromosome",           CSubSource::eSubtype_chromosome             },
    {  "clone",                CSubSource::eSubtype_clone                  },
    {  "clone_lib",            CSubSource::eSubtype_clone_lib              },
    {  "collected_by",         CSubSource::eSubtype_collected_by           },
    {  "collection_date",      CSubSource::eSubtype_collection_date        },
    {  "country",              CSubSource::eSubtype_country                },
    {  "dev_stage",            CSubSource::eSubtype_dev_stage              },
    {  "endogenous_virus",     CSubSource::eSubtype_endogenous_virus_name  },
    {  "environmental_sample", CSubSource::eSubtype_environmental_sample   },
    {  "frequency",            CSubSource::eSubtype_frequency              },
    {  "fwd_primer_name",      CSubSource::eSubtype_fwd_primer_name        },
    {  "fwd_primer_seq",       CSubSource::eSubtype_fwd_primer_seq         },
    {  "genotype",             CSubSource::eSubtype_genotype               },
    {  "germline",             CSubSource::eSubtype_germline               },
    {  "haplotype",            CSubSource::eSubtype_haplotype              },
    {  "identified_by",        CSubSource::eSubtype_identified_by          },
    {  "insertion_seq",        CSubSource::eSubtype_insertion_seq_name     },
    {  "isolation_source",     CSubSource::eSubtype_isolation_source       },
    {  "lab_host",             CSubSource::eSubtype_lab_host               },
    {  "lat_lon",              CSubSource::eSubtype_lat_lon                },
    {  "map",                  CSubSource::eSubtype_map                    },
    {  "metagenomic",          CSubSource::eSubtype_metagenomic            },
    {  "plasmid",              CSubSource::eSubtype_plasmid_name           },
    {  "plastid",              CSubSource::eSubtype_plastid_name           },
    {  "pop_variant",          CSubSource::eSubtype_pop_variant            },
    {  "rearranged",           CSubSource::eSubtype_rearranged             },
    {  "rev_primer_name",      CSubSource::eSubtype_rev_primer_name        },
    {  "rev_primer_seq",       CSubSource::eSubtype_rev_primer_seq         },
    {  "segment",              CSubSource::eSubtype_segment                },
    {  "sex",                  CSubSource::eSubtype_sex                    },
    {  "subclone",             CSubSource::eSubtype_subclone               },
    {  "tissue_lib ",          CSubSource::eSubtype_tissue_lib             },
    {  "tissue_type",          CSubSource::eSubtype_tissue_type            },
    {  "transgenic",           CSubSource::eSubtype_transgenic             },
    {  "transposon",           CSubSource::eSubtype_transposon_name        }
};

typedef CStaticPairArrayMap <const char*, CSubSource::ESubtype, PCase_CStr> TSubSrcMap;
DEFINE_STATIC_ARRAY_MAP(TSubSrcMap, sm_SubSrcKeys, subsrc_key_to_subtype);


typedef SStaticPair<const char *, COrgMod::ESubtype> TOrgModKey;

static const TOrgModKey orgmod_key_to_subtype [] = {
    {  "acronym",            COrgMod::eSubtype_acronym             },
    {  "anamorph",           COrgMod::eSubtype_anamorph            },
    {  "authority",          COrgMod::eSubtype_authority           },
    {  "bio_material",       COrgMod::eSubtype_bio_material        },
    {  "biotype",            COrgMod::eSubtype_biotype             },
    {  "biovar",             COrgMod::eSubtype_biovar              },
    {  "breed",              COrgMod::eSubtype_breed               },
    {  "chemovar",           COrgMod::eSubtype_chemovar            },
    {  "common",             COrgMod::eSubtype_common              },
    {  "cultivar",           COrgMod::eSubtype_cultivar            },
    {  "culture_collection", COrgMod::eSubtype_culture_collection  },
    {  "dosage",             COrgMod::eSubtype_dosage              },
    {  "ecotype",            COrgMod::eSubtype_ecotype             },
    {  "forma",              COrgMod::eSubtype_forma               },
    {  "forma_specialis",    COrgMod::eSubtype_forma_specialis     },
    {  "gb_acronym",         COrgMod::eSubtype_gb_acronym          },
    {  "gb_anamorph",        COrgMod::eSubtype_gb_anamorph         },
    {  "gb_synonym",         COrgMod::eSubtype_gb_synonym          },
    {  "group",              COrgMod::eSubtype_group               },
    {  "isolate",            COrgMod::eSubtype_isolate             },
    {  "metagenome_source",  COrgMod::eSubtype_metagenome_source   },
    {  "nat_host",           COrgMod::eSubtype_nat_host            },
    {  "natural_host",       COrgMod::eSubtype_nat_host            },
    {  "old_lineage",        COrgMod::eSubtype_old_lineage         },
    {  "old_name",           COrgMod::eSubtype_old_name            },
    {  "pathovar",           COrgMod::eSubtype_pathovar            },
    {  "serogroup",          COrgMod::eSubtype_serogroup           },
    {  "serotype",           COrgMod::eSubtype_serotype            },
    {  "serovar",            COrgMod::eSubtype_serovar             },
    {  "spec_host",          COrgMod::eSubtype_nat_host            },
    {  "specific_host",      COrgMod::eSubtype_nat_host            },
    {  "specimen_voucher",   COrgMod::eSubtype_specimen_voucher    },
    {  "strain",             COrgMod::eSubtype_strain              },
    {  "sub_species",        COrgMod::eSubtype_sub_species         },
    {  "subgroup",           COrgMod::eSubtype_subgroup            },
    {  "substrain",          COrgMod::eSubtype_substrain           },
    {  "subtype",            COrgMod::eSubtype_subtype             },
    {  "synonym",            COrgMod::eSubtype_synonym             },
    {  "teleomorph",         COrgMod::eSubtype_teleomorph          },
    {  "type",               COrgMod::eSubtype_type                },
    {  "variety",            COrgMod::eSubtype_variety             }
};

typedef CStaticPairArrayMap <const char*, COrgMod::ESubtype, PCase_CStr> TOrgModMap;
DEFINE_STATIC_ARRAY_MAP(TOrgModMap, sm_OrgModKeys, orgmod_key_to_subtype);


typedef SStaticPair<const char *, int> TTrnaKey;

static const TTrnaKey trna_key_to_subtype [] = {
    {  "Ala",            'A'  },
    {  "Alanine",        'A'  },
    {  "Arg",            'R'  },
    {  "Arginine",       'R'  },
    {  "Asn",            'N'  },
    {  "Asp",            'D'  },
    {  "Asp or Asn",     'B'  },
    {  "Asparagine",     'N'  },
    {  "Aspartate",      'D'  },
    {  "Aspartic Acid",  'D'  },
    {  "Asx",            'B'  },
    {  "Cys",            'C'  },
    {  "Cysteine",       'C'  },
    {  "Gln",            'Q'  },
    {  "Glu",            'E'  },
    {  "Glu or Gln",     'Z'  },
    {  "Glutamate",      'E'  },
    {  "Glutamic Acid",  'E'  },
    {  "Glutamine",      'Q'  },
    {  "Glx",            'Z'  },
    {  "Gly",            'G'  },
    {  "Glycine",        'G'  },
    {  "His",            'H'  },
    {  "Histidine",      'H'  },
    {  "Ile",            'I'  },
    {  "Isoleucine",     'I'  },
    {  "Leu",            'L'  },
    {  "Leu or Ile",     'J'  },
    {  "Leucine",        'L'  },
    {  "Lys",            'K'  },
    {  "Lysine",         'K'  },
    {  "Met",            'M'  },
    {  "Methionine",     'M'  },
    {  "OTHER",          'X'  },
    {  "Phe",            'F'  },
    {  "Phenylalanine",  'F'  },
    {  "Pro",            'P'  },
    {  "Proline",        'P'  },
    {  "Pyl",            'O'  },
    {  "Pyrrolysine",    'O'  },
    {  "Sec",            'U'  },
    {  "Selenocysteine", 'U'  },
    {  "Ser",            'S'  },
    {  "Serine",         'S'  },
    {  "TERM",           '*'  },
    {  "Ter",            '*'  },
    {  "Termination",    '*'  },
    {  "Thr",            'T'  },
    {  "Threonine",      'T'  },
    {  "Trp",            'W'  },
    {  "Tryptophan",     'W'  },
    {  "Tyr",            'Y'  },
    {  "Tyrosine",       'Y'  },
    {  "Val",            'V'  },
    {  "Valine",         'V'  },
    {  "Xle",            'J'  },
    {  "Xxx",            'X'  },
    {  "fMet",           'M'  }
};

typedef CStaticPairArrayMap <const char*, int, PCase_CStr> TTrnaMap;
DEFINE_STATIC_ARRAY_MAP(TTrnaMap, sm_TrnaKeys, trna_key_to_subtype);


static const char * const single_key_list [] = {
    "environmental_sample",
    "germline",
    "metagenomic",
    "partial",
    "pseudo",
    "rearranged",
    "ribosomal_slippage",
    "trans_splicing",
    "transgenic"
};

typedef CStaticArraySet <const char*, PCase_CStr> TSingleSet;
DEFINE_STATIC_ARRAY_MAP(TSingleSet, sc_SingleKeys, single_key_list);


// constructor
CFeature_table_reader_imp::CFeature_table_reader_imp(void)
{
}

// destructor
CFeature_table_reader_imp::~CFeature_table_reader_imp(void)
{
}


bool CFeature_table_reader_imp::x_ParseFeatureTableLine (
    const string& line,
    Int4* startP,
    Int4* stopP,
    bool* partial5P,
    bool* partial3P,
    bool* ispointP,
    bool* isminusP,
    string& featP,
    string& qualP,
    string& valP,
    Int4 offset,

    IErrorContainer *container, 
    int line_num, 
    const string &seq_id
)

{
    SIZE_TYPE      numtkns;
    bool           badNumber = false;
    bool           isminus = false;
    bool           ispoint = false;
    size_t         len;
    bool           partial5 = false;
    bool           partial3 = false;
    Int4           startv = -1;
    Int4           stopv = -1;
    Int4           swp;
    string         start, stop, feat, qual, val, stnd;
    vector<string> tkns;

    if (line.empty ()) return false;

    /* offset and other instructions encoded in brackets */
    if (NStr::StartsWith (line, '[')) return false;

    tkns.clear ();
    x_TokenizeLenient(line, tkns);
    numtkns = tkns.size ();

    if (numtkns > 0) {
        start = NStr::TruncateSpaces(tkns[0]);
    }
    if (numtkns > 1) {
        stop = NStr::TruncateSpaces(tkns[1]);
    }
    if (numtkns > 2) {
        feat = NStr::TruncateSpaces(tkns[2]);
    }
    if (numtkns > 3) {
        qual = NStr::TruncateSpaces(tkns[3]);
    }
    if (numtkns > 4) {
        val = NStr::TruncateSpaces(tkns[4]);
        // trim enclosing double-quotes
        if( val.length() >= 2 && val[0] == '"' && val[val.length()-1] == '"' ) {
            val = val.substr(1, val.length() - 2);
        }
    }
    if (numtkns > 5) {
        stnd = NStr::TruncateSpaces(tkns[5]);
    }

    if (! start.empty ()) {
        if (start [0] == '<') {
            partial5 = true;
            start.erase (0, 1);
        }
        len = start.length ();
        if (len > 1 && start [len - 1] == '^') {
          ispoint = true;
          start [len - 1] = '\0';
        }
        try {
            startv = x_StringToLongNoThrow(start, container, seq_id, line_num, feat, qual,
                ILineError::eProblem_BadFeatureInterval);
        } catch (...) {
            badNumber = true;
        }
    }

    if (! stop.empty ()) {
        if (stop [0] == '>') {
            partial3 = true;
            stop.erase (0, 1);
        }
        try {
            stopv = x_StringToLongNoThrow (stop, container, seq_id, line_num, feat, qual,
                ILineError::eProblem_BadFeatureInterval);
        } catch (CStringException) {
            badNumber = true;
        }
    }

    if (badNumber) {
        startv = -1;
        stopv = -1;
    } else {
        startv--;
        stopv--;
        if (! stnd.empty ()) {
            if (stnd == "minus" || stnd == "-" || stnd == "complement") {
                if (start < stop) {
                    swp = startv;
                    startv = stopv;
                    stopv = swp;
                }
                isminus = true;
            }
        }
    }

    *startP = startv + offset;
    *stopP = stopv + offset;
    *partial5P = partial5;
    *partial3P = partial3;
    *ispointP = ispoint;
    *isminusP = isminus;
    featP = feat;
    qualP = qual;
    valP = val;

    return true;
}

void CFeature_table_reader_imp::x_TokenizeStrict( 
    const string &line, 
    vector<string> &out_tokens )
{
    out_tokens.clear();

    // each token has spaces before it and a tab or end-of-line after it
    string::size_type startPosOfNextRoundOfTokenization = 0;
    while ( startPosOfNextRoundOfTokenization < line.size() ) {
        const string::size_type posAfterSpaces = line.find_first_not_of( ' ', startPosOfNextRoundOfTokenization );
        if( posAfterSpaces == string::npos ) {
            return;
        }

        string::size_type posOfTab = line.find( '\t', posAfterSpaces );
        if( posOfTab == string::npos ) {
            posOfTab = line.length();
        }

        // The next token is between the spaces and the tab (or end of string)
        out_tokens.push_back(kEmptyStr);
        string &new_token = out_tokens.back();
        copy( line.begin() + posAfterSpaces, line.begin() + posOfTab, back_inserter(new_token) );
        NStr::TruncateSpacesInPlace( new_token );

        startPosOfNextRoundOfTokenization = ( posOfTab + 1 );
    }
}

// since some compilers won't let me use isspace for find_if
class CIsSpace {
public:
    bool operator()( char c ) { return isspace(c); }
};

class CIsNotSpace {
public:
    bool operator()( char c ) { return ! isspace(c); }
};

void CFeature_table_reader_imp::x_TokenizeLenient( 
    const string &line, 
    vector<string> &out_tokens )
{
    out_tokens.clear();

    if( line.empty() ) {
        return;
    }

    // if it starts with whitespace, it must be a qual line, else it's a feature line
    if( isspace(line[0]) ) {
        // In regex form, we're doing something like this:
        // \s+(\S+)(\s+(\S.*))?
        // Where the first is the qual, and the rest is the val
        const string::const_iterator start_of_qual = find_if( line.begin(), line.end(), CIsNotSpace() );
        if( start_of_qual == line.end() ) {
            return;
        }
        const string::const_iterator start_of_whitespace_after_qual = find_if( start_of_qual, line.end(), CIsSpace() );
        const string::const_iterator start_of_val = find_if( start_of_whitespace_after_qual, line.end(), CIsNotSpace() );

        // first 3 are empty
        out_tokens.push_back(kEmptyStr);
        out_tokens.push_back(kEmptyStr);
        out_tokens.push_back(kEmptyStr);

        // then qual
        out_tokens.push_back(kEmptyStr);
        string &qual = out_tokens.back();
        copy( start_of_qual, start_of_whitespace_after_qual, back_inserter(qual) );

        // then val
        if( start_of_val != line.end() ) {
            out_tokens.push_back(kEmptyStr);
            string &val = out_tokens.back();
            copy( start_of_val, line.end(), back_inserter(val) );
            NStr::TruncateSpacesInPlace( val );
        }

    } else {
        // parse a feature line

        // Since we're being lenient, we consider it to be 3 ( or 6 ) parts separated by whitespace
        const string::const_iterator first_column_start = line.begin();
        const string::const_iterator first_whitespace = find_if( first_column_start, line.end(), CIsSpace() );
        const string::const_iterator second_column_start = find_if( first_whitespace, line.end(), CIsNotSpace() );
        const string::const_iterator second_whitespace = find_if( second_column_start, line.end(), CIsSpace() );
        const string::const_iterator third_column_start = find_if( second_whitespace, line.end(), CIsNotSpace() );
        const string::const_iterator third_whitespace = find_if( third_column_start, line.end(), CIsSpace() );
        // columns 4 and 5 are unused on feature lines
        const string::const_iterator sixth_column_start = find_if( third_whitespace, line.end(), CIsNotSpace() );
        const string::const_iterator sixth_whitespace = find_if( sixth_column_start, line.end(), CIsSpace() );

        out_tokens.push_back(kEmptyStr);
        string &first = out_tokens.back();
        copy( first_column_start, first_whitespace, back_inserter(first) );

        out_tokens.push_back(kEmptyStr);
        string &second = out_tokens.back();
        copy( second_column_start, second_whitespace, back_inserter(second) );

        out_tokens.push_back(kEmptyStr);
        string &third = out_tokens.back();
        copy( third_column_start, third_whitespace, back_inserter(third) );

        if( sixth_column_start != line.end() ) {
            // columns 4 and 5 are unused
            out_tokens.push_back(kEmptyStr);
            out_tokens.push_back(kEmptyStr);

            out_tokens.push_back(kEmptyStr);
            string &sixth = out_tokens.back();
            copy( sixth_column_start, sixth_whitespace, back_inserter(sixth) );
        }
    }
}


bool CFeature_table_reader_imp::x_AddQualifierToGene (
    CSeqFeatData& sfdata,
    EQual qtype,
    const string& val
)

{
    CGene_ref& grp = sfdata.SetGene ();
    switch (qtype) {
        case eQual_gene:
            grp.SetLocus (val);
            return true;
        case eQual_allele:
            grp.SetAllele (val);
            return true;
        case eQual_gene_desc:
            grp.SetDesc (val);
            return true;
        case eQual_gene_syn:
            {
                CGene_ref::TSyn& syn = grp.SetSyn ();
                syn.push_back (val);
                return true;
            }
        case eQual_map:
            grp.SetMaploc (val);
            return true;
        case eQual_locus_tag:
            grp.SetLocus_tag (val);
            return true;
        case eQual_nomenclature:
            /* !!! need to implement !!! */
            return true;
        default:
            break;
    }
    return false;
}


bool CFeature_table_reader_imp::x_AddQualifierToCdregion (
    CRef<CSeq_feat> sfp,
    CSeqFeatData& sfdata,
    EQual qtype, const string& val,
    IErrorContainer *container, 
    int line, 
    const string &seq_id 
)

{
    CCdregion& crp = sfdata.SetCdregion ();
    switch (qtype) {
        case eQual_codon_start:
            {
                int frame = x_StringToLongNoThrow (val, container, seq_id, line, "CDS", "codon_start");
                switch (frame) {
                    case 0:
                        crp.SetFrame (CCdregion::eFrame_not_set);
                        break;
                    case 1:
                        crp.SetFrame (CCdregion::eFrame_one);
                        break;
                    case 2:
                        crp.SetFrame (CCdregion::eFrame_two);
                        break;
                    case 3:
                        crp.SetFrame (CCdregion::eFrame_three);
                        break;
                    default:
                        break;
                }
                return true;
            }
        case eQual_EC_number:
            {
                CProt_ref& prp = sfp->SetProtXref ();
                CProt_ref::TEc& ec = prp.SetEc ();
                ec.push_back (val);
                return true;
            }
        case eQual_function:
            {
                CProt_ref& prp = sfp->SetProtXref ();
                CProt_ref::TActivity& fun = prp.SetActivity ();
                fun.push_back (val);
                return true;
            }
        case eQual_product:
            {
                CProt_ref& prp = sfp->SetProtXref ();
                CProt_ref::TName& prod = prp.SetName ();
                prod.push_back (val);
                return true;
            }
        case eQual_prot_desc:
            {
                CProt_ref& prp = sfp->SetProtXref ();
                prp.SetDesc (val);
                return true;
            }
        case eQual_prot_note:
            return x_AddGBQualToFeature(sfp, "prot_note", val);
        case eQual_transl_except:
            // add as GBQual, let cleanup convert to code_break
            return x_AddGBQualToFeature(sfp, "transl_except", val);
        case eQual_translation:
            // we should accept, but ignore this qual on CDSs.
            // so, do nothing but return success
            return true;
        case eQual_transl_table:
            // set genetic code directly, or add qualifier and let cleanup convert?
            try {
                int num = NStr::StringToLong(val);
                CRef<CGenetic_code::C_E> code(new CGenetic_code::C_E());
                code->SetId(num);
                crp.SetCode().Set().push_back(code);
                return true;
            } catch( ... ) {
                return x_AddGBQualToFeature(sfp, "transl_table", val);
            }
            break;
            
        default:
            break;
    }
    return false;
}


bool CFeature_table_reader_imp::x_StringIsJustQuotes (
    const string& str
)

{
    ITERATE (string, it, str) {
      char ch = *it;
      if (ch > ' ' && ch != '"' && ch != '\'') return false;
    }

    return true;
}

// returns true if this is a feature line.
// If it's a feature line it also repairs it if necessary
static bool 
s_IsFeatureLineAndFix (
    CTempString& line)
{
    // this is a Feature line if the first 
    // non-space character is a '>'

    ITERATE(CTempString, str_iter, line) {
        const char ch = *str_iter;
        if( ! isspace(ch) ) {
            if(ch == '>') {
                if( str_iter != line.begin() ) {
                    line = NStr::TruncateSpaces(line, NStr::eTrunc_Begin);
                }
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

static bool
s_LineIndicatesOrder( const string & line )
{
    // basically, this is true if the line starts with "order" (whitespaces disregarded)

    const static char* kOrder = "ORDER";

    // find first non-whitespace character
    string::size_type pos = 0;
    for( ; pos < line.length() && isspace(line[pos]); ++pos) {
        // nothing to do here
    }

    // line is all whitespace
    if( pos >= line.length() ) {
        return false;
    }

    // check if starts with "order" after whitespace
    return ( 0 == NStr::CompareNocase( line, pos, strlen(kOrder), kOrder ) );
}

// Turns a "join" location into an "order" by putting nulls between it
// Returns an unset CRef if the loc doesn't need nulls (e.g. if it's just an interval)
static CRef<CSeq_loc>
s_LocationJoinToOrder( const CSeq_loc & loc )
{
    // create result we're returning
    CRef<CSeq_loc> result( new CSeq_loc );
    CSeq_loc_mix::Tdata & mix_pieces  = result->SetMix().Set();

    // keep this around for whenever we need a "null" piece
    CRef<CSeq_loc> loc_piece_null( new CSeq_loc );
    loc_piece_null->SetNull();

    // push pieces of source, with NULLs between
    CSeq_loc_CI loc_iter( loc );
    for( ; loc_iter; ++loc_iter ) {
        if( ! mix_pieces.empty() ) {
            mix_pieces.push_back( loc_piece_null );
        }
        CRef<CSeq_loc> new_piece( new CSeq_loc );
        new_piece->Assign( loc_iter.GetEmbeddingSeq_loc() );
        mix_pieces.push_back( new_piece );
    }

    // Only wrap in "mix" if there was more than one piece
    if( mix_pieces.size() > 1 ) {
        return result;
    } else {
        return CRef<CSeq_loc>();
    }
}

int CFeature_table_reader_imp::x_ParseTrnaString (
    const string& val
)

{
    string fst, scd;

    scd = val;
    if (NStr::StartsWith (val, "tRNA-")) {
        NStr::SplitInTwo (val, "-", fst, scd);
    }

    TTrnaMap::const_iterator t_iter = sm_TrnaKeys.find (scd.c_str ());
    if (t_iter != sm_TrnaKeys.end ()) {
        return t_iter->second;
    }

    return 0;
}

bool
CFeature_table_reader_imp::x_ParseTrnaExtString(
    CTrna_ext & ext_trna, const string & str, const CSeq_id *seq_id )
{
    if (NStr::IsBlank (str)) return false;

    if ( NStr::StartsWith(str, "(pos:") ) {
        // find position of closing paren
        string::size_type pos_end = x_MatchingParenPos( str, 0 );
        if (pos_end != string::npos) {
            string pos_str = str.substr (5, pos_end - 5);
            string::size_type aa_start = NStr::FindNoCase (pos_str, "aa:");
            if (aa_start != string::npos) {
                string abbrev = pos_str.substr (aa_start + 3);
                TTrnaMap::const_iterator t_iter = sm_TrnaKeys.find (abbrev.c_str ());
                if (t_iter == sm_TrnaKeys.end ()) {
                    // unable to parse
                    return false;
                }
                CRef<CTrna_ext::TAa> aa(new CTrna_ext::TAa);
                aa->SetNcbieaa (t_iter->second);
                ext_trna.SetAa(*aa);
                pos_str = pos_str.substr (0, aa_start);
                NStr::TruncateSpacesInPlace (pos_str);
                if (NStr::EndsWith (pos_str, ",")) {
                    pos_str = pos_str.substr (0, pos_str.length() - 1);
                }
            }
            CGetSeqLocFromStringHelper helper;
            CRef<CSeq_loc> anticodon = GetSeqLocFromString (pos_str, seq_id, & helper);
            if (anticodon == NULL) {
                ext_trna.ResetAa();
                return false;
            } else {
                switch( anticodon->GetStrand() ) {
                case eNa_strand_unknown:
                case eNa_strand_plus:
                case eNa_strand_minus:
                    ext_trna.SetAnticodon(*anticodon);
                    return true;
                default:
                    ext_trna.ResetAa();
                    return false;
                }
            }
        }
    }

    return false;
}


SIZE_TYPE CFeature_table_reader_imp::x_MatchingParenPos( 
    const string &str, SIZE_TYPE open_paren_pos )
{
    _ASSERT( str[open_paren_pos] == '(' );
    _ASSERT( open_paren_pos < str.length() );

    // nesting level. start at 1 since we know there's an open paren
    int level = 1;

    SIZE_TYPE pos = open_paren_pos + 1;
    for( ; pos < str.length(); ++pos ) {
        switch( str[pos] ) {
            case '(':
                // nesting deeper
                ++level;
                break;
            case ')':
                // closed a level of nesting
                --level;
                if( 0 == level ) {
                    // reached the top: we're closing the initial paren,
                    // so we return our position
                    return pos;
                }
                break;
            default:
                // ignore other characters.
                // maybe in the future we'll handle ignoring parens in quotes or
                // things like that.
                break;
        }
    }
    return NPOS;
}

long CFeature_table_reader_imp::x_StringToLongNoThrow (
    CTempString strToConvert,
    IErrorContainer *container, 
    const std::string& strSeqId,
    unsigned int uLine,
    CTempString strFeatureName,
    CTempString strQualifierName,
    ILineError::EProblem eProblem
)
{
    try {
        return NStr::StringToLong(strToConvert);
    } catch( ... ) {
        // See if we start with a number, but there's extra junk after it, try again
        if( ! strToConvert.empty() && isdigit(strToConvert[0]) ) {
            try {
                long result = NStr::StringToLong(strToConvert, NStr::fAllowTrailingSymbols);

                ILineError::EProblem problem = 
                    ILineError::eProblem_NumericQualifierValueHasExtraTrailingCharacters;
                if( eProblem != ILineError::eProblem_Unset ) {
                    problem = eProblem;
                }

                x_ProcessMsg( container, 
                    problem,
                    eDiag_Warning,
                    strSeqId, uLine, strFeatureName, strQualifierName, strToConvert );
                return result;
            } catch( ... ) { } // fall-thru to usual handling
        }

        ILineError::EProblem problem = 
            ILineError::eProblem_NumericQualifierValueIsNotANumber;
        if( eProblem != ILineError::eProblem_Unset ) {
            problem = eProblem;
        }

        x_ProcessMsg( container,
            problem,
            eDiag_Warning,
            strSeqId, uLine, strFeatureName, strQualifierName, strToConvert );
        // we have no idea, so just return zero
        return 0;
    }
}


bool CFeature_table_reader_imp::x_AddQualifierToRna (
    CSeqFeatData& sfdata,
    EQual qtype,
    const string& val,
    IErrorContainer *container, 
    int line_num, 
    const string &seq_id
)
{
    CRNA_ref& rrp = sfdata.SetRna ();
    CRNA_ref::EType rnatyp = rrp.GetType ();
    switch (rnatyp) {
        case CRNA_ref::eType_premsg:
        case CRNA_ref::eType_mRNA:
        case CRNA_ref::eType_rRNA:
            switch (qtype) {
                case eQual_product:
                    {
                        CRNA_ref::TExt& tex = rrp.SetExt ();
                        CRNA_ref::C_Ext::E_Choice exttype = tex.Which ();
                        if (exttype == CRNA_ref::C_Ext::e_TRNA) return false;
                        tex.SetName (val);
                        return true;
                    }
                default:
                    break;
            }
            break;
        case CRNA_ref::eType_ncRNA:
            switch (qtype) {
                case eQual_product:
                    rrp.SetExt().SetGen().SetProduct(val);
                    return true;
                    break;
                case eQual_ncRNA_class:
                    rrp.SetExt().SetGen().SetClass(val);
                    return true;
                    break;
                default:
                    break;
            }
            break;
        case CRNA_ref::eType_tmRNA:
            switch (qtype) {
                case eQual_product:
                    rrp.SetExt().SetGen().SetProduct(val);
                    return true;
                case eQual_tag_peptide:
                  {
                    CRef<CRNA_qual> q(new CRNA_qual());
                    q->SetQual("tag_peptide");
                    q->SetVal(val);
                    rrp.SetExt().SetGen().SetQuals().Set().push_back(q);
                    return true;
                  }
                  break;
                default:
                    break;
            }
            break;
        case CRNA_ref::eType_snRNA:
        case CRNA_ref::eType_scRNA:
        case CRNA_ref::eType_snoRNA:
        case CRNA_ref::eType_other:
            return false;
        case CRNA_ref::eType_tRNA:
            switch (qtype) {
                case eQual_product: {
                        CRNA_ref::TExt& tex = rrp.SetExt ();
                        CRNA_ref::C_Ext::E_Choice exttype = tex.Which ();
                        if (exttype == CRNA_ref::C_Ext::e_Name) return false;
                        CTrna_ext& trx = tex.SetTRNA ();
                        int aaval = x_ParseTrnaString (val);
                        if (aaval > 0) {
                            CTrna_ext::TAa& taa = trx.SetAa ();
                            taa.SetNcbieaa (aaval);
                            trx.SetAa (taa);
                            tex.SetTRNA (trx);
                        } else {
                            x_ProcessMsg( container, 
                                ILineError::eProblem_QualifierBadValue, eDiag_Error,
                                seq_id, line_num,
                                "tRNA", "product", val );
                        }
                        return true;
                    }
                    break;
                case eQual_anticodon:
                    {
                        CRNA_ref::TExt& tex = rrp.SetExt ();
                        CRNA_ref::C_Ext::TTRNA & ext_trna = tex.SetTRNA();
                        CRef<CSeq_id> seq_id_obj( new CSeq_id(seq_id) );
                        if( ! x_ParseTrnaExtString(ext_trna, val, &*seq_id_obj) ) {
                            x_ProcessMsg( container, 
                                ILineError::eProblem_QualifierBadValue, eDiag_Error,
                                seq_id, line_num,
                                "tRNA", "anticodon", val );
                        }
                        return true;
                    }
                    break;
                case eQual_codon_recognized: 
                    {
                        CRNA_ref::TExt& tex = rrp.SetExt ();
                        CRNA_ref::C_Ext::TTRNA & ext_trna = tex.SetTRNA();
                        ext_trna.SetAa().SetNcbieaa();
                        ext_trna.SetCodon().push_back( CGen_code_table::CodonToIndex(val) );
                        return true;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return false;
}


bool CFeature_table_reader_imp::x_AddQualifierToImp (
    CRef<CSeq_feat> sfp,
    CSeqFeatData& sfdata,
    EQual qtype,
    const string& qual,
    const string& val
)

{
    const char *str = NULL;

    CSeqFeatData::ESubtype subtype = sfdata.GetSubtype ();
    switch (subtype) {
        case CSeqFeatData::eSubtype_variation:
            {
                switch (qtype) {
                    case eQual_chrcnt:
                    case eQual_ctgcnt:
                    case eQual_loccnt:
                    case eQual_snp_class:
                    case eQual_snp_gtype:
                    case eQual_snp_het:
                    case eQual_snp_het_se:
                    case eQual_snp_linkout:
                    case eQual_snp_maxrate:
                    case eQual_snp_valid:
                    case eQual_weight:
                        str = "dbSnpSynonymyData";
                        break;
                    default:
                        break;
                }
            }
            break;
        case CSeqFeatData::eSubtype_STS:
            {
                switch (qtype) {
                    case eQual_sts_aliases:
                    case eQual_sts_dsegs:
                    case eQual_weight:
                        str = "stsUserObject";
                        break;
                    default:
                        break;
                }
            }
            break;
        case CSeqFeatData::eSubtype_misc_feature:
            {
                switch (qtype) {
                    case eQual_bac_ends:
                    case eQual_clone_id:
                    case eQual_method:
                    case eQual_sequence:
                    case eQual_STS:
                    case eQual_weight:
                        str = "cloneUserObject";
                        break;
                    default:
                        break;
                }
            }
            break;
        default:
            break;
    }

    if( NULL != str ) {
        CSeq_feat::TExt& ext = sfp->SetExt ();
        CObject_id& obj = ext.SetType ();
        if ((! obj.IsStr ()) || obj.GetStr ().empty ()) {
            obj.SetStr ();
        }
        ext.AddField (qual, val, CUser_object::eParse_Number);
        return true;
    }

    return false;
}


bool CFeature_table_reader_imp::x_AddQualifierToBioSrc (
    CSeqFeatData& sfdata,
    const string &feat_name,
    EOrgRef rtype,
    const string& val,
    IErrorContainer *container, 
    int line, 	
    const string &seq_id 
)

{
    CBioSource& bsp = sfdata.SetBiosrc ();

    switch (rtype) {
        case eOrgRef_organism:
            {
                CBioSource::TOrg& orp = bsp.SetOrg ();
                orp.SetTaxname (val);
                return true;
            }
        case eOrgRef_organelle:
            {
                TGenomeMap::const_iterator g_iter = sm_GenomeKeys.find (val.c_str ());
                if (g_iter != sm_GenomeKeys.end ()) {
                    CBioSource::EGenome gtype = g_iter->second;
                    bsp.SetGenome (gtype);
                } else {
                    x_ProcessMsg( container, 
                        ILineError::eProblem_QualifierBadValue, eDiag_Error,
                        seq_id, line,
                        feat_name, "organelle", val );
                }
                return true;
            }
        case eOrgRef_div:
            {
                CBioSource::TOrg& orp = bsp.SetOrg ();
                COrg_ref::TOrgname& onp = orp.SetOrgname ();
                onp.SetDiv (val);
                return true;
            }
        case eOrgRef_lineage:
            {
                CBioSource::TOrg& orp = bsp.SetOrg ();
                COrg_ref::TOrgname& onp = orp.SetOrgname ();
                onp.SetLineage (val);
                return true;
            }
        case eOrgRef_gcode:
            {
                CBioSource::TOrg& orp = bsp.SetOrg ();
                COrg_ref::TOrgname& onp = orp.SetOrgname ();
                int code = x_StringToLongNoThrow (val, container, seq_id, line, feat_name, "gcode");
                onp.SetGcode (code);
                return true;
            }
        case eOrgRef_mgcode:
            {
                CBioSource::TOrg& orp = bsp.SetOrg ();
                COrg_ref::TOrgname& onp = orp.SetOrgname ();
                int code = x_StringToLongNoThrow (val, container, seq_id, line, feat_name, "mgcode");
                onp.SetMgcode (code);
                return true;
            }
        default:
            break;
    }
    return false;
}


bool CFeature_table_reader_imp::x_AddQualifierToBioSrc (
    CSeqFeatData& sfdata,
    CSubSource::ESubtype stype,
    const string& val
)

{
    CBioSource& bsp = sfdata.SetBiosrc ();
    CBioSource::TSubtype& slist = bsp.SetSubtype ();
    CRef<CSubSource> ssp (new CSubSource);
    ssp->SetSubtype (stype);
    ssp->SetName (val);
    slist.push_back (ssp);
    return true;
}


bool CFeature_table_reader_imp::x_AddQualifierToBioSrc (
    CSeqFeatData& sfdata,
    COrgMod::ESubtype mtype,
    const string& val
)

{
    CBioSource& bsp = sfdata.SetBiosrc ();
    CBioSource::TOrg& orp = bsp.SetOrg ();
    COrg_ref::TOrgname& onp = orp.SetOrgname ();
    COrgName::TMod& mlist = onp.SetMod ();
    CRef<COrgMod> omp (new COrgMod);
    omp->SetSubtype (mtype);
    omp->SetSubname (val);
    mlist.push_back (omp);
    return true;
}


bool CFeature_table_reader_imp::x_AddGBQualToFeature (
    CRef<CSeq_feat> sfp,
    const string& qual,
    const string& val
)

{
    if (qual.empty ()) return false;

    CSeq_feat::TQual& qlist = sfp->SetQual ();
    CRef<CGb_qual> gbq (new CGb_qual);
    gbq->SetQual (qual);
    if (x_StringIsJustQuotes (val)) {
        gbq->SetVal (kEmptyStr);
    } else {
        gbq->SetVal (val);
    }
    qlist.push_back (gbq);

    return true;
}


static const string k_GoQuals[] = { "go_process", "go_component", "go_function" };
static const int k_NumGoQuals = sizeof (k_GoQuals) / sizeof (string);

bool CFeature_table_reader_imp::x_AddGeneOntologyToFeature (
    CRef<CSeq_feat> sfp,
    const string& qual,
    const string& val
)

{
    if (qual.empty ()) return false;

    int j = 0;
    while (j < k_NumGoQuals && !NStr::EqualNocase(k_GoQuals[j], qual)) {
        j++;
    }
    if (j == k_NumGoQuals) {
        return false;
    }

    vector<string> fields;
    NStr::Tokenize(val, "|", fields);
    while (fields.size() < 4) {
        fields.push_back("");
    }
    if (NStr::StartsWith(fields[1], "GO:")) {
        fields[1] = fields[1].substr(3);
    }
    if (NStr::StartsWith(fields[2], "GO_REF:")) {
        fields[2] = fields[2].substr(7);
    }

    int pmid = 0;
    
    if (!NStr::IsBlank(fields[2])) {
        if (!NStr::StartsWith(fields[2], "0")) {
            try {
                pmid = NStr::StringToLong(fields[2]);
                fields[2] = "";
            } catch( ... ) {
                pmid = 0;
            }
        }
    }
    string label (1, toupper((unsigned char) qual[0]));
    label += qual.substr(1);

    sfp->SetExt().SetType().SetStr("GeneOntology");
    CUser_field& field = sfp->SetExt().SetField(label);
    CRef<CUser_field> text_field (new CUser_field());
    text_field->SetLabel().SetStr("text_string");
    text_field->SetData().SetStr( CUtf8::AsUTF8(fields[0], eEncoding_Ascii));
    field.SetData().SetFields().push_back(text_field);
  
    if (!NStr::IsBlank(fields[1])) {
        CRef<CUser_field> goid (new CUser_field());
        goid->SetLabel().SetStr("go id");
        goid->SetData().SetStr( CUtf8::AsUTF8(fields[1], eEncoding_Ascii) );
        field.SetData().SetFields().push_back(goid);
    }

    if (pmid > 0) {
        CRef<CUser_field> pubmed_id (new CUser_field());
        pubmed_id->SetLabel().SetStr("pubmed id");
        pubmed_id->SetData().SetInt(pmid);
        field.SetData().SetFields().push_back(pubmed_id);
    }

    if (!NStr::IsBlank(fields[2])) {
        CRef<CUser_field> goref (new CUser_field());
        goref->SetLabel().SetStr("go ref");
        goref->SetData().SetStr( CUtf8::AsUTF8(fields[2], eEncoding_Ascii) );
        field.SetData().SetFields().push_back(goref);
    }

    if (!NStr::IsBlank(fields[3])) {
        CRef<CUser_field> evidence (new CUser_field());
        evidence->SetLabel().SetStr("evidence");
        evidence->SetData().SetStr( CUtf8::AsUTF8(fields[3], eEncoding_Ascii) );
        field.SetData().SetFields().push_back(evidence);
    }

    return true;
}


static const string s_QualsWithCaps[] = {
  "EC_number",
  "PCR_conditions",
  "PubMed",
  "STS",
  "ncRNA_class"
};

static const int s_NumQualsWithCaps = sizeof (s_QualsWithCaps) / sizeof (string);

static string s_FixQualCapitalization (const string& qual)
{
    string lqual = qual;
    lqual = NStr::ToLower(lqual);
    for (int j = 0; j < s_NumQualsWithCaps; j++) {    
        if (NStr::EqualNocase(lqual, s_QualsWithCaps[j])) {
            lqual = s_QualsWithCaps[j];
            break;
        }
    }
    return lqual;
}


bool CFeature_table_reader_imp::x_AddQualifierToFeature (
    CRef<CSeq_feat> sfp,
    const string &feat_name,
    const string& qual,
    const string& val,
    IErrorContainer *container, 
    int line, 	
    const string &seq_id 
)

{
    CSeqFeatData&          sfdata = sfp->SetData ();
    CSeqFeatData::E_Choice typ = sfdata.Which ();

    if (typ == CSeqFeatData::e_Biosrc) {

        TOrgRefMap::const_iterator o_iter = sm_OrgRefKeys.find (qual.c_str ());
        if (o_iter != sm_OrgRefKeys.end ()) {
            EOrgRef rtype = o_iter->second;
            if (x_AddQualifierToBioSrc (sfdata, feat_name, rtype, val, container, line, seq_id)) return true;

        } else {

            TSubSrcMap::const_iterator s_iter = sm_SubSrcKeys.find (qual.c_str ());
            if (s_iter != sm_SubSrcKeys.end ()) {

                CSubSource::ESubtype stype = s_iter->second;
                if (x_AddQualifierToBioSrc (sfdata, stype, val)) return true;

            } else {

                TOrgModMap::const_iterator m_iter = sm_OrgModKeys.find (qual.c_str ());
                if (m_iter != sm_OrgModKeys.end ()) {

                    COrgMod::ESubtype  mtype = m_iter->second;
                    if (x_AddQualifierToBioSrc (sfdata, mtype, val)) return true;

                }
            }
        }

    } else {
        string lqual = s_FixQualCapitalization(qual);
        TQualMap::const_iterator q_iter = sm_QualKeys.find (lqual.c_str ());
        if (q_iter != sm_QualKeys.end ()) {
            EQual qtype = q_iter->second;
            switch (typ) {
                case CSeqFeatData::e_Gene:
                    if (x_AddQualifierToGene (sfdata, qtype, val)) return true;
                    break;
                case CSeqFeatData::e_Cdregion:
                    if (x_AddQualifierToCdregion (sfp, sfdata, qtype, val, container, line, seq_id)) return true;
                    break;
                case CSeqFeatData::e_Rna:
                    if (x_AddQualifierToRna (sfdata, qtype, val, container, line, seq_id)) return true;
                    break;
                case CSeqFeatData::e_Imp:
                    if (x_AddQualifierToImp (sfp, sfdata, qtype, qual, val)) return true;
                    break;
                case CSeqFeatData::e_Region:
                    if (qtype == eQual_region_name) {
                        sfdata.SetRegion (val);
                        return true;
                    }
                    break;
                case CSeqFeatData::e_Bond:
                    if (qtype == eQual_bond_type) {
                        CSeqFeatData::EBond btyp = CSeqFeatData::eBond_other;
                        if (CSeqFeatData::GetBondList()->IsBondName(val.c_str(), btyp)) {
                            sfdata.SetBond (btyp);
                            return true;
                        }
                    }
                    break;
                case CSeqFeatData::e_Site:
                    if (qtype == eQual_site_type) {
                        CSeqFeatData::ESite styp = CSeqFeatData::eSite_other;
                        if (CSeqFeatData::GetSiteList()->IsSiteName( val.c_str(), styp)) {
                            sfdata.SetSite (styp);
                            return true;
                        }
                    }
                    break;
                case CSeqFeatData::e_Pub:
                    if( qtype == eQual_PubMed ) {
                        CRef<CPub> new_pub( new CPub );
                        new_pub->SetPmid( CPubMedId( x_StringToLongNoThrow(val, container, seq_id, line, feat_name, qual) ) );
                        sfdata.SetPub().SetPub().Set().push_back( new_pub );
                        return true;
                    }
                    break;
                case CSeqFeatData::e_Prot:
                    switch( qtype ) {
                    case eQual_product:
                        sfdata.SetProt().SetName().push_back( val );
                        return true;
                    case eQual_function:
                        sfdata.SetProt().SetActivity().push_back( val );
                        return true;
                    case eQual_EC_number:
                        sfdata.SetProt().SetEc().push_back( val );
                        return true;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
            }
            switch (qtype) {
                case eQual_pseudo:
                    sfp->SetPseudo (true);
                    return true;
                case eQual_partial:
                    sfp->SetPartial (true);
                    return true;
                case eQual_exception:
                    sfp->SetExcept (true);
                    sfp->SetExcept_text (val);
                    return true;
                case eQual_ribosomal_slippage:
                    sfp->SetExcept (true);
                    sfp->SetExcept_text (qual);
                    return true;
                case eQual_trans_splicing:
                    sfp->SetExcept (true);
                    sfp->SetExcept_text (qual);
                    return true;
                case eQual_evidence:
                    if (val == "experimental") {
                        sfp->SetExp_ev (CSeq_feat::eExp_ev_experimental);
                    } else if (val == "not_experimental" || val == "non_experimental" ||
                               val == "not-experimental" || val == "non-experimental") {
                        sfp->SetExp_ev (CSeq_feat::eExp_ev_not_experimental);
                    }
                    return true;
                case eQual_note:
                    {
                        if (sfp->CanGetComment ()) {
                            const CSeq_feat::TComment& comment = sfp->GetComment ();
                            CSeq_feat::TComment revised = comment + "; " + val;
                            sfp->SetComment (revised);
                        } else {
                            sfp->SetComment (val);
                        }
                        return true;
                    }
                case eQual_inference:
                    {
                        string prefix = "", remainder = "";
                        CInferencePrefixList::GetPrefixAndRemainder (val, prefix, remainder);
                        if (!NStr::IsBlank(prefix) && NStr::StartsWith (val, prefix)) {
                            x_AddGBQualToFeature (sfp, qual, val);
                        } else {
                            x_ProcessMsg( container, 
                                ILineError::eProblem_QualifierBadValue, eDiag_Error,
                                seq_id, line,
                                feat_name, qual, val );
                        }
                        return true;
                    }
                case eQual_replace:
                    {
                        string val_copy = val;
                        NStr::ToLower( val_copy );
                        x_AddGBQualToFeature (sfp, qual, val_copy );
                        return true;
                    }
                case eQual_allele:
                case eQual_bound_moiety:
                case eQual_clone:
                case eQual_compare:
                case eQual_cons_splice:
                case eQual_direction:
                case eQual_EC_number:
                case eQual_estimated_length:
                case eQual_experiment:
                case eQual_frequency:
                case eQual_function:
                case eQual_gap_type:
                case eQual_insertion_seq:
                case eQual_label:
                case eQual_linkage_evidence:
                case eQual_map:
                case eQual_ncRNA_class:
                case eQual_number:
                case eQual_old_locus_tag:
                case eQual_operon:
                case eQual_organism:
                case eQual_PCR_conditions:
                case eQual_phenotype:
                case eQual_product:
                case eQual_protein_id:
                case eQual_pseudogene:
                case eQual_satellite:
                case eQual_rpt_family:
                case eQual_rpt_type:
                case eQual_rpt_unit:
                case eQual_rpt_unit_range:
                case eQual_rpt_unit_seq:
                case eQual_standard_name:
                case eQual_tag_peptide:
                case eQual_transcript_id:
                case eQual_transposon:
                case eQual_usedin:
                case eQual_cyt_map:
                case eQual_gen_map:
                case eQual_rad_map:
                case eQual_mobile_element_type:

                    {
                        x_AddGBQualToFeature (sfp, qual, val);
                        return true;
                    }
                case eQual_gene:
                    {
                        CGene_ref& grp = sfp->SetGeneXref ();
                        if (val == "-") {
                            grp.SetLocus ("");
                        } else {
                            grp.SetLocus (val);
                        }
                        return true;
                    }
                case eQual_gene_desc:
                    {
                        CGene_ref& grp = sfp->SetGeneXref ();
                        grp.SetDesc (val);
                        return true;
                    }
                case eQual_gene_syn:
                    {
                        CGene_ref& grp = sfp->SetGeneXref ();
                        CGene_ref::TSyn& syn = grp.SetSyn ();
                        syn.push_back (val);
                        return true;
                    }
                case eQual_locus_tag:
                    {
                        CGene_ref& grp = sfp->SetGeneXref ();
                        grp.SetLocus_tag (val);
                        return true;
                    }
                case eQual_db_xref:
                    {
                        string db, tag;
                        int num;
                        if (NStr::SplitInTwo (val, ":", db, tag)) {
                            CSeq_feat::TDbxref& dblist = sfp->SetDbxref ();
                            CRef<CDbtag> dbt (new CDbtag);
                            dbt->SetDb (db);
                            CRef<CObject_id> oid (new CObject_id);
                            try {
                                num = NStr::StringToLong(tag);
                                oid->SetId(num);
                            } catch( ... ) {
                                oid->SetStr(tag);
                            }
                            dbt->SetTag (*oid);
                            dblist.push_back (dbt);
                            return true;
                        }
                        return true;
                    }
                case eQual_nomenclature:
                    {
                        /* !!! need to implement !!! */
                        return true;
                    }
                case eQual_go_component:
                case eQual_go_function:
                case eQual_go_process:
                    if (typ == CSeqFeatData::e_Gene || typ == CSeqFeatData::e_Cdregion || typ == CSeqFeatData::e_Rna) {
                        return x_AddGeneOntologyToFeature(sfp, qual, val);
                    }
                    return false;
                default:
                    break;
            }
        }
    }
    return false;
}

bool CFeature_table_reader_imp::x_IsWebComment(CTempString line)
{
    // This function is testing for a match against the following regular
    // expression, but we avoid actual regexps for max speed:
    // "^(===================================================================| INFO:| WARNING:| ERROR:).*"

    // (that magic number is the size of the smallest possible match)
    if( line.length() < 6 ) { 
        return false;
    }

    if( line[0] == '=' ) {
        static const CTempString kAllEqualsMatch =
            "===================================================================";
        if( NStr::StartsWith(line, kAllEqualsMatch) ) {
            return true;
        }
    } else if( line[0] == ' ') {
        switch(line[1]) {
        case 'I':
            {
                static const CTempString kInfo = " INFO:";
                if( NStr::StartsWith(line, kInfo) ) {
                    return true;
                }
            }
            break;
        case 'W':
            {
                static const CTempString kWarning = " WARNING:";
                if( NStr::StartsWith(line, kWarning) ) {
                    return true;
                }
            }
            break;
        case 'E':
            {
                static const CTempString kError = " ERROR:";
                if( NStr::StartsWith(line, kError) ) {
                    return true;
                }
            }
            break;
        default:
            // no match
            break;
        }
    }

    // no match
    return false;
}

bool CFeature_table_reader_imp::x_AddIntervalToFeature(
    CTempString strFeatureName,
    CRef<CSeq_feat> sfp,
    CSeq_loc_mix& mix,
    Int4 start,
    Int4 stop,
    bool partial5,
    bool partial3,
    bool ispoint,
    bool isminus,
    IErrorContainer *container, 
    int line_num, 
    const string& seqid
)

{
    CSeq_interval::TStrand strand = eNa_strand_plus;

    if (start > stop) {
        swap(start, stop);
        strand = eNa_strand_minus;
    }
    if (isminus) {
        strand = eNa_strand_minus;
    }

    // construct loc, which will be added to the mix
    CRef<CSeq_id> seq_id ( new CSeq_id(seqid) );
    CRef<CSeq_loc> loc(new CSeq_loc);
    if (ispoint || start == stop ) {
        // a point of some kind
        CRef<CSeq_point> pPoint( new CSeq_point(*seq_id, start, strand) );
        if( ispoint ) {
            // between two bases
            pPoint->SetRightOf (true);
        } else {
            // just a point. do nothing
        }
        loc->SetPnt( *pPoint );
    } else {
        // interval
        CRef<CSeq_interval> pIval( new CSeq_interval(*seq_id, start, stop, strand) );
        if (partial5) {
            pIval->SetPartialStart (true, eExtreme_Biological);
        }
        if (partial3) {
            pIval->SetPartialStop (true, eExtreme_Biological);
        }
        loc->SetInt(*pIval);
    }

    // check for internal partials
    CSeq_loc_mix::Tdata & mix_set = mix.Set();
    if( ! mix_set.empty() ) {
        const CSeq_loc & last_loc = *mix_set.back();
        if( last_loc.IsPartialStop(eExtreme_Biological) ||
            loc->IsPartialStart(eExtreme_Biological) ) 
        {
            // internal partials
            x_ProcessMsg(container, ILineError::eProblem_InternalPartialsInFeatLocation,
                eDiag_Warning, seqid, line_num, strFeatureName );
        }
    }

    mix_set.push_back(loc);


    if (partial5 || partial3) {
        sfp->SetPartial (true);
    }

    return true;
}


bool CFeature_table_reader_imp::x_SetupSeqFeat (
    CRef<CSeq_feat> sfp,
    const string& feat,
    const CFeature_table_reader::TFlags flags,
    unsigned int line,
    const std::string &seq_id,
    IErrorContainer* container,
    ITableFilter *filter
)

{
    if (feat.empty ()) return false;

    // check filter, if any
    if( NULL != filter ) {
        ITableFilter::EResult result = filter->IsFeatureNameOkay(feat);
        if( result != ITableFilter::eResult_Okay ) {
            x_ProcessMsg( container, 
                ILineError::eProblem_FeatureNameNotAllowed,
                eDiag_Warning, seq_id, line, feat );
            if( result == ITableFilter::eResult_Disallowed ) {
                return false;
            }
        }
    }

    TFeatMap::const_iterator f_iter = sm_FeatKeys.find (feat.c_str ());
    if (f_iter != sm_FeatKeys.end ()) {

        CSeqFeatData::ESubtype sbtyp = f_iter->second;
        if (sbtyp != CSeqFeatData::eSubtype_bad) {

            // populate *sfp here...

            CSeqFeatData::E_Choice typ = CSeqFeatData::GetTypeFromSubtype (sbtyp);
            sfp->SetData ().Select (typ);
            CSeqFeatData& sfdata = sfp->SetData ();
    
            if (typ == CSeqFeatData::e_Rna) {
                CRNA_ref& rrp = sfdata.SetRna ();
                CRNA_ref::EType rnatyp = CRNA_ref::eType_unknown;
                switch (sbtyp) {
                    case CSeqFeatData::eSubtype_preRNA :
                        rnatyp = CRNA_ref::eType_premsg;
                        break;
                    case CSeqFeatData::eSubtype_mRNA :
                        rnatyp = CRNA_ref::eType_mRNA;
                        break;
                    case CSeqFeatData::eSubtype_tRNA :
                        rnatyp = CRNA_ref::eType_tRNA;
                        break;
                    case CSeqFeatData::eSubtype_rRNA :
                        rnatyp = CRNA_ref::eType_rRNA;
                        break;
                    case CSeqFeatData::eSubtype_snRNA :
                        rnatyp = CRNA_ref::eType_ncRNA;
                        rrp.SetExt().SetGen().SetClass("snRNA");
                        break;
                    case CSeqFeatData::eSubtype_scRNA :
                        rnatyp = CRNA_ref::eType_ncRNA;
                        rrp.SetExt().SetGen().SetClass("scRNA");
                        break;
                    case CSeqFeatData::eSubtype_snoRNA :
                        rnatyp = CRNA_ref::eType_ncRNA;
                        rrp.SetExt().SetGen().SetClass("snoRNA");
                        break;
                    case CSeqFeatData::eSubtype_ncRNA :
                        rnatyp = CRNA_ref::eType_ncRNA;
                        rrp.SetExt().SetGen();
                        break;
                    case CSeqFeatData::eSubtype_tmRNA :
                        rnatyp = CRNA_ref::eType_tmRNA;
                        rrp.SetExt().SetGen();
                        break;
                    case CSeqFeatData::eSubtype_otherRNA :
                        rrp.SetExt().SetName("misc_RNA");
                        rnatyp = CRNA_ref::eType_other;
                        break;
                    default :
                        break;
                }
                rrp.SetType (rnatyp);
   
            } else if (typ == CSeqFeatData::e_Imp) {
                CImp_feat_Base& imp = sfdata.SetImp ();
                imp.SetKey (feat);
    
            } else if (typ == CSeqFeatData::e_Bond) {
                sfdata.SetBond (CSeqFeatData::eBond_other);
                
            } else if (typ == CSeqFeatData::e_Site) {
                sfdata.SetSite (CSeqFeatData::eSite_other);
            } else if (typ == CSeqFeatData::e_Prot ) {
                CProt_ref &prot_ref = sfdata.SetProt();
                if( sbtyp == CSeqFeatData::eSubtype_mat_peptide_aa ) {
                    prot_ref.SetProcessed( CProt_ref::eProcessed_mature );
                }
            }

            return true;
        }
    }

    // unrecognized feature key

    if ((flags & CFeature_table_reader::fReportBadKey) != 0) {
        x_ProcessMsg(container, ILineError::eProblem_UnrecognizedFeatureName, eDiag_Warning, seq_id, line, feat );
    }

    if ((flags & CFeature_table_reader::fTranslateBadKey) != 0) {

        sfp->SetData ().Select (CSeqFeatData::e_Imp);
        CSeqFeatData& sfdata = sfp->SetData ();
        CImp_feat_Base& imp = sfdata.SetImp ();
        imp.SetKey ("misc_feature");
        x_AddQualifierToFeature (sfp, kEmptyStr, "standard_name", feat, container, line, seq_id);

        return true;

    } else if ((flags & CFeature_table_reader::fKeepBadKey) != 0) {

        sfp->SetData ().Select (CSeqFeatData::e_Imp);
        CSeqFeatData& sfdata = sfp->SetData ();
        CImp_feat_Base& imp = sfdata.SetImp ();
        imp.SetKey (feat);

        return true;
    }

    return false;
}


void CFeature_table_reader_imp::x_ProcessMsg(
    IErrorContainer* container,
    ILineError::EProblem eProblem,
    EDiagSev eSeverity,
    const std::string& strSeqId,
    unsigned int uLine,
    const std::string & strFeatureName,
    const std::string & strQualifierName,
    const std::string & strQualifierValue )
{
    CObjReaderLineException err (eSeverity, uLine, "", eProblem, strSeqId, strFeatureName, strQualifierName, strQualifierValue);
    if (container == 0) {
        throw (err);
    }

    if ( !container->PutError(err) ) {
        throw(err);
    }
}
                                             

CRef<CSeq_annot> CFeature_table_reader_imp::ReadSequinFeatureTable (
    ILineReader& reader,
    const string& seqid,
    const string& annotname,
    const CFeature_table_reader::TFlags flags,
    IErrorContainer* container,
    ITableFilter *filter
)
{
    string feat, qual, val;
    string curr_feat_name;
    Int4 start, stop;
    bool partial5, partial3, ispoint, isminus, ignore_until_next_feature_key = false;
    Int4 offset = 0;
    CRef<CSeq_annot> sap(new CSeq_annot);
    CSeq_annot::C_Data::TFtable& ftable = sap->SetData().SetFtable();
    const bool bIgnoreWebComments = 
        ( (flags & CFeature_table_reader::fIgnoreWebComments) != 0 );

    // if sequence ID is a list, use just one sequence ID string    
    string real_seqid = seqid;
    if (!NStr::IsBlank(real_seqid)) {
        try {
            CSeq_id seq_id (seqid);
        } catch (...) {
            CBioseq::TId ids;
            CSeq_id::ParseIDs(ids, seqid);
            real_seqid.clear();
            ids.front()->GetLabel(&real_seqid, CSeq_id::eFasta);
        }
    }

    // Use this to efficiently find the best CDS for a prot feature
    // (only add CDS's for it to work right)
    CBestFeatFinder best_CDS_finder;

    CRef<CSeq_feat> sfp;

    if (! annotname.empty ()) {
      CAnnot_descr& descr = sap->SetDesc ();
      CRef<CAnnotdesc> annot(new CAnnotdesc);
      annot->SetName (annotname);
      descr.Set().push_back (annot);
    }

    while ( !reader.AtEOF() ) {

        CTempString line = *++reader;

        if (! line.empty () && (!bIgnoreWebComments || ! x_IsWebComment(line) ) ) {
            if( s_IsFeatureLineAndFix(line) ) {
                // if next feature table, return current sap
                reader.UngetLine(); // we'll get this feature line the next time around
                return sap;
            } if (line [0] == '[') {

                // set offset !!!!!!!!

            } else if ( s_LineIndicatesOrder(line) ) {

                // put nulls between feature intervals
                CRef<CSeq_loc> loc_with_nulls = s_LocationJoinToOrder( sfp->GetLocation() );
                // loc_with_nulls is unset if no change was needed
                if( loc_with_nulls ) {
                    sfp->SetLocation( *loc_with_nulls );
                }

            } else if (x_ParseFeatureTableLine (line, &start, &stop, &partial5, &partial3,
                                                &ispoint, &isminus, feat, qual, val, offset,
                                                container, reader.GetLineNumber(), real_seqid)) {

                // process line in feature table

                replace( val.begin(), val.end(), '\"', '\'' );

                if ((! feat.empty ()) && start >= 0 && stop >= 0) {

                    // process start - stop - feature line

                    sfp.Reset (new CSeq_feat);
                    sfp->ResetLocation ();

                    if (x_SetupSeqFeat (sfp, feat, flags, reader.GetLineNumber(), real_seqid, container, filter)) {

                        ftable.push_back (sfp);

                        // now create location

                        CRef<CSeq_loc> location (new CSeq_loc);
                        sfp->SetLocation (*location);

                        // if new feature is a CDS, remember it for later lookups
                        if( sfp->CanGetData() && sfp->GetData().IsCdregion() ) {
                            best_CDS_finder.AddFeat( *sfp );
                        }

                        // and add first interval
                        x_AddIntervalToFeature (curr_feat_name, sfp, location->SetMix(), 
                            start, stop, partial5, partial3, ispoint, isminus, 
                            container, reader.GetLineNumber(), real_seqid );

                        ignore_until_next_feature_key = false;

                        curr_feat_name = feat;

                    } else {

                        // bad feature, set ignore flag

                        ignore_until_next_feature_key = true;
                    }

                } else if (ignore_until_next_feature_key) {

                    // bad feature, ignore qualifiers until next feature key

                } else if (start >= 0 && stop >= 0 && feat.empty () && qual.empty () && val.empty ()) {

                    // process start - stop multiple interval line

                    if (sfp  &&  sfp->IsSetLocation()  &&  sfp->GetLocation().IsMix()) {
                        x_AddIntervalToFeature (curr_feat_name, sfp, sfp->SetLocation().SetMix(), 
                                                start, stop, partial5, partial3, ispoint, isminus, 
                                                container, reader.GetLineNumber(), real_seqid);
                    } else {
                        if ((flags & CFeature_table_reader::fReportBadKey) != 0) {
                            x_ProcessMsg(container, ILineError::eProblem_NoFeatureProvidedOnIntervals,
                                eDiag_Warning,
                                real_seqid,
                                reader.GetLineNumber() );
                        }
                    }

                } else if ((! qual.empty ()) && (! val.empty ())) {

                    // process qual - val qualifier line

                    if ( !sfp ) {
                        if ((flags & CFeature_table_reader::fReportBadKey) != 0) {
                            x_ProcessMsg(container, 
                                ILineError::eProblem_QualifierWithoutFeature, 
                                eDiag_Warning,
                                real_seqid,
                                reader.GetLineNumber(), kEmptyStr, qual, val );
                        }
                    } else if ( !x_AddQualifierToFeature (sfp, curr_feat_name, qual, val, container, reader.GetLineNumber(), real_seqid) ) {

                        // unrecognized qualifier key

                        if ((flags & CFeature_table_reader::fReportBadKey) != 0) {
                            x_ProcessMsg(container,
                                ILineError::eProblem_UnrecognizedQualifierName, 
                                eDiag_Warning, real_seqid, reader.GetLineNumber(), curr_feat_name, qual, val );
                        }

                        if ((flags & CFeature_table_reader::fKeepBadKey) != 0) {
                            x_AddGBQualToFeature (sfp, qual, val);
                        }
                    }

                } else if ((! qual.empty ()) && (val.empty ())) {

                    // check for the few qualifiers that do not need a value
                    if ( !sfp ) {
                        if ((flags & CFeature_table_reader::fReportBadKey) != 0) {
                            x_ProcessMsg(container, 
                                ILineError::eProblem_QualifierWithoutFeature, eDiag_Warning,
                                real_seqid, reader.GetLineNumber(),
                                kEmptyStr, qual );
                        }
                    } else {
                        TSingleSet::const_iterator s_iter = sc_SingleKeys.find (qual.c_str ());
                        if (s_iter != sc_SingleKeys.end ()) {

                            x_AddQualifierToFeature (sfp, curr_feat_name, qual, val, container, reader.GetLineNumber(), real_seqid);
                        }
                    }
                } else if (! feat.empty ()) {
                
                    // unrecognized location

                    if ((flags & CFeature_table_reader::fReportBadKey) != 0) {
                        x_ProcessMsg( container, 
                            ILineError::eProblem_FeatureBadStartAndOrStop, eDiag_Warning,
                            real_seqid, reader.GetLineNumber(),
                            feat );
                    }
                }
            }
        }
    }

    return sap;
}


CRef<CSeq_feat> CFeature_table_reader_imp::CreateSeqFeat (
    const string& feat,
    CSeq_loc& location,
    const CFeature_table_reader::TFlags flags,
    IErrorContainer* container,
    unsigned int line,
    const string &seq_id,
    ITableFilter *filter
)

{
    CRef<CSeq_feat> sfp (new CSeq_feat);

    sfp->ResetLocation ();

    if ( ! x_SetupSeqFeat (sfp, feat, flags, line, seq_id, container, filter) ) {

        // bad feature, make dummy

        sfp->SetData ().Select (CSeqFeatData::e_not_set);
        /*
        sfp->SetData ().Select (CSeqFeatData::e_Imp);
        CSeqFeatData& sfdata = sfp->SetData ();
        CImp_feat_Base& imp = sfdata.SetImp ();
        imp.SetKey ("bad_feature");
        */
    }
 
    sfp->SetLocation (location);

    return sfp;
}


void CFeature_table_reader_imp::AddFeatQual (
    CRef<CSeq_feat> sfp,
    const string& feat_name,
    const string& qual,
    const string& val,
    const CFeature_table_reader::TFlags flags,
    IErrorContainer* container,
    int line, 	
    const string &seq_id )

{
    if ((! qual.empty ()) && (! val.empty ())) {

        if (! x_AddQualifierToFeature (sfp, feat_name, qual, val, container, line, seq_id)) {

            // unrecognized qualifier key

            if ((flags & CFeature_table_reader::fReportBadKey) != 0) {
                ERR_POST_X (5, Warning << "Unrecognized qualifier '" << qual << "'");
            }

            if ((flags & CFeature_table_reader::fKeepBadKey) != 0) {
                x_AddGBQualToFeature (sfp, qual, val);
            }
        }

    } else if ((! qual.empty ()) && (val.empty ())) {

        // check for the few qualifiers that do not need a value

        TSingleSet::const_iterator s_iter = sc_SingleKeys.find (qual.c_str ());
        if (s_iter != sc_SingleKeys.end ()) {

            x_AddQualifierToFeature (sfp, feat_name, qual, val, container, line, seq_id);

        }
    }
}


// public access functions

CRef<CSeq_annot> CFeature_table_reader::ReadSequinFeatureTable (
    CNcbiIstream& ifs,
    const string& seqid,
    const string& annotname,
    const TFlags flags,
    IErrorContainer* container,
    ITableFilter *filter
)
{
    CStreamLineReader reader(ifs);
    return ReadSequinFeatureTable(reader, seqid, annotname, flags, container, filter);
}


CRef<CSeq_annot> CFeature_table_reader::ReadSequinFeatureTable (
    ILineReader& reader,
    const string& seqid,
    const string& annotname,
    const TFlags flags,
    IErrorContainer* container,
    ITableFilter *filter
)
{
    // just read features from 5-column table

    CRef<CSeq_annot> sap = x_GetImplementation().ReadSequinFeatureTable 
      (reader, seqid, annotname, flags, container, filter);

    // go through all features and demote single interval seqlocmix to seqlocint
    for (CTypeIterator<CSeq_feat> fi(*sap); fi; ++fi) {
        CSeq_feat& feat = *fi;
        CSeq_loc& location = feat.SetLocation ();
        if (location.IsMix ()) {
            CSeq_loc_mix& mx = location.SetMix ();
            CSeq_loc &keep_loc(*mx.Set ().front ());
            CRef<CSeq_loc> guard_loc(&keep_loc);            
            switch (mx.Get ().size ()) {
                case 0:
                    location.SetNull ();
                    break;
                case 1:
                    feat.SetLocation (*mx.Set ().front ());
                    break;
                default:
                    break;
            }
        }
    }

    return sap;
}


CRef<CSeq_annot> CFeature_table_reader::ReadSequinFeatureTable (
    CNcbiIstream& ifs,
    const TFlags flags,
    IErrorContainer* container,
    ITableFilter *filter
)
{
    CStreamLineReader reader(ifs);
    return ReadSequinFeatureTable(reader, flags, container, filter);
}


CRef<CSeq_annot> CFeature_table_reader::ReadSequinFeatureTable (
    ILineReader& reader,
    const TFlags flags,
    IErrorContainer* container,
    ITableFilter *filter
)
{
    string fst, scd, seqid, annotname;

    // first look for >Feature line, extract seqid and optional annotname
    while (seqid.empty () && !reader.AtEOF() ) {

        CTempString line = *++reader;

        if (! line.empty ()) {
            if ( s_IsFeatureLineAndFix(line) ) {
                if (NStr::StartsWith (line, ">Feature")) {
                    NStr::SplitInTwo (line, " ", fst, scd);
                    NStr::SplitInTwo (scd, " ", seqid, annotname);
                }
            }
        }
    }

    // then read features from 5-column table
    return ReadSequinFeatureTable (reader, seqid, annotname, flags, container, filter);

}


void CFeature_table_reader::ReadSequinFeatureTables(
    CNcbiIstream& ifs,
    CSeq_entry& entry,
    const TFlags flags,
    IErrorContainer* container,
    ITableFilter *filter
)
{
    CStreamLineReader reader(ifs);
    return ReadSequinFeatureTables(reader, entry, flags, container, filter);
}


void CFeature_table_reader::ReadSequinFeatureTables(
    ILineReader& reader,
    CSeq_entry& entry,
    const TFlags flags,
    IErrorContainer* container,
    ITableFilter *filter
)
{
    while ( !reader.AtEOF() ) {
        CRef<CSeq_annot> annot = ReadSequinFeatureTable(reader, flags, container, filter);
        if (entry.IsSeq()) { // only one place to go
            entry.SetSeq().SetAnnot().push_back(annot);
            continue;
        }
        _ASSERT(annot->GetData().IsFtable());
        if (annot->GetData().GetFtable().empty()) {
            continue;
        }
        // otherwise, take the first feature, which should be representative
        const CSeq_feat& feat    = *annot->GetData().GetFtable().front();
        const CSeq_id*   feat_id = feat.GetLocation().GetId();
        CBioseq*         seq     = NULL;
        _ASSERT(feat_id); // we expect a uniform sequence ID
        for (CTypeIterator<CBioseq> seqit(entry);  seqit  &&  !seq;  ++seqit) {
            ITERATE (CBioseq::TId, seq_id, seqit->GetId()) {
                if (feat_id->Match(**seq_id)) {
                    seq = &*seqit;
                    break;
                }
            }
        }
        if (seq) { // found a match
            seq->SetAnnot().push_back(annot);
        } else { // just package on the set
            ERR_POST_X(6, Warning
                       << "ReadSequinFeatureTables: unable to find match for "
                       << feat_id->AsFastaString());
            entry.SetSet().SetAnnot().push_back(annot);
        }
    }
}


CRef<CSeq_feat> CFeature_table_reader::CreateSeqFeat (
    const string& feat,
    CSeq_loc& location,
    const TFlags flags,
    IErrorContainer* container,
    unsigned int line,
    string *seq_id,
    ITableFilter *filter
)

{
    return x_GetImplementation ().CreateSeqFeat (feat, location, flags, container, line, 
        (seq_id ? *seq_id : string() ), filter);
}


void CFeature_table_reader::AddFeatQual (
    CRef<CSeq_feat> sfp,
    const string& feat_name,
    const string& qual,
    const string& val,
    const CFeature_table_reader::TFlags flags,
    IErrorContainer* container,
    int line, 	
    const string &seq_id 
)

{
    x_GetImplementation ().AddFeatQual ( sfp, feat_name, qual, val, flags, container, line, seq_id ) ;
}


END_objects_SCOPE
END_NCBI_SCOPE
