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
* Unit tests for FASTQ loader
*/
#include <cstring>
#include <stdexcept> 
#include <ktst/unit_test.hpp> 
#include <klib/rc.h>
#include <loader/common-writer.h>

#include "fastq-parse.h"

using namespace std;

TEST_SUITE(FastqLoaderWbTestSuite);

//////////////////////////////////////////// tests for flex-generated scanner

#include "fastq-tokens.h"

#include <common/test_assert.h>

// test fixture for scanner tests
class FastqScanFixture
{
public:
    FastqScanFixture() 
    {
        pb.self = this;
        pb.input = Input;
        consumed = 0;
    }
    ~FastqScanFixture() 
    {
        FASTQScan_yylex_destroy(&pb);
        if (pb.record)
        {
            RecordRelease((const Record*)pb.record);
        }
    }
    void InitScan(const char* p_input, size_t length = 0, bool trace=false)
    {
        input = p_input;
        FASTQScan_yylex_init(&pb, trace);
    }
    int Scan()
    {
        int tokenId=FASTQ_lex(&sym, pb.scanner);
        if (tokenId != 0)
        {
            if (sym.tokenText != 0)
            {
                tokenText=string(sym.tokenText, sym.tokenLength);
            }
            else
            {
                tokenText.clear();
            }
        }
        return tokenId;
    }
    static size_t CC Input(FASTQParseBlock* sb, char* buf, size_t max_size)
    {
        FastqScanFixture* self = (FastqScanFixture*)sb->self;
        if (self->input.size() < self->consumed)
            return 0;

        size_t to_copy = min(self->input.size() - self->consumed, max_size);
        if (to_copy == 0)
            return 0;

        memcpy(buf, self->input.c_str(), to_copy);
        if (to_copy < max_size && buf[to_copy-1] != '\n')
        {
            buf[to_copy] = '\n';
            ++to_copy;
        }
        self->consumed += to_copy;
        return to_copy;
    }

    string input;
    size_t consumed;
    FASTQParseBlock pb;
    FASTQToken sym;
    string tokenText;
};

FIXTURE_TEST_CASE(EmptyInput, FastqScanFixture)
{   
    InitScan("");
    REQUIRE_EQUAL(Scan(), 0);
}
#define REQUIRE_TOKEN(tok)              REQUIRE_EQUAL(Scan(), (int)tok);
#define REQUIRE_TOKEN_TEXT(tok, text)   REQUIRE_TOKEN(tok); REQUIRE_EQ(tokenText, string(text));

#define REQUIRE_TOKEN_COORD(tok, text, line, col)  \
    REQUIRE_TOKEN_TEXT(tok, text); \
    REQUIRE_EQ(pb.lastToken->line_no, (size_t)line); \
    REQUIRE_EQ(pb.lastToken->column_no, (size_t)col);

FIXTURE_TEST_CASE(TagLine1, FastqScanFixture)
{   
    InitScan("@HWUSI-EAS499_1:1:3:9:1822\n", 0, true);
    REQUIRE_TOKEN('@');
    REQUIRE_TOKEN_COORD(fqALPHANUM, "HWUSI-EAS499_1", 1, 2);

    REQUIRE_TOKEN(':');
    REQUIRE_TOKEN_TEXT(fqNUMBER, "1"); 
    REQUIRE_TOKEN(':');
    REQUIRE_TOKEN_TEXT(fqNUMBER, "3"); 
    REQUIRE_TOKEN(':');
    REQUIRE_TOKEN_TEXT(fqNUMBER, "9");  
    REQUIRE_TOKEN(':');
    REQUIRE_TOKEN_TEXT(fqNUMBER, "1822"); 
    REQUIRE_TOKEN(fqENDLINE); 
    REQUIRE_TOKEN(0); 
}

FIXTURE_TEST_CASE(SequenceQuality, FastqScanFixture)
{   
    InitScan("@8\n" "GATC\n" "+8:1:46:673\n" "!**'\n", 0, true);
    REQUIRE_TOKEN('@');
    REQUIRE_TOKEN_TEXT(fqNUMBER, "8"); 
    REQUIRE_TOKEN(fqENDLINE); 
    REQUIRE_TOKEN_COORD(fqBASESEQ, "GATC", 2, 1); 
    REQUIRE_TOKEN(fqENDLINE); 
    REQUIRE_TOKEN('+'); 
    REQUIRE_TOKEN_TEXT(fqTOKEN,  "8:1:46:673"); 
    REQUIRE_TOKEN(fqENDLINE); 
    REQUIRE_TOKEN_TEXT(fqASCQUAL, "!**'"); 
    REQUIRE_TOKEN(fqENDLINE); 
}

FIXTURE_TEST_CASE(QualityOnly, FastqScanFixture)
{   
    InitScan(">8\n" "!**'\n", 0, true);
    REQUIRE_TOKEN('>'); 
    REQUIRE_TOKEN_TEXT(fqNUMBER, "8"); 
    REQUIRE_TOKEN(fqENDLINE); 
    REQUIRE_TOKEN_TEXT(fqASCQUAL, "!**'"); 
    REQUIRE_TOKEN(fqENDLINE); 
    REQUIRE_TOKEN(0); 
}

