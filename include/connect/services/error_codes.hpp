#ifndef CONNECT_SERVICES___ERROR_CODES__HPP
#define CONNECT_SERVICES___ERROR_CODES__HPP

/*  $Id: error_codes.hpp 383181 2012-12-12 16:51:50Z kazimird $
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
 * Authors:  Pavel Ivanov
 *
 */

/// @file error_codes.hpp
/// Definition of all error codes used in connect services library
/// (xconnserv.lib and others).
///


#include <corelib/ncbidiag.hpp>


BEGIN_NCBI_SCOPE


NCBI_DEFINE_ERRCODE_X(ConnServ_WorkerNode,  401, 69);
NCBI_DEFINE_ERRCODE_X(ConnServ_NetCache,    402, 12);
NCBI_DEFINE_ERRCODE_X(ConnServ_NetSchedule, 403, 13);
NCBI_DEFINE_ERRCODE_X(ConnServ_Remote,      404, 18);
NCBI_DEFINE_ERRCODE_X(ConnServ_Connection,  405, 10);
NCBI_DEFINE_ERRCODE_X(ConnServ_ReadWrite,   406,  2);


END_NCBI_SCOPE


#endif  /* CONNECT_SERVICES___ERROR_CODES__HPP */
