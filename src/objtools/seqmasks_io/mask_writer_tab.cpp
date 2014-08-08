/*  $Id: mask_writer_tab.cpp 389772 2013-02-20 20:48:34Z camacho $
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
 * Author:  Christiam Camacho
 *
 * File Description:
 *   CMaskWriterTabular class member and method definitions.
 *
 */

#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] = "$Id: mask_writer_tab.cpp 389772 2013-02-20 20:48:34Z camacho $";
#endif /* SKIP_DOXYGEN_PROCESSING */

#include <ncbi_pch.hpp>
#include <objtools/seqmasks_io/mask_writer_tab.hpp>
#include <objects/seqloc/Seq_loc.hpp>


BEGIN_NCBI_SCOPE

//-------------------------------------------------------------------------
void CMaskWriterTabular::Print( objects::CBioseq_Handle& bsh,
                               const TMaskList & mask,
                               bool parsed_id )
{
    const string id = IdToString(bsh, parsed_id);
    ITERATE(TMaskList, i, mask) {
        os << id << "\t" << i->first << "\t" << i->second << "\n";
    }
}


END_NCBI_SCOPE
