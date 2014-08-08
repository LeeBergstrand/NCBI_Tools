/*  $Id: win_mask_sdust_masker.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
 *   CSDustMasker class definition.
 *
 */

#ifndef C_SDUST_MASKER_H
#define C_SDUST_MASKER_H

#include <string>

#include <corelib/ncbitype.h>

#include <algo/dustmask/symdust.hpp>
#include <algo/winmask/seq_masker.hpp>

BEGIN_NCBI_SCOPE

/**
 **\brief This class encapsulates the dusting functionality of winmask.
 **
 **/
class CSDustMasker
{
public:

    /**\brief Type representing a masked subsequence of a sequence. */
    typedef CSeqMasker::TMaskedInterval TMaskedInterval;

    /**\brief Type representing a list of masked segments. */
    typedef CSeqMasker::TMaskList TMaskList;

    /**
     **\brief Object constructor.
     **
     **\param window dust window
     **\param level dust level
     **\param linker dust linker
     **
     **/
    CSDustMasker( Uint4 window, Uint4 level, Uint4 linker );

    /**
     **\brief Object destructor.
     **
     **/
    ~CSDustMasker();

    /**
     **\brief Function performing the actual dusting.
     **
     **\param data sequence data in IUPACNA format
     **\return pointer to a list of dusted sequences
     **
     **/
    TMaskList * operator()( const objects::CSeqVector & data,
                            const TMaskList & exclude_ranges );

private:

    Uint4 window;   /**<\internal dust window in base pairs */
    Uint4 level;    /**<\internal dust level */
    Uint4 linker;   /**<\internal dust linker length in base pairs */

    CSymDustMasker duster_; /**<\internal symmetric dust masker */
};

END_NCBI_SCOPE

#endif
