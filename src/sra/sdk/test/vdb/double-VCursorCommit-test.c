#include <kapp/main.h> /* KMain */
#include <vdb/manager.h> /* VDBManager */
#include <vdb/schema.h> /* VSchema */
#include <vdb/table.h> /* VTable */
#include <vdb/cursor.h> /* VCursor */
#include <klib/log.h>  /* PLOGERR */

#include <limits.h> /* PATH_MAX in general */
#if !defined(PATH_MAX)  &&  !defined(_WIN32)
#include <dirent.h> /* PATH_MAX on icc*/
#endif

#include <stdlib.h> /* getenv */
#include <stdio.h> /* printf */
#include <string.h> /* strcat */

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

ver_t CC KAppVersion() { return 0; }
rc_t CC Usage ( struct Args const * args ) { return 0; }
const char UsageDefaultName[] = "";
rc_t CC UsageSummary (const char * prog_name) { return 0; }

rc_t CC KMain ( int argc, char *argv [] ) {
	rc_t rc = 0;
	VDBManager *mgr = NULL;
	VSchema *schema = NULL;
	VTable *table = NULL;
    	VCursor *cursor = NULL;
	uint32_t idx = 0;
	char buffer[] = "table Table #1 {\n"
	        	"  column ascii C1 = .C1; physical ascii .C1 = C1; };";

	const char* dir = getenv("HOME");
	char path[PATH_MAX];
	if (dir == NULL)
	{ dir = "."; }
	sprintf(path, "%s/double-VCursorCommit-test", dir);

	rc = KLogLevelSet(klogInfo);
	if (rc)
	{ return rc; }

	rc = VDBManagerMakeUpdate ( &mgr, NULL);
	if (rc)
	{ return rc; }
	rc = VDBManagerMakeSchema(mgr, &schema);
	if (rc)
	{ return rc; }
        rc = VSchemaParseText(schema, "Schema", buffer, sizeof buffer - 1);
	if (rc)
	{ return rc; }

	printf("VDBManagerCreateTable(%s)\n", path);
	rc = VDBManagerCreateTable(mgr, &table, schema, "Table", kcmInit, path);
	if (rc)
	{ return rc; }
	rc = VTableCreateCursorWrite(table, &cursor, kcmInsert);
	if (rc)
	{ return rc; }
	rc = VCursorAddColumn(cursor, &idx, "C1");
	if (rc)
	{ return rc; }
        rc = VCursorOpen(cursor);
	if (rc)
	{ return rc; }
        rc = VCursorOpenRow(cursor);
	if (rc)
	{ return rc; }
	rc = VCursorWrite(cursor, idx, 8, buffer, 0, sizeof buffer);
	if (rc)
	{ return rc; }
	rc = VCursorCommitRow (cursor);
	if (rc)
	{ return rc; }

	printf("VCursorCommit 1...\n");
	rc = VCursorCommit ( cursor );
	if (rc)
	{ return rc; }

	printf("VCursorCommit 1 = 0\n");
	printf("VCursorCommit 2...\n");
	rc = VCursorCommit ( cursor );
	LOGERR(klogInfo, rc, "VCursorCommit 2");

	return rc;
}
