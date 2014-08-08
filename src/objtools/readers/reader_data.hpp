/*  $Id: reader_data.hpp 267957 2011-03-28 12:28:58Z ludwigf $
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
 *   data structures of interest to multiple readers
 *
 */

#ifndef OBJTOOLS_READERS___READERDATA__HPP
#define OBJTOOLS_READERS___READERDATA__HPP

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects) // namespace ncbi::objects::

//  ============================================================================
class CBrowserData
//  ============================================================================
{
public:
    typedef std::vector< std::string > LineData;
    typedef std::map< std::string, std::string > BrowserData;

public:
    CBrowserData() {};
    ~CBrowserData() {};
    bool ParseLine(
        const LineData& );
    static bool IsBrowserData(
        const LineData& );
    const BrowserData& Values() const;

protected:
    BrowserData m_Data;
};

//  ============================================================================
class CTrackData
//  ============================================================================
{
public:
    typedef std::vector< std::string > LineData;
    typedef std::map< std::string, std::string > TrackData;
public:
    CTrackData() {};
    ~CTrackData() {};
    bool ParseLine(
        const LineData& );
    static bool IsTrackData(
        const LineData& );
    const TrackData& Values() const;
    string Type() const { return m_strType; };
    string Description() const { return m_strDescription; };
    string Name() const { return m_strName; };

protected:
    TrackData m_Data;
    string m_strType;
    string m_strDescription;
    string m_strName;
};

END_SCOPE(objects)
END_NCBI_SCOPE

#endif // OBJTOOLS_READERS___READERDATA__HPP
