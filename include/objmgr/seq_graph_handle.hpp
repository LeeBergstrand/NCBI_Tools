#ifndef SEQ_GRAPH_HANDLE__HPP
#define SEQ_GRAPH_HANDLE__HPP

/*  $Id: seq_graph_handle.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author: Aleksey Grichenko, Eugene Vasilchenko
*
* File Description:
*   Seq-graph handle
*
*/

#include <corelib/ncbiobj.hpp>
#include <corelib/ncbi_limits.h>
#include <objects/seqres/Seq_graph.hpp>
#include <objmgr/seq_annot_handle.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


/** @addtogroup ObjectManagerHandles
 *
 * @{
 */


class CSeq_annot_Handle;
class CMappedGraph;
class CAnnotObject_Info;

/////////////////////////////////////////////////////////////////////////////
///
///  CSeq_graph_Handle --
///
///  Proxy to access seq-graph objects data
///

template<typename Handle>
class CSeq_annot_Add_EditCommand;
template<typename Handle>
class CSeq_annot_Replace_EditCommand;
template<typename Handle>
class CSeq_annot_Remove_EditCommand;

class NCBI_XOBJMGR_EXPORT CSeq_graph_Handle
{
public:
    CSeq_graph_Handle(void);

    void Reset(void);

    DECLARE_OPERATOR_BOOL(m_Annot && m_AnnotIndex != eNull && !IsRemoved());

    /// Get scope this handle belongs to
    CScope& GetScope(void) const;

    /// Get an annotation handle for the current seq-graph
    const CSeq_annot_Handle& GetAnnot(void) const;

    /// Get constant reference to the current seq-graph
    CConstRef<CSeq_graph> GetSeq_graph(void) const;

    // Mappings for CSeq_graph methods
    bool IsSetTitle(void) const;
    const string& GetTitle(void) const;
    bool IsSetComment(void) const;
    const string& GetComment(void) const;
    bool IsSetLoc(void) const;
    const CSeq_loc& GetLoc(void) const;
    bool IsSetTitle_x(void) const;
    const string& GetTitle_x(void) const;
    bool IsSetTitle_y(void) const;
    const string& GetTitle_y(void) const;
    bool IsSetComp(void) const;
    TSeqPos GetComp(void) const;
    bool IsSetA(void) const;
    double GetA(void) const;
    bool IsSetB(void) const;
    double GetB(void) const;
    TSeqPos GetNumval(void) const;
    const CSeq_graph::TGraph& GetGraph(void) const;

    /// Return true if this Seq-graph was removed already
    bool IsRemoved(void) const;
    /// Remove the Seq-graph from Seq-annot
    void Remove(void) const;
    /// Replace the Seq-graph with new Seq-graph object.
    /// All indexes are updated correspondingly.
    void Replace(const CSeq_graph& new_obj) const;
    /// Update index after manual modification of the object
    void Update(void) const;

private:
    friend class CMappedGraph;
    friend class CSeq_annot_EditHandle;
    typedef Int4 TIndex;
    enum {
        eNull = kMax_I4
    };

    const CSeq_graph& x_GetSeq_graph(void) const;

    CSeq_graph_Handle(const CSeq_annot_Handle& annot, TIndex index);

    CSeq_annot_Handle          m_Annot;
    TIndex                     m_AnnotIndex;

private:
    friend class CSeq_annot_Add_EditCommand<CSeq_graph_Handle>;
    friend class CSeq_annot_Replace_EditCommand<CSeq_graph_Handle>;
    friend class CSeq_annot_Remove_EditCommand<CSeq_graph_Handle>;

    /// Remove the Seq-graph from Seq-annot
    void x_RealRemove(void) const;
    /// Replace the Seq-graph with new Seq-graph object.
    /// All indexes are updated correspondingly.
    void x_RealReplace(const CSeq_graph& new_obj) const;

};


inline
CSeq_graph_Handle::CSeq_graph_Handle(void)
    : m_AnnotIndex(eNull)
{
}


inline
const CSeq_annot_Handle& CSeq_graph_Handle::GetAnnot(void) const
{
    return m_Annot;
}


inline
CScope& CSeq_graph_Handle::GetScope(void) const
{
    return GetAnnot().GetScope();
}


inline
bool CSeq_graph_Handle::IsSetTitle(void) const
{
    return x_GetSeq_graph().IsSetTitle();
}


inline
const string& CSeq_graph_Handle::GetTitle(void) const
{
    return x_GetSeq_graph().GetTitle();
}


inline
bool CSeq_graph_Handle::IsSetComment(void) const
{
    return x_GetSeq_graph().IsSetComment();
}


inline
const string& CSeq_graph_Handle::GetComment(void) const
{
    return x_GetSeq_graph().GetComment();
}


inline
bool CSeq_graph_Handle::IsSetLoc(void) const
{
    return x_GetSeq_graph().IsSetLoc();
}


inline
const CSeq_loc& CSeq_graph_Handle::GetLoc(void) const
{
    return x_GetSeq_graph().GetLoc();
}


inline
bool CSeq_graph_Handle::IsSetTitle_x(void) const
{
    return x_GetSeq_graph().IsSetTitle_x();
}


inline
const string& CSeq_graph_Handle::GetTitle_x(void) const
{
    return x_GetSeq_graph().GetTitle_x();
}


inline
bool CSeq_graph_Handle::IsSetTitle_y(void) const
{
    return x_GetSeq_graph().IsSetTitle_y();
}


inline
const string& CSeq_graph_Handle::GetTitle_y(void) const
{
    return x_GetSeq_graph().GetTitle_y();
}


inline
bool CSeq_graph_Handle::IsSetComp(void) const
{
    return x_GetSeq_graph().IsSetComp();
}


inline
TSeqPos CSeq_graph_Handle::GetComp(void) const
{
    return x_GetSeq_graph().GetComp();
}


inline
bool CSeq_graph_Handle::IsSetA(void) const
{
    return x_GetSeq_graph().IsSetA();
}


inline
double CSeq_graph_Handle::GetA(void) const
{
    return x_GetSeq_graph().GetA();
}


inline
bool CSeq_graph_Handle::IsSetB(void) const
{
    return x_GetSeq_graph().IsSetB();
}


inline
double CSeq_graph_Handle::GetB(void) const
{
    return x_GetSeq_graph().GetB();
}


inline
TSeqPos CSeq_graph_Handle::GetNumval(void) const
{
    return x_GetSeq_graph().GetNumval();
}


inline
const CSeq_graph::TGraph& CSeq_graph_Handle::GetGraph(void) const
{
    return x_GetSeq_graph().GetGraph();
}


/* @} */


END_SCOPE(objects)
END_NCBI_SCOPE

#endif  // SEQ_GRAPH_HANDLE__HPP
