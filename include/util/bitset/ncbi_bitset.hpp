#ifndef UTIL___BITSET_BM__HPP
#define UTIL___BITSET_BM__HPP


/*  $Id: ncbi_bitset.hpp 177076 2009-11-24 22:14:02Z ucko $
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
 * Authors:  Anatoliy Kuznetsov
 *
 */

/// @file ncbi_bitset.hpp
/// Compressed bitset (entry point to bm.h)

#include <corelib/ncbistd.hpp>

#define BM_ASSERT _ASSERT

#ifdef HAVE_RESTRICT_CXX
#define BM_HASRESTRICT
#define BMRESTRICT NCBI_RESTRICT
#endif

#if defined(NCBI_FORCEINLINE)  &&  \
    ( !defined(NCBI_COMPILER_GCC)  ||  NCBI_COMPILER_VERSION >= 400  || \
      defined(__OPTIMIZE__))
#define BM_HASFORCEINLINE
#define BMFORCEINLINE NCBI_FORCEINLINE
#endif

#include <util/bitset/bm.h>

#endif /* UTIL___BITSET_BM__HPP */
