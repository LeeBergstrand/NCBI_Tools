#ifndef NCBI_LIMITS__HPP
#define NCBI_LIMITS__HPP

/*  $Id: ncbi_limits.hpp 209008 2010-10-22 13:30:22Z lavr $
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
 * Author:  Denis Vakatov
 *
 * File Description:
 *
 *   Temporary replacement for "numeric_limits<>".
 *   Extremely incomplete implementation,
 *
 *    only min() and max() methods for:
 *
 *      numeric_limits<char>
 *      numeric_limits<signed   char>
 *      numeric_limits<unsigned char>
 *
 *      numeric_limits<signed   short>
 *      numeric_limits<unsigned short>
 *      numeric_limits<signed   int>
 *      numeric_limits<unsigned int>
 *      numeric_limits<signed   long>
 *      numeric_limits<unsigned long>
 *
 *      numeric_limits<float>
 *      numeric_limits<double>
 *
 *      (platform-specific)
 *      numeric_limits<signed   long long>
 *      numeric_limits<unsigned long long>
 *      numeric_limits<signed   __int64>
 *      numeric_limits<unsigned __int64>
 *
 */

#include <corelib/ncbi_limits.h>
#include <corelib/ncbistl.hpp>


/** @addtogroup Portability
 *
 * @{
 */


#if defined(HAVE_LIMITS)  &&  ( !defined(NCBI_COMPILER_WORKSHOP) || NCBI_COMPILER_VERSION >= 550)
// Ideally, we would use std::numeric_limits<> whenever available.
// However, certain compiler versions leave out support for extensions such
// as long long, so we still have to use our implementation with them.
#  include <limits>
#else

BEGIN_NCBI_SCOPE

///
///  Pre-declaration of the "numeric_limits<>" template
///  Forcibly overrides (using preprocessor) the original "numeric_limits<>"!
///

#  define numeric_limits ncbi_numeric_limits
template <class T> class numeric_limits;


///
///  Auxiliary macro to implement (a limited edition of) the
///  "numeric_limits<>" template
///

#  define NCBI_NUMERIC_LIMITS(type, alias) \
  template <> \
  class numeric_limits<type> \
  { \
  public: \
      static inline type min() THROWS_NONE { return kMin_##alias; } \
      static inline type max() THROWS_NONE { return kMax_##alias; } \
  }

#  define NCBI_NUMERIC_LIMITS_UNSIGNED(type, alias) \
  template <> \
  class numeric_limits<type> \
  { \
  public: \
      static inline type min() THROWS_NONE { return 0; } \
      static inline type max() THROWS_NONE { return kMax_##alias; } \
  }


//
//  Implement (a limited edition of) the "numeric_limits<>" template
//  for various built-in types
//

NCBI_NUMERIC_LIMITS          (         char,  Char);
NCBI_NUMERIC_LIMITS          (signed   char, SChar);
NCBI_NUMERIC_LIMITS_UNSIGNED (unsigned char, UChar);

#if defined(HAVE_WCHAR_H)  &&  defined(WCHAR_MIN)
NCBI_NUMERIC_LIMITS          (wchar_t, WChar);
#endif

NCBI_NUMERIC_LIMITS          (signed   short,  Short);
NCBI_NUMERIC_LIMITS_UNSIGNED (unsigned short, UShort);

NCBI_NUMERIC_LIMITS          (signed   int,  Int);
NCBI_NUMERIC_LIMITS_UNSIGNED (unsigned int, UInt);

NCBI_NUMERIC_LIMITS          (signed   long,  Long);
NCBI_NUMERIC_LIMITS_UNSIGNED (unsigned long, ULong);

NCBI_NUMERIC_LIMITS          (float,  Float);
NCBI_NUMERIC_LIMITS          (double, Double);

#  if (SIZEOF_LONG_LONG > 0)
NCBI_NUMERIC_LIMITS          (signed   long long,  LongLong);
NCBI_NUMERIC_LIMITS_UNSIGNED (unsigned long long, ULongLong);
#  endif

#  if defined(NCBI_INT8_IS_INT64)
NCBI_NUMERIC_LIMITS          (signed   __int64,  Int64);
NCBI_NUMERIC_LIMITS_UNSIGNED (unsigned __int64, UInt64);
#  endif


END_NCBI_SCOPE

#endif // !HAVE_LIMITS  ||  NCBI_COMPILER_WORKSHOP


BEGIN_NCBI_SCOPE

/// Generic template to get STD limits by a variable.
/// Typical use:
/// <pre>
///  int a = 10; 
///  get_limits(a).max();
/// </pre>
/// @note 
///   Causes a compile-time failure if used
///   instead of the specialized implementations.
template<typename T> 
inline numeric_limits<T> get_limits(const T&)
{
    typename T::TypeIsNotSupported tmp; 
    return numeric_limits<T>();
}

/// Macro to declare specilized get_limits
#  define NCBI_GET_NUMERIC_LIMITS(type) \
    EMPTY_TEMPLATE \
    inline numeric_limits<type> get_limits(const type&) \
        { return numeric_limits<type>(); }

NCBI_GET_NUMERIC_LIMITS(         char)
NCBI_GET_NUMERIC_LIMITS(signed   char)
NCBI_GET_NUMERIC_LIMITS(unsigned char)

NCBI_GET_NUMERIC_LIMITS(signed   short)
NCBI_GET_NUMERIC_LIMITS(unsigned short)

NCBI_GET_NUMERIC_LIMITS(signed   int)
NCBI_GET_NUMERIC_LIMITS(unsigned int)

NCBI_GET_NUMERIC_LIMITS(signed   long)
NCBI_GET_NUMERIC_LIMITS(unsigned long)

NCBI_GET_NUMERIC_LIMITS(float)
NCBI_GET_NUMERIC_LIMITS(double)

#  if (SIZEOF_LONG_LONG > 0)
NCBI_GET_NUMERIC_LIMITS(signed   long long)
NCBI_GET_NUMERIC_LIMITS(unsigned long long)
#  endif

#  if defined(NCBI_INT8_IS_INT64)
NCBI_GET_NUMERIC_LIMITS(signed   __int64)
NCBI_GET_NUMERIC_LIMITS(unsigned __int64)
#  endif


END_NCBI_SCOPE

/* @} */

#endif /* NCBI_LIMITS__HPP */
