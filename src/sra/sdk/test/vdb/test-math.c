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

#include "test-math.vers.h"

#include <vdb/manager.h>
#include <vdb/schema.h>
#include <vdb/table.h>
#include <vdb/cursor.h>

#include <kdb/manager.h>
#include <kdb/column.h>

#include <kapp/main.h>
#include <klib/text.h>
#include <klib/log.h>
#include <klib/rc.h>

#include <fmtdef.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


typedef struct vcol2kcol vcol2kcol;
struct vcol2kcol
{
    int64_t row_id;

    /* names/paths of the tested vcolumn, the control vcolumn and the kcolumn */
    String vcols;
    String ccols;
    String kcols;

    /* float operation for the control column to match schema tested function */
    float (*function) (float);

    /* "handles" for the columns */
    const KColumn *kcol;
    uint32_t vcol;
    uint32_t ccol;

    /* these have to be 32 for this to work. */
    uint32_t velem_bits;
    uint32_t celem_bits;
};
static int dummy = 0;
static
rc_t kdb_to_vdb_row (VCursor * curs, vcol2kcol * v)
{
    rc_t rc;
    const KColumnBlob *kblob;

    rc = KColumnOpenBlobRead (v->kcol, &kblob, v->row_id);
    if (rc)
    {
        if (GetRCState (rc) == rcNotFound)
            return RC (rcExe, rcColumn, rcReading, rcData, rcDone);

        pLOGERR (klogInt, rc, "failed to open blob for row $(row_id) in column '$(path)'",
                 "row_id=%" LD64 ",path=%s", v->row_id, v->kcols.addr);
    }
    else
    {
        uint32_t count;
        uint32_t ix;
        char     buff [1024];
        size_t   num_read;
        size_t   remaining;
        size_t   offset;
        uint32_t elem_bits;

/*         printf ("kdb_to_vdb_row %d\n", ++dummy); */

        elem_bits = v->velem_bits;

        /*
deciphering kdb2vdb
        */

/* offset + num_read + remaining = size of the blob */
        offset = 0;
        do
        {

            rc = KColumnBlobRead (kblob, offset, buff, sizeof buff, &num_read, &remaining);
/*             printf ("kdb_to_vdb_row call KColumnBlobRead %" LUSZ "\t%" LUSZ  "\t%" LUSZ "\n", */
/*                     offset, num_read, remaining); */
            /*
             * offset is how much of blob was previously read
             * num_read is how many bytes were read
             * remaining is how many bytes remain in the blob as yet unread
             */

            if (rc)
            {
                pLOGERR (klogInt, rc, "failed to read blob for row $(row_id) in column '$(path)'",
                          "row_id=%" LD64 ",path=%s", v->row_id, v->kcols.addr);
                break;
            }

            /* num_read is in bytes: set count to floats */
/*             count = num_read * 8 / v->velem_bits; */


            if (num_read & (sizeof (float) -1))
                LOGMSG (klogWarn, "blob size not compatible with float size");


            /*
             * count <- item count
             */
            count = num_read / sizeof (float);
            if (count == 0)
            {
                if (remaining != 0)
                    LOGMSG (klogWarn, "discarding data at end of row");
                break;
            }
            else
            {
                /*
                 * set up offset for the next read
                 */
                size_t bytes_used;

                bytes_used = count * sizeof (float);
                offset += bytes_used;
                remaining += (num_read - bytes_used); /* nominally push back bytes unused at end of read */
            }
/*             if ((remaining > 0) && (remaining < sizeof(float))) */
/*                 LOGMSG ( klogWarn, "blob " ); */


            rc = VCursorWrite (curs, v->vcol, v->velem_bits, buff, 0, count);
            {
                int64_t rid = 0;
                VCursorRowId (curs, & rid);
/*                 printf ("kdb_to_vdb_row call VCursorWrite %u\t %lu\n", count, rid); */
            }
            if (rc)
            {
                int64_t rid = 0;

                VCursorRowId (curs, & rid);

                pLOGERR (klogInt, rc, "failed to write data to row $(row_id) from column '$(path)'",
                         "row_id=%" LD64 ",path=%s", rid, v->kcols.addr);
                break;
            }

            {
                float * t = (float *)buff;

                for (ix = 0; ix < count; ++ix)
                {
/*                     printf("kdb_to_vdb_row %f", t[ix]); */
                    t[ix] = v->function (t[ix]);
/*                     printf("\t%f\n", t[ix]); */
                }
            }

            rc = VCursorWrite (curs, v->ccol, v->celem_bits, buff, 0, count);
            if (rc)
            {
                int64_t rid = 0;

                VCursorRowId (curs, & rid);

                pLOGERR (klogInt, rc, "failed to write data to row $(row_id) from column '$(path)'",
                         "row_id=%" LD64 ",path=%s", rid, v->kcols.addr
                   );
                break;
            }

        } while (remaining != 0);

        if (rc == 0)
        {
            int64_t start;
            uint32_t count;

            rc = KColumnBlobIdRange (kblob, &start, &count);
            if (rc != 0)
                LOGERR (klogInt, rc, "failed to obtain blob id range");
            v->row_id = start + count;
        }

        KColumnBlobRelease (kblob);

    }

    return rc;
}

