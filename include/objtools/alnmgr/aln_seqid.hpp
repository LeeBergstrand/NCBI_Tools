#ifndef OBJTOOLS_ALNMGR___ALN_SEQID__HPP
#define OBJTOOLS_ALNMGR___ALN_SEQID__HPP
/*  $Id: aln_seqid.hpp 359352 2012-04-12 15:23:21Z grichenk $
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
* Author:  Kamen Todorov, NCBI
*
* File Description:
*   Alignment Seq-id
*
* ===========================================================================
*/


#include <corelib/ncbistd.hpp>
#include <corelib/ncbiobj.hpp>

#include <objects/seq/Seq_inst.hpp>
#include <objects/seq/seq_id_handle.hpp>
#include <objmgr/bioseq_handle.hpp>


BEGIN_NCBI_SCOPE
USING_SCOPE(objects);


/// Wrapper interface for seq-ids used in alignments. In addition to seq-id
/// must provide the corresponding sequence type and base width.
/// For implementation see CAlnSeqId.
/// @sa CAlnSeqId
class NCBI_XALNMGR_EXPORT IAlnSeqId
{
public:
    /// Comparison operators
    virtual bool operator== (const IAlnSeqId& id) const = 0;
    virtual bool operator!= (const IAlnSeqId& id) const = 0;
    virtual bool operator<  (const IAlnSeqId& id) const = 0;

    /// Get CSeq_id
    virtual const CSeq_id& GetSeqId(void) const = 0;

    /// Get string representation of the seq-id
    virtual string AsString(void) const = 0;

    typedef CSeq_inst::TMol TMol;
    /// Check sequence type
    virtual TMol GetSequenceType(void) const = 0;
    bool IsProtein(void) const;
    bool IsNucleotide(void) const;

    /// Get base width for the sequence.
    /// @return 1 for nucleotides, 3 for proteins
    virtual int GetBaseWidth(void) const = 0;
    /// Set base width for the sequence.
    virtual void SetBaseWidth(int base_width) = 0;

    /// Virtual destructor
    virtual ~IAlnSeqId(void)
    {
    }
};


typedef CIRef<IAlnSeqId> TAlnSeqIdIRef;


struct SAlnSeqIdIRefComp :
    public binary_function<TAlnSeqIdIRef, TAlnSeqIdIRef, bool>
{
    bool operator()(const TAlnSeqIdIRef& l_id_ref,
                        const TAlnSeqIdIRef& r_id_ref) const
    {
        return *l_id_ref < *r_id_ref;
    }
};


struct SAlnSeqIdRefEqual :
    public binary_function<TAlnSeqIdIRef, TAlnSeqIdIRef, bool>
{
    bool operator()(const TAlnSeqIdIRef& l_id_ref,
                        const TAlnSeqIdIRef& r_id_ref) const
    {
        return *l_id_ref == *r_id_ref;
    }
};


/// Default IAlnSeqId implementation based on CSeq_id_Handle.
/// @sa CAlnSeqIdConverter
/// @sa CAlnSeqIdsExtract
class NCBI_XALNMGR_EXPORT CAlnSeqId :
    public CObject,
    public CSeq_id_Handle,
    public IAlnSeqId
{
public:
    /// Constructor
    CAlnSeqId(const CSeq_id& id)
        : CSeq_id_Handle(CSeq_id_Handle::GetHandle(id)),
          m_Seq_id(&id),
          m_Mol(CSeq_inst::eMol_not_set),
          m_BaseWidth(1)
    {
    }

    virtual const CSeq_id& GetSeqId(void) const;

    /// Sequence label - same as CSeq_id_Handle::AsString()
    virtual string AsString(void) const;

    /// Comparison operators
    virtual bool operator== (const IAlnSeqId& id) const;
    virtual bool operator!= (const IAlnSeqId& id) const;
    virtual bool operator<  (const IAlnSeqId& id) const;

    /// Store bioseq handle for the id.
    virtual void SetBioseqHandle(const CBioseq_Handle& handle);

    /// Check sequence type
    virtual TMol GetSequenceType(void) const;

    /// Base Width - 1 = nucleotide, 3 = protein.
    virtual int GetBaseWidth(void) const;
    virtual void SetBaseWidth(int base_width);

private:
    CConstRef<CSeq_id> m_Seq_id;
    CBioseq_Handle     m_BioseqHandle;
    mutable TMol       m_Mol;
    int                m_BaseWidth;
};


END_NCBI_SCOPE

#endif  // OBJTOOLS_ALNMGR___ALN_SEQID__HPP
