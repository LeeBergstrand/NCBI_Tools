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

#include <cstdlib>
#include <stdexcept>

#include <ktst/unit_test.hpp>
#include <klib/report.h>

#include "bam-reader.h"

#include <common/test_assert.h>

using namespace std;

TEST_SUITE(BamReaderTestSuite);

TEST_CASE(TestBamReaderRead)
{
    const BAMFile* r;
    REQUIRE_RC(BAMFileMake(&r, "./wb-test-bam-loader.bam"));
    const BAMAlignment *rec;
    uint16_t flags; 
    
#define REQUIRE_ALIGNMENT_FLAGS(expFlags)  \
    REQUIRE_RC(BAMFileRead(r, &rec)); \
    REQUIRE_RC(BAMAlignmentGetFlags(rec, &flags)); \
    REQUIRE_EQ(flags, (uint16_t)(expFlags)); \
    REQUIRE_RC(BAMAlignmentRelease(rec));
    
    REQUIRE_ALIGNMENT_FLAGS(97);  // primary, paired, mate_reverse
    REQUIRE_ALIGNMENT_FLAGS(147); // primary, paired, self_reverse
    REQUIRE_ALIGNMENT_FLAGS(354); // secondary, not paired
    REQUIRE_ALIGNMENT_FLAGS(402); // secondary, not paired
    REQUIRE_ALIGNMENT_FLAGS(359); // secondary, paired, unaligned
    REQUIRE_ALIGNMENT_FLAGS(403); // secondary, paired
    REQUIRE_ALIGNMENT_FLAGS(867); // secondary, paired, low quality
    REQUIRE_ALIGNMENT_FLAGS(1427); // secondary, paired, duplicate
#undef REQUIRE_ALIGNMENT_FLAGS
    
    REQUIRE_RC(BAMFileRelease(r));
}

////////////////// APi tests

class ApiFixture
{
public:
    ApiFixture()
    : self(0), rec(0), rej(0), seq(0), align(0), cg(0), ref(0)
    {
    }
    ~ApiFixture()
    {
        if (self)
            ReaderFileRelease(self);
        if (rec)
            RecordRelease(rec);
        if (rej)
            RejectedRelease(rej);
        if (seq)
            SequenceRelease(seq);
        if (align)
            AlignmentRelease(align);
        if (cg)
            CGDataRelease(cg);
        if (ref)
            ReferenceInfoRelease(ref);
    }
    
    void FileMake()
    {
        if (BamReaderFileMake( &self, NULL, "./wb-test-bam-loader.bam") != 0)
            throw logic_error("FileMake: BamReaderFileMake failed");
    }
    void GetRecord()
    {
        FileMake();
        if (ReaderFileGetRecord( self, &rec) != 0 || rec == 0)
            throw logic_error("GetRecord: ReaderFileGetRecord failed");
    }
    void NextRecord()
    {
        if (rec)
            RecordRelease(rec);
        if (ReaderFileGetRecord( self, &rec) != 0 || rec == 0)
            throw logic_error("NextRecord: ReaderFileGetRecord failed");
    }
    void GetSequence()
    {
        GetRecord();
        if (RecordGetSequence(rec, &seq) != 0 || seq == 0)
            throw logic_error("GetSequence: RecordGetSequence failed");
    }
    void NextSequence()
    {
        if (rec)
            RecordRelease(rec);
        if (ReaderFileGetRecord( self, &rec) != 0 || rec == 0)
            throw logic_error("NextSequence: ReaderFileGetRecord failed");
        if (seq)
            SequenceRelease(seq);
        if (RecordGetSequence(rec, &seq) != 0 || seq == 0)
            throw logic_error("NextSequence: RecordGetSequence failed");
    }
    void GetAlignment()
    {
        GetRecord();
        if (align)
            AlignmentRelease(align);
        if (RecordGetAlignment(rec, &align))
            throw logic_error("GetAlignment: RecordGetAlignment failed");
    }
    void NextAlignment()
    {
        NextSequence();
        if (align)
            AlignmentRelease(align);
        if (RecordGetAlignment(rec, &align))
            throw logic_error("NextAlignment: RecordGetAlignment failed");
    }
    void GetCGData()
    {   // fast-forward to the first line with GS data (fail if not found)
        FileMake();
        do
        {
            if (rec) 
                RecordRelease(rec);
            if (seq) 
                SequenceRelease(seq);
            if (align) 
                AlignmentRelease(align);
            if (cg) 
                CGDataRelease(cg);
                
            if (ReaderFileGetRecord( self, &rec ) != 0)
                throw logic_error("GetCGData: ReaderFileGetRecord failed");
            if (rec == 0)
                throw logic_error("GetCGData: premature end of file");
            if (RecordGetSequence( rec, &seq ) != 0 || seq == 0)
                throw logic_error("GetCGData: RecordGetSequence failed");
            if (RecordGetAlignment( rec, &align ) != 0)
                throw logic_error("GetCGData: RecordGetAlignment failed");
            if (align) // skip unaligned records
            {
                if (AlignmentGetCGData( align, &cg ) != 0)
                    throw logic_error("GetCGData: AlignmentGetCGData failed");
            }
           
        }
        while(cg == NULL);
    }
    void GetReferenceInfo()
    {
        FileMake();
        if (ReaderFileGetReferenceInfo(self, &ref) != 0)
                throw logic_error("GetReferenceInfo: ReaderFileGetReferenceInfo failed");
    }
    
