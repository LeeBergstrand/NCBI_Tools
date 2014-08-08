/*  $Id: util.hpp 398297 2013-05-03 21:10:05Z rafanovi $
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
 *   check biosource and structured comment descriptors against biosample database
 *
 */

#ifndef BIOSAMPLE_CHK__UTIL__HPP
#define BIOSAMPLE_CHK__UTIL__HPP

#include <corelib/ncbistd.hpp>
#include <corelib/ncbistr.hpp>

#include <objects/seq/Seq_descr.hpp>
#include <objects/seq/Seqdesc.hpp>
#include <objects/seqfeat/BioSource.hpp>

#include <objmgr/bioseq_handle.hpp>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

CRef< CSeq_descr > GetBiosampleData(string accession);

vector<string> GetBiosampleIDs(CBioseq_Handle bh);

class CBiosampleFieldDiff
{
public:
    CBiosampleFieldDiff() {};
    CBiosampleFieldDiff(string sequence_id, string biosample_id, string field_name, string src_val, string sample_val) :
        m_SequenceID(sequence_id), m_BiosampleID(biosample_id), m_FieldName(field_name), m_SrcVal(src_val), m_SampleVal(sample_val)
        {};

    ~CBiosampleFieldDiff(void);

    void Print(ncbi::CNcbiOstream & stream);
    void Print(ncbi::CNcbiOstream & stream, const CBiosampleFieldDiff& prev);
    const string& GetSequenceId() { return m_SequenceID; };
    void SetSequenceId(string id) { m_SequenceID = id; };
    const string& GetFieldName() { return m_FieldName; };
    const string& GetSrcVal() { return m_SrcVal; };

    int CompareAllButSequenceID(const CBiosampleFieldDiff& other);
    int Compare(const CBiosampleFieldDiff& other);

private:
    string m_SequenceID;
    string m_BiosampleID;
    string m_FieldName;
    string m_SrcVal;
    string m_SampleVal;
};


vector<CBiosampleFieldDiff *> GetFieldDiffs(string sequence_id, string biosample_id, const CBioSource& src, const CBioSource& sample);

END_SCOPE(objects)
END_NCBI_SCOPE

#endif //BIOSAMPLE_CHK__UTIL__HPP