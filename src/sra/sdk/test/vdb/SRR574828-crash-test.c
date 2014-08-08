#include <kapp/main.h> /* KMain */
#include <vdb/manager.h> /* VDBManager */
#include <vdb/schema.h> /* VSchema */
#include <vdb/table.h> /* VTable */
#include <vdb/cursor.h> /* VCursor */
#include <klib/log.h>  /* PLOGERR */
#include <limits.h> /* PATH_MAX elsewhere */
#include <stdlib.h> /* getenv */
#include <stdio.h> /* printf */
#include <string.h> /* strcat */

ver_t CC KAppVersion() { return 0; }
rc_t CC Usage ( struct Args const * args ) { return 0; }
const char UsageDefaultName[] = "";
rc_t CC UsageSummary (const char * prog_name) { return 0; }

rc_t CC KMain ( int argc, char *argv [] ) {
	rc_t rc = 0;

	const VDBManager *mgr = NULL;
	const VTable *table = NULL;

	rc = VDBManagerMakeRead(&mgr, NULL);
	if (rc)
	{   return rc; }

    rc = VDBManagerOpenTableRead(mgr, &table, NULL,
        "/netmnt/traces04/sra1/SRR/000561/SRR574828");
	if (rc)
	{   return rc; }

    VTableRelease(table);
    table = NULL;

    VDBManagerRelease(mgr);
    mgr = NULL;

    return rc;
}
