/*  $Id: checksum.cpp 387253 2013-01-28 18:23:06Z ivanov $
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
 * Author:  Eugene Vasilchenko
 *
 * File Description:  Checksum (CRC32, Adler32 or MD5) calculation class
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <util/checksum.hpp>


BEGIN_NCBI_SCOPE


#define NCBI_USE_PRECOMPILED_CRC32_TABLES 1

// sx_Start must begin with "/* O" (see ValidChecksumLine() in checksum.hpp)
static const char sx_Start[]     = "/* Original file checksum: ";
static const char sx_End[]       = " */";
static const char sx_LineCount[] = "lines: ";
static const char sx_CharCount[] = "chars: ";

#ifdef NCBI_USE_PRECOMPILED_CRC32_TABLES
inline void s_InitTableCRC32() {}
inline void s_InitTableCRC32ZIP() {}
#else
static Uint4 s_CalcByteCRC32(size_t byte);
static Uint4 s_CalcByteCRC32ZIP(size_t byte);
static void s_FillMultiBitsCRC(Uint4* table, size_t size);
static void s_InitTableCRC32();
static void s_InitTableCRC32ZIP();
#endif //NCBI_USE_PRECOMPILED_CRC32_TABLES
static Uint4 s_UpdateCRC32(Uint4 checksum, const char* str, size_t length);
static Uint4 s_UpdateCRC32ZIP(Uint4 checksum, const char* str, size_t length);
static Uint4 s_UpdateAdler32(Uint4 sum, const char* data, size_t len);
static void s_PrintTable(CNcbiOstream& out, const char* name,
                         const Uint4* table, size_t size);


CChecksum::CChecksum(EMethod method)
    : m_LineCount(0), m_CharCount(0), m_Method(method)
{
    switch ( GetMethod() ) {
    case eCRC32:
        s_InitTableCRC32();
        m_Checksum.m_CRC32 = 0;
        break;
    case eCRC32ZIP:
    case eCRC32INSD:
        s_InitTableCRC32ZIP();
        m_Checksum.m_CRC32 = ~0;
        break;
    case eMD5:
        m_Checksum.m_MD5 = new CMD5;
        break;
    case eAdler32:
        m_Checksum.m_CRC32 = 1;
        break;
    default:
        break;
    }
}


CChecksum::CChecksum(const CChecksum& cks)
    : m_LineCount(cks.m_LineCount), m_CharCount(cks.m_CharCount),
      m_Method(cks.m_Method)
{
    switch ( GetMethod() ) {
    case eCRC32:
    case eCRC32ZIP:
    case eCRC32INSD:
    case eAdler32:
        m_Checksum.m_CRC32 = cks.m_Checksum.m_CRC32;
        break;
    case eMD5:
        m_Checksum.m_MD5 = new CMD5(*cks.m_Checksum.m_MD5);
        break;
    default:
        break;
    }
}


CChecksum::~CChecksum()
{
    x_Free();
}


void CChecksum::x_Free(void)
{
    switch ( GetMethod() ) {
    case eMD5:
        delete m_Checksum.m_MD5;
        m_Checksum.m_MD5 = 0;
        break;
    default:
        break;
    }
}


CChecksum& CChecksum::operator= (const CChecksum& cks)
{
    x_Free();

    m_LineCount = cks.m_LineCount;
    m_CharCount = cks.m_CharCount;
    m_Method    = cks.m_Method;

    switch ( GetMethod() ) {
    case eCRC32:
    case eCRC32ZIP:
    case eCRC32INSD:
    case eAdler32:
        m_Checksum.m_CRC32 = cks.m_Checksum.m_CRC32;
        break;
    case eMD5:        
        m_Checksum.m_MD5 = new CMD5(*cks.m_Checksum.m_MD5);
        break;
    default:
        break;
    }
    return *this;
}


