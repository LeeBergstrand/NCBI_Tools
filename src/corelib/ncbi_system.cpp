/* $Id: ncbi_system.cpp 380144 2012-11-08 15:51:30Z ivanov $
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
 * Authors:  Vladimir Ivanov, Denis Vakatov, Anton Lavrentiev
 *
 * File Description:  System functions
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbimtx.hpp>
#include <corelib/ncbi_system.hpp>
#include <corelib/ncbi_safe_static.hpp>
#include <corelib/error_codes.hpp>
#include <corelib/ncbierror.hpp>
#include "ncbisys.hpp"


#define NCBI_USE_ERRCODE_X   Corelib_System

#if defined(NCBI_OS_UNIX)
#  include <sys/mman.h>
#  if defined(NCBI_OS_SOLARIS)
#    include <corelib/ncbifile.hpp>
#    ifndef VALID_ATTR // correlated with madvise() prototype
extern "C" {
    extern int madvise(caddr_t, size_t, int);
}
#    endif
#  endif //NCBI_OS_SOLARIS
#  include <sys/time.h>
#  include <sys/resource.h>
#  include <sys/times.h>
#  include <sys/types.h>
#  include <limits.h>
#  include <time.h>
#  include <unistd.h>
#  if defined(NCBI_OS_BSD) || defined(NCBI_OS_DARWIN)
#    include <sys/sysctl.h>
#  endif //NCBI_OS_BSD || NCBI_OS_DARWIN
#  if defined(NCBI_OS_IRIX)
#    include <sys/sysmp.h>
#  endif //NCBI_OS_IRIX
#  define USE_SETMEMLIMIT
#  define USE_SETCPULIMIT
#  define HAVE_MADVISE 1
#endif //NCBI_OS_UNIX

#ifdef NCBI_OS_DARWIN
extern "C" {
#  include <mach/mach.h>
#  include <mach/mach_host.h>
#  include <mach/host_info.h>
} /* extern "C" */
#endif //NCBI_OS_DARWIN

#ifdef USE_SETCPULIMIT
#  include <signal.h>
#endif //USE_SETCPULIMIT

#ifdef NCBI_OS_MSWIN
#  include <corelib/ncbidll.hpp>
#  include <crtdbg.h>
#  include <stdlib.h>
#  include <windows.h>

struct SProcessMemoryCounters
{
    DWORD  size;
    DWORD  page_fault_count;
    SIZE_T peak_working_set_size;
    SIZE_T working_set_size;
    SIZE_T quota_peak_paged_pool_usage;
    SIZE_T quota_paged_pool_usage;
    SIZE_T quota_peak_nonpaged_pool_usage;
    SIZE_T quota_nonpaged_pool_usage;
    SIZE_T pagefile_usage;
    SIZE_T peak_pagefile_usage;
};

#endif //NCBI_OS_MSWIN

#ifdef NCBI_OS_CYGWIN
#  define WINVER 0x0500
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif //NCBI_OS_CYGWIN


BEGIN_NCBI_SCOPE


// MIPSpro 7.3 workarounds:
//   1) it declares set_new_handler() in both global and std:: namespaces;
//   2) it apparently gets totally confused by `extern "C"' inside a namespace.
#if defined(NCBI_COMPILER_MIPSPRO)
#  define set_new_handler std::set_new_handler
#else
extern "C" {
    static void s_ExitHandler(void);
    static void s_SignalHandler(int sig);
}
#endif //NCBI_COMPILER_MIPSPRO


#ifdef NCBI_OS_UNIX

DEFINE_STATIC_FAST_MUTEX(s_ExitHandler_Mutex);
static bool                  s_ExitHandlerIsSet  = false;
static ELimitsExitCode       s_ExitCode          = eLEC_None;
static CSafeStaticPtr<CTime> s_TimeSet;
static size_t                s_MemoryLimitSoft   = 0;
static size_t                s_MemoryLimitHard   = 0;
static size_t                s_CpuTimeLimit      = 0;
static char*                 s_ReserveMemory     = 0;
static TLimitsPrintHandler   s_PrintHandler      = 0;
static TLimitsPrintParameter s_PrintHandlerParam = 0;


#if !defined(CLK_TCK)  &&  defined(CLOCKS_PER_SEC)
#  define CLK_TCK CLOCKS_PER_SEC
#endif


