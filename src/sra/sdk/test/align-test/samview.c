#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <klib/rc.h>
#include <vfs/path.h>
#include <align/align-access.h>
#include <align/bam.h>

typedef struct token_s {
    unsigned start;
    unsigned length;
} token_t;

static
int tokenize_region_string(const char *rgn, token_t *ref, token_t *beg, token_t *end)
{
    unsigned start;
    unsigned length;
    unsigned ends = strlen(rgn);
    
    ref->start = 0; ref->length = 0;
    *beg = *end = *ref;
    
    while (isspace(rgn[ends - 1]))
        --ends;
    
    start = 0;
    while (start != ends && isspace(rgn[start]))
        ++start;
    
    for (length = 0; start + length < ends && rgn[start + length] != ':'; ++length)
        ;
    
    ref->start = start;
    ref->length = length;
    while (isspace(rgn[ref->start + ref->length - 1]))
        --ref->length;
    
    start += length;
    if (start == ends)
        return 1;
    
    ++start;
    while (start < ends && isspace(rgn[start]))
        ++start;
    
    if (start == ends)
        return 0;
    
    for (length = 0; start + length < ends && rgn[start + length] != '-'; ++length)
        ;
    
    beg->start = start;
    beg->length = length;
    while (isspace(rgn[beg->start + beg->length - 1]))
        --beg->length;
    
    start += length;
    if (start == ends)
        return 1;
    
    ++start;
    while (start < ends && isspace(rgn[start]))
        ++start;
    
    if (start == ends)
        return 0;
    
    end->start = start;
    end->length = ends - start;
    while (isspace(rgn[end->start + end->length - 1]))
        --end->length;
    
    return 1;
}

static int parse_region(char *str, const char **ref, uint64_t *beg, uint64_t *end)
{
    token_t ref_tkn;
    token_t beg_tkn;
    token_t end_tkn;
    char buf[16];
    char *endp;
    
    if (!tokenize_region_string(str, &ref_tkn, &beg_tkn, &end_tkn))
        return 0;
    
    *beg = 0;
    *end = 0;
    *ref = &str[ref_tkn.start];
    str[ref_tkn.start + ref_tkn.length] = '\0';
    if (beg_tkn.length == 0)
        return 1;

    memcpy(buf, &str[beg_tkn.start], beg_tkn.length);
    buf[beg_tkn.length] = '\0';
    *beg = strtoul(buf, &endp, 10);
    if (endp - buf != beg_tkn.length)
        return 0;
    
    if (end_tkn.length == 0)
        return 1;
    
    memcpy(buf, &str[end_tkn.start], end_tkn.length);
    buf[end_tkn.length] = '\0';
    *end = strtoul(buf, &endp, 10);
    if (endp - buf != end_tkn.length)
        return 0;
    return 1;
}

static void Usage(void) __attribute__((noreturn));
static void Usage(void)
{
    exit(1);
}

static rc_t OpenFile(const AlignAccessDB **db, const AlignAccessMgr *mgr, const char *file, bool is_indexed)
{
    VPath *bam;
    VPath *bai = NULL;
    rc_t rc;

    rc = VPathMakeSysPath(&bam, file);
    if (rc)
        return rc;

    if (is_indexed) {
        char buf[4096];
        
        snprintf(buf, sizeof(buf), "%s.bai", file);
        rc = VPathMakeSysPath(&bai, buf);
        if (rc == 0)
            rc = AlignAccessMgrMakeIndexBAMDB(mgr, db, bam, bai);
    }
    else
        rc = AlignAccessMgrMakeBAMDB(mgr, db, bam);
    VPathRelease(bai);
    VPathRelease(bam);
    return rc;
}