FIXTURE_TEST_CASE(NoEOL_InQuality, FastqScanFixture)
{   
    InitScan("@8\n" "GATC\n" "+\n" "!**'", 0, true);
    REQUIRE_TOKEN('@');
    REQUIRE_TOKEN_TEXT(fqNUMBER, "8"); 
    REQUIRE_TOKEN(fqENDLINE); 
    REQUIRE_TOKEN_TEXT(fqBASESEQ, "GATC"); 
    REQUIRE_TOKEN(fqENDLINE); 
    REQUIRE_TOKEN('+'); 
    REQUIRE_TOKEN(fqENDLINE); 

    REQUIRE_TOKEN_TEXT(fqASCQUAL, "!**'"); 
    REQUIRE_TOKEN(fqENDLINE); /* this is auto-inserted by FastqScanFixture::Input() */ 
    REQUIRE_TOKEN(0); 
}

FIXTURE_TEST_CASE(CRnoLF, FastqScanFixture)
{   
    InitScan("@8\r", 0, true);
    REQUIRE_TOKEN('@');
    REQUIRE_TOKEN_TEXT(fqNUMBER, "8"); 
    REQUIRE_TOKEN(fqENDLINE); 
}

FIXTURE_TEST_CASE(CommaSeparatedQuality3, FastqScanFixture)
{   
    InitScan("@8\n" "GATC\n" "+\n" "0047044004,046,,4000,04444000,--,6-\n", 0, true);
    REQUIRE_TOKEN('@');
    REQUIRE_TOKEN_TEXT(fqNUMBER, "8"); 
    REQUIRE_TOKEN(fqENDLINE); 
    REQUIRE_TOKEN_TEXT(fqBASESEQ, "GATC"); 
    REQUIRE_TOKEN(fqENDLINE); 
    REQUIRE_TOKEN('+'); 
    REQUIRE_TOKEN(fqENDLINE); 

    REQUIRE_TOKEN_TEXT(fqASCQUAL, "0047044004,046,,4000,04444000,--,6-"); 
    REQUIRE_EQUAL(Scan(), (int)fqENDLINE);
    REQUIRE_EQUAL(Scan(), 0);
}

FIXTURE_TEST_CASE(WsBeforeEol, FastqScanFixture)
{   
    InitScan("@ \n");
    REQUIRE_TOKEN('@');
    REQUIRE_TOKEN(fqENDLINE); 
    REQUIRE_EQUAL(Scan(), 0);
}
///////////////////////////////////////////////// FastqReader test fixture

#include "fastq-reader.h"

#include <kfs/directory.h>
#include <kfs/file.h>

class LoaderFixture
{
public:
    LoaderFixture() 
    :   wd(0), rf(0), 
        record(0), seq(0), reject(0), 
        read(0), readLength(0), 
        name(0), length(0), 
        errorText(0), errorLine(0), column(0), 
        quality(0), qualityOffset(0), qualityType(-1),
        phredOffset(33), defaultReadNumber(0)
    {
        if ( KDirectoryNativeDir ( & wd ) != 0 )
            FAIL("KDirectoryNativeDir failed");
    }
    ~LoaderFixture() 
    {
        delete [] read;

        if (seq != 0 && SequenceRelease(seq) != 0)
            FAIL("SequenceRelease failed");

        if (record != 0 && RecordRelease(record) != 0)
            FAIL("RecordRelease failed");

        if (reject != 0 && RejectedRelease(reject) != 0)
            FAIL("RejectedRelease failed");

        if ( rf != 0 && ReaderFileRelease( rf ) != 0)
            FAIL("ReaderFileRelease failed");

        if ( !filename.empty() && KDirectoryRemove(wd, true, filename.c_str()) != 0)
            FAIL("KDirectoryRemove failed");

        if ( KDirectoryRelease ( wd ) != 0 )
            FAIL("KDirectoryRelease failed");

        FASTQ_debug = 0;
    }
    rc_t CreateFile(const char* p_filename, const char* contents)
    {   // create and open for read
        KFile* file;
        filename=p_filename;
        rc_t rc=KDirectoryCreateFile(wd, &file, true, 0664, kcmInit, p_filename);
        if (rc == 0)
        {
            size_t num_writ=0;
            rc=KFileWrite(file, 0, contents, strlen(contents), &num_writ);
            if (rc == 0)
            {
                rc=KFileRelease(file);
            }
            else
            {
                KFileRelease(file);
            }
            file=0;
        }
        return FastqReaderFileMake(&rf, wd, p_filename, phredOffset, defaultReadNumber);
    }
    void CreateFileGetRecord(const char* fileName, const char* contents)
    {
        if (record != 0 && RecordRelease(record) != 0)
            throw logic_error("CreateFileGetRecord: RecordRelease failed");

        if (CreateFile(fileName, contents) != 0)
            throw logic_error("CreateFileGetRecord: CreateFile failed");

        if (ReaderFileGetRecord(rf, &record) != 0 || record == 0)
            throw logic_error("CreateFileGetRecord: ReaderFileGetRecord failed");
    }
    bool GetRecord()
    {
        if (rf == 0)
            return false;

        if (record != 0 && RecordRelease(record) != 0)
            return false;
        if (reject != 0 && RejectedRelease(reject) != 0)
            return false;
        reject = 0;
        if (seq != 0 && SequenceRelease(seq) != 0)
            return false;
        seq = 0;

        return ReaderFileGetRecord(rf, &record) == 0;
    }

    bool GetRejected()
    {
        if (reject != 0 && RejectedRelease(reject) != 0)
            throw logic_error("GetRejected: RejectedRelease failed");
        if (record == 0)
            throw logic_error("GetRejected: record == 0");
        if (RecordGetRejected(record, &reject) != 0)
            throw logic_error("GetRejected: RecordGetRejected failed");
        if (reject == 0)
            return false;
        if (RejectedGetError(reject, &errorText, &errorLine, &column, &fatal) != 0)
            throw logic_error("IsFatal: RejectedGetError failed");
        return true;
    }
    