// Routine to be called at the exit from application
// It is not async-safe, so using it with SetCpuLimits() can lead to coredump
// and program crash. Be aware.
//
static void s_ExitHandler(void)
{
    CFastMutexGuard LOCK(s_ExitHandler_Mutex);

    // Free reserved memory
    if ( s_ReserveMemory ) {
        delete[] s_ReserveMemory;
        s_ReserveMemory = 0;
    }

    // User defined dump
    if ( s_PrintHandler ) {
        size_t limit_size; 

        switch ( s_ExitCode ) {
        case eLEC_Memory: {
            limit_size = s_MemoryLimitSoft;
            break;
        }
        case eLEC_Cpu: {
            limit_size = s_CpuTimeLimit;
            break;
        }
        default:
            return;
        }
        // Call user's print handler
        (*s_PrintHandler)(s_ExitCode, limit_size, s_TimeSet.Get(), 
                          s_PrintHandlerParam);
        return;
    }

    // Standard dump
    switch ( s_ExitCode ) {
        
    case eLEC_Memory:
        {
            ERR_POST_X(1, "Memory heap limit exceeded in allocating memory " \
                          "by operator new (" << s_MemoryLimitSoft << " bytes)");
            break;
        }
        
    case eLEC_Cpu:
        {
            ERR_POST_X(2, "CPU time limit exceeded (" << s_CpuTimeLimit << " sec)");
            tms buffer;
            if (times(&buffer) == (clock_t)(-1)) {
                ERR_POST_X(3, "Error in getting CPU time consumed by program");
                break;
            }
            clock_t tick = sysconf(_SC_CLK_TCK);
#if defined(CLK_TCK)
            if (!tick  ||  tick == (clock_t)(-1))
                tick = CLK_TCK;
#elif defined(CLOCKS_PER_SEC)
            if (!tick  ||  tick == (clock_t)(-1))
                tick = CLOCKS_PER_SEC;
#endif
            if (tick == (clock_t)(-1))
                tick = 0;
            LOG_POST_X(4, "\tuser CPU time   : " << 
                          buffer.tms_utime/(tick ? tick : 1) <<
                          (tick ? " sec" : " tick"));
            LOG_POST_X(5, "\tsystem CPU time : " << 
                          buffer.tms_stime/(tick ? tick : 1) <<
                          (tick ? " sec" : " tick"));
            LOG_POST_X(6, "\ttotal CPU time  : " <<
                          (buffer.tms_stime + buffer.tms_utime)/(tick ? tick : 1) <<
                          (tick ? " sec" : " tick"));
            break;
        }

    default:
        return;
    }
    
    // Write program's time
    CTime ct(CTime::eCurrent);
    CTime et(2000, 1, 1);
    et.AddSecond((int) (ct.GetTimeT() - s_TimeSet->GetTimeT()));
    LOG_POST_X(7, "Program's time: " << Endm <<
                  "\tstart limit - " << s_TimeSet->AsString() << Endm <<
                  "\ttermination - " << ct.AsString() << Endm);
    et.SetFormat("h:m:s");
    LOG_POST_X(8, "\texecution   - " << et.AsString());
}


// Set routine to be called at the exit from application
//
static bool s_SetExitHandler(TLimitsPrintHandler handler, 
                             TLimitsPrintParameter parameter)

{
    // Set exit routine if it not set yet
    CFastMutexGuard LOCK(s_ExitHandler_Mutex);
    if ( !s_ExitHandlerIsSet ) {
        if (atexit(s_ExitHandler) != 0) {
            return false;
        }
        s_ExitHandlerIsSet = true;
        s_TimeSet->SetCurrent();
        
        // Store new print handler and its parameter
        s_PrintHandler = handler;
        s_PrintHandlerParam = parameter;
        
        // Reserve some memory (10Kb) to allow the diagnostic API
        // print messages on exit if memory limit is set.
        s_ReserveMemory = new char[10*1024];
    }
    return true;
}
    
#endif //NCBI_OS_UNIX



/////////////////////////////////////////////////////////////////////////////
//
// SetHeapLimit
//

#ifdef USE_SETMEMLIMIT

// Handler for operator new
static void s_NewHandler(void)
{
    s_ExitCode = eLEC_Memory;
    exit(-1);
}


