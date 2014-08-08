#ifndef SEQ_TABLE_CI__HPP
#define SEQ_TABLE_CI__HPP

/*  $Id: seq_table_ci.hpp 386178 2013-01-16 18:34:29Z vasilche $
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
* Author: Eugene Vasilchenko
*
* File Description:
*   Object manager iterators
*
*/

#include <objmgr/annot_types_ci.hpp>
#include <objmgr/seq_annot_handle.hpp>
#include <objmgr/seq_entry_handle.hpp>
#include <objects/seqloc/Seq_loc.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


/** @addtogroup ObjectManagerIterators
 *
 * @{
 */

/////////////////////////////////////////////////////////////////////////////
///
/////////////////////////////////////////////////////////////////////////////
///
///  CSeq_table_CI --
///

class NCBI_XOBJMGR_EXPORT CSeq_table_CI : public CAnnotTypes_CI
{
public:
    /// Create an empty iterator
    CSeq_table_CI(void);

    /// Create an iterator that enumerates CSeq_table objects 
    /// related to the given bioseq
    CSeq_table_CI(const CBioseq_Handle& bioseq);

    /// Create an iterator that enumerates CSeq_table objects 
    /// related to the given bioseq
    ///
    /// @sa
    ///   SAnnotSelector
    CSeq_table_CI(const CBioseq_Handle& bioseq,
              const SAnnotSelector& sel);

    /// Create an iterator that enumerates CSeq_table objects 
    /// related to the given bioseq
    CSeq_table_CI(const CBioseq_Handle& bioseq,
              const CRange<TSeqPos>& range,
              ENa_strand strand = eNa_strand_unknown);

    /// Create an iterator that enumerates CSeq_table objects 
    /// related to the given bioseq
    ///
    /// @sa
    ///   SAnnotSelector
    CSeq_table_CI(const CBioseq_Handle& bioseq,
              const CRange<TSeqPos>& range,
              const SAnnotSelector& sel);

    /// Create an iterator that enumerates CSeq_table objects 
    /// related to the given bioseq
    ///
    /// @sa
    ///   SAnnotSelector
    CSeq_table_CI(const CBioseq_Handle& bioseq,
              const CRange<TSeqPos>& range,
              ENa_strand strand,
              const SAnnotSelector& sel);

    /// Create an iterator that enumerates CSeq_table objects 
    /// related to the given seq-loc
    CSeq_table_CI(CScope& scope,
              const CSeq_loc& loc);

    /// Create an iterator that enumerates CSeq_table objects 
    /// related to the given seq-loc
    ///
    /// @sa
    ///   SAnnotSelector
    CSeq_table_CI(CScope& scope,
              const CSeq_loc& loc,
              const SAnnotSelector& sel);

    /// Iterate all Seq-tables from the seq-annot regardless of their location
    CSeq_table_CI(const CSeq_annot_Handle& annot);

    /// Iterate all Seq-tables from the seq-annot regardless of their location
    ///
    /// @sa
    ///   SAnnotSelector
    CSeq_table_CI(const CSeq_annot_Handle& annot,
              const SAnnotSelector& sel);

    /// Iterate all Seq-tables from the seq-entry regardless of their location
    CSeq_table_CI(const CSeq_entry_Handle& entry);

    /// Iterate all Seq-tables from the seq-entry regardless of their location
    ///
    /// @sa
    ///   SAnnotSelector
    CSeq_table_CI(const CSeq_entry_Handle& entry,
              const SAnnotSelector& sel);

    bool IsMapped(void) const;
    const CSeq_loc& GetMappedLocation(void) const;
    const CSeq_loc& GetOriginalLocation(void) const;

    virtual ~CSeq_table_CI(void);

    CSeq_table_CI& operator++ (void);
    CSeq_table_CI& operator-- (void);

    DECLARE_OPERATOR_BOOL(IsValid());

private:
    CSeq_table_CI operator++ (int);
    CSeq_table_CI operator-- (int);

    mutable CConstRef<CSeq_loc> m_MappedLoc;
};


inline
CSeq_table_CI::CSeq_table_CI(void)
{
}

inline
CSeq_table_CI& CSeq_table_CI::operator++ (void)
{
    Next();
    return *this;
}

inline
CSeq_table_CI& CSeq_table_CI::operator-- (void)
{
    Prev();
    return *this;
}


/* @} */


END_SCOPE(objects)
END_NCBI_SCOPE

#endif  // SEQ_TABLE_CI__HPP
