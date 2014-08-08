#ifndef CONN___NETCACHE_API__HPP
#define CONN___NETCACHE_API__HPP

/*  $Id: netcache_api.hpp 398263 2013-05-03 19:23:25Z rafanovi $
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
 * Authors:  Anatoliy Kuznetsov, Maxim Didenko
 *
 * File Description:
 *   Net cache client API.
 *
 */

/// @file netcache_api.hpp
/// NetCache client specs.
///

#include "netcache_admin.hpp"
#include "netservice_api.hpp"
#include "netcache_api_expt.hpp"
#include "netcache_key.hpp"

#include <util/simple_buffer.hpp>

#include <corelib/plugin_manager.hpp>
#include <corelib/blob_storage.hpp>


BEGIN_NCBI_SCOPE


/** @addtogroup NetCacheClient
 *
 * @{
 */

struct SNetCacheAPIImpl;

/// Client API for NetCache server.
///
/// It is undesirable to create objects of this class on the heap
/// because they are essentially smart pointers to the implementation
/// objects allocated internally on the heap.
///
/// @note After sending blob data to a NetCache server,
/// this class waits for a confirmation from the server,
/// and the connection cannot be used before this
/// confirmation is read.
///
///
class NCBI_XCONNECT_EXPORT CNetCacheAPI
{
    NCBI_NET_COMPONENT(NetCacheAPI);

    /// Allows to define caching behavior on a per-call basis.
    enum ECachingMode {
        eCaching_AppDefault,
        eCaching_Disable,
        eCaching_Enable
    };

    /// Defines how this object must be initialized.
    enum EAppRegistry {
        eAppRegistry
    };

    /// Creates an instance of CNetCacheAPI and initializes
    /// it with parameters read from the application registry.
    /// @param use_app_reg
    ///   Selects this constructor.
    ///   The parameter is not used otherwise.
    /// @param conf_section
    ///   Name of the registry section to look for the configuration
    ///   parameters in.  If empty string is passed, then the section
    ///   name "netcache_api" will be used.
    explicit CNetCacheAPI(EAppRegistry use_app_reg,
        const string& conf_section = kEmptyStr);

    /// Constructs a CNetCacheAPI object and initializes it with
    /// parameters read from the specified registry object.
    /// @param reg
    ///   Registry to get the configuration parameters from.
    /// @param conf_section
    ///   Name of the registry section to look for the configuration
    ///   parameters in.  If empty string is passed, then the section
    ///   name "netcache_api" will be used.
    explicit CNetCacheAPI(const IRegistry& reg,
        const string& conf_section = kEmptyStr);

    /// Constructs a CNetCacheAPI object and initializes it with
    /// parameters read from the specified configuration object.
    /// @param conf
    ///   A CConfig object to get the configuration parameters from.
    /// @param conf_section
    ///   Name of the configuration section where to look for the
    ///   parameters.  If empty string is passed, then the section
    ///   name "netcache_api" will be used.
    explicit CNetCacheAPI(CConfig* conf,
        const string& conf_section = kEmptyStr);

    explicit CNetCacheAPI(const string& client_name);

    /// Construct client, working with the specified service
    CNetCacheAPI(const string& service_name,
        const string& client_name);

    /// Put BLOB to server.  This method is blocking and waits
    /// for a confirmation from NetCache after all data is
    /// transferred.
    ///
    /// @param time_to_live
    ///    BLOB time to live value in seconds.
    ///    0 - server side default is assumed.
    ///
    /// Please note that time_to_live is controlled by the server-side
    /// parameter so if you set time_to_live higher than server-side value,
    /// server side TTL will be in effect.
    ///
    /// @return NetCache access key
    string PutData(const void*   buf,
                   size_t        size,
                   unsigned int  time_to_live = 0);

    /// Put BLOB to server.  This method is blocking, it
    /// waits for a confirmation from NetCache after all
    /// data is transferred. Since blob EOF marker is sent in the
    /// destructor, the blob will not be created until the stream
    /// is deleted.
    ///
    /// @param key
    ///    NetCache key, if empty new key is created
    /// @param time_to_live
    ///    BLOB time to live value in seconds.
    ///    0 - server side default is assumed.
    /// @param caching_mode
    ///    Defines whether to enable file caching.
    /// @return
    ///    IEmbeddedStreamWriter* (caller must delete it).
    IEmbeddedStreamWriter* PutData(string* key,
        unsigned int time_to_live = 0,
        ECachingMode caching_mode = eCaching_AppDefault);

    /// Update an existing BLOB.  Just like all other PutData
    /// methods, this one is blocking and waits for a confirmation
    /// from NetCache after all data is transferred.
    string PutData(const string& key,
                   const void*   buf,
                   size_t        size,
                   unsigned int  time_to_live = 0);

    /// Create a stream object for sending data to a blob.
    /// If the string "key" is empty, a new blob will be created
    /// and its ID will be returned via the "key" parameter.
    /// Note:  The blob will not be available from NetCache until
    /// the stream is destructed.
    CNcbiOstream* CreateOStream(string& key, unsigned time_to_live = 0);

