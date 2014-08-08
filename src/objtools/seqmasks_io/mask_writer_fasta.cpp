/*  $Id: mask_writer_fasta.cpp 389772 2013-02-20 20:48:34Z camacho $
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
 * Author:  Aleksandr Morgulis
 *
 * File Description:
 *   CMaskWriterFasta class member and method definitions.
 *
 */

#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] = "$Id: mask_writer_fasta.cpp 389772 2013-02-20 20:48:34Z camacho $";
#endif /* SKIP_DOXYGEN_PROCESSING */

#include <ncbi_pch.hpp>
#include <objects/seq/Bioseq.hpp>
#include <objects/seq/Seq_inst.hpp>
#include <objects/seq/Seq_data.hpp>
#include <objects/seq/seqport_util.hpp>
#include <objects/seq/IUPACna.hpp>
#include <objmgr/seq_vector.hpp>

#include <objtools/seqmasks_io/mask_writer_fasta.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);

//-------------------------------------------------------------------------
void CMaskWriterFasta::Print( objects::CBioseq_Handle& bsh,
                              const TMaskList & mask,
                              bool parsed_id )
{
    PrintId( bsh, parsed_id );
    os << endl;
    CSeqVector data = bsh.GetSeqVector(CBioseq_Handle::eCoding_Iupac);

    /// FIXME: this can be implemented as a call to CFastaOstream, which
    /// understands masking via a seq-loc

    // if( dest->GetIupacna().CanGet() )
    if( true )
    {
        string accumulator;
        TMaskList::const_iterator imask = mask.begin();

        for( TSeqPos i = 0; i < data.size(); ++i )
        {
            char letter = data[i];

            if( imask != mask.end() && i >= imask->first ) {
                if( i <= imask->second ) 
                    letter = tolower((unsigned char) letter);
                else
                {
                    ++imask;

                    if(    imask != mask.end() 
                        && i >= imask->first && i <= imask->second )
                        letter = tolower((unsigned char) letter);
                }
            }

            accumulator.append( 1, letter );

            if( !((i + 1)%60) )
            {
                os << accumulator << "\n";
                accumulator = "";
            }
        }

        if( accumulator.length() ) os << accumulator << "\n";
    }
}


END_NCBI_SCOPE
