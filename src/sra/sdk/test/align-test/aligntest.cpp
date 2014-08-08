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

/**
* Unit tests for Alignment iterators
*/
#include <ktst/unit_test.hpp>

#include <cstring>
#include <cstdlib>

#include <klib/callconv.h>
#include <klib/rc.h>
#include <klib/printf.h>
#include <insdc/insdc.h>
#include <align/iterator.h>
#include <align/manager.h>
#include <align/refseq-mgr.h>

#include <common/test_assert.h>

using namespace std;

TEST_SUITE(AlignTestSuite);

///////////////////////////////////////////////// RefSeqMgr test cases

TEST_CASE(KTestRefSeqMgrExists)
{
    const VDBManager *vmgr;
    REQUIRE_RC(VDBManagerMakeRead(&vmgr, NULL));
    const RefSeqMgr* mgr;
    REQUIRE_RC(RefSeqMgr_Make(&mgr, vmgr, 0, 0, 0));
    string acc("NC_000001.10");
    REQUIRE_RC(RefSeqMgr_Exists(mgr, acc.c_str(), acc.size(), NULL));
    REQUIRE_RC(RefSeqMgr_Release(mgr));
    REQUIRE_RC(VDBManagerRelease(vmgr));
}

TEST_CASE(KTestRefSeqMgrExistsInArchive)
{
    const VDBManager *vmgr;
    REQUIRE_RC(VDBManagerMakeRead(&vmgr, NULL));
    const RefSeqMgr* mgr;
    REQUIRE_RC(RefSeqMgr_Make(&mgr, vmgr, 0, 0, 0));
    string acc("AAAB01000001.1");
    REQUIRE_RC(RefSeqMgr_Exists(mgr, acc.c_str(), acc.size(), NULL));
    REQUIRE_RC(RefSeqMgr_Release(mgr));
    REQUIRE_RC(VDBManagerRelease(vmgr));
}

///////////////////////////////////////////////// Expected/Actual data matching for AlignIterator tests

class AlignIterFixture
{
public:
    typedef struct 
    {
        int32_t             state;
        INSDC_coord_zero    pos;

        uint32_t                inserted;
        const INSDC_4na_bin*    insBases;

        uint32_t            deleted;
        INSDC_coord_zero    delPos;
    } Expected;

    const AlignMgr *mgr;
    AlignmentIterator* iter;

    AlignIterFixture() : mgr(0), iter(0)
    {
        if (AlignMgrMakeRead(&mgr) != 0)
        {
            FAIL("AlignMgrMakeRead failed");
        }
    }
    ~AlignIterFixture()
    {
        if (iter != 0 && AlignmentIteratorRelease(iter) != 0)
        {
            FAIL("AlignmentIteratorRelease failed");
        }
        if (AlignMgrRelease(mgr) != 0)
        {
            FAIL("AlignMgrRelease failed");
        }
    }

    string StateToString(int32_t state)
    {
        char buf[1024];
        buf[0] = INSDC_4na_map_CHARSET[state & 15];
        buf[1] = 0;
        if (state & align_iter_match)     strcat(buf, " match");
        if (state & align_iter_skip)      strcat(buf, " skip");
        if (state & align_iter_insert)    strcat(buf, " insert");
        if (state & align_iter_delete)    strcat(buf, " delete");
        if (state & align_iter_first)     strcat(buf, " first");
        if (state & align_iter_last)      strcat(buf, " last");
        if (state & align_iter_invalid)   strcat(buf, " invalid");
        return string(buf);
    }

