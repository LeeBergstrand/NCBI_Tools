/* $Id: ncbi_strerror.c 392462 2013-03-16 03:51:16Z rafanovi $
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
 * Authors:  Pavel Ivanov, Anton Lavrentiev, Denis Vakatov
 *
 * File Description:
 *   errno->text conversion helper
 */


#ifdef NCBI_INCLUDE_STRERROR_C


#  ifdef _FREETDS_LIBRARY_SOURCE

#    define s_StrError                s_StrErrorInternal
#    define UTIL_TcharToUtf8          UTIL_TcharToUtf8_ftds64
#    define UTIL_ReleaseBufferOnHeap  UTIL_ReleaseBufferOnHeap_ftds64

#  endif /*_FREETDS_LIBRARY_SOURCE*/


#  if defined(NCBI_OS_MSWIN)  &&  defined(_UNICODE)

static const char* s_WinStrdup(const char* str)
{
    size_t n = strlen(str);
    char*  s = (char*) LocalAlloc(LMEM_FIXED, ++n * sizeof(*s));
    return s ? (const char*) memcpy(s, str, n) : 0;
}

#    define   MSWIN_STRDUP(s)         s_WinStrdup(s)

#    ifndef   UTIL_ReleaseBuffer
#      define UTIL_ReleaseBuffer(x)   UTIL_ReleaseBufferOnHeap(x)
#    endif

#  else /*NCBI_OS_MSWIN && _UNICODE*/

#    define   MSWIN_STRDUP(s)         (s)

#    ifndef   UTIL_TcharToUtf8
#      define UTIL_TcharToUtf8(x)     (x)
#    endif

#    ifndef   UTIL_ReleaseBuffer
#      define UTIL_ReleaseBuffer(x)   /*void*/
#    endif

#  endif /*NCBI_OS_MSWIN && _UNICODE*/


#  ifdef NCBI_OS_MSWIN

#    ifdef _UNICODE

extern const char* UTIL_TcharToUtf8(const TCHAR* str)
{
    char* s = NULL;
    if (str) {
        /* Note "-1" means to consume all input including the trailing NUL */
        int n = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
        if (n > 0) {
            s = (char*) LocalAlloc(LMEM_FIXED, n * sizeof(*s));
            if (s)
                WideCharToMultiByte(CP_UTF8, 0, str, -1, s,    n, NULL, NULL);
        }
    }
    return s;
}

#    endif /*_UNICODE*/

extern void UTIL_ReleaseBufferOnHeap(const void* ptr)
{
    if (ptr)
        LocalFree((HLOCAL) ptr);
}

#  endif /*NCBI_OS_MSWIN*/


