/*  $Id: graphread.cpp 396271 2013-04-17 19:06:34Z vasilche $
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
 * Authors:  Eugene Vasilchenko
 *
 * File Description:
 *   Access to WGS files
 *
 */

#include <ncbi_pch.hpp>
#include <sra/readers/sra/graphread.hpp>
#include <sra/readers/ncbi_traces_path.hpp>
#include <corelib/ncbistr.hpp>
#include <corelib/ncbi_param.hpp>
#include <objects/general/general__.hpp>
#include <objects/seq/seq__.hpp>
#include <objects/seqset/seqset__.hpp>
#include <objects/seqloc/seqloc__.hpp>
#include <objects/seqalign/seqalign__.hpp>
#include <objects/seqres/seqres__.hpp>
#include <klib/rc.h>
#include <serial/objistrasnb.hpp>
#include <serial/serial.hpp>
#include <sra/error_codes.hpp>

#include <sra/readers/sra/kdbread.hpp>
#include <vdb/vdb-priv.h>
#include <kdb/table.h>
#include <kdb/meta.h>
#include <kdb/namelist.h>

BEGIN_NCBI_NAMESPACE;

#define NCBI_USE_ERRCODE_X   VDBGraphReader
NCBI_DEFINE_ERR_SUBCODE_X(1);

BEGIN_NAMESPACE(objects);


/////////////////////////////////////////////////////////////////////////////
// CVDBGraphDb_Impl
/////////////////////////////////////////////////////////////////////////////


CVDBGraphDb_Impl::SGraphTableCursor::SGraphTableCursor(const CVDBGraphDb_Impl& db)
    : m_Table(db.m_Mgr, db.GetPath()),
      m_Cursor(m_Table),
      INIT_VDB_COLUMN(SID),
      INIT_VDB_COLUMN(START),
      INIT_VDB_COLUMN(LEN),
      INIT_VDB_COLUMN(GR_Q0),
      INIT_VDB_COLUMN(GR_Q10),
      INIT_VDB_COLUMN(GR_Q50),
      INIT_VDB_COLUMN(GR_Q90),
      INIT_VDB_COLUMN(GR_Q100),
      INIT_VDB_COLUMN(GRAPH),
      INIT_VDB_COLUMN(SCALE)
{
}


CVDBGraphDb_Impl::CVDBGraphDb_Impl(CVDBMgr& mgr, CTempString path)
    : m_Mgr(mgr),
      m_Path(path)
{
    CRef<SGraphTableCursor> curs = Graph();

    uint64_t last_row = curs->m_Cursor.GetMaxRowId();
    SSeqInfo info;
    const KIndex* idx;
    if ( rc_t rc = VTableOpenIndexRead(curs->m_Table, &idx, "sid") ) {
        LOG_POST(Warning<<"CVDBGraphDb: cannot open SID index: "<<
                 CSraRcFormatter(rc)<<". Scanning sequentially.");
        for ( uint64_t row = 1; row <= last_row; ++row ) {
            // read range and names
            TSeqPos len = *curs->LEN(row), row_size = *curs->LEN(row);
            CVDBStringValue seq_id = curs->SID(row);
            if ( *seq_id == info.m_SeqId ) {
                info.m_SeqLength += len;
                continue;
            }
            if ( !info.m_SeqId.empty() ) {
                info.m_RowLast = row-1;
                m_SeqList.push_back(info);
            }
            info.m_SeqId = *seq_id;
            info.m_Seq_id_Handle = CSeq_id_Handle::GetHandle(info.m_SeqId);
            info.m_SeqLength = len;
            info.m_RowSize = row_size;
            info.m_StartBase = *curs->START(row);
            info.m_RowFirst = row;
        }
        if ( !info.m_SeqId.empty() ) {
            info.m_RowLast = last_row;
            m_SeqList.push_back(info);
        }
    }
    else {
        for ( uint64_t row = 1; row <= last_row; ++row ) {
            CVDBStringValue seq_id = curs->SID(row);
            info.m_SeqId = *seq_id;
            info.m_Seq_id_Handle = CSeq_id_Handle::GetHandle(info.m_SeqId);
            info.m_RowSize = *curs->LEN(row);
            info.m_StartBase = *curs->START(row);
            info.m_RowFirst = row;
            int64_t first;
            uint64_t count;
            if ( rc_t rc = KIndexFindText(idx, info.m_SeqId.c_str(),
                                          &first, &count, 0, 0) ) {
                NCBI_THROW2(CSraException, eInitFailed,
                            "Cannot read SID index", rc);
            }
            _ASSERT(row == first);
            _ASSERT(count > 0);
            info.m_RowLast = row += count-1;
            info.m_SeqLength =
                TSeqPos(*curs->START(row)+*curs->LEN(row)-info.m_StartBase);
            m_SeqList.push_back(info);
        }
        if ( rc_t rc = KIndexRelease(idx) ) {
            NCBI_THROW2(CSraException, eInitFailed,
                        "Cannot release SID index", rc);
        }
    }
    Put(curs);
    
    NON_CONST_ITERATE ( TSeqInfoList, it, m_SeqList ) {
        m_SeqMapBySeq_id.insert
            (TSeqInfoMapBySeq_id::value_type(it->m_Seq_id_Handle, it));
    }
}


