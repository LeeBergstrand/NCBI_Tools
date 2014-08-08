/*
 * Copyright (C) 2010 Vaclav Slavik <vslavik@fastmail.fm>
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * $Id: exception.hpp 189865 2010-04-26 15:28:41Z satskyse $
 * NOTE: This file was modified from its original version 0.7.0
 *       to fit the NCBI C++ Toolkit build framework and
 *       API and functionality requirements.
 */

/** @file
 * This file contains the definition of the xml::exception class.
 */


#ifndef _xmlwrapp_exception_h_
#define _xmlwrapp_exception_h_

#include <stdexcept>
#include <string>

/// XML library namespace
namespace xml
{
    /**
     * This exception class is thrown by xmlwrapp for all runtime XML-related
     * errors along with the xml::parser_exception.
     *
     * @note C++ runtime may still throw other errors when used from xmlwrapp.
     *       Also, std::bad_alloc() is thrown in out-of-memory situations.
     *
     * @since 0.7.0
     */
    class exception : public std::runtime_error
    {
    public:
        explicit exception(const std::string& what) : std::runtime_error(what)
        {}
    };

} // namespace xml

#endif // _xmlwrapp_exception_h_

