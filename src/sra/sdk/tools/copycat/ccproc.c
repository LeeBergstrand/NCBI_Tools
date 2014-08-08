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

#include <assert.h>
#include <atomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kapp/log.h>
#include <kfs/directory.h>
#include <kfs/file.h>
#include <kfs/md5.h>
#include <kfs/teefile.h>
#include "copycat.h"


/* ======================================================================
 * Process does up the copy portion of the copycat tool's functionality
 * and sets up ProcessOne that does the Catalog portion.
 */
struct Process
{
    atomic32_t refcount;
    const KDirectory * in;
    KDirectory * out;
    KDirectory * xml;
    KMD5SumFmt * md5;
    CCFileFormat * ff;
    uint32_t	timeout;
    bool force;
    char path[4096];
};

rc_t ProcessAddRef (const Process * self)
{
    if (self != NULL)
	atomic32_inc (&((Process*)self)->refcount);
    return 0;
}

rc_t ProcessRelease (const Process * cself)
{
    Process * self = (Process *)cself;
    rc_t rc = 0;
    LOGMSG (klogDebug10, "Enter: ProcessRelease");

    if (self != NULL)
    {
	if (atomic32_dec_and_test (&self->refcount))
	{
	    KDirectoryRelease (self->in);
	    KDirectoryRelease (self->out);
	    KDirectoryRelease (self->xml);
	    KMD5SumFmtRelease (self->md5);
	    CCFileFormatRelease (self->ff);
	    free (self);
	}
    }
    return rc;
}

rc_t ProcessMake (Process ** p, const KDirectory * in,  KDirectory * out, 
		  KDirectory * xml, bool force, KMD5SumFmt * md5,
		  CCFileFormat * ff, const char * path)
{
    rc_t rc;
    size_t pathlen;
    Process * self;
    char relpath [4096];

    assert (in != NULL);
    assert (out != NULL);
    assert (xml != NULL);
    assert (path != NULL);

    rc = KDirectoryVResolvePath (in, false, relpath, sizeof relpath, path, NULL);
    if (rc != 0)
    {
	pLOGERR (klogErr, rc, "unable to resolve path $(P)", PLOG_S(P), path);
	return rc;
    }
    if ((relpath[0] == '.') && (relpath[1] == '.') && (relpath[2] == '/'))
    {
	rc = RC (rcExe, rcDirectory, rcResolving, rcPath, rcOutOfKDirectory);
	pLOGERR (klogErr, rc, "Path must resolve to current directory or subdirectories $(P)",
		 PLOG_S(P), relpath);
	return rc;
    }

    pathlen = strlen(relpath);
    self = malloc (sizeof (*self) - sizeof (*self->path) + pathlen + 1);
    if (self == NULL)
	rc = RC (rcExe, rcNoTarg, rcAllocating, rcMemory, rcExhausted);
    else
    {
	atomic32_set (&self->refcount, 1);
	KDirectoryAddRef (in);
	KDirectoryAddRef (out);
	KDirectoryAddRef (xml);
	KMD5SumFmtAddRef (md5);
	CCFileFormatAddRef (ff);
	self->in = in;
	self->out = out;
	self->xml = xml;
	self->force = force;
	self->md5 = md5;
	self->ff = ff;
	memcpy (self->path, relpath, pathlen+1);
	*p = self;
    }
    return rc;
}
static
rc_t ProcessDoFile (Process * self)
{
    const KFile * original;
    rc_t rc = 0;
    enum KCreateMode mode;
    
    PLOGMSG (klogDebug10, "ProcessDoFile $(f)", PLOG_S(f), self->path);


    if (! self->force)
    {
	/* if not forced replace mode we fail on existing file */
	mode = kcmCreate;
    }
    else
    {
	uint32_t tt;

	tt = KDirectoryPathType (self->out, self->path);
	switch (tt)
	{
	default:
	    PLOGMSG (klogWarn, "File exists and will be replaced in output directory $(f)",
		     PLOG_S(f), self->path);
	    break;
	    /* if the path to the file or the file do not exist no warning */
	case kptNotFound:
	case kptBadPath:
	    break;
	}
	tt = KDirectoryPathType (self->xml, self->path);
	switch (tt)
	{
	default:
	    PLOGMSG (klogWarn, "File exists and might be replaced in xml directory $(f)",
		     PLOG_S(f), self->path);
	    break;
	    /* if the path to the file or the file do not exist no warning */
	case kptNotFound:
	case kptBadPath:
	    break;
	}

	/* forced mode we create with init instead of create forcing a delete/create effect */
	mode = kcmInit;
    }

    /* open originial source for read */
    rc = KDirectoryVOpenFileRead (self->in, &original, self->path, NULL);
    if (rc == 0)
    {
	KFile * copy;

	/* create copy output for write */
	rc = KDirectoryVCreateFile (self->out, &copy, false, 0644, mode|kcmParents,
				    self->path, NULL);
	if (rc == 0)
	{
	    KFile * fm;

	    /* create parallel <path>.md5 */
	    rc = KDirectoryCreateFile (self->out, &fm, true, 0644, mode, "%s.md5",
				       self->path);
	    if (rc == 0)
	    {
		KMD5SumFmt * md5f;

		/* make the .md5 an MD5 sum format file */
		rc = KMD5SumFmtMakeUpdate (&md5f, fm);
		if (rc == 0)
		{
		    union u
		    {
			KFile * kf;
			KMD5File * mf;
		    } outf;

		    /* combine the copy and MD5 file into our special KFile */
		    rc = KMD5FileMakeWrite (&outf.mf, copy, md5f, self->path);
		    if (rc == 0)
		    {
			const KFile * inf;

			/* release this outside reference to the MD5SumFMT leaving
			 * only the one internal to the KMD5File */
			KMD5SumFmtRelease (md5f);

			/* create the KTeeFile that copies reads from the
			 * original as writes to the copy.  Reads will be
			 * made by the cataloging process */
			rc = KFileMakeTeeRead (&inf, original, outf.kf);
			if (rc == 0)
			{
			    ProcessOne * po;

			    /* create the cataloger giving it the infile which
			     * is the KTeeFile, Indirectory, the XML directory,
			     * and the original path for the file */
			    rc = ProcessOneMake (&po, self->in, self->xml, inf,
						 self->md5, self->ff, self->path);
			    if (rc == 0)
			    {
				/* do the catalog (and thus copy) */
				rc = ProcessOneDo(po);
				/* release the cataloger object */
				ProcessOneRelease (po);
			    }
			    /* release the infile which will complete a  copy 
			     * regardless of the state of the cataloger */
			    KFileRelease (inf);
/* 			    return rc; */
			}
			else
			{
			    KFileRelease (outf.kf);
			    KFileRelease (original);
			} /* rc = KFileMakeTeeRead (&inf, original, outf.kf);*/
		    }
		    else
		    {
			KFileRelease (copy);
			KMD5SumFmtRelease (md5f);
		    } /* KMD5FileMakeWrite (&outf.mf, copy, md5f, self->path); */
		} /* KDirectoryCreateFile (self->out, &fm, true, 0644, mode, "%s.md5", */
		KFileRelease (fm);
	    } /* KDirectoryCreateFile (self->out, &fm, true, 0644, mode, "%s.md5", */
	    KFileRelease (copy);
	} /* rc = KDirectoryVCreateFile (self->out, &copy, false, 0644, mode|kcmParents, */
	KFileRelease (original);
    } /* rc = KDirectoryVOpenFileRead (self->in, &original, self->path, NULL); */
    return rc;
}