#define PRINTF(FMT, ARGS...)                                            \
    {   int wrote;                                                      \
        wrote = snprintf(ctx->buffer + ctx->bpos, sizeof(ctx->buffer) - ctx->bpos, FMT, ## ARGS); \
        if (wrote >= sizeof(ctx->buffer) - ctx->bpos) {                 \
            fprintf(stderr, "line too long\n");                         \
            return -1;                                                  \
        }                                                               \
        ctx->bpos += wrote;                                             \
    }

#define PUTCH(CH)                                                       \
    {                                                                   \
        if (1 >= sizeof(ctx->buffer) - ctx->bpos) {                     \
            fprintf(stderr, "line too long\n");                         \
            return -1;                                                  \
        }                                                               \
        ctx->buffer[ctx->bpos++] = CH;                                  \
    }

#define PUTS(STR)                                                       \
    {   unsigned len = strlen(STR);                                     \
        if (len >= sizeof(ctx->buffer) - ctx->bpos) {                   \
            fprintf(stderr, "line too long\n");                         \
            return -1;                                                  \
        }                                                               \
        memcpy(&ctx->buffer[ctx->bpos], STR, len);                      \
        ctx->bpos += len;                                               \
    }

static rc_t CC PrintOptionalFields(void *Ctx, const char tag[2],
                                   const BAMOptData *value)
{
    struct {
        unsigned bpos;
        char buffer[4096];
    } *ctx = Ctx;
    
    PRINTF("%.2s:", tag);
    switch (value->type) {
    case dt_CSTRING:
    case dt_HEXSTRING:
        PRINTF("%c:%.*s", value->type, value->element_count, value->u.asciiz);
        break;
    case dt_INT8:
        PRINTF("i:%i", (int)value->u.i8[0]);
        break;
    case dt_INT16:
        PRINTF("i:%i", (int)value->u.i16[0]);
        break;
    case dt_INT:
        PRINTF("i:%i", (int)value->u.i32[0]);
        break;
    case dt_UINT8:
        PRINTF("i:%u", (unsigned)value->u.u8[0]);
        break;
    case dt_UINT16:
        PRINTF("i:%u", (unsigned)value->u.u16[0]);
        break;
    case dt_UINT:
        PRINTF("i:%u", (unsigned)value->u.u32[0]);
        break;
    case dt_FLOAT32:
        PRINTF("f:%f", value->u.f32[0]);
        break;
    case dt_FLOAT64:
        PRINTF("f:%lf", value->u.f64[0]);
        break;
    case dt_ASCII:
        PRINTF("A:%c", value->u.asciiz[0]);
        break;
    default:
        break;
    }
    PUTCH('\t');
    return 0;
}

static rc_t WriteSam(const BAMFile *bam, const BAMAlignment *rec)
{
    const char *qname;
    uint16_t flags;
    int32_t refid;
    int32_t mref;
    int64_t pos;
    uint8_t mapQ;
    int64_t insert;
    uint32_t length;
    const char *name;
    const uint8_t *qual;
    const BAMRefSeq *refSeq;
    struct {
        unsigned bpos;
        char buffer[4096];
    } Ctx, *ctx = &Ctx;
    unsigned i;
    
    ctx->bpos = 0;
    
    BAMAlignmentGetReadName(rec, &qname);
    BAMAlignmentGetFlags(rec, &flags);
    BAMAlignmentGetMapQuality(rec, &mapQ);
    BAMAlignmentGetRefSeqId(rec, &refid);
    BAMAlignmentGetPosition(rec, &pos);
    
    if (refid < 0)
        name = "*";
    else {
        BAMFileGetRefSeq(bam, refid, &refSeq);
        if (refSeq == NULL) {
            name = "*";
            fprintf(stderr, "warning unknown Reference id: %u\n", (unsigned)refid);
        }
        else
            name = refSeq->name;
    }
    
    PRINTF("%s\t%u\t%s\t%u\t%u\t", qname, flags, name, (unsigned)(pos + 1), mapQ);

    BAMAlignmentGetCigarCount(rec, &length);
    if (length == 0) {
        PUTCH('*');
    }
    else {
        for (i = 0; i != length; ++i) {
            BAMCigarType type;
            uint32_t len;
            
            BAMAlignmentGetCigar(rec, i, &type, &len);
            PRINTF("%u%c", len, type);
        }
    }

    BAMAlignmentGetMateRefSeqId(rec, &mref);
    BAMAlignmentGetMatePosition(rec, &pos);
    BAMAlignmentGetInsertSize(rec, &insert);
    
    if (mref < 0)
        name = "*";
    else if (mref == refid)
        name = "=";
    else {
        BAMFileGetRefSeq(bam, mref, &refSeq);
        if (refSeq == NULL) {
            name = "*";
            fprintf(stderr, "warning unknown Reference id: %u\n", (unsigned)mref);
        }
        else
            name = refSeq->name;
    }
    PRINTF("\t%s\t%u\t%i\t", name, (unsigned)(pos + 1), (int)insert);
    
    BAMAlignmentGetReadLength(rec, &length);
    if (length > 0) {
        if (length >= sizeof(ctx->buffer) - ctx->bpos) {
            fprintf(stderr, "line too long\n");
            return 0;
        }
        BAMAlignmentGetSequence(rec, ctx->buffer + ctx->bpos);
        ctx->bpos += length;
        PUTCH('\t');

        BAMAlignmentGetQuality(rec, &qual);
        for (i = 0; i != length; ++i)
            PRINTF("%c", qual[i] + 33);
        PUTCH('\t');
    }
    else {
        PUTS("*\t*\t");
    }
    BAMAlignmentOptDataForEach(rec, ctx, PrintOptionalFields);
#if 1
    fwrite(ctx->buffer, 1, ctx->bpos - 1, stdout);
    fputc('\n', stdout);
#endif
    
    return 0;
}

static void DumpSam(const BAMFile *bam, AlignAccessAlignmentEnumerator *e)
{
    rc_t rc;
    
    do {
        const BAMAlignment *rec;
        
        AlignAccessAlignmentEnumeratorGetBAMAlignment(e, &rec);
        WriteSam(bam, rec);
        BAMAlignmentRelease(rec);
    } while ((rc = AlignAccessAlignmentEnumeratorNext(e)) == 0);
}

int main (int argc, char *argv[])
{
    const AlignAccessMgr *mgr;
    const AlignAccessDB *db;
    const BAMFile *bam;
    AlignAccessAlignmentEnumerator *align_enum;
    rc_t rc;
    
    if (argc < 2)
        Usage();
    rc = AlignAccessMgrMake(&mgr);
    if (rc) {
        fprintf(stderr, "Failed to create a AlignAccessMgr\n");
        exit(2);
    }
    rc = OpenFile(&db, mgr, argv[1], argc > 2);
    AlignAccessMgrRelease(mgr);
    if (rc) {
        if (argc == 2)
            fprintf(stderr, "Failed to open '%s'\n", argv[1]);
        else
            fprintf(stderr, "Failed to open '%s' and '%s.bai'\n", argv[1], argv[1]);
        exit(3);
    }
    AlignAccessDBExportBAMFile(db, &bam);
    if (argc == 2) {
        rc = AlignAccessDBEnumerateAlignments(db, &align_enum);
        if (rc) {
            fprintf(stderr, "Failed to read '%s'\n", argv[1]);
            exit(4);
        }
        DumpSam(bam, align_enum);
        AlignAccessAlignmentEnumeratorRelease(align_enum);
    }
    else {
        unsigned i;
        
        for (i = 2; i != argc; ++i) {
            uint64_t beg;
            uint64_t end;
            const char *ref;
            
            if (parse_region(argv[i], &ref, &beg, &end)) {
                rc = AlignAccessDBWindowedAlignments(db, &align_enum, ref, beg == 0 ? 0 : beg - 1, end == 0 ? 0 : end - beg);
                if (rc) {
                    fprintf(stderr, "Failed to read '%s' or '%s.bai'\n", argv[1], argv[1]);
                    exit(4);
                }
                DumpSam(bam, align_enum);
                AlignAccessAlignmentEnumeratorRelease(align_enum);
            }
            else {
                fprintf(stderr, "'%s' could not be parsed (should be ref[':'begin['-'end]]\n", argv[1]);
                break;
            }
        }
    }
    BAMFileRelease(bam);
    AlignAccessDBRelease(db);
    return 0;
}
