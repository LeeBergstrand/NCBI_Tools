/* $Id: interfaces_p.hpp 173033 2009-10-14 13:16:54Z ivanovp $
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
 * Author:  Victor Sapojnikov
 *
 * File Description:
 *   Some Sybase dblib constants, types and structure members
 *   translated to Microsoft-compatible ones.
 *
 */

#ifndef DBAPI_DRIVER_DBLIB___INTERFACES_P__HPP
#define DBAPI_DRIVER_DBLIB___INTERFACES_P__HPP


#define DBDATETIME4_days(x) ((x)->days)
#define DBDATETIME4_mins(x) ((x)->minutes)
#define DBNUMERIC_val(x) ((x)->array)

#endif  /* DBAPI_DRIVER_DBLIB___INTERFACES_P__HPP */