CRef<CVDBGraphDb_Impl::SGraphTableCursor> CVDBGraphDb_Impl::Graph(void)
{
    CRef<SGraphTableCursor> curs = m_Graph.Get();
    if ( !curs ) {
        try {
            curs = new SGraphTableCursor(*this);
        }
        catch ( CSraException& exc ) {
            if ( !(GetPath().size() > 8 &&
                   NStr::StartsWith(GetPath(), "NA")) ) {
                throw;
            }
            string new_path = NCBI_TRACES04_PATH;
            new_path += "/nannot01/";
            new_path += GetPath().substr(2, 3);
            new_path += '/';
            new_path += GetPath().substr(5, 3);
            new_path += '/';
            new_path += GetPath();
            m_Path = new_path;
            curs = new SGraphTableCursor(*this);
        }
    }
    return curs;
}


/////////////////////////////////////////////////////////////////////////////
// CVDBGraphSeqIterator
/////////////////////////////////////////////////////////////////////////////

CVDBGraphSeqIterator::CVDBGraphSeqIterator(const CVDBGraphDb& db)
    : m_Db(db),
      m_Iter(db->GetSeqInfoList().begin())
{
}


CVDBGraphSeqIterator::CVDBGraphSeqIterator(const CVDBGraphDb& db,
                                           const CSeq_id_Handle& seq_id)
{
    CVDBGraphDb_Impl::TSeqInfoMapBySeq_id::const_iterator iter =
        db->m_SeqMapBySeq_id.find(seq_id);
    if ( iter != db->m_SeqMapBySeq_id.end() ) {
        m_Db = db;
        m_Iter = iter->second;
    }
}


const CVDBGraphDb_Impl::SSeqInfo& CVDBGraphSeqIterator::GetInfo(void) const
{
    if ( !*this ) {
        NCBI_THROW(CSraException, eInvalidState,
                   "CVDBGraphSeqIterator is invalid");
    }
    return *m_Iter;
}


CRef<CSeq_graph>
CVDBGraphSeqIterator::x_MakeGraphQ(CVDBGraphDb_Impl::SGraphTableCursor& curs,
                                   CSeq_loc& loc,
                                   const string& annot_name,
                                   int level) const
{
    CRef<CSeq_graph> graph(new CSeq_graph);
    return graph;
}


template<class DstVector, class SrcVector>
static void sx_Assign(DstVector& dst, const SrcVector& src)
{
    dst.clear();
    dst.reserve(src.size());
    ITERATE ( typename SrcVector, it, src ) {
        dst.push_back(*it);
    }
}


