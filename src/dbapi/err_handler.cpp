/* $Id: err_handler.cpp 343796 2011-11-09 18:12:57Z ivanovp $
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
* File Name:  $Id: err_handler.cpp 343796 2011-11-09 18:12:57Z ivanovp $
*
* Author:  Michael Kholodov
*
* File Description:  DataSource implementation
*/

#include <ncbi_pch.hpp>
#include "err_handler.hpp"

BEGIN_NCBI_SCOPE

CToMultiExHandler::CToMultiExHandler()
    : m_ex( new CDB_MultiEx( DIAG_COMPILE_INFO, 0 ) )
{
}

CToMultiExHandler::~CToMultiExHandler()
{
}

bool CToMultiExHandler::HandleIt(CDB_Exception* ex)
{
    m_ex->Push(*ex);
    _TRACE("CToMultiExHandler::HandleIt(): exception received");

    return true;
}

bool CToMultiExHandler::HandleAll(const TExceptions& exceptions)
{
    ITERATE(TExceptions, it, exceptions) {
        m_ex->Push(**it);
    }
    return true;
}

END_NCBI_SCOPE
