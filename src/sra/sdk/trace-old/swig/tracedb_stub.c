#include "tracedb_stub.h"

#include "../tracedb.h"
#include "../tracedata.h"

#include <stdlib.h>
#include <stdio.h>

TraceDBPtr Create(unsigned int hard_file_limit, int hard_vm_limit ) {
  TraceDB* ret = static_cast<TraceDB*>(malloc( sizeof(TraceDB)));
  if( !ret ) { return NULL; }
  if( TraceDBInit( ret, hard_file_limit, hard_vm_limit) ) { free( ret); return NULL; }
  ret->timeout=0;
  if( TraceDBInitDecompression( ret)) { free( ret); return NULL; }
  return ret;
}

void Destroy( TraceDBPtr db) {
  TraceDBWhackDecompression( static_cast<TraceDB*>(db));
  TraceDBWhack( static_cast<TraceDB*>(db), 0);
  free( db);
}

tdata GetTraceData( TraceDBPtr db, const char* path, long long id, unsigned int fields) {
  tdata ret;
  ret.data = NULL;
  ret.error = TraceDBGetTraceData( static_cast<TraceDB*>(db), path, id, fields, (TraceData **)&ret.data);
  return ret;
}

void DestroyTraceData( TraceDataPtr data) {
  TraceDataWhack( static_cast<TraceData*>(data));
}

struct basecall GetBasecall( TraceDataPtr data) {
  struct basecall bc;
  bc.length = 0;
  bc.data = static_cast<const char*>(TraceDataGetBasecall( static_cast<const TraceData*>(data), (size_t*)&bc.length));
  return bc;
}

struct qualscore GetQualscore( TraceDataPtr data) {
  struct qualscore q;
  q.length = 0;
  q.data = static_cast<const long long*>(TraceDataGetQualscore( static_cast<const TraceData*>(data), (size_t*)&q.length));
  return q;
}