void CChecksum::Reset(EMethod method)
{
    x_Free();

    m_LineCount = 0;
    m_CharCount = 0;
    if (method != eNone) {
        m_Method = method;
    }

    switch ( GetMethod() ) {
    case eCRC32:
        m_Checksum.m_CRC32 = 0;
        break;
    case eCRC32ZIP:
    case eCRC32INSD:
        m_Checksum.m_CRC32 = ~0;
        break;
    case eMD5:
        m_Checksum.m_MD5 = new CMD5;
        break;
    case eAdler32:
        m_Checksum.m_CRC32 = 1;
        break;
    default:
        break;
    }
}


Uint4 CChecksum::GetChecksum(void) const
{
    switch ( GetMethod() ) {
    case eCRC32:
        return m_Checksum.m_CRC32;
    case eCRC32ZIP:
        return ~m_Checksum.m_CRC32;
    case eCRC32INSD:
        return m_Checksum.m_CRC32;
    case eAdler32:
        return m_Checksum.m_CRC32;
    default:
        _ASSERT(0);
        return 0;
    }
}


CNcbiOstream& CChecksum::WriteChecksum(CNcbiOstream& out) const
{
    if (!Valid()   ||  !out.good()) {
        return out;
    }
    out << sx_Start <<
        sx_LineCount << m_LineCount << ", " <<
        sx_CharCount << m_CharCount << ", ";
    WriteChecksumData(out);
    return out << sx_End << '\n';
}


bool CChecksum::ValidChecksumLineLong(const char* line, size_t length) const
{
    if ( !Valid() ) {
        return false;
    }
    CNcbiOstrstream buffer;
    WriteChecksum(buffer);
    size_t bufferLength = (size_t) buffer.pcount() - 1; // do not include '\n'
    if ( bufferLength != length ) {
        return false;
    }
    const char* bufferPtr = buffer.str();
    buffer.freeze(false);
    return memcmp(line, bufferPtr, length) == 0;
}


CNcbiOstream& CChecksum::WriteChecksumData(CNcbiOstream& out) const
{
    switch ( GetMethod() ) {
    case eCRC32:
    case eCRC32ZIP:
    case eCRC32INSD:
        return out << "CRC32: " << hex << setprecision(8)
                   << GetChecksum();
    case eMD5:
        return out << "MD5: " << m_Checksum.m_MD5->GetHexSum();
    case eAdler32:
        return out << "Adler32: " << hex << setprecision(8)
                   << GetChecksum();
    default:
        return out << "none";
    }
}


void CChecksum::x_Update(const char* str, size_t count)
{
    switch ( GetMethod() ) {
    case eCRC32:
        m_Checksum.m_CRC32 = s_UpdateCRC32(m_Checksum.m_CRC32, str, count);
        break;
    case eCRC32ZIP:
    case eCRC32INSD:
        m_Checksum.m_CRC32 = s_UpdateCRC32ZIP(m_Checksum.m_CRC32, str, count);
        break;
    case eAdler32:
        m_Checksum.m_CRC32 = s_UpdateAdler32(m_Checksum.m_CRC32, str, count);
        break;
    case eMD5:
        m_Checksum.m_MD5->Update(str, count);
        break;
    default:
        break;
    }
}


void CChecksum::NextLine(void)
{
    char eol = '\n';
    x_Update(&eol, 1);
    ++m_LineCount;
}


CChecksum ComputeFileChecksum(const string& path, CChecksum::EMethod method)
{
    CNcbiIfstream input(path.c_str(), IOS_BASE::in | IOS_BASE::binary);
    CChecksum cks(method);
    return ComputeFileChecksum(path, cks);
}


CChecksum& ComputeFileChecksum(const string& path, CChecksum& checksum)
{
    CNcbiIfstream input(path.c_str(), IOS_BASE::in | IOS_BASE::binary);
    if ( !input.is_open() ) {
        return checksum;
    }
    while ( !input.eof() ) {
        char buf[1024*4];
        input.read(buf, sizeof(buf));
        size_t count = (size_t)input.gcount();
        if ( count ) {
            checksum.AddChars(buf, count);
        }
    }
    input.close();
    return checksum;
}


void CChecksum::InitTables(void)
{
    s_InitTableCRC32();
    s_InitTableCRC32ZIP();
}