bool SetMemoryLimit(size_t max_size,
                    TLimitsPrintHandler handler, 
                    TLimitsPrintParameter parameter)
{
    if (s_MemoryLimitSoft == max_size) {
        return true;
    }
    if (!s_SetExitHandler(handler, parameter)) {
        return false;
    }
    CFastMutexGuard LOCK(s_ExitHandler_Mutex);
    
    rlimit rl;
    if ( max_size ) {
        set_new_handler(s_NewHandler);
        rl.rlim_cur = rl.rlim_max = max_size;
    } else {
        set_new_handler(0);
        rl.rlim_cur = rl.rlim_max = RLIM_INFINITY;
    }
    if (setrlimit(RLIMIT_DATA, &rl) != 0) {
        return false;
    }
#  if !defined(NCBI_OS_SOLARIS)
    if (setrlimit(RLIMIT_AS, &rl) != 0) {
        return false;
    }
#  endif //NCBI_OS_SOLARIS

    s_MemoryLimitSoft = max_size;
    s_MemoryLimitHard = max_size;
    if ( max_size ) {
        set_new_handler(s_NewHandler);
    } else {
        set_new_handler(0);
    }
    return true;
}


bool SetMemoryLimitSoft(size_t max_size,
                        TLimitsPrintHandler handler, 
                        TLimitsPrintParameter parameter)
{
    if (s_MemoryLimitSoft == max_size) {
        return true;
    }
    if (!s_SetExitHandler(handler, parameter)) {
        return false;
    }
    CFastMutexGuard LOCK(s_ExitHandler_Mutex);

    rlimit rl;
    if (getrlimit(RLIMIT_DATA, &rl) != 0) {
        return false;
    }
    if ( max_size ) {
        rl.rlim_cur = max_size;
    } else {
        rl.rlim_cur = RLIM_INFINITY;
    }
    if (setrlimit(RLIMIT_DATA, &rl) != 0) {
        return false;
    }
#  if !defined(NCBI_OS_SOLARIS)
    rlimit rlas;
    if (getrlimit(RLIMIT_AS, &rlas) != 0) {
        return false;
    }
    rl.rlim_max = rlas.rlim_max;
    if (setrlimit(RLIMIT_AS, &rl) != 0) {
        return false;
    }
#  endif //NCBI_OS_SOLARIS

    s_MemoryLimitSoft = max_size;
    if ( max_size ) {
        set_new_handler(s_NewHandler);
    } else {
        set_new_handler(0);
    }
    return true;
}


bool SetMemoryLimitHard(size_t max_size,
                        TLimitsPrintHandler handler, 
                        TLimitsPrintParameter parameter)
{
    if (s_MemoryLimitHard == max_size) {
        return true;
    }
    if (!s_SetExitHandler(handler, parameter)) {
        return false;
    }
    CFastMutexGuard LOCK(s_ExitHandler_Mutex);

    size_t cur_soft_limit = 0;
    rlimit rl;
    if (getrlimit(RLIMIT_DATA, &rl) != 0) {
        return false;
    }
    if ( max_size ) {
        rl.rlim_max = max_size;
        if (rl.rlim_cur > max_size) {
            rl.rlim_cur = max_size;
        }
        cur_soft_limit = rl.rlim_cur;
    } else {
        rl.rlim_max = RLIM_INFINITY;
    }
    if (setrlimit(RLIMIT_DATA, &rl) != 0) {
        return false;
    }
#  if !defined(NCBI_OS_SOLARIS)
    rlimit rlas;
    if (getrlimit(RLIMIT_AS, &rlas) != 0) {
        return false;
    }
    if ( max_size ) {
        rlas.rlim_max = max_size;
        // Descrease current soft limit of the virtual memory
        // to the size of data segment -- min(DATA,AS). 
        if (rlas.rlim_cur > cur_soft_limit) {
            rlas.rlim_cur = cur_soft_limit;
        }
        // And use this size as current value for the soft limit
        // in the print handler 
        cur_soft_limit = rlas.rlim_cur;
    } else {
        rlas.rlim_max = RLIM_INFINITY;
    }
    if (setrlimit(RLIMIT_AS, &rlas) != 0) {
        return false;
    }
#  endif //NCBI_OS_SOLARIS

    s_MemoryLimitSoft = cur_soft_limit;
    s_MemoryLimitHard = max_size;
    if ( max_size ) {
        set_new_handler(s_NewHandler);
    } else {
        set_new_handler(0);
    }
    return true;
}


