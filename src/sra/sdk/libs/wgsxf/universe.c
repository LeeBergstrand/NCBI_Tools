/*===========================================================================
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
*/

#include <sra/xf.h>
#include <kfs/dyload.h>
#include <stdlib.h>

#define COMMON_SYMBOLS(X) X(NCBI_WGS_build_read_type) \
  X(NCBI_WGS_build_scaffold_qual) X(NCBI_WGS_build_scaffold_read)

#define ALL_SYMBOLS(X) COMMON_SYMBOLS(X)

#define AS_DECL(sym) extern void sym();
ALL_SYMBOLS(AS_DECL)

#define AS_VALUE(sym) sym,
static const xf_function_t s_wgsxf_functions[] = { ALL_SYMBOLS(AS_VALUE) NULL };

#define FOR_KDYLD(sym) KDyldRegisterBuiltin ( #sym, sym );

LIB_EXPORT const xf_function_t * CC register_wgsxf_functions(void)
{
    COMMON_SYMBOLS(FOR_KDYLD)
    return s_wgsxf_functions;
}

#ifdef WUNIVERSE
LIB_EXPORT const xf_function_t * CC register_wwgsxf_functions(void)
{
    ALL_SYMBOLS(FOR_KDYLD)
    return s_wgsxf_functions;
}
#endif
