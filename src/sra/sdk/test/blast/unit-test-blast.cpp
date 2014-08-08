/* This application expects LD_LIBRARY_PATH to contain the sra-path.so.

It uses some etalon run files located in /panfs/pan1/sra-test/BLAST.
They were created by:
cd /panfs/pan1/sra-test/BLAST
for RUN in SRR002749 ; do
    ./prepare_in.pl SRR002749 > SRR002749
done */

/******************************************************************************/

#include <ncbi/vdb-blast-priv.h> /* VdbBlastMgr */
#include <ncbi/vdb-blast.h> /* VdbBlastMgr */
#include <ktst/unit_test.hpp>
#include <kapp/main.h> /* KMain */
#include <vdb/manager.h> /* VDBManager */
#include <vdb/database.h> /* VDBManagerOpenDBRead */
#include <vdb/table.h> /* VTable */
#include <vdb/cursor.h> /* VCursor */
#include <vdb/blob.h> // VBlobRelease
#include <kproc/thread.h> // KThread
#include <kproc/cond.h> // KCondition
#include <kproc/lock.h> /* KLock */

#include <klib/report.h> // ReportSilence
#include <klib/refcount.h> /* KRefcount */
#include <klib/printf.h> /* string_printf */
#include <klib/log.h> // LOGERR
#include <klib/out.h> // KOutMsg
#include <klib/debug.h> /* DBGMSG */
#include <klib/rc.h> // RC

#include <bitstr.h> // bitcmp
#include <algorithm> // sort
#include <fstream> // ifstream
#include <sstream> // ostringstream
#include <memory> // auto_ptr
#include <map>
#include <vector>
#include <cstdarg> /* va_start */
#include <cstdlib> // malloc
#include <cstring> /* memset */

#if WINDOWS
#include <WinSock.h> // Sleep
#endif

#include <common/test_assert.h>

using std::auto_ptr;
using std::cerr;
using std::ifstream;
using std::istringstream;
using std::map;
using std::ostringstream;
using std::sort;
using std::string;
using std::vector;

static uint32_t min_read_length = 6;
//static uint32_t min_read_length = 31;

static rc_t argsHandler(int argc, char* argv[]);
TEST_SUITE_WITH_ARGS_HANDLER(BlastTestSuite, argsHandler);

#define WGS
#define ALL
#define ALLWIN
class Runs : public vector<string> {
public:
    static bool VERBOSE;
    static const bool VARIABLE_DATA_BUF_LEN;
    static const size_t BSIZE;
    static const unsigned VERBOSE_BUF;
    Runs(bool none = false, bool quick = false, bool along = false) {
        if (along)
        {   none = quick = false; }
        else if (quick)
        {   none = false; }
        if (none)
        { return; }
        push_back("SRR002749"); // SPOT_COUNT: 1 // TB
        push_back("SRR363367"); // SPOT_COUNT: 1
        push_back("SRR393572"); // SPOT_COUNT: 3
        push_back("SRR011165");
        push_back("SRR170433");
#ifdef WGS
        push_back("ABBW01");
#endif
        push_back("SRR391017");
        push_back("SRR360473");
        push_back("SRR391383");
#if WINDOWS
    //  never mind, used to go out of memory
#endif
        push_back("SRR329666");
#ifdef WGS
        push_back("AACL01");
#endif
    //  valgrind: kludge of death point if not returning here
        push_back("SRR331347");
        push_back("SRR393639");
        push_back("SRR327666"); // 28844 / 33294 reads
        push_back("SRR045020"); // 36969 / 43849 reads
        if (quick) { return; }
        push_back("SRR170418");
        push_back("SRR044422");
      if (along)
      { push_back("SRR451239"); }
    }
};
#define MAX_SAVED 2300 /* 2250 < x < 2500 */
#define MAX_SAVED_PRN (MAX_SAVED - 45)
const bool VERBOSE_BUF = false;
const unsigned Runs::VERBOSE_BUF = MAX_SAVED + 2;
bool Runs::VERBOSE = false;
const bool Runs::VARIABLE_DATA_BUF_LEN = true;
const size_t Runs::BSIZE = 490885;

class MgrFixture {
protected:
    VdbBlastMgr *mgr;
    MgrFixture(void) : mgr(0) {
        uint32_t status = eVdbBlastNoErr;
        mgr = VdbBlastInit(&status);
        if (mgr == NULL || status != eVdbBlastNoErr)
        {   FAIL("VdbBlastInit failed"); }

        VdbBlastMgr *m = VdbBlastMgrAddRef(mgr);
        if (m == NULL)
        {   FAIL("VdbBlastMgrAddRef failed"); }

        VdbBlastMgrRelease(m);
    }

   ~MgrFixture(void) { VdbBlastMgrRelease(mgr); }
};

class Data {
public:
    const int spotN;
    const string acc;
    const unsigned readN;
    const string read;
    Data(const string &a, int spotNo, int readNo, const string &s,
            bool allowEmpty = false)
        : spotN(spotNo), acc(a), readN(readNo), read(s)
    {
        if (!allowEmpty)
        {   assert(s.size()); }
    }
};

class Convert {
public:
    static bool _4na(string &read /* io parameter */, bool verbose = false);
    static bool _2na(string &bases, bool verbose = false) {
        if (bases.size() == 0 || bases[0] != '[')
        {   return true; }
        string _2na(bases);
        bases = "";
        for (unsigned i = 0; i < _2na.size();) {
            if (i + 5 >= _2na.size()) {
                FAIL("bad etalon: " + _2na);
                return false;
            }
            char hi = _2na[i + 1];
            if (_2na[i + 2] != ',' && _2na[i + 3] != ' ') {
                FAIL("bad etalon: " + _2na);
                return false;
            }
            char lo = _2na[i + 4];
            if (_2na[i + 5] != ']') {
                FAIL("bad etalon: " + _2na);
                return false;
            }
            if (i + 7 < _2na.size()) {
                if (_2na[i + 6] != ',' && _2na[i + 7] != ' ') {
                    FAIL("bad etalon: " + _2na);
                    return false;
                }
                i += 8;
            }
            else { i += 6; }
            int n = lo - '0';
            if (n > 1) {
                FAIL("bad etalon: " + _2na);
                return false;
            }
            hi -=  '0';
            if (hi > 1) {
                FAIL("bad etalon: " + _2na);
                return false;
            }
            n += hi * 2;
            const char base[] = "ACGT";
            bases += base[n];
        }
//      size_t s = bases.size();
        return true;
    }
    static void Print2na(char *buffer, const uint8_t *_2na, size_t sz,
        uint32_t offset_to_first_bit = 0)
    {
        if (_2na == NULL)
        {   return; }
        size_t i = 0;
        size_t o = 0;
        for (size_t ib = 0; ib < sz; ++ib) {
            if (i >= sz)
            {   break; }
            uint8_t u = _2na[ib];
            uint8_t a[4];
            a[0] = (u >> 6) & 3;
            a[1] = (u >> 4) & 3;
            a[2] = (u >> 2) & 3;
            a[3] = u & 3;
            for (int j = offset_to_first_bit / 2; j < 4; ++j) {
                if (i >= sz)
                {   break; }
                uint8_t h = a[j];
                assert(h < 4);
                const char c[] = "ACGT";
                if (buffer) {
                    buffer[o++] = c[h];
                }
                else { cerr << c[h]; }
                ++i;
            }
            offset_to_first_bit = 0;
        }
        if (buffer) {
            buffer[o++] = '\0';
        }
        else { cerr << "\n"; }
    }
};

class Elm {
    bool hit;
public:
    const Data *d;
    Elm(const string &a, int spotNo, int readNo, const string &s,
            bool allowEmpty = false)
        : hit(false), d(new Data(a, spotNo, readNo, s, allowEmpty))
    {}
    ~Elm(void) {
        delete d;
        d = NULL;
    }
    const Data *Release(void) {
        const Data *r = d;
        d = NULL;
        return r;
    }
    void Hit(void) {
        assert (!hit);
        hit = true;
    }
    bool IsHit(void) const
    {   return hit; }
    void Reset(void) { hit = false; }
    int ReadN(void) const { return d->readN; }
};
//typedef vector<Elm*> TInput;
typedef map<uint64_t, Elm*> TInputImpl;
class InputImpl : public TInputImpl {
    typedef TInputImpl::const_iterator TConstIterator;
public:
    void PushBack(const string &a, int spotNo, int readNo, const string &s,
            bool allowEmpty = false)
    {
        uint64_t read_id = readNo;
        TConstIterator it = find(read_id);
        assert(it == end());
        (*this)[read_id] = new Elm(a, spotNo, readNo, s, allowEmpty);
    }
};
typedef InputImpl TInput;

class Reader : private ncbi::NK::TestCase{
    static bool ReadSpot(const string &acc, ifstream &in, bool &eof,
        TInput &input, bool _2na, bool verbose = false,
        bool fini = false)
    {
        int SPOT_LEN = 0;
        ++spot;
        static int sz = 256;
        static char *line = NULL;
        if (line == NULL)
        {   line = static_cast<char*>(malloc(sz)); }
        if (line == NULL) {
            FAIL("out of memory");
            return false;
        }
        static char *_4na = NULL;
        if (fini) {
            free(line);
            free(_4na);
            _4na = line = NULL;
            return true;
        }
        if (_4na == NULL)
        {   _4na = static_cast<char*>(malloc(sz * 4)); }
        if (_4na == NULL) {
            FAIL("out of memory");
            return false;
        }
        char header[256];
        in.getline(header, sizeof header);
        istringstream iss2(header, istringstream::in);
        int TRIM_START = 0;
        int TRIM_LEN = 0;
        iss2 >> SPOT_LEN >> TRIM_START >> TRIM_LEN;

        in.getline(header, sizeof header);
        istringstream iss(header, istringstream::in);
        char filter[256];
        in.getline(filter, sizeof filter);
        if (strlen(filter) >= sizeof filter - 1) {
            FAIL("too long string");
            return false;
        }
        int spotLen = 0;
        if (sz < SPOT_LEN * 8) {
            sz = SPOT_LEN * 8;
            char *tmp = static_cast<char*>(realloc(line, sz));
            if (tmp == NULL) {
                FAIL("out of memory");
                return NULL;
            }
            line = tmp;
            tmp = static_cast<char*>(realloc(_4na, sz * 4));
            if (tmp == NULL) {
                FAIL("out of memory");
                return NULL;
            }
            _4na = tmp;
        }
        in.getline(line, sz);
        if (line[0] == '\0') {
            eof = true;
            return false;
        }
        int n = strlen(line);
        if (n == sz - 1) {
            FAIL("too long string");
            return false;
        }
        string s(line);
        if (n != spotLen) {
            if (n + 2 != SPOT_LEN * 8) {
                FAIL("bad read " + s);
                return false;
            }
            Convert::_2na(s);
        }
        in.getline(_4na, sz * 4);
        if (_4na[0] == '\0') {
            eof = true;
            return false;
        }
        n = strlen(_4na);
        if (n == sz * 4 - 1) {
            FAIL("too long string");
            return false;
        }
        string ss(_4na);
        if (n != spotLen) {
            if ((n + 2) < SPOT_LEN * 3 || (n + 2) > SPOT_LEN * 4) {
                FAIL("bad read " + ss);
                return false;
            }
            Convert::_4na(ss);
        }
        bool filtered = false;
        char* afilter = NULL;
        const char delimiters[] = ", ";
        char* pch = strtok(filter, delimiters);
        while (pch) {
            if (afilter == NULL) {
                afilter = pch;
            }
            else if (strcmp(afilter, pch)) {
                FAIL(string("bad filter ") + filter);
            }
            pch = strtok(NULL, delimiters);
        }
        string sfilter;
        if (afilter)
        {   sfilter = afilter; }
        int offset = 0;
        int bioLen = 0;
        while (true) {
            while (true) {
                if (iss.eof())
                {   return true; }
                if (sfilter != "SRA_READ_FILTER_PASS")
                {   filtered = true; }
                int i;
                char c;
                iss >> c >> i;
                if (c == 'T')
                {   offset += i; }
                spotLen += i;
                if (c == 'B') {
                    bioLen = i;
                    break;
                }
            }
            int start = offset;
            int len = bioLen;
            if (TRIM_LEN > 0) {
                if (TRIM_START > start) {
                    len -= TRIM_START - start;
                    start = TRIM_START;
                    if (len < 0)
                    {   len = 0; }
                }
                if (len > 0) {
                    if ((TRIM_START + TRIM_LEN) < (start + len)) {
                       len -= (start + len) - (TRIM_START + TRIM_LEN);
                        if (len < 0)
                        {   len = 0; }
                    }
                }
            }
            string r(s.substr(start, len));
            string r4(ss.substr(start, len));
            if (_2na) {
                if (r.size() >= min_read_length && !filtered) {
                    input.PushBack(acc, spot, read, r);
                    if (verbose)
                    {   cerr << read << "\n" << r << "\n\n"; }
                }
            }
            else {
                if (filtered || r4.size() < min_read_length)
                {   r4 = ""; }
                input.PushBack(acc, spot, read, r4, true);
            }
            ++read;
            offset += bioLen;
        }
        return false;
    }
    static int spot;
    static int read;

public:
    static void Fini(void) {
        ifstream dummy;
        bool db = true;
        TInput di;
        ReadSpot("", dummy, db, di, true, false, true); // memory cleanup
    }
    static void Reset(void) { spot = read = 0; }
    static bool Get(const string &acc, TInput &input, bool _2na,
        bool verbose = false)
    {
        string name
            ("/home/klymenka/cvs/internal/asm-trace/test/blast/" + acc);
#if WINDOWS
        name = "//panfs/pan1/sra-test/BLAST/" + acc;
#else
        name =  "/panfs/pan1/sra-test/BLAST/" + acc;
#endif
        ifstream in(name.c_str(), ifstream::in);
        if (in.fail()) {
            FAIL("cannot open " + name);
            return false;
        }
        bool eof = false;
        while (ReadSpot(acc, in, eof, input, _2na, verbose)) {}
        return eof;
    }
};

class Input {
    static Runs accs; 
    TInput input;
public:
    typedef vector <string>::const_iterator CIAcc;
    typedef TInput::iterator TIDat;
    typedef TInput::const_iterator CIDat;
    Input(bool _2na) {
        cerr << "Started " << (_2na ? "2" : "4") << "na creating "
            "("<< accs.size() << " runs) ... ";
        Reader::Reset();
        for (CIAcc i = BeginAcc(); i < EndAcc(); ++i) {
            string acc(*i);
            if (!Reader::Get(acc, input, _2na))
            {   return; }
        }
        cerr << SizeDat() << " reads\n";
    }
    ~Input(void) {
        for (TIDat i = input.begin(); i != input.end(); ++i) {
            delete i->second;
            i->second = NULL;
        }
        Reader::Fini();
    }
    void Reset(void) {
        for (CIDat i = BeginDat(); i != EndDat(); ++i)
        {   i->second->Reset(); }
    }
    CIAcc BeginAcc(void) const { return accs.begin(); }
    CIAcc EndAcc(void) const { return accs.end(); }
    CIDat BeginDat(void) const { return input.begin(); }
    CIDat EndDat(void) const { return input.end(); }
    size_t SizeDat(void) const { return input.size(); }
    const Data *Find(uint64_t read_id) {
        CIDat i = input.find(read_id);
        if (i == EndDat()) {
            assert(0);
            return NULL;
        }
        i->second->Hit();
        return i->second->d;
    }
    bool Check(void) const {
        for (CIDat i = BeginDat(); i != EndDat(); ++i) {
            assert(i->second);
            if (!i->second->IsHit())
            {   return false; }
        }
        return true;
    }
};

