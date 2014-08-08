#ifdef __cplusplus
extern "C" {
#endif

typedef void* TraceDBPtr;
typedef void* TraceDataPtr;

TraceDBPtr Create(unsigned int hard_file_limit, int hard_vm_limit );
void Destroy( TraceDBPtr db);

struct tracedata {
  TraceDataPtr data;
  int error;
};

typedef struct tracedata tdata;

tdata GetTraceData( TraceDBPtr db, const char* path, long long id, unsigned int fields);
void DestroyTraceData( TraceDataPtr data);

struct basecall {
  const char* data;
  int length;
};

struct qualscore {
  const long long* data;
  int length;
};

typedef struct basecall bcall;
typedef struct qualscore qscore;

bcall GetBasecall( TraceDataPtr data);
qscore GetQualscore( TraceDataPtr data);

#ifdef __cplusplus
}
#endif


