#ifndef SERIALASNDEF__HPP
#define SERIALASNDEF__HPP

/*  $Id: serialasndef.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author: Eugene Vasilchenko
*
* File Description:
*   !!! PUT YOUR DESCRIPTION HERE !!!
*/

#include <corelib/ncbistd.hpp>


/** @addtogroup TypeInfoC
 *
 * @{
 */


struct asnio;
struct asntype;

BEGIN_NCBI_SCOPE

#ifndef ASNCALL
# ifdef HAVE_WINDOWS_H
#  define ASNCALL __stdcall
# else
#  define ASNCALL
# endif
#endif

typedef TObjectPtr (ASNCALL*TAsnNewProc)(void);
typedef TObjectPtr (ASNCALL*TAsnFreeProc)(TObjectPtr);
typedef TObjectPtr (ASNCALL*TAsnReadProc)(asnio*, asntype*);
typedef unsigned char (ASNCALL*TAsnWriteProc)(TObjectPtr, asnio*, asntype*);

NCBI_XSERIAL_EXPORT
TTypeInfo COctetStringTypeInfoGetTypeInfo(void);

NCBI_XSERIAL_EXPORT
TTypeInfo CAutoPointerTypeInfoGetTypeInfo(TTypeInfo type);

NCBI_XSERIAL_EXPORT
TTypeInfo CSetOfTypeInfoGetTypeInfo(TTypeInfo type);

NCBI_XSERIAL_EXPORT
TTypeInfo CSequenceOfTypeInfoGetTypeInfo(TTypeInfo type);

NCBI_XSERIAL_EXPORT
TTypeInfo COldAsnTypeInfoGetTypeInfo(const string& name,
                                     TAsnNewProc newProc,
                                     TAsnFreeProc freeProc,
                                     TAsnReadProc readProc,
                                     TAsnWriteProc writeProc);

END_NCBI_SCOPE


/* @} */

#endif  /* SERIALASNDEF__HPP */
