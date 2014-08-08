#ifndef MISC_GRID_CGI___CGI_SESSION_NETCACHE__HPP
#define MISC_GRID_CGI___CGI_SESSION_NETCACHE__HPP

/*  $Id: cgi_session_netcache.hpp 196780 2010-07-08 16:50:11Z kazimird $
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
* Author:  Maxim Didenko
*
*/

#include <connect/services/netcache_api.hpp>

#include <cgi/cgi_session.hpp>

#include <corelib/ncbiexpt.hpp>

#include <map>
#include <string>
#include <stddef.h>


BEGIN_NCBI_SCOPE

class IRegistry;

///////////////////////////////////////////////////////////////////////////////
//
//  CCgiSession_NetCache -- 
//  
//  Implementation of ICgiSessionStorage interface based on NetCache service
//
//  @note
//    This implementation can work only with one attribute at a time. This
//    means that if a user requested a stream to the first attribute and then
//    requests a stream or reading/writing the second one, the 
//    stream to the fist attribute is getting closed.
//
class NCBI_XGRIDCGI_EXPORT CCgiSession_NetCache : public ICgiSessionStorage
{
public:
    /// Create Session Storage from the registry
    CCgiSession_NetCache(const IRegistry&);

    virtual ~CCgiSession_NetCache();

    /// Create a new empty session. 
    /// @return ID of the new session
    virtual string CreateNewSession();

    /// Modify session id. 
    /// Change Id of the current session.
    virtual void ModifySessionId(const string& new_id);

    /// Load the session
    /// @param[in] sesseionid
    ///  ID of the session
    /// @return true if the session was loaded, false otherwise
    virtual bool LoadSession(const string& sessionid);

    /// Retrieve names of all attributes attached to this session.
    virtual TNames GetAttributeNames(void) const;

    /// Get input stream to read an attribute data from.
    /// @param[in] name
    ///  Name of the attribute
    /// @param[out] size
    ///  Size of the attribute data
    /// @return 
    ///  Stream to read attribute data from.If the attribute does not exist,
    ///  then return an empty stream.
    virtual CNcbiIstream& GetAttrIStream(const string& name,
                                         size_t* size = 0);

    /// Get output stream to write an attribute data to.
    /// If the attribute does not exist it will be created and added 
    /// to the session. If the attribute exists its content will be
    /// overwritten.
    /// @param[in] name
    ///  Name of the attribute
    virtual CNcbiOstream& GetAttrOStream(const string& name);

    /// Set attribute data as a string.
    /// @param[in] name
    ///  Name of the attribute to set
    /// @param[in] value
    ///  Value to set the attribute data to
    virtual void SetAttribute(const string& name, const string& value);

    /// Get attribute data as string.
    /// @param[in] name
    ///  Name of the attribute to retrieve
    /// @return
    ///  Data of the attribute, if set.
    /// @throw CCgiSessionException with error code eNotLoaded
    ///  if the session has not been loaded yet;
    ///  CCgiSessionException with error code eAttrNotFound if
    ///  attribute with the specified name was not found;
    ///  CCgiSessionException with error code eImplException if
    ///  an error occurred during attribute retrieval -- in the
    ///  latter case, more information can be obtained from the
    ///  embedded exception.
    virtual string GetAttribute(const string& name) const;  

    /// Remove attribute from the session.
    /// @param[in] name
    ///  Name of the attribute to remove
    virtual void RemoveAttribute(const string& name);

    /// Delete current session
    virtual void DeleteSession();

    /// Close all open connections
    virtual void Reset();

private:
    typedef map<string,string> TBlobs;

    string m_SessionId;
    CBlobStorage_NetCache m_Storage;
    
    TBlobs m_Blobs;
    bool m_Dirty;

    bool m_Loaded;
    
    void x_CheckStatus() const;

    CCgiSession_NetCache(const CCgiSession_NetCache&);
    CCgiSession_NetCache& operator=(const CCgiSession_NetCache&);
};


END_NCBI_SCOPE


/* @} */

#endif  /* MISC_GRID_CGI___CGI_SESSION_NETCACHE__HPP */
