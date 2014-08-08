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
#ifndef _h_sra_fuse_accessor_
#define _h_sra_fuse_accessor_

#include <unistd.h>

#include <klib/rc.h>

typedef struct SAccessor SAccessor;

typedef rc_t (AccessorRead)(const SAccessor* cself, char* buf, size_t size, off_t offset, size_t* num_read);
typedef rc_t (AccessorRelease)(const SAccessor* cself);

rc_t SAccessor_Make(const SAccessor** cself, size_t size, const char* name, AccessorRead* read, AccessorRelease* release);

rc_t SAccessor_GetName(const SAccessor* cself, const char** name);

rc_t SAccessor_Read(const SAccessor* cself, char* buf, size_t size, off_t offset, size_t* num_read);

rc_t SAccessor_Release(const SAccessor* cself);

#endif /* _h_sra_fuse_accessor_ */
