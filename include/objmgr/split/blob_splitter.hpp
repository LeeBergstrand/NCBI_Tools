#ifndef NCBI_OBJMGR_SPLIT_BLOB_SPLITTER__HPP
#define NCBI_OBJMGR_SPLIT_BLOB_SPLITTER__HPP

/*  $Id: blob_splitter.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author:  Eugene Vasilchenko
*
* File Description:
*   Application for splitting blobs withing ID1 cache
*
* ===========================================================================
*/


#include <corelib/ncbistd.hpp>

#include <objmgr/split/blob_splitter_params.hpp>
#include <objmgr/split/split_blob.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CSeq_entry;

class NCBI_ID2_SPLIT_EXPORT CBlobSplitter
{
public:
    CBlobSplitter(const SSplitterParams& params)
        : m_Params(params)
        {
        }

    bool Split(const CSeq_entry& entry);
    
    const CSplitBlob& GetBlob(void) const
        {
            return m_SplitBlob;
        }

private:
    SSplitterParams m_Params;

    CSplitBlob m_SplitBlob;
};

END_SCOPE(objects)
END_NCBI_SCOPE

#endif//NCBI_OBJMGR_SPLIT_BLOB_SPLITTER__HPP
