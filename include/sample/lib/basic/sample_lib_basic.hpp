#ifndef SAMPLELIB___OBJECT__HPP
#define SAMPLELIB___OBJECT__HPP

/*  $Id: sample_lib_basic.hpp 157249 2009-04-14 13:03:44Z gouriano $
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
 * Authors:  Andrei Gourianov
 *
 * File Description:
 *   Sample library
 *
 */

// NOTE: Put correct file name and description here:
/// @file basic_sample_lib.hpp
/// The NCBI C++ sample library


#include <corelib/ncbiapp.hpp>
#include <corelib/ncbifile.hpp>
#include <corelib/ncbistr.hpp>

BEGIN_NCBI_SCOPE

// NOTE: Put correct group here:
/** @addtogroup Sample
 *
 * @{
 */

class CSampleLibraryObject
{
public:
    CSampleLibraryObject(void);
    virtual ~CSampleLibraryObject(void);
    bool FindInPath(list<string>& found, const string& mask);
private:
    string m_EnvPath;
    string m_EnvSeparator;
};


/* @} */

END_NCBI_SCOPE
#endif  //SAMPLELIB___OBJECT__HPP

