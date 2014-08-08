/*  $Id: reader_id2.cpp 207239 2010-10-04 16:11:35Z vasilche $
 * ===========================================================================
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
 *  Author:  Eugene Vasilchenko
 *
 *  File Description: Data reader from ID2
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbi_param.hpp>
#include <corelib/ncbi_config.hpp>
#include <corelib/ncbi_system.hpp> // for SleepSec

#include <objtools/data_loaders/genbank/id2/reader_id2.hpp>
#include <objtools/data_loaders/genbank/id2/reader_id2_entry.hpp>
#include <objtools/data_loaders/genbank/id2/reader_id2_params.h>
#include <objtools/data_loaders/genbank/readers.hpp> // for entry point
#include <objtools/data_loaders/genbank/dispatcher.hpp>
#include <objtools/data_loaders/genbank/processors.hpp>
#include <objtools/error_codes.hpp>

#include <objmgr/objmgr_exception.hpp>
#include <objmgr/impl/tse_info.hpp>
#include <objmgr/impl/tse_chunk_info.hpp>
#include <objmgr/impl/tse_split_info.hpp>

#include <objtools/data_loaders/genbank/request_result.hpp>

#include <corelib/ncbimtx.hpp>

#include <corelib/plugin_manager_impl.hpp>

#include <objects/general/Dbtag.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqset/Seq_entry.hpp>
#include <objects/id2/id2__.hpp>
#include <objects/seqsplit/ID2S_Split_Info.hpp>
#include <objects/seqsplit/ID2S_Chunk_Info.hpp>
#include <objects/seqsplit/ID2S_Chunk.hpp>

#include <serial/iterator.hpp>
#include <serial/objistrasnb.hpp>

#include <connect/ncbi_conn_stream.hpp>

#include <corelib/plugin_manager_store.hpp>

#include <iomanip>


#define NCBI_USE_ERRCODE_X   Objtools_Reader_Id2

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

#define DEFAULT_SERVICE  "ID2"
#define DEFAULT_NUM_CONN 3
#define MAX_MT_CONN      5

//#define GENBANK_ID2_RANDOM_FAILS 1
#define GENBANK_ID2_RANDOM_FAILS_FREQUENCY 20
#define GENBANK_ID2_RANDOM_FAILS_RECOVER 10 // new + write + read

#ifdef GENBANK_ID2_RANDOM_FAILS
static int& GetFailCounter(CId2Reader::TConn /*conn*/)
{
#ifdef NCBI_THREADS
    static CStaticTls<int> fail_counter;
    int* ptr = fail_counter.GetValue();
    if ( !ptr ) {
        ptr = new int(0);
        fail_counter.SetValue(ptr);
    }
    return *ptr;
#else
    static int fail_counter = 0;
    return fail_counter;
#endif
}

static void SetRandomFail(CNcbiIostream& stream, CId2Reader::TConn conn)
{
    int& value = GetFailCounter(conn);
    if ( ++value <= GENBANK_ID2_RANDOM_FAILS_RECOVER ) {
        return;
    }
    if ( rand() % GENBANK_ID2_RANDOM_FAILS_FREQUENCY == 0 ) {
        value = 0;
        stream.setstate(ios::badbit);
    }
}
#else
# define SetRandomFail(stream, conn) do{}while(0)
#endif


NCBI_PARAM_DECL(string, GENBANK, ID2_CGI_NAME);
NCBI_PARAM_DECL(string, GENBANK, ID2_SERVICE_NAME);
NCBI_PARAM_DECL(string, NCBI, SERVICE_NAME_ID2);

NCBI_PARAM_DEF_EX(string, GENBANK, ID2_CGI_NAME, kEmptyStr,
                  eParam_NoThread, GENBANK_ID2_CGI_NAME);
NCBI_PARAM_DEF_EX(string, GENBANK, ID2_SERVICE_NAME, kEmptyStr,
                  eParam_NoThread, GENBANK_ID2_SERVICE_NAME);