static void s_PrintTable(CNcbiOstream& out, const char* name,
                         const Uint4* table, size_t size)
{
    const size_t kLineSize = 4;
    out << "static Uint4 " << name << "[" << size << "] = {";
    for ( size_t i = 0; i < size; ++i ) {
        if ( i != 0 ) {
            out << ',';
        }
        if ( i % kLineSize == 0 ) {
            out << "\n    ";
        }
        else {
            out << ' ';
        }
        out << "0x" << hex << setw(8) << setfill('0') << table[i];
    }
    out << dec << "\n};\n" << endl;
}


static const size_t kCRC32Size = 256;

#ifdef NCBI_USE_PRECOMPILED_CRC32_TABLES

static Uint4 s_CRC32Table[kCRC32Size] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static Uint4 s_CRC32ZIPTable[kCRC32Size] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

#else

static Uint4 s_CRC32Table[kCRC32Size];
static Uint4 s_CRC32ZIPTable[kCRC32Size];

/////////////////////////////////////////////////////////////////////////////
//  Implementation of CRC32 algorithm.
/////////////////////////////////////////////////////////////////////////////
//
//  This code assumes that an unsigned is at least 32 bits wide and
//  that the predefined type char occupies one 8-bit byte of storage.

//  The polynomial used is
//  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x^1+x^0
#define CRC32_POLYNOMIAL    0x04c11db7
#define CRC32ZIP_POLYNOMIAL 0xedb88320

// CRC32 is linear meaning that for any texts t1 & t2:
//   CRC32[t1 XOR t2] = CRC32[t1] XOR CRC32[t2].
// This allows to speed up calculation of CRC32 tables by first
// calculating CRC32 for bytes with only one bit set,
// and then xoring all CRC32 of lowest bit and CRC32 of remaining bits
// to get CRC32 of whole number.
// First part is done by calling s_CalcByteCRC32 or s_CalcByteCRC32ZIP for
// each bit.
// Second pass is universal for any CRC32 and is performed by function
// s_FillMultiBitsCRC().


Uint4 s_CalcByteCRC32(size_t byte)
{
    Uint4 byteCRC = byte << 24;
    for ( int j = 0;  j < 8;  ++j ) {
        if ( byteCRC & 0x80000000L )
            byteCRC = (byteCRC << 1) ^ CRC32_POLYNOMIAL;
        else
            byteCRC = (byteCRC << 1);
    }
    return byteCRC;
}


Uint4 s_CalcByteCRC32ZIP(size_t byte)
{
    Uint4 byteCRC = byte;
    for ( int j = 0;  j < 8;  ++j ) {
        if ( byteCRC & 1 )
            byteCRC = (byteCRC >> 1) ^ CRC32ZIP_POLYNOMIAL;
        else
            byteCRC = (byteCRC >> 1);
    }
    return byteCRC;
}


void s_FillMultiBitsCRC(Uint4* table, size_t size)
{
    // Preconditions:
    //  Entries at one-bit indexes (1<<k), are calculated.
    for ( size_t i = 0;  i < size;  ++i ) { // order is significant
        // Split bits of i into two parts:
        //  lobit contains lowest bit set, or zero if no bits are set,
        //  hibits contains all other bits.
        size_t hibits = i & (i-1);
        size_t lobit = i & ~(i-1);
        // Because of:
        //  1. i = lobit ^ hibits
        //  2. lobit <= i
        //  3. hibits <= i
        // we can calculate entry at i by xoring entries at lobit and hibits
        // There are 3 possible cases:
        //  A. i = 0
        //    In this case lobit = 0 and hibits = 0.
        //    As a result table[0] will become 0, which is correct for CRC.
        //  B. i = 1<<k
        //    In this case lobit = i, and hibits = 0.
        //    table[i] will become table[i] ^ table[0].
        //    Because table[0] is 0 (see case A above),
        //    table[i] will not change and will preserve precalculated value
        //    (see Preconditions above).
        //  C. all other i
        //    In this case lobit < i, and hibits < i
        //    It means the entries at lobit and hibits are calculated already
        //    because of the order of iteration by i.
        table[i] = table[lobit] ^ table[hibits];
    }
}


