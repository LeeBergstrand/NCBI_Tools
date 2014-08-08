/*  $Id: taxid_set.hpp 208050 2010-10-13 15:48:11Z maning $
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

/** @file taxid_set.hpp
 * Class which defines sequence id to taxid mapping.
 */

#ifndef OBJTOOLS_BLAST_SEQDB_WRITER___TAXID_SET__HPP
#define OBJTOOLS_BLAST_SEQDB_WRITER___TAXID_SET__HPP

#include <corelib/ncbistd.hpp>

// Blast databases
#include <objects/blastdb/Blast_def_line.hpp>
#include <objects/blastdb/Blast_def_line_set.hpp>

BEGIN_NCBI_SCOPE

class NCBI_XOBJWRITE_EXPORT CTaxIdSet : public CObject {
public:
    static const int kTaxIdNotSet = 0;

    CTaxIdSet(int global_taxid = kTaxIdNotSet)
        : m_GlobalTaxId(global_taxid),
          m_Matched(true) {}
    
    void SetMappingFromFile(CNcbiIstream & f);
    
    /// Check that each defline has the specified taxid; if not,
    /// replace the defline and set the taxid.
    /// @param deflines Deflines to fix taxIDs [in|out]
    void FixTaxId(CRef<objects::CBlast_def_line_set> deflines);

    bool HasEverFixedId() const { return m_Matched; };
    
private:
    int                m_GlobalTaxId;
    map< string, int > m_TaxIdMap;
    bool               m_Matched;

    /// Selects the most suitable tax id for the input passed in, checking the
    /// global taxid first, then the mapping provided by an input file, and
    /// finally what's found in the defline argument
    int x_SelectBestTaxid(const objects::CBlast_def_line & defline);
    
};

END_NCBI_SCOPE

#endif // OBJTOOLS_BLAST_SEQDB_WRITER___TAXID_SET__HPP

