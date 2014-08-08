default: tracedb.so libtracedb.a

MARCH = $(shell uname -m)
ifeq (x86_64, $(MARCH))
	ARCH = x86_64
	OPT = -O3
else
	ARCH = i386
	OPT = -O3 -march=pentium4
endif


CFLAGS =  -I/usr/lib/perl5/5.8.3/x86_64-linux-thread-multi/CORE/ -I../inc/$(ARCH) -I.. -I../inc/ -DLINUX -DUNIX -D_GNU_SOURCE -D_REENTRANT -D_FILE_OFFSET_BITS=64 $(OPT) -fPIC
DEBUG = -D_DEBUGGING -g -Wall
ifeq (x86_64, $(ARCH))
	RELEASE = -DNDEBUG -O3
else
	RELEASE = -DNDEBUG -O3 -march=pentium4
endif

.SUFFIXES:

%.o: %.c
	g++ -c -o $@ $(DEBUG) $(CFLAGS) -MD $<

tracedb_perl_wrap.c: tracedb_perl.i
	swig -perl5 tracedb_perl.i

tracedb_stub.o: tracedb_stub.c tracedb_stub.h

tracedb.so: tracedb_perl_wrap.o tracedb_stub.o
	g++ -shared tracedb_perl_wrap.o tracedb_stub.o ../tracedb-$(ARCH)-dbg.o -lpthread -lz -o $@ 

libtracedb.a: tracedb_stub.o ../tracedb-$(ARCH)-rel.o
	ar rc libtracedb.a ../tracedb-$(ARCH)-rel.o tracedb_stub.o

clean:
	rm -rf tracedb_perl_wrap.[cod] tracedb_stub.[od] tracedb.so tracedb.pm libtracedb.a

#include $(wildcard *.d)

