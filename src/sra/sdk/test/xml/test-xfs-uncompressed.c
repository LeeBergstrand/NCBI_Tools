/*
test-xfs-gz will:
- OpenXTocDirRead(BASE, XML)
- Read INNER_FILE1 : gzipped
  and write to OUTNAME created in the current directory

The original file is /home/klymenka/WORK/big.orig and has 32769 bytes.

Bug: KFileRead(INNER_FILE1) stops at byte 32768
*/

#include <kapp/main.h>
#include <kapp/args.h>

#include <klib/rc.h>
#include <klib/log.h>
#include <klib/out.h>
#include <klib/status.h>

#include <kfs/directory.h>
#include <kfs/file.h>

#include <kxml/xml.h>

#include <string.h>

#include <common/test_assert.h>

static
rc_t run(bool compressed)
{
    KDirectory * pwd;
    const KDirectory * xfs;
    const KFile * xml;
    rc_t rc;

    STSMSG (1, ("Open file system\n"));
    rc = KDirectoryNativeDir (&pwd);
    if (rc)
        LOGERR (klogErr, rc, "Failed to open filesystem");
    else
    {
        const char* cc_xml = "test-xfs/compressed.xml";
        if (!compressed)
        {   cc_xml = "test-xfs/uncompressed.xml"; }
        STSMSG (1, ("Open file %s\n", cc_xml));
        rc = KDirectoryOpenFileRead(pwd, &xml, cc_xml);
        if (rc)
        {   LOGERR(klogErr, rc, "failed to open xml file"); }
        else
        {
            const char* src = "test-xfs/compressed.tgz";
            if (!compressed)
            {   src = "test-xfs/uncompressed/"; }
            STSMSG(1, ("Open as XML TOC %s\n", src));
            rc = KDirectoryOpenXTocDirRead(pwd, &xfs, true, xml, src);
            if (rc)
                LOGERR (klogErr, rc, "Failed to open xfs directory");
            else
            {
                const char name[] = "compressed.tgz/compressed.tar/dir/file";
                const KFile * afile;
                STSMSG (1, ("Open file %s\n", name));
                rc = KDirectoryOpenFileRead (xfs, &afile, name);
                if (rc)
                    LOGERR (klogErr, rc, "failed to open inner most file");
                else
                {
                  uint64_t pos = 0;
                  while (true) {
                    char b[1024];
                    size_t z;

                    rc = KFileRead (afile, pos, b, sizeof (b), &z);
                    if (rc == 0) {
                        b[z] = 0;
                        KOutMsg ("%s", b);
                        if (z == 0) {
                            KOutMsg ("EOF: %zu bytes\n", pos);
                            break;
                        }
                        pos += z;
                    }
                    else {
                        LOGERR (klogErr, rc, "Failed to read");
                        break;
                    }
                  }
                  STSMSG (2, ("Closing file %s\n", name));
                  KFileRelease (afile);
                }
                KDirectoryRelease (xfs);
            }

            KFileRelease (xml);
        }
        KDirectoryRelease (pwd);
    }
    return rc;
}

rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        "  %s [OPTIONS]\n"
        "\n"
        "Summary:\n"
        "  Test reading uncompressed (by copycat) with KXTocDir\n",
        progname);
}


const char UsageDefaultName[] = "test-xfs-uncompressed";

rc_t CC Usage (const Args * args)
{
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
    rc_t rc;

    if (args == NULL)
        rc = RC (rcApp, rcArgv, rcAccessing, rcSelf, rcNull);
    else
        rc = ArgsProgram (args, &fullpath, &progname);

    UsageSummary (progname);

    KOutMsg ("Options:\n");

    HelpOptionsStandard();

    HelpVersion (fullpath, KAppVersion());

    return rc;
}


ver_t CC KAppVersion (void)
{
    return 0;
}
rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;
    rc_t rc;

    rc = ArgsMakeAndHandle (&args, argc, argv, 0);
    if (rc == 0)
    {
        if (0)
        {   rc = run(true); }
        if (!rc)
            rc = run(false);
        if (rc)
            LOGERR (klogErr, rc, "Exiting failure");
        else
            STSMSG (0, ("Exiting okay\n"));
    }

    if (rc)
        LOGERR (klogErr, rc, "Exiting status");
    else
        STSMSG (0, ("Existing status (%R)\n", rc));
    return rc;
}
