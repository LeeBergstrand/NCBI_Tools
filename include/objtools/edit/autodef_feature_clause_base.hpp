#ifndef OBJTOOLS_EDIT___AUTODEF_FEATURE_CLAUSE_BASE__HPP
#define OBJTOOLS_EDIT___AUTODEF_FEATURE_CLAUSE_BASE__HPP

/*  $Id: autodef_feature_clause_base.hpp 136370 2008-08-04 13:15:27Z dicuccio $
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
* Author:  Colleen Bollin
*
* File Description:
*   Creates unique definition lines for sequences in a set using organism
*   descriptions and feature clauses.
*/

#include <corelib/ncbistd.hpp>
#include <objects/seqfeat/Seq_feat.hpp>

#include <objmgr/util/seq_loc_util.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)
    
    
class NCBI_XOBJEDIT_EXPORT CAutoDefFeatureClause_Base
{
public:
    typedef vector<CAutoDefFeatureClause_Base *> TClauseList;

    CAutoDefFeatureClause_Base(bool suppress_locus_tags);
    virtual ~CAutoDefFeatureClause_Base();
    
    virtual void AddSubclause (CAutoDefFeatureClause_Base *subclause);

    string PrintClause(bool print_typeword, bool typeword_is_plural);
    
    virtual CSeqFeatData::ESubtype  GetMainFeatureSubtype();
    unsigned int GetNumSubclauses() { return m_ClauseList.size(); }
    virtual void Label();
    virtual bool AddmRNA (CAutoDefFeatureClause_Base *mRNAClause);
    virtual bool AddGene (CAutoDefFeatureClause_Base *gene_clause);
    
    virtual sequence::ECompare CompareLocation(const CSeq_loc& loc);
    virtual void AddToOtherLocation(CRef<CSeq_loc> loc);
    virtual void AddToLocation(CRef<CSeq_loc> loc, bool also_set_partials = true);
    virtual bool SameStrand(const CSeq_loc& loc);
    virtual bool IsPartial() { return false; }
    virtual bool IsTransposon() { return false; }
    virtual bool IsInsertionSequence() { return false; }
    virtual bool IsControlRegion() { return false; }
    virtual bool IsEndogenousVirusSourceFeature() { return false; }
    virtual bool IsGeneCluster() { return false; }
    virtual bool IsNoncodingProductFeat() { return false; }
    virtual bool IsIntergenicSpacer() { return false; }
    virtual bool IsSatelliteClause() { return false; }    
    virtual bool IsExonList() { return false; }
    virtual CAutoDefFeatureClause_Base *FindBestParentClause(CAutoDefFeatureClause_Base * subclause, bool gene_cluster_opp_strand);
    
    void GroupClauses(bool gene_cluster_opp_strand);
    void RemoveNonSegmentClauses(CRange<TSeqPos> range);
    void GroupAltSplicedExons(CBioseq_Handle bh);

    virtual CRef<CSeq_loc> GetLocation();
    
    string ListClauses(bool allow_semicolons, bool suppress_final_and);

    bool IsGeneMentioned(CAutoDefFeatureClause_Base *gene_clause);
    bool IsTypewordFirst() { return m_ShowTypewordFirst; }
    bool DisplayAlleleName ();

    string GetInterval() { return m_Interval; }
    string GetTypeword() { return m_Typeword; }
    string GetDescription() { return m_Description; }
    string GetProductName() { return m_ProductName; }
    string GetGeneName() { return m_GeneName; }
    string GetAlleleName() { return m_AlleleName; }
    virtual void SetProductName(string product_name);
    bool   GetGeneIsPseudo() { return m_GeneIsPseudo; }
    bool   NeedPlural() { return m_MakePlural; }
    bool   IsAltSpliced() { return m_IsAltSpliced; }
    void   SetAltSpliced(string splice_name);
    bool IsMarkedForDeletion() { return m_DeleteMe; }
    void MarkForDeletion() { m_DeleteMe = true; }
    void SetMakePlural() { m_MakePlural = true; }
    bool HasmRNA() { return m_HasmRNA; }
    void SetSuppressLocusTag(bool do_suppress) { m_SuppressLocusTag = do_suppress; }
    void SetInfoOnly (bool info_only) { m_ClauseInfoOnly = info_only; }
    void PluralizeInterval();
    void PluralizeDescription();
    
    void ShowSubclauses();
    
    // Grouping functions
    void RemoveDeletedSubclauses();
   
