/*  $Id: bed_reader.hpp 387807 2013-02-01 14:40:34Z ludwigf $
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

#ifndef OBJTOOLS_READERS___BEDREADER__HPP
#define OBJTOOLS_READERS___BEDREADER__HPP

#include <corelib/ncbistd.hpp>
#include <objtools/readers/reader_base.hpp>
#include <objtools/readers/error_container.hpp>
#include <objects/seq/Seq_annot.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_interval.hpp>

BEGIN_NCBI_SCOPE

BEGIN_SCOPE(objects) // namespace ncbi::objects::

//  ----------------------------------------------------------------------------
class CRawBedRecord
//  ----------------------------------------------------------------------------
{
public:
    CRawBedRecord(): m_score(-1) {};

    void SetInterval(
        CSeq_id& id,
        unsigned int start,
        unsigned int stop,
        ENa_strand strand)
    {
        m_pInterval.Reset(new CSeq_interval());
        m_pInterval->SetId(id);
        m_pInterval->SetFrom(start-1);
        m_pInterval->SetTo(stop-2);
        m_pInterval->SetStrand(strand);
    };

    void SetScore(
        unsigned int score)
    {
        m_score = score;
    };

    ~CRawBedRecord() {};

    void Dump(
        CNcbiOstream& ostr) const
    {
        ostr << "  [CRawBedRecord" << endl;
        ostr << "id=\"" << m_pInterval->GetId().AsFastaString() << "\" ";
        ostr << "start=" << m_pInterval->GetFrom() << " ";
        ostr << "stop=" << m_pInterval->GetTo() << " ";
        ostr << "strand=" << 
            (m_pInterval->GetStrand() == eNa_strand_minus ? "-" : "+") << " ";
        if (m_score >= 0) {
            ostr << "score=" << m_score << " ";
        }
        ostr << "]" << endl;
    }

public:
    CRef<CSeq_interval> m_pInterval;
    int m_score;
};

//  ----------------------------------------------------------------------------
class CRawBedTrack
//  ----------------------------------------------------------------------------
{
public:
    CRawBedTrack() {};
    ~CRawBedTrack() {};

public:
    void Reset() 
    {
        m_Records.clear();
    };

    void Dump(
        CNcbiOstream& ostr) const
    {
        ostr << "[CRawBedTrack" << endl;
        for (vector<CRawBedRecord>::const_iterator it = m_Records.begin();
                it != m_Records.end(); ++it) {
            it->Dump(ostr);
        }
        ostr << "]" << std::endl;
    }

    void AddRecord(
        CRawBedRecord& record)
    {
        m_Records.push_back(record);
    }

    const vector<CRawBedRecord>& Records() const
    {
        return m_Records;
    }

    bool HasData() const
    {
        return (!m_Records.empty());
    }

public:
    CRef<CSeq_id> m_pId;
    vector<CRawBedRecord> m_Records;
};


//  ----------------------------------------------------------------------------
/// CReaderBase implementation that reads BED data files, either a single object
/// or all objects found. For the purpose of CBedReader, an object consists of
/// a run of records all with the same ID (BED comlumn 1), and all contained
/// within a single track.
///
class NCBI_XOBJREAD_EXPORT CBedReader
//  ----------------------------------------------------------------------------
    : public CReaderBase
{
    //
    //  object management:
    //
public:
    CBedReader( 
        unsigned int =fNormal );
    virtual ~CBedReader();
    
    //
    //  object interface:
    //
public:
    enum EBedFlags {
        // currentry, only what's inherited from CReaderBase
    };
    typedef int TFlags;

    /// Read object from line reader containing BED data, rendering it as a
    /// Seq-annot
    /// @param lr
    ///   line reader to read from.
    /// @param pErrors
    ///   pointer to optional error container object. 
    ///
    virtual CRef< CSerialObject >
    ReadObject(
        ILineReader& lr,
        IErrorContainer* pErrors=0 );
                
    /// Read a single object from given line reader containing BED data. The
    /// resulting Seq-annot will contain a feature table.
    /// @param lr
    ///   line reader to read from.
    /// @param pErrors
    ///   pointer to optional error container object. 
    ///  
    virtual CRef< CSeq_annot >
    ReadSeqAnnot(
        ILineReader& lr,
        IErrorContainer* pErrors=0 );

    /// Read all objects from given insput stream, returning them as a vector of
    /// Seq-annots, each containing a feature table.
    /// @param annots
    ///   (out) vector containing read Seq-annots
    /// @param istr
    ///   input stream to read from.
    /// @param pErrors
    ///   pointer to optional error container object. 
    ///  
    virtual void
    ReadSeqAnnots(
        vector< CRef<CSeq_annot> >& annots,
        CNcbiIstream& istr,
        IErrorContainer* pErrors=0 );
                        
    /// Read all objects from given insput stream, returning them as a vector of
    /// Seq-annots, each containing a feature table.
    /// @param annots
    ///   (out) vector containing read Seq-annots
    /// @param lr
    ///   line reader to read from.
    /// @param pErrors
    ///   pointer to optional error container object. 
    ///  
    virtual void
    ReadSeqAnnots(
        vector< CRef<CSeq_annot> >& annots,
        ILineReader& lr,
        IErrorContainer* pErrors=0 );
                        
    virtual bool 
    ReadTrackData(
        ILineReader&,
        CRawBedTrack&,
        IErrorContainer* =0 );

protected:
    virtual bool xParseTrackLine(
        const string&,
        CRef< CSeq_annot >& );
        
    bool xParseFeature(
        const string&,
        CRef<CSeq_annot>&);
    /* throws CObjReaderLineException */

    bool xParseComment(
        const string&,
        CRef<CSeq_annot>&);

    void x_SetFeatureLocation(
        CRef<CSeq_feat>&,
        const vector<string>& );
        
    void x_SetFeatureDisplayData(
        CRef<CSeq_feat>&,
        const vector<string>& );

    virtual void x_SetTrackData(
        CRef<CSeq_annot>&,
        CRef<CUser_object>&,
        const string&,
        const string& );

    CRef< CSeq_annot > x_AppendAnnot(
        vector< CRef< CSeq_annot > >& );
                    
    void
    xProcessError(
        CObjReaderLineException&,
        IErrorContainer* );

    bool
    xReadBedDataRaw(
        ILineReader&,
        CRawBedTrack&,
        IErrorContainer*);

    bool
    xReadBedRecordRaw(
        const string&,
        CRawBedRecord&,
        IErrorContainer*);

    bool 
    xGetLine(
        ILineReader&,
        string&);

    bool 
    xCommentLine(
        const string&) const;

    static void xCleanColumnValues(
        vector<string>&);
    //
    //  data:
    //
protected:
    CErrorContainerLenient m_ErrorsPrivate;
    string m_currentId;

    vector<string>::size_type m_columncount;
    bool m_usescore;
    unsigned int m_CurBatchSize;
    const unsigned int m_MaxBatchSize;
};

END_SCOPE(objects)
END_NCBI_SCOPE

#endif // OBJTOOLS_READERS___BEDREADER__HPP
