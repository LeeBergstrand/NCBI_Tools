#ifndef CTOOLS___CTOOLS__H
#define CTOOLS___CTOOLS__H

/*  $Id: ctools.h 103491 2007-05-04 17:18:18Z kazimird $
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
 * Author:  Anton Lavrentiev
 *
 * File Description:
 *   A bridge between C and C++ Toolkits (C->C++, for use in C only)
 *
 */


/** @addtogroup CToolsBridge
 *
 * @{
 */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
   

/* Reroute C Toolkit error posting into C++ Toolkit error posting facility.
 */
void SetupCToolkitErrPost(void);


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */


/* @} */

#endif  /* CTOOLS___CTOOLS__H */
