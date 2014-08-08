#ifndef NETCACHE__LOGGING__HPP
#define NETCACHE__LOGGING__HPP
/*  $Id: logging.hpp 368585 2012-07-10 12:26:18Z gouriano $
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
 * Authors:  Pavel Ivanov
 *
 * File Description:
 */


BEGIN_NCBI_SCOPE


void SaveAppCmdLine(const string& cmd_line);
void SetLogFileName(CTempString name);
void ConfigureLogging(CNcbiRegistry* reg, CTempString section);
void InitLogging(void);
void LogNoteThreadsStarted(void);
void FinalizeLogging(void);
void AssignThreadLogging(SSrvThread* thr);
void StartThreadLogging(SSrvThread* thr);
void StopThreadLogging(SSrvThread* thr);
void ReleaseThreadLogging(SSrvThread* thr);
void CheckLoggingFlush(SSrvThread* thr);


END_NCBI_SCOPE

#endif /* NETCACHE__LOGGING__HPP */
