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

#ifndef BAM_LOAD_REFERENCE_WRITER_H_
#define BAM_LOAD_REFERENCE_WRITER_H_ 1

#include <vdb/manager.h>
#include <vdb/database.h>
#include <klib/data-buffer.h>

#include <align/writer-reference.h>
#include "alignment-writer.h"

typedef struct s_reference {
    const ReferenceMgr *mgr;
    const ReferenceSeq *rseq;
    int64_t lastRefId;
    int32_t lastOffset;
    unsigned curPos;
    unsigned endPos;
    unsigned length;
    KDataBuffer coverage;
    KDataBuffer mismatches;
    KDataBuffer indels;
    KDataBuffer pri_align;
    KDataBuffer sec_align;
    KDataBuffer pri_overlap;
    KDataBuffer sec_overlap;
    bool out_of_order;
    char last_id[256];
} Reference;

rc_t ReferenceInit(Reference *self, const VDBManager *mgr, VDatabase *db);
rc_t ReferenceSetFile(Reference *self, const char *id, uint64_t length, uint8_t const md5[16]);
rc_t ReferenceVerify(Reference const *self, char const id[], uint64_t length, uint8_t const md5[16]);
rc_t ReferenceGet1stRow(Reference const *self, int64_t *refID, char const refName[]);
rc_t ReferenceAddAlignId(Reference *self,
                         int64_t align_id,
                         bool is_primary
                         );
rc_t ReferenceRead(Reference *self, AlignmentRecord *data, uint64_t pos,
                   uint32_t const rawCigar[], uint32_t cigCount,
                   char const seqDNA[], uint32_t seqLen, uint32_t *matches);
rc_t ReferenceWhack(Reference *self, bool commit);

#endif
