/*  $Id: snp_bitfield_1_2.cpp 381763 2012-11-28 17:08:30Z rudnev $
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
 * Authors:  Melvin Quintos
 *
 * File Description:
 *  Provides implementation of CSnpBitfield_1_2 class. See snp_bitfield_1_2.hpp
 *  for class usage.
 *
 */

#include <ncbi_pch.hpp>

#include "snp_bitfield_1_2.hpp"

#include <stdio.h>

BEGIN_NCBI_SCOPE

///////////////////////////////////////////////////////////////////////////////
// Static globals, typedefs, etc
///////////////////////////////////////////////////////////////////////////////

//
// Create table lookups for the first 40 property fields
//

// table lookup for the 40 property fields and their byte offset
static const int g_byteOffset[] = {
    0,0,0,0,0,0,0,0,    // F1 Link (first 8 properties)     on byte 0
    1,1,                // F1 Link (9th and 10th properties) on byte 1
    3,3,3,              // F3 Map (next 3 properties)       on byte 3
    4,4,4,4,            // F4 Freq (next 4 properties)      on byte 4
    5,5,5,              // F5 GTY  (next 3 properties)      on byte 5
    6,6,6,6,6,6,        // F6 Hapmap (next 6 properties)    on byte 6
    7,7,7,7,7,7,7,7,    // F7 Phenotype (next 8 properties) on byte 7
    9,9,9,9,9,9         // F9 Quality (next 6 properties)   on byte 9
};

// table lookup for the 40 property fields and their bit offset in the byte
static const int g_bitOffset[] = {
    CSnpBitfield::IEncoding::fBit0, CSnpBitfield::IEncoding::fBit1, CSnpBitfield::IEncoding::fBit2, CSnpBitfield::IEncoding::fBit3, CSnpBitfield::IEncoding::fBit4, CSnpBitfield::IEncoding::fBit5, CSnpBitfield::IEncoding::fBit6, CSnpBitfield::IEncoding::fBit7,
    CSnpBitfield::IEncoding::fBit0, CSnpBitfield::IEncoding::fBit1,
    CSnpBitfield::IEncoding::fBit2, CSnpBitfield::IEncoding::fBit3, CSnpBitfield::IEncoding::fBit4,
    CSnpBitfield::IEncoding::fBit0, CSnpBitfield::IEncoding::fBit1, CSnpBitfield::IEncoding::fBit2, CSnpBitfield::IEncoding::fBit3,
    CSnpBitfield::IEncoding::fBit0, CSnpBitfield::IEncoding::fBit1, CSnpBitfield::IEncoding::fBit2,
    CSnpBitfield::IEncoding::fBit0, CSnpBitfield::IEncoding::fBit1, CSnpBitfield::IEncoding::fBit2, CSnpBitfield::IEncoding::fBit3, CSnpBitfield::IEncoding::fBit4, CSnpBitfield::IEncoding::fBit5,
    CSnpBitfield::IEncoding::fBit0, CSnpBitfield::IEncoding::fBit1, CSnpBitfield::IEncoding::fBit2, CSnpBitfield::IEncoding::fBit3, CSnpBitfield::IEncoding::fBit4, CSnpBitfield::IEncoding::fBit5, CSnpBitfield::IEncoding::fBit6, CSnpBitfield::IEncoding::fBit7,
    CSnpBitfield::IEncoding::fBit0, CSnpBitfield::IEncoding::fBit1, CSnpBitfield::IEncoding::fBit2, CSnpBitfield::IEncoding::fBit3, CSnpBitfield::IEncoding::fBit4, CSnpBitfield::IEncoding::fBit5
};

///////////////////////////////////////////////////////////////////////////////
// Public Methods
///////////////////////////////////////////////////////////////////////////////
CSnpBitfield1_2::CSnpBitfield1_2( const std::vector<char> &rhs )
{
    // TODO: use NCBI assertions?
    _ASSERT(rhs.size() == 10);

    std::vector<char>::const_iterator i_ci = rhs.begin();

    for(int i=0 ; i_ci != rhs.end(); ++i_ci, ++i) {
        m_listBytes[i] = *i_ci;
    }

    x_CreateString();
}

int CSnpBitfield1_2::GetVersion() const
{
    return 1;
}

CSnpBitfield::EFunctionClass CSnpBitfield1_2::GetFunctionClass() const
{
    // located on 3rd byte
    // Check higher order bits first
    // Multiple bits may be set, but only the highest bit takes effect
    if(         (m_listBytes[2] & 0x80) != 0)     return CSnpBitfield::eFrameshift;
    else if (   (m_listBytes[2] & 0x40) != 0)     return CSnpBitfield::eMissense;
    else if (   (m_listBytes[2] & 0x20) != 0)     return CSnpBitfield::eNonsense;
    else if (   (m_listBytes[2] & 0x10) != 0)     return CSnpBitfield::eSynonymous;
    else if (   (m_listBytes[2] & 0x08) != 0)     return CSnpBitfield::eUTR;
    else if (   (m_listBytes[2] & 0x04) != 0)     return CSnpBitfield::eAcceptor;
    else if (   (m_listBytes[2] & 0x02) != 0)     return CSnpBitfield::eDonor;
    else if (   (m_listBytes[2] & 0x01) != 0)     return CSnpBitfield::eIntron;
    else                                          return CSnpBitfield::eUnknownFxn;
}

CSnpBitfield::EVariationClass  CSnpBitfield1_2::GetVariationClass() const
{
    CSnpBitfield::EVariationClass c;
    unsigned char byte;

    byte = m_listBytes[8];
    if (byte <= CSnpBitfield::eMultiBase) // eMultiBase is last implemented variation
        c = (CSnpBitfield::EVariationClass) byte;
    else
        c = CSnpBitfield::eUnknownVariation;

    return c;
}

int  CSnpBitfield1_2::GetWeight() const
{
    const int mask = 0x03;
    return (m_listBytes[3] & mask);
}

bool CSnpBitfield1_2::IsTrue(CSnpBitfield::EProperty prop) const
{
    bool ret = false;

    // Return false if property queried is
    // newer than last property implemented at 1.2 release
    if(prop > CSnpBitfield::ePropertyV1Last)
        return false;

    int byteOffset  = g_byteOffset[prop];
    int bitMask     = g_bitOffset[prop];

    ret = (m_listBytes[byteOffset] & bitMask) != 0;

    return ret;
}

bool CSnpBitfield1_2::IsTrue( CSnpBitfield::EFunctionClass prop ) const
{
    return (prop == GetFunctionClass());
}

const char * CSnpBitfield1_2::GetString() const
{
    return m_strBits.c_str();
}

void CSnpBitfield1_2::GetBytes(vector<char>& bytes) const
{
    bytes.clear();
    bytes.reserve(sizeof(m_listBytes));
    for(size_t i=0; i<sizeof(m_listBytes); ++i) {
        bytes.push_back(m_listBytes[i]);
    }
}

void CSnpBitfield1_2::x_CreateString()
{
    m_strBits.erase();

    char buff[5];
    for(int i=0; i < 10; i++) {
        unsigned char x = (unsigned char)m_listBytes[i];
        sprintf(buff, "%02hX", (unsigned int)x);
        m_strBits += buff;
        if(i+1 != 10)
            m_strBits += "-";
    }
}

CSnpBitfield::IEncoding * CSnpBitfield1_2::Clone()
{
    CSnpBitfield1_2 * obj = new CSnpBitfield1_2();

    memcpy(obj->m_listBytes, m_listBytes, sizeof(m_listBytes));
    obj->m_strBits = m_strBits;

    return obj;
}

END_NCBI_SCOPE
