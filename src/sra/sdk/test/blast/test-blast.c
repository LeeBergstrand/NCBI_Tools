#include <ncbi/vdb-blast.h> /* VdbBlastMgr */
#include <kapp/main.h> /* KMain */
#include <klib/log.h> /* LOGMSG */

#include <ctype.h> /* isascii */
#include <string.h> /* memset */

#include <common/test_assert.h>

const char UsageDefaultName[] = "test-blast";
ver_t CC KAppVersion(void) { return 0; }

/******************************************************************************/

#define TODO -1

#define TEST(r) status = VdbBlastRunSetAddRun(set, r); if (status) return TODO;

static rc_t _testVdbBlastRunSetAddRun(VdbBlastRunSet *set)
{
    uint32_t status = eVdbBlastNoErr;

    TEST("SRR002749"); /* -q  72 /panfs/traces01/sra0/SRR/000002 2 reads */
    TEST("SRR363367"); /* -q  72 /panfs/traces01/sra0/SRR/000354 3 reads */
    TEST("SRR357545"); /* -q 104 /panfs/traces01/sra0/SRR/000349 2 reads */
    TEST("SRR004165"); /*    672 /panfs/traces01/sra0/SRR/000004 2 reads */
    TEST("SRR005380"); /*   1196 /panfs/traces01/sra0/SRR/000005 2 reads */
    TEST("SRR035640"); /*   2544 /panfs/traces01/sra0/SRR/000034 1 read */
    TEST("SRR038009"); /*   3268 /panfs/traces01/sra0/SRR/000037/ */
    TEST("SRR006901"); /*   4616 /panfs/traces01/sra0/SRR/000006 */
    TEST("SRR040026"); /*   4836 /panfs/traces01/sra0/SRR/000039/ */
    TEST("SRR042432"); /*   6172 /panfs/traces01/sra0/SRR/000041/ */
    TEST("SRR034219"); /*   7972 /panfs/traces01/sra0/SRR/000033/ */
    TEST("SRR035883"); /*  10996 /panfs/traces01/sra0/SRR/000035/ */
    TEST("SRR039772"); /*  14036 /panfs/traces01/sra0/SRR/000038/ */
    TEST("SRR014242"); /*  14204 /panfs/traces01/sra0/SRR/000013/ */
    TEST("SRR004046"); /*  15224 /panfs/traces01/sra0/SRR/000003 */
    TEST("SRR001588"); /*  15640 /panfs/traces01/sra0/SRR/000001 */
    TEST("SRR037169"); /*  21392 /panfs/traces01/sra0/SRR/000036/ */
    TEST("SRR015015"); /*  24560 /panfs/traces01/sra0/SRR/000014/ */
    TEST("SRR015443"); /*  27380 /panfs/traces01/sra0/SRR/000015/ */
    TEST("SRR012173"); /*  65488 /panfs/traces01/sra0/SRR/000011/ */
    TEST("SRR030951"); /*  68224 /panfs/traces01/sra0/SRR/000030/ */
    TEST("SRR009982"); /*  69164 /panfs/traces01/sra0/SRR/000009 */
    TEST("SRR007420"); /*  72960 /panfs/traces01/sra0/SRR/000007 */
    TEST("SRR000757"); /*  77860 /panfs/traces01/sra0/SRR/000000 */
    TEST("/panfs/traces01/sra0/SRR/000002/SRR002934"); /* 79812 */
    TEST("SRR008963"); /*  86020 /panfs/traces01/sra0/SRR/000008 */
    TEST("SRR012932"); /*  99644 /panfs/traces01/sra0/SRR/000012/ */
    TEST("SRR010862"); /* 121328 /panfs/traces01/sra0/SRR/000010 */
    TEST("SRR032900"); /* 123644 /panfs/traces01/sra0/SRR/000032/ */
    TEST("SRR029142"); /* 192684 /panfs/traces01/sra0/SRR/000028/ */
    TEST("SRR023839"); /* 212484 /panfs/traces01/sra0/SRR/000023/ */
    TEST("SRR026674"); /* 265688 /panfs/traces01/sra0/SRR/000026/ */
    TEST("SRR032345"); /* 340284 /panfs/traces01/sra0/SRR/000031/ */
    TEST("SRR029961"); /* 402416 /panfs/traces01/sra0/SRR/000029/ */
    TEST("SRR041658"); /*4434168 /panfs/traces01/sra0/SRR/000040/ */

    TEST("SRR393572"); /* 3 spots */

    return 0;
}