// @deprecated
bool SetHeapLimit(size_t max_size,
                  TLimitsPrintHandler handler, 
                  TLimitsPrintParameter parameter)
{
    if (s_MemoryLimitSoft == max_size) { 
        return true;
    }
    if (!s_SetExitHandler(handler, parameter)) {
        return false;
    }
    CFastMutexGuard LOCK(s_ExitHandler_Mutex);
    
    rlimit rl;
    if ( max_size ) {
        rl.rlim_cur = rl.rlim_max = max_size;
    } else {
        rl.rlim_cur = rl.rlim_max = RLIM_INFINITY;
    }
    if (setrlimit(RLIMIT_DATA, &rl) != 0) {
        return false;
    }
    s_MemoryLimitSoft = max_size;
    if ( max_size ) {
        set_new_handler(s_NewHandler);
    } else {
        set_new_handler(0);
    }
    return true;
}


#else

bool SetMemoryLimit(size_t max_size, 
                    TLimitsPrintHandler handler, 
                    TLimitsPrintParameter parameter)
{
  return false;
}

bool SetMemoryLimitSoft(size_t max_size, 
                    TLimitsPrintHandler handler, 
                    TLimitsPrintParameter parameter)
{
  return false;
}

bool SetMemoryLimitHard(size_t max_size, 
                    TLimitsPrintHandler handler, 
                    TLimitsPrintParameter parameter)
{
  return false;
}

bool SetHeapLimit(size_t max_size, 
                  TLimitsPrintHandler handler, 
                  TLimitsPrintParameter parameter)
{
  return false;
}

#endif //USE_SETMEMLIMIT



/////////////////////////////////////////////////////////////////////////////
//
// SetCpuTimeLimit
//

#ifdef USE_SETCPULIMIT

static void s_SignalHandler(int _DEBUG_ARG(sig))
{
    _ASSERT(sig == SIGXCPU);
    _VERIFY(signal(SIGXCPU, SIG_IGN) != SIG_ERR);
    s_ExitCode = eLEC_Cpu;
    // _exit() does not go over atexit() chain, so just call registered
    // handler directly. Be aware that it should be async-safe!
    if (s_ExitHandlerIsSet) {
        s_ExitHandler();
    }
    _exit(-1);
}

bool SetCpuTimeLimit(unsigned int          max_cpu_time,
                     unsigned int          terminate_delay_time,
                     TLimitsPrintHandler   handler, 
                     TLimitsPrintParameter parameter)
{
    if (s_CpuTimeLimit == max_cpu_time) {
        return true;
    }
    if (!s_SetExitHandler(handler, parameter)) {
        return false;
    }
    CFastMutexGuard LOCK(s_ExitHandler_Mutex);

    rlimit rl;
    if ( max_cpu_time ) {
        rl.rlim_cur = max_cpu_time;
        rl.rlim_max = max_cpu_time + terminate_delay_time;
    } else {
        rl.rlim_cur = rl.rlim_max = RLIM_INFINITY;
    }
    if (setrlimit(RLIMIT_CPU, &rl) != 0) {
        return false;
    }
    s_CpuTimeLimit = max_cpu_time;

    // Set signal handler for SIGXCPU
    if (signal(SIGXCPU, s_SignalHandler) == SIG_ERR) {
        return false;
    }

    return true;
}

#else

bool SetCpuTimeLimit(unsigned int          max_cpu_time,
                     unsigned int          terminate_delay_time,
                     TLimitsPrintHandler   handler, 
                     TLimitsPrintParameter parameter)
{
    return false;
}

#endif //USE_SETCPULIMIT


// @deprecated
bool SetCpuTimeLimit(size_t                max_cpu_time,
                     TLimitsPrintHandler   handler, 
                     TLimitsPrintParameter parameter,
                     size_t                terminate_delay_time)
{
    return SetCpuTimeLimit((unsigned int)max_cpu_time,
                           (unsigned int)terminate_delay_time,
                           handler, parameter);
}


/////////////////////////////////////////////////////////////////////////////
//
/// System information
//

