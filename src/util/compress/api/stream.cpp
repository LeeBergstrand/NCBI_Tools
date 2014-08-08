/*  $Id: stream.cpp 256120 2011-03-02 15:22:32Z lavr $
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
 * Authors:  Vladimir Ivanov
 *
 * File Description:  CCompression based C++ I/O streams
 *
 */

#include <ncbi_pch.hpp>
#include "streambuf.hpp"
#include <memory>


BEGIN_NCBI_SCOPE



//////////////////////////////////////////////////////////////////////////////
//
// CCompressionStreamProcessor
//

CCompressionStreamProcessor::CCompressionStreamProcessor(
    CCompressionProcessor*  processor,
    EDeleteProcessor        need_delete,
    streamsize              in_bufsize,
    streamsize              out_bufsize)

    : m_Processor(processor), 
      m_InBufSize(in_bufsize <= 1 ? kCompressionDefaultBufSize : in_bufsize),
      m_OutBufSize(out_bufsize <= 1 ? kCompressionDefaultBufSize :out_bufsize),
      m_NeedDelete(need_delete), m_State(eDone)
{
    Init();
    return;
}


CCompressionStreamProcessor::~CCompressionStreamProcessor(void)
{
    if ( m_Processor  &&  m_NeedDelete == eDelete ) {
        delete m_Processor;
    }
    m_Processor = 0;
}


void CCompressionStreamProcessor::Init(void)
{
    CCompressionProcessor::EStatus init_status = CCompressionProcessor::eStatus_Success;
    if ( m_Processor ) {
        if ( m_State == eDone ) {
            init_status = m_Processor->Init();
        } else if (m_InBuf != 0) { // reinitializing
            m_Processor->End();    // avoid leaking memory
            init_status = m_Processor->Init();
        }
    }
    m_InBuf         = 0;
    m_OutBuf        = 0;
    m_Begin         = 0;
    m_End           = 0;
    m_LastStatus    = init_status;
    m_State         = eInit;
}


//////////////////////////////////////////////////////////////////////////////
//
// CCompressionStream
//

CCompressionStream::CCompressionStream(CNcbiIos&                    stream,
                                       CCompressionStreamProcessor* read_sp,
                                       CCompressionStreamProcessor* write_sp,
                                       TOwnership                   ownership)
    : CNcbiIos(0), m_Stream(0)
{
    Create(stream, read_sp, write_sp, ownership);
}


CCompressionStream::CCompressionStream(void)
    : CNcbiIos(0), m_Stream(0), m_StreamBuf(0),
      m_Reader(0), m_Writer(0), m_Ownership(0)
{
    // see Create()
}

void CCompressionStream::Create(CNcbiIos&                    stream,
                                CCompressionStreamProcessor* read_sp,
                                CCompressionStreamProcessor* write_sp,
                                TOwnership                   ownership)
{
    if ( m_Stream ) {
        // Already initialized, do nothing
        return;
    }
    // Initialize members
    m_Stream    = &stream; 
    m_Reader    = read_sp;
    m_Writer    = write_sp;
    m_Ownership = ownership;
    
    // Create a new stream buffer
    auto_ptr<CCompressionStreambuf> sb(
        new CCompressionStreambuf(&stream, read_sp, write_sp));
    init(sb.get());
    m_StreamBuf = sb.release();
    if ( !m_StreamBuf->IsOkay() ) {
        setstate(badbit | eofbit);
    }
}


CCompressionStream::~CCompressionStream(void)
{
    delete m_StreamBuf;

    // Delete owned objects
    if ( m_Stream   &&   m_Ownership & fOwnStream ) {
#if defined(NCBI_COMPILER_GCC)  &&  NCBI_COMPILER_VERSION < 300
        // On GCC 2.9x ios::~ios() is protected
#else
        delete m_Stream;
        m_Stream = 0;
#endif
    }
    if ( m_Reader  &&  m_Ownership & fOwnReader ) {
        if ( m_Reader == m_Writer  &&  m_Ownership & fOwnWriter ) {
            m_Writer = 0;
        }
        delete m_Reader;
        m_Reader = 0;
    }
    if ( m_Writer  &&  m_Ownership & fOwnWriter ) {
        delete m_Writer;
        m_Writer = 0;
    }
}


void CCompressionStream::Finalize(CCompressionStream::EDirection dir) 
{
    if ( m_StreamBuf ) {
        m_StreamBuf->Finalize(dir);
    }
}


CCompressionProcessor::EStatus 
CCompressionStream::x_GetStatus(CCompressionStream::EDirection dir)
{
    CCompressionStreamProcessor* sp = (dir == eRead) ? m_Reader : m_Writer;
    if ( !sp ) {
        return CCompressionProcessor::eStatus_Unknown;
    }
    return sp->m_LastStatus;
}


bool CCompressionStream::x_GetError(CCompressionStream::EDirection dir,
                                    int& status, string& description)
{
    CCompressionStreamProcessor* sp = (dir == eRead) ? m_Reader : m_Writer;
    status = 0;
    description.clear();
    if (!sp  ||  !sp->m_Processor) {
        return false;
    }
    // We pass CCompression-CCompressionStreamProcessor derived object
    // to compression stream as 'stream processor', at least internally,
    // so try to get error code from CCompression.
    CCompression* cmp = dynamic_cast<CCompression*>(sp->m_Processor);
    if (!cmp) {
        return false;
    }
    status = cmp->GetErrorCode();
    description = cmp->GetErrorDescription();
    return true;
}


unsigned long CCompressionStream::x_GetProcessedSize(
                                  CCompressionStream::EDirection dir)
{
    CCompressionStreamProcessor* sp = (dir == eRead) ? m_Reader : m_Writer;
    if (!sp  ||  !sp->m_Processor) {
        return 0;
    }
    return sp->m_Processor->GetProcessedSize();
}


unsigned long CCompressionStream::x_GetOutputSize(
                                  CCompressionStream::EDirection dir)
{
    CCompressionStreamProcessor* sp = (dir == eRead) ? m_Reader : m_Writer;
    if (!sp  ||  !sp->m_Processor) {
        return 0;
    }
    return sp->m_Processor->GetOutputSize();
}


END_NCBI_SCOPE
