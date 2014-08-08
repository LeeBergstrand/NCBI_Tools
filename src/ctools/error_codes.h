#ifndef CTOOLS___ERROR_CODES__H
#define CTOOLS___ERROR_CODES__H

/*  $Id: error_codes.h 112625 2007-10-22 13:52:23Z ivanovp $
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

/** @file error_codes.hpp
 *  Definition of all error codes used in C code of ctools library
 *  (xctools.lib).
 */


#include "../connect/ncbi_priv.h"


NCBI_C_DEFINE_ERRCODE_X(Ctools_ASN,  901,  1);


#endif  /* CTOOLS___ERROR_CODES__H */
