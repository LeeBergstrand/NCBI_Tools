/*  $Id: transmissionrw.cpp 208686 2010-10-20 12:11:04Z kazimird $
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
 * Author: Anatoliy Kuznetsov
 *
 * File Description:  Transmission control.
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbidbg.hpp>
#include <corelib/ncbi_bswap.hpp>
#include <corelib/ncbistr.hpp>
#include <util/util_exception.hpp>
#include <util/transmissionrw.hpp>

BEGIN_NCBI_SCOPE

static const Uint4 sStartWord = 0x01020304;
static const Uint4 sEndPacket = 0xFFFFFFFF;

CTransmissionWriter::CTransmissionWriter(IWriter* wrt, 
                                         EOwnership own_writer,
                                         ESendEofPacket send_eof)
    : m_Wrt(wrt),
      m_OwnWrt(own_writer),
      m_SendEof(send_eof)
{
    _ASSERT(wrt);

    size_t written;
    ERW_Result res = m_Wrt->Write(&sStartWord, sizeof(sStartWord), &written);
    if (res != eRW_Success || written != sizeof(sStartWord)) {
        NCBI_THROW(CIOException, eWrite,  "Cannot write the byte order");
    }
}


namespace {
class CIOBytesCountGuard 
{
public:
    CIOBytesCountGuard(size_t* ret, const size_t& count)
        : m_Ret(ret), m_Count(count) 
    {}

    ~CIOBytesCountGuard() { if (m_Ret) *m_Ret = m_Count; }
private:
    size_t* m_Ret;
    const size_t& m_Count;
};
} // namespace

ERW_Result CTransmissionWriter::Write(const void* buf,
                                      size_t      count,
                                      size_t*     bytes_written)
{
    size_t wrt_count = 0;
    CIOBytesCountGuard guard(bytes_written, wrt_count);

    if (count < sEndPacket)
        return x_WritePacket(buf, count, wrt_count);

    // The buffer cannot fit in one packet -- split it.
    const size_t split_packet_size = (size_t) 0x80008000LU;

    const char* ptr = (const char*) buf;
    size_t remaining = count;

    do {
        size_t written;
        ERW_Result res = x_WritePacket(ptr, remaining < split_packet_size ?
            remaining : split_packet_size, written);
        if (res != eRW_Success)
            return res;
        ptr += written;
        wrt_count += written;
        remaining -= written;
    } while (remaining > 0);

    return eRW_Success;
}

ERW_Result CTransmissionWriter::x_WritePacket(const void* buf,
                                              size_t      count,
                                              size_t&     bytes_written)
{
    bytes_written = 0;
    Uint4 cnt = (Uint4) count;
    size_t written = 0;
    ERW_Result res = m_Wrt->Write(&cnt, sizeof(cnt), &written);
    if (res != eRW_Success) 
        return res;
    if (written != sizeof(cnt))
        return eRW_Error;
    for (const char* ptr = (char*)buf; count > 0; ptr += written) {
        res = m_Wrt->Write(ptr, count, &written);
        if (res != eRW_Success) 
            return res;
        count -= written;
        bytes_written += written;
    }
    return res;
}

ERW_Result CTransmissionWriter::Flush(void)
{
    return m_Wrt->Flush();
}

ERW_Result CTransmissionWriter::Close(void)
{
    if (m_SendEof != eSendEofPacket)
        return eRW_Success;

    m_SendEof = eDontSendEofPacket;

    return m_Wrt->Write(&sEndPacket, sizeof(sEndPacket));
}

CTransmissionWriter::~CTransmissionWriter()
{
    Close();

    if (m_OwnWrt)
        delete m_Wrt;
}

CTransmissionReader::CTransmissionReader(IReader* rdr, EOwnership own_reader)
    : m_Rdr(rdr),
      m_OwnRdr(own_reader),
      m_PacketBytesToRead(0),
      m_ByteSwap(false),
      m_StartRead(false)
{
    return;
}


CTransmissionReader::~CTransmissionReader()
{
    if (m_OwnRdr) {
        delete m_Rdr;
    }
}


ERW_Result CTransmissionReader::Read(void*    buf,
                                     size_t   count,
                                     size_t*  bytes_read)
{
    ERW_Result res;
    size_t read_count = 0;
    CIOBytesCountGuard guard(bytes_read, read_count);

    if (!m_StartRead) {
        res = x_ReadStart();
        if (res != eRW_Success) {
            return res;
        }
    }

    // read packet header
    while (m_PacketBytesToRead == 0) {
        Uint4 cnt;
        res = x_ReadRepeated(&cnt, sizeof(cnt));
        if (res != eRW_Success) { 
            return res;
        }
        if (m_ByteSwap) {
            m_PacketBytesToRead = CByteSwap::GetInt4((unsigned char*)&cnt);
        } else {
            m_PacketBytesToRead = cnt;
        }
    }

    if (m_PacketBytesToRead == sEndPacket) 
        return  eRW_Eof;

    size_t to_read = min(count, m_PacketBytesToRead);

    res = m_Rdr->Read(buf, to_read, &read_count);
    m_PacketBytesToRead -= read_count;

    return res;
}


ERW_Result CTransmissionReader::PendingCount(size_t* count)
{
    return m_Rdr->PendingCount(count);
}


ERW_Result CTransmissionReader::x_ReadStart()
{
    _ASSERT(!m_StartRead);

    m_StartRead = true;

    ERW_Result res;
    unsigned start_word_coming;

    res = x_ReadRepeated(&start_word_coming, sizeof(start_word_coming));
    if (res != eRW_Success) {
        return res;
    }
    m_ByteSwap = (start_word_coming != sStartWord);

    //    _ASSERT(start_word_coming == 0x01020304 || 
    //            start_word_coming == 0x04030201);

    if (start_word_coming != sStartWord &&
        start_word_coming != 0x04030201)
        NCBI_THROW(CUtilException, eWrongData,
                   "Cannot determine the byte order. Got: " 
                   + NStr::UIntToString(start_word_coming, 0, 16));

    return res;
}


ERW_Result CTransmissionReader::x_ReadRepeated(void* buf, size_t count)
{
    ERW_Result res = eRW_Success;
    char* ptr = (char*)buf;
    size_t read;
    
    for( ; count > 0; count -= read, ptr += read) {
        res = m_Rdr->Read(ptr, count, &read);
        if (res != eRW_Success) {
            return res;
       }
    }
    return res;
}


END_NCBI_SCOPE