    bool CreateFileGetSequence(const char* name, const char* contents)
    {
        if (CreateFile(name, contents) != 0)
            return false;

        if (!GetRecord() || record == 0)
            return false;

        if (GetRejected())
            return false;
            
        if (RecordGetSequence(record, &seq) != 0)
            return false;

        return seq != 0;
    }

    bool MakeReadBuffer()
    {
        if (SequenceGetReadLength(seq, &readLength) != 0 || readLength == 0)
            return false;

        delete [] read;
        read = new char[readLength];
        return true;
    }

    void BisonDebugOn()
    {
        FASTQ_debug = 1;
    }

    KDirectory* wd;
    string filename;
    const ReaderFile* rf;
    const Record* record;
    const Sequence* seq;
    const Rejected* reject;

    char* read;
    uint32_t readLength;

    const char* name;
    size_t length;

    const char* errorText;
    uint64_t errorLine;
    uint64_t column;
    bool fatal;
    const void* errorData;

    const int8_t* quality;
    uint8_t qualityOffset;
    int qualityType;

    uint8_t phredOffset;
    uint8_t defaultReadNumber;
};

///////////////////////////////////////////////// FASTQ test cases
FIXTURE_TEST_CASE(EmptyFile, LoaderFixture)
{
    REQUIRE_RC(CreateFile(GetName().c_str(), ""));
    REQUIRE_EQ( string(ReaderFileGetPathname(rf)), GetName() );  
    REQUIRE(GetRecord());  
    REQUIRE_NULL(record);
}

FIXTURE_TEST_CASE(EndLines, LoaderFixture)
{
    REQUIRE_RC(CreateFile(GetName().c_str(), "\n\n"));
    REQUIRE_EQ( string(ReaderFileGetPathname(rf)), GetName() );  
    REQUIRE(GetRecord());  
    REQUIRE_NULL(record);
}
 
//////////////////// syntax errors and recovery
FIXTURE_TEST_CASE(SyntaxError1, LoaderFixture)
{
    string input="qqq abcd";

    CreateFileGetRecord(GetName().c_str(), input.c_str());

    REQUIRE(GetRejected());
    REQUIRE(!fatal);
    REQUIRE_NOT_NULL(errorText);
    REQUIRE_EQ(string("syntax error"), string (errorText));
    REQUIRE_EQ(errorLine, (uint64_t)1); 
    REQUIRE_EQ(column, (uint64_t)4);
    
    const void* data;
    REQUIRE_RC(RejectedGetData(reject, &data, &length));
    REQUIRE_NOT_NULL(data);
    REQUIRE_EQ(input, string((const char*)data, length));
}

FIXTURE_TEST_CASE(SyntaxError2, LoaderFixture)
{
    #define input "qqq abcd"
    CreateFileGetRecord(GetName().c_str(), "@SEQ_ID1\n" "GATT\n" "+\n" "!''*\n" input );
    REQUIRE(GetRecord());

    REQUIRE(GetRejected());
    REQUIRE(!fatal);
    REQUIRE_NOT_NULL(errorText);
    REQUIRE_EQ(string("syntax error"), string (errorText));
    REQUIRE_EQ(errorLine, (uint64_t)5); 
    REQUIRE_EQ(column, (uint64_t)4);

    const void* data;
    REQUIRE_RC(RejectedGetData(reject, &data, &length));
    REQUIRE_EQ(string(input), string((const char*)data, length));

    
    #undef input
}

FIXTURE_TEST_CASE(RecoveryFromErrorAtTopLevel, LoaderFixture)
{
    CreateFileGetRecord(GetName().c_str(), "qqq abcd\n" "@SEQ_ID1\n" "GATT\n" "+\n" "!''*\n");
    REQUIRE(GetRejected());

    REQUIRE(GetRecord());    
    REQUIRE_NOT_NULL(record);

    REQUIRE_RC(RecordGetSequence(record, &seq));
    REQUIRE_NOT_NULL(seq);

    REQUIRE_RC(SequenceGetSpotName(seq, &name, &length));
    REQUIRE_EQ(string("SEQ_ID1"), string(name, length));
}

FIXTURE_TEST_CASE(RecoveryFromErrorInHeader, LoaderFixture)
{
    CreateFileGetRecord(GetName().c_str(), "@SEQ_ID1^\n" "GATT\n" "+\n" "!''*\n" "@SEQ_ID2\n" "GATT\n" "+\n" "!''*\n");
    REQUIRE(GetRejected());

    REQUIRE(GetRecord());    
    REQUIRE_NOT_NULL(record);

    REQUIRE_RC(RecordGetSequence(record, &seq));
    REQUIRE_NOT_NULL(seq);

    REQUIRE_RC(SequenceGetSpotName(seq, &name, &length));
    REQUIRE_EQ(string("SEQ_ID2"), string(name, length));
}

FIXTURE_TEST_CASE(RecoveryFromErrorInRead, LoaderFixture)
{
    CreateFileGetRecord(GetName().c_str(), "@SEQ_ID1\n" "G^ATT\n" "+\n" "!''*\n" "@SEQ_ID2\n" "GATT\n" "+\n" "!''*\n");
    REQUIRE(GetRejected());

    REQUIRE(GetRecord());    
    REQUIRE_NOT_NULL(record);

    REQUIRE_RC(RecordGetSequence(record, &seq));
    REQUIRE_NOT_NULL(seq);

    REQUIRE_RC(SequenceGetSpotName(seq, &name, &length));
    REQUIRE_EQ(string("SEQ_ID2"), string(name, length));
}

