 /*  $Id: gff3_reader.hpp 340591 2011-10-11 16:04:11Z ludwigf $
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
 * Author: Frank Ludwig
 *
 * File Description:
 *   BED file reader
 *
 */

#ifndef OBJTOOLS_READERS___GFF3_READER__HPP
#define OBJTOOLS_READERS___GFF3_READER__HPP

#include <corelib/ncbistd.hpp>
#include <objects/seq/Annotdesc.hpp>
#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqfeat/Cdregion.hpp>

#include <objtools/readers/reader_base.hpp>
#include <objtools/readers/gff2_reader.hpp>
//#include <objtools/readers/gff3_data.hpp>

BEGIN_NCBI_SCOPE

BEGIN_SCOPE(objects) // namespace ncbi::objects::

class CGFFReader;
class CGff3ReadRecord;
class SRecord;

//  ============================================================================
class CGff3ReadRecord
//  ============================================================================
    : public CGff2Record
{
public:
    CGff3ReadRecord() {};
    ~CGff3ReadRecord() {};

protected:
    string x_NormalizedAttributeKey(
        const string& );
};

//  ----------------------------------------------------------------------------
class NCBI_XOBJREAD_EXPORT CGff3Reader
//  ----------------------------------------------------------------------------
    : public CGff2Reader
{
public:
    //
    //  object management:
    //
public:
    CGff3Reader(
        unsigned int uFlags,
        const string& name = "",
        const string& title = "" );

    virtual ~CGff3Reader();

protected:
    virtual CGff2Record* x_CreateRecord() { return new CGff3ReadRecord(); };    

    virtual bool x_UpdateAnnot(
        const CGff2Record&,
        CRef< CSeq_annot > );

    virtual bool x_UpdateFeatureCds(
        const CGff2Record&,
        CRef<CSeq_feat>);
};

END_SCOPE(objects)
END_NCBI_SCOPE

#endif // OBJTOOLS_READERS___GFF3_READER__HPP
