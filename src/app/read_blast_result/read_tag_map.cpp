/*  $Id: read_tag_map.cpp 194721 2010-06-16 19:30:03Z badrazat $
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
* Author: Azat Badretdin
*
* File Description:
*
* ===========================================================================
*/
#include <ncbi_pch.hpp>
#include "read_blast_result.hpp"

int CReadBlastApp::ReadTagMap(const char *file)
{
  ifstream in(file);
 string key, value;
  while(!in.eof())
    {
    in >> key >> value;
    if(!in.eof()) 
      {
      if(m_tagmap.find(key) != m_tagmap.end())
        {
        NcbiCerr << "ReadTagMap("<<file<<"): ERROR: m_tagmap["<<key<<"]="<<m_tagmap[key]<<" has been already defined, discarding new value " << value << NcbiEndl;
        }
      else m_tagmap[key]=value;
      }
    }

  return m_tagmap.size();
}