FIXTURE_TEST_CASE(RecoveryFromErrorInQuality, LoaderFixture)
{
    CreateFileGetRecord(GetName().c_str(), "@SEQ_ID1\n" "GATT\n" "+\n" "\n" "@SEQ_ID2\n" "GATT\n" "+\n" "!''*\n");
    REQUIRE(GetRejected());

    REQUIRE(GetRecord());    
    REQUIRE_NOT_NULL(record);

    REQUIRE_RC(RecordGetSequence(record, &seq));
    REQUIRE_NOT_NULL(seq);

    REQUIRE_RC(SequenceGetSpotName(seq, &name, &length));
    REQUIRE_EQ(string("SEQ_ID2"), string(name, length));
}

//////////////////// platform specification
TEST_CASE(PlatformValid)
{
    REQUIRE_EQ((uint8_t)SRA_PLATFORM_ILLUMINA, PlatformToId("ILLUMINA"));
}    

TEST_CASE(PlatformInvalid)
{
    REQUIRE_EQ((uint8_t)SRA_PLATFORM_UNDEFINED, PlatformToId("FOO"));
}    

//////////////////// tag line parsing
#define TEST_TAGLINE(line)\
    CreateFileGetRecord(GetName().c_str(), line "\n" "GATT\n" "+\n" "!''*\n");\
    REQUIRE(! GetRejected()); /* no error */ \
    REQUIRE(GetRecord()); /* parsing done */ \
    REQUIRE_NULL(record); /* input consumed */

FIXTURE_TEST_CASE(Tag1,   LoaderFixture)  { TEST_TAGLINE("@HWUSI-EAS499:1:3:9:1822"); }
FIXTURE_TEST_CASE(Tag2_1, LoaderFixture)  { TEST_TAGLINE("@HWUSI-EAS499:1:3:9:1822:.2"); }
FIXTURE_TEST_CASE(Tag2_2, LoaderFixture)  { TEST_TAGLINE("@HWUSI-EAS499:1:3:9:1822:1.2"); }
FIXTURE_TEST_CASE(Tag3,   LoaderFixture)  { TEST_TAGLINE("@BILLIEHOLIDAY_1_FC20F3DAAXX:8:2:342:540"); }
FIXTURE_TEST_CASE(Tag4,   LoaderFixture)  { TEST_TAGLINE("@BILLIEHOLIDAY_1_FC200TYAAXX_3_1_751_675"); }
FIXTURE_TEST_CASE(Tag5,   LoaderFixture)  { TEST_TAGLINE("@HWUSI-EAS499:1:3:9:1822#0/1"); }
FIXTURE_TEST_CASE(Tag6,   LoaderFixture)  { TEST_TAGLINE("@1:3:9:1822:33.44"); }
FIXTURE_TEST_CASE(Tag7,   LoaderFixture)  { TEST_TAGLINE("@1:3:9:1822#1/2"); }
FIXTURE_TEST_CASE(Tag8,   LoaderFixture)  { TEST_TAGLINE("@HWUSI-EAS499:1:3:9:1822#CAT/1"); }
FIXTURE_TEST_CASE(Tag9,   LoaderFixture)  { TEST_TAGLINE("@ERBRDQF01EGP9U"); }
FIXTURE_TEST_CASE(Tag10,  LoaderFixture)  { TEST_TAGLINE("@ID57_120908_30E4FAAXX:3:1:1772:953.1"); }
FIXTURE_TEST_CASE(Tag11,  LoaderFixture)  { TEST_TAGLINE(">ID57_120908_30E4FAAXX:3:1:1772:953"); }
FIXTURE_TEST_CASE(Tag12,  LoaderFixture)  { TEST_TAGLINE("@741:6:1:1204:10747"); }
FIXTURE_TEST_CASE(Tag13,  LoaderFixture)  { TEST_TAGLINE("@741:6:1:1204:10747/1"); }
FIXTURE_TEST_CASE(Tag14,  LoaderFixture)  { TEST_TAGLINE("@SNPSTER4_246_30GCDAAXX_PE:1:1:3:896/1 run=090102_SNPSTER4_0246_30GCDAAXX_PE"); }
FIXTURE_TEST_CASE(Tag15,  LoaderFixture)  { TEST_TAGLINE("@G15-D_3_1_903_603_0.81"); }

//////////////////// building Sequence objects
FIXTURE_TEST_CASE(NotRejected, LoaderFixture)
{
    CreateFileGetRecord(GetName().c_str(), "@HWUSI-EAS499:1:3:9:1822:1.2\n" "GATT\n" "+\n" "!''*\n");
    reject = (const Rejected*)1;
    REQUIRE_RC(RecordGetRejected(record, &reject));
    REQUIRE_NULL(reject);
}

FIXTURE_TEST_CASE(SequenceNoAlignment, LoaderFixture)
{
    CreateFileGetRecord(GetName().c_str(), "@HWUSI-EAS499:1:3:9:1822:1.2\n" "GATT\n" "+\n" "!''*\n");

    const Alignment* align;
    REQUIRE_RC(RecordGetAlignment(record, &align));
    REQUIRE_NULL(align);

    REQUIRE_RC(RecordGetSequence(record, &seq));
    REQUIRE_NOT_NULL(seq);
}

