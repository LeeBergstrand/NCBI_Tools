/*  $Id: gff3_alignment_data.hpp 362195 2012-05-08 14:24:15Z ludwigf $
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
 *   GFF3 transient data structures
 *
 */

#ifndef OBJTOOLS_WRITERS___GFF3ALIGNMENTDATA__HPP
#define OBJTOOLS_WRITERS___GFF3ALIGNMENTDATA__HPP

#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objtools/alnmgr/alnmap.hpp>
#include <objtools/writers/gff2_write_data.hpp>

BEGIN_NCBI_SCOPE
BEGIN_objects_SCOPE // namespace ncbi::objects::

//  ----------------------------------------------------------------------------
class CGffAlignmentRecord
//  ----------------------------------------------------------------------------
    : public CGffWriteRecord
{
public:
    CGffAlignmentRecord(
            CGffFeatureContext& fc,
            unsigned int uFlags =0,
            unsigned int uRecordId =0 ):
        CGffWriteRecord(fc, NStr::UIntToString(uRecordId)),
        m_uFlags(uFlags),
        m_bIsTrivial(true) 
    {
        m_strType = "match";
        m_strAttributes = string( "ID=" ) + NStr::UIntToString( uRecordId );
    };

    virtual ~CGffAlignmentRecord() {};

    void SetSourceLocation( 
        const CSeq_id&,
        ENa_strand );

    void SetTargetLocation( 
        const CSeq_id&,
        ENa_strand );

    virtual void SetMatchType(
        const CSeq_id&, //source
        const CSeq_id&); //target

    void SetScore(
        const CScore& );

    void SetPhase(
        unsigned int );

    void AddInsertion(
        const CAlnMap::TSignedRange& targetPiece ); 

    void AddDeletion(
        const CAlnMap::TSignedRange& sourcePiece ); 

    void AddMatch(
        const CAlnMap::TSignedRange& sourcePiece,
        const CAlnMap::TSignedRange& targetPiece ); 

    string StrAttributes() const;

protected:
    unsigned int m_uFlags;
    string m_strAlignment;
    string m_strOtherScores;

    CAlnMap::TSignedRange m_targetRange;
    CAlnMap::TSignedRange m_sourceRange;
    bool m_bIsTrivial;
};

END_objects_SCOPE
END_NCBI_SCOPE

#endif // OBJTOOLS_WRITERS___GFF3ALIGNMENTDATA__HPP