class RunSetFixture : protected MgrFixture {
protected:
    VdbBlastRunSet *set;
    RunSetFixture(void) : set(0) {
        uint32_t status = eVdbBlastNoErr;
        set = VdbBlastMgrMakeRunSet(mgr, &status, min_read_length, false);
        if (set == NULL || status != eVdbBlastNoErr)
        {   FAIL("VdbBlastMgrMakeRunSet failed"); }

        VdbBlastRunSet *s = VdbBlastRunSetAddRef(set);
        if (s == NULL)
        {   FAIL("VdbBlastRunSetAddRef failed"); }

        VdbBlastRunSetRelease(s);
    }
   ~RunSetFixture(void) { VdbBlastRunSetRelease(set); }
    uint32_t AddRun(const char *rundesc, ...) {
        uint32_t status = eVdbBlastNoErr;

        va_list args;
        va_start(args, rundesc);

        bool done = false;
        while (!done) {
            status = Add1Run(rundesc);
            if (status != eVdbBlastNoErr)
            {   break; }
            rundesc = va_arg(args, const char*);
            if (rundesc == NULL)
            {   break; }
            for (const char *p = rundesc; *p; ++p) {
                if (!isascii(*p)) {
                    done = true;
                    break;
                }
            }
        }

        va_end(args);

        return status;
    }

public:
    uint32_t Add1Run(const string &rundesc)
    { return VdbBlastRunSetAddRun(set, rundesc.c_str()); }
    uint32_t AddRuns(const Input &input) {
        uint32_t status = eVdbBlastNoErr;
        for (Input::CIAcc i = input.BeginAcc(); i < input.EndAcc(); ++i)
        {
            string acc(*i);
            status = Add1Run(acc);
            if (status != eVdbBlastNoErr)
            {   return status; }
        }
        return status;
    }
};

class RunSetSRR002749Fixture : protected RunSetFixture {
protected:
    RunSetSRR002749Fixture(void) {
        uint32_t status = eVdbBlastNoErr;
        status = AddRun("SRR002749", 0);
        if (status != eVdbBlastNoErr)
        {   FAIL("VdbBlastRunSetAddRun SRR002749 failed"); }
    }
};

class Test2NA : public ncbi::NK::TestCase {
public:
    static bool convert(string &bases, bool verbose = false)
    {   return Convert::_2na(bases, verbose); }

    static bool equals(uint64_t read_id, uint8_t *buffer, size_t buffer_size,
        Packed2naRead *read)
    {
        assert(buffer && read);
        if (read_id != read->read_id)
        {   return false; }
        if (buffer_size != read->length_in_bases)
        {   return false; }
        return ! bitcmp(buffer, 0,
            read->starting_byte, read->offset_to_first_bit, buffer_size * 2);
    }

    /* bases is an array of ACGT, buffer is 2na read */
    static bool equals(const string &bases, uint8_t *buffer, int num_read,
        int offset_to_first_bit = 0, bool verbose = false)
    {
        size_t sz = bases.size();
        if (sz == 0 && num_read == 0)
        {   return true; }
        if (static_cast<int>(sz) != num_read) {
            ostringstream out;
            out << "Wrong etalon length: " << sz % 4 << " extra bases "
                << bases;
            FAIL(out.str());
            return false;
        }
        if (!num_read)
        {   return true; }
        int iBuf = 0;
        while (offset_to_first_bit >= 8) {
            offset_to_first_bit -= 8;
            ++iBuf;
        }
        if (verbose)
        {   cerr << "num_read = " << num_read << "\n"; }
        bool failed = false;
        for (int i = 0; ; ++iBuf) {
            uint8_t m = 0xC0;
            for (int b = 0, sh = 6; b < 4;
                ++b, m >>= 2, sh -= 2)
            {
                if (offset_to_first_bit) {
                    offset_to_first_bit -= 2;
                    continue;
                }
                if (i >= num_read) {
                    DBGMSG(DBG_APP, DBG_COND_1, ("\n"));
                    if (verbose)
                    {   cerr << "\n" << i << " >= " << num_read << "\n"; }
                    return !failed;
                }
                uint8_t byte = (buffer[iBuf] & m) >> sh;
                const char base[] = "ACGT";
                if (!failed) {
                  if (bases[i] != base[byte]) {
                    DBGMSG(DBG_APP, DBG_COND_1,
                        ("\n [%d]%c != %c\n", i, bases[i], base[byte]));
                    if (verbose) {
                        cerr
                            << "\n" << bases[i] << " != " << base[byte] << "\n";
                        failed = true;
                    }
                    else
                    {   return false; }
                  }
                  else {
                    DBGMSG(DBG_APP, DBG_COND_1, ("%c", bases[i]));
                    if (verbose)
                    {   cerr << bases[i]; }
                  }
                }
                else {
                    DBGMSG(DBG_APP, DBG_COND_1, ("%c", bases[i]));
                    if (verbose)
                    {   cerr << bases[i]; }
                }
                ++i;
            }
        }
        if (verbose)
        {   cerr << "\n"; }
        return !failed;
    }
    
    Test2NA(const string &name, const VdbBlast2naReader *reader,
            size_t expected_base = 0, uint64_t expected_read = 0,
            string bases = "" /* copy bases parameter */)
        : TestCase(name)
    {
        if (!convert(bases))
        {   return; }
        size_t elements = bases.size();
        DBGMSG(DBG_APP, DBG_COND_1, ("%d ", elements));
        size_t buffer_size = elements / 4;
        if (elements % 4)
        {   ++buffer_size; }
        uint32_t status = eVdbBlastErr;
        uint64_t read_id = 1;
        size_t starting_base = 1;
#define S 99
        uint8_t buffer[S] = "1";
        assert(S > buffer_size);
        uint64_t num_read = _VdbBlast2naReaderRead(reader,
            &status, &read_id, &starting_base, buffer, buffer_size);
        CHECK_EQUAL(num_read, (uint64_t)elements);
        CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
        CHECK_EQUAL(read_id, expected_read);
        CHECK_EQUAL(starting_base, expected_base);
        if (buffer_size)
        {   CHECK(equals(bases, buffer, num_read)); }
    }
};

class Test2NAData : public ncbi::NK::TestCase {
    void test1(const VdbBlast2naReader *readerRead,
        Packed2naRead *read,
        size_t expected_base = 0, uint64_t expected_read = 0,
        string *bases = NULL, bool release = false)
    {
        size_t elements = 0;
        size_t buffer_size = 0;
        if (!release) {
            assert(read);
            if (bases) {
                if (!Test2NA::convert(*bases))
                {   return; }
            }
            elements = read->length_in_bases;
            DBGMSG(DBG_APP, DBG_COND_1, ("%d ", elements));
            buffer_size = elements / 4;
            if (elements % 4)
            {   ++buffer_size; }
        }
        uint32_t status = eVdbBlastErr;
        uint64_t read_id = 1;
        size_t starting_base = 1;
        static size_t SSS = 99;
        static uint8_t *buffer = static_cast<uint8_t*>(malloc(SSS));
        if (release) {
            free(buffer);
            buffer = NULL;
            return;
        }
        if (buffer == NULL) {
            FAIL("out of memory");
            return;
        }
        if (SSS <= buffer_size) {
            SSS = buffer_size * 2;
            uint8_t *tmp = static_cast<uint8_t*>(realloc(buffer, SSS));
            if (tmp == NULL) {
                FAIL("out of memory");
                return;
            }
            buffer = tmp;
        }
        uint64_t num_read = _VdbBlast2naReaderRead(readerRead,
            &status, &read_id, &starting_base, buffer, buffer_size);
        if (num_read != elements)
        {   CHECK_EQUAL(num_read, (uint64_t)elements); }
        CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
        if (bases) {
            CHECK_EQUAL(read_id, expected_read);
            CHECK_EQUAL(starting_base, expected_base);
            CHECK_EQUAL((int)read->read_id, (int)expected_read);
            CHECK_EQUAL((int)read->length_in_bases, (int)bases->size());
            if (buffer_size) {
                CHECK(Test2NA::equals(*bases, buffer, num_read));
                CHECK(Test2NA::equals(*bases, (uint8_t*)read->starting_byte,
                    read->length_in_bases, read->offset_to_first_bit));
            }
        }
        CHECK(Test2NA::equals(read_id, buffer, num_read, read));
    }

    Packed2naRead *GetData(VdbBlast2naReader *readerData,
        uint32_t &buffer_length, unsigned count)
    {
//      buffer_length = 23;
        Packed2naRead *reads
            = (Packed2naRead*)calloc(count, sizeof *reads);
        for (unsigned i = 0; i < count; ++i) {
            Packed2naRead *read = reads + i;
            read->read_id = read->offset_to_first_bit = 1;
        }
        uint32_t status = eVdbBlastErr;
        uint32_t n = VdbBlast2naReaderData(readerData, &status, reads, count);
        CHECK_LE((unsigned)n, count);
        CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
        buffer_length = n;
        return reads;
    }
    void Init(const VdbBlast2naReader *readerRead,
            VdbBlast2naReader *readerData,
            size_t expected_base = 0, uint64_t expected_read = 0,
            string bases = "" /* copy bases parameter */)
    {
        uint32_t buffer_length = 0;
        Packed2naRead* reads = GetData(readerData, buffer_length, 1);
        test1(readerRead, reads, expected_base, expected_read, &bases);
        free(reads);
    }
    void Init(const VdbBlast2naReader *readerRead,
            VdbBlast2naReader *readerData,
            size_t expected_base = 0, Data *d = NULL)
    {
        string tmp;
        if (d)
        {   tmp = d->read; }
        Init(readerRead, readerData, expected_base, d ? d->readN : 0, tmp);
    }
    string MkName(const Data *d) {
        assert(d);
        ostringstream s;
        s << d->acc << ".2na." << d->spotN;
        return s.str();
    }
public:
    Test2NAData(void) : TestCase("release TestCase")
    {   test1(NULL, NULL, 0, 0, NULL, true); }
    Test2NAData(const string &name,
            const VdbBlast2naReader *readerRead, Packed2naRead *read)
        : TestCase(name)
    {   test1(readerRead, read, 0, 0, NULL); }
    Test2NAData(const string &name, const VdbBlast2naReader *readerRead,
            VdbBlast2naReader *readerData,
            size_t expected_base = 0, Data *d = NULL)
        : TestCase(name)
    {   Init(readerRead, readerData, expected_base, d); }
    Test2NAData(const VdbBlast2naReader *readerRead,
            VdbBlast2naReader *readerData,
            TInput::const_iterator &it,
            TInput::const_iterator &end,
            size_t expected_base = 0, unsigned count = 1)
        : TestCase(MkName(it->second->d))
    {
        uint32_t buffer_length = 0;
        Packed2naRead* reads = GetData(readerData, buffer_length, count);
        if (buffer_length < count) {
            ostringstream out;
//          out << "Data(" << count << ") returned " << buffer_length;
//          TEST_MESSAGE(out.str());
            count = buffer_length;
        }
        for (unsigned i = 0; i < count; ++i) {
            const Data *d = (it++)->second->d;
            uint64_t expected_read = d->readN;
            string tmp(d->read);
            test1(readerRead, &reads[i], expected_base, expected_read, &tmp);
            if (it == end)
            {   break; }
        }
        free(reads);
    }
    Test2NAData(const string &name, const VdbBlast2naReader *readerRead,
            VdbBlast2naReader *readerData,
            size_t expected_base = 0, uint64_t expected_read = 0,
            string bases = "" /* copy bases parameter */)
        : TestCase(name)
    {   Init(readerRead, readerData, expected_base, expected_read, bases); }
};

class Reader2naFixture : protected RunSetSRR002749Fixture {
    void Init(void) {
        uint32_t status = eVdbBlastNoErr;
        reader = VdbBlastRunSetMake2naReader(set, &status, 0);
        if (reader == NULL || status != eVdbBlastNoErr)
        {   FAIL("VdbBlastRunSetMake2naReader failed"); }
    }
protected:
    VdbBlast2naReader *reader;
    Reader2naFixture(void) { Init(); }
    Reader2naFixture(const char *r) {
        if (Add1Run(r))
        {   FAIL(string("Add1Run ") + r); }
        Init();
    }
   ~Reader2naFixture(void) {  VdbBlast2naReaderRelease(reader); }
};

class Reader2na2Fixture : protected Reader2naFixture {
protected:
    Reader2na2Fixture() : Reader2naFixture("SRR363367") {}
};
class Test4NAData : public ncbi::NK::TestCase {
    static bool convertPacked(string &read /* io parameter */);
public:
    static bool cmp(const uint8_t *s1, const uint8_t *s2, size_t n);
    static bool convert(string &read /* io parameter */, bool verbose = false)
    {   return Convert::_4na(read, verbose); }
    Test4NAData(const string &name, const VdbBlast4naReader *rRead,
        const VdbBlast4naReader *rData, const string &read, int read_id);
};

int Reader::spot = 0;
int Reader::read = 0;
Runs Input::accs;
static Input INPUT2na(true);
static Input INPUT4na(false);
bool Convert::_4na(string &read /* io parameter */, bool verbose) {
    const string save(read);
    if (read.size() == 0 || read[0] < 16)
    {   return true; }
    std::istringstream in(save);
    read = "";
    int i;
    while (in >> i) {
        read += i;
        string comma;
        if (!(in >> comma))
        {   return true; }
        if (comma != ",")
        {   return false; }
    }
    return false;
}
bool Test4NAData::convertPacked(string &read /* io parameter */) {
    if (read.size() == 0 || read[0] != '[')
    {   return true; }
    string _4na(read);
    read = "";
    int sum = 0;
    string tmp;
    for (unsigned i = 0; i < _4na.size();) {
        if (_4na[i] != '[') {
            FAIL("bad etalon: " + _4na);
            return false;
        }
        tmp += _4na[i];
        for (int j = 0; j < 4; ++j) {
            char c = _4na[i + j * 3 + 1] - '0';
            if (c > 1 || c < 0) {
                FAIL("bad etalon: " + _4na);
                return false;
            }
            tmp += _4na[i + j * 3 + 1];
            if (j != 3) {
                tmp += _4na[i + j * 3 + 2];
                tmp += _4na[i + j * 3 + 3];
                if (_4na[i + j * 3 + 2] != ',' &&
                    _4na[i + j * 3 + 3] != ' ')
                {
                    FAIL("bad etalon: " + _4na);
                    return false;
                }
            }
            sum = sum * 2 + c;
        }
        read += sum;
        sum = 0;
        tmp += _4na[i + 11];
        if (_4na[i + 11] != ']') {
            FAIL("bad etalon: " + _4na);
            return false;
        }
        if (i + 12 < _4na.size()) {
            tmp += _4na[i + 12];
            tmp += _4na[i + 13];
            if (_4na[i + 12] != ',' && _4na[i + 13] != ' ') {
                FAIL("bad etalon: " + _4na);
                return false;
            }
            i += 14;
        }
        else { i += 12; }
    }
    return true;
}
bool Test4NAData::cmp(const uint8_t *s1, const uint8_t *s2, size_t n) {
    if (n == 0)
    {   return true; }
    return memcmp(s1, s2, n) == 0;
}

Test4NAData::Test4NAData(const string &name, const VdbBlast4naReader *rRead,
        const VdbBlast4naReader *rData,
        const string &read, int read_id)
    : TestCase(name)
{
    string tmp(read);
    CHECK(convert(tmp));
    uint8_t buffer[Runs::BSIZE];
    uint32_t status = eVdbBlastErr;
    size_t length = 0;
    const uint8_t *d
        = VdbBlast4naReaderData(rData, &status, read_id, &length);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    REQUIRE_GE(sizeof buffer, length);
    CHECK_EQUAL(tmp.size(), length);
    size_t n = VdbBlast4naReaderRead
        (rRead, &status, read_id++, 0, buffer, sizeof buffer);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK_EQUAL(n, length);
    CHECK_LE(n, sizeof buffer);
    status = eVdbBlastErr;
    CHECK(cmp((uint8_t*)tmp.c_str(), buffer, tmp.size()));
    CHECK(cmp(buffer, d, n));
}