NCBI_PARAM_DEF_EX(string, NCBI, SERVICE_NAME_ID2, DEFAULT_SERVICE,
                  eParam_NoThread, GENBANK_SERVICE_NAME_ID2);

CId2Reader::CId2Reader(int max_connections)
    : m_Connector(DEFAULT_SERVICE)
{
    SetMaximumConnections(max_connections, DEFAULT_NUM_CONN);
}


CId2Reader::CId2Reader(const TPluginManagerParamTree* params,
                       const string& driver_name)
{
    CConfig conf(params);
    string service_name = conf.GetString(
        driver_name,
        NCBI_GBLOADER_READER_ID2_PARAM_SERVICE_NAME,
        CConfig::eErr_NoThrow,
        kEmptyStr);
    if ( service_name.empty() ) {
        service_name = NCBI_PARAM_TYPE(GENBANK, ID2_CGI_NAME)::GetDefault();
    }
    if ( service_name.empty() ) {
        service_name = NCBI_PARAM_TYPE(GENBANK,ID2_SERVICE_NAME)::GetDefault();
    }
    if ( service_name.empty() ) {
        service_name = NCBI_PARAM_TYPE(NCBI, SERVICE_NAME_ID2)::GetDefault();
    }
    m_Connector.SetServiceName(service_name);
    m_Connector.InitTimeouts(conf, driver_name);
    CReader::InitParams(conf, driver_name, DEFAULT_NUM_CONN);
}


CId2Reader::~CId2Reader()
{
}


int CId2Reader::GetMaximumConnectionsLimit(void) const
{
#ifdef NCBI_THREADS
    return MAX_MT_CONN;
#else
    return 1;
#endif
}


void CId2Reader::x_AddConnectionSlot(TConn conn)
{
    _ASSERT(!m_Connections.count(conn));
    m_Connections[conn];
}


void CId2Reader::x_RemoveConnectionSlot(TConn conn)
{
    _VERIFY(m_Connections.erase(conn));
}


void CId2Reader::x_DisconnectAtSlot(TConn conn, bool failed)
{
    _ASSERT(m_Connections.count(conn));
    CReaderServiceConnector::SConnInfo& conn_info = m_Connections[conn];
    m_Connector.RememberIfBad(conn_info);
    if ( conn_info.m_Stream ) {
        LOG_POST_X(1, Warning << "CId2Reader("<<conn<<"): ID2"
                   " GenBank connection "<<(failed? "failed": "too old")<<
                   ": reconnecting...");
        if ( GetDebugLevel() >= eTraceOpen ) {
            CDebugPrinter s(conn, "CId2Reader");
            s << "Closing ID2 connection";
        }
        conn_info.m_Stream.reset();
        if ( GetDebugLevel() >= eTraceOpen ) {
            CDebugPrinter s(conn, "CId2Reader");
            s << "Closed ID2 connection";
        }
    }
}


CConn_IOStream* CId2Reader::x_GetConnection(TConn conn)
{
    _ASSERT(m_Connections.count(conn));
    CReaderServiceConnector::SConnInfo& conn_info = m_Connections[conn];
    if ( conn_info.m_Stream.get() ) {
        return conn_info.m_Stream.get();
    }
    OpenConnection(conn);
    return m_Connections[conn].m_Stream.get();
}


CConn_IOStream* CId2Reader::x_GetCurrentConnection(TConn conn) const
{
    TConnections::const_iterator iter = m_Connections.find(conn);
    return iter == m_Connections.end()? 0: iter->second.m_Stream.get();
}


string CId2Reader::x_ConnDescription(CConn_IOStream& stream) const
{
    return m_Connector.GetConnDescription(stream);
}


string CId2Reader::x_ConnDescription(TConn conn) const
{
    CConn_IOStream* stream = x_GetCurrentConnection(conn);
    return stream? x_ConnDescription(*stream): "NULL";
}


