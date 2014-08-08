#ifndef CHECKING_CLASS
#define CHECKING_CLASS

/*  $Id: hchecking_class.hpp 385644 2013-01-11 15:52:16Z chenj $
 * ==========================================================================
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
 * ==========================================================================
 *
 * Author:  Jie Chen
 *
 * File Description:
 *   headfile of cpp discrepancy report checking class
 *
 */

#include <corelib/ncbistd.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbiargs.hpp>
#include <connect/ncbi_core_cxx.hpp>

#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/seq_vector.hpp>
#include <objmgr/seqdesc_ci.hpp>
#include <objmgr/feat_ci.hpp>
#include <objmgr/align_ci.hpp>
#include <objmgr/seq_feat_handle.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>
#include <objects/submit/Submit_block.hpp>

#include "hDiscRep_tests.hpp"

using namespace ncbi;
using namespace objects;

BEGIN_NCBI_SCOPE

namespace DiscRepNmSpc{

   class CCheckingClass 
   {
      public:
         CCheckingClass() :
             num_seq_feats(0),
             num_qual_locus_tag(0)
             {
                // subtypes cover type: RBS, exon and intron for IMP;
                sel_seqfeat.IncludeFeatType(CSeqFeatData::e_Gene)
                           .IncludeFeatType(CSeqFeatData::e_Cdregion)
                           .IncludeFeatType(CSeqFeatData::e_Rna)
                           .IncludeFeatSubtype(CSeqFeatData::eSubtype_RBS)
                           .IncludeFeatSubtype(CSeqFeatData::eSubtype_exon)
                           .IncludeFeatSubtype(CSeqFeatData::eSubtype_intron)
                           .IncludeFeatType(CSeqFeatData::e_Prot);

                sel_seqfeat_4_seq_entry.IncludeFeatType(CSeqFeatData::e_Pub)
                                       .IncludeFeatType(CSeqFeatData::e_Biosrc);

                sel_seqdesc.reserve(10);
                sel_seqdesc.push_back(CSeqdesc::e_Pub);
                sel_seqdesc.push_back(CSeqdesc::e_Comment);
                sel_seqdesc.push_back(CSeqdesc::e_Source);
                sel_seqdesc.push_back(CSeqdesc::e_Title);
                sel_seqdesc.push_back(CSeqdesc::e_User);
                sel_seqdesc.push_back(CSeqdesc::e_Org);

                sel_seqdesc_4_bioseq.reserve(10);
                sel_seqdesc_4_bioseq.push_back(CSeqdesc::e_Source);
                sel_seqdesc_4_bioseq.push_back(CSeqdesc::e_Molinfo);
                sel_seqdesc_4_bioseq.push_back(CSeqdesc::e_Title);
                sel_seqdesc_4_bioseq.push_back(CSeqdesc::e_User);
                sel_seqdesc_4_bioseq.push_back(CSeqdesc::e_Genbank);
             }; 

         void CheckBioseq( CBioseq& bioseq );
         void CheckSeqInstMol(CSeq_inst& seq_inst, CBioseq& bioseq);
         void CheckSeqFeat( CSeq_feat & seq_feat );
         void CheckGbQual( const vector < CRef< CGb_qual > >& gb_qual );
         void CheckSeqEntry ( CRef <CSeq_entry> seq_entry);
         void CheckBioseqSet ( CBioseq_set& bioseq_set);

         void CollectRepData();

         bool HasLocusTag (void) {
               return has_locus_tag;
         };

         int GetNumSeqFeats(void) {
             return num_seq_feats;
         };

         template < class T >
         void GoTests(vector <CRef < CTestAndRepData> >& test_category, const T& obj) 
         {
            NON_CONST_ITERATE (vector <CRef <CTestAndRepData> >, it, test_category)
{
//cerr << "GetNAme " << (*it)->GetName() << endl;
                  (*it)->TestOnObj(obj);
//cerr << "done\n";
}
         };

         void GoGetRep(vector < CRef < CTestAndRepData> >& test_category);

         static bool SortByFrom(const CSeq_feat* seqfeat1, const CSeq_feat* seqfeat2);

      protected:
         bool CanGetOrgMod(const CBioSource& biosrc);
         void CollectSeqdescFromSeqEntry(const CSeq_entry_Handle& seq_entry_h);

      private:
         SAnnotSelector sel_seqfeat, sel_seqfeat_4_seq_entry;
         vector <CSeqdesc :: E_Choice> sel_seqdesc, sel_seqdesc_4_bioseq;

         int num_seq_feats;
         int num_qual_locus_tag;
         bool has_locus_tag;
   };
};

END_NCBI_SCOPE

#endif // CHECKING_CLASS