class TData {
protected:
    const size_t buffer_size;
    size_t starting_base;
public:
    uint8_t *buffer;
    uint32_t status;
    uint64_t read_id;
    uint64_t num_read;
    const uint32_t offset_to_first_bit;
protected:
    TData(size_t sz = Runs::BSIZE, uint64_t num = ~0,
            uint32_t st = eVdbBlastErr,
            uint64_t id = ~0, uint32_t first = 0)
        : buffer_size(sz + 4), starting_base(1)
        , buffer(new uint8_t[buffer_size])
        , status(st), read_id(id), num_read(num), offset_to_first_bit(first)
    {   assert(buffer); }
public:
   ~TData(void) {
//     Print(0, true);
       delete[] buffer;
       buffer = NULL;
    }
    static bool Cmp(const TData *lhs, const TData *rhs)
    {   return lhs->read_id < rhs->read_id; }
    void Print(char end = '\0', bool reset = false) const {
        static char *b = NULL;
        static size_t bsz = 0;
        if (reset) {
            free(b);
            b = NULL;
        }
        flush(cerr);
        if (b == NULL) {
            bsz = num_read + 64;
            b = static_cast<char*>(malloc(bsz));
            if (b == NULL)
            {   return; }
        }
        else if (bsz < num_read + 64) {
            char *tmp = NULL;
            bsz = num_read + 64;
            tmp = static_cast<char*>(realloc(b, bsz));
            if (tmp == NULL)
            {   return; }
            b = tmp;
        }
        sprintf(b, "%lu\t", read_id);
        int last = strlen(b);
        unsigned i = 0;
        uint32_t offset = offset_to_first_bit;
        for (unsigned ib = 0; ib < num_read; ++ib) {
            if (i >= num_read)
            {   break; }
            uint8_t u = buffer[ib];
            uint8_t a[4];
            a[0] = (u >> 6) & 3;
            a[1] = (u >> 4) & 3;
            a[2] = (u >> 2) & 3;
            a[3] = u & 3;
            for (int j = offset / 2; j < 4; ++j) {
                if (i >= num_read)
                {   break; }
                uint8_t h = a[j];
                assert(h < 4);
                const char c[] = "ACGT";
                b[last++] = c[h];
                ++i;
            }
            offset = 0;
        }
        if (end)
        {   b[last++] = end; }
        b[last++] = '\n';
        b[last++] = '\0';
        cerr << b;
        flush(cerr);
    }
};

class TReadData : public TData {
public:
    TReadData(Packed2naRead &bf, uint32_t status)
        : TData(bf.length_in_bases, bf.length_in_bases, status,
            bf.read_id, bf.offset_to_first_bit)
    {
        if (bf.length_in_bases > 0) {
            size_t l = bf.length_in_bases / 4;
            if (bf.length_in_bases % 4 != 0)
            {   ++l; }
            if (bf.offset_to_first_bit > 0)
            {   ++l; }
            memcpy(buffer, bf.starting_byte, l);
        }
    }
};

class TRead2naRead : public TData {
public:
    TRead2naRead(VdbBlast2naReader *r) {
        num_read = _VdbBlast2naReaderRead(r,
            &status, &read_id, &starting_base, buffer, buffer_size);
    }
};
class TRead4naRead : public TData {
public:
    TRead4naRead(VdbBlast4naReader *r, int aRead_id) {
        starting_base = 0;
        read_id = aRead_id;
        num_read = VdbBlast4naReaderRead(r,
            &status, read_id, 0, buffer, buffer_size);
    }
};

class TestThread : ncbi::NK::TestCase {
    bool verbose;
    static uint64_t read_id;
    string MkName(int n) {
        ostringstream out;
        out << "THREAD-" << n;
        return out.str();
    }
    rc_t Read2na() {
        rc_t rc = 0;
        uint32_t status = eVdbBlastErr;
        VdbBlast2naReader *reader
            = VdbBlastRunSetMake2naReader(set1, &status, 0);
        if (status || !reader) {
            CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
            CHECK(reader);
            rc = -1;
            ostringstream out;
            out << "THREAD #" << thread_n << "\n";;
            cerr << out.str();
        }
        else {
            while (rc == 0) {
                TData *d = new TRead2naRead(reader);
                ostringstream out;
                if (d->num_read == 0) {
                    out << "THREAD #" << thread_n << " EOF\n";;
                    cerr << out.str();
                    delete d;
                    break;
                }
                if (Runs::VERBOSE) {
                    out << "THREAD #" << thread_n
                        << " " << d->read_id << " = " << d->num_read << "\n";
                    cerr << out.str();
                }
                rc = Save(d, verbose, thread_n);
                if (rc)
                {   CHECK_EQUAL((int)rc, 0); }
/*
                struct timespec tim;
                tim.tv_sec = 0;
                tim.tv_nsec = 50 * 1000 * 1000;
                int s = 0;
//              struct timespec tim2; s = nanosleep(&tim , &tim2); 
                if (s < 0)
                {   CHECK_EQUAL(s, 0); }
*/
            }
        }
        VdbBlast2naReaderRelease(reader);
        return rc;
    }
    rc_t Data2na(void) {
        rc_t rc = 0;
        uint32_t status = eVdbBlastErr;
        VdbBlast2naReader *reader
            = VdbBlastRunSetMake2naReader(set1, &status, 0);
        if (status || !reader) {
            CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
            CHECK(reader);
            rc = -1;
            ostringstream out;
            out << "THREAD #" << thread_n << "\n";;
            cerr << out.str();
        }
        else {
            while (true) {
                uint32_t status = eVdbBlastErr;
                Packed2naRead b[1];
                uint32_t n = VdbBlast2naReaderData(reader, &status,
                    b, sizeof b / sizeof b[0]);
                if (n > sizeof b / sizeof b[0] ||
                    status != eVdbBlastNoErr)
                {
                    ostringstream out;
//                  CHECK_GT((size_t)n, sizeof b / sizeof b[0]);
                    out << "CHECK_GT(n > sizeof b / sizeof b[0]) ("
                        << n << " > " << sizeof b / sizeof b[0] << ")\n";
                    cerr << out.str();
                    return -1;
                }
                if (n == 0) {
                    if (Runs::VERBOSE) {
                        ostringstream out;
                        out << "THREAD #" << thread_n << " EOF\n";
                        cerr << out.str();
                    }
                    break;
                }
                for (unsigned i = 0; i < n ; ++i) {
                    if (Runs::VERBOSE) {
                        ostringstream out;
                        out << "THREAD #" << thread_n
                            << " " << b[i].read_id << "\n";
                        cerr << out.str();
                    }
                    if (b[i].length_in_bases == 0)
                    {   CHECK_NE((int)b[i].length_in_bases, 0); }
                    TData *d = new TReadData(b[i], status);
                    rc = Save(d, verbose, thread_n);
                    if (rc)
                    {   CHECK_EQUAL((int)rc, 0); }
                }
            }
        }
        VdbBlast2naReaderRelease(reader);
        return rc;
    }
    rc_t _2na(const void *th) {
        assert(setRead);
        rc_t rc = 0;
        uint32_t status = eVdbBlastErr;
        VdbBlast2naReader *reader1
            = VdbBlastRunSetMake2naReader(set1, &status, 0);
        if (status || !reader1) {
            REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
            REQUIRE(reader1);
            rc = -1;
            ostringstream out;
            out << "THREAD #" << thread_n << "\n";;
            cerr << out.str();
        }
        VdbBlast2naReader *reader2
            = VdbBlastRunSetMake2naReader(setRead, &status, 0);
        if (status || !reader2) {
            REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
            REQUIRE(reader2);
            rc = -1;
            ostringstream out;
            out << "THREAD #" << thread_n << "\n";;
            cerr << out.str();
        }
        unsigned count = 0;
        size_t buffer_size = 0;
        uint8_t *buffer = NULL;
        while (true) {
            uint32_t status = eVdbBlastErr;
            Packed2naRead *b
                = static_cast<Packed2naRead*>(calloc(++count, sizeof *b));
            assert(b);
            uint32_t n = VdbBlast2naReaderData(reader1, &status, b, count);
            if (n > count || status != eVdbBlastNoErr) {
                ostringstream out;
//                  CHECK_GT((size_t)n, sizeof b / sizeof b[0]);
                out << "CHECK_GT(n > sizeof b / sizeof b[0]) ("
                    << n << " > " << count << ")\n";
                cerr << out.str();
                free(b);
                return -1;
            }
            if (n == 0) {
                if (Runs::VERBOSE) {
                    ostringstream out;
                    out << "THREAD #" << thread_n << " EOF\n";
                    cerr << out.str();
                }
                free(b);
                break;
            }
            for (unsigned i = 0; i < n ; ++i) {
                if (Runs::VERBOSE && th) {
                    ostringstream out;
                    out << "THREAD #" << thread_n
                        << " " << b[i].read_id << "\n";
                    cerr << out.str();
                }
                if (b[i].length_in_bases == 0)
                {   CHECK_NE((int)b[i].length_in_bases, 0); }
                if (buffer_size < b[i].length_in_bases) {
                    buffer_size = b[i].length_in_bases;
                    if (buffer) {
                        uint8_t *tmp =
                            static_cast<uint8_t*>(realloc(buffer, buffer_size));
                        assert(tmp);
                        buffer = tmp;
                    }
                    else {
                        buffer = static_cast<uint8_t*>(malloc(buffer_size));
                        assert(buffer != NULL);
                    }
                }
                Test2NAData a("thread_2na_DataVsRead", reader2, &b[i]);
                ErrorCounterAdd(a.GetErrorCounter());
                TData *d = new TReadData(b[i], status);
                rc = Save(d, verbose, thread_n);
                if (rc)
                {   CHECK_EQUAL((int)rc, 0); }
            }
            free(b);
            b = NULL;
        }
        free(buffer);
        buffer = NULL;
        VdbBlast2naReaderRelease(reader1);
        VdbBlast2naReaderRelease(reader2);
        return rc;
    }
    rc_t Read4na(void) {
        rc_t rc = 0;
        uint32_t status = eVdbBlastErr;
        VdbBlast4naReader *reader
            = VdbBlastRunSetMake4naReader(set1, &status);
        if (status || !reader) {
            CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
            CHECK(reader);
            rc = -1;
            ostringstream out;
            out << "THREAD #" << thread_n << "\n";;
            cerr << out.str();
        }
        else {
            for (uint64_t read_id = ReadId();
                read_id < nreads; read_id = ReadId())
            {
                TData *d = new TRead4naRead(reader, read_id);
                assert(d);
                rc = Save(d);
                if (rc)
                {   CHECK_EQUAL((int)rc, 0); }
            }
        }
        VdbBlast4naReaderRelease(reader);
        return rc;
    }
    static rc_t Save(TData *d, bool verbose = false, int thread_n = -1) {
        assert(d);
        if (false && verbose)
        { fprintf(stderr, "TestThread::Save : %d  KLockAcquire...\n", thread_n); }
        rc_t rc = KLockAcquire(mutex);
        if (verbose) {
            char b[256];
            rc_t rc = string_printf(b, sizeof b, NULL, "%d KLockAcquire(%p)\n",
                thread_n, mutex);
                assert(!rc);
            fprintf(stderr, "%s", b);
            fflush(stderr);
        }
        if (rc == 0) {
#if WINDOWS
         // Let the reader make it smaller. Or wait for out of memory exception.
            int dwMilliseconds = 0;
            while (saved.Size() > MAX_SAVED) {
                if (Runs::VERBOSE_BUF <= MAX_SAVED
                    && dwMilliseconds > 1)
                {
                    ostringstream s;
                    s << thread_n << "=" << saved.Size() << "\n";
                    cerr << s.str();
                }
                rc = KLockUnlock(mutex);
                if (rc == 0) {
                    rc = KConditionSignal(cond);

                    Sleep(dwMilliseconds++);
                    // otherwise it can take long time until the reader wakes up
                }
                if (rc == 0)
                {   rc = KLockAcquire(mutex); }
                if (rc != 0)
                {   return rc; }
            }
#endif
            if (verbose)
            {   d->Print('>'); }
            saved.PushBack(d);
#if WINDOWS
            if (saved.Size() >= Runs::VERBOSE_BUF) {
                ostringstream s;
                s << thread_n << " " << saved.Size() << "\n";
                cerr << s.str();
            }
#endif
            if (false) {
                char b[256];
                rc_t rc = string_printf(b, sizeof b, NULL,
                    "%d KLockUnlock(%p)\n", thread_n, mutex);
                assert(!rc);
                fprintf(stderr, "%s", b);
                fflush(stderr);
            }
            rc = KLockUnlock(mutex);
            if (rc == 0) {
//              cerr << thread_n << " KConditionSignal...\n";
                rc = KConditionSignal(cond);
//              cerr << "... " << thread_n << " KConditionSignal\n";
            }
        }
        return rc;
    }
    static uint64_t ReadId(void) {
        uint64_t id = ~0;
        rc_t rc = KLockAcquire(mutex);
        if (rc != 0) {
            assert(rc == 0);
        }
        else {
            id = read_id++;
            rc = KLockUnlock(mutex);
            assert(rc == 0);
        }
        return id;
    }
    typedef vector<const TData*> TVectorStorageImpl;
    typedef map<uint64_t, const TData*> TMapStorageImpl;
public:
    class Iterator {
    public:
        virtual ~Iterator(void) {}
        virtual const TData* Next(void) = 0;
    };
private:
    class VectorIterator;
    class VectorStorage : TVectorStorageImpl {
        friend class VectorIterator;
    public:
        typedef TVectorStorageImpl::iterator TIterator;
        typedef TVectorStorageImpl::const_iterator TConstIterator;
        TIterator Begin(void) { return begin(); }
        TIterator End(void) { return end(); }
    private:
        TConstIterator Begin(void) const { return begin(); }
        TConstIterator End(void) const { return end(); }
    public:
        void Clear(void) { clear(); }
        TVectorStorageImpl::size_type Size(void) const { return size(); }
        void PushBack(const TData *d) {
            assert(d);
            push_back(d);
        }
    };
    class VectorIterator : public Iterator {
        const VectorStorage& obj;
        VectorStorage::TConstIterator it;
    public:
        VectorIterator(const VectorStorage& aObj) : obj(aObj), it(obj.Begin())
        {}
        virtual const TData* Next(void) {
            if (it >= obj.End())
            {   return NULL; }
            const TData* d = *it;
            ++it;
            return d;
        }
    };
    class MapStorage : TMapStorageImpl {
    public:
        typedef TMapStorageImpl::iterator TIterator;
        typedef TMapStorageImpl::const_iterator TConstIterator;
        TConstIterator Begin(void) const { return begin(); }
        TConstIterator End(void) const { return end(); }
        TIterator Begin(void) { return begin(); }
        TIterator End(void) { return end(); }
        void Clear(void) { clear(); }
        bool Empty(void) const { return empty(); }
        TMapStorageImpl::size_type Size(void) const { return size(); }
        void PushBack(const TData *d) {
            assert(d);
            uint64_t read_id = d->read_id;
            if (Runs::VERBOSE) {
                ostringstream out;
                out << "PushBack(" << read_id << ")" << "\n";
                cerr << out.str();
            }
            TConstIterator it = find(read_id);
            assert(it == End());
            (*this)[read_id] = d;
        }
        const TData* Pop(void) {
            MapStorage::TIterator it(Begin());
            if (it == End())
            {   return NULL; }
            const TData* d = it->second;
            erase(it);
            return d;
        }
    };
    class MapIterator : public Iterator {
        const MapStorage& obj;
        MapStorage::TConstIterator it;
    public:
        MapIterator(const MapStorage& aObj) : obj(aObj), it(obj.Begin())
        {}
        virtual const TData* Next(void) {
            if (it == obj.End())
            {   return NULL; }
            const TData* d = it->second;
            ++it;
            return d;
        }
    };
    typedef MapStorage TStorage;
public:
    typedef MapIterator TIterator;
    static KLock *mutex;
public:
    static KCondition *cond;
    static TStorage saved;
    static bool done;
private:
    const int thread_n;
    const VdbBlastRunSet *set1;
    const VdbBlastRunSet *setRead;
    uint64_t nreads;
    static int verboseBuf;
public:
    TestThread(VdbBlastRunSet *aset1, int n, VdbBlastRunSet *set2,
            bool averbose = false, uint64_t reads = 0,
            int bufVerbose = MAX_SAVED + 9)
        : TestCase(MkName(n)), verbose(averbose), thread_n(n)
        , set1(aset1), setRead(set2), nreads(reads)
    {
        verboseBuf = bufVerbose;
        rc_t rc = Prepare();
        CHECK_EQUAL((int)rc, 0);
//      cerr << "Created thread " << thread_n << "\n";
    }
    static rc_t Prepare(void) {
        rc_t rc = 0;
        if (mutex == NULL)
        {   rc = KLockMake(&mutex); }
        if (!cond)
        {   rc = KConditionMake(&cond); }
        return rc;
    }
    static rc_t Fini(void) {
        done = false;
        read_id = 0;
        auto_ptr<Iterator> it(new TIterator(saved));
        while (const TData *d = it->Next()) {
            assert(d);
            delete d;
        }
        saved.Clear();

        rc_t rc = KLockRelease(mutex);
        mutex = NULL;

        {
            rc_t rc2 = KConditionRelease(cond);
            cond = NULL;
            rc = rc ? rc : rc2;
        }

        return rc;
    }
    static rc_t CC Funct2naRead(KThread const *const th,
        void *const vp)
    {
        rc_t rc = 0;
        TestThread *d = static_cast<TestThread*>(vp);
        assert(d);
        rc = d->Read2na();
//      cerr << "Exiting thread " << d->thread_n << "\n";
        return rc;
    }
    static rc_t CC Funct2naData(KThread const *const th,
        void *const vp)
    {
        TestThread *d = static_cast<TestThread*>(vp);
        assert(d);
        return d->Data2na();
    }
    static rc_t CC Funct2na(KThread const *const th,
        void *const vp)
    {
        TestThread *d = static_cast<TestThread*>(vp);
        assert(d);
        return d->_2na(th);
    }
    static rc_t CC Funct4naRead(KThread const *const th,
        void *const vp)
    {
        TestThread *d = static_cast<TestThread*>(vp);
        assert(d);
        rc_t rc = 0;
        try {
            rc = d->Read4na();
        } catch (std::bad_alloc&) {
            rc = RC(rcExe, rcBuffer, rcAllocating, rcMemory, rcExhausted);
            PLOGERR(klogFatal,
                (klogFatal, rc, "bad_alloc in $(f)", "f=TestThread::Funct4naRead"));
        }
        return rc;
    }
};

