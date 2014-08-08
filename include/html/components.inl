#if defined(HTML___COMPONENTS__HPP)  &&  !defined(HTML___COMPONENTS__INL)
#define HTML___COMPONENTS__INL

/*  $Id: components.inl 176066 2009-11-13 19:01:39Z ivanov $
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
 * Author: Eugene Vasilchenko
 *
 */


inline COptionDescription::COptionDescription(void)
{
    return;
}

inline COptionDescription::COptionDescription(const string& value)
    : m_Value(value)
{
    return;
}


inline COptionDescription::COptionDescription(const string& value,
                                              const string& label)
    : m_Value(value), m_Label(label)
{
    return;
}


inline void CSelectDescription::Add(int value)
{
    Add(NStr::IntToString(value));
}

#endif /* def HTML___COMPONENTS__HPP  &&  ndef HTML___COMPONENTS__INL */