    const ReaderFile*       self;
    const Record*           rec;
    const Rejected*         rej;
    const Sequence*         seq;
    const Alignment*        align;
    const CGData*           cg;
    const ReferenceInfo*    ref;
};

FIXTURE_TEST_CASE(TestBamReaderFileMake, ApiFixture)
{
    string file("./wb-test-bam-loader.bam");
    REQUIRE_RC(BamReaderFileMake( &self, " ", file.c_str()));
    REQUIRE_EQ( string(ReaderFileGetPathname(self)), file );  
    
}
    
FIXTURE_TEST_CASE(TestReaderFileGetRecord, ApiFixture)
{
    FileMake();
    for (size_t i=0; i < 10; ++i)
    {
        if (rec)
            REQUIRE_RC(RecordRelease(rec));
        REQUIRE_RC(ReaderFileGetRecord( self, &rec));
        REQUIRE_NOT_NULL(rec);
    }
    if (rec)
        REQUIRE_RC(RecordRelease(rec));
    REQUIRE_RC(ReaderFileGetRecord( self, &rec));
    REQUIRE_NULL(rec); // EOF
}   

FIXTURE_TEST_CASE(TestReaderFileGetProportionalPosition, ApiFixture)
{
    FileMake();
    // the test file is too small to show gradual progress from 0.0 to 1.0, so the expected results below are a bit odd
    REQUIRE_CLOSE(ReaderFileGetProportionalPosition( self ), 0.3, 0.1f); 
    const size_t TotalRecords=10;
    for (size_t i=0; i < TotalRecords; ++i)
    {
        if (rec)
            REQUIRE_RC(RecordRelease(rec));
        REQUIRE_RC(ReaderFileGetRecord( self, &rec));
        REQUIRE_CLOSE(ReaderFileGetProportionalPosition( self ), 0.95f, 0.01f); 
    }
    if (rec)
        REQUIRE_RC(RecordRelease(rec));
    REQUIRE_RC(ReaderFileGetRecord( self, &rec));
    REQUIRE_CLOSE(ReaderFileGetProportionalPosition( self ), 1.0f, 0.0001f); // end of file
}   

FIXTURE_TEST_CASE(TestGetRejectedFail, ApiFixture)
{   // no rejected records for BAM
    GetRecord();
    REQUIRE_RC(RecordGetRejected(rec, &rej));
    REQUIRE_NULL(rej);
}

FIXTURE_TEST_CASE(TestReaderFileGetReferenceInfo, ApiFixture)
{   
    FileMake();
    REQUIRE_RC(ReaderFileGetReferenceInfo(self, &ref));
    REQUIRE_NOT_NULL(ref);
}

