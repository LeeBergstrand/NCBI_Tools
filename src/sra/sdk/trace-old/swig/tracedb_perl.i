// File tracedb.i
%module tracedb
%include "typemaps.i"
%{
#include "../tracedata.h"
#include "tracedb_stub.h"
%}

typedef TraceDB* TraceDBPtr;
typedef TraceData* TraceDataPtr;

TraceDBPtr Create(unsigned int hard_file_limit=0, size_t hard_vm_limit=0);
void Destroy( TraceDBPtr db);

/* retrun reference to the array of ints */
%typemap(out) tdata {
  ST(argvi) = sv_newmortal();
  sv_setiv((SV*)ST(argvi++),($1).error);
  ST(argvi) = sv_newmortal();
  SWIG_MakePtr(ST(argvi++), (void *) ($1).data, SWIGTYPE_p_TraceData,0);
}

tdata GetTraceData( TraceDB* db, const char* path, long long id, unsigned int fields=3);
void DestroyTraceData( TraceData* data);

/* retrun reference to the array of ints */
%typemap(out) qscore {
  $result = sv_newmortal();
  sv_setpvn($result, (char*)($1).data, ($1).length);
/*  $result = newRV($result);*/
  argvi++;
/*
  AV *myav;
  SV **svs;
  int i = 0;
  svs = (SV **) malloc(($1).length*sizeof(SV *));
  for (i = 0; i < ($1).length; i++) {
      svs[i] = sv_newmortal();
      sv_setiv((SV*)svs[i],($1).data[i]);
  };
  myav = av_make(($1).length,svs);
  free(svs);
  $result = newRV((SV*)myav);
  sv_2mortal($result);
  argvi++;*/
}

/* retrun refference to the string */
%typemap(out) bcall {
  $result = sv_newmortal();
  sv_setpvn($result, (char*)($1).data, ($1).length);
/*  $result = newRV($result);*/
  argvi++;
}

bcall GetBasecall( TraceData* data);
qscore GetQualscore( TraceData* data);


