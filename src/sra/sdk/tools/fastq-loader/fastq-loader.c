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

#include "latf-load.vers.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <kapp/main.h>
#include <kapp/args.h>
#include <loader/common-writer.h>
#include <klib/rc.h>
#include <klib/out.h>
#include <klib/log.h>
#include <klib/text.h>
#include <klib/printf.h>
#include <kapp/log-xml.h>
#include <align/writer-refseq.h>

extern rc_t run(char const argv0[], 
                struct CommonWriterSettings* G,
                unsigned countReads, 
                const char* reads[],
                uint8_t qualityOffset,
                const uint8_t defaultReadNumbers[]);

/* MARK: Arguments and Usage */
static char const option_input[] = "input";
static char const option_output[] = "output";
static char const option_tmpfs[] = "tmpfs";
static char const option_qual_compress[] = "qual-quant";
static char const option_cache_size[] = "cache-size";
static char const option_max_err_count[] = "max-err-count";
static char const option_max_rec_count[] = "max-rec-count";
static char const option_platform[] = "platform";
static char const option_quality[] = "quality";
static char const option_read[] = "read";

#define OPTION_INPUT option_input
#define OPTION_OUTPUT option_output
#define OPTION_TMPFS option_tmpfs
#define OPTION_QCOMP option_qual_compress
#define OPTION_CACHE_SIZE option_cache_size
#define OPTION_MAX_ERR_COUNT option_max_err_count
#define OPTION_MAX_REC_COUNT option_max_rec_count
#define OPTION_PLATFORM option_platform
#define OPTION_QUALITY option_quality
#define OPTION_READ option_read

#define ALIAS_INPUT  "i"
#define ALIAS_OUTPUT "o"
#define ALIAS_TMPFS "t"
#define ALIAS_QCOMP "Q"
#define ALIAS_MAX_ERR_COUNT "E"
#define ALIAS_PLATFORM "p"
#define ALIAS_QUALITY "q"
#define ALIAS_READ "r"

static
char const * output_usage[] = 
{
    "Path and Name of the output database.",
    NULL
};

static
char const * tmpfs_usage[] = 
{
    "Path to be used for scratch files.",
    NULL
};

static
char const * qcomp_usage[] = 
{
    "Quality scores quantization level, can be a number (0: none - default, 1: 2bit, 2: 1bit), or a string like '1:10,10:20,20:30,30:-' (which is equivalent to 1).",
    NULL
};

static
char const * cache_size_usage[] = 
{
    "Set the cache size in MB for the temporary indices",
    NULL
};

static
char const * mrc_usage[] = 
{
    "Set the maximum number of records to process from the FASTQ file",
    NULL
};

static
char const * mec_usage[] = 
{
    "Set the maximum number of errors to ignore from the FASTQ file",
    NULL
};

static
char const * use_platform[] = 
{
    "Platform (ILLUMINA, LS454, SOLID, COMPLETE_GENOMICS, HELICOS, PACBIO, IONTORRENT, CAPILLARY)",
    NULL
};

static
char const * use_quality[] = 
{
    "Quality encoding (PHRED_33, PHRED_64)",
    NULL
};

static
char const * use_read[] = 
{
    "Default read number (1 or 2)",
    NULL
};

OptDef Options[] = 
{
    /* order here is same as in param array below!!! */               /* max#,  needs param, required */
    { OPTION_OUTPUT,        ALIAS_OUTPUT,           NULL, output_usage,     1,  true,        true },
    { OPTION_TMPFS,         ALIAS_TMPFS,            NULL, tmpfs_usage,      1,  true,        false },
    { OPTION_QCOMP,         ALIAS_QCOMP,            NULL, qcomp_usage,      1,  true,        false },
    { OPTION_CACHE_SIZE,    NULL,                   NULL, cache_size_usage, 1,  true,        false },
    { OPTION_MAX_REC_COUNT, NULL,                   NULL, mrc_usage,        1,  true,        false },
    { OPTION_MAX_ERR_COUNT, ALIAS_MAX_ERR_COUNT,    NULL, mec_usage,        1,  true,        false },
    { OPTION_PLATFORM,      ALIAS_PLATFORM,         NULL, use_platform,     1,  true,        false },
    { OPTION_QUALITY,       ALIAS_QUALITY,          NULL, use_quality,      1,  true,        true },
/*    { OPTION_READ,          ALIAS_READ,             NULL, use_read,         0,  true,        false },*/
};