static unsigned load_dummy = 0;
static
rc_t kdb_to_vdb_load (VCursor * curs, vcol2kcol * v)
{
    rc_t rc = VCursorOpen (curs);
/*     printf("kdb_to_vdb_load VCursorOpen\n"); */
    if (rc)
        LOGERR (klogErr, rc, "failed to open cursor");
    else
    {
        while (rc == 0)
        {
/*             printf(__FILE__ "kdb_to_vdb_load VCursorOpenRow %u\n", ++load_dummy); */
            rc = VCursorOpenRow (curs);
            if (rc)
                LOGERR (klogErr, rc, "failed to open cursor row");
            else
            {
                rc_t rc2;

                rc = kdb_to_vdb_row (curs, v);
                if (rc == 0)
                {
/*                     printf("kdb_to_vdb_load VCursorCommitRow %u\n", load_dummy); */
                    rc = VCursorCommitRow (curs);
                    if (rc)
                        LOGERR (klogErr, rc, "failed to commit row");
                }

/*                 printf("kdb_to_vdb_load VCursorCloseRow %u\n", load_dummy); */
                rc2 = VCursorCloseRow (curs);
                if (rc2)
                {
                    LOGERR (klogErr, rc2, "failed to close cursor row");
                    if (rc == 0)
                        rc = rc2;
                }
            }
        }

        if (GetRCState (rc) == rcDone)
            rc = 0;

        if (rc == 0)
            rc = VCursorCommit (curs);
    }

    return rc;
}

static
rc_t kdb_to_vdb_create (VDBManager * vmgr, const VSchema * schema,
                        const char * table_type, const char * table_path,
                        bool force, vcol2kcol * v)
{
    KCreateMode cmode;
    VTable * vtbl;
    rc_t rc;

    cmode = force ? kcmInit : kcmCreate;
    rc = VDBManagerCreateTable (vmgr, & vtbl, schema, table_type, cmode, table_path);
    if (rc != 0)
    {
        pLOGERR (klogErr, rc, "failed to $(cmode) table '$(path)'",
                 "cmode=%s,path=%s", force ? "create or replace" : "create",
                 table_path);
    }
    else
    {
        VCursor *curs;
        rc = VTableCreateCursorWrite (vtbl, &curs, kcmInsert);
        if (rc)
            LOGERR (klogInt, rc, "failed to create cursor");
        else
        {
            VTypedesc vdesc;
            VTypedesc cdesc;

            rc = VCursorAddColumn (curs, &v->vcol, "%.*s",
                                   (int)v->vcols.size, v->vcols.addr);
            if (rc)
            {
                pLOGERR (klogErr, rc, "failed to add test column '$(col)' to cursor",
                         "col=%.*s", (int)v->vcols.size, v->vcols.addr);
            }
            else
            {
                rc = VCursorAddColumn (curs, &v->ccol, "%.*s",
                                       (int)v->ccols.size, v->ccols.addr);
                if (rc)
                {
                    pLOGERR (klogErr, rc, "failed to add control column '$(col)' to cursor",
                             "col=%.*s", (int)v->ccols.size, v->ccols.addr);
                }
                else
                {
                    rc = VCursorDatatype (curs, v->vcol, 0, &vdesc);
                    if (rc)
                    {
                        pLOGERR (klogInt, rc,
                                 "failed to access datatype for test column '$(name)'",
                                 "name=%.*s", (int)v->vcols.size, v->vcols.addr);
                    }
                    else
                    {
                        rc = VCursorDatatype (curs, v->ccol, 0, &cdesc);
                        if (rc != 0)
                        {
                            pLOGERR (klogInt, rc, 
                                     "failed to access datatype for control column '$(name)'",
                                     "name=%.*s", (int)v->ccols.size, v->ccols.addr);
                        }
                        else
                        {
                            v->velem_bits = VTypedescSizeof (&vdesc);
                            v->celem_bits = VTypedescSizeof (&cdesc);

                            assert (v->velem_bits == 32);
                            assert (v->celem_bits == 32);

                            rc = kdb_to_vdb_load (curs, v);
                       }
                    }
                }
            }
            VCursorRelease (curs);
        }

#if 1
        if (rc == 0)
            rc = VTableReindex (vtbl);
#endif

        VTableRelease (vtbl);
    }

    return rc;
}