unsigned int GetCpuCount(void)
{
#if defined(NCBI_OS_MSWIN)  ||  defined(NCBI_OS_CYGWIN)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (unsigned int) si.dwNumberOfProcessors;

#elif defined(NCBI_OS_DARWIN)
    host_basic_info_data_t hinfo;
    mach_msg_type_number_t hinfo_count = HOST_BASIC_INFO_COUNT;
    kern_return_t rc;

    rc = host_info(mach_host_self(), HOST_BASIC_INFO,
                  (host_info_t)&hinfo, &hinfo_count);

    if (rc != KERN_SUCCESS) {
        return 1;
    }
    return hinfo.avail_cpus;

#elif defined(NCBI_OS_UNIX)
    long nproc = 0;
# if defined(_SC_NPROC_ONLN)
    nproc = sysconf(_SC_NPROC_ONLN);
# elif defined(_SC_NPROCESSORS_ONLN)
    nproc = sysconf(_SC_NPROCESSORS_ONLN);
# elif defined(NCBI_OS_BSD) || defined(NCBI_OS_DAWRIN)
    size_t len = sizeof(nproc);
    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    if (sysctl(mib, 2, &nproc, &len, 0, 0) < 0 || len != sizeof(nproc))
        nproc = -1;
# endif //UNIX_FLAVOR
    return nproc <= 0 ? 1 : (unsigned int) nproc;
#else
    return 1;
#endif //NCBI_OS_...
}


bool GetCurrentProcessTimes(double* user_time, double* system_time)
{
#if defined(NCBI_OS_MSWIN)
    // Each FILETIME structure contains the number of 100-nanosecond time units.
    FILETIME ft_creation, ft_exit, ft_kernel, ft_user;
    if (! ::GetProcessTimes(GetCurrentProcess(),
                            &ft_creation, &ft_exit,
                            &ft_kernel, &ft_user) ) {
            return false;
    }
    if (system_time) {
        *system_time = (ft_kernel.dwLowDateTime + ((Uint8)ft_kernel.dwHighDateTime<<32)) * 1.0e-7;
    }
    if (user_time) {
        *user_time = (ft_user.dwLowDateTime + ((Uint8)ft_user.dwHighDateTime<<32)) * 1.0e-7;
    }

#elif defined(NCBI_OS_UNIX)

    tms buf;
    clock_t t = times(&buf);
    if ( t == (clock_t)(-1) ) {
        return false;
    }
    clock_t tick = sysconf(_SC_CLK_TCK);
#if defined(CLK_TCK)
    if (!tick  ||  tick == (clock_t)(-1))
        tick = CLK_TCK;
#elif defined(CLOCKS_PER_SEC)
    if (!tick  ||  tick == (clock_t)(-1))
        tick = CLOCKS_PER_SEC;
#endif
    if (tick == (clock_t)(-1)) {
        return false;
    }
    if (system_time) {
        *system_time = (double)buf.tms_stime / tick;
    }
    if (user_time) {
        *user_time = (double)buf.tms_utime / tick;
    }
#endif
    return true;
}



/////////////////////////////////////////////////////////////////////////////
//
// Memory utilities
//

unsigned long GetVirtualMemoryPageSize(void)
{
    static unsigned long ps = 0;

    if (!ps) {
#if defined(NCBI_OS_MSWIN)  ||  defined(NCBI_OS_CYGWIN)
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        ps = (unsigned long) si.dwPageSize;
#elif defined(NCBI_OS_UNIX) 
#  if   defined(_SC_PAGESIZE)
#    define NCBI_SC_PAGESIZE _SC_PAGESIZE
#  elif defined(_SC_PAGE_SIZE)
#    define NCBI_SC_PAGESIZE _SC_PAGE_SIZE
#  elif defined(NCBI_SC_PAGESIZE)
#    undef  NCBI_SC_PAGESIZE
#  endif
#  ifndef   NCBI_SC_PAGESIZE
        long x = 0;
#  else
        long x = sysconf(NCBI_SC_PAGESIZE);
#    undef  NCBI_SC_PAGESIZE
#  endif
        if (x <= 0) {
#  ifdef HAVE_GETPAGESIZE
            if ((x = getpagesize()) <= 0)
                return 0;
#  endif
            return 0;
        }
        ps = x;
#endif //NCBI_OS_...
    }
    return ps;
}


unsigned long GetVirtualMemoryAllocationGranularity(void)
{
    static unsigned long ag = 0;

    if (!ag) {
#if defined(NCBI_OS_MSWIN)  ||  defined(NCBI_OS_CYGWIN)
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        ag = (unsigned long) si.dwAllocationGranularity;
#else
        ag = GetVirtualMemoryPageSize();
#endif //NCBI_OS_MSWIN || NCBI_OS_CYGWIN
    }
    return ag;
}


