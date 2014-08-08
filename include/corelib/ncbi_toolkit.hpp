#ifndef CORELIB___NCBI_TOOLKIT__HPP
#define CORELIB___NCBI_TOOLKIT__HPP

/*  $Id: ncbi_toolkit.hpp 309324 2011-06-23 16:58:27Z gouriano $
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
 * Authors:  Andrei Gourianov, Denis Vakatov
 *
 *
 */

/// @file ncbi_toolkit.hpp
///
/// This API provides an easy way to initialize NCBI C++ Toolkit internals
/// to use the Toolkit from other application frameworks.
///

/** @addtogroup AppFramework
 *
 * @{
 */

#include "impl/ncbi_toolkit_impl.hpp"

namespace ncbi {


/////////////////////////////////////////////////////////////////////////////
///
/// Initialialize NCBI C++ Toolkit internal infrastructure:
///     arguments, environment, diagnostics, logging
/// @note
///  This function may be called only once -- and, before calling any
///  other C++ Toolkit function.
///
/// @param argc
///   Argument count [argc in a regular main(argc, argv)].
/// @param argv
///   Argument vector [argv in a regular main(argc, argv)].
/// @param envp
///   Environment pointer [envp in a regular main(argc, argv, envp)];
///   a null pointer (the default) corresponds to the standard system
///   array (environ on most Unix platforms).
/// @param log_handler
///   Handler for diagnostic messages that are emitted by the C++ Toolkit code
/// @sa
///   INcbiToolkit_LogHandler, NcbiToolkit_Fini

void NCBI_TOOLKIT_EXPORT NcbiToolkit_Init
   (int                                argc,
    const TNcbiToolkit_XChar* const*   argv,
    const TNcbiToolkit_XChar* const*   envp        = NULL,
    INcbiToolkit_LogHandler*           log_handler = NULL);



/////////////////////////////////////////////////////////////////////////////
///
/// Release resources allocated for NCBI C++ Toolkit
///
/// @sa NcbiToolkit_Init

void NCBI_TOOLKIT_EXPORT NcbiToolkit_Fini(void);



/////////////////////////////////////////////////////////////////////////////
///
/// Logging message
///
/// Diagnostic message that comes from the NCBI C++ Toolkit to the log handler
/// @sa
///   INcbiToolkit_LogHandler


class NCBI_TOOLKIT_EXPORT CNcbiToolkit_LogMessage
{
public:
    /// Log message severity
    enum ESeverity {
        eInfo = 0, ///< Informational message
        eWarning,  ///< Warning message
        eError,    ///< Error message
        eCritical, ///< Critical error message
        eFatal,    ///< Fatal error
        //
        eTrace,    ///< Trace message
    };

    /// Get the message the way it is formatted by the Toolkit
    operator std::string(void) const;

    /// Get text part of the message
    std::string Message(void) const;

    /// Get message severity
    ESeverity Severity(void) const;

    /// Get error code
    int ErrCode(void) const;

    /// Get error subcode
    int ErrSubCode(void) const;

    /// Get file name in which message was originated
    std::string File(void) const;

    /// Get line number in which message was originated
    size_t Line(void) const;

    /// Get all of the message's data -- as provided natively by the Toolkit
    const SDiagMessage& GetNativeToolkitMessage(void) const;    

protected:
    // this object is created and destroyed by the C++ Toolkit code only
    CNcbiToolkit_LogMessage(const SDiagMessage& msg);
    virtual ~CNcbiToolkit_LogMessage(void);
private:
    const SDiagMessage& m_Msg;
    // prohibit copy ctor and assignment
    CNcbiToolkit_LogMessage(const CNcbiToolkit_LogMessage&);
    CNcbiToolkit_LogMessage& operator=(const CNcbiToolkit_LogMessage&);
};



/////////////////////////////////////////////////////////////////////////////
///
/// Logging interface
///
/// Client application must provide implementation of this interface
/// and pass it to the Toolkit to receive diagnostic messages.
/// Message data is encapsulated into an CNcbiToolkit_LogMessage object.
/// @sa
///   CNcbiToolkit_LogMessage, NcbiToolkit_Init

class INcbiToolkit_LogHandler
{
public:
    virtual ~INcbiToolkit_LogHandler() {}
    virtual void Post(const CNcbiToolkit_LogMessage& msg) = 0;
};


} /* namespace ncbi */

#endif  /* CORELIB___NCBI_TOOLKIT__HPP */
