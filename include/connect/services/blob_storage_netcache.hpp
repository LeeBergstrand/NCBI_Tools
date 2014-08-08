#ifndef CONNECT_SERVICES__BLOB_STORAGE_NETCACHE_HPP
#define CONNECT_SERVICES__BLOB_STORAGE_NETCACHE_HPP

/*  $Id: blob_storage_netcache.hpp 193444 2010-06-03 19:28:37Z kazimird $
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
 * Authors:  Maxim Didenko, Dmitry Kazimirov
 *
 */

#include <connect/services/netcache_api.hpp>


BEGIN_NCBI_SCOPE

extern NCBI_BLOBSTORAGE_NETCACHE_EXPORT
    const char* const kBlobStorageNetCacheDriverName;

extern "C"
{

NCBI_BLOBSTORAGE_NETCACHE_EXPORT
void NCBI_EntryPoint_xblobstorage_netcache(
     CPluginManager<IBlobStorage>::TDriverInfoList&   info_list,
     CPluginManager<IBlobStorage>::EEntryPointRequest method);

NCBI_BLOBSTORAGE_NETCACHE_EXPORT
void BlobStorage_RegisterDriver_NetCache(void);

} // extern C


END_NCBI_SCOPE

#endif // CONNECT_SERVICES__BLOB_STORAGE_NETCACHE_HPP