const char* OptHelpParam[] =
{
    /* order here is same as in OptDef array above!!! */
    "path",
    "path-to-file",
    "phred-score",
    "mbytes",
    "count",
    "count",
    NULL,
    NULL,
    NULL,
};

rc_t UsageSummary (char const * progname)
{
    return KOutMsg (
        "Usage:\n"
        "\t%s [options] <fastq-file> ...\n"
        "\n"
        "Summary:\n"
        "\tLoad FASTQ formatted data files\n"
        "\n"
        "Example:\n"
        "\t%s -p 454 -o /tmp/SRZ123456 123456-1.fastq 123456-2.fastq\n"
        "\n"
        ,progname, progname);
}

char const UsageDefaultName[] = "fastq-load";

rc_t CC Usage (const Args * args)
{
    rc_t rc;
    int i;
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
    const size_t argsQty = sizeof(Options) / sizeof(Options[0]);

    if (args == NULL)
        rc = RC (rcApp, rcArgv, rcAccessing, rcSelf, rcNull);
    else
        rc = ArgsProgram (args, &fullpath, &progname);
    if (rc)
        progname = fullpath = UsageDefaultName;

    UsageSummary (progname);

    for(i = 0; i < argsQty; i++ ) {
        if( Options[i].required && Options[i].help[0] != NULL ) {
            HelpOptionLine(Options[i].aliases, Options[i].name, OptHelpParam[i], Options[i].help);
        }
    }
    OUTMSG(("\nOptions:\n"));
    for(i = 0; i < argsQty; i++ ) {
        if( !Options[i].required && Options[i].help[0] != NULL ) {
            HelpOptionLine(Options[i].aliases, Options[i].name, OptHelpParam[i], Options[i].help);
        }
    }
    XMLLogger_Usage();
    OUTMSG(("\n"));
    HelpOptionsStandard ();
    HelpVersion (fullpath, KAppVersion());
    return rc;
}

/* MARK: Definitions and Globals */

#define SCHEMAFILE "align/align.vschema"

CommonWriterSettings G;

uint32_t CC KAppVersion (void)
{
    return LATF_LOAD_VERS;
}

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
static void set_pid(void)
{
    G.pid = getpid();
}

static rc_t PathWithBasePath(char rslt[], size_t sz, char const path[], char const base[])
{
    size_t const plen = strlen(path);
    bool const hasBase = base && base[0];
    bool const isBareName = strchr(path, '/') == NULL;
    
    if (isBareName && hasBase) {
        if (string_printf(rslt, sz, NULL, "%s/%s", base, path) == 0)
            return 0;
    }
    else if (plen < sz) {
        strcpy(rslt, path);
        return 0;
    }
    {
        rc_t const rc = RC(rcApp, rcArgv, rcAccessing, rcBuffer, rcInsufficient);
        (void)LOGERR(klogErr, rc, "The path to the file is too long");
        return rc;
    }
}