FIXTURE_TEST_CASE(TestSequenceGetReadLength, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@SEQ_ID1\n" "GATT\n" "+\n" "!''*\n" ));
    REQUIRE_RC(SequenceGetReadLength(seq, &readLength));
    REQUIRE_EQ(readLength, 4u);
}

FIXTURE_TEST_CASE(TestSequenceGetRead, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@SEQ_ID1\n" "GATT\n" "+\n" "!''*\n" ));
    REQUIRE(MakeReadBuffer());
    REQUIRE_RC(SequenceGetRead(seq, read));
    REQUIRE_EQ(string(read, readLength), string("GATT"));
}

FIXTURE_TEST_CASE(TestSequenceGetRead2, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@SEQ_ID1\n" "GATT\n" "+\n" "!''*\n" ));
    REQUIRE(MakeReadBuffer());

    // normal
    REQUIRE_RC(SequenceGetRead2(seq, read, 0, 2));
    REQUIRE_EQ(string(read, 2), string("GA"));

    // stop out of range
    REQUIRE_RC_FAIL(SequenceGetRead2(seq, read, 2, 6));

    // start out of range
    REQUIRE_RC_FAIL(SequenceGetRead2(seq, read, 20, 1));
}

FIXTURE_TEST_CASE(SequenceGetQuality33, LoaderFixture)
{
    phredOffset = 33;
    
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@SEQ_ID1\n" "GATT\n" "+\n" "!''*\n" ));
    
    REQUIRE_RC(SequenceGetQuality(seq, &quality, &qualityOffset, &qualityType));
    REQUIRE_NOT_NULL(quality);
    uint32_t l;
    REQUIRE_RC(SequenceGetReadLength(seq, &l));
    REQUIRE_EQ(qualityOffset, (uint8_t)phredOffset);
    REQUIRE_EQ(qualityType, (int)QT_Phred);
    REQUIRE_EQ(quality[0],  (int8_t)'!');
}

FIXTURE_TEST_CASE(SequenceGetQuality64, LoaderFixture)
{
    phredOffset = 64;
    
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@SEQ_ID1\n" "GATT\n" "+\n" "BBCC\n" ));
    
    REQUIRE_RC(SequenceGetQuality(seq, &quality, &qualityOffset, &qualityType));
    REQUIRE_NOT_NULL(quality);
    uint32_t l;
    REQUIRE_RC(SequenceGetReadLength(seq, &l));
    REQUIRE_EQ(qualityOffset, (uint8_t)phredOffset);
    REQUIRE_EQ(qualityType, (int)QT_Phred);
    REQUIRE_EQ(quality[0],  (int8_t)'B');
}

FIXTURE_TEST_CASE(SequenceBaseSpace, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@q\n" "GATC\n" "+\n" "!''*\n" ));
    REQUIRE(!SequenceIsColorSpace(seq));
    char k;
    REQUIRE_RC(SequenceGetCSKey(seq, &k)); // RC is 0 but k is undefined
}

FIXTURE_TEST_CASE(SequenceColorSpace, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@q\n" "G123\n" "+\n" "!''*\n" ));
    REQUIRE(SequenceIsColorSpace(seq));

    char k;
    REQUIRE_RC(SequenceGetCSKey(seq, &k));
    REQUIRE_EQ(k, 'G');
    
    uint32_t l;
    REQUIRE_RC(SequenceGetReadLength(seq, &l));
    REQUIRE_EQ(l, (uint32_t)0);
    REQUIRE_RC(SequenceGetCSReadLength(seq, &l));
    REQUIRE_EQ(l, (uint32_t)3);
    
    delete [] read;
    read = new char[l];
    REQUIRE_RC(SequenceGetCSRead(seq, read));
    REQUIRE_EQ(string(read, l), string("123"));
    
    REQUIRE_RC(SequenceGetCSQuality(seq, &quality, &qualityOffset, &qualityType));
    REQUIRE_NOT_NULL(quality);
    REQUIRE_EQ(qualityType, (int)QT_Phred);
    REQUIRE_EQ(qualityOffset,   (uint8_t)phredOffset);
    REQUIRE_EQ(quality[0],  (int8_t)'\'');
    REQUIRE_EQ(quality[1],  (int8_t)'\'');
    REQUIRE_EQ(quality[2],  (int8_t)'*' );
}

FIXTURE_TEST_CASE(SequenceGetOrientationIsReverse, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@q\n" "G123\n" "+\n" "!''*\n" ));
    REQUIRE_EQ(SequenceGetOrientationSelf(seq), (int)ReadOrientationUnknown);
    REQUIRE_EQ(SequenceGetOrientationMate(seq), (int)ReadOrientationUnknown);
}

//  Read Number
FIXTURE_TEST_CASE(ReadNumber, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@HWUSI-EAS499:1:3:9:1822#0/1\n" "GATT\n" "+\n" "!''*\n"));
    REQUIRE(SequenceIsFirst(seq));
    REQUIRE(!SequenceIsSecond(seq));
}
FIXTURE_TEST_CASE(ReadNumberMissing, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@HWUSI-EAS499:1:3:9:1822\n" "GATT\n" "+\n" "!''*\n"));
    REQUIRE(!SequenceIsFirst(seq));
    REQUIRE(!SequenceIsSecond(seq));
}
FIXTURE_TEST_CASE(ReadNumberDefault, LoaderFixture)
{
    defaultReadNumber = 2;
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@HWUSI-EAS499:1:3:9:1822\n" "GATT\n" "+\n" "!''*\n"));
    REQUIRE(!SequenceIsFirst(seq));
    REQUIRE(SequenceIsSecond(seq));
}
FIXTURE_TEST_CASE(ReadNumberOverride, LoaderFixture)
{
    defaultReadNumber = 1;
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@HWUSI-EAS499:1:3:9:1822/2\n" "GATT\n" "+\n" "!''*\n"));
    REQUIRE(!SequenceIsFirst(seq));
    REQUIRE(SequenceIsSecond(seq));
}