Uint8 GetPhysicalMemorySize(void)
{
#if defined(NCBI_OS_MSWIN)  ||  defined(NCBI_OS_CYGWIN)

    MEMORYSTATUSEX st;
    st.dwLength = sizeof(st);
    if ( GlobalMemoryStatusEx(&st) ) {
        return st.ullTotalPhys;
    }

#elif defined(NCBI_OS_UNIX)  &&  defined(_SC_PHYS_PAGES)

    unsigned long num_pages = sysconf(_SC_PHYS_PAGES);
    if (long(num_pages) != -1L) {
        return GetVirtualMemoryPageSize() * Uint8(num_pages);
    }

#elif defined(NCBI_OS_BSD)  ||  defined(NCBI_OS_DARWIN)

    size_t   len;
    int      mib[2];
#  ifdef HW_MEMSIZE
    uint64_t physmem;
    mib[1] = HW_MEMSIZE;
#  else
    /* Native BSD, may be truncated */
    int      physmem;
    mib[1] = HW_PHYSMEM;
#  endif /*HW_MEMSIZE*/
    mib[0] = CTL_HW;
    len = sizeof(physmem);
    if (sysctl(mib, 2, &physmem, &len, 0, 0) == 0  &&  len == sizeof(physmem)){
        return Uint8(physmem);
    }

#  ifdef NCBI_OS_DARWIN
    {
        /* heavier fallback */
        struct vm_statistics vm_stat;
        mach_port_t my_host = mach_host_self();
        mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
        if (host_statistics(my_host, HOST_VM_INFO,
                            (integer_t*) &vm_stat, &count) == KERN_SUCCESS) {
            return GetVirtualMemoryPageSize() *
                (Uint8(vm_stat.free_count) + Uint8(vm_stat.active_count) +
                 Uint8(vm_stat.inactive_count) + Uint8(vm_stat.wire_count));
        }
    }
#  endif //NCBI_OS_DARWIN

#elif defined(NCBI_OS_IRIX)

    struct rminfo rmi;
    if (sysmp(MP_SAGET, MPSA_RMINFO, &rmi, sizeof(rmi)) >= 0) {
        return GetVirtualMemoryPageSize() * Uint8(rmi.physmem);
    }

#endif

    return 0;
}


bool GetMemoryUsage(size_t* total, size_t* resident, size_t* shared)
{
    size_t scratch;
    if ( !total )    { total    = &scratch; }
    if ( !resident ) { resident = &scratch; }
    if ( !shared )   { shared   = &scratch; }
#if defined(NCBI_OS_MSWIN)
    try {
        // Load PSAPI dynamic library -- it should exist on MS-Win NT/2000/XP
        CDll psapi_dll("psapi.dll", CDll::eLoadNow, CDll::eAutoUnload);
        BOOL (STDMETHODCALLTYPE FAR * dllGetProcessMemoryInfo)
            (HANDLE process, SProcessMemoryCounters& counters, DWORD size) = 0;
        dllGetProcessMemoryInfo
            = psapi_dll.GetEntryPoint_Func("GetProcessMemoryInfo",
                                           &dllGetProcessMemoryInfo);
        if (dllGetProcessMemoryInfo) {
            SProcessMemoryCounters counters;
            dllGetProcessMemoryInfo(GetCurrentProcess(), counters,
                                    sizeof(counters));
            *total    = counters.quota_paged_pool_usage +
                        counters.quota_nonpaged_pool_usage;
            *resident = counters.working_set_size;
            *shared   = 0;
            return true;
        }
    } catch (CException) {
        // Just catch all exceptions from CDll
    }
#elif defined(NCBI_OS_LINUX)
    CNcbiIfstream statm("/proc/self/statm");
    if (statm) {
        unsigned long page_size = GetVirtualMemoryPageSize();
        statm >> *total >> *resident >> *shared;
        *total    *= page_size;
        *resident *= page_size;
        *shared   *= page_size;
        return true;
    }
#elif defined(NCBI_OS_SOLARIS)
    Int8 len = CFile("/proc/self/as").GetLength();
    if (len > 0) {
        *total    = (size_t)len;
        *resident = (size_t)len; // conservative estimate
        *shared   = 0;           // does this info exist anywhere?
        return true;
    }
#elif defined(HAVE_GETRUSAGE)
#  define _DIV0(a, b) ((a) / ((b) ? (b) : 1))
    // BIG FAT NOTE:  getrusage() seems to use different size units
    struct rusage ru;
    memset(&ru, '\0', sizeof(ru));
    if (getrusage(RUSAGE_SELF, &ru) == 0  &&  ru.ru_maxrss > 0) {
        struct tms t;
        memset(&t, '\0', sizeof(t));
        if (times(&t) != (clock_t)(-1)) {
            clock_t ticks = t.tms_utime + t.tms_stime;
            *total    = _DIV0(ru.ru_ixrss + ru.ru_idrss + ru.ru_isrss, ticks);
            *resident = _DIV0(ru.ru_idrss,                             ticks);
            *shared   = _DIV0(ru.ru_ixrss,                             ticks);
            return true;
        }
    }
#  undef _DIV0
#endif
    return false;
}


