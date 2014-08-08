#ifndef _RSMETA_IMPL_HPP_
#define _RSMETA_IMPL_HPP_

/* $Id: rsmeta_impl.hpp 119133 2008-02-05 21:14:42Z ssikorsk $
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
* File Name:  $Id: rsmeta_impl.hpp 119133 2008-02-05 21:14:42Z ssikorsk $
*
* Author:  Michael Kholodov
*   
* File Description:  Resultset metadata implementation
*
*
*/

#include <dbapi/dbapi.hpp>
#include "dbexception.hpp"
#include "active_obj.hpp"

#include <vector>

BEGIN_NCBI_SCOPE

class CDB_Result;

class CResultSetMetaData : public CActiveObject, 
			   public IResultSetMetaData
{

public:
  CResultSetMetaData(CDB_Result *rs);

  virtual ~CResultSetMetaData();
  
  virtual unsigned int GetTotalColumns() const;
  virtual EDB_Type GetType(const CDBParamVariant& param) const;
  virtual int GetMaxSize(const CDBParamVariant& param) const;
  virtual string GetName(const CDBParamVariant& param) const;
  virtual CDBParams::EDirection GetDirection(const CDBParamVariant& param) const;

  // Interface IEventListener implementation
  virtual void Action(const CDbapiEvent& e);

private:
  unsigned int FindParamPosInternal(const string& name) const;

  struct SColMetaData 
  {
    SColMetaData(const string& name,
		 EDB_Type type,
		 int maxSize)
      : m_name(name), 
      m_type(type), 
      m_maxSize(maxSize) 
      {
      }

    string   m_name;
    EDB_Type m_type;
    int      m_maxSize;
  };
  
  vector<SColMetaData> m_colInfo;

};


END_NCBI_SCOPE

#endif // _RSMETA_IMPL_HPP_
