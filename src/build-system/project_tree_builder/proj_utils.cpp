/* $Id: proj_utils.cpp 122761 2008-03-25 16:45:09Z gouriano $
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
 * Author:  Viatcheslav Gorelenkov
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbifile.hpp>
#include "proj_utils.hpp"

BEGIN_NCBI_SCOPE

string ParentDir(const string& dir_abs)
{
    string parent_dir_abs = CDirEntry::AddTrailingPathSeparator(dir_abs);
    parent_dir_abs += "..";
    parent_dir_abs = CDirEntry::AddTrailingPathSeparator(parent_dir_abs);
    parent_dir_abs = CDirEntry::NormalizePath(parent_dir_abs);

    return CDirEntry::AddTrailingPathSeparator(parent_dir_abs);
}

END_NCBI_SCOPE