// Components of Illumina tag lines:
// @HWUSI-EAS499:1:3:9:1822#0/1"
// spot-name HWUSI-EAS499:1:3:9:1822    tag line up to and including coordinates
// spot-group "0"                       token following '#'
// read-number "1"                      1 or 2 following '/'

// not implemented for now:
// run-group HWUSI-EAS499               tag line up to and excluding coordinates
// coords [ 1 3 9 182 ]                  
// fmt-name HWUSI-EAS499:1:3:$X:$Y

FIXTURE_TEST_CASE(SequenceGetSpotNameIllumina, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@HWUSI-EAS499:1:3:9:1822#0/1\n" "GATT\n" "+\n" "!''*\n"));
    REQUIRE_RC(SequenceGetSpotName(seq, &name, &length));
    REQUIRE_EQ(string("HWUSI-EAS499:1:3:9:1822"), string(name, length));
    REQUIRE(!SequenceIsSecond(seq));
    REQUIRE(SequenceIsFirst(seq));
}

FIXTURE_TEST_CASE(SequenceGetSpotGroupIllumina, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "@1:3:9:1822#CAT/1\n" "GATT\n" "+\n" "!''*\n"));
    REQUIRE_RC(SequenceGetSpotGroup(seq, &name, &length));
    REQUIRE_EQ(string("CAT"), string(name, length));
    REQUIRE(!SequenceIsSecond(seq));
    REQUIRE(SequenceIsFirst(seq));
}

FIXTURE_TEST_CASE(SequenceGetSpotGroup_Empty, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), 
        ">HWI-EAS6_4_FC2010T:1:1:80:366\n" 
        "GATT\n" 
        "+\n" 
        "!''*\n"
    ));
    REQUIRE_RC(SequenceGetSpotGroup(seq, &name, &length));
    REQUIRE_EQ(string(""), string(name, length));
    REQUIRE(!SequenceIsSecond(seq));
    REQUIRE(!SequenceIsFirst(seq));
}

FIXTURE_TEST_CASE(SequenceGetSpotGroup_Zero, LoaderFixture)
{
    REQUIRE(CreateFileGetSequence(GetName().c_str(), 
        ">HWI-EAS6_4_FC2010T:1:1:80:366#0\n" 
        "GATT\n" 
        "+\n" 
        "!''*\n"
    ));
    REQUIRE_RC(SequenceGetSpotGroup(seq, &name, &length));
    REQUIRE_EQ(string(""), string(name, length));
    REQUIRE(!SequenceIsSecond(seq));
    REQUIRE(!SequenceIsFirst(seq));
}

#define TEST_PAIRED(line, paired)\
    REQUIRE(CreateFileGetSequence(GetName().c_str(), line "\n" "GATT\n" "+\n" "!''*\n"));\
    if (paired)\
        REQUIRE(SequenceWasPaired(seq));\
    else\
        REQUIRE(!SequenceWasPaired(seq));

FIXTURE_TEST_CASE(SequenceWasPaired1, LoaderFixture) { TEST_PAIRED("@HWUSI-EAS499:1:3:9:1822",         false); }
FIXTURE_TEST_CASE(SequenceWasPaired2, LoaderFixture) { TEST_PAIRED("@HWUSI-EAS499:1:3:9:1822#0/0",     false); }
FIXTURE_TEST_CASE(SequenceWasPaired3, LoaderFixture) { TEST_PAIRED("@HWUSI-EAS499:1:3:9:1822#0/1",     true); }
FIXTURE_TEST_CASE(SequenceWasPaired4, LoaderFixture) { TEST_PAIRED("@HWUSI-EAS499:1:3:9:1822#CAT/1",   true); }
FIXTURE_TEST_CASE(SequenceWasPaired5, LoaderFixture) { TEST_PAIRED("@HWUSI-EAS499:1:3:9:1822/2",       true); }

FIXTURE_TEST_CASE(SequenceGetSpotNameOneLine, LoaderFixture)
{   // source: /panfs/pan1.be-md.ncbi.nlm.nih.gov/trace_work/backup/qa/cases/load/illFastq/SRR014283/SRR014283_orig.fastq
    REQUIRE(CreateFileGetSequence(GetName().c_str(), "USI-EAS50_1:6:1:392:881:GCT:!!!")); 
    REQUIRE_RC(SequenceGetSpotName(seq, &name, &length));
    REQUIRE_EQ(string("USI-EAS50_1:6:1:392:881"), string(name, length));
}

FIXTURE_TEST_CASE(SequenceGetNameNumeric, LoaderFixture)
{   // source: /panfs/pan1.be-md.ncbi.nlm.nih.gov/trace_work/backup/qa/cases/load/illFastq/SRR094419/I3kT.741.6.920.fq
    phredOffset = 64;
    REQUIRE(CreateFileGetSequence(GetName().c_str(), 
        "@741:6:1:1204:10747/1\n"
        "GTCGTTGTCCCGCTCCTCATATTCNNNNNNNNNNNN\n"
        "+\n"
        "bbbbbbbbbbbbbbbbbbbb````BBBBBBBBBBBB\n"
    ));
    REQUIRE_RC(SequenceGetSpotName(seq, &name, &length));
    REQUIRE_EQ(string("741:6:1:1204:10747"), string(name, length));
}