// Sequence messages
FIXTURE_TEST_CASE(TestGetSequence, ApiFixture)
{
    GetRecord();
    REQUIRE_RC(RecordGetSequence(rec, &seq));
    REQUIRE_NOT_NULL(seq);
}
FIXTURE_TEST_CASE(TestSequenceGetRead, ApiFixture)
{
    GetSequence();
    
    uint32_t length = 0;
    REQUIRE_RC(SequenceGetReadLength ( seq,  & length) );
    REQUIRE_EQ(length, 10u);
    
    char* buf = new char[length];
    REQUIRE_RC(SequenceGetRead(seq, buf));
    REQUIRE_EQ(string(buf, length), string("AGCTTGGTTT"));
    delete [] buf;
}
FIXTURE_TEST_CASE(TestSequenceGetRead2, ApiFixture)
{
    GetSequence();
    
    uint32_t start=2;
    uint32_t stop=5;
    char* buf = new char[stop-start];
    REQUIRE_RC(SequenceGetRead2(seq, buf, start, stop));
    REQUIRE_EQ(string(buf, stop-start), string("CTT"));
    delete [] buf;
}
FIXTURE_TEST_CASE(TestSequenceGetQuality, ApiFixture)
{
    GetSequence();
    const int8_t* quality=0;
    uint8_t offset=0;
    int qualType=0;
    REQUIRE_RC(SequenceGetQuality(seq, &quality, &offset, &qualType));
    REQUIRE_NOT_NULL(quality);
    REQUIRE_EQ(quality[0], (int8_t)'I');
    REQUIRE_EQ(quality[1], (int8_t)'%');
    REQUIRE_EQ(quality[2], (int8_t)'I');
    REQUIRE_EQ(offset, (uint8_t)33);
    REQUIRE_EQ(qualType, (int)QT_Phred);
}

// Color space
FIXTURE_TEST_CASE(TestSequenceIsColorSpace, ApiFixture)
{
    GetSequence();
    REQUIRE( SequenceIsColorSpace(seq) );
    NextSequence();
    REQUIRE( ! SequenceIsColorSpace(seq) );
}
FIXTURE_TEST_CASE(TestSequenceGetCSRead, ApiFixture)
{
    GetSequence();
    
    char key;
    REQUIRE_RC( SequenceGetCSKey(seq, &key) ); 
    REQUIRE_EQ(key, 'A');
    
    uint32_t length = 0;
    REQUIRE_RC(SequenceGetCSReadLength ( seq,  & length) );
    REQUIRE_EQ(length, 9u);
    
    char* buf = new char[length];
    REQUIRE_RC(SequenceGetCSRead(seq, buf));
    REQUIRE_EQ(string(buf, length), string("213322333"));
    delete [] buf;

    const int8_t* quality=0;
    uint8_t offset=0;
    int qualType=0;
    REQUIRE_RC(SequenceGetCSQuality(seq, &quality, &offset, &qualType));
    REQUIRE_NOT_NULL(quality);
    REQUIRE_EQ(quality[0], (int8_t)'&');
    REQUIRE_EQ(quality[1], (int8_t)'"');
    REQUIRE_EQ(quality[2], (int8_t)'"');
    REQUIRE_EQ(offset, (uint8_t)33);
    REQUIRE_EQ(qualType, (int)QT_Phred);
}

FIXTURE_TEST_CASE(TestSequenceGetSpotGroup, ApiFixture)
{
    GetSequence();
    const char *name = 0;
    size_t length = 0;
    REQUIRE_RC(SequenceGetSpotGroup(seq, &name, &length ));
    REQUIRE_EQ(string(name, length), string("SpotGroup"));
}
FIXTURE_TEST_CASE(TestSequenceGetSpotName, ApiFixture)
{
    GetSequence();
    const char *name = 0;
    size_t length = 0;
    REQUIRE_RC(SequenceGetSpotName(seq, &name, &length ));
    REQUIRE_EQ(string(name, length), string("SpotGroup.SpotName"));
}

FIXTURE_TEST_CASE(TestSequenceWasPaired, ApiFixture)
{
    GetSequence();
    REQUIRE( SequenceWasPaired(seq) );
    NextSequence();
    NextSequence();
    REQUIRE( ! SequenceWasPaired(seq) );
}
FIXTURE_TEST_CASE(TestSequenceIsReverse, ApiFixture)
{
    GetSequence();
    REQUIRE_EQ( SequenceGetOrientationSelf(seq), (int)ReadOrientationForward );
    REQUIRE_EQ( SequenceGetOrientationMate(seq), (int)ReadOrientationReverse );
}

