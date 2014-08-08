#ifndef __SAMPLE_EDIT_SAVER__HPP
#define __SAMPLE_EDIT_SAVER__HPP

/*  $Id: sample_saver.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author: Maxim Didenko
*
* File Description:
*
*/

#include <corelib/ncbiobj.hpp>
#include <objmgr/simple_editsaver.hpp>

#include <objmgr/bioseq_handle.hpp>

#include "asn_db.hpp"

#include <set>
#include <map>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CSampleEditSaver : public CSimpleEditSaver
{
public:
    CSampleEditSaver(const string& dbpath);
    CSampleEditSaver(CAsnDB& db);
    virtual ~CSampleEditSaver();

    virtual void BeginTransaction();
    virtual void CommitTransaction();
    virtual void RollbackTransaction();

    //-----------------------------------------------------------------
    virtual void UpdateSeq(const CBioseq_Handle& handle, 
                           IEditSaver::ECallMode mode);

    virtual void UpdateTSE(const CSeq_entry_Handle& handle, 
                           IEditSaver::ECallMode mode);
private:

    void x_CleanUp();

    auto_ptr<CAsnDB> m_AsnDBGuard;
    CAsnDB&          m_AsnDB;
    
    typedef set<CBioseq_Handle> TBioseqCont;
    typedef map<CBlobIdKey, TBioseqCont> TBlobCont;
    typedef set<CSeq_entry_Handle> TTSECont;

    TBlobCont m_Blobs;
    TTSECont  m_TSEs;
};

END_SCOPE(objects)
END_NCBI_SCOPE

#endif //__SAMPLE_EDIT_SAVER__HPP
