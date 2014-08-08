/*  $Id: utilities.hpp 348762 2012-01-04 16:38:05Z kans $
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
 *      Definition for utility classes and functions.
 */

#ifndef VALIDATOR___UTILITIES__HPP
#define VALIDATOR___UTILITIES__HPP

#include <corelib/ncbistd.hpp>
#include <corelib/ncbistr.hpp>

#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqfeat/SeqFeatData.hpp>
#include <objects/seqset/Bioseq_set.hpp>
#include <objmgr/seq_vector.hpp>

#include <serial/iterator.hpp>

#include <vector>
#include <list>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CGb_qual;
class CScope;
class CSeq_entry;

BEGIN_SCOPE(validator)


// =============================================================================
//                                 Functions
// =============================================================================

bool IsClassInEntry(const CSeq_entry& se, CBioseq_set::EClass clss);
bool IsDeltaOrFarSeg(const CSeq_loc& loc, CScope* scope);
bool IsBlankStringList(const list< string >& str_list);
int GetGIForSeqId(const CSeq_id& id);
CScope::TIds GetSeqIdsForGI(int gi);

CSeqVector GetSequenceFromLoc(const CSeq_loc& loc, CScope& scope,
    CBioseq_Handle::EVectorCoding coding = CBioseq_Handle::eCoding_Iupac);

CSeqVector GetSequenceFromFeature(const CSeq_feat& feat, CScope& scope,
    CBioseq_Handle::EVectorCoding coding = CBioseq_Handle::eCoding_Iupac,
    bool product = false);

string GetSequenceStringFromLoc(const CSeq_loc& loc,  CScope& scope);


inline
bool IsResidue(unsigned char residue) { return residue <= 250; }
string GetAccessionFromObjects(const CSerialObject* obj, const CSeq_entry* ctx, CScope& scope, int* version);

CBioseq_set_Handle GetSetParent (CBioseq_set_Handle set, CBioseq_set::TClass set_class);
CBioseq_set_Handle GetSetParent (CBioseq_Handle set, CBioseq_set::TClass set_class);
CBioseq_set_Handle GetGenProdSetParent (CBioseq_set_Handle set);
CBioseq_set_Handle GetGenProdSetParent (CBioseq_Handle set);
CBioseq_set_Handle GetNucProtSetParent (CBioseq_Handle bioseq);

CBioseq_Handle GetNucBioseq (CBioseq_set_Handle bioseq_set);
CBioseq_Handle GetNucBioseq (CBioseq_Handle bioseq);

typedef enum {
  eAccessionFormat_valid = 0,
  eAccessionFormat_no_start_letters,
  eAccessionFormat_wrong_number_of_digits,
  eAccessionFormat_null,
  eAccessionFormat_too_long,
  eAccessionFormat_missing_version,
  eAccessionFormat_bad_version } EAccessionFormatError;

EAccessionFormatError ValidateAccessionString (string accession, bool require_version);

bool s_FeatureIdsMatch (const CFeat_id& f1, const CFeat_id& f2);
bool s_StringHasPMID (string str);
bool HasBadCharacter (string str);
bool EndsWithBadCharacter (string str);

bool IsBioseqWithIdInSet (const CSeq_id& id, CBioseq_set_Handle set);

typedef enum {
  eDateValid_valid = 0x0,
  eDateValid_bad_str = 0x01,
  eDateValid_bad_year = 0x02,
  eDateValid_bad_month = 0x04,
  eDateValid_bad_day = 0x08,
  eDateValid_bad_season = 0x10,
  eDateValid_bad_other = 0x20 ,
  eDateValid_empty_date = 0x40 } EDateValid;

int CheckDate (const CDate& date, bool require_full_date = false);
string GetDateErrorDescription (int flags);

bool IsBioseqTSA (const CBioseq& seq, CScope* scope);

void GetPubdescLabels 
(const CPubdesc& pd, 
 vector<int>& pmids, vector<int>& muids, vector<int>& serials,
 vector<string>& published_labels, vector<string>& unpublished_labels);

bool IsNCBIFILESeqId (const CSeq_id& id);
bool IsRefGeneTrackingObject (const CUser_object& user);

string GetValidatorLocationLabel (const CSeq_loc& loc);
void AppendBioseqLabel(string& str, const CBioseq& sq, bool supress_context);
string GetBioseqIdLabel(const CBioseq& sq, bool limited = false);

bool HasECnumberPattern (const string& str);

string GetAuthorsString (const CAuth_list& auth_list);

bool SeqIsPatent (const CBioseq& seq);
bool SeqIsPatent (CBioseq_Handle seq);

bool s_PartialAtGapOrNs (CScope* scope, const CSeq_loc& loc, unsigned int tag);

CBioseq_Handle BioseqHandleFromLocation (CScope* m_Scope, const CSeq_loc& loc);

END_SCOPE(validator)
END_SCOPE(objects)
END_NCBI_SCOPE

#endif  /* VALIDATOR___UTILITIES__HPP */
