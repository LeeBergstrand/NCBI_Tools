/*  $Id: masked_range_set.hpp 163387 2009-06-15 18:32:16Z camacho $
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
* Author:  Kevin Bealer
*
*/

#ifndef _MASKED_RANGE_SET_HPP_
#define _MASKED_RANGE_SET_HPP_

#include <objtools/blast/seqdb_writer/build_db.hpp>

USING_NCBI_SCOPE;
USING_SCOPE(objects);

/** @file masked_range_set.hpp
  FIXME
*/

// First version will be simple -- it will use universal Seq-loc
// logic.  If performance is a problem then it might be best to have
// two containers, one with a Seq-loc key and one with an integer key.

class CMaskedRangeSet : public IMaskDataSource {
public:
    void Insert(int              algo_id,
                const CSeq_id  & id,
                const CSeq_loc & v)
    {
        x_CombineLocs(x_Set(algo_id, CSeq_id_Handle::GetHandle(id)), v);
    }
    
    virtual CMaskedRangesVector &
    GetRanges(const list< CRef<CSeq_id> > & idlist);
    
private:
    void x_FindAndCombine(CConstRef<CSeq_loc> & L1, int algo_id,
                          CSeq_id_Handle& id);
    
    static void x_CombineLocs(CConstRef<CSeq_loc> & L1, const CSeq_loc & L2);
    
    CConstRef<CSeq_loc> & x_Set(int algo_id, CSeq_id_Handle id);
    
    typedef map< CSeq_id_Handle, CConstRef<CSeq_loc> > TAlgoMap;
    
    CMaskedRangesVector m_Ranges;
    vector< TAlgoMap > m_Values;
};

#endif /* _MASKED_RANGE_SET_HPP_ */
