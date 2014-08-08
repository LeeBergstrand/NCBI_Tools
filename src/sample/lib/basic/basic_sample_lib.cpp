/*  $Id: basic_sample_lib.cpp 163861 2009-06-19 16:08:28Z ucko $
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

#include <ncbi_pch.hpp>
#include <sample/lib/basic/sample_lib_basic.hpp>

BEGIN_NCBI_SCOPE

CSampleLibraryObject::CSampleLibraryObject(void)
{
#if defined(NCBI_OS_UNIX)
    m_EnvPath = "PATH";
    m_EnvSeparator = ":";
#else
    m_EnvPath = "Path";
    m_EnvSeparator = ";";
#endif
}

CSampleLibraryObject::~CSampleLibraryObject(void)
{
}

bool CSampleLibraryObject::FindInPath(
    list<string>& found, const string& mask)
{
    // Get PATH
    const string path =
        CNcbiApplication::Instance()->GetEnvironment().Get(m_EnvPath);
    list<string> folders;
    // Get PATH folders
    NStr::Split( path, m_EnvSeparator, folders );
    // find file(s)
    ITERATE( list<string>, f, folders ) {
        CDir::TEntries ent = CDir( *f ).GetEntries( mask );
        ITERATE( CDir::TEntries, e, ent ) {
            found.push_back( (*e)->GetPath() );
        }
    }
    return !found.empty();
}

END_NCBI_SCOPE

