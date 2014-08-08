/*  $Id: seq_masker_ostat.cpp 103491 2007-05-04 17:18:18Z kazimird $
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
 *   Implementation of CSeqMaskerUStat class.
 *
 */

#include <ncbi_pch.hpp>

#include <algo/winmask/seq_masker_ostat.hpp>

BEGIN_NCBI_SCOPE

//------------------------------------------------------------------------------
const char * CSeqMaskerOstat::CSeqMaskerOstatException::GetErrCodeString() const
{
    switch( GetErrCode() )
    {
        case eBadState:     return "bad state";
        default:            return CException::GetErrCodeString();
    }
}
            
//------------------------------------------------------------------------------
void CSeqMaskerOstat::setUnitSize( Uint1 us )
{
    if( state != start )
    {
        CNcbiOstrstream ostr;
        ostr << "can not set unit size in state " << state;
        string s = CNcbiOstrstreamToString(ostr);
        NCBI_THROW( CSeqMaskerOstatException, eBadState, s );
    }

    doSetUnitSize( us );
    state = ulen;
}

//------------------------------------------------------------------------------
void CSeqMaskerOstat::setUnitCount( Uint4 unit, Uint4 count )
{
    if( state != ulen && state != udata )
    {
        CNcbiOstrstream ostr;
        ostr << "can not set unit count data in state " << state;
        string s = CNcbiOstrstreamToString(ostr);
        NCBI_THROW( CSeqMaskerOstatException, eBadState, s );
    }

    doSetUnitCount( unit, count );
    state = udata;
}

//------------------------------------------------------------------------------
void CSeqMaskerOstat::setParam( const string & name, Uint4 value )
{
    if( state != udata && state != thres )
    {
        CNcbiOstrstream ostr;
        ostr << "can not set masking parameters in state " << state;
        string s = CNcbiOstrstreamToString(ostr);
        NCBI_THROW( CSeqMaskerOstatException, eBadState, s );
    }

    doSetParam( name, value );
    state = thres;
}

//------------------------------------------------------------------------------
void CSeqMaskerOstat::finalize()
{
    if( state != udata && state != thres )
    {
        CNcbiOstrstream ostr;
        ostr << "can not finalize data structure in state " << state;
        string s = CNcbiOstrstreamToString(ostr);
        NCBI_THROW( CSeqMaskerOstatException, eBadState, s );
    }

    state = final;
    doFinalize();
}

END_NCBI_SCOPE