    void GroupmRNAs();
    void GroupGenes();
    void GroupConsecutiveExons(CBioseq_Handle bh);
    void GroupSegmentedCDSs();
    void RemoveGenesMentionedElsewhere();
    void RemoveTransSplicedLeaders();
    void ConsolidateRepeatedClauses();
    void FindAltSplices();
    void TransferSubclauses(TClauseList &other_clause_list);
    void CountUnknownGenes();
    void ExpandExonLists();
    virtual void ReverseCDSClauseLists();
   
    virtual bool OkToGroupUnderByType(CAutoDefFeatureClause_Base *parent_clause) { return false; }
    virtual bool OkToGroupUnderByLocation(CAutoDefFeatureClause_Base *parent_clause, bool gene_cluster_opp_strand) { return false; }
    
    virtual void SuppressTransposonAndInsertionSequenceSubfeatures();
    
    void SuppressSubfeatures() { m_SuppressSubfeatures = true; }
    
    string FindGeneProductName(CAutoDefFeatureClause_Base *gene_clause);
    void AssignGeneProductNames(CAutoDefFeatureClause_Base *main_clause);
    
    void RemoveFeaturesByType(unsigned int feature_type);
    bool IsFeatureTypeLonely(unsigned int feature_type);
    void RemoveFeaturesInmRNAsByType(unsigned int feature_type);
    
    virtual bool ShouldRemoveExons();
    
    void RemoveUnwantedExons();
    
    virtual bool IsBioseqPrecursorRNA();
    void RemoveBioseqPrecursorRNAs();
   
protected:
    CAutoDefFeatureClause_Base();
    TClauseList  m_ClauseList;

    string       m_GeneName;
    string       m_AlleleName;
    bool         m_GeneIsPseudo;
    string       m_Interval;
    bool         m_IsAltSpliced;
    bool         m_HasmRNA;
    bool         m_HasGene;
    bool         m_MakePlural;
    bool         m_IsUnknown;
    bool         m_ClauseInfoOnly;
    bool m_Pluralizable;
    bool m_ShowTypewordFirst;
    string m_Typeword;
    bool   m_TypewordChosen;
    string m_Description;
    bool   m_DescriptionChosen;
    string m_ProductName;
    bool   m_ProductNameChosen;
    bool m_SuppressLocusTag;
    
    bool   m_SuppressSubfeatures;
    
    bool   m_DeleteMe;

    unsigned int x_LastIntervalChangeBeforeEnd ();
    bool x_OkToConsolidate (unsigned int clause1, unsigned int clause2);
    bool x_MeetAltSpliceRules (unsigned int clause1, unsigned int clause2, string &splice_name);

};


class NCBI_XOBJEDIT_EXPORT CAutoDefUnknownGeneList : public CAutoDefFeatureClause_Base
{
public:    
    CAutoDefUnknownGeneList(bool suppress_locus_tags);
    ~CAutoDefUnknownGeneList();
  
    virtual void Label();
    virtual bool IsRecognizedFeature() { return true; }
};


class NCBI_XOBJEDIT_EXPORT CAutoDefExonListClause : public CAutoDefFeatureClause_Base
{
public:
    CAutoDefExonListClause(CBioseq_Handle bh, bool suppress_locus_tags);
    
    virtual void AddSubclause (CAutoDefFeatureClause_Base *subclause);
    virtual void Label();
    virtual bool IsRecognizedFeature() { return true; }
    virtual bool IsExonList() { return true; }
    virtual bool OkToGroupUnderByLocation(CAutoDefFeatureClause_Base *parent_clause, bool gene_cluster_opp_strand);    
    virtual bool OkToGroupUnderByType(CAutoDefFeatureClause_Base *parent_clause);
    virtual CSeqFeatData::ESubtype GetMainFeatureSubtype();
    void SetSuppressFinalAnd(bool suppress) { m_SuppressFinalAnd = suppress; }
private:
    CRef<CSeq_loc> SeqLocIntersect (CRef<CSeq_loc> loc1, CRef<CSeq_loc> loc2);

    bool m_SuppressFinalAnd;
    CRef<CSeq_loc> m_ClauseLocation;    
    CBioseq_Handle m_BH;
};

END_SCOPE(objects)
END_NCBI_SCOPE

#endif //OBJTOOLS_EDIT___AUTODEF_FEATURE_CLAUSE_BASE__HPP