    bool MatchCurrent(AlignmentIterator * iter, Expected* exp, bool quiet=false)
    {
        INSDC_coord_zero seq_pos;
        int32_t state=AlignmentIteratorState(iter, &seq_pos);
        if (state != exp->state)    // TODO: verify expected pos
        {
            if (!quiet)
            {
                char buf[1024];
                size_t num_writ;
                string_printf(buf, sizeof(buf), &num_writ, 
                                "AlignmentIterator: unexpected State\n  expected='%s'\n    actual='%s'", 
                                StateToString(exp->state).c_str(), StateToString(state).c_str());
                TEST_MESSAGE(buf);
            }
            return false;
        }

        if (AlignmentIteratorPosition(iter, &seq_pos) != 0) // TODO: verify expected pos
        {
            if (!quiet) TEST_MESSAGE("AlignmentIterator: unexpected Position\n");
            return false;
        }

        const INSDC_4na_bin* insBases;
        uint32_t inserted=AlignmentIteratorBasesInserted (iter, &insBases);
        if (inserted != exp->inserted)
        {
            if (!quiet) TEST_MESSAGE("AlignmentIterator: mismatched BasesInserted\n");
            return false;
        }
        if (inserted != 0)
        {
            if (insBases == 0)
            {
                if (exp->insBases != 0)
                {
                    if (!quiet) TEST_MESSAGE("AlignmentIterator: missing BasesInserted/insBases\n");
                    return false;
                }
            }
            else if (exp->insBases == 0)
            {
                if (!quiet) TEST_MESSAGE("AlignmentIterator: unexpected BasesInserted/insBases\n");
                return false;
            }
            else if (memcmp(insBases, exp->insBases, inserted) != 0)
            {
                if (!quiet) TEST_MESSAGE("AlignmentIterator: mismatched BasesInserted/insBases\n");
                return false;
            }
        }

        INSDC_coord_zero delPos;
        uint32_t deleted=AlignmentIteratorBasesDeleted(iter, &delPos);
        if (deleted != exp->deleted)
        {
            if (!quiet) TEST_MESSAGE("AlignmentIterator: mismatched BasesDeleted\n");
            return false;
        }
        if (deleted != 0 && delPos != exp->delPos)
        {
            if (!quiet) TEST_MESSAGE("AlignmentIterator: mismatched BasesDeleted/pos\n");
            return false;
        }
        return true;
    }
    rc_t Match(AlignmentIterator * iter, Expected* exp)
    {
        while (true) 
        {
            rc_t rc=AlignmentIteratorNext(iter);
            if (rc != 0)
            {
                if (exp->state != align_iter_invalid)
                {
                    TEST_MESSAGE("AlignmentIterator: premature end of iteration\n");
                    return rc;
                }
                return 0; // this is the only happy exit
            }
            else if (exp->state == align_iter_invalid)
            {
                TEST_MESSAGE("AlignmentIterator: extra steps available\n");
                return -1;
            }
            if (!MatchCurrent(iter, exp))
            {
                return -1;
            }
        }
    }
};

///////////////////////////////////////////////// AlignIterator test cases

INSDC_4na_bin A_4na=1;
INSDC_4na_bin C_4na=2;
INSDC_4na_bin G_4na=4;
INSDC_4na_bin T_4na=8;
INSDC_4na_bin N_4na=15;

// regular operation
// - AlignMgrMakeAlignmentIterator should return an iterator with refcount 1
// - AlignmentIteratorAddRef/AlignmentIteratorRelease increase/decrease ref count, last Release frees (?)
// - AlignmentIteratorRelease accepts NULL
// 
// + AlignMgrMakeAlignmentIterator(read len=0) should return an iterator that does not produce any results
//
// use prepared test read including matches, mismatches (different bases), inserts, deletes
// + AlignMgrMakeAlignmentIterator(non-empty read), should return an iterator that does produce expected results:
//    + use Next to advance
//    + use State to match against expected results
//    + repeated calls to State return the same result until the next Next
//    + verify Position
//    + verify BasesInserted where expected
//    + verify BasesDeleted where expected
//
// error cases:
// - bad arguments to AlignMgrMakeAlignmentIterator 
// + NULL argument to Next
// - NULL argument to State
// + State called before first Next
// + State called after the end of the read
// - Next called after the end of the read
// + Position called before first Next
// + Position called after the end of the read
// + BasesInserted called in a non-insert position
// + BasesDeleted called in a non-delete position
// - unexpected characters in read (?)
FIXTURE_TEST_CASE(KTestAlignmentIteratorStateNULL, AlignIterFixture)
{
    bool has_mismatch[]     ={ 0};
    bool has_ref_offset[]   ={ 0};
    const INSDC_4na_bin A[] ={ A_4na}; 
    REQUIRE_RC(AlignMgrMakeAlignmentIterator(mgr, &iter, false, 0, 1, A, 1, has_mismatch, has_ref_offset, 0, 0, 0, 0));
    INSDC_coord_zero pos;
    REQUIRE_RC_FAIL(AlignmentIteratorState(0, &pos));
    REQUIRE_RC_FAIL(AlignmentIteratorState(iter, 0));
}

