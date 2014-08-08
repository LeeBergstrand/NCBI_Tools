#ifndef OBJTOOLS_READERS___SOURCE_MOD_PARSER_WRAPPER__HPP
#define OBJTOOLS_READERS___SOURCE_MOD_PARSER_WRAPPER__HPP

/*  $Id: source_mod_parser_wrapper.hpp 332790 2011-08-30 15:45:46Z kornbluh $
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
 * Authors:  Michael Kornbluh
 *
 */

/// @file source_mod_parser_wrapper.hpp
/// Wraps CSourceModParser calls for CBioseq_Handles and such.

#include <corelib/tempstr.hpp>
#include <corelib/ncbistr.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CBioseq_Handle;
class CSourceModParser;

/////////////////////////////////////////////////////////////////////////////
///
/// CSourceModParserWrapper
///
/// Wraps calls to CSourceModParser which require certain extra dependencies.

class NCBI_XOBJREAD_EXPORT CSourceModParserWrapper
{
public:

    static void ExtractTitleAndApplyAllMods(CBioseq_Handle& bsh, CTempString organism = kEmptyStr);

private:
    CSourceModParserWrapper(void) { }

    static void x_ApplyAllMods(CSourceModParser &smp, CBioseq_Handle& bsh, CTempString organism = kEmptyStr);
};


//////////////////////////////////////////////////////////////////////




END_SCOPE(objects)
END_NCBI_SCOPE

#endif  /* OBJTOOLS_READERS___SOURCE_MOD_PARSER_WRAPPER__HPP */