TestThread::TStorage TestThread::saved;
KLock *TestThread::mutex = NULL;
KCondition *TestThread::cond = NULL;
bool TestThread::done = false;
uint64_t TestThread::read_id = 0;
int TestThread::verboseBuf = MAX_SAVED + 9;

typedef bool TConvert(string &bases, bool verbose);

class Comparator : protected ncbi::NK::TestCase {
protected:
    TConvert* convert;
    const bool verbose;
    Comparator(const string &name, bool aVerbose, TConvert* aConvert)
        : TestCase(name), convert(aConvert), verbose(aVerbose)
    {}
public:
    virtual ~Comparator(void) {}
    virtual bool Cmp(const TData *tst, const Data *etl) = 0;
};
class Comparator2na : public Comparator {
public:
    Comparator2na(const string &name, bool verbose = false)
        : Comparator(name, verbose, Convert::_2na)
    {}
    virtual bool Cmp(const TData *tst, const Data *etl) {
        assert(tst && etl);
        string bases(etl->read);
        if (!convert(bases, verbose))
        {   FAIL("cannot convert Data"); }
        if (tst->status != eVdbBlastNoErr)
        {   CHECK_EQUAL((int)tst->status, (int)eVdbBlastNoErr); }
        if (verbose) { cerr << tst->read_id << "\n"; }
        if (verbose) { cerr << bases << "\n"; }
        if (etl->readN != tst->read_id)
        {   REQUIRE_EQUAL(etl->readN, (unsigned)tst->read_id); }
        unsigned idx = 0;
        uint32_t offset_to_first_bit = tst->offset_to_first_bit;
        for (unsigned ib = 0; ib < tst->num_read; ++ib) {
            if (idx >= tst->num_read)
            {   break; }
            uint8_t u = tst->buffer[ib];
            uint8_t a[4];
            a[0] = (u >> 6) & 3;
            a[1] = (u >> 4) & 3;
            a[2] = (u >> 2) & 3;
            a[3] = u & 3;
            for (int j = offset_to_first_bit / 2; j < 4; ++j) {
                if (idx >= tst->num_read)
                {   break; }
                uint8_t h = a[j];
//              cerr << "\n=" << (int)h << "\n";
                assert(h < 4);
                const char c[] = "ACGT";
                if (verbose) { cerr << c[h]; }
                ++idx;
            }
            offset_to_first_bit = 0;
        }
        if (verbose) { cerr << "\n"; }

        if (!Test2NA::equals(bases, tst->buffer,
            tst->num_read, tst->offset_to_first_bit, verbose))
        {
            Test2NA::equals(bases, tst->buffer, tst->num_read,
                tst->offset_to_first_bit, true);
            tst->Print();
            ostringstream out;
            out << "Test2NA::equals failed for " << tst->read_id;
            report_error(out.str().c_str(), __FILE__, __LINE__);
        }

        if (verbose) { cerr << "\n"; }
        return GetErrorCounter() == 0;
    }
};
class Comparator4na : public Comparator {
public:
    Comparator4na(const string &name, bool verbose = false)
        : Comparator(name, verbose, Convert::_4na)
    {}
    virtual bool Cmp(const TData *tst, const Data *etl) {
        string bases(etl->read);
        if (!convert(bases, verbose))
        {   FAIL("cannot convert Data"); }
        CHECK_EQUAL((int)tst->status, (int)eVdbBlastNoErr);
        if (verbose) { cerr << tst->read_id << "\n"; }
        if (verbose) { cerr << bases << "\n"; }
        REQUIRE_EQUAL(etl->readN, (unsigned)tst->read_id);

        CHECK(Test4NAData::cmp(tst->buffer,
            reinterpret_cast<const uint8_t*>(bases.c_str()), tst->num_read));
        CHECK_EQUAL((uint64_t)bases.size(), tst->num_read);

        if (verbose) { cerr << "\n"; }
        return GetErrorCounter() == 0;
    }
};

class CompareThread : ncbi::NK::TestCase {
    Input &input;
    Comparator *cmp;
    const int verboseBuf;
    bool Cmp(const TData *tst) const {
        if (tst == NULL)
        {   return true; }
        const Data *etl = input.Find(tst->read_id);
        assert(cmp);
        assert(etl);
        return cmp->Cmp(tst, etl);
    }
#define TODO -1
    rc_t Do(void) {
        do {
            rc_t rc = KLockAcquire(TestThread::mutex);
            if (rc != 0) {
                LOGERR(klogErr, rc, "cannot KLockAcquire");
                return rc;
            }
            while (!TestThread::done && TestThread::saved.Empty()) {
                rc = KConditionWait(TestThread::cond, TestThread::mutex);
                if (rc != 0) {
                    LOGERR(klogErr, rc, "cannot KConditionWait");
                    return rc;
                }
            }
            const TData *tst = TestThread::saved.Pop();
            rc = KLockUnlock(TestThread::mutex);
            if (rc != 0) {
                LOGERR(klogErr, rc, "cannot KLockUnlock");
                return rc;
            }
            bool r = Cmp(tst);
            delete tst;
#if WINDOWS
            if (TestThread::saved.Size() >= Runs::VERBOSE_BUF) {
                ostringstream s;
                s << "< " << TestThread::saved.Size() << "\n";
                cerr << s.str();
            }
#endif
            if (!r)
            {   return TODO; }
        } while (!TestThread::done);
        while (const TData *tst = TestThread::saved.Pop()) {
            bool r = Cmp(tst);
            delete tst;
            if (TestThread::saved.Size() >= Runs::VERBOSE_BUF && VERBOSE_BUF) {
                ostringstream s;
                s << "{ " << TestThread::saved.Size() << "\n";
                cerr << s.str();
            }
            if (!r)
            {   return TODO; }
        }
        if (Runs::VERBOSE_BUF <= MAX_SAVED) {
            ostringstream s;
            s << "( " << TestThread::saved.Size() << "\n";
            cerr << s.str();
        }
        if (TestThread::saved.Size() != 0) {
            CHECK_EQUAL((size_t)TestThread::saved.Size(), (size_t)0);
            return TODO;
        }
        return 0;
    }
protected:
    const bool verbose;
    CompareThread(const string &name, Input &aInput, bool _2na, bool aVerbose,
            Comparator *aCmp, int bufVerbose)
        : TestCase(name + "-thread")
        , input(aInput), cmp(aCmp), verboseBuf(bufVerbose), verbose(aVerbose)
    {   CHECK_EQUAL((int)TestThread::Prepare(), 0); }
    ~CompareThread(void) {
        delete cmp;
        cmp = NULL;
    }
public:
    static rc_t CC Run(KThread const *const th, void *const vp) {
        CompareThread *d = static_cast<CompareThread*>(vp);
        assert(d);
        return d->Do();
    }
};
class Compare2naThread : CompareThread {
public:
    Compare2naThread(const string &name, Input &input, bool verbose,
            int bufVerbose = MAX_SAVED + 9)
        : CompareThread(name, input, true, verbose,
            new Comparator2na(name + "-thread-2na-cmp", verbose),
            bufVerbose)
    {}
};
class Compare4naThread : CompareThread {
public:
    Compare4naThread(const string &name,
            Input &input, bool verbose, int verboseBuf = MAX_SAVED + 9)
        : CompareThread(name, input, true, verbose,
            new Comparator4na(name + "-thread-4na-cmp", verbose), verboseBuf)
    {}
};

