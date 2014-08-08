/*  $Id: idmapper_builtin.cpp 170010 2009-09-08 14:24:26Z dicuccio $
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
 * Author:  Frank Ludwig
 *
 * File Description:
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbiapp.hpp>

// Objects includes
#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/Seq_loc.hpp>

#include <objtools/readers/reader_exception.hpp>
#include <objtools/readers/line_error.hpp>
#include <objtools/readers/error_container.hpp>
#include <objtools/readers/idmapper.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);


extern const char* sc_BuiltinConfig;

CIdMapperBuiltin::CIdMapperBuiltin(const std::string& strContext,
                                   bool bInvert,
                                   IErrorContainer* pErrors)
    : CIdMapperConfig(strContext, bInvert, pErrors)
{
    Initialize();
}


void CIdMapperBuiltin::Initialize()
{
    CNcbiIstrstream is(sc_BuiltinConfig, strlen(sc_BuiltinConfig));
    CIdMapperConfig::Initialize(is);
}



//////////////////////////////////////////////////////////////////////////////

#include "idmapper_builtin_config.inl"

END_NCBI_SCOPE

