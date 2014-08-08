/*  $Id: distribution.hpp 159154 2009-05-01 20:26:11Z kazimird $
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
 * Authors:
 *   Dmitry Kazimirov
 *
 * File Description:
 *   Declaration of the CDiscreteDistribution class.
 *
 */

/// @file uttp.hpp
/// This file contains declaration of the CDiscreteDistribution class.

#ifndef UTIL___DISTRIBUTION__HPP
#define UTIL___DISTRIBUTION__HPP

#include "random_gen.hpp"

BEGIN_NCBI_SCOPE

/// @internal
class NCBI_XUTIL_EXPORT CInvalidParamException : public CException
{
public:
    enum EErrCode {
        eUndefined = 1,
        eNotANumber,
        eInvalidCharacter
    };

    /// Translate from an error code value to its string representation.
    virtual const char* GetErrCodeString(void) const;

    // Standard exception boilerplate code.
    NCBI_EXCEPTION_DEFAULT(CInvalidParamException, CException);
};

/// This class generates a random integer from a series of ranges
/// defined like this: 5, 6 - 9, 10 - 50
class NCBI_XUTIL_EXPORT CDiscreteDistribution
{
public:
    /// Initializes internal structures of this object using
    /// the specified string. The string must be defined
    /// in the following format:
    ///     R1, R2, ..., Rn
    /// Where R1 ... Rn - integer ranges specified as either
    /// stand-alone numbers or intervals defined as Nfrom - Nto.
    /// Example:
    ///     5, 6 - 9, 10 - 50, 65
    ///
    /// @param parameter_name
    ///     Configuration parameter name. This argument is used
    ///     only for constructing a CInvalidParamException object
    ///     should there be a format error in parameter_value.
    ///
    /// @param parameter_value
    ///     Actual initialization string, which must conform
    ///     to the format described above.
    ///
    /// @param random_gen
    ///     Random number generator for use by this object.
    void InitFromParameter(
        const char* parameter_name,
        const char* parameter_value,
        CRandom* random_gen);

    /// Returns a random value from the distribution.
    unsigned GetNextValue() const;

private:
    typedef std::pair<unsigned, unsigned> TRange;
    typedef std::vector<TRange> TRangeVector;

    const char* SkipSpaces(const char* input_string);

    CRandom* m_RandomGen;
    TRangeVector m_RangeVector;
};

END_NCBI_SCOPE

#endif /* UTIL___DISTRIBUTION__HPP */