#ifdef ALL
FIXTURE_TEST_CASE(VDB_BLAST_TEST_PROTEIN, MgrFixture) {
    uint32_t status = eVdbBlastNoErr;
    VdbBlastRunSet *set = VdbBlastMgrMakeRunSet(mgr, &status, 0, true);
    CHECK(status == eVdbBlastNoErr && set);
    CHECK(VdbBlastRunSetIsProtein(set));
    VdbBlastRunSetRelease(set);
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(VDB_BLAST_TEST, RunSetSRR002749Fixture) {
    uint32_t status = eVdbBlastNoErr;
    uint64_t numSequences = VdbBlastRunSetGetNumSequences(set, &status);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK_EQUAL((int)numSequences, 1);

    uint64_t numSequencesApprox = VdbBlastRunSetGetNumSequencesApprox(set);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK(numSequences == numSequencesApprox);

    uint64_t length = VdbBlastRunSetGetTotalLength(set, &status);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK_EQUAL((int)length, 257);

    CHECK_EQUAL(length, VdbBlastRunSetGetTotalLengthApprox(set));

    CHECK_EQUAL(length, VdbBlastRunSetGetMaxSeqLen(set));
    CHECK_EQUAL(length, VdbBlastRunSetGetMinSeqLen(set));
    CHECK_EQUAL(length, VdbBlastRunSetGetAvgSeqLen(set));

    char name_buffer[16];
    memset(name_buffer, '-', sizeof name_buffer);
    name_buffer[sizeof name_buffer - 1] = '\0';

    CHECK_EQUAL((int)VdbBlastRunSetGetName(set, &status, name_buffer, 1), 9);
    CHECK_EQUAL((int)status, (int)eVdbBlastErr);
    CHECK_EQUAL(string(name_buffer), string("S--------------"));

    CHECK_EQUAL((int)VdbBlastRunSetGetName
        (set, &status, name_buffer, sizeof name_buffer), 9);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK_EQUAL(string(name_buffer), string("SRR002749"));

    CHECK(!VdbBlastRunSetIsProtein(set));
}

FIXTURE_TEST_CASE(VDB_BLAST_TEST_NAME, RunSetSRR002749Fixture) {
    /* TECHNICAL, TYPE_BIOLOGICAL */
    char name_buffer[16];
    memset(name_buffer, '-', sizeof name_buffer);
    name_buffer[sizeof name_buffer - 1] = '\0';
    CHECK_EQUAL((int)VdbBlastRunSetGetReadName
        (set, 0, name_buffer, 1), 13);
    CHECK_EQUAL(string(name_buffer), string("S--------------"));

    CHECK_EQUAL((int)VdbBlastRunSetGetReadName
        (set, 0, name_buffer, sizeof name_buffer), 13);
    CHECK_EQUAL(string(name_buffer), string("SRR002749.1.2"));

    uint64_t read_id = ~0;
    uint32_t status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(read_id, (uint64_t)0);

    CHECK_EQUAL((int)VdbBlastRunSetGetReadName
        (set, 1, name_buffer, sizeof name_buffer), 0);
    CHECK_EQUAL(name_buffer[0], '\0');

    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(VDB_BLAST_TEST_2NA, Reader2naFixture) {
    VdbBlast2naReader *r2 = VdbBlast2naReaderAddRef(reader);
    CHECK(r2);

    VdbBlast2naReaderRelease(r2);
//  Test2NA a("2NA 0" , reader);

if (0) {
/*  Test2NA b("2NA all",reader,  0, "GGTAGCCTAGCG"
                                    "GAAGGGAAGATAATAATTAGATCGGAAAATCTGACAAAAC"
                                    "AAATGCCAAAACAAATAATAGTACACCTTAATCAATCTGT");
                                    */
    Test2NA b("2NA read-2", reader, 257);
    ErrorCounterAdd(b.GetErrorCounter());
    }
else {
                            //TCAG,GGTA
    Test2NA b("2NA 1" , reader,  0, 0, "GGTA");
    ErrorCounterAdd(b.GetErrorCounter());

                                   //GCCTAGCG
    Test2NA c("2NA 2" , reader,  4, 0, "GCCTAGCG");
    ErrorCounterAdd(c.GetErrorCounter());

//  Test2NA d("2NA 0 12", reader, 12);
                                   //GAAGGGAAGATAATAATTAGATCGGAAAATCTGACAAAAC
    Test2NA e("2NA 40", reader, 12, 0, "GAAGGGAAGATAATAATTAGATCGGAAAATCTGACAAAAC");
    ErrorCounterAdd(e.GetErrorCounter());

    Test2NA f("2NA 200",reader, 52, 0,
        //                                   10        20        30        40
        //                           1234567890123456789012345678901234567890
                                    "AAATGCCAAAACAAATAATAGTACACCTTAATCAATCTGT"
                                    "ACAAATTATGTGTACAAGACCTGGCAATAATACAAGAAGA"
                                    "AGTATAAGGATAGGACCAGGACAAACATTCATTGCAAGAG"
                                    "AAGACATAATAGGAGACATAAGACAAG"
/*
AAATGCCAAAACAAATAATAGTACACCTTAATCAATCTGTACAAATTATGTGTACAAGACCTGGCAATAATACAAGAAGAAGTATAAGGATAGGACCAGGACAAACATTCATTGCAAGAGAAGACATAATAGGAGACATAAGACAAG
*/
#ifdef NO_TRIM
                                    "AAGACATAATAGGAGACATAAGACAAGCCATATTGTAATA"
                                    "TTAGTGCAAGGCAATGGAATGCTGAGACACGCAACAGGGA"
#endif
                                    );
    ErrorCounterAdd(f.GetErrorCounter());
#ifdef NO_TRIM
    Test2NA g("2NA last", reader, 252, 0, "GTAG");
    ErrorCounterAdd(g.GetErrorCounter());
    Test2NA h("2NA part", reader, 256, 0, "G");
    ErrorCounterAdd(h.GetErrorCounter());
#endif

    uint32_t status = Add1Run("SRR363367");
    CHECK_EQUAL((int)status, (int)eVdbBlastErr);

//  Test2NA i("2NA eos", reader, 257);
}
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(TEST_BLOB, Reader2na2Fixture) {
    VdbBlastRunSet *setD = NULL;
    uint32_t status = eVdbBlastNoErr;
    setD = VdbBlastMgrMakeRunSet(mgr, &status, min_read_length, false);
    CHECK(setD != NULL);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    status = VdbBlastRunSetAddRun(setD, "SRR002749"); // TB
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    status = VdbBlastRunSetAddRun(setD, "SRR363367"); // TTB
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    VdbBlast2naReader *readerD = VdbBlastRunSetMake2naReader(setD, &status, 0);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK(readerD != NULL);
    Test2NAData a("2NA2 al0",reader, readerD, 0, 0, "GGTAGCCTAGCG"
                                    "GAAGGGAAGATAATAATTAGATCGGAAAATCTGACAAAAC"
                                    "AAATGCCAAAACAAATAATAGTACACCTTAATCAATCTGT"
                                    "ACAAATTATGTGTACAAGACCTGGCAATAATACAAGAAGA"
                                    "AGTATAAGGATAGGACCAGGACAAACATTCATTGCAAGAG"
                                    "AAGACATAATAGGAGACATAAGACAAG"
#ifdef NO_TRIM
                                    "AAGACATAATAGGAGACATAAGACAAGCCATATTGTAATA"
                                    "TTAGTGCAAGGCAATGGAATGCTGAGACACGCAACAGGGA"
                                    "GTAGG"
#endif
                                    );
    ErrorCounterAdd(a.GetErrorCounter());
    status = eVdbBlastNoErr;
    VdbBlast2naReader *reader = VdbBlastRunSetMake2naReader(set, &status, 0);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK(reader != NULL);
    Packed2naRead buffer;
    memset(&buffer, 0, sizeof buffer);
    buffer.read_id = buffer.offset_to_first_bit = 1;
    uint32_t n = VdbBlast2naReaderData(reader, &status, &buffer, 1);
    CHECK(n == 1);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK_EQUAL((int)buffer.read_id, 1);
//  CHECK_EQUAL((int)buffer.offset_to_first_bit, 0);
//  CHECK_EQUAL((int)buffer.length_in_bases, 270);

    CHECK_EQUAL(VdbBlastRunSetGetNumSequences(setD, &status), (uint64_t)2);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);

    uint64_t read_id = 0;
    char name_buffer[256] = "";
    size_t s = VdbBlastRunSetGetReadName(setD, read_id,
        name_buffer, sizeof name_buffer);
    string name("SRR002749.1.2");
    CHECK_EQUAL(s, name.size());
    CHECK_EQUAL(name, string(name_buffer));
    uint64_t read = ~0;
    status = VdbBlastRunSetGetReadId(setD,
        name_buffer, strlen(name_buffer), &read);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(read, read_id);

    ++read_id;
    name = "SRR363367.1.3";
    s = VdbBlastRunSetGetReadName(setD, read_id,
        name_buffer, sizeof name_buffer);
    CHECK_EQUAL(s, name.size());
    CHECK_EQUAL(name, string(name_buffer));
    status = VdbBlastRunSetGetReadId(setD,
        name_buffer, strlen(name_buffer), &read);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(read, read_id);

    VdbBlast2naReaderRelease(reader);
    VdbBlast2naReaderRelease(readerD);
    VdbBlastRunSetRelease(setD);
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(TEST_BLOB2, RunSetFixture) {
    VdbBlastRunSet *setD = NULL;
    uint32_t status = Add1Run("SRR363367");
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    VdbBlast2naReader *reader = VdbBlastRunSetMake2naReader(set, &status, 0);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK(reader != NULL);
    setD = VdbBlastMgrMakeRunSet(mgr, &status, min_read_length, false);
    CHECK(setD != NULL);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    status = VdbBlastRunSetAddRun(setD, "SRR363367");
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    VdbBlast2naReader *readerD = VdbBlastRunSetMake2naReader(setD, &status, 0);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK(readerD != NULL);
    Test2NAData a("2NA2 al1",reader, readerD, 0, 0,
        "[1, 0], [0, 0], [0, 1], [1, 0], [0, 1], [1, 0], [0, 1], [1, 0], [0, 0], [1, 1], [0, 0], [0, 0], [0, 1], [0, 0], [1, 0], [1, 0], [0, 0], [0, 0], [1, 1], [0, 1], [0, 0], [0, 1], [1, 0], [0, 1], [0, 1], [1, 1], [1, 1], [1, 1], [0, 1], [0, 0], [0, 0], [1, 0], [0, 0], [0, 1], [1, 1], [1, 1], [1, 0], [0, 1], [1, 0], [1, 0], [1, 1], [1, 0], [0, 1], [0, 0], [1, 0], [1, 1], [1, 0], [0, 1], [1, 1], [1, 0], [1, 0], [0, 1], [1, 0], [1, 1], [0, 1], [1, 1], [1, 0], [0, 1], [0, 0], [1, 1], [1, 0], [0, 0], [1, 1], [1, 0], [1, 0], [0, 0], [1, 0], [0, 1], [1, 0], [0, 0], [0, 0], [0, 1], [1, 1], [1, 1], [0, 0], [1, 0], [0, 1], [0, 0], [0, 1], [0, 0], [0, 0], [1, 1], [1, 0], [0, 1], [1, 1], [0, 1], [1, 0], [0, 1], [0, 0], [0, 0], [1, 1], [1, 1], [0, 1], [1, 1]"
#ifdef NO_TRIM
        ", [1, 0], [0, 0], [1, 1], [0, 1], [1, 0], [1, 0], [1, 0], [0, 0], [0, 1], [0, 0], [1, 0], [0, 1], [0, 1], [1, 1], [1, 1], [0, 1], [0, 1], [0, 0], [1, 1], [1, 0], [0, 0], [0, 1], [1, 0], [0, 0], [0, 1], [1, 1], [1, 1], [0, 0], [1, 1], [1, 0], [1, 1], [1, 1], [1, 1], [1, 1], [1, 0], [0, 1], [1, 0], [0, 1], [1, 1], [0, 0], [0, 1], [1, 0], [0, 1], [0, 1], [1, 0], [1, 1], [0, 0], [1, 0], [1, 1], [0, 0], [0, 1], [1, 0], [1, 1], [1, 1], [1, 1], [0, 1], [0, 0], [0, 1], [1, 0], [1, 1], [0, 1], [0, 1], [1, 0], [1, 1], [0, 0], [1, 0], [0, 0], [1, 0], [1, 0], [0, 0], [0, 1], [1, 0], [1, 1], [0, 0], [1, 1], [1, 0], [0, 1], [1, 0], [0, 0], [0, 1], [1, 0], [1, 1], [0, 1], [1, 1], [1, 1], [0, 1], [0, 1], [1, 0], [0, 0], [1, 0], [0, 1], [0, 0], [0, 1], [0, 1], [1, 0], [0, 1], [1, 1], [0, 0], [1, 1], [1, 1], [0, 0], [1, 1], [0, 1], [1, 0], [0, 1], [1, 0], [1, 1], [0, 1], [1, 0], [0, 1], [0, 0], [1, 0], [0, 0], [0, 0], [0, 1], [1, 1], [0, 0], [1, 0], [1, 1], [0, 1], [1, 0], [0, 0], [0, 1], [1, 0], [1, 1], [0, 0], [1, 1], [0, 1], [1, 0], [0, 0], [0, 1], [1, 0], [1, 1], [0, 0], [0, 1], [1, 1], [0, 0], [1, 0], [1, 1], [0, 0], [1, 0], [1, 1], [0, 0], [0, 0], [1, 0], [1, 1], [1, 1], [0, 1], [1, 0], [1, 0], [1, 0], [0, 1], [1, 0], [0, 0], [0, 1], [1, 1], [0, 0], [0, 1], [1, 1], [0, 1], [0, 1], [1, 0], [1, 0], [0, 0], [1, 0], [1, 1], [0, 1], [1, 0], [1, 1], [0, 0], [0, 0], [0, 1], [0, 1], [0, 1], [0, 1], [1, 0]"
#endif
        );
    ErrorCounterAdd(a.GetErrorCounter());
#if 0
    Test2NA x("2NA2 al2",reader, /*readerD,*/ 0, 0, // "TCAGAACCAGC"
                                    "GACGCGCGATAACAGGAATCACGCCTTTCAAGACTTGCGG"
                                    "TGCAGTGCTGGCGTCTGCATGATGGAGCGAACTTAGCACA"
                                    "ATGCTCGCAATTCTGATCGGGACAGCCTTCCATGACGACT"
                                    "TATGTTTTGCGCTACG"
                                 /* "TATGTTTTGCGCTACG"*/
                                  /*"TATGTTTTGCGCTACGCCGNAGTACGTTTCNCGTCCGTAG"
                                    "AGGACGTATGCGACGTCTTCCGNGCACCGCTATTATCGCG"
                                    "TCGCAGAACTAGTCGACGTATCGACGTACTAGTAGTAAGT"
                                    "TCGGGCGACTACTCCGGAGTCGTAACCCCG"*/);
    ErrorCounterAdd(x.GetErrorCounter());
    Test2NA b("2NA all",reader, /*readerD,*/ 136, 0, // "TCAGAACCAGC"
                                                    "CCGN"
                                  /*"TATGTTTTGCGCTACGCCGNAGTACGTTTCNCGTCCGTAG"
                                    "AGGACGTATGCGACGTCTTCCGNGCACCGCTATTATCGCG"
                                    "TCGCAGAACTAGTCGACGTATCGACGTACTAGTAGTAAGT"
                                    "TCGGGCGACTACTCCGGAGTCGTAACCCCG"*/);
    ErrorCounterAdd(b.GetErrorCounter());
#endif
    VdbBlast2naReaderRelease(reader);
    VdbBlast2naReaderRelease(readerD);
    VdbBlastRunSetRelease(setD);
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(VDB_BLAST_TEST_2NA2, Reader2na2Fixture) {
    Test2NA a("2NA2 al3",reader, 0, 0, "GGTAGCCTAGCG"
                                    "GAAGGGAAGATAATAATTAGATCGGAAAATCTGACAAAAC"
                                    "AAATGCCAAAACAAATAATAGTACACCTTAATCAATCTGT"
                                    "ACAAATTATGTGTACAAGACCTGGCAATAATACAAGAAGA"
                                    "AGTATAAGGATAGGACCAGGACAAACATTCATTGCAAGAG"
                                    "AAGACATAATAGGAGACATAAGACAAG"
#ifdef NO_TRIM
                                    "AAGACATAATAGGAGACATAAGACAAGCCATATTGTAATA"
                                    "TTAGTGCAAGGCAATGGAATGCTGAGACACGCAACAGGGA"
                                    "GTAGG"
#endif
                                    );
    ErrorCounterAdd(a.GetErrorCounter());
//  Test2NA b("2NA2 eor1", reader, 0, 1);
    Test2NA c("2NA2 r2"  , reader, 0, 1, "GACG");
    ErrorCounterAdd(c.GetErrorCounter());
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(READ_vs_DATA_2NA, RunSetFixture) {
    Input &input(INPUT2na);
    input.Reset();
    uint32_t status = eVdbBlastErr;
    VdbBlastRunSet *setD = VdbBlastMgrMakeRunSet(mgr, &status,
        min_read_length, false);
    CHECK(setD != NULL);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    Reader::Reset();
    REQUIRE_EQUAL((int)AddRuns(input), (int)eVdbBlastNoErr);
    for (Input::CIAcc i = input.BeginAcc(); i < input.EndAcc(); ++i) {
        string acc(*i);
        status = VdbBlastRunSetAddRun(setD, acc.c_str());
        CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    }
    VdbBlast2naReader *reader = VdbBlastRunSetMake2naReader(set, &status, 0);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK(reader != NULL);
    VdbBlast2naReader *readerD = VdbBlastRunSetMake2naReader(setD, &status, 0);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK(readerD != NULL);
//  int min_len = -1;
//  cerr << "min_len = " << min_len << "\n";
    Input::CIDat end(input.EndDat());
    unsigned cnt = 1;
    for (Input::CIDat i = input.BeginDat(); i != end; ) {
//      Data *d = *i; assert(d);
        Test2NAData a(reader, readerD, i, end, 0, cnt);
/*      int len = d->read.size();
//      cerr << "\tlen = " << len << "\n";
        if (min_len < 0) {
            min_len = len;
//          cerr << "min_len = " << min_len << "\n";
        }
        else if (len < min_len) {
            min_len = len;
//          cerr << "min_len = " << min_len << "\n";
        } */
        ErrorCounterAdd(a.GetErrorCounter());
        if (Runs::VARIABLE_DATA_BUF_LEN)
        {   ++cnt; }
    }
//  out << "min read len = " << min_len << "\n";
//  cerr << input.SizeDat() << " 2NA reads processed" << "\n";
    VdbBlast2naReaderRelease(reader);
    VdbBlast2naReaderRelease(readerD);
    VdbBlastRunSetRelease(setD);
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(STUB_TEST, RunSetSRR002749Fixture) {
    VdbBlast4naReader *r = VdbBlastRunSetMake4naReader(set, NULL);
    VdbBlast4naReader *r2 = VdbBlast4naReaderAddRef(r);
    VdbBlast4naReaderRelease(r2);
    VdbBlastRunSet *setD = NULL;
    uint32_t status = eVdbBlastNoErr;
    setD = VdbBlastMgrMakeRunSet(mgr, &status, min_read_length, false);
    CHECK(setD != NULL);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    status = VdbBlastRunSetAddRun(setD, "SRR002749");
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    VdbBlast4naReader *readerD = VdbBlastRunSetMake4naReader(setD, &status);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK(readerD != NULL);
    Test4NAData a("4na", r, readerD, "4, 4, 8, 1, 4, 2, 2, 8, 1, 4, 2, 4, 4, 1, 1, 4, 4, 4, 1, 1, 4, 1, 8, 1, 1, 8, 1, 1, 8, 8, 1, 4, 1, 8, 2, 4, 4, 1, 1, 1, 1, 8, 2, 8, 4, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 8, 4, 2, 2, 1, 1, 1, 1, 2, 1, 1, 1, 8, 1, 1, 8, 1, 4, 8, 1, 2, 1, 2, 2, 8, 8, 1, 1, 8, 2, 1, 1, 8, 2, 8, 4, 8, 1, 2, 1, 1, 1, 8, 8, 1, 8, 4, 8, 4, 8, 1, 2, 1, 1, 4, 1, 2, 2, 8, 4, 4, 2, 1, 1, 8, 1, 1, 8, 1, 2, 1, 1, 4, 1, 1, 4, 1, 1, 4, 8, 1, 8, 1, 1, 4, 4, 1, 8, 1, 4, 4, 1, 2, 2, 1, 4, 4, 1, 2, 1, 1, 1, 2, 1, 8, 8, 2, 1, 8, 8, 4, 2, 1, 1, 4, 1, 4, 1, 1, 4, 1, 2, 1, 8, 1, 1, 8, 1, 4, 4, 1, 4, 1, 2, 1, 8, 1, 1, 4, 1, 2, 1, 1, 4"
#ifdef NO_TRIM
        ", 2, 2, 1, 8, 1, 8, 8, 4, 8, 1, 1, 8, 1, 8, 8, 1, 4, 8, 4, 2, 1, 1, 4, 4, 2, 1, 1, 8, 4, 4, 1, 1, 8, 4, 2, 8, 4, 1, 4, 1, 2, 1, 2, 4, 2, 1, 1, 2, 1, 4, 4, 4, 1, 4, 8, 1, 4, 4"
#endif
        , 0);
    uint8_t buffer[199];
    status = eVdbBlastErr;
    size_t n = VdbBlast4naReaderRead(r, &status, 0, 0, buffer, sizeof buffer);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK_EQUAL(n, sizeof buffer);
    size_t length = 0;
    status = eVdbBlastErr;
    const uint8_t *d = VdbBlast4naReaderData(r, &status, 0, &length);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK_EQUAL(n, length);
    CHECK(Test4NAData::cmp(d, buffer, length));
    VdbBlast4naReaderRelease(r);
    VdbBlast4naReaderRelease(readerD);
    VdbBlastRunSetRelease(setD);
}
#endif

#ifdef ALLWIN
FIXTURE_TEST_CASE(READ_vs_DATA_4NA, RunSetFixture) {
    Input &input(INPUT4na);
    input.Reset();
    uint32_t status = eVdbBlastErr;
    VdbBlastRunSet *setD
        = VdbBlastMgrMakeRunSet(mgr, &status, min_read_length, false);
    CHECK(setD != NULL);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    Reader::Reset();
    REQUIRE_EQUAL((int)AddRuns(input), (int)eVdbBlastNoErr);
    for (Input::CIAcc i = input.BeginAcc(); i < input.EndAcc(); ++i) {
        string acc(*i);
        status = VdbBlastRunSetAddRun(setD, acc.c_str());
        CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    }
    VdbBlast4naReader *reader = NULL;//VdbBlastRunSetMake4naReader(set, &status)
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
//  CHECK(reader != NULL);
#if 0
    VdbBlast4naReader *readerD = VdbBlastRunSetMake4naReader(setD, &status);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK(readerD != NULL);
    int read_id = 0;
    for (Input::CIDat i = input.BeginDat(); i < input.EndDat(); ++i) {
        const Data *d = *i;
        assert(d);
        ostringstream s;
        s << d->acc << ".4na." << d->spotN;
        Test4NAData a(s.str(), reader, readerD, d->read, read_id++);
        ErrorCounterAdd(a.GetErrorCounter());
    }
//  cerr << input.SizeDat() << " 4NA reads processed" << "\n";
#endif
    VdbBlast4naReaderRelease(reader);
    VdbBlastRunSetRelease(setD);
}
#endif

////////////////////////////////////////////////////////////////////////////////
#ifdef ALL
TEST_CASE(MemoryLeaks) {
    uint32_t status = eVdbBlastNoErr;
    VdbBlastMgr *mgr = VdbBlastInit(&status);
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    REQUIRE(mgr);
    VdbBlastRunSet *set
        = VdbBlastMgrMakeRunSet(mgr, &status, min_read_length, false);
    REQUIRE(status == eVdbBlastNoErr && set);
    CHECK(! VdbBlastRunSetIsProtein(set));
    status = VdbBlastRunSetAddRun(set, "SRR002749");
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    VdbBlast4naReader *reader = VdbBlastRunSetMake4naReader(set, &status);;
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    VdbBlast4naReaderRelease(reader);
    VdbBlastRunSetRelease(set);
    VdbBlastMgrRelease(mgr);
}
#endif

typedef rc_t CC TThreadFunct(KThread const *const th, void *const vp);

#ifdef ALL
FIXTURE_TEST_CASE(Thread2naRead, RunSetFixture) {
    VdbBlastRunSet *set2 = NULL;
    uint64_t expectedReads = 0;
    bool noThreads = false;

    Input &input(INPUT2na);

    const string type("2na Read thread");
    TThreadFunct* callback = TestThread::Funct2naRead;

    bool verbose = false;
    bool DP = verbose;
    Reader::Reset();
    input.Reset();
    if (verbose) { cerr << "ETALON >>>\n"; }
    REQUIRE_EQUAL((int)AddRuns(input), (int)eVdbBlastNoErr);
    if (verbose) { cerr << "<<< ETALON\n"; }
    CHECK_EQUAL((int)TestThread::Fini(), 0);
    rc_t rc = 0;
    Compare2naThread comp("Thread2naRead", input, verbose);
    KThread *tcomp = NULL;
    if (!noThreads) {
        rc = KThreadMake(&tcomp, CompareThread::Run, &comp);
        CHECK_EQUAL((int)rc, 0);
    }
#define THR 8
    size_t threads = THR;
    KThread *t[THR];
    memset(t, 0, sizeof *t);
    TestThread *d[THR];
    memset(d, 0, sizeof *d);
    if (verbose) { cerr << "STARTING TO SAVE >>>\n"; }
    for (size_t i = 0; i < sizeof t / sizeof t[0]; ++i) {
        if (noThreads) {
            threads = 0;
            break;
        }
        d[i] = new TestThread(set, i, set2, verbose, expectedReads);
        rc_t rc = KThreadMake(&t[i], callback, d[i]);
        CHECK_EQUAL((int)rc, 0);
    }
    rc_t trc = 1;
    if (threads == 0) {
        d[0] = new TestThread(set, 0, set2, verbose, expectedReads);
        rc_t rc = callback(NULL, d[0]);
        CHECK_EQUAL((int)rc, 0);
        delete d[0];
        d[0] = 0;
    }
    else {
        for (size_t i = 0; i < sizeof t / sizeof t[0]; ++i) {
            if (t[i]) {
                if (verbose)
                {   cerr << "KThreadWait(" << i << ")\n"; }
                rc_t rc = KThreadWait(t[i], &trc);
                if (rc)
                {   CHECK_EQUAL((int)rc, 0); }
                if (trc)
                {   CHECK_EQUAL((int)trc, 0); }
                KThreadRelease(t[i]);
                t[i] = 0;
                delete d[i];
                d[i] = 0;
            }
        }
    }
    TestThread::done = true;
    rc = KConditionSignal(TestThread::cond);
    if (rc)
    {   CHECK_EQUAL((int)rc, 0); }
    if (verbose) { cerr << "<<< SAVED\n"; }
    cerr << threads << " " << type << "s created\n";
    if (noThreads) {
        rc = CompareThread::Run(NULL, &comp);
        CHECK_EQUAL((int)rc, 0);
    }
    if (DP) {
        cerr << "\nafter\n";
        auto_ptr<TestThread::Iterator> i(
            new TestThread::TIterator(TestThread::saved));
        while (const TData *d = i->Next()) {
            assert(d);
            cerr << d->read_id << " ";
        }
        cerr << "\netalon\n";
        for (Input::CIDat i = input.BeginDat();
            i != input.EndDat(); ++i)
        {   cerr << i->second->ReadN() << " "; }
        cerr << "\n";
    }
    if (tcomp) {
        rc = KThreadWait(tcomp, &trc);
        CHECK_EQUAL((int)rc, 0);
        CHECK_EQUAL((int)trc, 0);
        KThreadRelease(tcomp);
        tcomp = NULL;
    }
    CHECK_EQUAL((size_t)TestThread::saved.Size(), (size_t)0);
    if (verbose) { cerr << "<<< COMPARED\n"; }
    CHECK_EQUAL((int)TestThread::Fini(), 0);
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(Thread2naData, RunSetFixture) {
    VdbBlastRunSet *set2 = NULL;
    uint64_t expectedReads = 0;
    bool noThreads = false;

    Input &input(INPUT2na);

    const string type("2na Data thread");
    TThreadFunct* callback = TestThread::Funct2naData;

    bool verbose = false;
    bool DP = verbose;
    Reader::Reset();
    input.Reset();
    if (verbose) { cerr << "ETALON >>>\n"; }
    REQUIRE_EQUAL((int)AddRuns(input), (int)eVdbBlastNoErr);
    if (verbose) { cerr << "<<< ETALON\n"; }
    CHECK_EQUAL((int)TestThread::Fini(), 0);
    Compare2naThread comp("Thread2naData", input, verbose);
    KThread *tcomp = NULL;
    rc_t rc = KThreadMake(&tcomp, CompareThread::Run, &comp);
    CHECK_EQUAL((int)rc, 0);
#define THR 8
    size_t threads = THR;
    KThread *t[THR];
    memset(t, 0, sizeof *t);
    TestThread *d[THR];
    memset(d, 0, sizeof *d);
    if (verbose) { cerr << "STARTING TO SAVE >>>\n"; }
    for (size_t i = 0; i < sizeof t / sizeof t[0]; ++i) {
        if (noThreads) {
            threads = 0;
            break;
        }
        d[i] = new TestThread(set, i, set2, verbose, expectedReads);
        rc_t rc = KThreadMake(&t[i], callback, d[i]);
        CHECK_EQUAL((int)rc, 0);
    }
    rc_t trc = 1;
    if (threads == 0) {
        d[0] = new TestThread(set, 0, set2, verbose, expectedReads);
        rc_t rc = callback(NULL, d[0]);
        CHECK_EQUAL((int)rc, 0);
        delete d[0];
        d[0] = 0;
    }
    else {
        for (size_t i = 0; i < sizeof t / sizeof t[0]; ++i) {
            if (t[i]) {
                rc_t rc = KThreadWait(t[i], &trc);
                if (rc)
                {   CHECK_EQUAL((int)rc, 0); }
                if (trc)
                {   CHECK_EQUAL((int)trc, 0); }
                KThreadRelease(t[i]);
                t[i] = 0;
                delete d[i];
                d[i] = 0;
            }
        }
    }
    TestThread::done = true;
    rc = KConditionSignal(TestThread::cond);
    if (rc)
    {   CHECK_EQUAL((int)rc, 0); }
    if (verbose) { cerr << "<<< SAVED\n"; }
    cerr << threads << " " << type << "s created\n";
    if (DP) {
        cerr << "\nafter\n";
        auto_ptr<TestThread::Iterator> i(
            new TestThread::TIterator(TestThread::saved));
        while (const TData *d = i->Next()) {
            assert(d);
            cerr << d->read_id << " ";
        }
        cerr << "\netalon\n";
        for (Input::CIDat i = input.BeginDat(); i != input.EndDat(); ++i)
        {   cerr << i->second->ReadN() << " "; }
        cerr << "\n";
    }
    rc = KThreadWait(tcomp, &trc);
    CHECK_EQUAL((int)rc, 0);
    CHECK_EQUAL((int)trc, 0);
    KThreadRelease(tcomp);
    tcomp = NULL;
    CHECK_EQUAL((size_t)TestThread::saved.Size(), (size_t)0);
    if (verbose) { cerr << "<<< COMPARED\n"; }
    CHECK_EQUAL((int)TestThread::Fini(), 0);
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(Thread2na, RunSetFixture) {
    uint32_t status = eVdbBlastErr;
    VdbBlastRunSet *set2 = NULL;
    uint64_t expectedReads = 0;
    bool noThreads = false;

    Input &input(INPUT2na);

    set2 = VdbBlastMgrMakeRunSet(mgr, &status, min_read_length, false);
    REQUIRE(set2);
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    for (Input::CIAcc i = input.BeginAcc(); i < input.EndAcc(); ++i) {
        string acc(*i);
        status = VdbBlastRunSetAddRun(set2, acc.c_str());
        REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    }
    noThreads = true; /*
1) On each thread loop this test tries to call 2naData(n), then 2naRead n times;
then it compared Data[n] with n Read-s.
It will not work for more than 1 threads:
Read could be called by any thread, so 1) and then, compare will fail.
    */

    const string type("2na thread");
    TThreadFunct* callback = TestThread::Funct2na;

    bool verbose = false;
    bool DP = verbose;
    Reader::Reset();
    input.Reset();
    if (verbose) { cerr << "ETALON >>>\n"; }
    REQUIRE_EQUAL((int)AddRuns(input), (int)eVdbBlastNoErr);
    if (verbose) { cerr << "<<< ETALON\n"; }
    CHECK_EQUAL((int)TestThread::Fini(), 0);
    Compare2naThread comp("Thread2na", input, verbose);
    KThread *tcomp = NULL;
    rc_t rc = KThreadMake(&tcomp, CompareThread::Run, &comp);
    CHECK_EQUAL((int)rc, 0);
#define THR 8
    size_t threads = THR;
    KThread *t[THR];
    memset(t, 0, sizeof *t);
    TestThread *d[THR];
    memset(d, 0, sizeof *d);
    if (verbose) { cerr << "STARTING TO SAVE >>>\n"; }
    for (size_t i = 0; i < sizeof t / sizeof t[0]; ++i) {
        if (noThreads) {
            threads = 0;
            break;
        }
        d[i] = new TestThread(set, i, set2, verbose, expectedReads);
        rc_t rc = KThreadMake(&t[i], callback, d[i]);
        CHECK_EQUAL((int)rc, 0);
    }
    rc_t trc = 1;
    if (threads == 0) {
        d[0] = new TestThread(set, 0, set2, verbose, expectedReads);
        rc_t rc = callback(NULL, d[0]);
        CHECK_EQUAL((int)rc, 0);
        delete d[0];
        d[0] = 0;
    }
    else {
        for (size_t i = 0; i < sizeof t / sizeof t[0]; ++i) {
            if (t[i]) {
                rc_t rc = KThreadWait(t[i], &trc);
                if (rc)
                {   CHECK_EQUAL((int)rc, 0); }
                if (trc)
                {   CHECK_EQUAL((int)trc, 0); }
                KThreadRelease(t[i]);
                t[i] = 0;
                delete d[i];
                d[i] = 0;
            }
        }
    }
    TestThread::done = true;
    rc = KConditionSignal(TestThread::cond);
    if (rc)
    {   CHECK_EQUAL((int)rc, 0); }
    if (verbose) { cerr << "<<< SAVED\n"; }
    cerr << threads << " 2na threads created\n";
    if (DP) {
        cerr << "\nafter\n";
        auto_ptr<TestThread::Iterator> i(
            new TestThread::TIterator(TestThread::saved));
        while (const TData *d = i->Next()) {
            assert(d);
            cerr << d->read_id << " ";
        }
        cerr << "\netalon\n";
        for (Input::CIDat i = input.BeginDat(); i != input.EndDat(); ++i)
        {   cerr << i->second->ReadN() << " "; }
        cerr << "\n";
    }
    rc = KThreadWait(tcomp, &trc);
    CHECK_EQUAL((int)rc, 0);
    CHECK_EQUAL((int)trc, 0);
    KThreadRelease(tcomp);
    tcomp = NULL;
    CHECK_EQUAL((size_t)TestThread::saved.Size(), (size_t)0);
    if (verbose) { cerr << "<<< COMPARED\n"; }
    CHECK_EQUAL((int)TestThread::Fini(), 0);

    VdbBlastRunSetRelease(set2);
}
#endif

#ifdef ALLWIN /* THR */
FIXTURE_TEST_CASE(Thread4naData, RunSetFixture) {
    VdbBlastRunSet *set2 = NULL;
    uint64_t expectedReads = 0;
    bool noThreads = false;

    Input &input(INPUT4na);

    expectedReads = input.SizeDat();

    const string type("4na Read thread");
    TThreadFunct* callback = TestThread::Funct4naRead;

    bool verbose = false;
    bool DP = verbose;
    Reader::Reset();
    input.Reset();
    if (verbose) { cerr << "ETALON >>>\n"; }
    REQUIRE_EQUAL((int)AddRuns(input), (int)eVdbBlastNoErr);
    if (verbose) { cerr << "<<< ETALON\n"; }
    CHECK_EQUAL((int)TestThread::Fini(), 0);
    rc_t rc = 0;
    KThread *tcomp = NULL;
    Compare4naThread comp("Thread4naData", input, verbose);
    if (!noThreads) {
        rc = KThreadMake(&tcomp, CompareThread::Run, &comp);
        CHECK_EQUAL((int)rc, 0);
    }
#define THR 8
    size_t threads = THR;
    KThread *t[THR];
    memset(t, 0, sizeof *t);
    TestThread *d[THR];
    memset(d, 0, sizeof *d);
    if (verbose) { cerr << "STARTING TO SAVE >>>\n"; }
    for (size_t i = 0; i < sizeof t / sizeof t[0]; ++i) {
        if (noThreads) {
            threads = 0;
            break;
        }
        d[i] = new TestThread(set, i, set2, verbose, expectedReads);
        rc_t rc = KThreadMake(&t[i], callback, d[i]);
        CHECK_EQUAL((int)rc, 0);
    }
    rc_t trc = 1;
    if (threads == 0) {
        d[0] = new TestThread(set, 0, NULL, verbose, expectedReads);
        rc_t rc = callback(NULL, d[0]);
        CHECK_EQUAL((int)rc, 0);
        delete d[0];
        d[0] = 0;
    }
    else {
        for (size_t i = 0; i < sizeof t / sizeof t[0]; ++i) {
            if (t[i]) {
                rc_t rc = KThreadWait(t[i], &trc);
                if (rc)
                {   CHECK_EQUAL((int)rc, 0); }
                if (trc)
                {   CHECK_EQUAL((int)trc, 0); }
                KThreadRelease(t[i]);
                t[i] = 0;
                delete d[i];
                d[i] = 0;
            }
        }
    }
    TestThread::done = true;
    rc = KConditionSignal(TestThread::cond);
    if (rc)
    {   CHECK_EQUAL((int)rc, 0); }
    if (verbose) { cerr << "<<< SAVED\n"; }
    cerr << threads << " 4na Read threads created\n";
    if (noThreads) {
        rc = CompareThread::Run(NULL, &comp);
        CHECK_EQUAL((int)rc, 0);
    }
    if (DP) {
        cerr << "\nafter\n";
        auto_ptr<TestThread::Iterator> i(
            new TestThread::TIterator(TestThread::saved));
        while (const TData *d = i->Next()) {
            assert(d);
            cerr << d->read_id << " ";
        }
        cerr << "\netalon\n";
        for (Input::CIDat i = input.BeginDat(); i != input.EndDat(); ++i)
        {   cerr << i->second->ReadN() << " "; }
        cerr << "\n";
    }
    if (!noThreads) {
        rc = KThreadWait(tcomp, &trc);
        CHECK_EQUAL((int)rc, 0);
        CHECK_EQUAL((int)trc, 0);
        KThreadRelease(tcomp);
        tcomp = NULL;
    }
    CHECK_EQUAL((size_t)TestThread::saved.Size(), (size_t)0);
    if (verbose) { cerr << "<<< COMPARED\n"; }
    CHECK_EQUAL((int)TestThread::Fini(), 0);
}
#endif

#ifdef WGS
TEST_CASE(WGS0) {
    const VDBManager *mgr = NULL;
    rc_t rc = VDBManagerMakeRead(&mgr, NULL);
    REQUIRE_RC(rc);
    REQUIRE(mgr);
    const VDatabase *db = NULL;
#if WINDOWS
    const char path[] = "//traces04/wgs01/WGS/AA/GV/AAGV01";
#else
    const char path[] =  "/netmnt/traces04/wgs01/WGS/AA/GV/AAGV01";
#endif
    rc = VDBManagerOpenDBRead(mgr, &db, NULL, path);
    REQUIRE(db);
    const VTable *tbl = NULL;
    rc = VDatabaseOpenTableRead(db, &tbl, "SEQUENCE");
    REQUIRE_RC(rc);
    REQUIRE(tbl);
    VTableRelease(tbl);
    VDatabaseRelease(db);
    VDBManagerRelease(mgr);
}
#endif

#ifdef WGS
FIXTURE_TEST_CASE(WGS1, RunSetFixture) {
//  uint32_t status = Add1Run("AAGV01"); // empty read
    uint32_t status = Add1Run("AACL01"); // 1 read
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    VdbBlast2naReader *r = VdbBlastRunSetMake2naReader(set, &status, 0);
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    REQUIRE(r);
    uint8_t buffer[Runs::BSIZE];
    uint64_t read_id = ~0;
    size_t starting_base = ~0;
    uint64_t num_read = _VdbBlast2naReaderRead(r,
            &status, &read_id, &starting_base, buffer, sizeof buffer);
    REQUIRE_LT(num_read, (uint64_t)4 * sizeof buffer);
    REQUIRE_EQ((int)read_id, 0);
    REQUIRE_EQ((int)starting_base, 0);
    const size_t last = 100;
    char out[last * 4 + 1];
    Convert::Print2na(out, buffer, num_read > last ? last : num_read);
    REQUIRE_EQ(string(
"TCTCGCAGAGTTCTTTTTTGTATTAACAAACCCAAAACCCATAGAATTTAATGAACCCAAACCGCAATCGTACAAAAATTTGTAAAATTCTCTTTCTTCT"), string(out));
    size_t off = (num_read - last) / 4;
    Convert::Print2na(out, buffer + off, num_read - off * 4);
    REQUIRE_EQ(string(
"AAAACAAAAACGCTCAAAATAATAAATAACAAAAGCGATAGAGGAAAGTGTAAGAGATTTCAAAAATAGCTAAAAGGCGTTTTCAAAAATGTAAAACCTTA"), string(out));
    VdbBlast2naReaderRelease(r);
}
#endif

#ifdef WGS
FIXTURE_TEST_CASE(WGS_N, RunSetFixture) {
    uint32_t status = Add1Run("ABBW01"); // multiple reads
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    VdbBlast2naReader *r = VdbBlastRunSetMake2naReader(set, &status, 0);
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    REQUIRE(r);
    uint8_t buffer[Runs::BSIZE];
    uint64_t read_id = ~0;
    size_t starting_base = ~0;
    uint64_t num_read = _VdbBlast2naReaderRead(r,
            &status, &read_id, &starting_base, buffer, sizeof buffer);
    REQUIRE_LT(num_read, (uint64_t)4 * sizeof buffer);
    REQUIRE_EQ((int)read_id, 0);
    REQUIRE_EQ((int)starting_base, 0);
    const size_t last = 100;
    char out[last * 4 + 1];
    Convert::Print2na(out, buffer, num_read > last ? last : num_read);
    REQUIRE_EQ(string(
"ATCTATTGCTCCATTGTGATGCTTCGCAAGGTTGTGGGATGCTTGATATTAATGTTGGTAGATTGGGTGTTGATATGTTAACTTTGACGCAAATTTCTTA"), string(out));
    size_t off = (num_read - last) / 4;
    Convert::Print2na(out, buffer + off, num_read - off * 4);
    REQUIRE_EQ(string(
"GTGCACGCTCATATTTTCGTATAATCGTACCGTAGTGCGGATTAATGTGCATAATAAATGTATCTATAAAATTTCGGCGGCGAGAAGGGGAGCCATGTAAAA"), string(out));
    while (true) {
        uint64_t read_id1 = ~0;
        size_t starting_base1 = ~0;
        uint8_t buffer1[Runs::BSIZE];
        uint64_t num_read1 = _VdbBlast2naReaderRead(r,
            &status, &read_id1, &starting_base1, buffer1, sizeof buffer1);
        if (num_read1 == 0)
        {   break; }
        REQUIRE_LT(num_read1, (uint64_t)4 * sizeof buffer1);
        REQUIRE_EQ(read_id1, read_id + 1);
        REQUIRE_EQ((int)starting_base1, 0);
        read_id = read_id1;
        num_read = num_read1;
        memcpy(buffer, buffer1, num_read1 / 4 + 1);
    }
    REQUIRE_EQ((int)read_id, 84);
    REQUIRE_EQ((int)num_read, 1034);
    Convert::Print2na(out, buffer, num_read > last ? last : num_read);
    REQUIRE_EQ(string(
"GTGAAGAAGGGGTGTAATAATAGATAGAAGTATGACTATTGATTAGATAATATTGATCATAGAAGGGCATATCAATAATTCTGAAAAATGTAGGTCTCGT"), string(out));
    off = (num_read - last) / 4;
    Convert::Print2na(out, buffer + off, num_read - off * 4);
    REQUIRE_EQ(string(
"ATTCTATATTGTTATAGTCTTTAGCAATATTTAAATATTTATACTGAGTTTCGCTATTCTTAATTTGCGTATTTCTTCGAGCCTTTGCTGACTTCAAAATTA"), string(out));
    VdbBlast2naReaderRelease(r);
}
#endif

/*TEST_CASE(READ_TYPETest) {
    const VDBManager *mgr = NULL;
    rc_t rc = VDBManagerMakeRead(&mgr, NULL);
    REQUIRE_RC(rc);
    REQUIRE(mgr);
    const VTable *tbl = NULL;
#if WINDOWS
    const char path[] = "//panfs/traces01/sra4/SRR/000002/SRR002749";
#else
    const char path[] =  "/panfs/traces01/sra4/SRR/000002/SRR002749";
#endif
    rc = VDBManagerOpenTableRead(mgr, &tbl, NULL, path);
    REQUIRE_RC(rc);
    REQUIRE(tbl);
    const VCursor *curs = NULL;
    rc = VTableCreateCursorRead(tbl, &curs);
    REQUIRE_RC(rc);
    REQUIRE(curs);
    uint32_t col_idx = 0;
    rc = VCursorAddColumn(curs, &col_idx, "READ_TYPE");
    REQUIRE_RC(rc);
    REQUIRE(col_idx);
    rc = VCursorOpen(curs);
    REQUIRE_RC(rc);
    rc = VCursorOpenRow(curs);
    REQUIRE_RC(rc);
    uint8_t buffer[256];
    uint32_t row_len = 0;
    rc = VCursorRead(curs, col_idx, 8, buffer, sizeof buffer, &row_len);
    REQUIRE_RC(rc);
}*/

#ifdef ALLWIN
TEST_CASE(WIN_CRASH) {
    uint32_t status = eVdbBlastErr;
    VdbBlastMgr *mgr = VdbBlastInit(&status);
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    REQUIRE(mgr);
    VdbBlastRunSet *setR
        = VdbBlastMgrMakeRunSet(mgr, &status, min_read_length, false);
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    REQUIRE(setR);
    VdbBlastRunSet *setD
        = VdbBlastMgrMakeRunSet(mgr, &status, min_read_length, false);
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    REQUIRE(setD);
    const char *accs[] = {"SRR002749", "SRR363367"};
    for (int i = 0; i < 2; ++i) {
        status = VdbBlastRunSetAddRun(setD, accs[i]);
        REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
        status = VdbBlastRunSetAddRun(setR, accs[i]);
        CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    }
    VdbBlast4naReader *readerR = VdbBlastRunSetMake4naReader(setR, &status);
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    REQUIRE(readerR);
    VdbBlast4naReader *readerD = VdbBlastRunSetMake4naReader(setD, &status);
    REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
    REQUIRE(readerD);
    for (int i = 0; i < 2; ++i) {
        uint8_t buffer[Runs::BSIZE];
        uint32_t status = eVdbBlastErr;
        /*size_t n =*/ VdbBlast4naReaderRead
            (readerR, &status, i, 0, buffer, sizeof buffer);
        size_t length = 0;
//      const uint8_t *dt =
            VdbBlast4naReaderData(readerD, &status, i, &length);
        REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
        REQUIRE_GE(sizeof buffer, length);
    }
    VdbBlast4naReaderRelease(readerR);
    VdbBlast4naReaderRelease(readerD);
    VdbBlastRunSetRelease(setR);
    VdbBlastRunSetRelease(setD);
    VdbBlastMgrRelease(mgr);
}
#endif

#ifdef ALLWIN
FIXTURE_TEST_CASE(WIN_CRASH_READ_vs_DATA_4NA, RunSetFixture) {
    const char *accs[] = {"SRR002749", "SRR363367"};
//  Input &input(INPUT4na);
    uint32_t status = eVdbBlastErr;
    VdbBlastRunSet *setD
        = VdbBlastMgrMakeRunSet(mgr, &status, min_read_length, false);
    CHECK(setD != NULL);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    Reader::Reset();
//  status = AddRuns(input);    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    for (int i = 0; i < 2; ++i) {
        //Input::CIAcc i = input.BeginAcc(); i < input.EndAcc(); ++i) {
        string acc(accs[i]);
        status = VdbBlastRunSetAddRun(setD, acc.c_str());
        REQUIRE_EQUAL((int)status, (int)eVdbBlastNoErr);
        status = VdbBlastRunSetAddRun(set, acc.c_str());
        CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    }
    VdbBlast4naReader *reader = VdbBlastRunSetMake4naReader(set, &status);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK(reader != NULL);
    VdbBlast4naReader *readerD = VdbBlastRunSetMake4naReader(setD, &status);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK(readerD != NULL);
    int read_id = 0;
    for (int i = 0; i < 2; ++i) {
        //Input::CIAcc i = input.BeginAcc(); i < input.EndAcc(); ++i) {
/*      const Data *d = *i;
        assert(d);
        ostringstream s;
        s << d->acc << ".4na." << d->spotN;
        string tmp(d->read);
        CHECK(Test4NAData::convert(tmp));*/
        uint8_t buffer[Runs::BSIZE];
        uint32_t status = eVdbBlastErr;
        size_t length = 0;
//      const uint8_t *dt =
            VdbBlast4naReaderData(readerD, &status, read_id, &length);
        CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
        REQUIRE_GE(sizeof buffer, length);
//      CHECK_EQUAL(tmp.size(), length);
      /*size_t n = */ VdbBlast4naReaderRead
            (reader, &status, read_id++, 0, buffer, sizeof buffer);
//      Test4NAData a(s.str(), reader, readerD, d->read, read_id++);
//      ErrorCounterAdd(a.GetErrorCounter());
    }
    VdbBlast4naReaderRelease(reader);
    VdbBlast4naReaderRelease(readerD);
    VdbBlastRunSetRelease(setD);
//  cerr << input.SizeDat() << " 4NA reads processed" << "\n";
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(UNCLIPPED_BIOLOGICAL_READS, RunSetFixture) {
    REQUIRE_EQUAL(Add1Run("SRR066117"), (uint32_t)eVdbBlastNoErr);
    uint32_t status = eVdbBlastErr;
    VdbBlast4naReader *reader
            = VdbBlastRunSetMake4naReader(set, &status);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK(reader);
    size_t length = 0;
    const uint8_t *d = VdbBlast4naReaderData(reader, &status, 0, &length);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK(d);

    VdbBlast4naReaderRelease(reader);
}
#endif

#ifdef WGS
FIXTURE_TEST_CASE(VDB_BLAST_TEST_NAME_WGS, RunSetFixture) {
    REQUIRE_EQUAL(Add1Run("ABBW01"), (uint32_t)eVdbBlastNoErr);
    char name_buffer[16];
    memset(name_buffer, '-', sizeof name_buffer);
    name_buffer[sizeof name_buffer - 1] = '\0';
    CHECK_EQUAL((int)VdbBlastRunSetGetReadName
        (set, 0, name_buffer, sizeof name_buffer), 13);
    CHECK_EQUAL(string(name_buffer), string("ABBW01000001"));
}
#endif

#if 0
TEST_CASE(TEST_CHUNKED) {
    const VDBManager *mgr = NULL;
    rc_t rc = VDBManagerMakeRead(&mgr, NULL);
    assert(!rc && mgr);
    const VDatabase *db = NULL;
    const char path[] = "/panfs/traces01/wgs01/WGS/AA/CC/AACC01";
    rc = VDBManagerOpenDBRead(mgr, &db, NULL, path);
    assert(!rc && db);
    const VTable *tbl = NULL;
    rc = VDatabaseOpenTableRead(db, &tbl, "SEQUENCE");
    assert(!rc && tbl);
    const VCursor *curs = NULL;
    rc = VTableCreateCursorRead(tbl, &curs);
    assert(!rc && curs);
    uint32_t col_idx = 0;
    rc = VCursorAddColumn(curs, &col_idx, "READ");
    assert(!rc && col_idx);
    rc = VCursorOpen(curs);
    assert(!rc);
    for (int64_t spot = 1; spot <= 26; ++spot) {
        const VBlob *blob = NULL;
        rc = VCursorGetBlobDirect(curs, &blob, spot, col_idx);
        assert(!rc);
        VBlobRelease(blob);
    }
    VCursorRelease(curs);
    VTableRelease(tbl);
    VDatabaseRelease(db);
    VDBManagerRelease(mgr);
}
#endif

#ifdef TEST_VdbBlastRunFillReadDesc
typedef struct VdbBlastRun VdbBlastRun;
typedef struct ReadDesc {
    const VdbBlastRun *prev;

    VdbBlastRun *run;
    uint64_t spot; /* 1-based */
    uint32_t read; /* 1-based */

    uint64_t read_id; /* BioReadId in RunSet */
} ReadDesc;
typedef struct ReaderCols {
    uint32_t col_READ_FILTER;
    uint32_t col_READ_LEN;
    uint8_t *read_filter;
    uint32_t *read_len;
    uint8_t nReadsAllocated;
} ReaderCols;
typedef struct Reader2na {
    bool eor;
    ReadDesc desc;
    uint32_t col_READ;
    const VCursor *curs;
    size_t starting_base; /* 0-based, in current read */
    ReaderCols cols;
} Reader2na;
typedef struct Core2na {
    uint32_t min_read_length;
    bool hasReader;
    KLock *mutex;
    uint64_t initial_read_id;
    uint32_t irun; /* index in RunSet */
    bool eos;
    Reader2na reader;
} Core2na;
typedef struct Core4na {
    uint32_t min_read_length;
    KLock *mutex;
    ReadDesc desc;
    const VCursor *curs;
    const VBlob *blob; /* TODO */
    ReaderCols cols;
    uint32_t col_READ;
} Core4na;
typedef struct RunSet {
    VdbBlastRun *run;
    uint32_t krun; /* number of run-s */
    uint32_t nrun; /* sizeof of run-s */
} RunSet;
struct VdbBlastRunSet {
    KRefcount refcount;
    bool protein;
    VdbBlastMgr *mgr;

    RunSet runs;

    bool beingRead;
    Core2na core2na;
    Core4na core4na;
};

#endif

#ifdef ALL
static int f(char* buffer, size_t sz, const char* path, ...) {
    va_list args;
    va_start(args, path);
    int p = vsnprintf(buffer, sz, path, args);
    va_end(args);
    return p;
}
FIXTURE_TEST_CASE(TEST_VdbBlastRunFillReadDesc, RunSetFixture) {
    const char path[] = "/netmnt/traces04/sra4/SRR/000002/SRR002749/";
    char buffer[4096];
    f(buffer, sizeof buffer, path);

#ifdef TEST_VdbBlastRunFillReadDesc
    ReadDesc desc;

    VdbBlastRun *r = set->runs.run;
    CHECK_EQUAL(_VdbBlastRunFillReadDesc(r, 0, &desc), (uint32_t)eVdbBlastErr);
#endif

    CHECK_EQUAL(Add1Run("/netmnt/traces04/sra4/SRR/000002/SRR002749/"),
        (uint32_t)eVdbBlastNoErr);

#ifdef TEST_VdbBlastRunFillReadDesc
    r = set->runs.run;
    CHECK_EQUAL(
        _VdbBlastRunFillReadDesc(r, 0, &desc), (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(
        _VdbBlastRunFillReadDesc(r, 1, &desc), (uint32_t)eVdbBlastErr);
#endif

    uint32_t status = eVdbBlastErr;
    char name_buffer[256] = "";
    size_t s = VdbBlastRunSetGetName(set,
        &status, name_buffer, sizeof name_buffer);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK(s);
    CHECK_LT(s, sizeof name_buffer);
    CHECK_EQUAL(string(name_buffer), string("SRR002749"));

    s = VdbBlastRunSetGetReadName(set, 1, name_buffer, sizeof name_buffer);
    CHECK_EQUAL(s, (size_t)0);

    s = VdbBlastRunSetGetReadName(set, 0, name_buffer, sizeof name_buffer);
    CHECK_LT(s, sizeof name_buffer);
    CHECK(s);
    CHECK_EQUAL(string(name_buffer), string("SRR002749.1.2"));

    uint64_t read_id = ~0;
    status = VdbBlastRunSetGetReadId(set, name_buffer, strlen(name_buffer),
        &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(read_id, (uint64_t)0);

    char name[256] = "";
    s = VdbBlastRunSetGetReadName(set, read_id, name, sizeof name);
    CHECK_EQUAL(s, strlen(name_buffer));
    CHECK_EQUAL(string(name), string(name_buffer));

    name_buffer[0] = '\0';
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "S");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "SRR002749");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "SRR002749.");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "SRR002749.1");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "SRR002749..");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "SRR002749.1.");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, ".1.2");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "SRR002749..2");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "SRR002740.1.2");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "SRR002749.0.2");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "SRR002749.1.0");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "SRR002749.2.2");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    strcpy(name_buffer, "SRR002749.1.1");
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);

    CHECK_EQUAL(Add1Run("SRR363367"), (uint32_t)eVdbBlastErr);
}
#endif