FIXTURE_TEST_CASE(TestSequenceIsFirst, ApiFixture) 
{
    GetSequence();
    REQUIRE( SequenceIsFirst(seq) );
    REQUIRE( ! SequenceIsSecond(seq) );
    NextSequence();
    REQUIRE( ! SequenceIsFirst(seq) );
    REQUIRE( SequenceIsSecond(seq) );
}
FIXTURE_TEST_CASE(TestSequenceIsDuplicate, ApiFixture)
{
    GetSequence();
    NextSequence();
    NextSequence();
    NextSequence();
    NextSequence();
    NextSequence();
    NextSequence();
    REQUIRE( ! SequenceIsDuplicate(seq) );
    NextSequence();
    REQUIRE( SequenceIsDuplicate(seq) );
}
FIXTURE_TEST_CASE(TestSequenceIsLowQuality, ApiFixture)
{
    GetSequence();
    NextSequence();
    NextSequence();
    NextSequence();
    NextSequence();
    NextSequence();
    REQUIRE( ! SequenceIsLowQuality(seq) );
    NextSequence();
    REQUIRE( SequenceIsLowQuality(seq) );
}

// Alignment messages
FIXTURE_TEST_CASE(TestGetAlignment, ApiFixture)
{   
    GetAlignment();
    REQUIRE_NOT_NULL(align);
}

FIXTURE_TEST_CASE(TestGetAlignmentUnaligned, ApiFixture)
{   
    GetRecord();
    NextRecord();
    NextRecord();
    NextRecord();
    NextRecord();
    if (align)
        AlignmentRelease(align);
    REQUIRE_RC(RecordGetAlignment(rec, &align));
    REQUIRE_NULL(align);
}

FIXTURE_TEST_CASE(TestAlignmentGetRefSeqId, ApiFixture)
{   
    GetAlignment();
    int32_t id;
    REQUIRE_RC(AlignmentGetRefSeqId(align, &id));
    REQUIRE_EQ(id, 1);
}
FIXTURE_TEST_CASE(TestAlignmentGetMateRefSeqId, ApiFixture)
{   
    GetAlignment();
    int32_t id;
    REQUIRE_RC(AlignmentGetMateRefSeqId(align, &id));
    REQUIRE_EQ(id, 1);
}

FIXTURE_TEST_CASE(TestAlignmentGetPosition, ApiFixture)
{   
    GetAlignment();
    int64_t pos;
    REQUIRE_RC(AlignmentGetPosition(align, &pos));
    REQUIRE_EQ(pos, (int64_t)10); // 0 based internally vs 1-based in the source
}
FIXTURE_TEST_CASE(TestAlignmentGetMatePosition, ApiFixture)
{   
    GetAlignment();
    int64_t pos;
    REQUIRE_RC(AlignmentGetMatePosition(align, &pos));
    REQUIRE_EQ(pos, (int64_t)30); // 0 based internally vs 1-based in the source
}

FIXTURE_TEST_CASE(TestAlignmentGetMapQuality, ApiFixture)
{   
    GetAlignment();
    uint8_t qual;
    REQUIRE_RC(AlignmentGetMapQuality(align, &qual));
    REQUIRE_EQ(qual, (uint8_t)60); 
}

FIXTURE_TEST_CASE(TestAlignmentGetAlignmentDetail, ApiFixture)
{   
    GetAlignment();
    uint32_t actual; 
    // this call fails but reports back the number of elements to expect in the results structure
    REQUIRE_RC_FAIL(AlignmentGetAlignmentDetail(align, NULL, 0, &actual, NULL, NULL));
    REQUIRE_EQ(actual, (uint32_t)1); 
    
    AlignmentDetail *rslt = new AlignmentDetail[actual];
    int32_t firstMatch; 
    int32_t lastMatch;
    REQUIRE_RC(AlignmentGetAlignmentDetail(align, rslt, actual, NULL, &firstMatch, &lastMatch));
    REQUIRE_EQ(rslt->refSeq_pos, (int64_t)10);
    REQUIRE_EQ(rslt->read_pos, (int32_t)0);
    REQUIRE_EQ(rslt->length, (uint32_t)10);
    REQUIRE_EQ(rslt->type, (uint32_t)align_Match);
    REQUIRE_EQ(firstMatch, (int32_t)0);
    REQUIRE_EQ(lastMatch, (int32_t)0);
    
    delete [] rslt;
}

