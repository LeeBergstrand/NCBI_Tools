/*  $Id: plugin_manager_store.cpp 113313 2007-11-01 14:54:51Z ivanov $
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
 * File Description:  Object Store implementations
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/plugin_manager_store.hpp>
#include <corelib/obj_store.hpp>
#include <corelib/ncbi_safe_static.hpp>

#define NCBI_USE_ERRCODE_X   Corelib_PluginMgr


BEGIN_NCBI_SCOPE


SSystemFastMutex& CPluginManagerGetterImpl::GetMutex(void)
{
    DEFINE_STATIC_FAST_MUTEX(s_Mutex);
    return s_Mutex;
}


typedef CPluginManagerGetterImpl::TKey TObjectStoreKey;
typedef CPluginManagerGetterImpl::TObject TObjectStoreObject;
typedef CReverseObjectStore<TObjectStoreKey, TObjectStoreObject> TObjectStore;

static TObjectStore& GetObjStore(void)
{
    static CSafeStaticPtr<TObjectStore> s_obj_store;
    return s_obj_store.Get();
}


CPluginManagerGetterImpl::TObject*
CPluginManagerGetterImpl::GetBase(const TKey& key)
{
    return GetObjStore().GetObject(key);
}


void CPluginManagerGetterImpl::PutBase(const TKey& key,
                                       TObject* pm)
{
    GetObjStore().PutObject(key, pm);
}


void CPluginManagerGetterImpl::ReportKeyConflict(const TKey& key,
                                                 const TObject* old_pm,
                                                 const type_info& new_pm_type)
{
    ERR_POST_X(4, Fatal << "Plugin Manager conflict, key=\"" << key << "\", "
                  "old type=" << typeid(*old_pm).name() << ", "
                  "new type=" << new_pm_type.name());
}


END_NCBI_SCOPE