void CId2Reader::x_ConnectAtSlot(TConn conn)
{
    if ( GetDebugLevel() >= eTraceOpen ) {
        CDebugPrinter s(conn, "CId2Reader");
        s << "New connection to " << m_Connector.GetServiceName() << "...";
    }
    
    CReaderServiceConnector::SConnInfo conn_info = m_Connector.Connect();

    CConn_IOStream& stream = *conn_info.m_Stream;
    SetRandomFail(stream, conn);
    if ( stream.bad() ) {
        NCBI_THROW(CLoaderException, eConnectionFailed,
                   "cannot open connection: "+x_ConnDescription(stream));
    }

    if ( GetDebugLevel() >= eTraceOpen ) {
        CDebugPrinter s(conn, "CId2Reader");
        s << "New connection: " << x_ConnDescription(stream);
    }
    try {
        x_InitConnection(stream, conn);
    }
    catch ( CException& exc ) {
        m_Connector.RememberIfBad(conn_info);
        NCBI_RETHROW(exc, CLoaderException, eConnectionFailed,
                     "connection initialization failed: "+
                     x_ConnDescription(stream));
    }
    SetRandomFail(stream, conn);
    if ( stream.bad() ) {
        NCBI_THROW(CLoaderException, eConnectionFailed,
                   "connection initialization failed: "+
                   x_ConnDescription(stream));
    }
    // successfully received reply, server is good, forget it
    conn_info.MarkAsGood();

    STimeout tmout;
    m_Connector.SetTimeoutTo(&tmout);
    CONN_SetTimeout(stream.GetCONN(), eIO_ReadWrite, &tmout);
    tmout.sec = 0; tmout.usec = 1; // no wait on close
    CONN_SetTimeout(stream.GetCONN(), eIO_Close, &tmout);
   
    m_Connections[conn] = conn_info;
}


#define MConnFormat MSerial_AsnBinary


void CId2Reader::x_InitConnection(CConn_IOStream& stream, TConn conn)
{
    // prepare init request
    CID2_Request req;
    req.SetRequest().SetInit();
    x_SetContextData(req);
    CID2_Request_Packet packet;
    packet.Set().push_back(Ref(&req));
    // that's it for now
    // TODO: add params

    // send init request
    {{
        if ( GetDebugLevel() >= eTraceConn ) {
            CDebugPrinter s(conn, "CId2Reader");
            s << "Sending";
            if ( GetDebugLevel() >= eTraceASN ) {
                s << ": " << MSerial_AsnText << packet;
            }
            else {
                s << " ID2-Request-Packet";
            }
            s << "...";
        }
        try {
            stream << MConnFormat << packet << flush;
        }
        catch ( CException& exc ) {
            NCBI_RETHROW(exc, CLoaderException, eConnectionFailed,
                         "failed to send init request: "+
                         x_ConnDescription(stream));
        }
        if ( GetDebugLevel() >= eTraceConn ) {
            CDebugPrinter s(conn, "CId2Reader");
            s << "Sent ID2-Request-Packet.";
        }
        if ( !stream ) {
            NCBI_THROW(CLoaderException, eConnectionFailed,
                       "failed to send init request: "+
                       x_ConnDescription(stream));
        }
    }}
    
    // receive init reply
    CID2_Reply reply;
    {{
        if ( GetDebugLevel() >= eTraceConn ) {
            CDebugPrinter s(conn, "CId2Reader");
            s << "Receiving ID2-Reply...";
        }
        stream >> MConnFormat >> reply;
        if ( GetDebugLevel() >= eTraceConn   ) {
            CDebugPrinter s(conn, "CId2Reader");
            s << "Received";
            if ( GetDebugLevel() >= eTraceASN ) {
                s << ": " << MSerial_AsnText << reply;
            }
            else {
                s << " ID2-Reply.";
            }
        }
        if ( !stream ) {
            NCBI_THROW(CLoaderException, eLoaderFailed,
                       "failed to receive init reply: "+
                       x_ConnDescription(stream));
        }
    }}

    // check init reply
    if ( reply.IsSetDiscard() ) {
        NCBI_THROW(CLoaderException, eLoaderFailed,
                   "bad init reply: 'discard' is set: "+
                   x_ConnDescription(stream));
    }
    if ( reply.IsSetError() ) {
        NCBI_THROW(CLoaderException, eLoaderFailed,
                   "bad init reply: 'error' is set: "+
                   x_ConnDescription(stream));
    }
    if ( !reply.IsSetEnd_of_reply() ) {
        NCBI_THROW(CLoaderException, eLoaderFailed,
                   "bad init reply: 'end-of-reply' is not set: "+
                   x_ConnDescription(stream));
    }
    if ( reply.GetReply().Which() != CID2_Reply::TReply::e_Init ) {
        NCBI_THROW(CLoaderException, eLoaderFailed,
                   "bad init reply: 'reply' is not 'init': "+
                   x_ConnDescription(stream));
    }
    // that's it for now
    // TODO: process params
}