FIXTURE_TEST_CASE(TestAlignmentGetAlignOpCount, ApiFixture)
{   
    GetAlignment();
    uint32_t n; 
    REQUIRE_RC( AlignmentGetAlignOpCount ( align, &n ) );
    REQUIRE_EQ(n, (uint32_t)1); 
}

FIXTURE_TEST_CASE(TestAlignmentGetInsertSize, ApiFixture)
{   
    GetAlignment();
    int64_t size; 
    REQUIRE_RC( AlignmentGetInsertSize ( align, &size ) );
    REQUIRE_EQ(size, (int64_t)27); 
}

FIXTURE_TEST_CASE(TestAlignmentGetBAMCigar, ApiFixture)
{   
    GetAlignment();
    uint32_t const *cigar;
    uint32_t n; 
    REQUIRE_RC( AlignmentGetBAMCigar ( align, &cigar, &n ) );
    REQUIRE_EQ(n, (uint32_t)1); 
    // 10M
    REQUIRE_EQ(cigar[0], (uint32_t)( (10<<4) | 0 ) );
}

FIXTURE_TEST_CASE(TestAlignmentIsSecondary, ApiFixture)
{   
    GetAlignment();
    REQUIRE( ! AlignmentIsSecondary ( align ) );
    NextAlignment();
    NextAlignment();
    REQUIRE( AlignmentIsSecondary ( align ) );
}

// CG messages
FIXTURE_TEST_CASE(TestAlignmentGetCGData, ApiFixture)
{   
    GetCGData();
    const char *name = 0;
    size_t length = 0;
    REQUIRE_RC(SequenceGetSpotGroup(seq, &name, &length ));
    REQUIRE_EQ(string(name, length), string("GS"));
}

void AsciiToPhred(char* str)
{
    while (*str)
    {
        *str -= 33;
        ++str;
    }
}
void PhredToAscii(char* str)
{
    while (*str)
    {
        *str += 33;
        ++str;
    }
}

FIXTURE_TEST_CASE(TestCGDataGetSeqQual, ApiFixture)
{   
    GetCGData();
    char sequence[36]   ="AGATCCAGAGGTGGAAGAGGAAGCTTGGAACCC\0\0"; // 0-padded to 35
    char quality[36]    ="8::::877367778<<<<<1;6:49:99878/-\0\0";
    AsciiToPhred(quality);
    
    REQUIRE_RC( CGDataGetSeqQual ( cg, sequence, (uint8_t*)quality ) );
    REQUIRE_EQ(string(sequence),    string("AGATCTCCAGAGGTGGAAGAGGAAGCTTGGAACCC"));

    PhredToAscii(quality);
    REQUIRE_EQ(string(quality), string("8::::88877367778<<<<<1;6:49:99878/-"));
                                           //^^ inserted
}

FIXTURE_TEST_CASE(TestCGDataGetCigar, ApiFixture)
{   
    GetCGData();
    const uint32_t cig_max=1024;
    uint32_t cigar[cig_max];
    uint32_t cig_act;
    REQUIRE_RC( CGDataGetCigar( cg, cigar, cig_max, &cig_act ) );
    // 1I 4M 2B 2M 18M 4N 10M
    REQUIRE_EQ(cig_act, (uint32_t)7);
    REQUIRE_EQ(cigar[0], (uint32_t)( ( 1<<4) | 1 ) );
    REQUIRE_EQ(cigar[1], (uint32_t)( ( 4<<4) | 0 ) );
    REQUIRE_EQ(cigar[2], (uint32_t)( ( 2<<4) | 9 ) );
    REQUIRE_EQ(cigar[3], (uint32_t)( ( 2<<4) | 0 ) );
    REQUIRE_EQ(cigar[4], (uint32_t)( (18<<4) | 0 ) );
    REQUIRE_EQ(cigar[5], (uint32_t)( ( 4<<4) | 3 ) );
    REQUIRE_EQ(cigar[6], (uint32_t)( (10<<4) | 0 ) );
}

FIXTURE_TEST_CASE(TestCGDataGetAlignGroup, ApiFixture)
{   
    GetCGData();
    char buffer[1024];
    size_t max_size=sizeof(buffer);
    size_t act_size;
    REQUIRE_RC( CGDataGetAlignGroup( cg, buffer, max_size, &act_size ) );
    REQUIRE_EQ(string(buffer, act_size), string("0_1"));
}

