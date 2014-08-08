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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <klib/rc.h>
#include <kfs/directory.h>
#include <kfs/file.h>
#include <kfs/arc.h>
#include <kfs/tar.h>
#include <klib/namelist.h>
#include <klib/log.h>
#include <klib/status.h>
#include <klib/out.h>
#include <kapp/main.h>


const char * KPathTypeToString (KPathType t)
{
    switch (t)
    {
    default:
	return "kptUNKNOWN";
    case kptNotFound:
	return "kptNotFound";
    case kptBadPath:
	return "kptBadPath";
    case kptFile:
	return "kptFile";
    case kptDir:
	return "kptDir";
    case kptCharDev:
	return "kptCharDev";
    case kptBlockDev:
	return "kptBlockDev";
    case kptFIFO:
	return "kptFIFO";

    case kptZombieFile:
        return "kptZombieFile";

    case kptAlias:
	return "kptAlias";

    case kptAlias|kptBadPath:
	return "kptAlias|kptBadPath";
    case kptAlias|kptFile:
	return "kptAlias|kptFile";
    case kptAlias|kptDir:
	return "kptAlias|kptDir";
    case kptAlias|kptLastDefined:
	return "kptAlias|kptLastDefined";
    case kptAlias|kptCharDev:
	return "kptAlias|kptCharDev";
    case kptAlias|kptBlockDev:
	return "kptAlias|kptBlockDev";
    case kptAlias|kptFIFO:
	return "kptAlias|kptFIFO";
    case kptAlias|kptZombieFile:
        return "kptAlias|kptZombieFile";

    }
}


/* buffer must be at least 9 characters long */
void AccessToString (char * b, uint32_t a)
{
    b[0] = (a & 0400) ? 'r' : '-';
    b[1] = (a & 0200) ? 'w' : '-';
    b[2] = (a & 0100) ? 'x' : '-';
    b[3] = (a & 0040) ? 'r' : '-';
    b[4] = (a & 0020) ? 'w' : '-';
    b[5] = (a & 0010) ? 'x' : '-';
    b[6] = (a & 0004) ? 'r' : '-';
    b[7] = (a & 0002) ? 'w' : '-';
    b[8] = (a & 0001) ? 'x' : '-';
    b[9] = '\0';
}

static
rc_t	directory_lister (const KDirectory * karcdir, bool recur, char * prefix)
{
    rc_t	rc;
    KNamelist *	knames;

/*     plogmsg (klogDebug1, */
/* 	     "** Enter directory_lister (kardir=[$(d)],recur=[$(r)],prefix=[$(p)])", */
/* 	     PLOG_3(PLOG_X64(d),PLOG_U8(r),PLOG_S(p)), */
/* 	     karcdir,recur,prefix); */

    rc = KDirectoryList (karcdir, &knames, NULL, NULL, ".");
    if (rc == 0)
    {
	uint32_t count;

	rc = KNamelistCount (knames, &count);
	if (rc == 0)
	{
	    int ix;

	    for (ix = 0; ix < count; ++ix )
	    {
		const char * name;

		rc = KNamelistGet (knames, ix, &name);
		if (rc == 0)
		{
		    KPathType	type;
		    uint32_t	access;
		    uint64_t	size;
		    char	abname		[4096];
		    char	access_str	[10];

		    
		    rc = KDirectoryResolvePath (karcdir, true, abname, sizeof (abname), name);
		    OUTMSG(("%s%i %s\n", prefix, ix, abname));
		    if (rc == 0)
		    {
			type = KDirectoryPathType (karcdir, abname);

			KDirectoryAccess (karcdir, &access, abname);
			AccessToString (access_str, access);

			size = 0;
			KDirectoryFileSize (karcdir, &size, name);

			OUTMSG (("%s name[%s] access[%s] type[%s] size[%lu]\n",
                                 prefix, name, access_str, KPathTypeToString (type), size));

			if (type == kptFile)
			{
			}
			if ((type == kptDir) && (recur) && (strcmp (name, ".") != 0))
			{
			    const KDirectory * sub;
			    char * new_prefix = malloc (strlen(prefix) + 5);
                            if (new_prefix == NULL)
                            {
                                OUTMSG (("Out of memory aborting listing\n"));
                            }
                            else
                            {
                                strcpy(new_prefix,prefix);
                                strcat(new_prefix,"    ");

                                rc = KDirectoryOpenDirRead (karcdir, &sub, false, abname);
                                if (rc == 0)
                                {
                                    directory_lister (sub, recur, new_prefix);
                                }
                                free (new_prefix);
                            }
			}
		    }
		}
		
	    }
	}
    }
    if (rc != 0)
	LOGERR (klogInfo, rc, "directory_lister not success return");
    return rc;
}


