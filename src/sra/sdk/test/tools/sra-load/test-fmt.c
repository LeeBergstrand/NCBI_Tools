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
#include <kapp/main.h>
#include <klib/rc.h>
#include <kapp/log.h>
#include <vdb/types.h>
#include <sra/wsradb.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct TestLoaderFmt TestLoaderFmt;

#define SRALOADERFMT_IMPL TestLoaderFmt
#include "loader-fmt.h"
#include "test-load.vers.h"
#include "sra-writer.h"

struct TestLoaderFmt {
    SRALoaderFmt dad; /* base type -> must be first in struct */ 

    const SRALoaderConfig* config;

    /* tmp file buffer data */
    const char* file_name;
    uint32_t spots;

    const SRAWriter* ld_data;
};

static
rc_t TestLoaderFmt_Spots(TestLoaderFmt* self, const char* line, size_t len)
{
    rc_t rc = 0;
    unsigned long int ss = 0;
    char* end = NULL;

    rc = RC(rcSRA, rcFormatter, rcParsing, rcData, rcCorrupt);
    errno = 0;
    ss = strtoul(line, &end, 10);
    if( errno != 0 ) {
        PLOGERR((klogErr, rc, "spot count: $(errno)", PLOG_S(errno), strerror(errno)));
    } else if( ss == 0 ) {
        LOGERR(klogErr, rc, "no spots");
    } else if( end - line - len != 0 ) {
        PLOGERR((klogErr, rc, "invalid line format: $(t)", PLOG_C(t), *end));
    } else {
        rc = 0;
        end++;
        self->spots = ss;
        PLOGMSG((klogInfo, "spot count: $(spots)", PLOG_U32(spots), self->spots));
    }
    return rc;
}

static
rc_t TestLoaderFmt_ReadLine(TestLoaderFmt* self, const char* line, size_t len)
{
    rc_t rc = 0;
    if( len == 0 ) {
        RC(rcSRA, rcFormatter, rcParsing, rcData, rcEmpty);
        LOGERR(klogErr, rc, "spot line");
    } else {
        PLOGMSG((klogInfo, "read: '$(read)'", "read=%.*s", len, line));
    }
    return rc;
}

static
rc_t TestLoaderFmtWriteDataFile(TestLoaderFmt* self, const SRALoaderFile* file)
{
    rc_t rc = 0;
    uint32_t spot = 0, spots = 0;
    const char* line;
    size_t line_len;

    if( (rc = SRALoaderFileName(file, &self->file_name)) != 0 ) {
        LOGERR(klogErr, rc, "Failure to obtain name of the opened file");
    } else if( (rc = SRALoaderFileReadline(file, (const void**)&line, &line_len)) != 0 || (line == NULL) ) {
        rc = rc ? rc : RC(rcSRA, rcFormatter, rcParsing, rcData, rcExhausted);
        LOGERR(klogErr, rc, "reading 1st line");
    } else if( (rc = TestLoaderFmt_Spots(self, line, line_len)) != 0 ) {
        LOGERR(klogErr, rc, "reading spot count");
    } else if( (rc = SRAWriter_WriteDefaults(self->ld_data)) != 0 ) {
        LOGERR(klogErr, rc, "writing defaults");
    }
    for(spot = 0; rc == 0 && spot < self->spots; spot++) {
        if( (rc = SRALoaderFileReadline(file, (const void**)&line, &line_len)) != 0 || (line == NULL) ) {
            rc = rc ? rc : RC(rcSRA, rcFormatter, rcParsing, rcData, rcExhausted);
            PLOGERR((klogErr, rc, "$(file): reading $(spot) spot line", PLOG_2(PLOG_S(file),PLOG_U32(spot)), self->file_name, spot + 1));
        } else if( (rc = TestLoaderFmt_ReadLine(self, line, line_len)) != 0 ) {
            PLOGERR((klogErr, rc, "$(file): spot $(spot)", PLOG_2(PLOG_S(file),PLOG_U32(spot)), self->file_name, spot + 1));
        } else {
            rc = Quitting();
        }
        if( rc == 0 && self->config->spots_to_run > 0 && spots >= self->config->spots_to_run ) {
            PLOGMSG((klogInfo, "Spot limit reached $(n) of $(m)", PLOG_2(PLOG_U32(n),PLOG_I64(m)), spots, self->config->spots_to_run));
            break;
        }
    }
    return rc;
}

/* ======================================================================
* SRALoaderFmt Interface/Table Methods
* ======================================================================
*/
static
rc_t TestLoaderFmtVersion( const TestLoaderFmt*self, uint32_t *vers, const char** name )
{
    if( self == NULL || vers == NULL || name == NULL ) {
        return RC(rcSRA, rcFormatter, rcValidating, rcParam, rcNull);
    }
    *vers = TEST_PARSE_VERS;
    *name = "Test";
    return 0;
}

static
rc_t TestLoaderFmtDestroy(TestLoaderFmt* self)
{
    if(self != NULL) {
        free(self);
    }
    return 0;
}

static
rc_t TestLoaderFmtWriteData(TestLoaderFmt* self, struct SRATable* table, uint32_t argc, const SRALoaderFile* const argv[])
{
    rc_t rc = 0;
    uint32_t idx = 0;

    if( self == NULL || table == NULL || argc == 0 || argv == NULL ) {
        return RC(rcSRA, rcFormatter, rcWriting, rcParam, rcNull);
    }

    rc = SRAWriter_Make(&self->ld_data, self->config, table);

    for(idx = 0; rc == 0 && idx < argc; idx ++) {
        rc = TestLoaderFmtWriteDataFile(self, argv[idx]);
    }
    SRAWriter_Whack(self->ld_data);
    self->ld_data = NULL;
    return rc;
}

static SRALoaderFmt_vt_v1 vtTestLoaderFmt = 
{
    /* interface version 1.0 */
    1,
    0,
    TestLoaderFmtDestroy,
    TestLoaderFmtVersion,
    TestLoaderFmtWriteData
};

rc_t TestLoaderFmtMake(SRALoaderFmt** self, const SRALoaderConfig* config)
{
    rc_t rc = 0;
    TestLoaderFmt* obj = NULL;

    if( self == NULL || config == NULL ) {
        return RC(rcSRA, rcFormatter, rcConstructing, rcSelf, rcNull);
    }
    *self = NULL;
    obj = calloc(1, sizeof(TestLoaderFmt));
    if(obj == NULL) {
        return RC(rcSRA, rcFormatter, rcConstructing, rcMemory, rcExhausted);
    }
    if( (rc = SRALoaderFmtInit(&obj->dad, (const SRALoaderFmt_vt*)&vtTestLoaderFmt)) == 0 ) {
        obj->config = config;
    }
    if(rc == 0) {
        *self = &obj->dad;
    } else {
        free(obj);
    }
    return rc;
}

rc_t SRALoaderFmtMake(SRALoaderFmt **self, const SRALoaderConfig *config)
{
    return TestLoaderFmtMake(self, config);
}
