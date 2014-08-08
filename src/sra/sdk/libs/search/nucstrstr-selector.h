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

#if defined(__linux__)  &&  !defined(NUCSTRSTR_NO_ASM)
#  ifdef __x86_64__
#    ifdef NCBI_PROFILED_BUILD
#      define NUCSTRSTR_ASM_IMPLEMENTATION "nucstrstr-icc-x86_64-prof.pic.s"
#    elif defined(__OPTIMIZE__)
#      define NUCSTRSTR_ASM_IMPLEMENTATION "nucstrstr-icc-x86_64-rel.pic.s"
#    else
#      define NUCSTRSTR_ASM_IMPLEMENTATION "nucstrstr-icc-x86_64-dbg.pic.s"
#    endif
#  elif defined(__i386__)
#    ifdef NCBI_PROFILED_BUILD
#      define NUCSTRSTR_ASM_IMPLEMENTATION "nucstrstr-icc-i386-prof.pic.s"
#    elif defined(__OPTIMIZE__)
#      define NUCSTRSTR_ASM_IMPLEMENTATION "nucstrstr-icc-i386-rel.pic.s"
#    else
#      define NUCSTRSTR_ASM_IMPLEMENTATION "nucstrstr-icc-i386-dbg.pic.s"
#    endif
#  endif
#endif