rc_t CC KMain (int argc, char * argv[])
{
    Args * args;
    rc_t rc;
    char *files[256];
    uint8_t defaultReadNumbers[256];
    char *name_buffer = NULL;
    unsigned next_name = 0;
    unsigned nbsz = 0;
    char const *value;
    char *dummy;
    const XMLLogger* xml_logger = NULL;
    uint8_t qualityOffset;
    
    memset(&G, 0, sizeof(G));
    
    G.mode = mode_Archive;
    G.maxSeqLen = TableWriterRefSeq_MAX_SEQ_LEN;
    G.schemaPath = SCHEMAFILE;
    G.omit_aligned_reads = true;
    G.omit_reference_reads = true;
    G.minMapQual = 0; /* accept all */
    G.tmpfs = "/tmp";
#if _ARCH_BITS == 32
    G.cache_size = ( size_t ) 1 << 30;
#else
    G.cache_size = ( size_t ) 10 << 30;
#endif
    G.maxErrCount = 1000;
    G.acceptNoMatch = true; 
    G.minMatchCount = 0; 
    G.QualQuantizer="0";
    
    set_pid();

    rc = ArgsMakeAndHandle (&args, argc, argv, 2, Options,
                            sizeof Options / sizeof (OptDef), XMLLogger_Args, XMLLogger_ArgsQty);

    while (rc == 0) {
        uint32_t pcount;

        if( (rc = XMLLogger_Make(&xml_logger, NULL, args)) != 0 ) {
            break;
        }
        
        rc = ArgsOptionCount (args, OPTION_TMPFS, &pcount);
        if (rc)
            break;
        if (pcount == 1)
        {
            rc = ArgsOptionValue (args, OPTION_TMPFS, 0, &G.tmpfs);
            if (rc)
                break;
        }
        else if (pcount > 1)
        {
            rc = RC(rcApp, rcArgv, rcAccessing, rcParam, rcExcessive);
            OUTMSG (("Single parameter required\n"));
            MiniUsage (args);
            break;
        }
        
        rc = ArgsOptionCount (args, OPTION_OUTPUT, &pcount);
        if (rc)
            break;
        if (pcount == 1)
        {
            rc = ArgsOptionValue (args, OPTION_OUTPUT, 0, &G.outpath);
            if (rc)
                break;
        }
        else if (pcount > 1)
        {
            rc = RC(rcApp, rcArgv, rcAccessing, rcParam, rcExcessive);
            OUTMSG (("Single output parameter required\n"));
            MiniUsage (args);
            break;
        }
        else {
            rc = RC(rcApp, rcArgv, rcAccessing, rcParam, rcInsufficient);
            OUTMSG (("Output parameter required\n"));
            MiniUsage (args);
            break;
        }
        
        rc = ArgsOptionCount (args, OPTION_QCOMP, &pcount);
        if (rc)
            break;
        if (pcount == 1)
        {
            rc = ArgsOptionValue (args, OPTION_QCOMP, 0, &G.QualQuantizer);
            if (rc)
                break;
        }
        
        rc = ArgsOptionCount (args, OPTION_CACHE_SIZE, &pcount);
        if (rc)
            break;
        if (pcount == 1)
        {
            rc = ArgsOptionValue (args, OPTION_CACHE_SIZE, 0, &value);
            if (rc)
                break;
            G.cache_size = strtoul(value, &dummy, 0) * 1024UL * 1024UL;
            if (G.cache_size == 0  || G.cache_size == ULONG_MAX) {
                rc = RC(rcApp, rcArgv, rcAccessing, rcParam, rcIncorrect);
                OUTMSG (("cache-size: bad value\n"));
                MiniUsage (args);
                break;
            }
        }
        
        G.expectUnsorted = true;
        
        rc = ArgsOptionCount (args, OPTION_MAX_REC_COUNT, &pcount);
        if (rc)
            break;
        if (pcount == 1)
        {
            rc = ArgsOptionValue (args, OPTION_MAX_REC_COUNT, 0, &value);
            if (rc)
                break;
            G.maxAlignCount = strtoul(value, &dummy, 0);
        }
        
        rc = ArgsOptionCount (args, OPTION_MAX_ERR_COUNT, &pcount);
        if (rc)
            break;
        if (pcount == 1)
        {
            rc = ArgsOptionValue (args, OPTION_MAX_ERR_COUNT, 0, &value);
            if (rc)
                break;
            G.maxErrCount = strtoul(value, &dummy, 0);
        }
        
        rc = ArgsOptionCount (args, OPTION_PLATFORM, &pcount);
        if (rc)
            break;
        if (pcount == 1)
        {
            rc = ArgsOptionValue (args, OPTION_PLATFORM, 0, &value);
            if (rc)
                break;
            G.platform = PlatformToId(value);
            if (G.platform == SRA_PLATFORM_UNDEFINED)
            {
                rc = RC(rcApp, rcArgv, rcAccessing, rcParam, rcIncorrect);
                (void)PLOGERR(klogErr, (klogErr, rc, "Invalid platform $(v)",
                            "v=%s", value));
                break;
            }
        }
        else
            G.platform = SRA_PLATFORM_UNDEFINED;

        rc = ArgsOptionCount (args, OPTION_QUALITY, &pcount);
        if (rc)
            break;
        if (pcount == 1)
        {
            rc = ArgsOptionValue (args, OPTION_QUALITY, 0, &value);
            if (rc)
                break;
            if (strcmp(value, "PHRED_33") == 0)
                qualityOffset = 33;
            else if (strcmp(value, "PHRED_64") == 0)
                qualityOffset = 64;
            else
            {
                rc = RC(rcApp, rcArgv, rcAccessing, rcParam, rcIncorrect);
                (void)PLOGERR(klogErr, (klogErr, rc, "Invalid quality encoding $(v)",
                            "v=%s", value));
                break;
            }
        }
        else
            qualityOffset = 0;
            
        rc = ArgsParamCount (args, &pcount);
        if (rc) break;
        if (pcount == 0)
        {
            rc = RC(rcApp, rcArgv, rcAccessing, rcParam, rcInsufficient);
            MiniUsage (args);
            break;
        }
        else if (pcount > sizeof(files)/sizeof(files[0])) {
            rc = RC(rcApp, rcArgv, rcAccessing, rcParam, rcExcessive);
            (void)PLOGERR(klogErr, (klogErr, rc, "$(count) input files is too many, $(max) is the limit",
                        "count=%u,max=%u", (unsigned)pcount, (unsigned)(sizeof(files)/sizeof(files[0]))));
            break;
        }
        else {
            unsigned need = G.inpath ? (strlen(G.inpath) + 1) * pcount : 0;
            unsigned i;
            
            for (i = 0; i < pcount; ++i) {
                rc = ArgsParamValue(args, i, &value);
                if (rc) break;
                need += strlen(value) + 1;
            }
            nbsz = need;
        }
        
        name_buffer = malloc(nbsz);
        if (name_buffer == NULL) {
            rc = RC(rcApp, rcArgv, rcAccessing, rcMemory, rcExhausted);
            break;
        }
        
        rc = ArgsParamCount (args, &pcount);
        if (rc == 0) {
            unsigned i;
            
            for (i = 0; i < pcount; ++i) {
                rc = ArgsParamValue(args, i, &value);
                if (rc) break;

                defaultReadNumbers[i] = 0;
                files[i] = name_buffer + next_name;
                rc = PathWithBasePath(name_buffer + next_name, nbsz - next_name, value, G.inpath);
                if (rc) break;
                next_name += strlen(name_buffer + next_name) + 1;
            }
        }
        else
            break;
        
        rc = run(argv[0], &G, pcount, (char const **)files, qualityOffset, defaultReadNumbers);
        break;
    }
    free(name_buffer);

    value = G.outpath ? strrchr(G.outpath, '/') : "/???";
    if( value == NULL ) {
        value = G.outpath;
    } else {
        value++;
    }
    if (rc) {
        (void)PLOGERR(klogErr, (klogErr, rc, "load failed",
                "severity=total,status=failure,accession=%s,errors=%u", value, G.errCount));
    } else {
        (void)PLOGMSG(klogInfo, (klogInfo, "loaded",
                "severity=total,status=success,accession=%s,errors=%u", value, G.errCount));
    }
    ArgsWhack(args);
    XMLLogger_Release(xml_logger);
    return rc;
}
