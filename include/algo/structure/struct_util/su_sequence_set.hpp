/*  $Id: su_sequence_set.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Authors:  Paul Thiessen
*
* File Description:
*      Classes to hold sets of sequences
*
* ===========================================================================
*/

#ifndef SU_SEQUENCE_SET__HPP
#define SU_SEQUENCE_SET__HPP

#include <corelib/ncbistd.hpp>
#include <corelib/ncbistl.hpp>
#include <corelib/ncbiobj.hpp>

#include <string>
#include <list>

#include <objects/seqset/Seq_entry.hpp>
#include <objects/seq/Bioseq.hpp>
#include <objects/seqloc/Seq_id.hpp>


BEGIN_SCOPE(struct_util)

class NCBI_STRUCTUTIL_EXPORT Sequence : public ncbi::CObject
{
public:
    Sequence(ncbi::objects::CBioseq& bioseq);
    ~Sequence(void);

    ncbi::CRef < ncbi::objects::CBioseq > m_bioseqASN;

    std::string m_sequenceString, m_description;
    bool m_isProtein;
    unsigned int Length(void) const { return m_sequenceString.size(); }

    // Seq-id stuff
	const ncbi::objects::CSeq_id& GetPreferredIdentifier(void) const;
    const ncbi::objects::CBioseq::TId& GetAllIdentifiers(void) const { return m_bioseqASN->GetId(); }
	bool MatchesSeqId(const ncbi::objects::CSeq_id& seqID) const;
    std::string IdentifierString(void) const;
};

class NCBI_STRUCTUTIL_EXPORT SequenceSet : public ncbi::CObject
{
public:
    typedef std::list < ncbi::CRef < ncbi::objects::CSeq_entry > > SeqEntryList;
    SequenceSet(SeqEntryList& seqEntries);

    typedef std::list < ncbi::CRef < Sequence > > SequenceList;
    SequenceList m_sequences;
};

END_SCOPE(struct_util)

#endif // SU_SEQUENCE_SET__HPP
