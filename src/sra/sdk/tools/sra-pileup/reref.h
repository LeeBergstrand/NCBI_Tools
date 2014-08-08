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
#ifndef _h_reref_
#define _h_reref_

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#include <kapp/args.h>

#include <klib/out.h>
#include <klib/rc.h>
#include <klib/log.h>

#include <vdb/manager.h>
#include <vdb/vdb-priv.h>
#include <kdb/manager.h>

rc_t report_on_reference( Args * args, bool short_report );

#endif