#if !defined(HAVE_MADVISE)
bool MemoryAdvise(void*, size_t, EMemoryAdvise) {
    return false;
}
#else  /* HAVE_MADVISE */
bool MemoryAdvise(void* addr, size_t len, EMemoryAdvise advise)
{
    if ( !addr /*|| !len*/ ) {
        ERR_POST_X(11, "Memory address is not specified");
        CNcbiError::Set(CNcbiError::eBadAddress);
        return false;
    }
    int adv;
    switch (advise) {
    case eMADV_Normal:
        adv = MADV_NORMAL;     break;
    case eMADV_Random:
        adv = MADV_RANDOM;     break;
    case eMADV_Sequential:
        adv = MADV_SEQUENTIAL; break;
    case eMADV_WillNeed:
        adv = MADV_WILLNEED;   break;
    case eMADV_DontNeed:
        adv = MADV_DONTNEED;   break;
    case eMADV_DontFork:
        #if defined(MADV_DONTFORK)
            adv = MADV_DONTFORK;
            break;
        #else
            ERR_POST_X_ONCE(12, Warning << "MADV_DONTFORK not supported");
            CNcbiError::Set(CNcbiError::eNotSupported);
            return false;
        #endif        
    case eMADV_DoFork:
        #if defined(MADV_DOFORK)
            adv = MADV_DOFORK;
            break;
        #else
            ERR_POST_X_ONCE(12, Warning << "MADV_DOTFORK not supported");
            CNcbiError::Set(CNcbiError::eNotSupported);
            return false;
        #endif        
    case eMADV_Mergeable:
        #if defined(MADV_MERGEABLE)
            adv = MADV_MERGEABLE;
            break;
        #else
            ERR_POST_X_ONCE(12, Warning << "MADV_MERGEABLE not supported");
            CNcbiError::Set(CNcbiError::eNotSupported);
            return false;
        #endif        
    case eMADV_Unmergeable:
        #if defined(MADV_UNMERGEABLE)
            adv = MADV_UNMERGEABLE;
            break;
        #else
            ERR_POST_X_ONCE(12, Warning << "MADV_UNMERGEABLE not supported");
            CNcbiError::Set(CNcbiError::eNotSupported);
            return false;
        #endif        
    default:
        _TROUBLE;
        return false;
    }
    // Conversion type of "addr" to char* -- Sun Solaris fix
    if ( madvise((char*) addr, len, adv) != 0 ) {
        int x_errno = errno; \
        ERR_POST_X(13, "madvise() failed: " << 
                   _T_STDSTRING(NcbiSys_strerror(x_errno)));
        CNcbiError::SetErrno(errno = x_errno);
        return false;
    }
    return true;
}
#endif  /* HAVE_MADVISE */



/////////////////////////////////////////////////////////////////////////////
//
// Sleep
//