static
rc_t vcol2kcol_make (vcol2kcol **out, const char * vcol, const char * ccol,
                     const char * kcol, float (*function) (float))
{
    vcol2kcol *v;

    v = malloc (sizeof (*v));
    if (v == NULL)
        return RC (rcExe, rcColumn, rcOpening, rcMemory, rcExhausted);

    v->row_id = 1;
    StringInitCString (&v->vcols, vcol);
    StringInitCString (&v->ccols, ccol);
    StringInitCString (&v->kcols, kcol);
    v->function = function;
    v->kcol = NULL;
    v->vcol = 0;
    v->ccol = 0;
    v->velem_bits = 32;
    v->celem_bits = 32;
    *out = v;
    return 0;
}

static
rc_t vcol2kcol_open (vcol2kcol *v, const KDBManager *kmgr)
{
    rc_t rc;

    rc = KDBManagerOpenColumnRead (kmgr, &v->kcol, v->kcols.addr);
    if (rc != 0)
    {
        pLOGERR (klogErr, rc, "failed to open KColumn '(path)'",
                 "path=%s", v->kcols.addr);
    }
    else
    {
        rc = KColumnIdRange (v->kcol, & v->row_id, NULL);
        if (rc != 0)
        {
            pLOGERR (klogErr, rc, "failed to determine id range for KColumn '(path)'",
                     "path=%s", v->kcols.addr);
        }
    }

    return rc;
}

static
void vcol2kcol_whack (vcol2kcol *v)
{
    KColumnRelease (v->kcol);
    free (v);
}

static
rc_t kdb_to_vdb (const char * schema_path, const char * table_type, const char * table_path, 
                 bool force, const char * kcol_name, const char * vcol_name,
                 const char * ccol_name, float (*function) (float))
{
    KDBManager *kmgr;
    rc_t rc = KDBManagerMakeUpdate (& kmgr, NULL);
    if (rc != 0)
        LOGERR (klogInt, rc, "failed to make KDB manager");
    else
    {
        VDBManager *vmgr;
        rc = VDBManagerMakeUpdate (& vmgr, NULL);
        if (rc != 0)
            LOGERR (klogInt, rc, "failed to make VDB manager");
        else
        {
            VSchema *schema;
            rc = VDBManagerMakeSchema (vmgr, &schema);
            if (rc != 0)
                LOGERR (klogInt, rc, "failed to make empty schema");
            else
            {
                rc = VSchemaParseFile (schema, schema_path);
                if (rc != 0)
                    pLOGERR (klogErr, rc, "failed to parse schema file '$(file)'", "file=%s", schema_path);
                else
                {
                    vcol2kcol *v;

                    rc = vcol2kcol_make (& v, vcol_name, ccol_name, kcol_name, function);
                    if (rc == 0)
                    {
                        rc = vcol2kcol_open (v, kmgr);
                        if (rc == 0)
                        {
                            rc = kdb_to_vdb_create (vmgr, schema, table_type, table_path, force, v);
                            if (rc == 0)
                                printf("Test passed\n");
                        }
                        vcol2kcol_whack (v);
                    }
                }

                VSchemaRelease (schema);
            }

            VDBManagerRelease (vmgr);
        }

        KDBManagerRelease (kmgr);
    }

    return rc;
}


