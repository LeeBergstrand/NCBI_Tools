/*  $Id: extension_element_impl.hpp 377654 2012-10-15 14:11:19Z satskyse $
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
 * Author:  Sergey Satskiy, NCBI
 * Credits: Denis Vakatov, NCBI (API design)
 *
 */


/** @file
 * This file contains definition of the
 * xslt::impl::extension_element_impl class.
**/


#ifndef _xmlwrapp_extension_element_impl_hpp_
#define _xmlwrapp_extension_element_impl_hpp_

#include <libxslt/xsltInternals.h>


namespace xslt {

    namespace impl {
        struct extension_element_impl
        {
            xsltTransformContextPtr     xslt_ctxt;
            xmlNodePtr                  instruction_node;
            extension_element_impl();
        };
    }
}

#endif

