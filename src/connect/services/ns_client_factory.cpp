/*  $Id: ns_client_factory.cpp 193444 2010-06-03 19:28:37Z kazimird $
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
 *
 */

#include <ncbi_pch.hpp>

#include <corelib/ncbifile.hpp>

#ifdef _MSC_VER
// Disable warning C4191: 'type cast' :
//     unsafe conversion from 'ncbi::CDll::FEntryPoint'
//     to 'void (__cdecl *)(...)' [comes from plugin_manager.hpp]
#pragma warning (disable: 4191)
#endif

#include <connect/services/ns_client_factory.hpp>

BEGIN_NCBI_SCOPE

//////////////////////////////////////////////////////////////////////////////
//


//////////////////////////////////////////////////////////////////////////////
//
CNetScheduleClientFactory::CNetScheduleClientFactory(const IRegistry& reg)
: m_Registry(reg)
{
    m_PM_NetSchedule.RegisterWithEntryPoint(NCBI_EntryPoint_xnetscheduleapi);
}


CNetScheduleAPI CNetScheduleClientFactory::CreateInstance()
{
    CConfig conf(m_Registry);
    const CConfig::TParamTree* param_tree = conf.GetTree();
    const TPluginManagerParamTree* netschedule_tree =
            param_tree->FindSubNode(kNetScheduleAPIDriverName);

    if (netschedule_tree) {
        SNetScheduleAPIImpl* ret = m_PM_NetSchedule.CreateInstance(
                    kNetScheduleAPIDriverName,
                    TPMNetSchedule::GetDefaultDrvVers(),
                    netschedule_tree);

        if (ret != NULL)
            return ret;
    }

    NCBI_THROW(CNSClientFactoryException, eNSClientIsNotCreated,
        "Couldn't create NetSchedule client. Check registry.");
}


END_NCBI_SCOPE
