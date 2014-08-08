#ifndef WRITER_CACHE__HPP_INCLUDED
#define WRITER_CACHE__HPP_INCLUDED

/*  $Id: writer_cache.hpp 201218 2010-08-17 14:38:33Z vasilche $
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
*  Author:  Eugene Vasilchenko, Anatoliy Kuznetsov
*
*  File Description: Cached extension of data reader from ID1
*
*/

#include <objtools/data_loaders/genbank/writer.hpp>
#include <objtools/data_loaders/genbank/cache/reader_cache.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>

BEGIN_NCBI_SCOPE

class ICache;

BEGIN_SCOPE(objects)

class CLoadLockSeq_ids;

class NCBI_XREADER_CACHE_EXPORT CCacheWriter : public CWriter,
                                               public CCacheHolder,
                                               public SCacheInfo
{
public:
    CCacheWriter(void);

    virtual void SaveStringSeq_ids(CReaderRequestResult& result,
                                   const string& seq_id);
    virtual void SaveStringGi(CReaderRequestResult& result,
                              const string& seq_id);
    virtual void SaveSeq_idSeq_ids(CReaderRequestResult& result,
                                   const CSeq_id_Handle& seq_id);
    virtual void SaveSeq_idGi(CReaderRequestResult& result,
                              const CSeq_id_Handle& seq_id);
    virtual void SaveSeq_idAccVer(CReaderRequestResult& result,
                                  const CSeq_id_Handle& seq_id);
    virtual void SaveSeq_idLabel(CReaderRequestResult& result,
                                 const CSeq_id_Handle& seq_id);
    virtual void SaveSeq_idTaxId(CReaderRequestResult& result,
                                 const CSeq_id_Handle& seq_id);
    virtual void SaveSeq_idBlob_ids(CReaderRequestResult& result,
                                    const CSeq_id_Handle& seq_id,
                                    const SAnnotSelector* sel);
    virtual void SaveBlobVersion(CReaderRequestResult& result,
                                 const TBlobId& blob_id,
                                 TBlobVersion version);

    virtual CRef<CBlobStream> OpenBlobStream(CReaderRequestResult& result,
                                             const TBlobId& blob_id,
                                             TChunkId chunk_id,
                                             const CProcessor& processor);

    virtual bool CanWrite(EType type) const;

    void WriteSeq_ids(const string& key, const CLoadLockSeq_ids& ids);

    virtual void InitializeCache(CReaderCacheManager& cache_manager,
                                 const TPluginManagerParamTree* params);
    virtual void ResetCache(void);
};


END_SCOPE(objects)
END_NCBI_SCOPE

#endif // WRITER_CACHE__HPP_INCLUDED