//////////////////// parsing Quality lines

FIXTURE_TEST_CASE(GtStartsQualityOnly, LoaderFixture)
{   // source: /panfs/pan1.be-md.ncbi.nlm.nih.gov/trace_work/backup/qa/cases/load/illFastq/XXX001656/s_1_sequence.qual
    CreateFileGetRecord(GetName().c_str(), 
        ">HWI-EAS6_4_FC2010T:1:1:80:366\n"
        "!!\n"
        ">HWI-EAS6_4_FC2010T:1:1:80:366\n"
        "!!\n"
        );
    REQUIRE(GetRejected());
    REQUIRE(!fatal);
}

FIXTURE_TEST_CASE(OneLineRead, LoaderFixture)
{   
// source: /panfs/pan1.be-md.ncbi.nlm.nih.gov/trace_work/backup/qa/cases/load/illFastq/SRR016872/Billy_CG54K_PGP3_s_5_sequence_12_03_08.txt
    CreateFileGetRecord(GetName().c_str(), "USI-EAS50_1:6:1:392:881:GCTC:!!!!\n"); 
    REQUIRE(!GetRejected());
}

FIXTURE_TEST_CASE(ForcePhredOffset, LoaderFixture)
{   // quality line looks like it may be Phred64, but we know we are dealing with Phred33 
// source: /panfs/pan1.be-md.ncbi.nlm.nih.gov/trace_work/backup/qa/cases/load//illFastq/SRR014126/SRR014126_orig_2.fastq
    phredOffset = 33;
    CreateFileGetSequence(GetName().c_str(), 
        "@R16:8:1:19:1012#0/2\n"
        "TTAAATGACTCTTTAAAAAACACAACATACATTGATATATTTATTCCTAGATATTTGCTTATAAGACTCTAATCA\n"
        "+\n"
        "BCCBBBACBBCCCCBCCCCCCCCCCBCBCCCABBBBBBCCBBCBBCCBBCBCCCABBCAAABC@CCCAB@CBACC\n"
    );
    REQUIRE_RC(SequenceGetQuality(seq, &quality, &qualityOffset, &qualityType));
    REQUIRE_NOT_NULL(quality);
    REQUIRE_EQ(qualityType, (int)QT_Phred);
    REQUIRE_EQ(qualityOffset,  (uint8_t)33);
    REQUIRE_EQ(quality[0],    (int8_t)'B');
}

//////////////////// odd syntax cases
FIXTURE_TEST_CASE(NoEolAtEof, LoaderFixture)
{
    CreateFileGetRecord(GetName().c_str(), 
        "@SEQ_ID1\n" "GATT\n" "+\n" "!''*\n" 
        "@SEQ_ID2\n" "GATT\n" "+\n" "!''*"
    );
    REQUIRE_NOT_NULL(record);
    REQUIRE(! GetRejected());

    REQUIRE(GetRecord());    
    REQUIRE(! GetRejected());
    REQUIRE_NOT_NULL(record);
} 
 
FIXTURE_TEST_CASE(GtStartsReadOnly, LoaderFixture)
{   // source: /panfs/pan1.be-md.ncbi.nlm.nih.gov/trace_work/backup/qa/cases/load/illFastq/XXX001656/s_1_sequence.seq
    CreateFileGetRecord(GetName().c_str(), 
        ">q\n"
        "AAC\n"
        ">q\n"
        "ACA\n"
    );   
    REQUIRE(!GetRejected());
}

FIXTURE_TEST_CASE(IlluminaCasava_1_8, LoaderFixture)
{ // source: /panfs/traces01.be-md.ncbi.nlm.nih.gov/sra_backup/SRA/BACKUP/1225/905527/SAMN01860354.fastq
    REQUIRE(CreateFileGetSequence(GetName().c_str(), 
                "@HWI-ST273:315:C0LKAACXX:7:1101:1487:2221 2:Y:0:GGCTAC\n"
                "CAT\n"
                "+\n"
                "@@C\n"
    ));

    REQUIRE_RC(SequenceGetSpotName(seq, &name, &length));
    REQUIRE_EQ(string("HWI-ST273:315:C0LKAACXX:7:1101:1487:2221"), string(name, length));
    REQUIRE(SequenceIsSecond(seq));
    REQUIRE(SequenceIsLowQuality(seq));
    REQUIRE_RC(SequenceGetSpotGroup(seq, &name, &length));
    REQUIRE_EQ(string("GGCTAC"), string(name, length));
}

FIXTURE_TEST_CASE(IlluminaCasava_1_8_SpotGroupNumber, LoaderFixture)
{ // source: /panfs/traces01.be-md.ncbi.nlm.nih.gov/sra_backup/SRA/BACKUP/1225/905527/SAMN01860354.fastq
    REQUIRE(CreateFileGetSequence(GetName().c_str(), 
                "@HWI-ST273:315:C0LKAACXX:7:1101:1487:2221 2:Y:0:1\n"
                "CAT\n"
                "+\n"
                "@@C\n"
    ));

    REQUIRE_RC(SequenceGetSpotName(seq, &name, &length));
    REQUIRE_EQ(string("HWI-ST273:315:C0LKAACXX:7:1101:1487:2221"), string(name, length));
    REQUIRE(SequenceIsSecond(seq));
    REQUIRE(SequenceIsLowQuality(seq));
    REQUIRE_RC(SequenceGetSpotGroup(seq, &name, &length));
    REQUIRE_EQ(string("1"), string(name, length));
}