    /// Check if the BLOB identified by the key "key" exists.
    ///
    /// @param key
    ///    Key of the BLOB to check for existence.
    ///
    /// @return
    ///    True, if the BLOB exists; false otherwise.
    bool HasBlob(const string& blob_id);

    /// Returns the size of the BLOB identified by the "key" parameter.
    ///
    /// @param key
    ///    The key of the BLOB the size of which to be returned.
    ///
    /// @return
    ///    Size of the BLOB in bytes.
    size_t GetBlobSize(const string& blob_id);

    /// Get a pointer to the IReader interface to read blob contents.
    /// This is a safe version of the GetData method having the same
    /// signature. Unlike GetData, GetReader will throw an exception
    /// if the requested blob is not found.
    ///
    /// @note
    ///   The Read() method of the returned IReader object is not
    ///   blocking. A reading completion loop is required, see an
    ///   example below.
    ///
    /// @code
    /// size_t blob_size;
    /// auto_ptr<IReader> reader(nc_api.GetReader(key, &blob_size));
    /// size_t bytes_read;
    /// size_t total_bytes_read = 0;
    ///
    /// while (buf_size > 0) {
    ///     ERW_Result rw_res = reader->Read(buf_ptr, buf_size, &bytes_read);
    ///     if (rw_res == eRW_Success) {
    ///         total_bytes_read += bytes_read;
    ///         buf_ptr += bytes_read;
    ///         buf_size -= bytes_read;
    ///     } else if (rw_res == eRW_Eof) {
    ///         break;
    ///     } else {
    ///         NCBI_THROW(CNetServiceException, eCommunicationError,
    ///             "Error while reading BLOB");
    ///     }
    /// }
    ///
    /// return total_bytes_read;
    /// @endcode
    IReader* GetReader(const string& key, size_t* blob_size = NULL,
        ECachingMode caching_mode = eCaching_AppDefault);

    /// Get a pointer to the IReader interface to read a portion of
    /// the blob contents. See the description of GetReader() for details.
    /// The Read() method of the returned IReader interface implementation
    /// is not blocking.
    /// @see CNetCacheAPI::GetReader() for details.
    IReader* GetPartReader(const string& key,
        size_t offset, size_t part_size, size_t* blob_size = NULL,
        ECachingMode caching_mode = eCaching_AppDefault);

    /// Read the blob pointed to by "key" and store its contents
    /// in "buffer". The output string is resized as required.
    ///
    /// @throw CNetCacheException
    ///    Thrown if either the blob was not found or
    ///    a protocol error occurred.
    /// @throw CNetServiceException
    ///    Thrown if a communication error occurred.
    void ReadData(const string& key, string& buffer);

    /// Read a part of the blob pointed to by "key" and store its contents
    /// in "buffer". The output string is resized as required.
    ///
    /// @throw CNetCacheException
    ///    Thrown if either the blob was not found or
    ///    a protocol error occurred.
    /// @throw CNetServiceException
    ///    Thrown if a communication error occurred.
    void ReadPart(const string& key,
        size_t offset, size_t part_size, string& buffer);

    /// Retrieve BLOB from server by key.
    //
    /// Caller is responsible for deletion of the IReader* object.
    /// It must be deleted before the destruction of CNetCacheAPI.
    ///
    /// @note
    ///   IReader implementation used here is based on TCP/IP
    ///   sockets; when reading the blob, please remember to check
    ///   IReader::Read return codes, it may not be able to read
    ///   the whole blob in one call because of network delays.
    /// @see CNetCacheAPI::GetReader() for details.
    ///
    /// @param key
    ///    BLOB key to read (returned by PutData)
    /// @param blob_size
    ///    Pointer to the memory location where the size
    ///    of the requested blob will be stored.
    /// @param caching_mode
    ///    Overrides the default caching mode defined in the application
    ///    registry.
    /// @return
    ///    If the requested blob is found, the method returns a pointer
    ///    to the IReader interface for reading the blob contents (the
    ///    caller must delete it). If the blob is not found (that is,
    ///    if it's expired), NULL is returned.
    IReader* GetData(const string& key, size_t* blob_size = NULL,
        ECachingMode caching_mode = eCaching_AppDefault);

    /// Status of GetData() call
    /// @sa GetData
    enum EReadResult {
        eReadComplete, ///< The whole BLOB has been read
        eNotFound,     ///< BLOB not found or error
        eReadPart      ///< Read part of the BLOB (buffer capacity)
    };

    /// Retrieve BLOB from server by key
    ///
    /// @note
    ///    Function waits for enough data to arrive.
    EReadResult GetData(const string&  key,
                        void*          buf,
                        size_t         buf_size,
                        size_t*        n_read    = 0,
                        size_t*        blob_size = 0);

    /// Retrieve BLOB from server by key
    /// This method retrieves BLOB size, allocates memory and gets all
    /// the data from the server.
    ///
    /// Blob size and binary data is placed into blob_to_read structure.
    /// Do not use this method if you are not sure you have memory
    /// to load the whole BLOB.
    ///
    /// @return
    ///    eReadComplete if BLOB found (eNotFound otherwise)
    EReadResult GetData(const string& key, CSimpleBuffer& buffer);

