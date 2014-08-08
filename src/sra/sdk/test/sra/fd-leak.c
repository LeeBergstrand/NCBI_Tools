#include <kapp/main.h>
#include <kdb/meta.h>
#include <sra/sradb-priv.h>
#include <klib/log.h>
#include <klib/out.h>
#include <stdio.h>
#include <klib/rc.h>
#include <unistd.h>
#include <ulimit.h>
#include <sys/types.h>

ver_t CC KAppVersion (void)
{
    return 0;
}

rc_t CC Usage (const struct Args * args)
{
    return 0;
}

rc_t CC UsageSummary (const char * prog_name)
{
    return 0;
}

const char UsageDefaultName [] = "fd-leak";




rc_t CC KMain(int argc, char *argv[])
{
    rc_t rc = 0;
    pid_t pid = getpid();
    const SRAMgr * mgr;
    char accession[3+8+1 + 2000] = {0};
    int i,j;

    ulimit (4,128);

    if( (rc = SRAMgrMakeRead(&mgr)) != 0 )
    {
        LOGERR(klogErr, rc, "SRA manager");
        return 1;
    }
#if ! USING_THIS_TO_DEVELOP_NOT_AS_A_REGRESSION_TEST && 0
/*     for (j = 1;;) for (i = 1; i < 830; i++) */
    for (j = 1;;)
    {
        for (i = 700; i < 830; j++, i++)
        {
            const SRATable* tbl = NULL;
            snprintf(accession, sizeof(accession), "SRR%06d", i);

#if USING_THIS_TO_DEVELOP_NOT_AS_A_REGRESSION_TEST && 0
            KOutMsg ("pid %d\n",pid);
            KOutMsg ("Opening %s %d\n",accession, j);
#endif
            rc = SRAMgrOpenTableRead(mgr, &tbl, "%s", accession);
            if( rc == 0 )
            {
                if (SRATableRelease(tbl) != 0)
                    KOutMsg("release failed\n");
            }
            else
            {
                if (GetRCState(rc) == rcExhausted)
                    KOutMsg ("%s: out of pids %s\n",accession);

                PLOGERR(klogErr, (klogErr, rc, "SRA refresh $(a)", PLOG_S(a), accession));
            }
        }
        break;  /* without changes, it is once through. */
    }
#else
    for (j = 0, i = 1;j < 200;++j)
    {
        const SRATable* tbl = NULL;
            snprintf(accession, sizeof(accession), "SRR%06d", i);
/*         snprintf(accession, sizeof(accession), "/usr/tmp/killian2/SRR000702/"); */

/*             KOutMsg ("pid %d\n",pid); */
/*             KOutMsg ("Opening %s %d\n",accession, j); */

            rc = SRAMgrOpenTableRead(mgr, &tbl, "%s", accession);
            if( rc == 0 )
            {
                if (SRATableRelease(tbl) != 0)
                    KOutMsg("release failed\n");
            }
            else
            {
                if (GetRCState(rc) == rcExhausted)
/*                 goto die; */
                    KOutMsg ("%s: out of pids %s\n",accession);

                PLOGERR(klogErr, (klogErr, rc, "SRA refresh $(a)", PLOG_S(a), accession));
            }
    }
#endif
    SRAMgrRelease (mgr);
    return 0;
}
