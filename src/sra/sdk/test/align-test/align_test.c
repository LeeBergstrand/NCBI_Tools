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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <klib/writer.h>
#include <kapp/args.h>
#include <align/align-access.h>
#include <vfs/path.h>

#include <common/test_assert.h>

static void testEnum(const AlignAccessDB *db) {
    AlignAccessRefSeqEnumerator *e;
    rc_t rc;
    char buf[1024];
    
    rc = AlignAccessDBEnumerateRefSequences(db, &e);
    if (rc)
        return;
    do {
        AlignAccessRefSeqEnumeratorGetID(e, buf, sizeof(buf), NULL);
        printf("ID: %s\n", buf);
    } while (AlignAccessRefSeqEnumeratorNext(e) == 0);
    AlignAccessRefSeqEnumeratorRelease(e);
}

static void print1(AlignAccessAlignmentEnumerator *e) {
    uint64_t start;
    char buf[4 * 1024];
    
    AlignAccessAlignmentEnumeratorGetRefSeqPos(e, &start);
    kprintf(NULL, "Ref. Seq. Position: %lu\n", start + 1);
    AlignAccessAlignmentEnumeratorGetShortSeqID(e, buf, sizeof(buf), NULL);
    printf("spot name: %s\n", buf);
    AlignAccessAlignmentEnumeratorGetCIGAR(e, &start, buf, sizeof(buf), NULL);
    printf("CIGAR: %s\n", buf);
    AlignAccessAlignmentEnumeratorGetShortSequence(e, buf, sizeof(buf), NULL);
    printf("sequence: %s\n\n", buf);
}

static void print2(AlignAccessAlignmentEnumerator *e) {
    print1(e);
    AlignAccessAlignmentEnumeratorNext(e);
    print1(e);
}

static
void test(const char *bamName, const char *idxName, const char *refSeqID, uint64_t start, unsigned len) {
    rc_t rc;
    const AlignAccessMgr *mgr;
    const AlignAccessDB *db;
    AlignAccessAlignmentEnumerator *e;
    const VPath *bam;
    const VPath *idx = NULL;

    rc = VPathMakeSysPath(&bam, bamName);
    if (rc)
        return;
    if (idxName) {
        rc = VPathMakeSysPath(&idx, idxName);
        if (rc)
            return;
    }
    
    rc = AlignAccessMgrMake(&mgr);
    if (rc)
        return;
    if (idx) {
        rc = AlignAccessMgrMakeIndexBAMDB(mgr, &db, bam, idx);
        VPathRelease(idx);
    }
    else {
        rc = AlignAccessMgrMakeBAMDB(mgr, &db, bam);
    }
    VPathRelease(bam);
    AlignAccessMgrRelease(mgr);
    if (rc)
        return;
    if (idx) {
        unsigned n;
        unsigned mapped;
        uint64_t min;
        uint64_t max;
        clock_t stime = clock();
        float elapsed;
        
        mapped = n = 0;
        rc = AlignAccessDBWindowedAlignments(db, &e, refSeqID, start, len);
        if (rc == 0) {
            AlignAccessAlignmentEnumeratorGetRefSeqPos(e, &min);
            do {
                ++n;
                AlignAccessAlignmentEnumeratorGetRefSeqPos(e, &max);
#if 0
                print1(e);
#endif
            } while (AlignAccessAlignmentEnumeratorNext(e) == 0);
            AlignAccessAlignmentEnumeratorRelease(e);
            elapsed = (clock() - stime)/(double)CLOCKS_PER_SEC;
            printf("%u alignments in %.3f sec ( %.0f per sec ), min: %llu; max: %llu\n",
                   n, elapsed, n / elapsed, min + 1, max + 1);
        }
    }
    else {
        rc = AlignAccessDBEnumerateAlignments(db, &e);
        if (rc == 0) {
            unsigned n = 0;
            clock_t stime = clock();
            float elapsed;
            
            do { ++n; } while (AlignAccessAlignmentEnumeratorNext(e) == 0);
            AlignAccessAlignmentEnumeratorRelease(e);
            elapsed = (clock() - stime)/(double)CLOCKS_PER_SEC;
            printf("%u alignments in %.3f sec ( %.0f per sec )\n",
                   n, elapsed, n / elapsed);
        }
    }
    AlignAccessDBRelease(db);
}

static
void testGetCIGAR(const char *bamName) {
    const AlignAccessMgr *mgr;
    const AlignAccessDB *db;
    AlignAccessAlignmentEnumerator *e;
    uint64_t pos;
    char buffer[4096];
    size_t in_buf;
    size_t buflen;
    rc_t rc;
    uint64_t i = 0;
    VPath *bam;

    rc = VPathMakeSysPath(&bam, bamName);
    if (rc)
        return;    
        
    rc = AlignAccessMgrMake(&mgr);
    if (rc)
        return;
    rc = AlignAccessMgrMakeBAMDB(mgr, &db, bam);
    VPathRelease(bam);
    AlignAccessMgrRelease(mgr);
    if (rc)
        return;
    
    rc = AlignAccessDBEnumerateAlignments(db, &e);
    if (rc == 0) {
        do {
        	++i;
            AlignAccessAlignmentEnumeratorGetRefSeqPos(e, &pos);
            AlignAccessAlignmentEnumeratorGetCIGAR(e, &pos, buffer, sizeof(buffer), &in_buf);
            buflen = strlen(buffer);
            if (!(in_buf == 0 || buflen + 1 == in_buf)) {
            	printf("failed at position %lu, record number %lu\n", pos, i);
            	printf("in_buf: %lu\tbuflen: %lu\n", in_buf, buflen);
            	abort();
            }
        } while (AlignAccessAlignmentEnumeratorNext(e) == 0);
        AlignAccessAlignmentEnumeratorRelease(e);
    }
    AlignAccessDBRelease(db);
}

uint32_t CC KAppVersion (void)
{
    return 1;
}

const char UsageDefaultName[] = "align-test";
rc_t CC UsageSummary (const char * progname)
{
    return 0;
}
rc_t CC Usage ( const Args * args )
{
    return 0;
}

#define BASEDIR "/net/traces03/1kg_pilot_data/data/NA10851/alignment/"
#define BAMFILE "NA10851.SLX.maq.SRP000031.2009_08.bam"

rc_t KMain(int argc, char *argv[])
{
    KConfigDisableUserSettings();

    test(BASEDIR BAMFILE,
         BASEDIR BAMFILE ".bai",
         "1", 1000, 1000);
#if 0
    test("/panfs/traces03/1000genomes/ftp/data/HG00096/alignment/HG00096.chrom20.ILLUMINA.bwa.GBR.low_coverage.20101123.bam",
         "/panfs/traces03/1000genomes/ftp/data/HG00096/alignment/HG00096.chrom20.ILLUMINA.bwa.GBR.low_coverage.20101123.bam.bai",
         "20", 0, 100000);
#endif
    return 0;
}
