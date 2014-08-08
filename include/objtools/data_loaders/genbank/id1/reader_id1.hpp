#ifndef READER_ID1__HPP_INCLUDED
#define READER_ID1__HPP_INCLUDED

/*  $Id: reader_id1.hpp 178597 2009-12-14 20:14:16Z vasilche $
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
* ===========================================================================
*
*  Author:  Anton Butanaev, Eugene Vasilchenko
*
*  File Description: Data reader from ID1
*
*/

#include <objtools/data_loaders/genbank/reader_id1_base.hpp>
#include <objtools/data_loaders/genbank/reader_service.hpp>
#include <map>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CID1server_back;
class CID1server_request;
class CID1server_maxcomplex;
class CLoadLockSeq_ids;
class CLoadLockBlob_ids;

class NCBI_XREADER_ID1_EXPORT CId1Reader : public CId1ReaderBase
{
public:
    CId1Reader(int max_connections = 0);
    CId1Reader(const TPluginManagerParamTree* params,
               const string& driver_name);
    ~CId1Reader();

    int GetMaximumConnectionsLimit(void) const;

    //////////////////////////////////////////////////////////////////
    // Id resolution methods:

    bool LoadSeq_idGi(CReaderRequestResult& result,
                      const CSeq_id_Handle& seq_id);

    void GetBlobVersion(CReaderRequestResult& result,
                        const CBlob_id& blob_id);
    void GetSeq_idSeq_ids(CReaderRequestResult& result,
                          CLoadLockSeq_ids& ids,
                          const CSeq_id_Handle& id);
    bool GetSeq_idBlob_ids(CReaderRequestResult& result,
                           CLoadLockBlob_ids& ids,
                           const CSeq_id_Handle& id,
                           const SAnnotSelector* sel);
    void GetGiSeq_ids(CReaderRequestResult& result,
                      const CSeq_id_Handle& seq_id,
                      CLoadLockSeq_ids& ids);
    void GetGiBlob_ids(CReaderRequestResult& result,
                       const CSeq_id_Handle& seq_id,
                       CLoadLockBlob_ids& ids);

    //////////////////////////////////////////////////////////////////
    // Blob loading methods:

    void GetBlob(CReaderRequestResult& result,
                 const TBlobId& blob_id,
                 TChunkId chunk_id);

protected:
    virtual void x_AddConnectionSlot(TConn conn);
    virtual void x_RemoveConnectionSlot(TConn conn);
    virtual void x_DisconnectAtSlot(TConn conn, bool failed);
    virtual void x_ConnectAtSlot(TConn conn);

    string x_ConnDescription(CConn_IOStream& stream) const;
    CConn_IOStream* x_GetConnection(TConn conn);

    // returns error blob state parsed from ID1server-back.error
    TBlobState x_ResolveId(CReaderRequestResult& result,
                           CID1server_back& id1_reply,
                           const CID1server_request& id1_request);

    void x_SendRequest(TConn conn, const CID1server_request& request);
    void x_ReceiveReply(TConn conn, CID1server_back& reply);
    void x_SendRequest(const CBlob_id& blob_id, TConn conn);

    void x_SetParams(CID1server_maxcomplex& params,
                     const CBlob_id& blob_id);
    void x_SetBlobRequest(CID1server_request& request,
                          const CBlob_id& blob_id);

protected:
    CRef<CTSE_Info> x_ReceiveMainBlob(CConn_IOStream* stream);

    CReaderServiceConnector m_Connector;

    typedef map< TConn, CReaderServiceConnector::SConnInfo > TConnections;
    TConnections m_Connections;
};


END_SCOPE(objects)
END_NCBI_SCOPE

#endif // READER_ID1__HPP_INCLUDED