static
rc_t ProcessDoDirectory (Process * self)
{
    rc_t rc = 0;
    PLOGMSG (klogInfo, "ProcessDoDirectory $(d)", PLOG_S(d), self->path);

    rc = RC (rcExe, rcDirectory, rcCopying, rcParam, rcUnsupported);
    return rc;
}
rc_t ProcessDo (Process * self)
{
    rc_t rc = 0;
    enum KPathType type;

    assert (self != NULL);
    assert (self->path != NULL);

    type = KDirectoryVPathType (self->in, self->path, NULL);
    switch (type & ~kptAlias)
    {
    case kptNotFound:
	rc = RC (rcExe, rcPath, rcAccessing, rcPath, rcNotFound);
	break;
    default:
    case kptBadPath:
	rc = RC (rcExe, rcPath, rcAccessing, rcPath, rcInvalid);
	break;
    case kptFile:
	rc = ProcessDoFile (self);
	break;
    case kptDir:
#if 0
	rc = ProcessDoDirectory (self);
#else
	rc = PLOGMSG (klogInfo, "Ignoring directory $(p)",PLOG_S(p),self->path);
#endif
	break;
    case kptCharDev:
	rc = PLOGMSG (klogInfo, "Ignoring kptCharDev $(p)",PLOG_S(p),self->path);
	break;
    case kptBlockDev:
	rc = PLOGMSG (klogInfo, "Ignoring kptBlockDev $(p)",PLOG_S(p),self->path);
	break;
    case kptFIFO:
	rc = PLOGMSG (klogInfo, "Ignoring kptFIFO $(p)",PLOG_S(p),self->path);
	break;
    }
    return rc;
}

