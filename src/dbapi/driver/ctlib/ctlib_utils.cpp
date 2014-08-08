/* $Id: ctlib_utils.cpp 147457 2008-12-10 18:21:48Z ivanovp $
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
 * Author:  Sergey Sikorskiy
 *
 * File Description:  Small utility classes common to the ctlib driver.
 *
 */

#include <ncbi_pch.hpp>

#include "ctlib_utils.hpp"


BEGIN_NCBI_SCOPE


/////////////////////////////////////////////////////////////////////////////
impl::CDBExceptionStorage& GetCTLExceptionStorage(void)
{
    static CStaticTls<impl::CDBExceptionStorage> instance;

    impl::CDBExceptionStorage* result = instance.GetValue();
    if (!result) {
		auto_ptr<impl::CDBExceptionStorage> tmp_value(new impl::CDBExceptionStorage);
        instance.SetValue(tmp_value.get(), s_DelExceptionStorage);
		result = tmp_value.release();
    }

    return *result;
}



END_NCBI_SCOPE

