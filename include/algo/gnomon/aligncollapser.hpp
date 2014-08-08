#ifndef ALGO_GNOMON___ALIGNCOLLAPSER__HPP
#define ALGO_GNOMON___ALIGNCOLLAPSER__HPP

/*  $Id: aligncollapser.hpp 381780 2012-11-28 19:15:26Z souvorov $
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
 * Authors:  Alexandre Souvorov
 *
 * File Description:
 *
 */

#include <algo/gnomon/gnomon_model.hpp>

BEGIN_SCOPE(ncbi);
BEGIN_SCOPE(gnomon);

struct SAlignIndividual {
    SAlignIndividual() : m_weight(0) {};
    SAlignIndividual(const CAlignModel& align, deque<char>& target_id_pool) : m_range(align.Limits()), m_align_id(align.ID()), m_weight(align.Weight()) {
        m_target_id = target_id_pool.size();
        string acc = align.TargetAccession();
        copy(acc.begin(),acc.end(),back_inserter(target_id_pool));
        target_id_pool.push_back(0);
    };

    TSignedSeqRange m_range;
    Int8 m_align_id;
    float m_weight;
    int m_target_id;   // shift in deque<char> for 0 terminated string; deque is maintained by CAlignCollapser
};

class CAlignCommon {
public:
    CAlignCommon() : m_flags(0) {}
    CAlignCommon(const CGeneModel& align);
    typedef vector<TSignedSeqRange> Tintrons;
    const Tintrons& GetIntrons() const { return m_introns; }
    CAlignModel GetAlignment(const SAlignIndividual& ali, const deque<char>& target_id_pool) const;
    bool isSR() const { return (m_flags&esr); }
    bool isEST() const { return (m_flags&eest); }
    bool isPolyA() const { return (m_flags&epolya); }
    bool isCap() const { return (m_flags&ecap); }
    bool isUnknown() const { return (m_flags&eunknownorientation); }
    bool isPlus() const { return (m_flags&eplus); }
    bool isMinus() const { return (m_flags&eminus); }
    bool operator<(const CAlignCommon& cas) const {
        if(m_flags != cas.m_flags)
            return m_flags < cas.m_flags;
        else if(m_introns.size() != cas.m_introns.size())
            return m_introns.size() < cas.m_introns.size();
        else
            return m_introns < cas.m_introns;
    }    

private:
    enum {
        esr = 1,
        eest = 2,
        epolya = 4,
        ecap = 8,
        eunknownorientation = 16,
        eplus = 32,
        eminus = 64
    };

    Tintrons m_introns;
    int m_flags;
};

class CAlignCollapser {
public:
    CAlignCollapser() : m_count(0) {}
    void AddAlignment(const CAlignModel& align, bool includeincollaps);
    void GetCollapsedAlgnmnets(TAlignModelClusterSet& clsset, int oep, int max_extend);

private:
    void CollapsIdentical();
    typedef map< CAlignCommon,deque<SAlignIndividual> > Tdata;
    Tdata m_aligns;
    int m_count;
    typedef map< CAlignCommon,deque<char> > Tidpool;
    Tidpool m_target_id_pool;
    set<int> m_left_exon_ends, m_right_exon_ends;
};

END_SCOPE(gnomon)
END_SCOPE(ncbi)


#endif  // ALGO_GNOMON___ALIGNCOLLAPSER__HPP