#if 0
   
rc_t CC KMain (int argc, char *argv[]) {
    const VDBManager *mgr;
    VSchema *schema;
    const VTable *tbl = NULL;
    Args *args = NULL;
    rc_t rc = ArgsMakeAndHandle(&args, argc, argv, 0);
    if (rc)
    {   return rc; }
    rc = VDBManagerMakeRead(&mgr, NULL);
    if (rc)
    {   return rc; }
    rc = VDBManagerMakeSRASchema(mgr, &schema);
    if (rc)
    {   return rc; }
    LOGMSG(klogWarn, "calling VDBManagerOpenTableRead");
    rc = VDBManagerOpenTableRead
        (mgr, &tbl, schema, "/panfs/traces01/sra0/SRR/000000/SRR000757");
    if (rc) {
        LOGERR(klogErr, rc, "VDBManagerOpenTableRead failure");
        return rc;
    }
    return 0;
}
#endif

typedef struct Blast {
    VdbBlastMgr *mgr;
    VdbBlastRunSet *set;
} Blast;
static uint32_t BlastInit(Blast *self, VdbBlastRunSet **set, ...) {
    bool done = false;
    uint32_t status = eVdbBlastNoErr;
    va_list args;

    va_start(args, set);

    assert(self && set);
    memset(self, 0, sizeof *self);

    self->mgr = VdbBlastInit(&status);
    assert(self->mgr && status == eVdbBlastNoErr);

    self->set = VdbBlastMgrMakeRunSet(self->mgr, &status, 6, false);
    assert(self->set && status == eVdbBlastNoErr);

    *set = self->set;

    while (!done) {
        const char *p = NULL;
        const char *path = va_arg(args, const char*);
        if (path == NULL)
        {   break; }
        for (p = path; *p; ++p) {
            if (!isascii(*p)) {
                done = true;
                break;
            }
        }

        if (!done) {
            status = VdbBlastRunSetAddRun(self->set, path);
            if (status != eVdbBlastNoErr)
            {   break; }
        }
    }

    va_end(args);

    return status;
}

static void BlastFini(Blast *self) {
    assert(self);

    VdbBlastMgrRelease(self->mgr);
    self->mgr = NULL;

    VdbBlastRunSetRelease(self->set);
    self->set = NULL;
}

static rc_t testVdbBlastRunSetAddRun(void)
{
    rc_t rc = 0;

    uint32_t status = eVdbBlastNoErr;
    VdbBlastMgr *mgr = NULL;
    VdbBlastMgr *mgr2 = NULL;
    VdbBlastRunSet *set = NULL;
    VdbBlastRunSet *set2 = NULL;

    VdbBlastMgrRelease(mgr);

    mgr = VdbBlastInit(&status);
    assert(mgr && status == eVdbBlastNoErr);

    mgr2 = VdbBlastMgrAddRef(mgr);

    VdbBlastMgrRelease(mgr2);
    mgr2 = NULL;

    VdbBlastRunSetRelease(set);

    set = VdbBlastMgrMakeRunSet(mgr, &status, 5, true);
    assert(set && status == eVdbBlastNoErr);

    set2 = VdbBlastRunSetAddRef(set);

    VdbBlastRunSetRelease(set2);
    set2 = NULL;

    rc = _testVdbBlastRunSetAddRun(set);

    VdbBlastRunSetRelease(set);
    set = NULL;

    VdbBlastMgrRelease(mgr);
    mgr = NULL;

    return rc;
}

/*define ADD_RUN(path) if (status == eVdbBlastNoErr) \
    status = VdbBlastRunSetAddRun(self.set, path); */

static rc_t testVdbBlastRunSetGetTotalLength(void) {
    Blast self; VdbBlastRunSet *set = NULL;
    uint32_t status = BlastInit(&self, &set, "SRR002749", "SRR363367", 0);

    if (status == eVdbBlastNoErr) {
        uint64_t num = VdbBlastRunSetGetTotalLength(set, &status);
        if (status) {
            PLOGMSG(klogInfo, (klogInfo,
                "$(func): VdbBlastRunSetGetTotalLength failure",
                "func=%s", __func__));
        }
        else {
            uint64_t numa = VdbBlastRunSetGetTotalLengthApprox(set);
            assert(num == numa);
            PLOGMSG(klogInfo, (klogInfo,
                "$(func): TotalLength = $(num)", "func=%s,num=%lu",
                __func__, num));
        }
    }

    BlastFini(&self);

    return status == eVdbBlastNoErr ? 0 : TODO;
}

