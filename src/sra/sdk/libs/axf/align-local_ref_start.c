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
#include <vdb/extern.h>

#include <vdb/xform.h>
#include <vdb/database.h>
#include <vdb/table.h>
#include <vdb/cursor.h>
#include <vdb/vdb-priv.h>
#include <insdc/insdc.h>
#include <klib/data-buffer.h>
#include <klib/rc.h>
#include <sysalloc.h>

#include <bitstr.h>

#include "ref-tbl.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct LocalRefStart LocalRefStart;
struct LocalRefStart
{
    uint32_t max_seq_len;
};

static
void CC LocalRefStartWhack ( void *obj )
{
    LocalRefStart * self = obj;
    if ( self != NULL )
    {
        free ( self );
    }
}

static
rc_t LocalRefStartMake ( LocalRefStart **objp, const VTable *tbl, const VCursor *native_curs )
{
    rc_t rc;

    /* create the object */
    LocalRefStart *obj = malloc ( sizeof * obj );
    if ( obj == NULL ) {
        rc = RC ( rcXF, rcFunction, rcConstructing, rcMemory, rcExhausted );
    } else {
	const VCursor *curs=NULL;
         /* open the reference table cursor*/
        if( (rc = AlignRefTableCursor(tbl, native_curs, &curs, NULL)) == 0 ){
                uint32_t itmp;
                if(  (rc = VCursorAddColumn(curs, &itmp, "(U32)MAX_SEQ_LEN")) == 0 || GetRCState(rc) == rcExists)  {
                    const void *base;
                    uint32_t row_len;
                    rc = VCursorCellDataDirect(curs, 1, itmp, NULL, &base, NULL, &row_len);
                    if(rc == 0) {
                        assert(row_len == 1);
                        memcpy(&obj->max_seq_len, base, 4);
                    }
                }
                if( GetRCObject(rc) == rcColumn && GetRCState(rc) == rcNotFound ) {
                    obj->max_seq_len = 0;
                    rc = 0;
                }
                VCursorRelease(curs);
                if(rc == 0){
                        *objp = obj;
                        return 0;
                }
        }
        free ( obj );
    }
    return rc;
}

/*
 function INSDC:coord:zero NCBI:align:local_ref_start ( U64 global_ref_start )
 */
static
rc_t CC align_local_ref_start ( void *data, const VXformInfo *info, void *Dst, const void *Src, uint64_t elem_count )
{
    LocalRefStart const *self = (void const *)data;
    INSDC_coord_zero *dst=Dst;
    uint64_t const *global_ref_start = Src;
    unsigned i;
    
    for (i = 0; i != elem_count; ++i) {
        dst[i] = global_ref_start[i] % self->max_seq_len;
    }
    return 0;
}

VTRANSFACT_IMPL ( NCBI_align_local_ref_start, 1, 0, 0 ) ( const void *self, const VXfactInfo *info,
    VFuncDesc *rslt, const VFactoryParams *cp, const VFunctionParams *dp )
{
    LocalRefStart *fself;
    rc_t rc = LocalRefStartMake ( & fself, info -> tbl, (const VCursor*)info->parms  );
    if ( rc == 0 )
    {
        rslt -> self = fself;
        rslt -> whack = LocalRefStartWhack;
        rslt -> u . af = align_local_ref_start;
        rslt -> variant = vftArray;
    }

    return rc;
}