void s_InitTableCRC32(void)
{
    // check the last element to make sure we minimize chances of races 
    // in MT programs.
    if ( s_CRC32Table[kCRC32Size-1] == 0 ) {
        // Initialize CRC32 for bytes with only one bit set
        for ( size_t i = 1;  i < kCRC32Size;  i <<= 1 ) {
            s_CRC32Table[i] = s_CalcByteCRC32(i);
        }
        // Fill the rest of table
        s_FillMultiBitsCRC(s_CRC32Table, kCRC32Size);
    }
}


void s_InitTableCRC32ZIP(void)
{
    // check the last element to make sure we minimize chances of races 
    // in MT programs.
    if ( s_CRC32ZIPTable[kCRC32Size-1] == 0 ) {
        // Initialize CRC32 for bytes with only one bit set
        for ( size_t i = 1;  i < kCRC32Size;  i <<= 1 ) {
            s_CRC32ZIPTable[i] = s_CalcByteCRC32ZIP(i);
        }
        // Fill the rest of table
        s_FillMultiBitsCRC(s_CRC32ZIPTable, kCRC32Size);
    }
}

#endif //NCBI_USE_PRECOMPILED_CRC32_TABLES


Uint4 s_UpdateCRC32(Uint4 checksum, const char *str, size_t count)
{
    for ( size_t j = 0;  j < count;  ++j ) {
        size_t tableIndex = ((checksum >> 24) ^ *str++) & 0xff;
        checksum = (checksum << 8) ^ s_CRC32Table[tableIndex];
    }
    return checksum;
}


Uint4 s_UpdateCRC32ZIP(Uint4 checksum, const char *str, size_t count)
{
    for ( size_t j = 0;  j < count;  ++j ) {
        size_t tableIndex = (checksum ^ *str++) & 0xff;
        checksum = (checksum >> 8) ^ s_CRC32ZIPTable[tableIndex];
    }
    return checksum;
}


Uint4 s_UpdateAdler32(Uint4 sum, const char* data, size_t len)
{
    const Uint4 MOD_ADLER = 65521;

#define ADJUST_ADLER(a) a = (a & 0xffff) + (a >> 16) * (0x10000-MOD_ADLER)
#define FINALIZE_ADLER(a) if (a >= MOD_ADLER) a -= MOD_ADLER

    Uint4 a = sum & 0xffff, b = sum >> 16;
    
    const size_t kMaxLen = 5548u;
    while (len) {
        if ( len >= kMaxLen ) {
            len -= kMaxLen;
            for ( size_t i = 0; i < kMaxLen/4; ++i ) {
                b += a += Uint1(data[0]);
                b += a += Uint1(data[1]);
                b += a += Uint1(data[2]);
                b += a += Uint1(data[3]);
                data += 4;
            }
        }
        else {
            for ( size_t i = len >> 2; i; --i ) {
                b += a += Uint1(data[0]);
                b += a += Uint1(data[1]);
                b += a += Uint1(data[2]);
                b += a += Uint1(data[3]);
                data += 4;
            }
            for ( len &= 3; len; --len ) {
                b += a += Uint1(data[0]);
                data += 1;
            }
        }
        ADJUST_ADLER(a);
        ADJUST_ADLER(b);
    }
    // It can be shown that a <= 0x1013a here, so a single subtract will do.
    FINALIZE_ADLER(a);
    // It can be shown that b can reach 0xffef1 here.
    ADJUST_ADLER(b);
    FINALIZE_ADLER(b);
    return (b << 16) | a;
}


void CChecksum::PrintTables(CNcbiOstream& out)
{
    InitTables();
    s_PrintTable(out, "s_CRC32Table", s_CRC32Table, kCRC32Size);
    s_PrintTable(out, "s_CRC32ZIPTable", s_CRC32ZIPTable, kCRC32Size);
}


END_NCBI_SCOPE