static rc_t testVdbBlastRunSetGetTotalLengthExpensive(void) {
    Blast self; VdbBlastRunSet *set = NULL;
    uint32_t status = BlastInit(&self, &set, "SRR004165", "SRR005380", 0);

    if (status == eVdbBlastNoErr) {
        uint64_t num = VdbBlastRunSetGetTotalLength(set, &status);
        if (status) {
            assert(status == eVdbBlastTooExpensive);
            PLOGMSG(klogInfo, (klogInfo,
                "$(func): VdbBlastRunSetGetTotalLength TooExpensive",
                "func=%s", __func__));
            num = VdbBlastRunSetGetTotalLengthApprox(set);
            PLOGMSG(klogInfo, (klogInfo,
                "$(func): TotalLengthApprox = $(num)", "func=%s,num=%lu",
                __func__, num));
            status = eVdbBlastNoErr;
        }
        else { assert(0); }
    }

    BlastFini(&self);

    return status == eVdbBlastNoErr ? 0 : TODO;
}

static rc_t testVdbBlastRunSetGetNumSequences(void)
{
    Blast self; VdbBlastRunSet *set = NULL;
    uint32_t status = BlastInit(&self, &set, "SRR002749", "SRR004165", 0);

    if (status == eVdbBlastNoErr) {
        uint64_t num = VdbBlastRunSetGetNumSequences(set, &status);
        if (status) {
            PLOGMSG(klogInfo, (klogInfo,
                "$(func): VdbBlastRunSetGetNumSequences failure",
                "func=%s", __func__));
        }
        else {
            uint64_t numa = VdbBlastRunSetGetNumSequencesApprox(set);
            assert(num == numa);
            PLOGMSG(klogInfo, (klogInfo, 
                "$(func): NumSequences = $(num)",
                "func=%s,num=%lu", __func__, num));
        }
    }

    BlastFini(&self);

    return status == eVdbBlastNoErr ? 0 : TODO;
}

static rc_t testVdbBlastRunSetGetMaxSeqLen(void)
{
    Blast self; VdbBlastRunSet *set = NULL;
    uint32_t status = BlastInit(&self, &set, "SRR004165", "SRR002749", 0);

    if (status == eVdbBlastNoErr) {
        uint64_t l = VdbBlastRunSetGetMinSeqLen(set);
        if (status) {
            PLOGMSG(klogInfo, (klogInfo,
                "$(func): VdbBlastRunSetGetMinSeqLen failure",
                "func=%s", __func__));
        }
        else {
            PLOGMSG(klogInfo, (klogInfo, 
                "$(func): MinSeqLen = $(num)",
                "func=%s,num=%lu", __func__, l));
        }
    }

    if (status == eVdbBlastNoErr) {
        uint64_t maxL = VdbBlastRunSetGetMaxSeqLen(set);
        if (status) {
            PLOGMSG(klogInfo, (klogInfo,
                "$(func): VdbBlastRunSetGetMaxSeqLen failure",
                "func=%s", __func__));
        }
        else {
            PLOGMSG(klogInfo, (klogInfo, 
                "$(func): MaxSeqLen = $(num)",
                "func=%s,num=%lu", __func__, maxL));
        }
    }

    if (status == eVdbBlastNoErr) {
        uint64_t l = VdbBlastRunSetGetAvgSeqLen(set);
        if (status) {
            PLOGMSG(klogInfo, (klogInfo,
                "$(func): VdbBlastRunSetGetAvgSeqLen failure",
                "func=%s", __func__));
        }
        else {
            PLOGMSG(klogInfo, (klogInfo, 
                "$(func): AvgSeqLen = $(num)",
                "func=%s,num=%lu", __func__, l));
        }
    }

    {
        int i = 0;
        char b[28];
        memset(b, '-', sizeof b);
        b[sizeof b - 1] = '\0';
        for (i = 0; i < sizeof b; ++i) {
            VdbBlastRunSetGetName(set, &status, b, i);
            PLOGMSG(klogInfo, (klogInfo,
                "$(func): VdbBlastRunSetGetName($(s))=$(name)",
                "func=%s,s=%d,name=%s", __func__, i, b));
            if (status == eVdbBlastNoErr)
            {   break; }
        }
    }

    BlastFini(&self);

    return status == eVdbBlastNoErr ? 0 : TODO;
}