// ReferenceInfo messages
FIXTURE_TEST_CASE(TestReferenceInfoGetRefSeqCount, ApiFixture)
{
    GetReferenceInfo();
    
    uint32_t count;
    REQUIRE_RC(ReferenceInfoGetRefSeqCount(ref, &count));
    REQUIRE_EQ(count, (uint32_t)2);
}
FIXTURE_TEST_CASE(TestReferenceInfoGetRefSeq, ApiFixture)
{
    GetReferenceInfo();
    
    ReferenceSequence refSeq;

    REQUIRE_RC(ReferenceInfoGetRefSeq(ref, 0, &refSeq));
    REQUIRE_EQ(refSeq.length, (uint64_t)500);
    REQUIRE_EQ(string(refSeq.name), string("dummy"));

    REQUIRE_RC(ReferenceInfoGetRefSeq(ref, 1, &refSeq));
    REQUIRE_EQ(refSeq.length, (uint64_t)50);
    REQUIRE_EQ(string(refSeq.name), string("ref"));
}
FIXTURE_TEST_CASE(TestReferenceInfoGetReadGroupCount, ApiFixture)
{
    GetReferenceInfo();
    uint32_t count;
    REQUIRE_RC(ReferenceInfoGetReadGroupCount(ref, &count));
    REQUIRE_EQ(count, (uint32_t)3);
}
FIXTURE_TEST_CASE(TestReferenceInfoGetReadGroup, ApiFixture)
{
    GetReferenceInfo();
    ReadGroup rg;

    // read groups are sorted by name 
    REQUIRE_RC(ReferenceInfoGetReadGroup(ref, 0, &rg));
    REQUIRE_NOT_NULL(rg.name);
    REQUIRE_EQ(string(rg.name), string("GS"));
    REQUIRE_NOT_NULL(rg.platform);
    REQUIRE_EQ(string(rg.platform), string("ILLUMINA"));

    REQUIRE_RC(ReferenceInfoGetReadGroup(ref, 1, &rg));
    REQUIRE_NOT_NULL(rg.name);
    REQUIRE_EQ(string(rg.name), string("SpotGroup"));
    REQUIRE_NOT_NULL(rg.platform);
    REQUIRE_EQ(string(rg.platform), string("LS454"));
    
    REQUIRE_RC(ReferenceInfoGetReadGroup(ref, 2, &rg));
    REQUIRE_NOT_NULL(rg.name);
    REQUIRE_EQ(string(rg.name), string("SpotGroupA"));
    REQUIRE_NOT_NULL(rg.platform);
    REQUIRE_EQ(string(rg.platform), string("CAPILLARY"));
}
FIXTURE_TEST_CASE(TestReferenceInfoGetReadGroupByName, ApiFixture)
{
    GetReferenceInfo();
    ReadGroup rg;

    // read groups are sorted by name 
    REQUIRE_RC(ReferenceInfoGetReadGroupByName(ref, "GS", &rg));
    REQUIRE_NOT_NULL(rg.name);
    REQUIRE_EQ(string(rg.name), string("GS"));
    REQUIRE_NOT_NULL(rg.platform);
    REQUIRE_EQ(string(rg.platform), string("ILLUMINA"));

    REQUIRE_RC(ReferenceInfoGetReadGroupByName(ref, "SpotGroup", &rg));
    REQUIRE_NOT_NULL(rg.name);
    REQUIRE_EQ(string(rg.name), string("SpotGroup"));
    REQUIRE_NOT_NULL(rg.platform);
    REQUIRE_EQ(string(rg.platform), string("LS454"));
    
    REQUIRE_RC(ReferenceInfoGetReadGroupByName(ref, "SpotGroupA", &rg));
    REQUIRE_NOT_NULL(rg.name);
    REQUIRE_EQ(string(rg.name), string("SpotGroupA"));
    REQUIRE_NOT_NULL(rg.platform);
    REQUIRE_EQ(string(rg.platform), string("CAPILLARY"));
}

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
const char UsageDefaultName[] = "test-bam";

rc_t CC KMain ( int argc, char *argv [] )
{
    ReportSilence();
    KConfigDisableUserSettings();
    rc_t rc=BamReaderTestSuite(argc, argv);
    return rc;
}

}