FIXTURE_TEST_CASE(KTestAlignIterEmptyRead, AlignIterFixture)
{
    REQUIRE_RC_FAIL(AlignMgrMakeAlignmentIterator(mgr, &iter, false, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
}

#if broken
FIXTURE_TEST_CASE(KTestAlignIterMatch1, AlignIterFixture)
{
    // reference at index 100:  "A"
    // read:                    "A"
    const INSDC_4na_bin read[]  ={ A_4na }; 
    bool has_mismatch[]         ={ 0};
    bool has_ref_offset[]       ={ 0};
    REQUIRE_RC(AlignMgrMakeAlignmentIterator(mgr, 
                                             &iter, 
                                             false, 
                                             100, 1, 
                                             read, 1, 
                                             has_mismatch, 
                                             has_ref_offset, 
                                             NULL, 0));

    // Return values expected from State() after each Next():
    static Expected expected[]=
    {
        { A_4na | align_iter_match | align_iter_first | align_iter_last,  100, 0, 0, 0, 0},
        { align_iter_invalid},
    };
    REQUIRE_RC(Match(iter, expected));
}

FIXTURE_TEST_CASE(KTestAlignIterMatch2, AlignIterFixture)
{
    // reference at index 100:  "AC"
    // read:                    "AC"
    const INSDC_4na_bin read[]  ={ A_4na, C_4na }; 
    bool has_mismatch[]         ={ 0, 0};
    bool has_ref_offset[]       ={ 0, 0};
    REQUIRE_RC(AlignMgrMakeAlignmentIterator(mgr, 
                                             &iter, 
                                             false, 
                                             100, 2, 
                                             read, 2, 
                                             has_mismatch, 
                                             has_ref_offset, 
                                             NULL, 0));

    // Return values expected from State() after each Next():
    static Expected expected[]=
    {
        { A_4na | align_iter_match | align_iter_first,  100, 0, 0, 0, 0},
        { C_4na | align_iter_match | align_iter_last,   100, 0, 0, 0, 0},
        { align_iter_invalid},
    };
    REQUIRE_RC(Match(iter, expected));
}

FIXTURE_TEST_CASE(KTestAlignIterMismatch, AlignIterFixture)
{
    // reference at index 100:  "A"
    // read:                    "C"
    const INSDC_4na_bin read[]  ={ C_4na }; 
    bool has_mismatch[]         ={ 1 };
    bool has_ref_offset[]       ={ 0 };
    REQUIRE_RC(AlignMgrMakeAlignmentIterator(mgr, 
                                             &iter, 
                                             false, 
                                             100, 1, 
                                             read, 1, 
                                             has_mismatch, 
                                             has_ref_offset, 
                                             NULL, 0));

    // Return values expected from State() after each Next():
    static Expected expected[]=
    {
        { C_4na | align_iter_first | align_iter_last,  100, 0, 0, 0, 0 },
        { align_iter_invalid },
    };
    REQUIRE_RC(Match(iter, expected));
}

FIXTURE_TEST_CASE(KTestAlignIterSkip, AlignIterFixture)
{
    // reference at index 100:  "ACT"
    // read:                    "AxT" (x - skipped)
    const INSDC_4na_bin read[]  ={ A_4na, T_4na }; 
    bool has_mismatch[]         ={ 0, 0 };
    bool has_ref_offset[]       ={ 0, 1 };
    int32_t ref_offest          ={ -1 };
    REQUIRE_RC(AlignMgrMakeAlignmentIterator(mgr, 
                                             &iter, 
                                             false, 
                                             100, 3, 
                                             read, 2, 
                                             has_mismatch, 
                                             has_ref_offset, 
                                             &ref_offest, 1));

    // Return values expected from State() after each Next():
    static Expected expected[]=
    {
        { A_4na | align_iter_match | align_iter_first,  100, 0, 0, 0, 0 },
        {         align_iter_skip,                      101, 0, 0, 0, 0 },
        { T_4na | align_iter_match | align_iter_last,   102, 0, 0, 0, 0 },
        { align_iter_invalid },
    };
    REQUIRE_RC(Match(iter, expected));
}

FIXTURE_TEST_CASE(KTestAlignIterRead, AlignIterFixture)
{
    // reference at index 100:        ‘AxCGTTN’ (x = skipped)
    // read “AAAGCATN”, aligned as: ‘AAAGCATxN’ (x = skipped)

    // call to AlignMgrMakeAlignmentIterator:
    //  ref_pos=100
    //  ref_len=6
    //  read= AAACATN
    //  read_len=8
    //  has_mismatch=   { 0, 0, 0, 0, 0, 1, 0, 1}
    //  has_ref_offset= { 1, 0, 0, 1, 0, 0, 0, 1}
    //  ref_offest= { -2, 1, 1}
    //  ref_offset_len= 3
    bool has_mismatch[]     ={ 0, 0, 0, 0, 0, 1, 0, 1};
    bool has_ref_offset[]   ={ 1, 0, 0, 1, 0, 0, 0, 1};
    int32_t ref_offset[]    ={ -2, 1, 1};
    const INSDC_4na_bin AAACATN[] ={ A_4na, A_4na, A_4na, C_4na, A_4na, T_4na, N_4na}; 
    REQUIRE_RC(AlignMgrMakeAlignmentIterator(mgr, 
                                             &iter, 
                                             false, 
                                             100, 6, 
                                             AAACATN, sizeof(AAACATN), 
                                             has_mismatch, 
                                             has_ref_offset, 
                                             ref_offset, sizeof(ref_offset)));

    // Return values expected from State() after each Next():
    //  A | align_iter_match; Position returns 100
    //  A | align_iter_insert; BasesInserted returns 1, *bases == "GCATN"; Position returns 101
    //  C | align_iter_match; Position returns 101
    //  A; Position returns 102
    //  T | align_iter_match; Position returns 103
    //  0 | align_iter_delete; BasesDeleted returns 1, *pos == 104; Position returns 105
    //  align_iter_invalid
    const INSDC_4na_bin GCATN[] = { 4, 2, 1, 8, 0};
    static Expected expected[]=
    {
        { A_4na | align_iter_match,  100, 0, 0,         0, 0},
        {        577/*align_iter_insert*/, 100, 1, GCATN,     0, 0},
        { C_4na | align_iter_match,  101, 0, 0,         0, 0},
        { A_4na                   ,  102, 0, 0,         0, 0},
        { T_4na | align_iter_match,  103, 0, 0,         0, 0},
        {         align_iter_skip,   104, 0, 0,         0, 0},
        { N_4na | align_iter_match,  105, 0, 0,         0, 0},
        { align_iter_invalid},
    };
    REQUIRE_RC(Match(iter, expected));
}

FIXTURE_TEST_CASE(KTestAlignIterConsequtiveStates, AlignIterFixture)
{   //- repeated calls to State return the same result until the next Next
    bool has_mismatch[]     ={ 0, 1};
    bool has_ref_offset[]   ={ 0, 0};
    const INSDC_4na_bin AA[] ={ 1, 1};
    REQUIRE_RC(AlignMgrMakeAlignmentIterator(mgr, &iter, false, 0, 2, AA, sizeof(AA), has_mismatch, has_ref_offset, 0, 0));

    static Expected expected[]=
    {
        { 0,   0, 0, 0,  0, 0},
        { 'A', 1, 1, AA, 0, 0},
        { align_iter_invalid},
    };
    REQUIRE_RC(AlignmentIteratorNext(iter));
    // repeated calls to State etc. produce same results until the next call to Next
    REQUIRE(MatchCurrent(iter, expected));
    REQUIRE(MatchCurrent(iter, expected));
    REQUIRE(MatchCurrent(iter, expected));
    REQUIRE_RC(AlignmentIteratorNext(iter));
    // now results change
    REQUIRE(!MatchCurrent(iter, expected, true)); // true=do not report discrepancy
}

FIXTURE_TEST_CASE(KTestAlignNextNull, AlignIterFixture)
{   // NULL argument to Next
    REQUIRE_RC_FAIL(AlignmentIteratorNext(NULL));
}

FIXTURE_TEST_CASE(KTestAlignIterUsedBeforeNext, AlignIterFixture)
{   // calls to State, Position, BasesInserted, BasesDeleted before the first Next return appropriate values
    // BasesInserted called in a non-insert position
    // BasesDeleted called in a non-delete position
    
    bool has_mismatch[]     ={ 0};
    bool has_ref_offset[]   ={ 0};
    const INSDC_4na_bin A[] ={ 1}; 

    REQUIRE_RC(AlignMgrMakeAlignmentIterator(mgr, &iter, false, 0, 1, A, sizeof(A), has_mismatch, has_ref_offset, 0, 0));

    INSDC_coord_zero seq_pos;
    REQUIRE_EQ(AlignmentIteratorState(iter, &seq_pos), (int32_t)align_iter_invalid);
    REQUIRE_RC_FAIL(AlignmentIteratorPosition(iter, &seq_pos));
    REQUIRE_EQ(seq_pos, (INSDC_coord_zero)0); // not sure this is correct
    const INSDC_4na_bin* bases; 
    REQUIRE_EQ(AlignmentIteratorBasesInserted(iter, &bases), (uint32_t)0);
    REQUIRE_EQ(bases, (const INSDC_4na_bin*)0); // not sure this is correct
    REQUIRE_EQ(AlignmentIteratorBasesDeleted(iter, &seq_pos), (uint32_t)0);
    REQUIRE_EQ(seq_pos, (INSDC_coord_zero)0); // not sure this is correct

    // advance, see a valid state
    REQUIRE_RC(AlignmentIteratorNext(iter));
    REQUIRE_NE(AlignmentIteratorState(iter, &seq_pos), (int32_t)align_iter_invalid);
    
    // BasesInserted and BasesDeleted in a valid but non-insert/delete position
    REQUIRE_EQ(AlignmentIteratorBasesInserted(iter, &bases), (uint32_t)0);
    REQUIRE_EQ(bases, (const INSDC_4na_bin*)0); // not sure this is correct
    REQUIRE_EQ(AlignmentIteratorBasesDeleted(iter, &seq_pos), (uint32_t)0);
    REQUIRE_EQ(seq_pos, (INSDC_coord_zero)0); // not sure this is correct
}

FIXTURE_TEST_CASE(KTestAlignIterUsedAfterLast, AlignIterFixture)
{   // calls to State, Position, BasesInserted, BasesDeleted after the end of read return appropriate values
    bool has_mismatch[]     ={ 0};
    bool has_ref_offset[]   ={ 0};
    const INSDC_4na_bin A[] ={ 1}; //"A"
    REQUIRE_RC(AlignMgrMakeAlignmentIterator(mgr, &iter, false, 0, 1, A, sizeof(A), has_mismatch, has_ref_offset, 0, 0));

    // advance, see a valid state
    REQUIRE_RC(AlignmentIteratorNext(iter));
    INSDC_coord_zero seq_pos;
    REQUIRE_NE(AlignmentIteratorState(iter, &seq_pos), (int32_t)align_iter_invalid);

    // advance, end of read
    REQUIRE_RC_FAIL(AlignmentIteratorNext(iter));
    REQUIRE_EQ(AlignmentIteratorState(iter, &seq_pos), (int32_t)align_iter_invalid);
    REQUIRE_RC_FAIL(AlignmentIteratorPosition(iter, &seq_pos));
    REQUIRE_EQ(seq_pos, (INSDC_coord_zero)0); // not sure this is correct
    const INSDC_4na_bin* bases; 
    REQUIRE_EQ(AlignmentIteratorBasesInserted(iter, &bases), (uint32_t)0);
    REQUIRE_EQ(bases, (const INSDC_4na_bin*)0); // not sure this is correct
    REQUIRE_EQ(AlignmentIteratorBasesDeleted(iter, &seq_pos), (uint32_t)0);
    REQUIRE_EQ(seq_pos, (INSDC_coord_zero)0); // not sure this is correct

}
#endif

///////////////////////////////////////////////// PlacementIterator test cases

// regular operation
// - AlignMgrMakePlacementIterator  should return an iterator with refcount 1
// - AlignMgrMakePlacementIteratorAddRef/AlignMgrMakePlacementIteratorRelease increase/decrease ref count, last Release frees (?)
// - AlignMgrMakePlacementIteratorRelease accepts NULL
// 
// use a prepared database with 1 reference and several overlaping alignments
// - AlignMgrMakePlacementIterator(reference segmant that has no placements) should return an iterator that does not produce any results
// - AlignMgrMakePlacementIterator(reference segmant that has placements, primary), should return an iterator that does produce expected results
//   - use NextAvailPos to advance to next position
//   - use NextAvailPos to jump over a gap, segment
//   - use NextRecordAt to retrieve a placement record
//   - verify NextIdAt
//   - define custom population/destruction callbacks, make sure thay are called
//
// - AlignMgrMakePlacementIterator(reference segmant that has no placements) should return an iterator that does not produce any results
// - AlignMgrMakePlacementIterator(reference segmant that has no placements, secondary), should return an iterator that does produce expected results
//
// error cases
// TBD

//////////////////////////////////////////// Main
extern "C"
{

#include <kapp/args.h>
#include <kfg/config.h>

ver_t CC KAppVersion ( void )
{
    return 0x1000000;
}
rc_t CC UsageSummary (const char * progname)
{
    return 0;
}

rc_t CC Usage ( const Args * args )
{
    return 0;
}
const char UsageDefaultName[] = "test-align";

rc_t CC KMain ( int argc, char *argv [] )
{
    KConfigDisableUserSettings();
    rc_t rc=AlignTestSuite(argc, argv);
    return rc;
}

}
