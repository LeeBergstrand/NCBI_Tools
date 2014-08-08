/*  $Id: mask_fasta_reader.cpp 148871 2009-01-05 16:51:12Z camacho $
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
 *   CMaskFastaReader class member and method definitions.
 *
 */

#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] = "$Id: mask_fasta_reader.cpp 148871 2009-01-05 16:51:12Z camacho $";
#endif /* SKIP_DOXYGEN_PROCESSING */

#include <ncbi_pch.hpp>
#include <corelib/ncbidbg.hpp>
#include <objects/seq/Bioseq.hpp>

#include <objtools/seqmasks_io/mask_fasta_reader.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);


//-------------------------------------------------------------------------
CRef< CSeq_entry > CMaskFastaReader::GetNextSequence()
{
    while( !fasta_reader_.AtEOF() )
    {
        CRef< CSeq_entry > aSeqEntry( null );
        aSeqEntry = fasta_reader_.ReadSet( 1 );

        if( !input_stream && !input_stream.eof() ) {
            NCBI_THROW( Exception, eBadStream,
                    "error reading input stream" );
        }

        if( aSeqEntry != 0 && aSeqEntry->IsSeq() && 
            aSeqEntry->GetSeq().IsNa() == is_nucleotide_)
            return aSeqEntry;
        else break;
    }

    return CRef< CSeq_entry >( null );
}


END_NCBI_SCOPE