void CId2Reader::x_SendPacket(TConn conn, const CID2_Request_Packet& packet)
{
    CConn_IOStream& stream = *x_GetConnection(conn);
    stream << MConnFormat << packet << flush;
}


void CId2Reader::x_ReceiveReply(TConn conn, CID2_Reply& reply)
{
    CConn_IOStream& stream = *x_GetConnection(conn);
    SetRandomFail(stream, conn);
    CObjectIStreamAsnBinary in(stream);
    CId2ReaderBase::x_ReceiveReply(in, conn, reply);
    if ( !stream ) {
        NCBI_THROW(CLoaderException, eConnectionFailed,
                   "failed to receive reply: "+
                   x_ConnDescription(stream));
    }
    // successfully receved reply, server is good, forget it
    m_Connections[conn].MarkAsGood();
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


END_SCOPE(objects)

void GenBankReaders_Register_Id2(void)
{
    RegisterEntryPoint<objects::CReader>(NCBI_EntryPoint_Id2Reader);
}


/// Class factory for ID2 reader
///
/// @internal
///
class CId2ReaderCF : 
    public CSimpleClassFactoryImpl<objects::CReader, objects::CId2Reader>
{
public:
    typedef CSimpleClassFactoryImpl<objects::CReader,
                                    objects::CId2Reader> TParent;
public:
    CId2ReaderCF()
        : TParent(NCBI_GBLOADER_READER_ID2_DRIVER_NAME, 0) {}
    ~CId2ReaderCF() {}

    objects::CReader* 
    CreateInstance(const string& driver  = kEmptyStr,
                   CVersionInfo version =
                   NCBI_INTERFACE_VERSION(objects::CReader),
                   const TPluginManagerParamTree* params = 0) const
    {
        objects::CReader* drv = 0;
        if ( !driver.empty()  &&  driver != m_DriverName ) {
            return 0;
        }
        if (version.Match(NCBI_INTERFACE_VERSION(objects::CReader)) 
                            != CVersionInfo::eNonCompatible) {
            drv = new objects::CId2Reader(params, driver);
        }
        return drv;
    }
};


void NCBI_EntryPoint_Id2Reader(
     CPluginManager<objects::CReader>::TDriverInfoList&   info_list,
     CPluginManager<objects::CReader>::EEntryPointRequest method)
{
    CHostEntryPointImpl<CId2ReaderCF>::NCBI_EntryPointImpl(info_list, method);
}


void NCBI_EntryPoint_xreader_id2(
     CPluginManager<objects::CReader>::TDriverInfoList&   info_list,
     CPluginManager<objects::CReader>::EEntryPointRequest method)
{
    NCBI_EntryPoint_Id2Reader(info_list, method);
}


END_NCBI_SCOPE
