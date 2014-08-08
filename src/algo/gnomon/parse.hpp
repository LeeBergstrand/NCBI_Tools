#ifndef ALGO_GNOMON___PARSE__HPP
#define ALGO_GNOMON___PARSE__HPP

/*  $Id: parse.hpp 116643 2008-01-07 14:42:04Z chetvern $
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

#include <corelib/ncbistd.hpp>

#include <algo/gnomon/gnomon.hpp>
#include "hmm.hpp"

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(gnomon)

class CParse
{
    public:
        CParse(const CSeqScores& ss,
               const CIntronParameters&     intron_params,
               const CIntergenicParameters& intergenic_params,
               const CExonParameters&       exon_params,
               bool leftanchor, bool rightanchor);
        const CHMM_State* Path() const { return m_path; }
        void PrintInfo() const;
        list<CGeneModel> GetGenes() const;
    //        typedef list<CGene>::iterator TGenIt;
    
    private:
        CParse& operator=(const CParse&);
        const CSeqScores& m_seqscr;
        const CHMM_State* m_path;
        
        vector<CIntergenic> m_igplus, m_igminus;
        vector<CIntron> m_inplus[2][3], m_inminus[2][3];
        vector<CFirstExon> m_feplus[2][3], m_feminus;
        vector<CInternalExon> m_ieplus[2][3], m_ieminus[2][3];
        vector<CLastExon> m_leplus, m_leminus[2][3];
        vector<CSingleExon> m_seplus, m_seminus;
};

void LogicalCheck(const CHMM_State& st, const CSeqScores& ss);


END_SCOPE(gnomon)
END_NCBI_SCOPE

#endif  // ALGO_GNOMON___PARSE__HPP
