#ifndef PROJECT_TREE_BULDER__MSVC_DLLS_INDO__HPP
#define PROJECT_TREE_BULDER__MSVC_DLLS_INDO__HPP

/* $Id: msvc_dlls_info.hpp 125101 2008-04-21 13:42:57Z gouriano $
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

#include "msvc_prj_utils.hpp"
#include "ptb_registry.hpp"
#include <list>
#include <set>
#include "proj_tree.hpp"

#include <corelib/ncbienv.hpp>

BEGIN_NCBI_SCOPE


void FilterOutDllHostedProjects(const CProjectItemsTree& tree_src, 
                                CProjectItemsTree*       tree_dst);

void AnalyzeDllData(CProjectItemsTree& tree_src);

string GetDllHost(const CProjectItemsTree& tree, const string& lib);

void CreateDllBuildTree(const CProjectItemsTree& tree_src, 
                        CProjectItemsTree*       tree_dst);


void CreateDllsList(const CProjectItemsTree& tree_src,
                    list<string>*            dll_ids);


void CollectDllsDepends(const CProjectItemsTree& tree_src, 
                        const list<string>& dll_ids,
                        list<string>*       dll_depends_ids);

END_NCBI_SCOPE

#endif //PROJECT_TREE_BULDER__MSVC_DLLS_INDO__HPP
