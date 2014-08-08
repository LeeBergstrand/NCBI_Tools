/*==============================================================================
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


#include "vdb-decrypt.vers.h"

#include "shared.h"

#include <krypto/wgaencrypt.h>
#include <krypto/encfile.h>
#include <kfs/file.h>
#include <klib/rc.h>
#include <klib/defs.h>
#include <klib/log.h>
#include <klib/status.h>

#include <string.h>
#include <assert.h>

/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
ver_t CC KAppVersion ( void )
{
    return VDB_DECRYPT_VERS;
}


/* Usage
 */
const char UsageDefaultName [] = "vdb-decrypt";
const char * UsageSra []       = { "decrypt sra archives - [NOT RECOMMENDED]",
                                   NULL };
const char De[]             = "De";
const char de[]             = "de";
const char OptionSra[] = OPTION_DEC_SRA;


static
OptDef Options[] = 
{
    /* name            alias max times oparam required fmtfunc help text loc */
    { OPTION_DEC_SRA, ALIAS_DEC_SRA, NULL, UsageSra,      0, false, false },
    { OPTION_FORCE,   ALIAS_FORCE,   NULL, ForceUsage,   0, false, false }
};


static 
bool DecryptSraFlag = false;


const bool Decrypting = true;

void CryptOptionLines ()
{
    HelpOptionLine (ALIAS_DEC_SRA, OPTION_DEC_SRA, NULL, UsageSra);
}

bool DoThisFile (const KFile * infile, EncScheme enc, ArcScheme * parc)
{
    const KFile * Infile;
    ArcScheme arc;
    rc_t rc;

    *parc = arcNone;

    switch (enc)
    {
    default:
        STSMSG (1, ("not encrypted"));
        return false;

    case encEncFile:
        rc = KEncFileMakeRead (&Infile, infile, &Key);
        if (rc)
            return false;
        break;

    case encWGAEncFile:
        rc = KFileMakeWGAEncRead (&Infile, infile, Password, PasswordSize);
        if (rc)
            return false;
        break;
    }
    arc = ArchiveTypeCheck (Infile);
    KFileRelease (Infile);
    switch (arc)
    {
    default:
        return false;
    case arcNone:
        return true;
    case arcSRAFile:
        *parc = arcSRAFile;
        STSMSG (1, ("encrypted sra archive\ndecryption%s requested",
                    DecryptSraFlag ? "" : " not"));
        return DecryptSraFlag;
    }
}

bool NameFixUp (char * name)
{
    char * pc = strrchr (name, '.');
    if (pc != NULL)
    {
        if (strcmp (pc, EncExt) == 0)
        {
            pc[0] = '\0';
            return true;
        }
    }
    return false;
}

rc_t CryptFile (const KFile * in, const KFile ** new_in,
                KFile * out, KFile ** new_out, EncScheme scheme)
{
    const KFile * dec;
    rc_t rc;

    assert (in);
    assert (out);
    assert (new_in);
    assert (new_out);


    rc = KFileAddRef (out);
    if (rc)
        return rc;

    switch (scheme)
    {
    default:
    case encError:
        rc = RC (rcExe, rcFile, rcClassifying, rcFile, rcInvalid);
        break;
        
    case encNone:
    copy:
        rc = KFileAddRef (in);
        if (rc)
            goto fail;
        *new_in = in;
        *new_out = out;
        STSMSG (1, ("not encrypted just copying"));
        return 0;

    case encEncFile:
        rc = KEncFileMakeRead (&dec, in, &Key);
    made_enc:
        if (rc)
            goto fail;

        switch (ArchiveTypeCheck (dec))
        {
        default:
        case arcError:
            rc = RC (rcExe, rcFile, rcClassifying, rcFile, rcInvalid);
            break;

        case arcSRAFile:
            if (!DecryptSraFlag)
            {
                rc = KFileRelease (dec);
                if (rc)
                {
                    KFileRelease (dec);
                    KFileRelease (in);
                    goto fail;
                }
                goto copy;
            }
            /* fall through */
        case arcNone:
            *new_out = out;
            *new_in = dec;
            return 0;
        }
        break;

    case encWGAEncFile:
        rc = KFileMakeWGAEncRead (&dec, in, Password, PasswordSize);
        goto made_enc;
        break;
    }
    fail:
        KFileRelease (out);
        *new_in = *new_out = NULL;
        return rc;
}


/* KMain - EXTERN
 *  executable entrypoint "main" is implemented by
 *  an OS-specific wrapper that takes care of establishing
 *  signal handlers, logging, etc.
 *
 *  in turn, OS-specific "main" will invoke "KMain" as
 *  platform independent main entrypoint.
 *
 *  "argc" [ IN ] - the number of textual parameters in "argv"
 *  should never be < 0, but has been left as a signed int
 *  for reasons of tradition.
 *
 *  "argv" [ IN ] - array of NUL terminated strings expected
 *  to be in the shell-native character set: ASCII or UTF-8
 *  element 0 is expected to be executable identity or path.
 */
rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;
    rc_t rc;

    rc = ArgsMakeAndHandle (&args, argc, argv, 1, Options,
                            sizeof (Options) / sizeof (Options[0]));
    if (rc)
        LOGERR (klogInt, rc, "failed to parse command line parameters");

    else
    {
        uint32_t ocount;

        rc = ArgsOptionCount (args, OPTION_DEC_SRA, &ocount);
        if (rc)
            LOGERR (klogInt, rc, "failed to examine decrypt "
                    "sra option");
        else
        {
            DecryptSraFlag = (ocount > 0);

            rc = CommonMain (args);
        }
        ArgsWhack (args);
    }

    STSMSG (1, ("exiting: %R (%u)", rc, rc));
    return rc;
}


/* EOF */