static rc_t testVdbBlast2naReader(void)
{
    Blast self; VdbBlastRunSet *set = NULL;
    uint32_t status = BlastInit(&self, &set, "SRR002749", 0);
    VdbBlast2naReader *reader = NULL, *reader2 = NULL;
    uint64_t n = 0;
    uint8_t buf[10] = "";

    assert(status == eVdbBlastNoErr);

    reader = VdbBlastRunSetMake2naReader(set, &status, 0);
    assert(status == eVdbBlastNoErr);
    reader2 = VdbBlast2naReaderAddRef(reader);
    assert(status == eVdbBlastNoErr);

    VdbBlast2naReaderRelease(reader2);
    reader2 = NULL;

    n = VdbBlast2naReaderRead(reader, &status, NULL, buf, sizeof buf);

    VdbBlast2naReaderRelease(reader);
    reader = NULL;

    BlastFini(&self);

    return status == eVdbBlastNoErr ? 0 : TODO;
}

static rc_t testVdbBlastRunSetGetReadName(void)
{
    Blast self; VdbBlastRunSet *set = NULL;
    uint32_t status = BlastInit(&self, &set,
#if 1
        "SRR002749", /* TECHNICAL, TYPE_BIOLOGICAL */
        "SRR363367", /* TECHNICAL, _TYPE_TECHNICAL, SRA_READ_TYPE_BIOLOGICAL */
        "SRR035640", /* BIOLOGICAL */
#endif
        "SRR038009", /* BIOLOGICAL, TYPE_BIOLOGICAL */
        0);
    if (status == eVdbBlastNoErr) {
        bool done = false;
        int i = 0;
        char n[64];
        memset(n, '-', sizeof n);
        n[sizeof n - 1] = '\0';
        while (!done) {
            size_t r = VdbBlastRunSetGetReadName(set, i, n, sizeof n);
            PLOGMSG(klogInfo, (klogInfo,
                "$(func): $(need) = VdbBlastRunSetGetReadName($(id)) = $(n)",
                "func=%s,need=%d,id=%d,n=%s", __func__, r, i, n));
            switch (i) {
                case 0: case 1: case 256: case 526: case 527:
                case 37613: case 37614: case 37615: case 37616: case 65521:
                    ++i; break;
                case 2: i = 9; break;
                case 9: i = 256; break;
                case 257: i = 526; break;
                case 528: i = 37613; break;
                case 37617: i = 65521; break;
                default: done = true; break;
            }
        }
    }
    BlastFini(&self);

    return status == eVdbBlastNoErr ? 0 : TODO;
}

#if 0
static rc_t testVdbBlastRunSetGetReadLength(void)
{
    uint64_t read_id = 0;
    Blast self; VdbBlastRunSet *set = NULL;
    uint32_t status = BlastInit(&self, &set, "SRR002749", 0);

    for (read_id = 0; read_id > 0; ++read_id) {
        uint64_t l = VdbBlastRunSetGetReadLength(set, read_id);
    }

    BlastFini(&self);

    return status == eVdbBlastNoErr ? 0 : TODO;
}
#endif

rc_t CC KMain (int argc, char *argv[]) {
    bool all = 0;
    Args *args = NULL;
    rc_t rc = ArgsMakeAndHandle(&args, argc, argv, 0);
    if (rc)
    {   return rc; }

    if (all && rc == 0)
    {   rc = testVdbBlastRunSetAddRun(); }

    if (all && rc == 0)
    {   rc = testVdbBlastRunSetGetTotalLength(); }

    if (all && rc == 0)
    {   rc = testVdbBlastRunSetGetTotalLengthExpensive(); }

    if (all && rc == 0)
    {   rc = testVdbBlastRunSetGetNumSequences(); } /* check with nreads */

    if (all && rc == 0)
    {   rc = testVdbBlastRunSetGetMaxSeqLen(); }

    if (rc == 0)
    {   rc = testVdbBlast2naReader(); }

    if (all && rc == 0)
    {   rc = testVdbBlastRunSetGetReadName(); }

    if (all && rc == 0)
    {   rc = testVdbBlastRunSetGetReadName(); }

#if 0
    if (rc == 0)
    {   rc = testVdbBlastRunSetGetReadLength(); }
#endif

    /* ???  
    VdbBlastRunSetLastUpdatedDate
    GetReadLength comment ??? */

    ArgsWhack(args);
    args = NULL;

    return rc;
}

rc_t CC UsageSummary(const char* progname) { return 0; }
rc_t CC Usage(const Args* args) { return 0; }

/* EOF */
