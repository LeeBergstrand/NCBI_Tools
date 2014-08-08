default std all: link-test

MARCH = $(shell uname -m)
ifeq (x86_64, $(MARCH))
	ARCH = x86_64
	OPT = -O3
else
	ARCH = i386
	OPT = -O3 -march=pentium4
endif


CFLAGS = -I../.. -I../../inc/$(ARCH) -I../../inc -DLINUX -DUNIX -D_GNU_SOURCE -D_REENTRANT -D_FILE_OFFSET_BITS=64
DEBUG = -D_DEBUGGING -g -Wall
ifeq (x86_64, $(ARCH))
	RELEASE = -DNDEBUG -O3
else
	RELEASE = -DNDEBUG -O3 -march=pentium4
endif

.SUFFIXES:

%.dbg.o: %.c
	gcc -c -o $@ $(DEBUG) $(CFLAGS) -MD $<

%.rel.o: %.c
	gcc -c -o $@ $(RELEASE) $(CFLAGS) -MD $<

%.prof.o: %.c
	gcc -c -pg -o $@ -D_PROFILING $(RELEASE) $(CFLAGS) -MD $<

%.pic.o: %.c
	gcc -c -o $@ $(RELEASE) -fPIC $(CFLAGS) -MD $<

VPATH = ../../src

tracedb-$(ARCH).o:
	@ mkdir -p obj/$(ARCH)
	@ $(MAKE) -C obj/$(ARCH) -f ../../Makefile ../../$@

link-test-$(ARCH)-dbg: tracedb-$(ARCH).o src/link-test.c
	gcc -I. -Iinc/$(ARCH) -Iinc -D_DEBUGGING -Wall -o $@ tracedb-$(ARCH)-dbg.o src/link-test.c -lpthread -lz -MD

link-test-$(ARCH)-rel: tracedb-$(ARCH).o src/link-test.c
	rm -f tracedb.o
	ln -s tracedb-$(ARCH)-rel.o tracedb.o
	gcc -I. -Iinc/$(ARCH) -Iinc -DNDEBUG -o $@ tracedb-$(ARCH)-rel.o src/link-test.c -lpthread -lz -MD

link-test: link-test-$(ARCH)-dbg link-test-$(ARCH)-rel

TRACEDB_DBG_OBJ = \
	logging.dbg.o \
	timeout.dbg.o \
	lock.dbg.o \
	cond.dbg.o \
	thread.dbg.o \
	semaphore.dbg.o \
	container.dbg.o \
	btcache.dbg.o \
	idecompress.dbg.o \
	idecompress8.dbg.o \
	idecompress32.dbg.o \
	text.dbg.o \
	ascii.dbg.o \
	vfdio.dbg.o \
	vfd.dbg.o \
	mmap.dbg.o \
	tracedb.dbg.o \
	tracedata.dbg.o \
	nuenc.dbg.o \
	nudec.dbg.o

TRACEDB_REL_OBJ = \
	timeout.rel.o \
	lock.rel.o \
	cond.rel.o \
	thread.rel.o \
	semaphore.rel.o \
	container.rel.o \
	btcache.rel.o \
	idecompress.rel.o \
	idecompress8.rel.o \
	idecompress32.rel.o \
	text.rel.o \
	ascii.rel.o \
	vfdio.rel.o \
	vfd.rel.o \
	mmap.rel.o \
	tracedb.rel.o \
	tracedata.rel.o \
	nudec.rel.o

TRACEDB_PROF_OBJ = \
	timeout.prof.o \
	lock.prof.o \
	cond.prof.o \
	thread.prof.o \
	semaphore.prof.o \
	container.prof.o \
	btcache.prof.o \
	idecompress.prof.o \
	idecompress8.prof.o \
	idecompress32.prof.o \
	text.prof.o \
	ascii.prof.o \
	vfdio.prof.o \
	vfd.prof.o \
	mmap.prof.o \
	tracedb.prof.o \
	tracedata.prof.o \
	nudec.prof.o

TRACEDB_PIC_OBJ = \
	timeout.pic.o \
	lock.pic.o \
	cond.pic.o \
	thread.pic.o \
	semaphore.pic.o \
	container.pic.o \
	btcache.pic.o \
	idecompress.pic.o \
	idecompress8.pic.o \
	idecompress32.pic.o \
	text.pic.o \
	ascii.pic.o \
	vfdio.pic.o \
	vfd.pic.o \
	mmap.pic.o \
	tracedb.pic.o \
	tracedata.pic.o \
	nudec.pic.o

../../tracedb-$(ARCH)-dbg.o: $(TRACEDB_DBG_OBJ)
	ld -r -o $@ $^

../../tracedb-$(ARCH)-rel.o: $(TRACEDB_REL_OBJ)
	ld -r -o $@ $^

../../tracedb-$(ARCH)-prof.o: $(TRACEDB_PROF_OBJ)
	ld -r -o $@ $^

../../tracedb-$(ARCH)-pic.o: $(TRACEDB_PIC_OBJ)
	ld -r -o $@ $^

../../tracedb-$(ARCH).o: ../../tracedb-$(ARCH)-dbg.o ../../tracedb-$(ARCH)-rel.o ../../tracedb-$(ARCH)-prof.o ../../tracedb-$(ARCH)-pic.o

clean:
	rm -rf obj/$(ARCH) tracedb-$(ARCH)*.[od] tracedb.o link-test-$(ARCH)*

include $(wildcard *.d)