#define OPTION_SOFTWARN "warn-on-soft-errors"
#define ALIAS_SOFTWARN "w"
static const char * softwarn_usage[] = { "Extra warnings issued on soft errors", NULL };

static OptDef Options[] = 
{ { OPTION_SOFTWARN, ALIAS_SOFTWARN, NULL, softwarn_usage, 0, false, false }};


const char UsageDefaultName[] = "ktartest";
rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg (
        "\n"
        "Usage:\n"
        "  %s [OPTIONS] <tar-file>\n"
        "\n"
        "Summary:\n"
        "  Display the KFS view of a tar file.\n"
        "\n", progname);
}

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

    HelpOptionLine (ALIAS_SOFTWARN, OPTION_SOFTWARN, NULL, softwarn_usage);

    HelpOptionsStandard();

    HelpVersion (fullpath, KAppVersion());

    return rc;
}


ver_t CC KAppVersion (void)
{
    return 0;
}

rc_t CC KMain (int argc, char * argv[])
{
    Args * args;
    rc_t rc;

    rc = ArgsMakeAndHandle (&args, argc, argv, 1, Options, sizeof Options / sizeof * Options);
    if (rc)
        LOGERR (klogFatal, rc, "Could not parse command line");
    else
    {
        do
        {
            KDirectory * pwd;
            uint32_t pcount;
            bool warnsoft;

            rc = ArgsOptionCount (args, OPTION_SOFTWARN, &pcount);
            if (rc)
            {
                LOGERR (klogInt, rc, "Failed to retrieve softwarn count");
                break;
            }
            warnsoft = (pcount != 0);

            rc = ArgsOptionCount (args, OPTION_VERBOSE, &pcount);
            if (rc)
            {
                LOGERR (klogInt, rc, "Failed to retrieve softwarn count");
                break;
            }
            warnsoft = (pcount != 0);

            rc = ArgsParamCount (args, &pcount);
            if (rc)
            {
                LOGERR (klogInt, rc, "Failed to retrienve parameter count");
                break;
            }
            if (pcount == 0)
            {
                MiniUsage(args);
                break;
            }
            if (pcount > 1) /* could easily be made to loop across parameters */
            {
                rc = RC (rcExe, rcArgv, rcParsing, rcParam, rcTooLong);
                MiniUsage (args);
                break;
            }

            rc = KDirectoryNativeDir (&pwd);
            if (rc)
            {
                LOGERR (klogInt, rc, "Failed to open file system");
                break;
            }
            else
            {
                const KDirectory * arcdir;
                const char * path;
                rc_t rcsoft = 0;

                arcdir = NULL;
                rc = ArgsParamValue (args, 0, &path);
                if (rc)
                    LOGERR (klogInt, rc, "Failed to retrieve parameter");
                else
                {
                    rc = KDirectoryOpenArcDirRead (pwd, &arcdir, false, path, tocKFile,
                                                   KArcParseTAR, NULL, NULL);
                    /* check for soft errors */
                    /* truncation is only one so far */
                    if ((KStsLevelGet() >= 1) &&
                        ((rc == 0) ||
                         (warnsoft &&
                          (rcsoft == RC (rcFS, rcArc, rcParsing, rcToc, rcIncomplete)))))
                    {
                        OUTMSG (("Opened archive: %s\n", path));
                        rcsoft = directory_lister (arcdir, true, "");
                    }
                    if (rc == 0)
                        rc = rcsoft;
                    KDirectoryRelease (arcdir);
                }
                KDirectoryRelease (pwd);
            }
        } while (0);
        ArgsWhack (args);
    }
    return rc;
}




