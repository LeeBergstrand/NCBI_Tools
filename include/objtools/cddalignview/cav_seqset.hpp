/*  $Id: cav_seqset.hpp 103491 2007-05-04 17:18:18Z kazimird $
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

#ifndef CAV_SEQUENCE_SET__HPP
#define CAV_SEQUENCE_SET__HPP

#include <corelib/ncbistl.hpp>
#include <corelib/ncbiobj.hpp>

#include <string>
#include <list>
#include <vector>

#include <objects/seqset/Seq_entry.hpp>


BEGIN_NCBI_SCOPE

BEGIN_SCOPE(objects)
class CBioseq_set;
class CBioseq;
class CSeq_id;
END_SCOPE(objects)

typedef list < CRef < objects::CSeq_entry > > SeqEntryList;

class Sequence;
class MasterSlaveAlignment;

class SequenceSet
{
private:
    int status;
    void UnpackSeqSet(const objects::CBioseq_set& bss);
    void UnpackSeqEntry(const objects::CSeq_entry& seqEntry);

public:
    SequenceSet(const objects::CSeq_entry& seqEntry);
    SequenceSet(const SeqEntryList& seqEntries);
    ~SequenceSet();

    typedef vector < const Sequence * > SequenceList;
    SequenceList sequences;

    int Status(void) const { return status; }

    // there is one and only one sequence in this set that will be the master
    // for all alignments of sequences from this set
    const Sequence *master;
};

class Sequence
{
private:
    int status;

public:
    Sequence(const objects::CBioseq& bioseq);

    CConstRef < objects::CBioseq > bioseqASN;
    typedef list < CRef < objects::CSeq_id > > SeqIdList;
    const SeqIdList seqIDs;
    string sequenceString, description;

    static const int NOT_SET;
    int mmdbLink;

    unsigned int Length(void) const { return sequenceString.size(); }
    bool Matches(const objects::CSeq_id& seqID) const;
    bool Matches(const SeqIdList& others) const;
    string GetTitle(void) const;    // includes some indicator if id type
    string GetLabel(void) const;    // just the content

    int Status(void) const { return status; }
};

END_NCBI_SCOPE


#endif // CAV_SEQUENCE_SET__HPP