//////////////////// detecting older formats

FIXTURE_TEST_CASE(Quality33TooLow, LoaderFixture)
{   // negative qualities are not allowed for Phred33
// source: /panfs/pan1.be-md.ncbi.nlm.nih.gov/trace_work/backup/qa/cases/load/illFastq/SRR016872/SRR016872_orig.fastq
    phredOffset = 33;
    CreateFileGetRecord(GetName().c_str(), 
            "@HWI-EAS102_1_30LWPAAXX:5:1:1792:566\n"
            "GAAACCCCCTATTAGANNNNCNNNNCNATCATGTCA\n"
            "+HWI-EAS102_1_30LWPAAXX:5:1:1792:566\n"
            "II IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII\n"
    );
    REQUIRE(GetRejected());
    REQUIRE(fatal);
}

FIXTURE_TEST_CASE(Quality64TooLow, LoaderFixture)
{   // negative qualities are not allowed for Phred64
// source: /panfs/pan1.be-md.ncbi.nlm.nih.gov/trace_work/backup/qa/cases/load/illFastq/SRR016872/SRR016872_orig.fastq
    phredOffset = 64;
    CreateFileGetRecord(GetName().c_str(), 
            "@HWI-EAS102_1_30LWPAAXX:5:1:1511:102\n"
            "GGGGTTAGTGGCAGGGGGGGGGTCTCGGGGGGGGGG\n" 
            "+HWI-EAS102_1_30LWPAAXX:5:1:1511:102\n"
            "IIIIIIIIIIIIIIIIII;IIIIIIIIIIIIIIIII\n"
        );
    REQUIRE(GetRejected());
    REQUIRE(fatal);
}

FIXTURE_TEST_CASE(DecimalQualityRejected, LoaderFixture)
{
    CreateFileGetRecord(GetName().c_str(), 
        "@BILLIEHOLIDAY_1_FC20F3DAAXX:8:2:342:540\n"
        "GTCGCTTCTCGGAAGNGTGAAAGACAANAATNTTNN\n"
        "+BILLIEHOLIDAY_1_FC20F3DAAXX:8:2:342:540\n"
        "40 3 1 22 17 18 34 8 13 21 3 7 5 0 0 5 1 0 7 3 2 3 3 3 1 1 4 5 5 2 2 5 0 1 5 5\n"
    );
    REQUIRE(GetRejected());
    REQUIRE(fatal);
}

//TODO:

// FIXTURE_TEST_CASE(MissingRead, LoaderFixture)
// { // source: /panfs/pan1.be-md.ncbi.nlm.nih.gov/trace_work/backup/qa/cases/dump/newFastq/SRR529889/t1.fastq
    // REQUIRE(CreateFileGetSequence(GetName().c_str(), 
        // "@GG3IVWD03HIDOA length=3 xy=2962_2600 region=3 run=R_2010_05_11_11_15_22_\n"
        // "AAT\n"
        // "+\n"
        // "111\n"
    // ));
// }

// FIXTURE_TEST_CASE(Pacbio, LoaderFixture)
// { // source: /panfs/traces01.be-md.ncbi.nlm.nih.gov/sra_backup/SRA/BACKUP/1078/855879/salmon.filtered.PacBio.fastq -n100
    // REQUIRE(CreateFileGetSequence(GetName().c_str(), 
                // "@m120419_100821_42161_c100329130310000001523018509161273_s1_p0/11/2511_3149\n"
                // "CTGCTTCTCCTGCTCTTCCTACTGTCCTCTCCCTGCTGTCGCTTCGCCCC\n"
                // "TCGGTGGAGGCCGCGTTTGAGCGGCCGGTGTCCGCTGC\n"
                // "+\n"
                // "+*.,+*(,$+-.)**('#%*',*.*&,.(,$',,,($.,)#.-.-%'*#%\n"
                // ",../*+*!.%!!-!/-&(.+.!.'!!//,/)!//!*-'\n"
    // ));

    // REQUIRE_RC(SequenceGetReadLength(seq, &readLength));
    // REQUIRE_EQ(readLength, 88u);
    // REQUIRE(MakeReadBuffer());
    // REQUIRE_RC(SequenceGetRead(seq, read));
    // REQUIRE_EQ(string(read, readLength), string("CTGCTTCTCCTGCTCTTCCTACTGTCCTCTCCCTGCTGTCGCTTCGCCCCTCGGTGGAGGCCGCGTTTGAGCGGCCGGTGTCCGCTGC"));

    // REQUIRE_RC(SequenceGetQuality(seq, &quality, &qualityOffset, &qualityType));
    // REQUIRE_NOT_NULL(quality);
    // REQUIRE_EQ(qualityType, (int)QT_Phred);
    // REQUIRE_EQ((unsigned int)qualityOffset,  (unsigned int)phredOffset);
    // REQUIRE_EQ(quality[0],  (int8_t)'+');
    // REQUIRE_EQ(quality[87], (int8_t)'\'');
// }

// @ERBRDQF01EGP9U
// spot-name 
// spot-group 
// read-number 
// run-group 
// coords 
// fmt-name 

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

const char UsageDefaultName[] = "test-loader-fq";

rc_t CC KMain ( int argc, char *argv [] )
{
    KConfigDisableUserSettings();
    rc_t rc=FastqLoaderWbTestSuite(argc, argv);
    return rc;
}

}  
