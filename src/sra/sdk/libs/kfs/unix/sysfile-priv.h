/*===========================================================================
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
*/

#ifndef USE_TIMEOUT
#define USE_TIMEOUT false
#endif

#ifndef USE_NO_TIMEOUT_TTY
#define USE_NO_TIMEOUT_TTY false
#endif

#ifndef _h_sysfile_priv_
#define _h_sysfile_priv_

#ifndef _h_kfs_impl_
#include <kfs/impl.h>
#endif

#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * KSysFile
 *  a Unix file
 */
typedef struct KSysFile KSysFile;
struct KSysFile
{
    KFile dad;
    int fd;
#if USE_TIMEOUT
    fd_set fds;
    struct timeval to;
    bool use_to;
#endif
};

/* KSysFileMake
 *  create a new file object
 *  from file descriptor
 */
rc_t KSysFileMake ( KSysFile **fp, int fd, const char *path, bool read_enabled, bool write_enabled );


#ifdef __cplusplus
}
#endif

#endif /*  _h_sysfile_priv_ */