/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
uint32_t KAppVersion (void)
{
    return TEST_MATH_VERS;
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
static
void Usage (const char *app_name)
{
    printf ("\n"
             "test math functions by creating a two column named VTable loaded from a KColumn\n"
             "\n"
             "Usage: %s [ options ] -s schema \\\n"
             "    -o path-to-vtable vcolumn=path-to-kcolumn...\n"
             "\n"
             "    options:\n"
             "      -f         forced create or replace output table\n"
             "\n%s%s%s",
             app_name,
             "   One of the next four is required\n"
             "      -R         test the round transform function\n"
             "      -F         test the floor transform function\n"
             "      -C         test the ceiling transform function\n"
             "      -T         test the truncate transform function\n"
             "    parses schema text supplied from '-s' option and locates table-type by\n"
             "    name implied by the R, F, C or T option.\n"
             "\n",
             "    creates VTable using path from 'o' option. this operation will fail if\n"
             "    the table exists unless the '-f' option is specified.\n"
             "\n",
             "    the single vcol=kcol pair will cause the KColumn to be opened and loaded into\n"
             "    the corresponding named VColumn.\n"
             "\n"
       );
}

rc_t KMain (int argc, char *argv[])
{
    const char * schema_path = NULL;    /* path the test-math.vschema is required */
    float (*function) (float) = NULL;   /* function is a required parameter */
    const char * table_type = NULL;     /* output table type is set by function parameter */
    const char * table_path = NULL;     /* output table path is a required option */    
    const char * kcol_name = NULL;      /* input data column is a required option */
    const char * vcol_name = "xform";   /* tested vcolumn has fixed name */
    const char * ccol_name = "ctl";     /* control vcolumn has fixed name */

    int ix;
    bool force = false;

    if (argc == 1)
        goto usage;

    function = NULL;
    for (ix = 1; ix < argc; ++ix)
    {
        const char * arg = argv[ix];

        if (*arg != '-')
        {
            fprintf (stderr, "No parameters without option introducers\n");
            return RC (rcExe, rcArgv, rcReading, rcParam, rcExcessive);
        }
           
        do switch (*(++arg))
        {
            /* requested help */
        case 'h':
        case '?':
            goto usage;

            /* kcolumn path */
        case 'k':
            kcol_name = NextArg (&arg, &ix, argc, argv, NULL, NULL);
            break;

            /* forced replace now true */
        case 'f':
            force = true;
            break;

            /* path to test-math.vschema or compatible */
        case 's':
            schema_path = NextArg (&arg, &ix, argc, argv, NULL, NULL);
            break;

            /* path to VDB Table to be created/over-written */
        case 'o':
            table_path = NextArg (&arg, &ix, argc, argv, NULL, NULL);
            break;
        case 'l':
            NextLogLevelh (&ix, argc, argv, NULL, NULL);
            break;

            /* function option that chooses function and table type */
            /* floorf()  integer <= float value */
        case 'F':
            if (function != NULL)
            {
                fprintf (stderr, "Only allow one option F, C, R or T\n");
                return RC (rcExe, rcArgv, rcReading, rcParam, rcExcessive);
            }
            function = floorf;
            table_type = "t_floor";
            break;
            /* ceil() |integer| >= |float value| */
        case 'C':
            if (function != NULL)
            {
                fprintf (stderr, "Only allow one option F, C, R or T\n");
                return RC (rcExe, rcArgv, rcReading, rcParam, rcExcessive);
            }
            function = ceilf;
            table_type = "t_ceil";
            break;
            /* round() integer */
        case 'R':
            if (function != NULL)
            {
                fprintf (stderr, "Only allow one option F, C, R or T\n");
                return RC (rcExe, rcArgv, rcReading, rcParam, rcExcessive);
            }
            function = roundf;
            table_type = "t_round";
            break;
        case 'T':
            if (function != NULL)
            {
                fprintf (stderr, "Only allow one option F, C, R or T\n");
                return RC (rcExe, rcArgv, rcReading, rcParam, rcExcessive);
            }
            function = truncf;
            table_type = "t_trunc";
            break;
        default:
            fprintf (stderr, "unrecognized switch: '%s'\n", argv [ix]);
            return RC (rcExe, rcArgv, rcReading, rcParam, rcInvalid);
        }
        while (arg[1] != 0);
    }

    if (kcol_name == NULL)
    {
        fprintf (stderr, "%s takes a single KColumn parameter\n", argv[0]);
        return RC (rcExe, rcArgv, rcReading, rcParam, rcExcessive);
    }

    if (function == NULL)
    {
        fprintf (stderr, "missing test function option (one of FCRT)\n");
        return RC (rcExe, rcArgv, rcReading, rcParam, rcNotFound);
    }
    if (schema_path == NULL)
    {
        fprintf (stderr, "missing schema path\n");
        return RC (rcExe, rcArgv, rcReading, rcParam, rcNotFound);
    }

    if (table_path == NULL)
    {
        fprintf (stderr, "missing table path\n");
        return RC (rcExe, rcArgv, rcReading, rcParam, rcNotFound);
    }

    /* parameters handled to now construct the objects and process the data */

    kdb_to_vdb (schema_path, table_type, table_path, force, kcol_name, vcol_name, ccol_name, function);

    return 0;
usage:
    Usage (argv[0]);
    return 0;
}

