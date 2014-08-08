/*  $Id: regexp.cpp 398236 2013-05-03 18:54:43Z rafanovi $
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
 * Author: Vladimir Ivanov, Clifford Clausen
 * File Description:
 *     C++ wrappers for Perl Compatible Regular Expression (pcre) library
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbi_limits.h>
#include <corelib/ncbistl.hpp>
#include <util/xregexp/regexp.hpp>
#include <pcre.h>

#include <memory>
#include <stdlib.h>

BEGIN_NCBI_SCOPE


//////////////////////////////////////////////////////////////////////////////
//
//  CRegexp
//

// Regular expression meta characters
static char s_Special[] = ".?*+$^[](){}/\\|-";


// Macro to check bits
#define F_ISSET(flags, mask) ((flags & (mask)) == (mask))

// Auxiliary functions to convert CRegexp flags to real flags.
static int s_GetRealCompileFlags(CRegexp::TCompile compile_flags)
{
    int flags = 0;

    if ( !compile_flags  &&
         !F_ISSET(compile_flags, CRegexp::fCompile_default )) {
        NCBI_THROW(CRegexpException, eBadFlags,
                   "Bad regular expression compilation flags");
    }
    if ( F_ISSET(compile_flags, CRegexp::fCompile_ignore_case) ) {
        flags |= PCRE_CASELESS;
    }
    if ( F_ISSET(compile_flags, CRegexp::fCompile_dotall) ) {
        flags |= PCRE_DOTALL;
    }
    if ( F_ISSET(compile_flags, CRegexp::fCompile_newline) ) {
        flags |= PCRE_MULTILINE;
    }
    if ( F_ISSET(compile_flags, CRegexp::fCompile_ungreedy) ) {
        flags |= PCRE_UNGREEDY;
    }
    if ( F_ISSET(compile_flags, CRegexp::fCompile_extended) ) {
        flags |= PCRE_EXTENDED;
    }
    return flags;
}

static int s_GetRealMatchFlags(CRegexp::TMatch match_flags)
{
    int flags = 0;

    if ( !match_flags  &&
         !F_ISSET(match_flags, CRegexp::fMatch_default) ) {
        NCBI_THROW(CRegexpException, eBadFlags,
                   "Bad regular expression match flags");
    }
    if ( F_ISSET(match_flags, CRegexp::fMatch_not_begin) ) {
        flags |= PCRE_NOTBOL;
    }
    if ( F_ISSET(match_flags, CRegexp::fMatch_not_end) ) {
        flags |= PCRE_NOTEOL;
    }
    return flags;
}


CRegexp::CRegexp(CTempStringEx pattern, TCompile flags)
    : m_PReg(NULL), m_Extra(NULL), m_NumFound(0)
{
    Set(pattern, flags);
}


CRegexp::~CRegexp()
{
    (*pcre_free)(m_PReg);
    (*pcre_free)(m_Extra);
}


void CRegexp::Set(CTempStringEx pattern, TCompile flags)
{
    if (m_PReg != NULL) {
        (*pcre_free)(m_PReg);
    }
    const char *err;
    int err_offset;
    int x_flags = s_GetRealCompileFlags(flags);

    if ( pattern.HasZeroAtEnd() ) {
        m_PReg = pcre_compile(pattern.data(), x_flags, &err, &err_offset, NULL);
    } else {
        m_PReg = pcre_compile(string(pattern).c_str(), x_flags, &err, &err_offset, NULL);
    }
    if (m_PReg == NULL) {
        NCBI_THROW(CRegexpException, eCompile, "Compilation of the pattern '" +
                   string(pattern) + "' failed: " + err);
    }
    m_Extra = pcre_study((pcre*)m_PReg, 0, &err);
}


// @deprecated
void CRegexp::GetSub(CTempString str, size_t idx, string& dst) const
{
    int start = m_Results[2 * idx];
    int end   = m_Results[2 * idx + 1];

    if ((int)idx >= m_NumFound  ||  start == -1  ||  end == -1) {
        dst.erase();
    } else {
        dst.assign(str.data() + start, end - start);
    }
}


CTempString CRegexp::GetSub(CTempString str, size_t idx) const
{
    int start = m_Results[2 * idx];
    int end   = m_Results[2 * idx + 1];

    if ((int)idx >= m_NumFound  ||  start == -1  ||  end == -1) {
        return CTempString();
    } else {
        return CTempString(str.data() + start, end - start);
    }
}


CTempString CRegexp::GetMatch(CTempString str, size_t offset, size_t idx,
                              TMatch flags, bool noreturn)
{
    int x_flags = s_GetRealMatchFlags(flags);
    m_NumFound = pcre_exec((pcre*)m_PReg, (pcre_extra*)m_Extra, str.data(),
                           (int)str.length(), (int)offset,
                           x_flags, m_Results,
                           (int)(kRegexpMaxSubPatterns +1) * 3);
    if ( noreturn ) {
        return CTempString();
    }
    return GetSub(str, idx);
}


bool CRegexp::IsMatch(CTempString str, TMatch flags)
{
    int x_flags = s_GetRealMatchFlags(flags);
    m_NumFound = pcre_exec((pcre*)m_PReg, (pcre_extra*)m_Extra, str.data(),
                           (int)str.length(), 0, x_flags, m_Results,
                           (int)(kRegexpMaxSubPatterns +1) * 3);
    return m_NumFound > 0;
}


string CRegexp::Escape(CTempString str)
{
    // Find first special character
    SIZE_TYPE prev = 0;
    SIZE_TYPE pos = str.find_first_of(s_Special, prev);
    if ( pos == NPOS ) {
        // All characters are good - return original string
        return str;
    }
    CNcbiOstrstream out;
    do {
        // Write first good characters in one chunk
        out.write(str.data() + prev, pos - prev);
        // Escape char
        out.put('\\');
        out.put(str[pos]);
        // Find next
        prev = pos + 1;
        pos = str.find_first_of(s_Special, prev);
    } while (pos != NPOS);

    // Write remaining part of the string
    out.write(str.data() + prev, str.length() - prev);
    // Return encoded string
	return CNcbiOstrstreamToString(out);
}


string CRegexp::WildcardToRegexp(CTempString mask)
{
    // Find first special character
    SIZE_TYPE prev = 0;
    SIZE_TYPE pos = mask.find_first_of(s_Special, prev);
    if ( pos == NPOS ) {
        // All characters are good - return original string
        return mask;
    }
    CNcbiOstrstream out;
    do {
        // Write first good characters in one chunk
        out.write(mask.data() + prev, pos - prev);
        // Convert or escape found character
        if (mask[pos] == '*') {
            out.put('.');
            out.put(mask[pos]);
        } else if (mask[pos] == '?') {
            out.put('.');
        } else {
            // Escape character
            out.put('\\');
            out.put(mask[pos]);
        }
        // Find next
        prev = pos + 1;
        pos = mask.find_first_of(s_Special, prev);
    } while (pos != NPOS);

    // Write remaining part of the string
    out.write(mask.data() + prev, mask.length() - prev);
    // Return encoded string
	return CNcbiOstrstreamToString(out);
}


//////////////////////////////////////////////////////////////////////////////
//
//  CRegexpUtil
//

CRegexpUtil::CRegexpUtil(CTempString str) 
    : m_Delimiter("\n")
{
    Reset(str);
    return;
}


void CRegexpUtil::SetRange(
    CTempStringEx addr_start,
    CTempStringEx addr_end,
    CTempString   delimiter)
{
    m_RangeStart = addr_start;
    m_RangeEnd   = addr_end;
    m_Delimiter  = delimiter;
    x_Divide(delimiter);
}


size_t CRegexpUtil::Replace(
    CTempStringEx     search,
    CTempString       replace,
    CRegexp::TCompile compile_flags,
    CRegexp::TMatch   match_flags,
    size_t            max_replace)
{
    if ( search.empty() ) {
        return 0;
    }
    size_t n_replace = 0;

    // Join string to parts with delimiter
    x_Join();

    // Compile regular expression.
    CRegexp re(search, compile_flags);
    size_t  start_pos = 0;

    for (size_t count = 0; !(max_replace && count >= max_replace); count++) {

        // Match pattern.
        re.GetMatch(m_Content, (int)start_pos, 0, match_flags, true);
        int num_found = re.NumFound();
        if (num_found <= 0) {
            break;
        }

        // Substitute all subpatterns "$<digit>" to values in the "replace" string
        const int* result;
        string     x_replace(replace.data(), replace.length());
        size_t     pos = 0;

        for (;;) {
            // Find "$"
            pos = x_replace.find("$", pos);
            if (pos == NPOS) {
                break;
            }
            // Try to convert string after the "$" to number
            errno = 0;
            const char* startptr = x_replace.c_str() + pos + 1;
            char* endptr = 0;
            long value = strtol(startptr, &endptr, 10);

            if ( errno  ||  endptr == startptr  ||  !endptr  ||
                 value < kMin_Int  ||  value > kMax_Int) {
                // Format error, skip single "$".
                pos++;
                continue;

            }
            int n = (int)value;

            // Get subpattern value
            CTempString subpattern;
            if ( n > 0  &&  n < num_found ) {
                result = re.GetResults(n);
                if (result[0] >= 0  &&  result[1] >= 0) {
                    subpattern.assign(m_Content.data() + result[0], result[1] - result[0]);
                }
            }

            // Check braces {$...}
            size_t sp_start = pos;
            size_t sp_end   = endptr - x_replace.c_str();
            if ( sp_start > 0  &&  x_replace[sp_start-1] == '{') {
                sp_start--;
                if ( sp_end <  x_replace.length()  &&
                     x_replace[sp_end] == '}') {
                    sp_end++;
                } else {
                    // Format error -- missed closed brace.
                    sp_start++;
                }
            }
            // Replace $n with subpattern value.
            x_replace.replace(sp_start, sp_end - sp_start, subpattern.data(), subpattern.length());
            pos += subpattern.length();
        }

        // Replace pattern with "x_replace".
        result = re.GetResults(0);
        m_Content.replace(result[0], result[1] - result[0], x_replace);
        n_replace++;
        start_pos = result[0] + x_replace.length();
        // Guard against endless loop when regular expression
        // can match the empty string.
        if ( !x_replace.length() &&  result[0] == result[1] )
            start_pos++;
    }
    return n_replace;
}


size_t CRegexpUtil::ReplaceRange(
    CTempStringEx       search,
    CTempString         replace,
    CRegexp::TCompile   compile_flags,
    CRegexp::TMatch     match_flags,
    CRegexpUtil::ERange process_inside,
    size_t              max_replace
    )
{
    if ( search.empty() ) {
        return 0;
    }

    // Number of replaced strings
    size_t n_replace = 0;

    // Split source string to parts by delimiter
    x_Divide();

    // Flag which denote that current line is inside "range"
    bool inside = m_RangeStart.empty();
    bool close_inside = false;

    NON_CONST_ITERATE (list<string>, i, m_ContentList) {
        // Get new line
        CTempString line(*i);

        // Check for beginning of block [addr_re_start:addr_re_end]
        if ( !inside  &&  !m_RangeStart.empty() ) {
            CRegexp re(m_RangeStart);
            re.GetMatch(line, 0, 0, CRegexp::fMatch_default, true);
            inside = (re.NumFound() > 0);
        } else {
            inside = true;
        }

        // Two addresses were specified?
        // Check for ending of block [addr_re_start:addr_re_end]
        // before doing any replacements in the string
        if ( inside  &&  !m_RangeEnd.empty() ) {
            CRegexp re(m_RangeEnd);
            re.GetMatch(line, 0, 0, CRegexp::fMatch_default, true);
            close_inside = (re.NumFound() > 0);
        } else {
            // One address -- process one current string only
            close_inside = true;
        }

        // Process current line
        if ( (inside   &&  process_inside == eInside)  ||
             (!inside  &&  process_inside == eOutside) ) {
            CRegexpUtil re(line);
            n_replace += re.Replace(search, replace,
                                    compile_flags, match_flags, max_replace);
            *i = re; // invalidates CTempString line
        }

        // Finish processing block?
        if ( close_inside ) {
            inside = false;
        }
    }

    return n_replace;
}

void CRegexpUtil::x_Divide(CTempString delimiter)
{
    /// Join substrings back to entire string if divided
    if ( m_IsDivided  ) {
        if ( delimiter == m_Delimiter ) {
            return;
        }
        x_Join();
    }
    m_ContentList.clear();

    // Split source string to parts by new delimiter
    size_t pos;
    size_t start_pos = 0;
    for (;;) {
        pos = m_Content.find(delimiter.data(), start_pos, delimiter.length());
        if (pos == NPOS) {
            m_ContentList.push_back(m_Content.substr(start_pos));
            break;
        } else {
            m_ContentList.push_back(m_Content.substr(start_pos, pos - start_pos));
            start_pos = pos + delimiter.length();
        }
    }
    m_IsDivided = true;
    // Save delimiter for consecutive joining
    m_Delimiter = delimiter;
}


void CRegexpUtil::x_Join(void)
{
    if ( m_IsDivided ) {
        m_Content = NStr::Join(m_ContentList, m_Delimiter);
        m_IsDivided = false;
    }
}

const char* CRegexpException::GetErrCodeString(void) const
{
    switch ( GetErrCode() ) {
    case eCompile:    return "eCompile";
    case eBadFlags:   return "eBadFlags";
    default:          return CException::GetErrCodeString();
    }
}


END_NCBI_SCOPE
