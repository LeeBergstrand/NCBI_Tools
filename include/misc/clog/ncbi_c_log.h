#ifndef NCBI_C_LOG__H
#define NCBI_C_LOG__H

/* $Id: ncbi_c_log.h 388542 2013-02-08 16:06:58Z ivanov $
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
 * Author:  Vladimir Ivanov
 *
 * @file
 * File Description:
 *    The C library to provide C++ Toolkit-like logging semantics 
 *    and output for C/C++ programs and CGIs.
 *
 * The C Logging API have two usage patterns:
 *
 *   1) First (recommended) method.
 *      Usage:
 *      - At the program start call NcbiLog_Init() (or MT/ST version),
 *        only once, probably from the main thread.
 *      - Call any logging methods.
 *      - For each application's thread that have used the C Logging API 
 *        call NcbiLog_Destroy_Thread() before thread termination.
 *      - After using API, or before program's exit call NcbiLog_Destroy().
 *
 *   2) Second method should be used only in special cases, where impossible
 *      to call NcbiLog_Init/NcbiLog_Destroy. Any of these two methods still 
 *      can be used...  But, if you can call both of them, consider to use
 *      pattern (1) instead. Forgot about NcbiLog_Destroy_Thread(), don't
 *      use it at all. And aware, that default MT locking implementation
 *      will be used automatically in MT configurations if you didn't use
 *      NcbiLog_Init() with your own MT-handler explicitly.
 *      Usage:
 *      - First, if you didn't call NcbiLog_Init() yet, you can call
 *        NcbiLog_InitForAttachedContext(). It does the same initializations
 *        as NcbiLog_Init(), but have better MT protection, and can be called
 *        as many times as you want, even in any thread. And even this step
 *        can be skipped, and all initialization will be done in 
 *        NcbiLog_Context_Create(). But you still have a possibility to pass
 *        an application name into the logging API. Only first call of 
 *        NcbiLog_Init*() have effect. All subsequent calls will be ignored.
 *      - Call NcbiLog_Context_Create() to create thread-specific contex.
 *      - Use NcbiLog_Context_Attach() to attach created context to the API.
 *      - Call any logging methods. Do not call them before attaching context,
 *        or you can get memory/resource leaks and unpredictable results.
 *      - Detach context with NcbiLog_Context_Detach().
 *        The pairs of NcbiLog_Context_Attach/NcbiLog_Context_Detach methods
 *        can be used as many times as needed. The thread-specific context
 *        should be saved somewhere between logging sessions. 
 *      - Use NcbiLog_Context_Destroy() when logging is done and you don't
 *        need a logging context object anymore.
 *      - Call NcbiLog_Destroy() to shutdown API. Do this only if possible.
 *        This step is recommended, and can help to avoid memory leaks.
 *
 * Any C logging API functions can be called only after first call of
 * NcbiLog_Init*() or NcbiLog_Context_Create(), otherwise its call
 * will be ignored.
 *
 * This library calls assert() when presented with unexpected
 * input or other error situations. In Release modes such errors can just
 * stop a logging, without aborting a program (except some critical cases).
 * But generally, very little graceful error recovery is attempted; that 
 * burden is pushed back to the caller.
 *
 * By default the module is dependent on the /etc/toolkitrc file in order to
 * determine which applications should log to which /log sub-directory. 
 * If this file can't be found or can't be read, or the log files can not
 * be open, it fall back to logging to destination specified here:
 * http://www.ncbi.nlm.nih.gov/books/NBK7185/#ch_core.Where_Diagnostic_Messages_Go
 *
 * Thread-specific serial number is always 0. We cannot provide mechanism
 * to maintain correct value for each thread.
 *
 */

#include <time.h>


/** @addtogroup Diagnostics
 *
 * @{
 */

#if defined(_MSC_VER)
#  if !defined(NCBI_OS_MSWIN)
#    define NCBI_OS_MSWIN  1
#  endif
#else
#  if !defined(NCBI_OS_UNIX)
#    define NCBI_OS_UNIX   1
#  endif
#endif

