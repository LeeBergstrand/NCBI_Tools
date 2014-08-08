#ifndef _GRID_DEBUG_CONTEXT_HPP_
#define _GRID_DEBUG_CONTEXT_HPP_


/*  $Id: grid_debug_context.hpp 196780 2010-07-08 16:50:11Z kazimird $
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
 *   Government have not placed any restriction on its use or reproduction.
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
 * Authors:  Maxim Didenko
 *
 * File Description:
 *    NetSchedule Worker Node implementation
 */

#include <connect/services/netcache_api.hpp>

#include <corelib/ncbimisc.hpp>
#include <corelib/ncbistre.hpp>
#include <corelib/ncbimtx.hpp>

#include <connect/connect_export.h>

#include <map>

BEGIN_NCBI_SCOPE

/////////////////////////////////////////////////////////////////////////////
//
///@internal
class NCBI_XCONNECT_EXPORT CGridDebugContext
{
public:
    enum eMode {
        eGDC_NoDebug = 0,
        eGDC_Gather,  /// Gather input/output/msg
        eGDC_Execute
    };

    static CGridDebugContext& Create(eMode, CNetCacheAPI::TInstance);
    static CGridDebugContext* GetInstance();

    ~CGridDebugContext();

    void SetRunName(const string& name);
    int SetExecuteList(const string& files);
    eMode GetDebugMode() const { return m_Mode; }

    bool GetNextJob(string& job_key, string& blob_id);

    string GetLogFileName() const;

    void DumpInput(const string& blob_id, unsigned int job_number);
    void DumpOutput(const string& job_key,
                    const string& blob_id,
                    unsigned int job_number);
    void DumpProgressMessage(const string& job_key,
                             const string& msg,
                             unsigned int job_number);


private:
    CGridDebugContext(eMode, CNetCacheAPI::TInstance);
    static auto_ptr<CGridDebugContext> sm_Instance;

    eMode m_Mode;
    string m_RunName;
    string m_SPid;

    CNetCacheAPI m_NetCacheAPI;

    map<string,string> m_Blobs;
    map<string,string>::const_iterator m_CurrentJob;

    CGridDebugContext(const CGridDebugContext&);
    CGridDebugContext& operator=(const CGridDebugContext&);

    void x_DumpBlob(const string& blob_id, const string& fname);
    CNetCacheAPI GetNetCacheAPI() { return  m_NetCacheAPI; }

};

END_NCBI_SCOPE

#endif // _GRID_DEBUG_CONTEXT_HPP_
