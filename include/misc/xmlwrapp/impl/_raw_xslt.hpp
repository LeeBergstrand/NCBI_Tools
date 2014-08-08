/*
 * $Id: _raw_xslt.hpp 376268 2012-09-27 21:55:41Z satskyse $
 */

#ifndef _raw_xslt_hpp_
#define _raw_xslt_hpp_


#include <ncbiconf.h>

namespace xslt
{
    class stylesheet;
}

namespace xslt {

    namespace impl {
        // This function is temporary and created specifically to cover
        // a request described in CXX-3519.
        // The function will be removed as soon as C++ style support for
        // extension functions and elements is introduced.
        // Note: the provided pointer is of type xsltStylesheet *
        NCBI_DEPRECATED
        void *  temporary_existing_get_raw_xslt_stylesheet(xslt::stylesheet & s);
    }
}


#endif