#if defined(NCBI_OS_UNIX)
#  ifndef   __STDC_FORMAT_MACROS
#    define __STDC_FORMAT_MACROS
#  endif
#  include <inttypes.h>
#  include <sys/types.h>
#elif defined(NCBI_OS_MSWIN)
#endif


#ifdef __cplusplus
extern "C" {
#endif



/******************************************************************************
 *  MT locking
 */

/** Lock handle -- keeps all data needed for the locking and the cleanup.
 */
struct         TNcbiLog_MTLock_tag;
typedef struct TNcbiLog_MTLock_tag*  TNcbiLog_MTLock;


/** The action passed to user defined MT lock handler.
 */
typedef enum {
    eNcbiLog_MT_Init,          /**< Init the locker (call first)    */
    eNcbiLog_MT_Lock,          /**< Lock                            */
    eNcbiLog_MT_Unlock,        /**< Unlock                          */
    eNcbiLog_MT_Destroy        /**< Unlock and cleanup (call last)  */
} ENcbiLog_MTLock_Action;


/** Type of ownership for MT lock handle.
 */
typedef enum {
    eNcbiLog_MT_NoOwnership,   /**< No ownership relationship */
    eNcbiLog_MT_TakeOwnership  /**< NcbiLog API takes ownership of MT lock */
} ENcbiLog_MTLock_Ownership;


/** MT locking callback.
 *  @param user_data
 *    See "user_data" in NcbiLog_MTLock_Create()
 *  @param action
 *    Operation that should be done in the callback handler.
 *  @return
 *    Non-zero value if the requested operation was successful.
 *  @note
 *    The "-1" value is reserved for unset handler;  you also
 *    may want to return "-1" if your locking function does no locking, and
 *    you don't consider it as an error, but still want the caller to be
 *    ware of this "rightful non-doing" as opposed to the "rightful doing".
 * @sa
 *    NcbiLog_MTLock_Create, NcbiLog_MTLock_Delete
 */
typedef int/*bool*/ (*FNcbiLog_MTLock_Handler)
    (void*                  user_data,
     ENcbiLog_MTLock_Action action
     );


/** Create new MT lock.
 *  @param user_data
 *    Unspecified data to call "handler" with
 *  @param handler
 *    Locking callback
 *  @return
 *    Handle to newly created MT lock object or NULL on error.
 *  @sa
 *    NcbiLog_MTLock_Delete, FNcbiLog_MTLock_Handler
 */
extern TNcbiLog_MTLock NcbiLog_MTLock_Create
    (void*                   user_data,
     FNcbiLog_MTLock_Handler handler
     );


/** Call cleanup action on the handler, then destroy it.
 *  @param lock
 *    A handle previously obtained from NcbiLog_MTLock_Create().
 *  @sa
 *    NcbiLog_MTLock_Create, FNcbiLog_MTLock_Handler
 */
extern void NcbiLog_MTLock_Delete(TNcbiLog_MTLock lock);


/** Default implementation of simple MT locking callback.
 *  @param user_data
 *    Simple handler don't use this parameter; will be ignored.
 *  @param action
 *    Operation that should be done in the callback handler.
 *  @return
 *    Non-zero value if the requested operation was successful.
 *  @sa
 *    FNcbiLog_MTLock_Handler, NcbiLog_MTLock_Create
 */
extern int/*bool*/ NcbiLog_Default_MTLock_Handler
    (void*                  user_data, 
     ENcbiLog_MTLock_Action action
     );



/******************************************************************************
 *  Common types and definitions
 */

/** Where to write the application's diagnostics to.
 *  @sa NcbiLog_SetDestination
 */
typedef enum {
    eNcbiLog_Default,         /**< Try /log/<*>/<appname>.log, fallback to STDERR */
    eNcbiLog_Stdlog,          /**< Try /log/<*>/<appname>.log,
                                   fallback to ./<appname>.log, then to STDERR */
    eNcbiLog_Cwd,             /**< Try ./<appname>.log, fallback to to STDERR */
    eNcbiLog_Stdout,          /**< To standard output stream */
    eNcbiLog_Stderr,          /**< To standard error stream  */
    eNcbiLog_Disable          /**< Don't write it anywhere   */
} ENcbiLog_Destination;


/** Severity level for the posted diagnostics
 */
typedef enum {
    eNcbiLog_Trace = 0,       /**< Trace message          */
    eNcbiLog_Info,            /**< Informational message  */
    eNcbiLog_Warning,         /**< Warning message        */
    eNcbiLog_Error,           /**< Error message          */
    eNcbiLog_Critical,        /**< Critical error message */
    eNcbiLog_Fatal            /**< Fatal error -- guarantees exit (or abort) */
} ENcbiLog_Severity;


/** Structure to describe pairs 'key=value', used to posting parameters.
 *  @sa NciLog_ReqStart, NcbiLog_Extra, NcbiLog_Perf
 */
typedef struct {
    const char* key;
    const char* value;
} SNcbiLog_Param;


/** Big integer type
*/
#if defined(__MINGW32__)  ||  defined(__MINGW64__)
   typedef          long long TNcbiLog_Int8;
   typedef unsigned long long TNcbiLog_UInt8;
#  define NCBILOG_INT8_FORMAT_SPEC "I64u"
#  define NCBILOG_UINT8_FORMAT_SPEC "I64"
#elif defined(NCBI_OS_MSWIN)
   typedef          __int64   TNcbiLog_Int8;
   typedef unsigned __int64   TNcbiLog_UInt8;
#  define NCBILOG_INT8_FORMAT_SPEC  "I64"
#  define NCBILOG_UINT8_FORMAT_SPEC "I64u"
#else
   typedef int64_t            TNcbiLog_Int8;
   typedef uint64_t           TNcbiLog_UInt8;
#  define NCBILOG_INT8_FORMAT_SPEC  PRIi64
#  define NCBILOG_UINT8_FORMAT_SPEC PRIu64
#endif


/** Process, thread and counter types
 */
typedef TNcbiLog_UInt8 TNcbiLog_PID;
typedef TNcbiLog_UInt8 TNcbiLog_TID;
typedef TNcbiLog_UInt8 TNcbiLog_Counter;


/** Declaration for thread-specific context
 */
struct SContext_tag;
typedef struct SContext_tag* TNcbiLog_Context;



/******************************************************************************
 *  Logging setup functions
 */

/** Initializing NcbiLog API.
 *  This function should be called before any other API's function.
 *  Only first call of NcbiLog_Init() have effect. All subsequent calls
 *  will be ignored. Preferable, in MT applications it should be called
 *  before creating any child thread uses logging.
 *  @param appname
 *    Set the application name shown in logs and used for logfile names.
 *    By default the name is unknown. The application name can be set only
 *    once at API initialization (using NcbiLog_Init() call).
 *    This name could include path and extension, only base name will be
 *    used to show in logs. Also, any spaces contained in the base file 
 *    name will be URL-encoded.
 *    Also, the application name can be hard-coded on the compilation step,
 *    using "-D NCBI_C_LOG_APPNAME=appname" in the compiler's command line
 *    for this API. Hard-coded name have a priority over the passed parameter.
 *  @param mt_lock
 *    User defined MT lock. It is necessary to use NcbiLog API in
 *    multi-threaded applications.
 *  @param own_mt_lock
 *    MT lock ownership flag. If eNcbiLog_MT_TakeOwnership is passed,
 *    then the MT lock handler will be destroyed in NcbiLog_Destroy().
 *  @note
 *    It is recommended to call NcbiLog_InitST() instead of NcbiLog_Init[MT]()
 *    if you don't use logging simultaneously from some threads.
 *  @sa
 *    NcbiLog_InitMT, NcbiLog_InitST, NcbiLog_Destroy, NcbiLog_MTLock_Create, 
 *    NcbiLog_Context_Create
 *  @note
 *    Sometimes, simplified logging initialization can be used. For details
 *    please see NcbiLog_Context_Create() description and comments at 
 *    the beginning of this file.
 */
extern void NcbiLog_Init(const char*               appname, 
                         TNcbiLog_MTLock           mt_lock, 
                         ENcbiLog_MTLock_Ownership mt_lock_ownership);


/** Version of NcbiLog_Init with default MT lock implementation.
 *  This function should be called before any other API's function.
 *  Preferable, in MT applications it should be called before
 *  creating any child thread uses logging.
 *  @sa NcbiLog_Init, NcbiLog_InitST
 */
extern void NcbiLog_InitMT(const char* appname);


/** Version of NcbiLog_Init to use in single-threaded applications.
 *  This function should be called before any other API's function.
 *  @note
 *    You can call NcbiLog_InitST() instead of NcbiLog_Init[MT]()
 *    in MT applications also if you don't use logging from some
 *    threads simultaneously. This will turn off API's MT-safety.
 *  @sa NcbiLog_Init, NcbiLog_InitMT
 */
extern void NcbiLog_InitST(const char* appname);


/** Version of NcbiLog_Init to use with NcbiLog_Context_* functions only.
 *  This function should be called before any other API's function.
 *  Only first call of NcbiLog_InitForAttachedContext() have effect.
 *  All subsequent calls will be ignored. You can skip it if you don't 
 *  wish to set up an application name for logging, all initialization
 *  will be done in background.
 *  @note
 *    Default MT locking implementation will be used, you cannot use
 *    your own locking using with this type on initialization.
 *  @note
 *    For details please see NcbiLog_Context_Create() description
 *    and comments at the beginning of this file.
 *  @param appname
 *    Set the application name shown in logs and used for logfile names.
 *    By default the name is unknown. The application name can be set only
 *    once at API initialization.
 *    This name could include path and extension, only base name will be
 *    used to show in logs. Also, any spaces contained in the base file 
 *    name will be URL-encoded.
 *    Also, the application name can be hard-coded on the compilation step,
 *    using "-D NCBI_C_LOG_APPNAME=appname" in the compiler's command line
 *    for this API. Hard-coded name have a priority over passed parameter.
 *  @sa 
 *    NcbiLog_Init, NcbiLog_InitForAttachedContextST, NcbiLog_Context_Create
 */
extern void NcbiLog_InitForAttachedContext(const char* appname);


/** Version of NcbiLog_InitForAttachedContext() intended to use 
  * in single-threaded applications. Use it if you use API from single thread
  * only, it can be a little bit faster than NcbiLog_InitForAttachedContext()
  * because don't use MT-safety.
  * @sa 
  *   NcbiLog_InitForAttachedContext, NcbiLog_Init, NcbiLog_InitST,
  *   NcbiLog_Context_Create
  */
extern void NcbiLog_InitForAttachedContextST(const char* appname);


/** Destroy NcbiLog API.
 *  This function should be called last. After it any other API's calls
 *  will be ignored. For MT applications see also NcbiLog_Destroy_Thread().
 *  @sa
 *    NcbiLog_Init, NcbiLog_InitForAttachedContext
 */
extern void NcbiLog_Destroy(void);


/** Destroy thread-specific NcbiLog API information.
 *  Each thread should call this function before termination, and
 *  before NcbiLog_Destroy() call.
 *  Calling any other API function in the current thread except
 *  NcbiLog_Destroy() is prohibited and can lead to application crash.
 *  @note 
 *    Not necessary to call this function in single-threaded
 *    applications if NcbiLog_InitST() was used.
 *  @sa
 *    NcbiLog_Init, NcbiLog_InitST, NcbiLog_Destroy
 */
extern void NcbiLog_Destroy_Thread(void);


/** Set up diagnostics destination.
 *  @param ds
 *    An enum value to specify an application's diagnostics destination.
 *  @return
 *    The diagnostic destination that was really set.
 *    It is not always possible to set up an destination for logging messages
 *    that specified in parameters and fallback can be used.
 *    See ENcbiLog_Destination description for details.
 * @note
 *    By default, if SetDestination() is not called or set to eNcbiLog_Default,
 *    and environment variable $NCBI_CONFIG__LOG__FILE is defined and not empty,
 *    its value will be used as base name for logfiles.
 *    It can have special value "-" to redirect all output to STDERR.
 *  @sa
 *    ENcbiLog_Destination, NcbiLog_Init
 */
extern ENcbiLog_Destination NcbiLog_SetDestination(ENcbiLog_Destination ds);


/** Set log_site.
 *  The new value overrides any environment value. If set to an empty string,
 *  the value is not printed at all.
 */
extern void NcbiLog_SetLogSite(const char* logsite);


/** Set PID/TID values
 *  @note
 *    This methods do not affect GUID value if called after starting logging.
 */
extern void NcbiLog_SetProcessId(TNcbiLog_PID pid);
extern void NcbiLog_SetThreadId (TNcbiLog_TID tid);


/** Set/get request ID. 
 *  Calling this method before NcbiLog_AppRun() is not allowed.
 * 
 *  @note
 *    NcbiLog_SetRequestId() do not affect already started requests.
 *    Only newly started request will have new ID.
 *  @note
 *    NcbiLog_ReqStart() automaticaly increase request number. 
 *    So, next request will start with (rid + 1).
 *  @sa NcbiLog_ReqStart
 */
extern void             NcbiLog_SetRequestId(TNcbiLog_Counter rid);
extern TNcbiLog_Counter NcbiLog_GetRequestId(void);


/** This function allows a program to override the posting date and time
 *  that is logged. It can speed up a logging if date/time is already known
 *  before calling any post function.
 *  @param timer
 *    GMT date and time at which the message was posted (see time()).
 *    This time value will be converted to local date/time automatically.
 *    The current date/time will be used if this parameter is zero.
 *  @param nanoseconds
 *    Nanosecond part of the time.
 *  @note
 *    The set value will be used for all postings.
 *    To use system time again, call NcbiLog_SetTime(0,0).
 */
extern void NcbiLog_SetTime(time_t timer, unsigned long nanoseconds);


/** This function allows a program to override the normal host string
 *  that is logged. Usually this value is either taken from the system.
 *  Using this method will override it with the user supplied string.
 *  @param host
 *    New host name.
 *    Will be set to 'UNK_HOST' if parameter is NULL or empty string.
 *  @note
 *    This method do not affect GUID value if called after starting logging.
 */
extern void NcbiLog_SetHost(const char* host);


/** This function allows a program to override the normal client string
 *  that is logged. Usually this value is either taken from the 
 *  HTTP_CAF_PROXIED_HOST environment variable or, if that is not populated,
 *  set to 'UNK_CLIENT'. Using this method will override both of those with
 *  the user supplied string.
 *  @param client
 *    New client name (IP address).
 *    Will be set to UNK_CLIENT if parameter is NULL or empty string.
 *  @note
 *    This value is reset by the NcbiLog_ReqStart(). If you are overriding
 *    the HTTP_CAF_PROXIED_HOST environment variable with your own value,
 *    you must recall NcbiLog_SetClient() with your string after every call
 *    to NcbiLog_ReqStart() in order to preserve your setting.
 *  @sa
 *    NcbiLog_ReqStart
 */
extern void NcbiLog_SetClient(const char* client);


/** This function allows a program to override the normal session ID string
 *  that is logged. By default it is set to 'UNK_SESSION'. Using this method
 *  will override it with the user supplied string. Usually this value is
 *  taken from the ncbi_sid key in the standard NCBI web cookie.
 *  @param client
 *    New session ID (URL encoded).
 *    Will be set to 'UNK_SESSION' if parameter is NULL or empty string.
 *    Any spaces contained in the string will be URL-encoded.
 *  @note
 *    This value is reset by the NcbiLog_ReqStart() to 'UNK_SESSION'.
 *    If you are overriding it with your own value, you must recall
 *    NcbiLog_SetSession() with your string after every call to
 *    NcbiLog_ReqStart() in order to preserve your setting.
 *  @sa
 *    NcbiLog_ReqStart
 */
extern void NcbiLog_SetSession(const char* session);


/** Set new posting level.
 *  All messages with severity lower than specified will be ignored.
 *  Always returns the active level. 
 */
extern ENcbiLog_Severity NcbiLog_SetPostLevel(ENcbiLog_Severity sev);



/******************************************************************************
 *  NcbiLog context functions.
 *  NOTE:  see comments at the beginning of this file.
 */

/** Create new thread-specific logging context object.
 *  These context-specific methods to create, attach, detach and
 *  destroy thread-specific logging context objects allow to separate
 *  context from the API, store it somewhere else and use when necessary,
 *  even from different threads. Because the API initialization
 *  can be done in background, it is impossible to pass application name
 *  used in each logging message, so it will be "UNKNOWN" by default.
 *  But, you can call NcbiLog_Init() or NcbiLog_InitForAttachedContext(),
 *  or one of its variants. Or, you can hard-code application name on the
 *  compilation step, using "-D NCBI_C_LOG_APPNAME=appname" in the compiler's
 *  command line for this API.
 *  @note
 *    You should call this method first, before using any other API methods.
 *  @note
 *    Created context have a thread id of the thread that created it.
 *    Thread id can be changed if necessary using NcbiLog_SetThreadId()
 *    after attaching context to the API.
 *  @note
 *    Default MT locking implementation will be used in MT configurations,
 *    you cannot use your own MT-handler with this type on initialization.
 *  @note
 *    Also, please read NcbiLog_Init() and comments at the beginning of this file.
 *  @return
 *    Handle to newly created context object or NULL on error.
 *  @sa
 *    NcbiLog_Context_Attach, NcbiLog_Context_Detach, NcbiLog_Context_Destroy,
 *    NcbiLog_ForAttachedContext
 */
extern TNcbiLog_Context NcbiLog_Context_Create(void);


/** Attach logging context object to the C Logging API.
 *  @note
 *    You should call this method before logging or using any other
 *    methods that change context information. All API methods works
 *    with attached context only, otherwise you can get unexpected results.
 *  @param ctx
 *    A handle previously obtained from NcbiLog_Context_Create().
 *  @return
 *    Non-zero value if the requested operation was successful.
 *    Also, return zero value if some other context is already attached.
 *  @sa
 *    NcbiLog_Context_Create, NcbiLog_Context_Attach, NcbiLog_Context_Destroy
 */
extern int /*bool*/ NcbiLog_Context_Attach(TNcbiLog_Context ctx);


/** Detach logging context object from the C Logging API.
 *  @note
 *    The C Logging API cannot be used without any context attached.
 *  @return
 *    Non-zero value if the requested operation was successful.
 *  @sa
 *    NcbiLog_Context_Create, NcbiLog_Context_Attach, NcbiLog_Context_Destroy
 */
extern void NcbiLog_Context_Detach(void);


/** Destroy context structure.
 *  @note
 *    NcbiLog_Context_Detach() should be called before context destroying.
 *  @param ctx
 *    A handle previously obtained from NcbiLog_Context_Create().
 *  @sa
 *    NcbiLog_Context_Create, NcbiLog_Context_Attach, NcbiLog_Context_Detach
 */
extern void NcbiLog_Context_Destroy(TNcbiLog_Context ctx);



/******************************************************************************
 *  NcbiLog posting functions
 */

/** Should be called as soon as possible during application initialization.
 *  Writes the 'start' line to the <appname>.log, including the command line.
 *  Calling this function early ensures that the start time of the application
 *  is recorded; that data is later used by NcbiLog_AppStop() to calculate 
 *  the application runtime duration. If not called explicitly,
 *  NcbiLog_AppStart() is called by most other functions (but application
 *  arguments will not be logged in this case).
 *  @param argc
 *    The count of arguments that follow in argv (from main()).
 *  @param argv
 *    An array of null-terminated strings representing command-line
 *    arguments of the program (from main()).
 *  @sa NcbiLog_AppRun, NcbiLog_AppStop
 */
extern void NcbiLog_AppStart(const char* argv[]);


/** Should be called after the application is initialized and before its
 *  main part.Just changes the application state shown in the log to 'A'.
 *  It does not cause any information to be logged.
 *
 *  @sa NcbiLog_AppStart, NcbiLog_AppStop
 */
extern void NcbiLog_AppRun(void);


/** Should be called immediately prior to the application exit. 
 *  Logs the 'stop' line to the <appname>.log files.
 *  Calculates the application runtime duration based on a start time
 *  recorded when the application starts up.
 *
 *  @sa NcbiLog_AppStart, NcbiLog_AppStopSignal
 */
extern void NcbiLog_AppStop(int exit_status);


/** The same as NcbiLog_AppStop(), but accept also a signal number,
 *  if application exited due to a signal.
 *
 *  @sa NcbiLog_AppStart, NcbiLog_AppStop
 */
extern void NcbiLog_AppStopSignal(int exit_status, int exit_signal);


/** Should be called once application startup is complete, before any
 *  request processing code is run. Writes the 'request-start' line to the
 *  <appname>.log file.
 *  Sets or resets the start time of the current request; that data is later
 *  used by NcbiLog_ReqStop() to calculate the request runtime duration. 
 *  Can optionally be passed a pointer to NULL-terminated array with
 *  parameters, as key/value pairs; that data will be logged.
 *  This method also resets the client and session information. If the user
 *  has provided alternate info, it may be overridden as a side-effect of
 *  calling this method. Automatically increase request id number.
 *
 *  @sa NcbiLog_SetRequestId, NcbiLog_ReqStop
 */
extern void NcbiLog_ReqStart(const SNcbiLog_Param* params);


/** Should be called once request processing initialization is complete.
 *  It does not cause any information to be logged. Just changes
 *  the application state shown in the log to 'R'. 
 *
 *  @sa NcbiLog_ReqStart, NcbiLog_ReqStop
 */
extern void NcbiLog_ReqRun(void);


/** Should be called once request processing is complete. Writes the
 * 'request-stop' line to the <appname>.log file. 
 *  Calculates request runtime duration based on a start time recorded
 *  when NcbiLog_ReqStart() is called.
 *  Session and client information is reset to UNK_SESSION and UNK_CLIENT.
 *
 *  @sa NcbiLog_ReqStart
 */
extern void NcbiLog_ReqStop(int status, size_t bytes_rd, size_t bytes_wr);


/** Can be called at any time. Writes the 'extra' line to the <appname>.log
 *  file. Can take a pointer to NULL-terminated array with parameters,
 *  as key/value pairs; that data will be logged. Appropriate for logging
 *  thing such as HTTP referrer information and cookie values. Should NOT
 *  be used for executing tracing or logging of debugging info -- see 
 *  the NcbiLog_Trace function for those purposes.
 */
extern void NcbiLog_Extra(const SNcbiLog_Param* params);


/** Can be called at any time. Writes the 'perf' line to the <appname>.log
 *  file. Used to log a performance information. Takes status code of some
 *  operation, its execution time and a pointer to NULL-terminated array with
 *  parameters, as key/value pairs.
 */
extern void NcbiLog_Perf(int status, double timespan,
                         const SNcbiLog_Param* params);


/** Writes a message to the <appname>.trace file at level 'TRACE'.
 *  Note that the default log level is INFO, meaning that unless the log
 *  level is explicitly set lower, messages logged via TRACE will not be
 *  seen or recorded.
 */
extern void NcbiLog_Trace(const char* msg);


/** Writes a message to the <appname>.trace file at level 'INFO'.
 *  Note that the default log level is INFO, meaning that these messages
 *  will be stored to disk or sent to stderr/stdout.
 */
extern void NcbiLog_Info(const char* msg);


/** Writes a message to the <appname>.err log at level 'WARNING'.
 */
extern void NcbiLog_Warning(const char* msg);


/** Writes a message to the <appname>.err log at level 'ERROR'.
 */
extern void NcbiLog_Error(const char* msg);


/** Writes a message to the <appname>.err log at level 'CRITICAL'.
 */
extern void NcbiLog_Critical(const char* msg);


/** Writes a message to the <appname>.err log at level 'FATAL' and then
 *  terminate the application.
 */
extern void NcbiLog_Fatal(const char* msg);


/* extern void NcbiLog_PostEx(ENcbiLog_Severity severity, const char* msg, const SNcbiLog_PostCtx* ctx); */

#ifdef __cplusplus
}  /* extern "C" */
#endif


/* @} */

#endif /* NCBI_C_LOG__H */