CRef<CSeq_annot>
CVDBGraphSeqIterator::GetAnnot(COpenRange<TSeqPos> range,
                               const string& annot_name,
                               TContentFlags content) const
{
    const CVDBGraphDb_Impl::SSeqInfo& info = GetInfo();
    if ( range.GetToOpen() > info.m_SeqLength ) {
        range.SetToOpen(info.m_SeqLength);
    }
    if ( range.Empty() ) {
        return null;
    }
    CRef<CSeq_annot> annot(new CSeq_annot);
    annot->SetData().SetGraph();
    if ( !annot_name.empty() ) {
        CRef<CAnnotdesc> desc(new CAnnotdesc);
        desc->SetName(annot_name);
        annot->SetDesc().Set().push_back(desc);
    }
    TSeqPos row_size = info.m_RowSize;
    if ( TSeqPos adjust = range.GetFrom() % row_size ) {
        range.SetFrom(range.GetFrom() - adjust);
    }
    if ( TSeqPos adjust = range.GetToOpen() % row_size ) {
        range.SetToOpen(min(info.m_SeqLength,
                            range.GetToOpen() + (row_size - adjust)));
    }
    CRef<CSeq_loc> loc(new CSeq_loc);
    loc->SetInt().SetId(*SerialClone(*info.m_Seq_id_Handle.GetSeqId()));
    loc->SetInt().SetFrom(range.GetFrom());
    loc->SetInt().SetTo(range.GetTo());
    CRef<CVDBGraphDb_Impl::SGraphTableCursor> curs(GetDb().Graph());

    for ( TContentFlags f = fGraphQ0; f & fGraphQAll; f <<= 1 ) {
        if ( !(content & f) ) {
            continue;
        }
        CVDBColumn* vdb_col = 0;
        int level = 0;
        switch ( f ) {
        case fGraphQ0: vdb_col = &curs->m_GR_Q0; level = 0; break;
        case fGraphQ10: vdb_col = &curs->m_GR_Q10; level = 10; break;
        case fGraphQ50: vdb_col = &curs->m_GR_Q50; level = 50; break;
        case fGraphQ90: vdb_col = &curs->m_GR_Q90; level = 90; break;
        case fGraphQ100: vdb_col = &curs->m_GR_Q100; level = 100; break;
        default: break;
        }
        if ( !vdb_col ) {
            continue;
        }

        CRef<CSeq_graph> graph(new CSeq_graph);
        string title;
        if ( !annot_name.empty() ) {
            title = annot_name + " ";
        }
        title += "values at "+NStr::IntToString(level)+"%";
        graph->SetTitle(title);

        typedef CVDBGraphDb_Impl::SGraphTableCursor::TGraphQ TValue;
        TValue max_v = 0;
        const TValue kMaxIntValue = kMax_I4;
        CInt_graph* int_graph = &graph->SetGraph().SetInt();
        CInt_graph::TValues* int_vv = &int_graph->SetValues();
        CRef<CReal_graph> real_graph;
        TSeqPos pos = range.GetFrom();
        uint64_t row = info.m_RowFirst + pos/row_size;
        uint32_t scale = curs->SCALE(row);
        for ( ; pos < range.GetToOpen(); ++row, pos += row_size ) {
            TValue v = *CVDBValueFor<TValue>(curs->m_Cursor, row, *vdb_col);
            if ( v > max_v ) {
                max_v = v;
                if ( max_v > kMaxIntValue ) {
                    // switch to real graph
                    real_graph = new CReal_graph;
                    CReal_graph::TValues* real_vv = &real_graph->SetValues();
                    sx_Assign(*real_vv, *int_vv);
                    graph->SetGraph().SetReal(*real_graph);
                    int_graph = 0;
                    int_vv = 0;
                    real_vv->push_back(double(v));
                    ++row; pos += row_size;
                    for ( ; pos < range.GetToOpen(); ++row, pos += row_size ) {
                        v = *CVDBValueFor<TValue>(curs->m_Cursor, row, *vdb_col);
                        if ( v > max_v ) {
                            max_v = v;
                        }
                        real_vv->push_back(double(v));
                    }
                    break;
                }
            }
            int_vv->push_back(int(v));
        }
        graph->SetLoc(*loc);
        graph->SetComp(row_size);
        graph->SetNumval((range.GetLength()+row_size-1)/row_size);
        graph->SetA(1./scale);
        if ( max_v <= kMax_UI1 ) {
            // use smaller byte representation
            CRef<CByte_graph> byte_graph(new CByte_graph);
            byte_graph->SetAxis(0);
            byte_graph->SetMin(0);
            byte_graph->SetMax(int(max_v));
            sx_Assign(byte_graph->SetValues(), *int_vv);
            graph->SetGraph().SetByte(*byte_graph);
            int_graph = 0;
            int_vv = 0;
        }
        else if ( max_v > kMaxIntValue ) {
            // need bigger double representation
            real_graph->SetAxis(0);
            real_graph->SetMin(0);
            real_graph->SetMax(double(max_v));
        }
        else {
            // int graph
            int_graph->SetAxis(0);
            int_graph->SetMin(0);
            int_graph->SetMax(int(max_v));
        }
        annot->SetData().SetGraph().push_back(graph);
    }

    if ( content & fGraphMain ) {
        CRef<CSeq_graph> graph(new CSeq_graph);
        if ( !annot_name.empty() ) {
            graph->SetTitle(annot_name);
        }

        typedef CVDBGraphDb_Impl::SGraphTableCursor::TGraphV TValue;
        TValue max_v = 0;
        const TValue kMaxIntValue = kMax_I4;
        CInt_graph* int_graph = &graph->SetGraph().SetInt();
        CInt_graph::TValues* int_vv = &int_graph->SetValues();
        CRef<CReal_graph> real_graph;
        TSeqPos pos = range.GetFrom();
        uint64_t row = info.m_RowFirst + pos/row_size;
        uint32_t scale = curs->SCALE(row);
        for ( ; pos < range.GetToOpen(); ++row, pos += row_size ) {
            //LOG_POST("Scale: "<<*curs->SCALE(row));
            CVDBValueFor<TValue> vv(curs->GRAPH(row));
            TSeqPos cnt = min(row_size, range.GetToOpen()-pos);
            for ( TSeqPos i = 0; i < cnt; ++i ) {
                TValue v = vv[i];
                if ( v > max_v ) {
                    max_v = v;
                    if ( max_v > kMaxIntValue ) {
                        // switch to real graph
                        real_graph = new CReal_graph;
                        CReal_graph::TValues* real_vv = &real_graph->SetValues();
                        sx_Assign(*real_vv, *int_vv);
                        graph->SetGraph().SetReal(*real_graph);
                        int_graph = 0;
                        int_vv = 0;
                        real_vv->push_back(double(v));
                        for ( ++i; i < cnt; ++i ) {
                            v = vv[i];
                            if ( v > max_v ) {
                                max_v = v;
                            }
                            real_vv->push_back(double(v));
                        }
                        break;
                    }
                }
                int_vv->push_back(int(v));
            }
        }
        graph->SetLoc(*loc);
        graph->SetNumval(range.GetLength());
        graph->SetA(1./scale);
        if ( max_v <= kMax_UI1 ) {
            // use smaller byte representation
            CRef<CByte_graph> byte_graph(new CByte_graph);
            byte_graph->SetAxis(0);
            byte_graph->SetMin(0);
            byte_graph->SetMax(int(max_v));
            sx_Assign(byte_graph->SetValues(), *int_vv);
            graph->SetGraph().SetByte(*byte_graph);
            int_graph = 0;
            int_vv = 0;
        }
        else if ( max_v > kMaxIntValue ) {
            // need bigger double representation
            real_graph->SetAxis(0);
            real_graph->SetMin(0);
            real_graph->SetMax(double(max_v));
        }
        else {
            // int graph
            int_graph->SetAxis(0);
            int_graph->SetMin(0);
            int_graph->SetMax(int(max_v));
        }
        annot->SetData().SetGraph().push_back(graph);
        
    }
    GetDb().Put(curs);

    return annot;
}


END_NAMESPACE(objects);
END_NCBI_NAMESPACE;