    /// Create an istream object for reading blob data.
    /// @throw CNetCacheException
    ///    The requested blob does not exist.
    CNcbiIstream* GetIStream(const string& key, size_t* blob_size = NULL);

    /// Retrieve BLOB's owner information as registered by the server
    string GetOwner(const string& blob_id);

    /// Remove BLOB by key
    void Remove(const string& blob_id);

    /// Return a CNetServerMultilineCmdOutput object for reading
    /// meta information about the specified blob.
    CNetServerMultilineCmdOutput GetBlobInfo(const string& blob_id);

    /// Print meta information about the specified blob.
    void PrintBlobInfo(const string& blob_key);

    void ProlongBlobLifetime(const string& blob_key, unsigned ttl);

    /// Mirroring modes. eIfKeyMirrored means unconditionally
    /// enable mirroring for blobs that were created in mirroring
    /// mode.
    enum EMirroringMode {
        eMirroringDisabled,
        eMirroringEnabled,
        eIfKeyMirrored
    };
    void SetMirroringMode(EMirroringMode mirroring_mode);

    CNetCacheAdmin GetAdmin();

    CNetService GetService();

    void SetCommunicationTimeout(const STimeout& to)
        {GetService().GetServerPool().SetCommunicationTimeout(to);}

    /// Extract one of the servers comprising this service
    /// as a separate CNetCacheAPI object.
    /// This method is for use by the grid_cli utility only.
    /// @internal
    CNetCacheAPI GetServer(CNetServer::TInstance server);

    /// This method is for use by the grid_cli utility only.
    /// @internal
    void SetEventHandler(INetEventHandler* event_handler);
};

class NCBI_XCONNECT_EXPORT CNetCachePasswordGuard
{
public:
    CNetCachePasswordGuard(CNetCacheAPI::TInstance nc_api,
        const string& password);

    CNetCacheAPI* operator ->() {return &m_NetCacheAPI;}

private:
    CNetCacheAPI m_NetCacheAPI;
};

NCBI_DECLARE_INTERFACE_VERSION(SNetCacheAPIImpl, "xnetcacheapi", 1, 1, 0);

extern NCBI_XCONNECT_EXPORT const char* const kNetCacheAPIDriverName;

void NCBI_XCONNECT_EXPORT NCBI_EntryPoint_xnetcacheapi(
     CPluginManager<SNetCacheAPIImpl>::TDriverInfoList&   info_list,
     CPluginManager<SNetCacheAPIImpl>::EEntryPointRequest method);


/// CBlobStorage_NetCache -- NetCache-based implementation of IBlobStorage
///
class NCBI_XCONNECT_EXPORT CBlobStorage_NetCache : public IBlobStorage
{
public:
    CBlobStorage_NetCache();

    /// Create Blob Storage
    /// @param[in] nc_client
    ///  NetCache client - an instance of CNetCacheAPI.
    CBlobStorage_NetCache(CNetCacheAPI::TInstance nc_client) :
        m_NCClient(nc_client) {}

    virtual ~CBlobStorage_NetCache();


    virtual bool IsKeyValid(const string& str);

    /// Get a blob content as a string
    ///
    /// @param[in] blob_key
    ///    Blob key to read
    virtual string        GetBlobAsString(const string& data_id);

    /// Get an input stream to a blob
    ///
    /// @param[in] blob_key
    ///    Blob key to read
    /// @param[out] blob_size_ptr
    ///    if blob_size_ptr if not NULL the size of a blob is returned
    /// @param[in] lock_mode
    ///    Blob locking mode
    virtual CNcbiIstream& GetIStream(const string& data_id,
                                     size_t* blob_size_ptr = 0,
                                     ELockMode lock_mode = eLockWait);

    /// Get an output stream to a blob
    ///
    /// @param[in,out] blob_key
    ///    Blob key to read. If a blob with a given key does not exist
    ///    an key of a newly create blob will be assigned to blob_key
    /// @param[in] lock_mode
    ///    Blob locking mode
    virtual CNcbiOstream& CreateOStream(string& data_id,
                                        ELockMode lock_mode = eLockNoWait);

    /// Create an new blob
    ///
    /// @return
    ///     Newly create blob key
    virtual string CreateEmptyBlob();

    /// Delete a blob
    ///
    /// @param[in] blob_key
    ///    Blob key to read
    virtual void DeleteBlob(const string& data_id);

    /// Close all streams and connections.
    virtual void Reset();

    CNetCacheAPI GetNetCacheAPI() const {return m_NCClient;}

private:
    CNetCacheAPI m_NCClient;

    auto_ptr<CNcbiIstream> m_IStream;
    auto_ptr<CNcbiOstream> m_OStream;

    CBlobStorage_NetCache(const CBlobStorage_NetCache&);
    CBlobStorage_NetCache& operator=(CBlobStorage_NetCache&);
};

/* @} */

END_NCBI_SCOPE

#endif  /* CONN___NETCACHE_API__HPP */
