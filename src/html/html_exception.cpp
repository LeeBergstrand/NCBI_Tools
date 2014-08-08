/*  $Id: html_exception.cpp 103491 2007-05-04 17:18:18Z kazimird $
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
 * Authors:  Andrei Gourianov, Vladimir Ivanov
 *
 */


#include <ncbi_pch.hpp>
#include <html/html_exception.hpp>
#include <html/node.hpp>


BEGIN_NCBI_SCOPE


void CHTMLException::AddTraceInfo(const string& node_name)
{
    string name = node_name.empty() ? "?" : node_name;
    m_Trace.push_front(name);
}


void CHTMLException::ReportExtra(ostream& out) const
{
    CNCBINode::TExceptionFlags flags = CNCBINode::GetExceptionFlags();
    if ( (flags  &  CNCBINode::fAddTrace) != 0 ) {
        string trace;
        ITERATE(list<string>, it, m_Trace) {
            if ( !trace.empty() ) {
                trace += ":";
            }
            trace += *it;
        }
        out << trace;
    }
}


void CHTMLException::x_Assign(const CException& src)
{
    CException::x_Assign(src);
    m_Trace = dynamic_cast<const CHTMLException&>(src).m_Trace;
}


END_NCBI_SCOPE