static const char* s_StrErrorInternal(int error)
{
    static const struct {
        int         errnum;
        const char* errtxt;
    } errmap[] = {
#  ifdef NCBI_OS_MSWIN
        {WSAEINTR,  "Interrupted system call"},
        {WSAEBADF,  "Bad file number"},
        {WSAEACCES, "Access denied"},
        {WSAEFAULT, "Segmentation fault"},
        {WSAEINVAL, "Invalid agrument"},
        {WSAEMFILE, "Too many open files"},
        /*
         * Windows Sockets definitions of regular Berkeley error constants
         */
        {WSAEWOULDBLOCK,     "Resource temporarily unavailable"},
        {WSAEINPROGRESS,     "Operation now in progress"},
        {WSAEALREADY,        "Operation already in progress"},
        {WSAENOTSOCK,        "Not a socket"},
        {WSAEDESTADDRREQ,    "Destination address required"},
        {WSAEMSGSIZE,        "Invalid message size"},
        {WSAEPROTOTYPE,      "Wrong protocol type"},
        {WSAENOPROTOOPT,     "Bad protocol option"},
        {WSAEPROTONOSUPPORT, "Protocol not supported"},
        {WSAESOCKTNOSUPPORT, "Socket type not supported"},
        {WSAEOPNOTSUPP,      "Operation not supported"},
        {WSAEPFNOSUPPORT,    "Protocol family not supported"},
        {WSAEAFNOSUPPORT,    "Address family not supported"},
        {WSAEADDRINUSE,      "Address already in use"},
        {WSAEADDRNOTAVAIL,   "Cannot assign requested address"},
        {WSAENETDOWN,        "Network is down"},
        {WSAENETUNREACH,     "Network is unreachable"},
        {WSAENETRESET,       "Connection dropped on network reset"},
        {WSAECONNABORTED,    "Software caused connection abort"},
        {WSAECONNRESET,      "Connection reset by peer"},
        {WSAENOBUFS,         "No buffer space available"},
        {WSAEISCONN,         "Socket is already connected"},
        {WSAENOTCONN,        "Socket is not connected"},
        {WSAESHUTDOWN,       "Cannot send after socket shutdown"},
        {WSAETOOMANYREFS,    "Too many references"},
        {WSAETIMEDOUT,       "Operation timed out"},
        {WSAECONNREFUSED,    "Connection refused"},
        {WSAELOOP,           "Infinite loop"},
        {WSAENAMETOOLONG,    "Name too long"},
        {WSAEHOSTDOWN,       "Host is down"},
        {WSAEHOSTUNREACH,    "Host unreachable"},
        {WSAENOTEMPTY,       "Not empty"},
        {WSAEPROCLIM,        "Too many processes"},
        {WSAEUSERS,          "Too many users"},
        {WSAEDQUOT,          "Quota exceeded"},
        {WSAESTALE,          "Stale descriptor"},
        {WSAEREMOTE,         "Remote error"},
        /*
         * Extended Windows Sockets error constant definitions
         */
        {WSASYSNOTREADY,         "Network subsystem is unavailable"},
        {WSAVERNOTSUPPORTED,     "Winsock.dll version out of range"},
        {WSANOTINITIALISED,      "Not yet initialized"},
        {WSAEDISCON,             "Graceful shutdown in progress"},
#    ifdef WSAENOMORE
        /*NB: replaced with WSA_E_NO_MORE*/
        {WSAENOMORE,             "No more data available"},
#    endif /*WSAENOMORE*/
#    ifdef WSA_E_NO_MORE
        {WSA_E_NO_MORE,          "No more data available"},
#    endif /*WSA_E_NO_MORE*/
#    ifdef WSAECANCELLED
        /*NB: replaced with WSA_E_CANCELLED*/
        {WSAECANCELLED,          "Call has been cancelled"},
#    endif /*WSAECANCELLED*/
#    ifdef WSA_E_CANCELLED
        {WSA_E_CANCELLED,        "Call has been cancelled"},
#    endif /*WSA_E_CANCELLED*/
        {WSAEINVALIDPROCTABLE,   "Invalid procedure table"},
        {WSAEINVALIDPROVIDER,    "Invalid provider version number"},
        {WSAEPROVIDERFAILEDINIT, "Cannot init provider"},
        {WSASYSCALLFAILURE,      "System call failed"},
        {WSASERVICE_NOT_FOUND,   "Service not found"},
        {WSATYPE_NOT_FOUND,      "Class type not found"},
        {WSAEREFUSED,            "Query refused"},
        /*
         * WinSock 2 extension
         */
#    ifdef WSA_IO_PENDING
        {WSA_IO_PENDING,         "Operation has been queued"},
#    endif /*WSA_IO_PENDING*/
#    ifdef WSA_IO_INCOMPLETE
        {WSA_IO_INCOMPLETE,      "Operation still in progress"},
#    endif /*WSA_IO_INCOMPLETE*/
#    ifdef WSA_INVALID_HANDLE
        {WSA_INVALID_HANDLE,     "Invalid handle"},
#    endif /*WSA_INVALID_HANDLE*/
#    ifdef WSA_INVALID_PARAMETER
        {WSA_INVALID_PARAMETER,  "Invalid parameter"},
#    endif /*WSA_INVALID_PARAMETER*/
#    ifdef WSA_NOT_ENOUGH_MEMORY
        {WSA_NOT_ENOUGH_MEMORY,  "Out of memory"},
#    endif /*WSA_NOT_ENOUGH_MEMORY*/
#    ifdef WSA_OPERATION_ABORTED
        {WSA_OPERATION_ABORTED,  "Operation aborted"},
#    endif /*WSA_OPERATION_ABORTED*/
#  endif /*NCBI_OS_MSWIN*/
#  ifdef NCBI_OS_MSWIN
#    define EAI_BASE 0
#  else
#    define EAI_BASE 100000
#  endif /*NCBI_OS_MSWIN*/
#  ifdef EAI_ADDRFAMILY
        {EAI_ADDRFAMILY + EAI_BASE,
                                 "Address family not supported"},
#  endif /*EAI_ADDRFAMILY*/
#  ifdef EAI_AGAIN
        {EAI_AGAIN + EAI_BASE,
                                 "Temporary failure in name resolution"},
#  endif /*EAI_AGAIN*/
#  ifdef EAI_BADFLAGS
        {EAI_BADFLAGS + EAI_BASE,
                                 "Invalid value for lookup flags"},
#  endif /*EAI_BADFLAGS*/
#  ifdef EAI_FAIL
        {EAI_FAIL + EAI_BASE,
                                 "Non-recoverable failure in name resolution"},
#  endif /*EAI_FAIL*/
#  ifdef EAI_FAMILY
        {EAI_FAMILY + EAI_BASE,
                                 "Address family not supported"},
#  endif /*EAI_FAMILY*/
#  ifdef EAI_MEMORY
        {EAI_MEMORY + EAI_BASE,
                                 "Memory allocation failure"},
#  endif /*EAI_MEMORY*/
#  ifdef EAI_NODATA
        {EAI_NODATA + EAI_BASE,
                                 "No address associated with nodename"},
#  endif /*EAI_NODATA*/
#  ifdef EAI_NONAME
        {EAI_NONAME + EAI_BASE,
                                 "Host/service name not known"},
#  endif /*EAI_NONAME*/
#  ifdef EAI_SERVICE
        {EAI_SERVICE + EAI_BASE,
                                 "Service name not supported for socket type"},
#  endif /*EAI_SERVICE*/
#  ifdef EAI_SOCKTYPE
        {EAI_SOCKTYPE + EAI_BASE,
                                 "Socket type not supported"},
#  endif /*EAI_SOCKTYPE*/
#  ifdef NCBI_OS_MSWIN
#    define DNS_BASE 0
#  else
#    define DNS_BASE 200000
#  endif /*NCBI_OS_MSWIN*/
#  ifdef HOST_NOT_FOUND
        {HOST_NOT_FOUND + DNS_BASE,
                                 "Host not found"},
#  endif /*HOST_NOT_FOUND*/
#  ifdef TRY_AGAIN
        {TRY_AGAIN + DNS_BASE,
                                 "DNS server failure"},
#  endif /*TRY_AGAIN*/
#  ifdef NO_RECOVERY
        {NO_RECOVERY + DNS_BASE,
                                 "Unrecoverable DNS error"},
#  endif /*NO_RECOVERY*/
#  ifdef NO_ADDRESS
        {NO_ADDRESS + DNS_BASE,
                                 "No address record found in DNS"},
#  endif /*NO_ADDRESS*/
#  ifdef NO_DATA
        {NO_DATA + DNS_BASE,
                                 "No DNS data of requested type"},
#  endif /*NO_DATA*/

        /* Last dummy entry - must present */
        {0, 0}
    };
    size_t i;

    if (!error)
        return 0;
    for (i = 0;  i < sizeof(errmap) / sizeof(errmap[0]) - 1/*dummy*/;  ++i) {
        if (errmap[i].errnum == error)
            return MSWIN_STRDUP(errmap[i].errtxt);
    }
#  if defined(NCBI_OS_MSWIN)  &&  defined(_UNICODE)
    return UTIL_TcharToUtf8(error > 0 ? _wcserror(error) : L"");
#  else
    return error > 0 ? strerror(error) : "";
#  endif /*NCBI_OS_MSWIN && _UNICODE*/
}


#endif /*NCBI_INCLUDE_STRERROR_C*/