#ifdef WGS
FIXTURE_TEST_CASE(TESTVdbBlastRunSetGetReadId, RunSetFixture) {
    REQUIRE_EQUAL(Add1Run("SRR002749"), (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(Add1Run("SRR363367"), (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(Add1Run("ABBW01"), (uint32_t)eVdbBlastNoErr);
    string s("ABBW01000002");
    uint64_t read_id = ~0;
    CHECK_EQUAL(VdbBlastRunSetGetReadId(set, s.c_str(), s.size(), &read_id),
        (uint32_t)0);
    CHECK_EQUAL(read_id, (uint64_t)3);
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(TESTNameVsId, RunSetFixture) {
    uint32_t status = VdbBlastRunSetAddRun(set, "SRR393572"); // TB
    REQUIRE_EQUAL(status, (uint32_t)eVdbBlastNoErr);

    CHECK_EQUAL(VdbBlastRunSetGetNumSequences(set, &status), (uint64_t)3);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);

    uint64_t read_id = 0;
    char name_buffer[256] = "";
    size_t s = VdbBlastRunSetGetReadName(set, read_id,
        name_buffer, sizeof name_buffer);
    string name("SRR393572.1.2");
    CHECK_EQUAL(s, name.size());
    CHECK_EQUAL(name, string(name_buffer));
    uint64_t read = ~0;
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(read, read_id);


    ++read_id;
    s = VdbBlastRunSetGetReadName(set, read_id,
        name_buffer, sizeof name_buffer);
    name = "SRR393572.2.2";
    CHECK_EQUAL(s, name.size());
    CHECK_EQUAL(name, string(name_buffer));
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(read, read_id);
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(TESTAllNamesVsIds, RunSetFixture) {
    Input &input(INPUT2na);
    REQUIRE_EQUAL((int)AddRuns(input), (int)eVdbBlastNoErr);

    uint32_t status = eVdbBlastErr;
    uint64_t n = VdbBlastRunSetGetNumSequences(set, &status);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    string prev;
    uint64_t i = 0;
    char name_buffer[256] = "";
    uint64_t read = ~0;
    size_t s = ~0;
    uint64_t pr = -1;
    for (; i < n; ++i) {
        s = VdbBlastRunSetGetReadName(set, i,
            name_buffer, sizeof name_buffer);
        CHECK(s);
        status = VdbBlastRunSetGetReadId(set,
            name_buffer, strlen(name_buffer), &read);
        CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
        CHECK_EQUAL(read, i);
        CHECK_NE(prev, string(name_buffer));
        prev = name_buffer;
        if (pr >= 0) {
            CHECK_EQUAL(pr + 1, i);
        }
        pr = i;
    }
    cerr << i << " == VdbBlastRunSetGetNumSequences = " << n << "\n";
    s = VdbBlastRunSetGetReadName(set, i, name_buffer, sizeof name_buffer);
    CHECK(s == 0);
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastErr);
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(NamesVsIds4naReader, RunSetFixture) {
    uint32_t status = Add1Run("BABY01");
    REQUIRE_EQUAL(status, (uint32_t)eVdbBlastNoErr);

    uint64_t read = ~0;

    string sname("BABY01000002");
    const char *name = sname.c_str();
    status = VdbBlastRunSetGetReadId(set, name, strlen(name), &read);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(read, (uint64_t)1);
    VdbBlast4naReader* r = VdbBlastRunSetMake4naReader(set, &status);
    CHECK(r);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    size_t length = 0;
    const uint8_t *d = VdbBlast4naReaderData(r, &status, read, &length);
    CHECK(d);
    CHECK_EQUAL(length, (size_t)25118);

    sname = "BABY01000001";
    name = sname.c_str();
    status = VdbBlastRunSetGetReadId(set, name, strlen(name), &read);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(read, (uint64_t)0);
    CHECK(r);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    d = VdbBlast4naReaderData(r, &status, read, &length);
    CHECK(d);
    CHECK_EQUAL(length, (size_t)17388);
}
#endif

#ifdef ALL
FIXTURE_TEST_CASE(TESTRefseq, RunSetFixture) {
/*3994 NT_113887.1
  4262 NT_113947.1 9334
  4262 GL000207.1  9340
  7260 NW_003571062.1
 12854 NT_113903.1 */
    REQUIRE_EQUAL(Add1Run("NW_003571062.1"), (uint32_t)eVdbBlastNoErr);
// TODO: STRANGE WGS CHECK_EQUAL(Add1Run("DAAB01"), (uint32_t)eVdbBlastNoErr);
    uint32_t status = eVdbBlastErr;
    uint64_t n = 1;
    CHECK_EQUAL(VdbBlastRunSetGetNumSequences(set, &status), n);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(VdbBlastRunSetGetNumSequencesApprox(set), n);
    uint64_t l = 7260;
    CHECK_EQUAL(VdbBlastRunSetGetTotalLength(set, &status), l);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(VdbBlastRunSetGetTotalLengthApprox(set), l);
    CHECK_EQUAL(VdbBlastRunSetGetMaxSeqLen(set), l);
    CHECK_EQUAL(VdbBlastRunSetGetMinSeqLen(set), l);
    CHECK_EQUAL(VdbBlastRunSetGetAvgSeqLen(set), l);
    CHECK(!VdbBlastRunSetIsProtein(set));

    char name_buffer[256];
    size_t s
        = VdbBlastRunSetGetName(set, &status, name_buffer, sizeof name_buffer);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_LT(s, sizeof name_buffer);
    CHECK_EQUAL(s, strlen(name_buffer));
    CHECK_EQUAL(string("NW_003571062.1"), string(name_buffer));

    s = VdbBlastRunSetGetReadName(set, 0, name_buffer, sizeof name_buffer);
    CHECK_LT(s, sizeof name_buffer);
    CHECK_EQUAL(s, strlen(name_buffer));
    CHECK_EQUAL(string("NW_003571062.1"), string(name_buffer));

    uint64_t read_id = ~0;
    status = VdbBlastRunSetGetReadId(set,
        name_buffer, strlen(name_buffer), &read_id);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(read_id, (uint64_t)0);

    s = VdbBlastRunSetGetReadName(set, 1, name_buffer, sizeof name_buffer);
    CHECK_EQUAL(s, (size_t)0);

    strcpy(name_buffer, "NW_003571062.1");
    CHECK_EQUAL(VdbBlastRunSetGetReadId(set, name_buffer, strlen(name_buffer),
        &read_id), (uint32_t)eVdbBlastNoErr);
    CHECK_EQUAL(read_id, (uint64_t)0);

    char name[256] = "";
    s = VdbBlastRunSetGetReadName(set, read_id, name, sizeof name);
    CHECK_EQUAL(s, strlen(name_buffer));
    CHECK_EQUAL(string(name), string(name_buffer));

    VdbBlast2naReader* r = VdbBlastRunSetMake2naReader(set, &status, 0);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastNoErr);
    CHECK(r);

    uint8_t buffer[7260];
    n = VdbBlast2naReaderRead(r, &status, &read_id, buffer, sizeof buffer);
    //CHECK_EQUAL(n, (uint64_t)7260);

    VdbBlast2naReaderRelease(r);
}
#endif

#ifdef ALL
// TODO find reads with var-read-len: find other reads with no BIO_BASE_COUNT
FIXTURE_TEST_CASE(TESTVdbBlastRunSetGetTotalLengthApprox, RunSetFixture) {
    REQUIRE_EQUAL(Add1Run("DRR001056"), (uint32_t)eVdbBlastNoErr);
    uint32_t status = eVdbBlastErr;
    uint64_t l = VdbBlastRunSetGetTotalLength(set, &status);
    CHECK_EQUAL(status, (uint32_t)eVdbBlastTooExpensive);
    CHECK_EQUAL(l, (uint64_t)0);
    l = VdbBlastRunSetGetTotalLengthApprox(set);
    CHECK_EQUAL(l, (uint64_t)180008856);
}
#endif

/*Test4NAData::Test4NAData(const string &name, const VdbBlast4naReader *rRead,
        const VdbBlast4naReader *rData,
        const string &read, int read_id)
    : TestCase(name)
{
    string tmp(read);
    CHECK(convert(tmp));
    uint8_t buffer[Runs::BSIZE];
    uint32_t status = eVdbBlastErr;
    size_t length = 0;
    const uint8_t *d
        = VdbBlast4naReaderData(rData, &status, read_id, &length);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    REQUIRE_GE(sizeof buffer, length);
    CHECK_EQUAL(tmp.size(), length);
    size_t n = VdbBlast4naReaderRead
        (rRead, &status, read_id++, 0, buffer, sizeof buffer);
    CHECK_EQUAL((int)status, (int)eVdbBlastNoErr);
    CHECK_EQUAL(n, length);
    CHECK_LE(n, sizeof buffer);
    status = eVdbBlastErr;
    CHECK(cmp((uint8_t*)tmp.c_str(), buffer, tmp.size()));
    CHECK(cmp(buffer, d, n));
}
*/

/*
    VdbBlastStdaaReader *a = VdbBlastRunSetMakeStdaaReader(set, NULL);
    VdbBlastStdaaReader *a2 = VdbBlastStdaaReaderAddRef(a);
    VdbBlastStdaaReaderRelease(a2);
    n = VdbBlastStdaaReaderRead(a, NULL, 0, NULL, 0);
    d = VdbBlastStdaaReaderData(a, NULL, 0, NULL);
    VdbBlastStdaaReaderRelease(a); */

static rc_t argsHandler(int argc, char* argv[]) {
    Args* args = NULL;
    rc_t rc = ArgsMakeAndHandle(&args, argc, argv, 0, NULL, 0);
    ArgsWhack(args);
    return rc;
}

extern "C" {
    const char UsageDefaultName[] = "unit-test-blast";
	rc_t CC UsageSummary(const char *prog_name)
    {   return KOutMsg("Usage:\t%s\n", prog_name); }
	rc_t CC Usage(const Args *args) {
        const char* progname = UsageDefaultName;
        const char* fullpath = UsageDefaultName;

        rc_t rc = (args == NULL) ?
            RC (rcApp, rcArgv, rcAccessing, rcSelf, rcNull):
            ArgsProgram(args, &fullpath, &progname);

        if (rc != 0)
            progname = fullpath = UsageDefaultName;

        UsageSummary(progname);

        KOutMsg("\nOptions:\n");
        HelpOptionsStandard();
        HelpVersion(fullpath, KAppVersion());
        
        return rc;
    }
    ver_t CC KAppVersion(void) { return 0; }
    rc_t CC KMain(int argc, char *argv[]) {
//      assert(!VdbBlastMgrKConfigPrint(NULL));
        assert(!VdbBlastMgrKLogLevelSetInfo(NULL));
//      assert(!VdbBlastMgrKDbgSetString(NULL, "VDB"));
        int r = BlastTestSuite(argc, argv);
        Test2NAData release;
        return r;
    }
}
