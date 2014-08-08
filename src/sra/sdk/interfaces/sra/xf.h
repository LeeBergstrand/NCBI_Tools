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

#ifndef _h_sra_xf_
#define _h_sra_xf_

#ifdef __cplusplus
extern "C" {
#endif

#if ! defined EXPORT_LATCH && defined _LIBRARY
#define XF_EXTERN LIB_EXPORT
#define EXPORT_LATCH 1
#else
#define XF_EXTERN LIB_IMPORT
#endif

#ifndef _h_klib_extern_
#include <klib/extern.h>
#endif

typedef void (*xf_function_t)();

XF_EXTERN const xf_function_t * register_axf_functions(void);
XF_EXTERN const xf_function_t * register_waxf_functions(void);

XF_EXTERN const xf_function_t * register_sraxf_functions(void);
XF_EXTERN const xf_function_t * register_wsraxf_functions(void);

XF_EXTERN const xf_function_t * register_wgsxf_functions(void);
XF_EXTERN const xf_function_t * register_wwgsxf_functions(void);

XF_EXTERN const xf_function_t * register_vxf_functions(void);
XF_EXTERN const xf_function_t * register_vxfentrez_functions(void);
XF_EXTERN const xf_function_t * register_wvxf_functions(void);

/* legacy API */
#define vxf_functions   register_vxf_functions()
#define sraxf_functions register_sraxf_functions()

#ifdef __cplusplus
}
#endif

#endif /* _h_sra_xf_ */
