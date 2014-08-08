/*  $Id: pacc.cpp 170010 2009-09-08 14:24:26Z dicuccio $
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
 * Author:  Victor Sapojnikov
 *
 * File Description:
 *     Usage example for CAccPatternCounter.
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbifile.hpp>

#include <objtools/readers/agp_util.hpp>

USING_NCBI_SCOPE;

int main(int argc, char* argv[])
{
    CAccPatternCounter pc;

    // Add accessions
    string s;
    while( NcbiGetline(cin, s, "\r\n") ) {
        if( s.size() ) {
            pc.AddName(s);
        }
    }

    // Print expanded patterns and counts, most frequent patterns first.
    // Runs of digits are replaced with ranges, or kept as is.
    CAccPatternCounter::TMapCountToString cnt_pat; // multimap<int,string>
    pc.GetSortedPatterns(cnt_pat);

    for(CAccPatternCounter::TMapCountToString::reverse_iterator
        it = cnt_pat.rbegin(); it != cnt_pat.rend(); ++it
    ) {
        // pattern <tab> count
        cout<< it->second << "\t" << it->first  << "\n";
    }
}