void SleepMicroSec(unsigned long mc_sec, EInterruptOnSignal onsignal)
{
#if defined(NCBI_OS_MSWIN)  ||  defined(NCBI_OS_CYGWIN)

    // Unlike some of its (buggy) Unix counterparts, MS-Win's Sleep() is safe
    // to use with 0, which causes the current thread to sleep at most until
    // the end of the current timeslice (and only if the CPU is not idle).
    Sleep((mc_sec + 500) / 1000);

#elif defined(NCBI_OS_UNIX)

#  if defined(HAVE_NANOSLEEP)
    struct timespec delay, unslept;
    memset(&unslept, 0, sizeof(unslept));
    delay.tv_sec  =  mc_sec / kMicroSecondsPerSecond;
    delay.tv_nsec = (mc_sec % kMicroSecondsPerSecond) * 1000;
    while (nanosleep(&delay, &unslept) < 0) {
        if (errno != EINTR  ||  onsignal == eInterruptOnSignal)
            break;
        delay = unslept;
        memset(&unslept, 0, sizeof(unslept));
    }
#  elif defined(HAVE_USLEEP)
    unsigned int sec  = mc_sec / kMicroSecondsPerSecond;
    unsigned int usec = mc_sec % kMicroSecondsPerSecond;
    if (sec) {
        while ((sec = sleep(sec)) > 0) {
            if (onsignal == eInterruptOnSignal)
                return;
        }
    }
    usleep(usec);
#  else
    // Portable but ugly.
    // Most implementations of select() do not modify timeout to reflect
    // the amount of time unslept;  but some (e.g. Linux) do.  Also, on
    // some platforms it can be interrupted by a signal, but not on others.
    // OTOH, we don't want to sandwich this with gettimeofday(), either.
    struct timeval delay;
    delay.tv_sec  = mc_sec / kMicroSecondsPerSecond;
    delay.tv_usec = mc_sec % kMicroSecondsPerSecond;
    while (select(0, (fd_set*) 0, (fd_set*) 0, (fd_set*) 0, &delay) < 0) {
#    if defined(SELECT_UPDATES_TIMEOUT)
        if (errno != EINTR  ||  onsignal == eInterruptOnSignal)
#    endif
            break;
    }
#  endif //HAVE FINE SLEEP API

#endif //NCBI_OS_...
}


void SleepMilliSec(unsigned long ml_sec, EInterruptOnSignal onsignal)
{
#if defined(NCBI_OS_MSWIN)  ||  defined(NCBI_OS_CYGWIN)
    Sleep(ml_sec);
#elif defined(NCBI_OS_UNIX)
    SleepMicroSec(ml_sec * 1000, onsignal);
#endif //NCBI_OS_...
}


void SleepSec(unsigned long sec, EInterruptOnSignal onsignal)
{
    SleepMicroSec(sec * kMicroSecondsPerSecond, onsignal);
}



/////////////////////////////////////////////////////////////////////////////
///
/// Suppress Diagnostic Popup Messages (all MS-Win specific)
///

#ifdef NCBI_OS_MSWIN

static bool s_EnableSuppressSystemMessageBox  = true;
static bool s_DoneSuppressSystemMessageBox    = false;
static bool s_SuppressedDebugSystemMessageBox = false;

// Handler for "Unhandled" exceptions
static LONG CALLBACK _SEH_Handler(EXCEPTION_POINTERS* ep)
{
    // Always terminate a program
    return EXCEPTION_EXECUTE_HANDLER;
}

#endif //NCBI_OS_MSWIN

extern void SuppressSystemMessageBox(TSuppressSystemMessageBox mode)
{
#ifdef NCBI_OS_MSWIN
    if ( !s_EnableSuppressSystemMessageBox ) {
        return;
    }
    // System errors
    if ( (mode & fSuppress_System) == fSuppress_System ) {
       SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX |
                    SEM_NOOPENFILEERRORBOX);
    }
    // Runtime library
    if ( (mode & fSuppress_Runtime) == fSuppress_Runtime ) {
        _set_error_mode(_OUT_TO_STDERR);
    }
    // Debug library
    if ( !IsDebuggerPresent() ) {
        if ( (mode & fSuppress_Debug) == fSuppress_Debug ) {
            _CrtSetReportFile(_CRT_WARN,   _CRTDBG_FILE_STDERR);
            _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDERR);
            _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
            _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
            s_SuppressedDebugSystemMessageBox = true;
        }
    }
    // Exceptions
    if ( (mode & fSuppress_Exception) == fSuppress_Exception ) {
        SetUnhandledExceptionFilter(_SEH_Handler);
    }
    s_DoneSuppressSystemMessageBox = true;
#endif //NCBI_OS_MSWIN
}


extern void DisableSuppressSystemMessageBox(void)
{
#ifdef NCBI_OS_MSWIN
    if ( s_DoneSuppressSystemMessageBox ) {
        ERR_POST_X(9, Critical << "SuppressSystemMessageBox() already called");
    }
    s_EnableSuppressSystemMessageBox = false;
#endif //NCBI_OS_MSWIN
}


extern bool IsSuppressedDebugSystemMessageBox(void)
{
#ifdef NCBI_OS_MSWIN
    return s_DoneSuppressSystemMessageBox  &&
        s_SuppressedDebugSystemMessageBox;
#else
    return false;
#endif
}


END_NCBI_SCOPE
