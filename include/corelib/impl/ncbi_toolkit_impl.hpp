#ifndef CORELIB___NCBI_TOOLKIT_IMPL__HPP
#define CORELIB___NCBI_TOOLKIT_IMPL__HPP

/*  $Id: ncbi_toolkit_impl.hpp 309867 2011-06-28 18:43:15Z gouriano $
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
 * Authors:  Andrei Gourianov, Denis Vakatov
 *
 */

/// @file ncbi_toolkit_impl.hpp

/** @addtogroup AppFramework
 *
 * @{
 */

#include <string>

namespace ncbi {



// Forward declarations
class  INcbiToolkit_LogHandler;
struct SDiagMessage;
class  CNcbiApplication;

// Single/Multi-byte characters
#if defined(_MSC_VER) && defined(_UNICODE)
typedef wchar_t TNcbiToolkit_XChar;
#else
typedef char    TNcbiToolkit_XChar;
#endif

// Export/import specifications
#if defined(_MSC_VER) && defined(_USRDLL)
#  ifdef NCBI_XNCBI_EXPORTS
#    define NCBI_TOOLKIT_EXPORT __declspec(dllexport)
#  else
#    define NCBI_TOOLKIT_EXPORT __declspec(dllimport)
#  endif
#else
#  define NCBI_TOOLKIT_EXPORT
#endif



/////////////////////////////////////////////////////////////////////////////
/// Provide means of creating custom CNcbiApplication object -- to use the
/// latter instead of "dummy" NCBI application.
/// @note
///  It is an esoteric feature that is very rarely used.

typedef CNcbiApplication* (*FNcbiApplicationFactory)(void);

void NCBI_TOOLKIT_EXPORT NcbiToolkit_RegisterNcbiApplicationFactory
    (FNcbiApplicationFactory f);

} /* namespace ncbi */



#endif  /* CORELIB___NCBI_TOOLKIT_IMPL__HPP */
