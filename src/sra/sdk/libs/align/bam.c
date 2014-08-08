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

#include <align/extern.h>
#include <klib/defs.h>
#include <klib/debug.h>
#include <klib/sort.h>
#include <klib/rc.h>
#include <kfs/file.h>
#include <kfs/directory.h>
#include <kfs/mmap.h>
#include <klib/printf.h>
#include <klib/log.h>
#include <sysalloc.h>

#include <atomic32.h>
#include <strtol.h>

#include <align/bam.h>
#include "bam-priv.h"

#include <vfs/path.h>
#include <vfs/path-priv.h>
#include <kfs/kfs-priv.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#if 1
/*_DEBUGGING*/
#include <stdio.h>
#endif

#include <endian.h>
#include <byteswap.h>

#include <zlib.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
static uint16_t LE2HUI16(void const *X) { uint16_t y; memcpy(&y, X, sizeof(y)); return y; }
static uint32_t LE2HUI32(void const *X) { uint32_t y; memcpy(&y, X, sizeof(y)); return y; }
static uint64_t LE2HUI64(void const *X) { uint64_t y; memcpy(&y, X, sizeof(y)); return y; }
static  int16_t  LE2HI16(void const *X) {  int16_t y; memcpy(&y, X, sizeof(y)); return y; }
static  int32_t  LE2HI32(void const *X) {  int32_t y; memcpy(&y, X, sizeof(y)); return y; }
static  int64_t  LE2HI64(void const *X) {  int64_t y; memcpy(&y, X, sizeof(y)); return y; }
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
static uint16_t LE2HUI16(void const *X) { uint16_t y; memcpy(&y, X, sizeof(y)); return (uint16_t)bswap_16(y); }
static uint32_t LE2HUI32(void const *X) { uint32_t y; memcpy(&y, X, sizeof(y)); return (uint32_t)bswap_32(y); }
static uint64_t LE2HUI64(void const *X) { uint64_t y; memcpy(&y, X, sizeof(y)); return (uint64_t)bswap_64(y); }
static  int16_t  LE2HI16(void const *X) {  int16_t y; memcpy(&y, X, sizeof(y)); return ( int16_t)bswap_16(y); }
static  int32_t  LE2HI32(void const *X) {  int32_t y; memcpy(&y, X, sizeof(y)); return ( int32_t)bswap_32(y); }
static  int64_t  LE2HI64(void const *X) {  int64_t y; memcpy(&y, X, sizeof(y)); return ( int64_t)bswap_64(y); }
#endif

typedef struct BAMIndex BAMIndex;
typedef struct BGZFile BGZFile;

/* #pragma mark BGZFile *** Start *** */

#define VALIDATE_BGZF_HEADER 1
#if (ZLIB_VERNUM < 0x1230)
#undef VALIDATE_BGZF_HEADER
#warning "zlib too old, inflateGetHeader not available, not validating BGZF headers"
#else
#endif

#define ZLIB_BLOCK_SIZE ( 64 * 1024 )
typedef uint8_t zlib_block_t[ZLIB_BLOCK_SIZE];

#define MEM_ALIGN_SIZE ( 64 * 1024 )
/* MEM_CHUNK_SIZE must be an integer multiple of ZLIB_BLOCK_SIZE.
 * The multiple must be >= 2 shouldn't be < 3.
 */
#define MEM_CHUNK_SIZE ( 256 * ZLIB_BLOCK_SIZE )
#define CG_NUM_SEGS 4

typedef struct BGZFile_vt_s {
    rc_t (*FileRead)(void *, zlib_block_t, unsigned *);
    uint64_t (*FileGetPos)(void const *);
    float (*FileProPos)(void const *);
    uint64_t (*FileGetSize)(void const *);
    rc_t (*FileSetPos)(void *, uint64_t);
    void (*FileWhack)(void *);
} BGZFile_vt;

struct BGZFile {
    uint64_t fsize;
    uint64_t fpos;  /* position in file of first byte in buffer */
    const uint8_t *buf;   /* page aligned or memmapped */
    const KFile *kfp;
    uint8_t *_buf;  /* allocated */
    unsigned malign;
    size_t bcount;  /* number of valid bytes in buffer */
    uint32_t bpos;  /* position in buffer of read head */
    z_stream zs;
};

static
rc_t BGZFileGetMoreBytes(BGZFile *self)
{
    rc_t rc;
    
    self->fpos += self->bpos;
    self->bpos &= (MEM_ALIGN_SIZE - 1);
    self->fpos -= self->bpos;

    rc = KFileRead(self->kfp, self->fpos, self->_buf + self->malign,
                   MEM_CHUNK_SIZE, &self->bcount);
    if (rc) {
        DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BGZF), ("Error reading BAM file: %R\n", rc));
        return rc;
    }
    if (self->bcount == 0 || self->bcount == self->bpos)
        return RC(rcAlign, rcFile, rcReading, rcData, rcInsufficient);

    self->zs.avail_in = (uInt)(self->bcount - self->bpos);
    self->zs.next_in = (Bytef *)&self->buf[self->bpos];
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BGZF), ("Read %u bytes from BAM file at position %lu\n", self->zs.avail_in, self->fpos));
    
    return 0;
}

static
rc_t BGZFileRead(BGZFile *self, zlib_block_t dst, unsigned *pNumRead)
{
#if VALIDATE_BGZF_HEADER
    uint8_t extra[256];
    gz_header head;
#endif
    rc_t rc = 0;
    unsigned loops;
    int zr;
    
    *pNumRead = 0;
    if (self->bcount == 0 || self->zs.avail_in == 0) {
        rc = BGZFileGetMoreBytes(self);
        if (rc)
            return rc;
    }

#if VALIDATE_BGZF_HEADER
    memset(&head, 0, sizeof(head));
    head.extra = extra;
    head.extra_max = sizeof(extra);
    
    zr = inflateGetHeader(&self->zs, &head);
    assert(zr == Z_OK);
#endif
    
    self->zs.next_out = (Bytef *)dst;
    self->zs.avail_out = sizeof(zlib_block_t);

    for (loops = 0; loops != 2; ++loops) {
        uInt const in = self->zs.total_in;
        
        zr = inflate(&self->zs, Z_FINISH);
        self->bpos += self->zs.total_in - in;
        assert(self->zs.avail_in == self->bcount - self->bpos);
        
        switch (zr) {
        case Z_OK:
        case Z_BUF_ERROR:
            rc = BGZFileGetMoreBytes(self);
            if (rc) {
                if (GetRCObject(rc) == rcData && GetRCState(rc) == rcInsufficient) {
                    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BGZF), ("EOF in Zlib block after %lu bytes\n", self->fpos + self->bpos));
                    rc = RC(rcAlign, rcFile, rcReading, rcFile, rcTooShort);
                }
                return rc;
            }
            break;
        case Z_STREAM_END:
            DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BGZF), ("Zlib block size (before/after): %u/%u\n", self->zs.total_in, self->zs.total_out));
#if VALIDATE_BGZF_HEADER
            if (head.done) {
                unsigned const extra_len = head.extra_len;
                unsigned i;
                unsigned bsize = 0;
                
                DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BGZF), ("GZIP Header extra length: %u\n", extra_len));
                for (i = 0; i < extra_len; ) {
                    uint8_t const si1 = extra[i + 0];
                    uint8_t const si2 = extra[i + 1];
                    unsigned const slen = LE2HUI16(&extra[i + 2]);
                    
                    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BGZF), ("GZIP Header extra: %c%c(%u)\n", si1, si2, slen));
                    if (si1 == 'B' && si2 == 'C') {
                        bsize = 1 + LE2HUI16(&extra[i + 4]);
                        DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BGZF), ("BGZF Header extra field BC: bsize %u\n", bsize));
                        break;
                    }
                    i += slen + 4;
                }
                if (bsize == 0 || bsize != self->zs.total_in) {
                    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BGZF), ("BGZF Header extra field BC not found\n"));
                    rc = RC(rcAlign, rcFile, rcReading, rcFormat, rcInvalid); /* not BGZF */
                }
            }
            else {
                DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BGZF), ("GZIP Header not found\n"));
                rc = RC(rcAlign, rcFile, rcReading, rcFile, rcCorrupt);
            }
#endif
            *pNumRead = self->zs.total_out;
            zr = inflateReset(&self->zs);
            assert(zr == Z_OK);
            return rc;
        default:
            DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BGZF), ("Unexpected Zlib result %i\n", zr));
            return RC(rcAlign, rcFile, rcReading, rcFile, rcCorrupt);
        }
    }
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BGZF), ("Failed reading BAM file after %lu bytes\n", self->fpos + self->bpos));
    return RC(rcAlign, rcFile, rcReading, rcFile, rcTooShort);
}

static uint64_t BGZFileGetPos(const BGZFile *self)
{
    return self->fpos + self->bpos;
}

/* returns the position as proportion of the whole file */ 
static float BGZFileProPos(const BGZFile *self)
{
    return BGZFileGetPos(self) / (double)self->fsize;
}

static rc_t BGZFileSetPos(BGZFile *self, uint64_t pos)
{
    if (self->fpos > pos || pos >= self->fpos + self->bcount) {
        self->fpos = pos;
        self->fpos -= pos & (MEM_ALIGN_SIZE - 1);
        self->bpos = pos - self->fpos;
        self->bcount = 0; /* force re-read */
    }
    else {
        self->bpos = pos - self->fpos;
        self->zs.avail_in = (uInt)(self->bcount - self->bpos);
        self->zs.next_in = (Bytef *)&self->buf[self->bpos];
    }

    return 0;
}

typedef rc_t (*BGZFileWalkBlocks_cb)(void *ctx, const BGZFile *file,
                                     rc_t rc, uint64_t fpos,
                                     const zlib_block_t data, unsigned dsize);

/* Without Decompression */
static rc_t BGZFileWalkBlocksND(BGZFile *self, BGZFileWalkBlocks_cb cb, void *ctx)
{
    rc_t rc = 0;
#if VALIDATE_BGZF_HEADER
    uint8_t extra[256];
    char dummy[64];
    gz_header head;
    int zr;

    memset(&head, 0, sizeof(head));
    head.extra = extra;
    head.extra_max = sizeof(extra);
    
    do {
        unsigned loops;
        unsigned hsize = 0;
        unsigned bsize = 0;
        unsigned bsize2;
        uint64_t const fpos = self->fpos + self->bpos;
        
        self->zs.next_out = (Bytef *)dummy;
        self->zs.avail_out = sizeof(dummy);
        
        zr = inflateGetHeader(&self->zs, &head);
        assert(zr == Z_OK);
        
        for (loops = 0; loops != 2; ++loops) {
            uInt temp = self->zs.total_in;
            
            zr = inflate(&self->zs, Z_BLOCK);
            temp = self->zs.total_in - temp;
            self->bpos += temp;
            hsize += temp;
            if (head.done) {
                unsigned i;
                
                for (i = 0; i < head.extra_len; ) {
                    if (extra[i] == 'B' && extra[i + 1] == 'C') {
                        bsize = 1 + LE2HUI16(&extra[i + 4]);
                        break;
                    }
                    i += LE2HUI16(&extra[i + 2]);
                }
                break;
            }
            else if (self->zs.avail_in == 0) {
                rc = BGZFileGetMoreBytes(self);
                if (rc) {
                    rc = RC(rcAlign, rcFile, rcReading, rcFile, rcTooShort);
                    goto DONE;
                }
            }
            else {
                rc = RC(rcAlign, rcFile, rcReading, rcFile, rcCorrupt);
                goto DONE;
            }
        }
        if (bsize == 0) {
            rc = RC(rcAlign, rcFile, rcReading, rcFormat, rcInvalid); /* not BGZF */
            break;
        }
        bsize2 = bsize;
        bsize -= hsize;
        for ( ; ; ) {
            unsigned n = bsize;
            
            if (n > self->bcount - self->bpos)
                n = self->bcount - self->bpos;
            self->bpos += n;
            bsize -= n;
            if (self->bpos == self->bcount) {
                rc = BGZFileGetMoreBytes(self);
                if (rc) {
                    if (bsize)
                        rc = RC(rcAlign, rcFile, rcReading, rcFile, rcTooShort);
                    goto DONE;
                }
            }
            else {
                zr = inflateReset(&self->zs);
                assert(zr == Z_OK);
                self->zs.avail_in = (uInt)(self->bcount - self->bpos);
                self->zs.next_in = (Bytef *)&self->buf[self->bpos];
                rc = cb(ctx, self, fpos, 0, NULL, bsize2);
                break;
            }
        }
    } while (rc == 0);
DONE:
    if (GetRCState(rc) == rcInsufficient && GetRCObject(rc) == rcData)
        rc = 0;
    rc = cb(ctx, self, self->fpos + self->bpos, rc, NULL, 0);
#endif
    return rc;
}

static rc_t BGZFileWalkBlocksUnzip(BGZFile *self, zlib_block_t *bufp, BGZFileWalkBlocks_cb cb, void *ctx)
{
    rc_t rc;
    rc_t rc2;
    unsigned dsize;
    
    do {
        uint64_t const fpos = self->fpos + self->bpos;
        
        rc2 = BGZFileRead(self, *bufp, &dsize);
        rc = cb(ctx, self, fpos, rc2, *bufp, dsize);
    } while (rc == 0 && rc2 == 0);
    if (GetRCState(rc2) == rcInsufficient && GetRCObject(rc2) == rcData)
        rc2 = 0;
    rc = cb(ctx, self, self->fpos + self->bpos, rc2, NULL, 0);
    return rc ? rc : rc2;
}

static rc_t BGZFileWalkBlocks(BGZFile *self, bool decompress, zlib_block_t *bufp,
                              BGZFileWalkBlocks_cb cb, void *ctx)
{
    rc_t rc;
    
#if VALIDATE_BGZF_HEADER
#else
    decompress = true;
#endif
    self->fpos = 0;
    self->bpos = 0;
    
    rc = BGZFileGetMoreBytes(self);
    if (rc)
        return rc;
    
    if (decompress)
        return BGZFileWalkBlocksUnzip(self, bufp, cb, ctx);
    else
        return BGZFileWalkBlocksND(self, cb, ctx);
}

static uint64_t BGZFileGetSize(BGZFile const *const self)
{
    return self->fsize;
}

static void BGZFileWhack(BGZFile *self)
{
    inflateEnd(&self->zs);
    KFileRelease(self->kfp);
    if (self->_buf)
        free(self->_buf);
}

static rc_t BGZFileInit(BGZFile *self, const KFile *kfp, BGZFile_vt *vt)
{
    int i;
    rc_t rc;
    static BGZFile_vt const my_vt = {
        (rc_t (*)(void *, zlib_block_t, unsigned *))BGZFileRead,
        (uint64_t (*)(void const *))BGZFileGetPos,
        (float (*)(void const *))BGZFileProPos,
        (uint64_t (*)(void const *))BGZFileGetSize,
        (rc_t (*)(void *, uint64_t))BGZFileSetPos,
        (void (*)(void *))BGZFileWhack
    };
    
    memset(self, 0, sizeof(*self));
    memset(vt, 0, sizeof(*vt));
    
    i = inflateInit2(&self->zs, MAX_WBITS + 16); /* max + enable gzip headers */
    switch (i) {
    case Z_OK:
        break;
    case Z_MEM_ERROR:
        return RC(rcAlign, rcFile, rcConstructing, rcMemory, rcExhausted);
    default:
        return RC(rcAlign, rcFile, rcConstructing, rcNoObj, rcUnexpected);
    }
    
    rc = KFileSize(kfp, &self->fsize);
    if (rc)
        return rc;
    
    self->_buf = malloc(MEM_CHUNK_SIZE + MEM_ALIGN_SIZE);
    if (self->_buf == NULL)
        return RC(rcAlign, rcFile, rcConstructing, rcMemory, rcExhausted);
    self->malign = (MEM_ALIGN_SIZE - ((intptr_t)self->_buf & (MEM_ALIGN_SIZE - 1))) & (MEM_ALIGN_SIZE - 1);
    self->buf = self->_buf + self->malign;
    
    self->kfp = kfp;
    KFileAddRef(kfp);

    *vt = my_vt;
    
    return 0;
}

/* #pragma mark BGZFile *** End *** */

#ifndef WINDOWS

#include <kproc/thread.h>
#include <kproc/lock.h>
#include <kproc/cond.h>

/* #pragma mark BGZThreadFile *** Start *** */
typedef struct BGZThreadFile_s BGZThreadFile;

#define BUFFER_COUNT (3)

typedef struct BGZThreadFileWorkQElem_s BGZThreadFileWorkQElem;

struct BGZThreadFileWorkQElem_s {
    uint8_t *buf;
    uint64_t pos;
    unsigned bsz;
};

struct BGZThreadFile_s {
    BGZFile file;
    KLock *lock;
    KCondition *have_data;
    KCondition *need_data;
    KThread *th;
    uint64_t pos;
    BGZThreadFileWorkQElem que[BUFFER_COUNT];
    rc_t volatile rc;
    unsigned volatile nque;
    bool eof;
    uint8_t buffer[sizeof(zlib_block_t) * BUFFER_COUNT];
};

static rc_t BGZThreadFileRead(BGZThreadFile *self, zlib_block_t dst, unsigned *pNumRead)
{
    rc_t rc;
    
    *pNumRead = 0;
    
    KLockAcquire(self->lock);
    if ((rc = self->rc) == 0) {
        while (self->nque == 0 && (rc = self->rc) == 0)
            KConditionWait(self->have_data, self->lock);
        if (rc == 0) {
            BGZThreadFileWorkQElem const work = self->que[0];
            
            self->pos = work.pos;
            if (work.buf) {
                memcpy(dst, work.buf, *pNumRead = work.bsz);
                memmove(&self->que[0], &self->que[1], --self->nque * sizeof(self->que[0]));
                KConditionSignal(self->need_data);
            }
            else {
                self->eof = true;
                self->rc = rc = RC(rcAlign, rcFile, rcReading, rcData, rcInsufficient);
            }
        }
    }
    KLockUnlock(self->lock);
    return rc;
}

static rc_t CC BGZThreadFileMain(KThread const *const th, void *const vp)
{
    BGZThreadFile *const self = (BGZThreadFile *)vp;
    rc_t rc = 0;
    unsigned bufno;
    
    KLockAcquire(self->lock);
    for (bufno = 0; ; bufno = (bufno + 1) % BUFFER_COUNT) {
        while (self->nque == BUFFER_COUNT)
            KConditionWait(self->need_data, self->lock);
        {
            BGZThreadFileWorkQElem work;
            
            work.pos = BGZFileGetPos(&self->file);
            work.buf = &self->buffer[bufno * sizeof(zlib_block_t)];
            rc = BGZFileRead(&self->file, work.buf, &work.bsz);
            if (GetRCObject(rc) == rcData && GetRCState(rc) == rcInsufficient)
                work.buf = NULL;
            else if (rc)
                break;
            self->que[self->nque++] = work;
            KConditionSignal(self->have_data);
        }
    }
    self->rc = rc;
    KLockUnlock(self->lock);
    return 0;
}

static uint64_t BGZThreadFileGetPos(BGZThreadFile const *const self)
{
    return self->pos;
}

/* returns the position as proportion of the whole file */ 
static float BGZThreadFileProPos(BGZThreadFile const *const self)
{
    return BGZThreadFileGetPos(self) / (double)self->file.fsize;
}

static uint64_t BGZThreadFileGetSize(BGZThreadFile const *const self)
{
    return BGZFileGetSize(&self->file);
}

static rc_t BGZThreadFileSetPos(BGZThreadFile *const self)
{
    return RC(rcAlign, rcFile, rcPositioning, rcFunction, rcUnsupported);
}

static void BGZThreadFileWhack(BGZThreadFile *const self)
{
    KThreadCancel(self->th);
    KThreadWait(self->th, NULL);
    BGZFileWhack(&self->file);
    KConditionRelease(self->need_data);
    KConditionRelease(self->have_data);
    KLockRelease(self->lock);
    KThreadRelease(self->th);
}

static rc_t BGZThreadFileInit(BGZThreadFile *self, const KFile *kfp, BGZFile_vt *vt)
{
    rc_t rc;
    static BGZFile_vt const my_vt = {
        (rc_t (*)(void *, zlib_block_t, unsigned *))BGZThreadFileRead,
        (uint64_t (*)(void const *))BGZThreadFileGetPos,
        (float (*)(void const *))BGZThreadFileProPos,
        (uint64_t (*)(void const *))BGZThreadFileGetSize,
        (rc_t (*)(void *, uint64_t))BGZThreadFileSetPos,
        (void (*)(void *))BGZThreadFileWhack
    };
    
    memset(self, 0, sizeof(self));
    
    rc = BGZFileInit(&self->file, kfp, vt);
    if (rc == 0) {
        rc = KLockMake(&self->lock);
        if (rc == 0) {
            rc = KConditionMake(&self->have_data);
            if (rc == 0) {
                rc = KConditionMake(&self->need_data);
                if (rc == 0) {
                    rc = KThreadMake(&self->th, BGZThreadFileMain, self);
                    if (rc == 0) {
                        *vt = my_vt;
                        return 0;
                    }
                    KConditionRelease(self->need_data);
                }
                KConditionRelease(self->have_data);
            }
            KLockRelease(self->lock);
        }
        BGZFileWhack(&self->file);
    }
    memset(self, 0, sizeof(self));
    memset(vt, 0, sizeof(*vt));
    return rc;
}

#endif

struct BAMIndex {
    BAMFilePosition *refSeq[1];
};

struct BAMFile {
    uint64_t fpos_first;
    uint64_t fpos_cur;
    
    union {
        BGZFile plain;
#ifndef WINDOWS
        BGZThreadFile thread;
#endif
    } file;
    BGZFile_vt vt;
    
    BAMRefSeq *refSeq;          /* pointers into headerData1 except name points into headerData2 */ 
    BAMReadGroup *readGroup;    /* pointers into headerData1 */
    char const *version;
    char const *header;
    char    *headerData1;       /* gets used for refSeq and readGroup */
    uint8_t *headerData2;       /* gets used for refSeq */
    BAMAlignment *bufLocker;
    BAMIndex const *ndx;
    
    unsigned refSeqs;
    unsigned readGroups;
    
    atomic32_t refcount;
    unsigned ucfirst;           /* offset of first record in uncompressed buffer */
    unsigned bufSize;           /* current size of uncompressed buffer */
    unsigned bufCurrent;        /* location in uncompressed buffer of read head */
    bool eof;
    bool threaded;
    zlib_block_t buffer;        /* uncompressed buffer */
};

struct bam_alignment_s {
    uint8_t rID[4];
    uint8_t pos[4];
    uint8_t read_name_len;
    uint8_t mapQual;
    uint8_t bin[2];
    uint8_t n_cigars[2];
    uint8_t flags[2];
    uint8_t read_len[4];
    uint8_t mate_rID[4];
    uint8_t mate_pos[4];
    uint8_t ins_size[4];
    char read_name[1 /* read_name_len */];
/* if you change length of read_name,
 * adjust calculation of offsets in BAMFileRead */
/*  uint32_t cigar[n_cigars];
 *  uint8_t seq[(read_len + 1) / 2];
 *  uint8_t qual[read_len];
 *  uint8_t extra[...];
 */
};

typedef union bam_alignment_u {
    struct bam_alignment_s cooked;
    uint8_t raw[sizeof(struct bam_alignment_s)];
} bam_alignment;

struct offset_size_s {
    unsigned offset;
    unsigned size; /* this is the total length of the tag; length of data is size - 3 */
};

struct BAMAlignment {
    atomic32_t refcount;
    
    BAMFile *parent;
    bam_alignment *data;
    uint8_t *storage;
    unsigned datasize;
        
    unsigned cigar;
    unsigned seq;
    unsigned qual;
    unsigned numExtra;
    struct offset_size_s extra[1];
};

static const char cigarChars[] = {
    ct_Match,
    ct_Insert,
    ct_Delete,
    ct_Skip,
    ct_SoftClip,
    ct_HardClip,
    ct_Padded,
    ct_Equal,
    ct_NotEqual
    /* ct_Overlap must not appear in actual BAM file */
};

static int32_t getRefSeqId(const BAMAlignment *cself) {
    return LE2HI32(cself->data->cooked.rID);
}

static int32_t getPosition(const BAMAlignment *cself) {
    return LE2HI32(cself->data->cooked.pos);
}

static uint8_t getReadNameLength(const BAMAlignment *cself) {
    return cself->data->cooked.read_name_len;
}

static uint16_t getBin(const BAMAlignment *cself) {
    return LE2HUI16(cself->data->cooked.bin);
}

static uint8_t getMapQual(const BAMAlignment *cself) {
    return cself->data->cooked.mapQual;
}

static uint16_t getCigarCount(const BAMAlignment *cself) {
    return LE2HUI16(cself->data->cooked.n_cigars);
}

static uint16_t getFlags(const BAMAlignment *cself) {
    return LE2HUI16(cself->data->cooked.flags);
}

static uint32_t getReadLen(const BAMAlignment *cself) {
    return LE2HUI32(cself->data->cooked.read_len);
}

static int32_t getMateRefSeqId(const BAMAlignment *cself) {
    return LE2HI32(cself->data->cooked.mate_rID);
}

static int32_t getMatePos(const BAMAlignment *cself) {
    return LE2HI32(cself->data->cooked.mate_pos);
}

static int32_t getInsertSize(const BAMAlignment *cself) {
    return LE2HI32(cself->data->cooked.ins_size);
}

static char const *getReadName(const BAMAlignment *cself) {
    return &cself->data->cooked.read_name[0];
}

static void const *getCigarBase(BAMAlignment const *cself)
{
    return &cself->data->raw[cself->cigar];
}

static int opt_tag_cmp(uint8_t const a[2], uint8_t const b[2])
{
    int const d0 = (int)a[0] - (int)b[0];
    return d0 ? d0 : ((int)a[1] - (int)b[1]);
}

static int CC OptTag_sort(void const *a, void const *b, void *ctx)
{
    BAMAlignment const *const self = ctx;
    unsigned const a_off = ((struct offset_size_s const *)a)->offset;
    unsigned const b_off = ((struct offset_size_s const *)b)->offset;
    
    return opt_tag_cmp(&self->data->raw[a_off], &self->data->raw[b_off]);
}

static int CC OptTag_cmp(void const *key, void const *item, void *ctx)
{
    BAMAlignment const *const self = ctx;
    unsigned const offset = ((struct offset_size_s const *)item)->offset;
    
    return opt_tag_cmp(key, &self->data->raw[offset]);
}

static char const *get_RG(BAMAlignment const *cself)
{
    struct offset_size_s const *const x = kbsearch("RG", cself->extra, cself->numExtra, sizeof(cself->extra[0]), OptTag_cmp, (void *)cself);
    return (char const *)(x && cself->data->raw[x->offset + 2] == 'Z' ? &cself->data->raw[x->offset + 3] : NULL);
}

static struct offset_size_s const *get_CS_info(BAMAlignment const *cself)
{
    return kbsearch("CS", cself->extra, cself->numExtra, sizeof(cself->extra[0]), OptTag_cmp, (void *)cself);
}

static struct offset_size_s const *get_CQ_info(BAMAlignment const *cself)
{
    return kbsearch("CQ", cself->extra, cself->numExtra, sizeof(cself->extra[0]), OptTag_cmp, (void *)cself);
}

static char const *get_CS(BAMAlignment const *cself)
{
    struct offset_size_s const *const x = get_CS_info(cself);
    return (char const *)(x && cself->data->raw[x->offset + 2] == 'Z' ? &cself->data->raw[x->offset + 3] : NULL);
}

static uint8_t const *get_CQ(BAMAlignment const *cself)
{
    struct offset_size_s const *const x = get_CQ_info(cself);
    return (uint8_t const *)(x && cself->data->raw[x->offset + 2] == 'Z' ? &cself->data->raw[x->offset + 3] : NULL);
}

static struct offset_size_s const *get_OQ_info(BAMAlignment const *cself)
{
    return kbsearch("OQ", cself->extra, cself->numExtra, sizeof(cself->extra[0]), OptTag_cmp, (void *)cself);
}

static uint8_t const *get_OQ(BAMAlignment const *cself)
{
    struct offset_size_s const *const x = get_OQ_info(cself);
    return (uint8_t const *)(x && cself->data->raw[x->offset + 2] == 'Z' ? &cself->data->raw[x->offset + 3] : NULL);
}

static char const *get_XT(BAMAlignment const *cself)
{
    struct offset_size_s const *const x = kbsearch("XT", cself->extra, cself->numExtra, sizeof(cself->extra[0]), OptTag_cmp, (void *)cself);
    return (char const *)(x && cself->data->raw[x->offset + 2] == 'Z' ? &cself->data->raw[x->offset + 3] : NULL);
}

static struct offset_size_s const *get_CG_ZA_info(BAMAlignment const *cself)
{
    return kbsearch("ZA", cself->extra, cself->numExtra, sizeof(cself->extra[0]), OptTag_cmp, (void *)cself);
}

static struct offset_size_s const *get_CG_ZI_info(BAMAlignment const *cself)
{
    return kbsearch("ZI", cself->extra, cself->numExtra, sizeof(cself->extra[0]), OptTag_cmp, (void *)cself);
}

static struct offset_size_s const *get_CG_GC_info(BAMAlignment const *cself)
{
    return kbsearch("GC", cself->extra, cself->numExtra, sizeof(cself->extra[0]), OptTag_cmp, (void *)cself);
}

static struct offset_size_s const *get_CG_GS_info(BAMAlignment const *cself)
{
    return kbsearch("GS", cself->extra, cself->numExtra, sizeof(cself->extra[0]), OptTag_cmp, (void *)cself);
}

static struct offset_size_s const *get_CG_GQ_info(BAMAlignment const *cself)
{
    return kbsearch("GQ", cself->extra, cself->numExtra, sizeof(cself->extra[0]), OptTag_cmp, (void *)cself);
}

static rc_t BAMFileReadn(BAMFile *self, const unsigned len, uint8_t dst[/* len */]) {
    rc_t rc;
    unsigned cur;
    unsigned n = 0;
    
    if (len == 0)
        return 0;
    
    for (cur = 0; ; cur += n) {
        if (self->bufSize > self->bufCurrent) {
            n = self->bufSize - self->bufCurrent;
            if (cur + n > len)
                n = len - cur;
            memcpy(&dst[cur], &self->buffer[self->bufCurrent], n);
            self->bufCurrent += n;
        }
        if (self->bufCurrent != self->bufSize && self->bufSize != 0)
            return 0;
        if (self->bufSize != 0) {
            /* a seek has not just been done so update the file position.
             * if we didn't and a request for the position is made before the
             * next read, we will not have the position of the next read.
             *
             * if a seek had just been done then
             *    self->fpos_cur == BGZFileGetPos(&self->file)
             * is already true.
             */
            self->fpos_cur = self->vt.FileGetPos(&self->file);
            self->bufCurrent = 0;
            self->bufSize = 0;
            if (cur + n == len)
                return 0;
        }

        rc = self->vt.FileRead(&self->file, self->buffer, &self->bufSize);
        if (rc)
            return rc;
        if (self->bufSize == 0 || self->bufSize <= self->bufCurrent)
            return RC(rcAlign, rcFile, rcReading, rcData, rcInsufficient);
    }
}

static rc_t BAMFileReadI32(BAMFile *self, int32_t *rhs)
{
    uint8_t buf[sizeof(int32_t)];
    rc_t rc = BAMFileReadn(self, sizeof(int32_t), buf);
    
    if (rc == 0)
        *rhs = LE2HI32(buf);
    return rc;
}

static int CC comp_ReadGroup(const void *a, const void *b, void *ignored) {
    return strcmp(((BAMReadGroup const *)a)->name, ((BAMReadGroup const *)b)->name);
}

static rc_t ParseHD(BAMFile *self, char hdata[], unsigned hlen, unsigned *used)
{
    unsigned i;
    unsigned tag;
    unsigned value;
    int st = 0;
    int ws = 1;

    for (i = 0; i < hlen; ++i) {
        char const cc = hdata[i];
        
        if (ws && isspace(cc))
            continue;
        ws = 0;
        
        switch (st) {
        case 0:
            tag = i;
            ++st;
            break;
        case 1:
            if (isspace(cc))
                return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
            ++st;
            break;
        case 2:
            if (cc != ':')
                return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
            hdata[i] = '\0';
            value = i + 1;
            ++st;
            break;
        case 3:
            if (cc == '\t' || cc == '\r' || cc == '\n') {
                hdata[i] = '\0';
                
                if (strcmp(&hdata[tag], "VN") == 0)
                    self->version = &hdata[value];
                
                ++st;
                ws = 1;
            }
            break;
        case 4:
            if (cc == '@')
                goto DONE;
            tag = i;
            st = 1;
            break;
        }
    }
    if (st == 4) {
DONE:
        *used = i;
        return 0;
    }
    return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
}

static rc_t ParseSQ(BAMFile *self, char hdata[], unsigned hlen, unsigned *used, unsigned const rs_by_name[])
{
    unsigned i;
    unsigned tag;
    unsigned value;
    int st = 0;
    int ws = 1;
    BAMRefSeq rs;
    
    memset(&rs, 0, sizeof(rs));
    
    for (i = 0; i < hlen; ++i) {
        char const cc = hdata[i];
        
        if (ws && isspace(cc))
            continue;
        ws = 0;
        
        switch (st) {
        case 0:
            tag = i;
            ++st;
            break;
        case 1:
            if (isspace(cc))
                return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
            ++st;
            break;
        case 2:
#define HACKAMATIC 1
#if HACKAMATIC
            if (cc != ':') {
                if (i + 1 >= hlen || hdata[i+1] != ':')
                    return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
                else
                    ++i;
            }
#else
            if (cc != ':')
                return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
#endif
            hdata[i] = '\0';
            value = i + 1;
            ++st;
            break;
        case 3:
            if (cc == '\t' || cc == '\r' || cc == '\n') {
                unsigned j;
                
                hdata[i] = '\0';
                
                while (value < i && isspace(hdata[value]))
                    ++value;
                for (j = i; value < j && isspace(hdata[j - 1]); )
                    hdata[--j] = '\0';
                
                if (strcmp(&hdata[tag], "SN") == 0)
                    rs.name = &hdata[value];
                else if (strcmp(&hdata[tag], "LN") == 0)
                    rs.length = strtou64(&hdata[value], NULL, 10);
                else if (strcmp(&hdata[tag], "AS") == 0)
                    rs.assemblyId = &hdata[value];
#if HACKAMATIC
                else if (strcmp(&hdata[tag], "M5") == 0 || strcmp(&hdata[tag], "MD5") == 0)
#else
                else if (strcmp(&hdata[tag], "M5") == 0)
#endif
#undef HACKAMATIC
                {
                    unsigned len = j - value;
                    
                    if ((hdata[value] == '\'' || hdata[value] == '"') && hdata[value + len - 1] == hdata[value]) {
                        ++value;
                        len -= 2;
                    }
                    if (len == 32) {
                        rs.checksum = &rs.checksum_array[0];
                        for (j = 0; j != 16; ++j) {
                            int const ch1 = toupper(hdata[value + j * 2 + 0]);
                            int const ch2 = toupper(hdata[value + j * 2 + 1]);
                            
                            if (isxdigit(ch1) && isxdigit(ch2)) {
                                rs.checksum_array[j] =
                                    ((ch1 > '9' ? (ch1 - ('A' - 10)) : (ch1 - '0')) << 4) +
                                     (ch2 > '9' ? (ch2 - ('A' - 10)) : (ch2 - '0'));
                            }
                            else {
                                rs.checksum = NULL;
                                break;
                            }
                        }
                    }
                }
                else if (strcmp(&hdata[tag], "UR") == 0)
                    rs.uri = &hdata[value];
                else if (strcmp(&hdata[tag], "SP") == 0)
                    rs.species = &hdata[value];
                
                ++st;
                ws = 1;
            }
            break;
        case 4:
            if (cc == '@')
                goto DONE;
            tag = i;
            st = 1;
            break;
        }
    }
DONE:
    if (st == 4) {
        unsigned f = 0;
        unsigned e = self->refSeqs;
        
        if (rs.name == NULL || rs.length == 0) /* required tags */
            return RC(rcAlign, rcFile, rcParsing, rcConstraint, rcViolated);
        
        while (f < e) {
            unsigned const m = (f + e) >> 1;
            BAMRefSeq *const x = &self->refSeq[rs_by_name[m]];
            int const cmp = strcmp(rs.name, x->name);
            
            if (cmp < 0)
                e = m;
            else if (cmp > 0)
                f = m + 1;
            else {
                x->assemblyId = rs.assemblyId;
                x->uri = rs.uri;
                x->species = rs.species;
                if (rs.checksum) {
                    x->checksum = &x->checksum_array[0];
                    memcpy(x->checksum_array, rs.checksum_array, 16);
                }
                else
                    x->checksum = NULL;
                break;
            }
        }
        *used = i;
        return 0;
    }
    return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
}

static rc_t ParseRG(BAMFile *self, char hdata[], unsigned hlen, unsigned *used, BAMReadGroup *dst)
{
    unsigned i;
    unsigned tag;
    unsigned value;
    int st = 0;
    int ws = 1;
#if _DEBUGGING
    char const *cur = hdata;
#endif
    
    memset(dst, 0, sizeof(*dst));
    
    for (i = 0; i < hlen; ++i) {
        char const cc = hdata[i];
        
        if (ws && isspace(cc))
            continue;
        ws = 0;
        
        switch (st) {
        case 0:
            tag = i;
            ++st;
            break;
        case 1:
            if (isspace(cc))
                return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
            ++st;
            break;
        case 2:
            if (cc != ':')
                return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
#if _DEBUGGING
            cur = hdata + i + 1;
#endif
            hdata[i] = '\0';
            value = i + 1;
            ++st;
            break;
        case 3:
            if (cc == '\t' || cc == '\r' || cc == '\n') {
                unsigned j = i;
#if _DEBUGGING
                cur = hdata + i + 1;
#endif
                hdata[i] = '\0';

                while (value < i && isspace(hdata[value]))
                    ++value;
                while (value < j && isspace(hdata[j - 1]))
                    hdata[--j] = '\0';
                
                if ((hdata[value] == '\"' || hdata[value] == '\'') && hdata[value] == hdata[j - 1]) {
                    ++value;
                    hdata[j - 1] = '\0';
                }
                if (strcmp(&hdata[tag], "ID") == 0)
                    dst->name = &hdata[value];
                else if (strcmp(&hdata[tag], "SM") == 0)
                    dst->sample = &hdata[value];
                else if (strcmp(&hdata[tag], "LB") == 0)
                    dst->library = &hdata[value];
                else if (strcmp(&hdata[tag], "DS") == 0)
                    dst->description = &hdata[value];
                else if (strcmp(&hdata[tag], "PU") == 0)
                    dst->unit = &hdata[value];
                else if (strcmp(&hdata[tag], "PI") == 0)
                    dst->insertSize = &hdata[value];
                else if (strcmp(&hdata[tag], "CN") == 0)
                    dst->center = &hdata[value];
                else if (strcmp(&hdata[tag], "DT") == 0)
                    dst->runDate = &hdata[value];
                else if (strcmp(&hdata[tag], "PL") == 0)
                    dst->platform = &hdata[value];
                
                ++st;
                ws = 1;
            }
            break;
        case 4:
            if (cc == '@')
                goto DONE;
            tag = i;
            st = 1;
            break;
        }
    }
    if (st == 4) {
DONE:
        *used = i;
        if (dst->name == NULL) /* required */
            return RC(rcAlign, rcFile, rcParsing, rcConstraint, rcViolated);
        return 0;
    }
    return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
}

static rc_t ParseHeader(BAMFile *self, char hdata[], unsigned hlen, unsigned const rs_by_name[]) {
    unsigned rg = 0;
    unsigned i;
    unsigned tag;
    int st = 0;
    int ws = 1;
    unsigned used;
    rc_t rc;
    
    for (i = 0; i < hlen; ++i) {
        char const cc = hdata[i];
        
        if (ws && isspace(cc))
            continue;
        ws = 0;
        
        switch (st) {
        case 0:
            if (cc == '@')
                ++st;
            else
                return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
            break;
        case 1:
            if (isspace(cc))
                return RC(rcAlign, rcFile, rcParsing, rcData, rcInvalid);
            tag = i;
            ++st;
            break;
        case 2:
            if (isspace(cc)) {
                hdata[i] = '\0';
                if (i - tag == 2) {
                    if (strcmp(&hdata[tag], "HD") == 0) {
                        rc = ParseHD(self, &hdata[i + 1], hlen - i - 1, &used);
                        if (rc) return rc;
                        i += used;
                        st = 0;
                    }
                    else if (strcmp(&hdata[tag], "SQ") == 0) {
                        rc = ParseSQ(self, &hdata[i + 1], hlen - i - 1, &used, rs_by_name);
                        if (rc) return rc;
                        i += used;
                        st = 0;
                    }
                    else if (strcmp(&hdata[tag], "RG") == 0) {
                        rc = ParseRG(self, &hdata[i + 1], hlen - i - 1, &used, &self->readGroup[rg]);
                        if (GetRCObject(rc) == rcConstraint && GetRCState(rc) == rcViolated) {
                            (void)LOGERR(klogWarn, rc, "Read Group is missing ID in BAM header");
                            rc = 0;
                            if (self->readGroups) --self->readGroups;
                        }
                        else if (rc)
                            return rc;
                        else
                            ++rg;
                        i += used;
                        st = 0;
                    }
                }
                if (st == 2) {
                    ++st;
                    ws = 0;
                }
            }
            else if (i - tag > 2)
                ++st;
            break;
        case 3:
            if (cc == '\r' || cc == '\n') {
                st = 0;
                ws = 1;
            }
            break;
        }
    }
    ksort( self->readGroup, self->readGroups, sizeof(self->readGroup[0]), comp_ReadGroup, NULL );
    for (rg = 0; rg != self->readGroups; ++rg) {
        if (rg > 0 && strcmp(self->readGroup[rg - 1].name, self->readGroup[rg].name) == 0)
            return RC(rcAlign, rcFile, rcParsing, rcConstraint, rcViolated);  /* name must be unique */
        self->readGroup[rg].id = rg;
    }
    
    return 0;
}

static rc_t CountReadGroups(char const txt[], size_t len, unsigned *reads) {
    const char *const endp = txt + len;
    
    *reads = 0;
    
    do {
        while (txt != endp && isspace(*txt))
            ++txt;
        if (txt == endp || txt + 3 >= endp)
            break;
        
        if (txt[0] == '@' && txt[1] == 'R' && txt[2] == 'G')
            ++*reads;
        
        txt = memchr(txt, '\n', endp - txt);
    } while (txt);
    return 0;
}

static rc_t ReadMagic(BAMFile *self)
{
    uint8_t sig[4];
    rc_t rc = BAMFileReadn(self, 4, sig);
    
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("BAM signature: '%c%c%c' %u\n", sig[0], sig[1], sig[2], sig[3]));
    if (rc == 0 && (sig[0] != 'B' || sig[1] != 'A' || sig[2] != 'M' || sig[3] != 1))
        rc = RC(rcAlign, rcFile, rcReading, rcHeader, rcBadVersion);
    return rc;
}

static rc_t ReadHeaders(BAMFile *self,
                        char **headerText, unsigned *headerTextLen,
                        uint8_t **refData, unsigned *numrefs)
{
    unsigned hlen;
    char *htxt = NULL;
    unsigned nrefs;
    uint8_t *rdat = NULL;
    unsigned rdsz;
    unsigned rdms;
    unsigned i;
    int32_t i32;
    rc_t rc = BAMFileReadI32(self, &i32);
    
    if (rc) return rc;

    if (i32 < 0) {
        rc = RC(rcAlign, rcFile, rcReading, rcHeader, rcInvalid);
        goto BAILOUT;
    }
    hlen = i32;
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("BAM Header text size: %u\n", hlen));
    if (hlen) {
        htxt = malloc(hlen + 1);
        if (htxt == NULL) {
            rc = RC(rcAlign, rcFile, rcReading, rcMemory, rcExhausted);
            goto BAILOUT;
        }
        
        rc = BAMFileReadn(self, hlen, (uint8_t *)htxt); if (rc) goto BAILOUT;
        htxt[hlen] = '\0';
    }
    rc = BAMFileReadI32(self, &i32); if (rc) goto BAILOUT;
    if (i32 < 0) {
        rc = RC(rcAlign, rcFile, rcReading, rcHeader, rcInvalid);
        goto BAILOUT;
    }
    nrefs = i32;
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("BAM Header reference count: %u\n", nrefs));
    if (nrefs) {
        rdms = nrefs * 16;
        if (rdms < 4096)
            rdms = 4096;
        rdat = malloc(rdms);
        if (rdat == NULL) {
            rc = RC(rcAlign, rcFile, rcReading, rcMemory, rcExhausted);
            goto BAILOUT;
        }
        for (i = rdsz = 0; i < nrefs; ++i) {
            rc = BAMFileReadI32(self, &i32); if (rc) goto BAILOUT;
            if (i32 <= 0) {
                rc = RC(rcAlign, rcFile, rcReading, rcHeader, rcInvalid);
                goto BAILOUT;
            }
            if (rdsz + i32 + 8 > rdms) {
                void *tmp;
                
                do { rdms <<= 1; } while (rdsz + i32 + 8 > rdms);
                tmp = realloc(rdat, rdms);
                if (tmp == NULL) {
                    rc = RC(rcAlign, rcFile, rcReading, rcMemory, rcExhausted);
                    goto BAILOUT;
                }
                rdat = tmp;
            }
            memcpy(rdat + rdsz, &i32, 4);
            rdsz += 4;
            rc = BAMFileReadn(self, i32, &rdat[rdsz]); if (rc) goto BAILOUT;
            rdsz += i32;
            rc = BAMFileReadI32(self, &i32); if (rc) goto BAILOUT;
            memcpy(rdat + rdsz, &i32, 4);
            rdsz += 4;
        }
    }
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("BAM Header reference size: %u\n", rdsz));
    
    *headerText = htxt;
    *headerTextLen = hlen;
    *refData = rdat;
    *numrefs = nrefs;
    return 0;
    
BAILOUT:
    if (htxt)
        free(htxt);
    if (rdat)
        free(rdat);
    
    return rc;
}

static int CC comp_RefSeqName(const void *A, const void *B, void *ignored) {
    BAMFile const *self = (BAMFile const *)ignored;
    unsigned const a = *(unsigned const *)A;
    unsigned const b = *(unsigned const *)B;
    
    return strcmp(self->refSeq[a].name, self->refSeq[b].name);
}

static rc_t ProcessHeader(BAMFile *self, char const headerText[])
{
    unsigned *rs_by_name = NULL;
    unsigned i;
    unsigned cp;
    char *htxt;
    uint8_t *rdat;
    unsigned hlen;
    unsigned nrefs;
    rc_t rc = ReadMagic(self);

    if (rc) return rc;

    rc = ReadHeaders(self, &htxt, &hlen, &rdat, &nrefs);
    if (rc) return rc;
    
    self->fpos_first = self->fpos_cur;
    self->ucfirst = self->bufCurrent;
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("BAM Data records start at: %lu+%u\n", self->ucfirst, self->fpos_first));

    if (headerText) {
        free(htxt);
        hlen = strlen(headerText);
        htxt = malloc(hlen + 1);
        if (htxt == NULL) {
            free(rdat);
            return RC(rcAlign, rcFile, rcConstructing, rcMemory, rcExhausted);
        }
        memcpy(htxt, headerText, hlen + 1);
    }
    
    self->headerData2 = rdat;
    if (hlen) {
        self->header = htxt;
        self->headerData1 = malloc(hlen + 1);
        if (self->headerData1 == NULL)
            return RC(rcAlign, rcFile, rcConstructing, rcMemory, rcExhausted);
        memcpy(self->headerData1, self->header, hlen + 1);
    }
    else {
        htxt = malloc(1);
        htxt[0] = '\0';
        self->header = htxt;
        self->headerData1 = NULL;
    }
    self->refSeqs = nrefs;
    if (nrefs) {
        self->refSeq = calloc(nrefs, sizeof(self->refSeq[0]));
        if (self->refSeq == NULL)
            return RC(rcAlign, rcFile, rcConstructing, rcMemory, rcExhausted);
        
        rs_by_name = calloc(nrefs, sizeof(rs_by_name[0]));
        if (rs_by_name == NULL)
            return RC(rcAlign, rcFile, rcConstructing, rcMemory, rcExhausted);
        
        for (i = cp = 0; i < nrefs; ++i) {
            uint32_t nlen;
            uint32_t rlen;
            
            rs_by_name[i] = i;
            self->refSeq[i].id = i;
            memcpy(&nlen, &self->headerData2[cp], 4);
            cp += 4;
            self->refSeq[i].name = (char const *)&self->headerData2[cp];
            cp += nlen;
            memcpy(&rlen, &self->headerData2[cp], 4);
            self->headerData2[cp] = 0;
            cp += 4;
            self->refSeq[i].length = rlen;
        }
        ksort((void *)rs_by_name, self->refSeqs, sizeof(rs_by_name[0]), comp_RefSeqName, self);
    }
    if (self->headerData1) {
        rc = CountReadGroups(self->headerData1, hlen, &self->readGroups);
        if (rc == 0) {
            self->readGroup = calloc(self->readGroups, sizeof(self->readGroup[0]));
            if (self->readGroup != NULL)
                rc = ParseHeader(self, self->headerData1, hlen, rs_by_name);
            else
                rc = RC(rcAlign, rcFile, rcConstructing, rcMemory, rcExhausted);
        }
    }
    if (rs_by_name != NULL)
        free((void *)rs_by_name);
    
    return rc;
}

static rc_t BAMIndexWhack(const BAMIndex *);

static rc_t BAMFileWhack(BAMFile *self) {
    if (self->refSeq)
        free(self->refSeq);
    if (self->readGroup)
        free(self->readGroup);
    if (self->header)
        free((void *)self->header);
    if (self->headerData1)
        free((void *)self->headerData1);
    if (self->headerData2)
        free((void *)self->headerData2);
    if (self->ndx)
        BAMIndexWhack(self->ndx);
    if (self->vt.FileWhack)
        self->vt.FileWhack(&self->file);

    return 0;
}

/* file is retained */
static rc_t BAMFileMakeWithKFileAndHeader(BAMFile const **cself,
                                          KFile const *file,
                                          char const *headerText,
                                          bool threaded)
{
    BAMFile *self = calloc(1, sizeof(*self));
    rc_t rc;
    
    if (self == NULL)
        return RC(rcAlign, rcFile, rcConstructing, rcMemory, rcExhausted);
    
    atomic32_set(&self->refcount, 1);
#ifndef WINDOWS
    if (threaded)
        rc = BGZThreadFileInit(&self->file.thread, file, &self->vt);
    else
#endif
        rc = BGZFileInit(&self->file.plain, file, &self->vt);

    if (rc == 0) {
        rc = ProcessHeader(self, headerText);
        if (rc == 0) {
            *cself = self;
            return 0;
        }
    }
    BAMFileWhack(self);
    return rc;
}

/* file is retained */
LIB_EXPORT rc_t CC BAMFileMakeWithKFile(const BAMFile **cself, const KFile *file)
{
    return BAMFileMakeWithKFileAndHeader(cself, file, NULL, false);
}

LIB_EXPORT rc_t CC BAMFileVMakeWithDir(const BAMFile **result,
                                         const KDirectory *dir,
                                         const char *path,
                                         va_list args
                                         )
{
    rc_t rc;
    const KFile *kf;
    
    if (result == NULL)
        return RC(rcAlign, rcFile, rcOpening, rcParam, rcNull);
    *result = NULL;
    rc = KDirectoryVOpenFileRead(dir, &kf, path, args);
    if (rc == 0) {
        rc = BAMFileMakeWithKFile(result, kf);
        KFileRelease(kf);
    }
    return rc;
}

LIB_EXPORT rc_t CC BAMFileMakeWithDir(const BAMFile **result,
                                        const KDirectory *dir,
                                        const char *path, ...
                                        )
{
    va_list args;
    rc_t rc;
    
    va_start(args, path);
    rc = BAMFileVMakeWithDir(result, dir, path, args);
    va_end(args);
    return rc;
}

LIB_EXPORT rc_t CC BAMFileMake(const BAMFile **cself, const char *path, ...)
{
    KDirectory *dir;
    va_list args;
    rc_t rc;
    
    if (cself == NULL)
        return RC(rcAlign, rcFile, rcOpening, rcParam, rcNull);
    *cself = NULL;
    
    rc = KDirectoryNativeDir(&dir);
    if (rc) return rc;
    va_start(args, path);
    rc = BAMFileVMakeWithDir(cself, dir, path, args);
    va_end(args);
    KDirectoryRelease(dir);
    return rc;
}

LIB_EXPORT rc_t CC BAMFileMakeWithHeader ( const BAMFile **cself,
                                          char const headerText[],
                                          char const path[], ... )
{
    KDirectory *dir;
    va_list args;
    rc_t rc;
    const KFile *kf;
    
    if (cself == NULL)
        return RC(rcAlign, rcFile, rcOpening, rcParam, rcNull);
    *cself = NULL;
    
    rc = KDirectoryNativeDir(&dir);
    if (rc) return rc;
    va_start(args, path);
    rc = KDirectoryVOpenFileRead(dir, &kf, path, args);
    if (rc == 0) {
        rc = BAMFileMakeWithKFileAndHeader(cself, kf, headerText, false);
        KFileRelease(kf);
    }
    va_end(args);
    KDirectoryRelease(dir);
    return rc;
}


LIB_EXPORT rc_t CC BAMFileMakeWithVPath(const BAMFile **cself, const VPath *kpath)
{
    char path[4096];
    size_t nread;
    rc_t rc;

    rc = VPathReadPath(kpath, path, sizeof(path), &nread);
    if (rc == 0)
        rc = BAMFileMake(cself, "%.*s", (int)nread, path);
    return rc;
}

LIB_EXPORT rc_t CC BAMFileAddRef(const BAMFile *cself) {
    if (cself != NULL)
        atomic32_inc(&((BAMFile *)cself)->refcount);
    return 0;
}

LIB_EXPORT rc_t CC BAMFileRelease(const BAMFile *cself) {
    rc_t rc = 0;
    BAMFile *self = (BAMFile *)cself;
    
    if (cself != NULL) {
        if (atomic32_dec_and_test(&self->refcount)) {
            rc = BAMFileWhack(self);
            if (rc)
                atomic32_set(&self->refcount, 1);
            else
                free(self);
        }
    }
    return rc;
}

LIB_EXPORT float CC BAMFileGetProportionalPosition(const BAMFile *self)
{
    return self->vt.FileProPos(&self->file);
}

LIB_EXPORT rc_t CC BAMFileGetPosition(const BAMFile *self, BAMFilePosition *pos) {
    *pos = (self->fpos_cur << 16) | self->bufCurrent;
    return 0;
}

static rc_t BAMFileSetPositionInt(const BAMFile *cself, uint64_t fpos, uint16_t bpos)
{
    rc_t rc;
    BAMFile *self = (BAMFile *)cself;
    
    if (cself->fpos_first > fpos || fpos > cself->vt.FileGetSize(&cself->file) ||
        (fpos == cself->fpos_first && bpos < cself->ucfirst))
    {
        return RC(rcAlign, rcFile, rcPositioning, rcParam, rcInvalid);
    }
    if (cself->fpos_cur == fpos) {
        if (bpos <= cself->bufSize) {
            self->eof = false;
            self->bufCurrent = bpos;
            return 0;
        }
        return RC(rcAlign, rcFile, rcPositioning, rcParam, rcInvalid);
    }
    rc = self->vt.FileSetPos(&self->file, fpos);
    if (rc == 0) {
        self->eof = false;
        self->bufSize = 0; /* force re-read */
        self->bufCurrent = bpos;
        self->fpos_cur = fpos;
    }
    return rc;
}

LIB_EXPORT rc_t CC BAMFileSetPosition(const BAMFile *cself, const BAMFilePosition *pos)
{
    return BAMFileSetPositionInt(cself, *pos >> 16, (uint16_t)(*pos));
}

LIB_EXPORT rc_t CC BAMFileRewind(const BAMFile *cself)
{
    return BAMFileSetPositionInt(cself, cself->fpos_first, cself->ucfirst);
}

typedef bool (* i_OptData_f)(void *ctx, char const tag[2], BAMOptDataValueType type, unsigned length,
                             unsigned count, unsigned size, void const *value);

static rc_t ParseOptData(uint8_t const data[], unsigned const n, void *ctx, i_OptData_f f)
{
    unsigned i = 0;
    rc_t rc = 0;
    
    while (rc == 0 && i + 3 < n) {
        char const *const tag = (char const *)&data[i];
        int type = data[i + 2];
        uint8_t const *const vp = &data[i + 3];
        unsigned len = 0;
        unsigned size = 0;
        unsigned count = 1;
        unsigned offset = 0;
        
        switch (type) {
        case dt_CSTRING:
        case dt_HEXSTRING:
            len = 0;
            while (i + len + 3 != n && vp[len] != '\0')
                ++len;
            if (i + len + 3 == n) {
                rc = RC(rcAlign, rcFile, rcReading, rcData, rcInvalid);
                break;
            }
            size = len;
            ++len;
            break;
        case dt_INT8:
        case dt_UINT8:
        case dt_ASCII:
            size = len = 1;
            break;
        case dt_INT16:
        case dt_UINT16:
            size = len = 2;
            break;
        case dt_INT:
        case dt_FLOAT32:
        case dt_UINT:
            size = len = 4;
            break;
        case dt_FLOAT64:
            size = len = 8;
            break;
        case dt_NUM_ARRAY:
            offset = len = 5;
            {
                unsigned elem_size = 0;
                uint32_t elem_count = 0;
                
                switch (vp[0]) {
                case dt_INT8:
                case dt_UINT8:
                    elem_size = 1;
                    break;
                case dt_INT16:
                case dt_UINT16:
                    elem_size = 2;
                    break;
                case dt_FLOAT32:
                case dt_INT:
                case dt_UINT:
                    elem_size = 4;
                    break;
                case dt_FLOAT64:
                    elem_size = 8;
                    break;
                default:
                    rc = RC(rcAlign, rcFile, rcReading, rcData, rcUnexpected);
                    break;
                }
                if (rc)
                    break;
                elem_count = LE2HUI32(&vp[1]);
                len += elem_size * elem_count;
                if (i + len + 3 > n) {
                    rc = RC(rcAlign, rcFile, rcReading, rcData, rcInvalid);
                    break;
                }
                type = vp[0];
                count = elem_count;
                size = elem_size;
            }
            break;
        default:
            rc = RC(rcAlign, rcFile, rcReading, rcData, rcUnexpected);
            break;
        }
        if (rc)
            break;
        i += len + 3;
        if (f(ctx, tag, type, len + 3, count, size, &vp[offset]))
            break;
    }
    return rc;
}

static bool CountOptTags(void *ctx, char const tag[2], BAMOptDataValueType type, unsigned length,
                         unsigned count, unsigned size, void const *value)
{
    ++*(unsigned *)ctx;
    return false;
}

struct ctx_LoadOptTags_s {
    unsigned i;
    BAMAlignment *self;
};

static bool LoadOptTags(void *Ctx, char const tag[2], BAMOptDataValueType type, unsigned length,
                         unsigned count, unsigned size, void const *value)
{
    struct ctx_LoadOptTags_s *ctx = Ctx;
    BAMAlignment *self = ctx->self;
    
    self->extra[ctx->i].offset = (uint8_t const *)&tag[0] - (uint8_t const *)&self->data->raw[0];
    self->extra[ctx->i].size = length;
    ++ctx->i;
    return false;
}

LIB_EXPORT rc_t CC BAMFileRead(const BAMFile *cself, const BAMAlignment **rhs)
{
    BAMFile *self = (BAMFile *)cself;
    BAMAlignment *y;
    BAMAlignment x;
    struct ctx_LoadOptTags_s load_ctx;
    bool local = false;
    int32_t i32;
    rc_t rc;
    unsigned xtra;
    
    *rhs = NULL;

    if (cself == NULL)
        return RC(rcAlign, rcFile, rcReading, rcParam, rcNull);
    
    if (cself->bufCurrent >= cself->bufSize && cself->eof)
        return RC(rcAlign, rcFile, rcReading, rcRow, rcNotFound);

    if (self->bufLocker != NULL) {
        if (self->bufLocker->storage == NULL)
            self->bufLocker->storage = malloc(self->bufLocker->datasize);
        if (self->bufLocker->storage == NULL)
            return RC(rcAlign, rcFile, rcReading, rcMemory, rcExhausted);
        memcpy(self->bufLocker->storage, self->bufLocker->data, self->bufLocker->datasize);
        self->bufLocker->data = (bam_alignment *)&self->bufLocker->storage[0];
        self->bufLocker = NULL;
    }
    
    rc = BAMFileReadI32(self, &i32);
    if (rc) {
        if (GetRCObject(rc) == rcData && GetRCState(rc) == rcInsufficient) {
            self->eof = true;
            rc = RC(rcAlign, rcFile, rcReading, rcRow, rcNotFound);
        }
        return rc;
    }
    if (i32 <= 0)
        return RC(rcAlign, rcFile, rcReading, rcData, rcInvalid);
    
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("BAM record: { size: %i }\n", i32));
    memset(&x, 0, sizeof(x));
    x.datasize = i32;
    
    if (self->bufCurrent + x.datasize <= self->bufSize) {
        x.data = (bam_alignment *)&self->buffer[self->bufCurrent];
        local = true;
    }
    else {
        x.storage = malloc(x.datasize);
        if (x.storage == NULL)
            return RC(rcAlign, rcFile, rcReading, rcMemory, rcExhausted);

        rc = BAMFileReadn(self, x.datasize, x.storage);
        if (rc) {
            free(x.storage);
            return rc;
        }
        x.data = (bam_alignment *)x.storage;
    }
    {{
        unsigned const nameLen = getReadNameLength(&x);
        unsigned const cigCnt  = getCigarCount(&x);
        unsigned const readLen = getReadLen(&x);
        
        DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("BAM record: { name length: %u, cigar count: %u, read length %u }\n", nameLen, cigCnt, readLen));
        x.cigar = sizeof(x.data[0]) + nameLen - 1;
        x.seq   = x.cigar + 4 * cigCnt;
        x.qual  = x.seq + (readLen + 1) / 2;
        xtra    = x.qual + readLen;
    }}
    if (   x.cigar >= x.datasize
        || x.seq   >  x.datasize
        || x.qual  >  x.datasize
        || xtra    >  x.datasize
        || (rc = ParseOptData(&x.data->raw[xtra], x.datasize - xtra, &x.numExtra, CountOptTags)) != 0)
    {
	rc = RC(rcAlign, rcFile, rcReading, rcRow, rcInvalid);
        free(x.storage);
        return rc;
    }
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("BAM record: { number of extra fields: %u }\n", x.numExtra));
    y = calloc(sizeof(*y) + (x.numExtra ? x.numExtra - 1 : 0) * sizeof(y->extra), 1);
    if (y == NULL) {
        free(x.storage);
        return RC(rcAlign, rcFile, rcReading, rcMemory, rcExhausted);
    }
    *y = x;
    load_ctx.self = y;
    load_ctx.i = 0;
    if ((rc = ParseOptData(&y->data->raw[xtra], y->datasize - xtra, &load_ctx, LoadOptTags)) != 0) {
        free(x.storage);
        free(y);
        return rc;
    }
    ksort(y->extra, y->numExtra, sizeof(y->extra[0]), OptTag_sort, y);

    BAMFileAddRef(y->parent = self);
    atomic32_set(&y->refcount, 1);
    if (local) {
        self->bufLocker = y;
        self->bufCurrent += y->datasize;
        if (self->bufCurrent == self->bufSize) {
            self->fpos_cur = self->vt.FileGetPos(&self->file);
            self->bufCurrent = 0;
            self->bufSize = 0;
        }
    }
    
    *rhs = y;
    return 0;
}

LIB_EXPORT rc_t CC BAMFileGetRefSeqById(const BAMFile *cself, int32_t id, const BAMRefSeq **rhs)
{
    *rhs = NULL;
    if (id >= 0 && id < cself->refSeqs)
        *rhs = &cself->refSeq[id];
    return 0;
}

LIB_EXPORT rc_t CC BAMFileGetReadGroupByName(const BAMFile *cself, const char *name, const BAMReadGroup **rhs)
{
    BAMReadGroup rg;
    
    *rhs = NULL;

    rg.name = name;
    if (rg.name != NULL)
        *rhs = kbsearch(&rg, cself->readGroup, cself->readGroups, sizeof(rg), comp_ReadGroup, NULL);

    return 0;
}

LIB_EXPORT rc_t CC BAMFileGetRefSeqCount(const BAMFile *cself, unsigned *rhs)
{
    *rhs = cself->refSeqs;
    return 0;
}

LIB_EXPORT rc_t CC BAMFileGetRefSeq(const BAMFile *cself, unsigned i, const BAMRefSeq **rhs)
{
    *rhs = NULL;
    if (i < cself->refSeqs)
        *rhs = &cself->refSeq[i];
    return 0;
}

LIB_EXPORT rc_t CC BAMFileGetReadGroupCount(const BAMFile *cself, unsigned *rhs)
{
    *rhs = cself->readGroups;
    return 0;
}

LIB_EXPORT rc_t CC BAMFileGetReadGroup(const BAMFile *cself, unsigned i, const BAMReadGroup **rhs)
{
    *rhs = NULL;
    if (i < cself->readGroups)
        *rhs = &cself->readGroup[i];
    return 0;
}

LIB_EXPORT rc_t CC BAMFileGetHeaderText(BAMFile const *cself, char const **header, size_t *header_len)
{
    *header = cself->header;
    *header_len = *header ? strlen(*header) : 0;
    return 0;
}

/* MARK: BAM Alignment Stuff */

LIB_EXPORT rc_t CC BAMAlignmentAddRef(const BAMAlignment *cself)
{
    if (cself != NULL)
        atomic32_inc(&((BAMAlignment *)cself)->refcount);
    return 0;
}

static rc_t BAMAlignmentWhack(BAMAlignment *self)
{
    if (self->parent->bufLocker == self)
        self->parent->bufLocker = NULL;
    BAMFileRelease(self->parent);
    free(self->storage);
    free(self);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentRelease(const BAMAlignment *cself)
{
    if (cself != NULL) {
        BAMAlignment *self = (BAMAlignment *)cself;
        
        if (atomic32_dec_and_test(&self->refcount))
            BAMAlignmentWhack(self);
    }
    return 0;
}

#if 0
LIB_EXPORT uint16_t CC BAMAlignmentIffyFields(const BAMAlignment *self)
{
}

LIB_EXPORT uint16_t CC BAMAlignmentBadFields(const BAMAlignment *self)
{
}
#endif

static uint32_t BAMAlignmentGetCigarElement(const BAMAlignment *self, unsigned i)
{
    return LE2HUI32(&((uint8_t const *)getCigarBase(self))[i * 4]);
}

LIB_EXPORT rc_t CC BAMAlignmentGetRefSeqId(const BAMAlignment *cself, int32_t *rhs)
{
    *rhs = getRefSeqId(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetPosition(const BAMAlignment *cself, int64_t *rhs)
{
    *rhs = getPosition(cself);
    return 0;
}

LIB_EXPORT bool CC BAMAlignmentIsMapped(const BAMAlignment *cself)
{
    if (((getFlags(cself) & BAMFlags_SelfIsUnmapped) == 0) && getRefSeqId(cself) >= 0 && getPosition(cself) >= 0)
        return true;
    return false;
}

LIB_EXPORT rc_t CC BAMAlignmentGetAlignmentDetail(
                                                  const BAMAlignment *self,
                                                  BAMAlignmentDetail *rslt, uint32_t count, uint32_t *actual,
                                                  int32_t *pfirst, int32_t *plast
                                                  )
{
    unsigned i;
    unsigned ccnt; /* cigar count */
    int32_t  gpos; /* refSeq pos in global coordinates */
    unsigned rpos; /* read pos (always local coordinates) */
    uint32_t rlen; /* read length */
    int32_t first = -1;
    int32_t last = -1;

    if (!self)
        return RC(rcAlign, rcFile, rcReading, rcSelf, rcNull);

    rlen = getReadLen(self);
    ccnt = getCigarCount(self);
    gpos = getPosition(self);
    
    if (gpos < 0)
        ccnt = 0;
    
    if (actual)
        *actual = ccnt;
    
    if (pfirst)
        *pfirst = -1;

    if (plast)
        *plast = -1;

    if (ccnt == 0)
        return 0;
    
    if (rslt == NULL) {
        if (actual == NULL)
            return RC(rcAlign, rcFile, rcReading, rcParam, rcNull);
        count = 0;
    }
    
    if (count < ccnt)
        return RC(rcAlign, rcFile, rcReading, rcBuffer, rcInsufficient);
        
    for (rpos = 0, i = 0; i != ccnt; ++i) {
        uint32_t len = BAMAlignmentGetCigarElement(self, i);
        int op = len & 0x0F;
        
        if (op > sizeof(cigarChars))
            return RC(rcAlign, rcFile, rcReading, rcData, rcInvalid);
        
        op = cigarChars[op];
        len >>= 4;
        
        rslt[i].refSeq_pos = gpos;
        rslt[i].read_pos = rpos;
        rslt[i].length = len;
        rslt[i].type = (BAMCigarType)op;
        
        switch ((BAMCigarType)op) {
        case ct_Match:
        case ct_Equal:
            if (first == -1)
                first = i;
            last = i;
            gpos += len;
            rpos += len;
            break;
        case ct_Insert:
        case ct_SoftClip:
            gpos += len;
            break;
        case ct_Delete:
        case ct_Skip:
            rpos += len;
            break;
        case ct_HardClip:
        case ct_Padded:
            rslt[i].refSeq_pos = -1;
            rslt[i].read_pos = -1;
            break;
        default:
            break;
        }
        
        if (rslt[i].read_pos > rlen)
            return RC(rcAlign, rcFile, rcReading, rcData, rcInvalid);
    }
    if (pfirst)
        *pfirst = first;
    
    if (plast)
        *plast = last;
    
    return 0;
}

static
unsigned ReferenceLengthFromCIGAR(const BAMAlignment *self)
{
    unsigned i;
    unsigned n = getCigarCount(self);
    unsigned y;
    
    for (i = 0, y = 0; i != n; ++i) {
        uint32_t const len = BAMAlignmentGetCigarElement(self, i);
        
        switch (cigarChars[len & 0x0F]) {
        case ct_Match:
        case ct_Equal:
        case ct_NotEqual:
        case ct_Delete:
        case ct_Skip:
            y += len >> 4;
            break;
        default:
            break;
        }
    }
    return y;
}

static
unsigned SequenceLengthFromCIGAR(const BAMAlignment *self)
{
    unsigned i;
    unsigned n = getCigarCount(self);
    unsigned y;
    
    for (i = 0, y = 0; i != n; ++i) {
        uint32_t const len = BAMAlignmentGetCigarElement(self, i);
        
        switch (cigarChars[len & 0x0F]) {
        case ct_Match:
        case ct_Equal:
        case ct_NotEqual:
        case ct_Insert:
        case ct_SoftClip:
            y += len >> 4;
            break;
        default:
            break;
        }
    }
    return y;
}

LIB_EXPORT rc_t CC BAMAlignmentGetPosition2(const BAMAlignment *cself, int64_t *rhs, uint32_t *length)
{
    *rhs = getPosition(cself);
    if (*rhs >= 0)
        *length = ReferenceLengthFromCIGAR(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetReadGroupName(const BAMAlignment *cself, const char **rhs)
{
    *rhs = get_RG(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetReadName(const BAMAlignment *cself, const char **rhs)
{
    *rhs = getReadName(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetReadName2(const BAMAlignment *cself, const char **rhs, size_t *length)
{
    *length = getReadNameLength(cself) - 1;
    *rhs = getReadName(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetFlags(const BAMAlignment *cself, uint16_t *rhs)
{
    *rhs = getFlags(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetMapQuality(const BAMAlignment *cself, uint8_t *rhs)
{
    *rhs = getMapQual(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetCigarCount(const BAMAlignment *cself, unsigned *rhs)
{
    *rhs = getCigarCount(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetRawCigar(const BAMAlignment *cself, uint32_t const *rslt[], uint32_t *length)
{
    *rslt = getCigarBase(cself);
    *length = getCigarCount(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetCigar(const BAMAlignment *cself, uint32_t i, BAMCigarType *type, uint32_t *length)
{
    uint32_t x;
    
    if (i >= getCigarCount(cself))
        return RC(rcAlign, rcFile, rcReading, rcParam, rcInvalid);

    x = BAMAlignmentGetCigarElement(cself, i);
    *type = (BAMCigarType)(cigarChars[x & 0x0F]);
    *length = x >> 4;
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetReadLength(const BAMAlignment *cself, uint32_t *rhs)
{
    *rhs = getReadLen(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetSequence2(const BAMAlignment *cself, char *rhs, uint32_t start, uint32_t stop)
{
    /*
     *   =    A    C    M    G    R    S    V    T    W    Y    H    K    D    B    N
     * 0000 0001 0010 0011 0100 0101 0110 0111 1000 1001 1010 1011 1100 1101 1110 1111
     * 1111 1000 0100 1100 0010 1010 0110 1110 0001 1001 0101 1101 0011 1011 0111 0000
     *   N    T    G    K    C    Y    S    B    A    W    R    D    M    H    V    =
     */
    static const char  tr[16] = "=ACMGRSVTWYHKDBN";
 /* static const char ctr[16] = "=TGKCYSBAWRDMHVN"; */
    unsigned const n = getReadLen(cself);
    const uint8_t * const seq = &cself->data->raw[cself->seq];
    unsigned si, di;
    
    if (stop == 0 || stop > n)
        stop = n;
    
    for (di = 0, si = start; si != stop; ++si, ++di) {
        unsigned const b4na2 = seq[si >> 1];
        unsigned const b4na = (si & 1) == 0 ? (b4na2 >> 4) : (b4na2 & 0x0F);
        
        rhs[di] = tr[b4na];
    }
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetSequence(const BAMAlignment *cself, char *rhs)
{
    return BAMAlignmentGetSequence2(cself, rhs, 0, 0);
}

LIB_EXPORT bool CC BAMAlignmentHasColorSpace(BAMAlignment const *cself)
{
    return get_CS(cself) != NULL;
}

LIB_EXPORT rc_t CC BAMAlignmentGetCSKey(BAMAlignment const *cself, char rhs[1])
{
    char const *const vCS = get_CS(cself);
    
    if (vCS)
        rhs[0] = vCS[0];
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetCSSeqLen(BAMAlignment const *cself, uint32_t *const rhs)
{
    struct offset_size_s const *const vCS = get_CS_info(cself);
    
    *rhs = vCS ? vCS->size - 5 : 0;
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetCSSequence(BAMAlignment const *cself, char rhs[], uint32_t const seqlen)
{
    char const *const vCS = get_CS(cself);
    
    if (vCS) {
        unsigned i;
        
        for (i = 0;i != seqlen; ++i) {
            char const ch = vCS[i+1];
            
            rhs[i] = (ch == '4') ? '.' : ch;
        }
    }
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetQuality(const BAMAlignment *cself, const uint8_t **rhs)
{
    *rhs = &cself->data->raw[cself->qual];
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetQuality2(BAMAlignment const *cself, uint8_t const **rhs, uint8_t *offset)
{
    uint8_t const *const OQ = get_OQ(cself);
    
    if (OQ) {
        struct offset_size_s const *const oq = get_OQ_info(cself);
        
        if (oq->size - 4 == getReadLen(cself)) {
            *offset = 33;
            *rhs = OQ;
        }
        else
            return RC(rcAlign, rcRow, rcReading, rcData, rcInconsistent);
    }
    else {
        *offset = 0;
        *rhs = &cself->data->raw[cself->qual];
    }
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetCSQuality(BAMAlignment const *cself, uint8_t const **rhs, uint8_t *offset)
{
    struct offset_size_s const *const cs = get_CS_info(cself);
    struct offset_size_s const *const cq = get_CQ_info(cself);
    uint8_t const *const CQ = get_CQ(cself);
    
    if (cs && cq && CQ) {
        if (cs->size == cq->size) {
            *offset = 33;
            *rhs = CQ + 1;
            return 0;
        }
        if (cs->size == cq->size + 1) {
            *offset = 33;
            *rhs = CQ;
            return 0;
        }
        return RC(rcAlign, rcRow, rcReading, rcData, rcInconsistent);
    }
    *offset = 0;
    *rhs = &cself->data->raw[cself->qual];
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetMateRefSeqId(const BAMAlignment *cself, int32_t *rhs)
{
    *rhs = getMateRefSeqId(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetMatePosition(const BAMAlignment *cself, int64_t *rhs)
{
    *rhs = getMatePos(cself);
    return 0;
}

LIB_EXPORT rc_t CC BAMAlignmentGetInsertSize(const BAMAlignment *cself, int64_t *rhs)
{
    *rhs = getInsertSize(cself);
    return 0;
}

typedef struct OptForEach_ctx_s {
    BAMOptData *val;
    BAMOptData **alloced;
    size_t valsize;
    rc_t rc;
    BAMOptionalDataFunction user_f;
    void *user_ctx;
} OptForEach_ctx_t;

static bool i_OptDataForEach(BAMAlignment const *cself, void *Ctx, char const tag[2], BAMOptDataValueType type, unsigned count, void const *value, unsigned size)
{
    OptForEach_ctx_t *ctx = (OptForEach_ctx_t *)Ctx;
    size_t const need = (size_t)&((BAMOptData const *)NULL)->u.f64[(count * size + sizeof(double) - 1)/sizeof(double)];
    
    if (need > ctx->valsize) {
        void *const temp = realloc(ctx->alloced, need);
        if (temp == NULL) {
            ctx->rc = RC(rcAlign, rcFile, rcReading, rcMemory, rcExhausted);
            return true;
        }
        *ctx->alloced = ctx->val = temp;
        ctx->valsize = need;
    }
    ctx->val->type = type;
    ctx->val->element_count = (type == dt_CSTRING || type == dt_HEXSTRING) ? size - 1 : count;
    
    memcpy(ctx->val->u.u8, value, size * count);
#if __BYTE_ORDER == __BIG_ENDIAN
    {{
        unsigned di;
	uint32_t elem_count = ctx->val->element_count;
        
        switch (size) {
        case 2:
            for (di = 0; di != elem_count; ++di)
                ctx->val->u.u16[di] = LE2HUI16(&ctx->val->u.u16[di]);
            break;
        case 4:
            for (di = 0; di != elem_count; ++di)
                ctx->val->u.u32[di] = LE2HUI32(&ctx->val->u.u32[di]);
            break;
        case 8:
            for (di = 0; di != elem_count; ++di)
                ctx->val->u.u64[di] = LE2HUI64(&ctx->val->u.u64[di]);
            break;
        }
    }}
#endif
    ctx->rc = ctx->user_f(ctx->user_ctx, tag, ctx->val);
    return ctx->rc != 0;
}

LIB_EXPORT rc_t CC BAMAlignmentOptDataForEach(const BAMAlignment *cself, void *user_ctx, BAMOptionalDataFunction f)
{
    union u {
        BAMOptData value;
        uint8_t storage[4096];
    } value_auto;
    OptForEach_ctx_t ctx;
    rc_t rc;
    unsigned i;
    
    ctx.val = &value_auto.value;
    ctx.alloced = NULL;
    ctx.valsize = sizeof(value_auto);
    ctx.rc = 0;
    ctx.user_f = f;
    ctx.user_ctx = user_ctx;
    
    for (i = 0; i != cself->numExtra; ++i) {
        char const *const tag = (char const *)&cself->data->raw[cself->extra[i].offset];
        uint8_t type = tag[2];
        uint8_t const *const vp = (uint8_t const *)&tag[3];
        unsigned len = cself->extra[i].size - 3;
        unsigned size = cself->extra[i].size - 3;
        unsigned count = 1;
        unsigned offset = 0;
        
        if (type == dt_NUM_ARRAY) {
            unsigned elem_size = 0;
            uint32_t elem_count = 0;
            
            offset = len = 5;
            switch (vp[0]) {
            case dt_INT8:
            case dt_UINT8:
                elem_size = 1;
                break;
            case dt_INT16:
            case dt_UINT16:
                elem_size = 2;
                break;
            case dt_FLOAT32:
            case dt_INT:
            case dt_UINT:
                elem_size = 4;
                break;
            case dt_FLOAT64:
                elem_size = 8;
                break;
            default:
                rc = RC(rcAlign, rcFile, rcReading, rcData, rcUnexpected);
                break;
            }
            if (rc)
                break;
            elem_count = LE2HUI32(&vp[1]);
            len += elem_size * elem_count;
            type = vp[0];
            count = elem_count;
            size = elem_size;
            break;
        }
        if (i_OptDataForEach(cself, &ctx, tag, type, count, &vp[offset], size))
            break;
    }
    rc = ctx.rc;
    if (ctx.alloced)
        free(ctx.alloced);
    return rc;
}

LIB_EXPORT bool CC BAMAlignmentHasCGData(BAMAlignment const *self)
{
    return get_CG_GC_info(self) && get_CG_GS_info(self) && get_CG_GQ_info(self);
}

static bool BAMAlignmentParseCGTag(BAMAlignment const *self,uint32_t *cg_segs,uint32_t max_cg_segs)
{
/*** patern in cg_segs should be nSnGnSnG - no more then 7 segments **/
    struct offset_size_s const *const GCi = get_CG_GC_info(self);
    char *cg  = (char *) &self->data->raw[GCi->offset + 3];
    char *end = cg + GCi->size -4;
    /** init**/
    int iseg=0;
    char last_op='S';
    cg_segs[iseg] = 0;

    while(cg < end && iseg < max_cg_segs){
        int op_len = strtol(cg,&cg,10);
        char op = *cg;
        cg++;
        if(op==last_op){
                cg_segs[iseg] += op_len;
        } else{
                last_op = op;
                iseg++;
                cg_segs[iseg] = op_len;
        }
    }
    for(iseg+=1;iseg < max_cg_segs;iseg ++){
        cg_segs[iseg] = 0;
    }
    return true;
}

static
rc_t ExtractInt32(BAMAlignment const *self, int32_t *result,
                  struct offset_size_s const *const tag)
{
    int32_t y;
    
    switch (self->data->raw[tag->offset + 2]) {
    case 'c':
        if (tag->size == 4)
            y = ((int8_t const *)self->data->raw)[tag->offset + 3];
        else
            return RC(rcAlign, rcRow, rcReading, rcData, rcInvalid);
        break;
    case 'C':
        if (tag->size == 4)
            y = ((uint8_t const *)self->data->raw)[tag->offset + 3];
        else
            return RC(rcAlign, rcRow, rcReading, rcData, rcInvalid);
        break;
    case 's':
        if (tag->size == 5)
            y = LE2HI16(self->data->raw + tag->offset + 3);
        else
            return RC(rcAlign, rcRow, rcReading, rcData, rcInvalid);
        break;
    case 'S':
        if (tag->size == 5)
            y = LE2HUI16(self->data->raw + tag->offset + 3);
        else
            return RC(rcAlign, rcRow, rcReading, rcData, rcInvalid);
        break;
    case 'i':
        if (tag->size == 7)
            y = LE2HI32(self->data->raw + tag->offset + 3);
        else
            return RC(rcAlign, rcRow, rcReading, rcData, rcInvalid);
        break;
    case 'I':
        if (tag->size == 7)
            y = LE2HUI32(self->data->raw + tag->offset + 3);
        else
            return RC(rcAlign, rcRow, rcReading, rcData, rcInvalid);
        break;
    default:
        return RC(rcAlign, rcRow, rcReading, rcData, rcNotFound);
    }
    *result = y;
    return 0;
}

LIB_EXPORT
rc_t CC BAMAlignmentGetCGAlignGroup(BAMAlignment const *self,
                                    char buffer[],
                                    size_t max_size,
                                    size_t *act_size)
{
    struct offset_size_s const *const ZA = get_CG_ZA_info(self);
    struct offset_size_s const *const ZI = get_CG_ZI_info(self);
    
    if (ZA && ZI) {
        rc_t rc;
        int32_t za;
        int32_t zi;
        
        rc = ExtractInt32(self, &za, ZA); if (rc) return rc;
        rc = ExtractInt32(self, &zi, ZI); if (rc) return rc;
        return string_printf(buffer, max_size, act_size, "%i_%i", zi, za);
    }
    return RC(rcAlign, rcRow, rcReading, rcData, rcNotFound);
}

LIB_EXPORT
rc_t CC BAMAlignmentGetCGSeqQual(BAMAlignment const *self,
                                 char sequence[],
                                 uint8_t quality[])
{
    struct offset_size_s const *const GCi = get_CG_GC_info(self);
    struct offset_size_s const *const GSi = get_CG_GS_info(self);
    struct offset_size_s const *const GQi = get_CG_GQ_info(self);

    if (GCi && GSi && GQi) {
        char const *const vGS = (char const *)&self->data->raw[GSi->offset + 3];
        char const *const GQ = (char const *)&self->data->raw[GQi->offset + 3];
        unsigned const GSsize = GSi->size - 4;
        unsigned const sn = getReadLen(self);
        uint32_t cg_segs[2*CG_NUM_SEGS-1]; /** 4 segments + 3gaps **/
        unsigned i,G,S;

        if (GSi->size != GQi->size)
            return RC(rcAlign, rcRow, rcReading, rcData, rcInvalid);
        if (SequenceLengthFromCIGAR(self) != sn)
            return RC(rcAlign, rcRow, rcReading, rcData, rcInvalid);
        if (!BAMAlignmentParseCGTag(self, cg_segs,2*CG_NUM_SEGS-1))
            return RC(rcAlign, rcRow, rcReading, rcData, rcInvalid);

        for(S=cg_segs[0],G=0,i=1;i<CG_NUM_SEGS;i++){ /** sum all S and G **/
                S += cg_segs[2*i];
                G += cg_segs[2*i-1];
        }
        if (G + G != GSsize || S + G > sn || sn + G != 35) {
            /*fprintf(stderr, "GSsize: %u; sn: %u; S: %u; G: %u\n", GSsize, sn, S, G);*/
            return RC(rcAlign, rcRow, rcReading, rcData, rcInvalid);
        }
        if(G > 0){
                int nsi=cg_segs[0];/** new index into sequence */
                int osi=nsi+G;     /** old index into sequence */
                int k;             /** index into inserted sequence **/
                /***make room for inserts **/
                memmove(sequence + osi, sequence + nsi, sn - nsi);
                memmove(quality  + osi, quality  + nsi, sn - nsi);
                for(i=1,k=0;i<CG_NUM_SEGS && nsi < osi;i++){/*** when osi and nsi meet we are done ***/
                        int j;
                        for(j = cg_segs[2*i-1];j>0;j--){/** insert mode **/
                                sequence[nsi] = vGS[k];
                                quality [nsi] = GQ[k] - 33;
                                nsi++;k++;
                                sequence[nsi] = vGS[k];
                                quality [nsi] = GQ[k] - 33;
                                nsi++;k++;
                                osi++;
                        }
                        if(nsi < osi){
                                for(j=cg_segs[2*i];j>0;j--){/** copy mode **/
                                        sequence[nsi] = sequence[osi];
                                        quality[nsi]  = quality[osi];
                                        nsi++;osi++;
                                }
                        }
                }
        }
        return 0;
    }
    return RC(rcAlign, rcRow, rcReading, rcData, rcNotFound);
}


static unsigned splice(uint32_t cigar[], unsigned n, unsigned at, unsigned out, unsigned in, uint32_t const new_values[/* in */])
{
    assert(at + out <= n);
    memmove(&cigar[at + in], &cigar[at + out], (n - at - out) * 4);
    if (in)
        memcpy(&cigar[at], new_values, in * 4);
    return n + in - out;
}

#define OPCODE_2_FIX (0xF)

static unsigned insert_B(unsigned S, unsigned G, uint32_t cigar[], unsigned n)
{
    unsigned i;
    unsigned pos;
    unsigned T=S+G;
    
    for (pos = i = 0; i < n; ++i) {
        unsigned const opcode = cigar[i] & 0xF;
        
        switch (opcode) {
        case 0:
        case 1:
        case 4:
        case 7:
        case 8:
            {{
                unsigned const len = cigar[i] >> 4;
                unsigned const nxt = pos + len;
                
                if (pos <= T && T <= nxt) {
                    unsigned const l = T - pos;
                    unsigned const r = len - l;
                    unsigned B = i + 2;
                    unsigned in = 4;
                    uint32_t Ops[4];
                    uint32_t *ops = Ops;
                    
                    Ops[0] = (l << 4) | opcode;
                    Ops[1] = (G << 4) | 9; /* B */
                    Ops[2] = (G << 4) | 0; /* M this is not backwards */
                    Ops[3] = (r << 4) | opcode;
                    
                    if (r == 0)
                        --in;
                    if (l == 0) {
                        ++ops;
                        --in;
                        --B;
                    }
                    n = splice(cigar, n, i, 1, in, ops);
                    return n;;
                }
                pos = nxt;
            }}
            break;
        default:
            break;
        }
    }
    return n;
}

static unsigned fix_I(uint32_t cigar[], unsigned n)
{
    unsigned i;
    int last_b = 0;
    
    for (i = 0; i < n; ++i) {
        unsigned const opcode = cigar[i] & 0xF;
        
        if (opcode == 0xF) {
            unsigned const oplen = cigar[i] >> 4;
            uint32_t ops[2];
            
            if (0/*last_b*/) {
                ops[0] = (oplen << 4) | 0; /* M */
                ops[1] = (oplen << 4) | 9; /* B */
            }
            else {
                ops[0] = (oplen << 4) | 9; /* B */
                ops[1] = (oplen << 4) | 0; /* M */
            }
            
            n = splice(cigar, n, i, 1, 2, ops);
            ++i;
        }
        else if (opcode == 9)
            last_b = 1;
        else
            last_b = 0;
    }
    return n;
}

static unsigned fix_IN(uint32_t cigar[], unsigned n)
{
    unsigned i;
    
    for (i = 1; i < n; ++i) {
        unsigned const opL = cigar[i-1] & 0xF;
        unsigned const opI = cigar[ i ] & 0xF;
        
        if (opL == 1 && opI == 3) {
            unsigned const oplen = cigar[i-1] >> 4;
            uint32_t ops[2];
            
            ops[0] = (oplen << 4) | 9; /* B */
            ops[1] = (oplen << 4) | 0; /* M */
            
            n = splice(cigar, n, i-1, 1, 2, ops);
            ++i;
        }
        else if (opL == 3 && opI == 1) {
            unsigned const oplen = cigar[i] >> 4;
            uint32_t ops[2];
            
            ops[0] = (oplen << 4) | 9; /* M */
            ops[1] = (oplen << 4) | 0; /* B */
            
            n = splice(cigar, n, i, 1, 2, ops);
            ++i;
        }
    }
    return n;
}

static unsigned canonicalize(uint32_t cigar[], unsigned n)
{
    unsigned i;
    
    for (i = n; i > 0; ) {
        --i;
        if (cigar[i] >> 4 == 0 || (cigar[i] & 0xF) == 6)
            n = splice(cigar, n, i, 1, 0, NULL);
    }
    for (i = 1; i < n; ) {
        unsigned const opL = cigar[i-1] & 0xF;
        unsigned const opI = cigar[ i ] & 0xF;
        
        if (opI == opL) {
            unsigned const oplen = (cigar[i] >> 4) + (cigar[i-1] >> 4);
            uint32_t const op = (oplen << 4) | opI;

            n = splice(cigar, n, i-1, 2, 1, &op);
        }
        else
            ++i;
    }
#if 0
    if ((cigar[0] & 0xF) == 1)
        cigar[0] = (cigar[0] & ~(uint32_t)0xF) | 4; /* I -> S */
    if ((cigar[n - 1] & 0xF) == 1)
        cigar[n - 1] = (cigar[n - 1] & ~(uint32_t)0xF) | 4; /* I -> S */
#endif
    return n;
}

static void reverse(uint32_t cigar[], unsigned n)
{
    unsigned i;
    unsigned j;
    
    for (j = n - 1, i = 0; i < j; ++i, --j) {
        uint32_t const tmp = cigar[i];
        cigar[i] = cigar[j];
        cigar[j] = tmp;
    }
}

LIB_EXPORT
rc_t CC BAMAlignmentGetCGCigar(BAMAlignment const *self,
                               uint32_t *cigar,
                               uint32_t cig_max,
                               uint32_t *cig_act)
{
    struct offset_size_s const *const GCi = get_CG_GC_info(self);

    *cig_act = 0;

    if (GCi) {
        uint32_t i,G,S;
        unsigned n = getCigarCount(self);
        uint32_t cg_segs[2*CG_NUM_SEGS-1]; /** 4 segments + 3gaps **/

        if (!BAMAlignmentParseCGTag(self, cg_segs, 2*CG_NUM_SEGS-1))
            return RC(rcAlign, rcRow, rcReading, rcData, rcInvalid);
        if (cig_max < n + 5)
            return RC(rcAlign, rcRow, rcReading, rcBuffer, rcInsufficient);

        memcpy(cigar, getCigarBase(self), n * 4);
        n = canonicalize(cigar, n); /* just in case */
        for(i=0,S=0; i< CG_NUM_SEGS-1;i++){
                S+=cg_segs[2*i];
                G=cg_segs[2*i+1];
                if(G > 0){
                        n = insert_B(S, G, cigar, n);
                        S+=G;
                }
        }
        *cig_act = n;
        return 0;
    }
    return RC(rcAlign, rcRow, rcReading, rcData, rcNotFound);
}


LIB_EXPORT rc_t BAMAlignmentGetTI(BAMAlignment const *self, uint64_t *ti)
{
    char const *const TI = get_XT(self);
    long long unsigned temp;
    
    if (TI && sscanf(TI, "ti|%llu", &temp) == 1) {
        *ti = (uint64_t)temp;
        return 0;
    }
    return RC(rcAlign, rcRow, rcReading, rcData, rcNotFound);
}

/* MARK: BAMIndex stuff */

static uint64_t get_pos(uint8_t const buf[])
{
    return LE2HUI64(buf);
}

#define MAX_BIN 37449
static uint16_t bin2ival(uint16_t bin)
{
    if (bin < 1)
        return 0; /* (bin - 0) << 15; */
    
    if (bin < 9)
        return (bin - 1) << 12;
    
    if (bin < 73)
        return (bin - 9) << 9;
    
    if (bin < 585)
        return (bin - 73) << 6;
    
    if (bin < 4681)
        return (bin - 585) << 3;
    
    if (bin < 37449)
        return (bin - 4681) << 0;
    
    return 0;
}

static uint16_t bin_ival_count(uint16_t bin)
{
    if (bin < 1)
        return 1 << 15;
    
    if (bin < 9)
        return 1 << 12;
    
    if (bin < 73)
        return 1 << 9;
    
    if (bin < 585)
        return 1 << 6;
    
    if (bin < 4681)
        return 1 << 3;
    
    if (bin < 37449)
        return 1;
    
    return 0;
}

enum BAMIndexStructureTypes {
    bai_StartStopPairs,
    bai_16kIntervals
};

typedef rc_t (*WalkIndexStructureCallBack)(const uint8_t data[], size_t dlen,
                                           unsigned refNo,
                                           unsigned refs,
                                           enum BAMIndexStructureTypes type,
                                           unsigned binNo,
                                           unsigned bins,
                                           unsigned elements,
                                           void *ctx);

static
rc_t WalkIndexStructure(uint8_t const buf[], size_t const blen,
                        WalkIndexStructureCallBack func,
                        void *ctx
                        )
{
    unsigned cp = 0;
    int32_t nrefs;
    unsigned i;
    rc_t rc;
    
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("Index data length: %u", blen));

    if (cp + 4 > blen)
        return RC(rcAlign, rcIndex, rcReading, rcData, rcInsufficient);
    
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("Index signature: '%c%c%c%u'", buf[cp+0], buf[cp+1], buf[cp+2], buf[cp+3]));
    if (memcmp(buf + cp, "BAI\1", 4) != 0)
        return RC(rcAlign, rcIndex, rcReading, rcFormat, rcUnknown);
    
    cp += 4;
    if (cp + 4 > blen)
        return RC(rcAlign, rcIndex, rcReading, rcData, rcInsufficient);

    nrefs = LE2HI32(buf + cp); cp += 4;
    DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("Index reference count: %i", nrefs));
    
    if (nrefs == 0)
        return RC(rcAlign, rcIndex, rcReading, rcData, rcEmpty);
    
    for (i = 0; i < nrefs; ++i) {
        int32_t bins;
        int32_t chunks;
        int32_t intervals;
        unsigned di;
        
        DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("Index reference %u: starts at %u", i, cp));
        if (cp + 4 > blen)
            return RC(rcAlign, rcIndex, rcReading, rcData, rcInsufficient);
        
        bins = LE2HI32(buf + cp); cp += 4;
        DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("Index reference %u: %i bins", i, nrefs));

        for (di = 0; di < bins; ++di) {
            uint32_t binNo;
            
            if (cp + 8 > blen)
                return RC(rcAlign, rcIndex, rcReading, rcData, rcInsufficient);

            binNo = LE2HUI32(buf + cp); cp += 4;
            chunks = LE2HI32(buf + cp); cp += 4;
            DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("Index reference %u, bin %u: %i chunks", i, binNo, chunks));
            
            if (cp + 16 * chunks > blen)
                return RC(rcAlign, rcIndex, rcReading, rcData, rcInsufficient);
            rc = func(&buf[cp], 16 * chunks, i, nrefs, bai_StartStopPairs, binNo, bins, chunks, ctx);
            if (rc)
                return rc;
            cp += 16 * chunks;
        }
        if (cp + 4 > blen)
            return RC(rcAlign, rcIndex, rcReading, rcData, rcInsufficient);

        intervals = LE2HI32(buf + cp); cp += 4;
        DBGMSG(DBG_ALIGN, DBG_FLAG(DBG_ALIGN_BAM), ("Index reference %u: %i intervals", i, intervals));

        if (cp + 8 * intervals > blen)
            return RC(rcAlign, rcIndex, rcReading, rcData, rcInsufficient);
        rc = func(&buf[cp], 8 * intervals, i, nrefs, bai_16kIntervals, ~(unsigned)0, bins, intervals, ctx);
        if (rc)
            return rc;
        cp += 8 * intervals;
    }
    if (cp > blen)
        return RC(rcAlign, rcIndex, rcReading, rcData, rcInsufficient);
    return 0;
}

struct LoadIndex1_s {
    const BAMFile *self;
    int refNo;
    unsigned refs;
    unsigned intervals;
    unsigned total_interval_count;
};

static
rc_t LoadIndex1(const uint8_t data[], size_t dlen, unsigned refNo,
                unsigned refs, enum BAMIndexStructureTypes type,
                unsigned binNo, unsigned bins,
                unsigned elements, void *Ctx)
{
    struct LoadIndex1_s *ctx = (struct LoadIndex1_s *)Ctx;
    
    ctx->refs = refs;
    if (refNo != ctx->refNo) {
        ctx->total_interval_count += ctx->intervals;
        ctx->intervals = 0;
        ctx->refNo = refNo;
    }
    if (elements != 0) {
        if (refNo > ctx->self->refSeqs)
            return RC(rcAlign, rcIndex, rcReading, rcData, rcInvalid);
        ctx->intervals = (ctx->self->refSeq[refNo].length + 16383) >> 14;
        if (type == bai_16kIntervals && elements > ctx->intervals)
            return RC(rcAlign, rcIndex, rcReading, rcData, rcExcessive);
    }
    return 0;
}

struct LoadIndex2_s {
    const BAMFile *self;
    BAMFilePosition **refSeq;
    BAMFilePosition *cur;
#if _DEBUGGING
    BAMFilePosition *end;
#endif
    const uint8_t *base;
    unsigned bins[MAX_BIN + 1];
    bool hasData;
};

static
rc_t LoadIndex2a(const uint8_t data[], size_t dlen, unsigned refNo,
                 unsigned refs, enum BAMIndexStructureTypes type,
                 unsigned binNo, unsigned bins,
                 unsigned elements, struct LoadIndex2_s *ctx)
{
    const unsigned max_ival = (ctx->self->refSeq[refNo].length + 16383) >> 14;
    unsigned i;
    unsigned cp;
    unsigned k;
    uint32_t chunk_count;
    uint64_t minOffset[1u << 15];

    assert(ctx->refSeq[refNo] == NULL);
    ctx->refSeq[refNo] = ctx->cur;
    ctx->cur += max_ival;
    
#if _DEBUGGING
    assert(refNo < ctx->self->refSeqs);
    assert(ctx->cur <= ctx->end);
    assert(elements <= max_ival);
#endif
    /* get the positions of the first records in the 16kbp intervals */
    for (cp = i = 0; i != elements; ++i, cp += 8)
        ctx->refSeq[refNo][i] = get_pos(&data[cp]);
    /* get the positions of the first records in the 16kbp bins */
    for (i = MAX_BIN; i != 0; ) {
        const unsigned ival = bin2ival(--i);
        const unsigned n_ival = bin_ival_count(i);
        uint64_t found;
        
        cp = ctx->bins[i];
        if (cp == 0)
            continue;
        if (n_ival > 1)
            break;
        
        assert(i == LE2HI32(ctx->base + cp));
        cp += 4;
        chunk_count = LE2HI32(ctx->base + cp); cp += 4;
        found = ctx->refSeq[refNo][ival];
        for (k = 0; k < chunk_count; ++k) {
            const uint64_t start = get_pos(ctx->base + cp);
            
            cp += 16;
            if (found == 0 || start < found)
                found = start;
        }
        ctx->refSeq[refNo][ival] = found;
    }
    /* The interval list now contains the offsets to the first alignment
     * that starts at or after the interval's starting position.
     * An interval's starting position is 16kpb * interval number.
     *
     * We will now use the information from the bigger bins to find the
     * offsets of the first chunk of alignments that ends after an
     * interval's first alignment.
     */
    memset(minOffset, 0, sizeof(minOffset));
    for (i = 0; i != MAX_BIN; ++i) {
        const unsigned ival = bin2ival(i);
        unsigned n_ival = bin_ival_count(i);
        
        cp = ctx->bins[i];
        if (cp == 0)
            continue;
        if (n_ival <= 1)
            break;
        
        if (ival + n_ival > max_ival)
            n_ival = max_ival - ival;
        
        chunk_count = LE2HI32(ctx->base + cp + 4); cp += 8;
        for (k = 0; k < chunk_count; ++k) {
            const uint64_t start = get_pos(ctx->base + cp);
            const uint64_t end   = get_pos(ctx->base + cp + 8);
            unsigned l;
            
            cp += 16;
            for (l = 0; l != n_ival; ++l) {
                if (start < ctx->refSeq[refNo][ival + l] &&
                    ctx->refSeq[refNo][ival + l] <= end &&
                    (start < minOffset[ival + l] ||
                     minOffset[ival + l] == 0
                     )
                    )
                {
                    minOffset[ival + l] = start;
                }
            }
        }
    }
    /* update the intervals to the new earlier offsets if any */
    for (i = 0; i != max_ival; ++i) {
        if (minOffset[i] != 0)
            ctx->refSeq[refNo][i] = minOffset[i];
    }
    memset(ctx->bins, 0, sizeof(ctx->bins));
    ctx->hasData = false;
    return 0;
}

static
rc_t LoadIndex2(const uint8_t data[], size_t dlen, unsigned refNo,
                unsigned refs, enum BAMIndexStructureTypes type,
                unsigned binNo, unsigned bins,
                unsigned elements, void *Ctx)
{
    struct LoadIndex2_s *ctx = (struct LoadIndex2_s *)Ctx;
    
    if (type == bai_StartStopPairs) {
        if (binNo < MAX_BIN && elements != 0) {
            ctx->bins[binNo] = &data[-8] - ctx->base;
            ctx->hasData = true;
        }
    }
    else if (elements != 0 || ctx->hasData)
        return LoadIndex2a(data, dlen, refNo, refs, type, binNo, bins,
                           elements, (struct LoadIndex2_s *)Ctx);
    return 0;
}    

static
rc_t LoadIndex(BAMFile *self, const uint8_t buf[], size_t blen)
{
    BAMIndex *idx;
    rc_t rc;
    struct LoadIndex1_s loadIndex1ctx;
    unsigned const posArray = ((uintptr_t)&((const BAMFilePosition **)(NULL))[self->refSeqs]) / sizeof(BAMFilePosition *);

    memset(&loadIndex1ctx, 0, sizeof(loadIndex1ctx));
    loadIndex1ctx.refNo = -1;
    loadIndex1ctx.self = self;
    
    rc = WalkIndexStructure(buf, blen, LoadIndex1, &loadIndex1ctx);
    if (rc == 0) {
        loadIndex1ctx.total_interval_count += loadIndex1ctx.intervals;
        idx = calloc(1, posArray * sizeof(BAMFilePosition *) +
                     loadIndex1ctx.total_interval_count * sizeof(BAMFilePosition));
        if (idx == NULL)
            rc = RC(rcAlign, rcIndex, rcReading, rcMemory, rcExhausted);
        else {
            struct LoadIndex2_s *loadIndex2ctx;
            
            if (self->ndx)
                BAMIndexWhack(self->ndx);
            self->ndx = idx;
            
            loadIndex2ctx = malloc(sizeof(*loadIndex2ctx));
            if (loadIndex2ctx == NULL) {
                rc = RC(rcAlign, rcIndex, rcReading, rcMemory, rcExhausted);
                free(idx);
                self->ndx = NULL;
            }
            else {
                memset(loadIndex2ctx->bins, 0, sizeof(loadIndex2ctx->bins));
                loadIndex2ctx->self = self;
                loadIndex2ctx->refSeq = &idx->refSeq[0];
                loadIndex2ctx->base = buf;
                loadIndex2ctx->hasData = false;
                loadIndex2ctx->cur = (BAMFilePosition *)&idx->refSeq[posArray];
#if _DEBUGGING
                loadIndex2ctx->end = loadIndex2ctx->cur + loadIndex1ctx.total_interval_count;
#endif
                
                WalkIndexStructure(buf, blen, LoadIndex2, loadIndex2ctx);
                free(loadIndex2ctx);
            }
        }
    }
    return rc;
}

static
rc_t BAMFileOpenIndexInternal(const BAMFile *self, const char *path)
{
    const KFile *kf;
    rc_t rc;
    size_t fsize;
    uint8_t *buf;
    KDirectory *dir;
    
    rc = KDirectoryNativeDir(&dir);
    if (rc) return rc;
    rc = KDirectoryOpenFileRead(dir, &kf, path);
    KDirectoryRelease(dir);
    if (rc) return rc;
    {
        uint64_t u64;

        rc = KFileSize(kf, &u64);
        if (sizeof(size_t) < sizeof(u64) && (size_t)u64 != u64) {
            KFileRelease(kf);
            return RC(rcAlign, rcIndex, rcReading, rcData, rcExcessive);
        }
        fsize = u64;
    }
    if (rc == 0) {
        buf = malloc(fsize);
        if (buf != NULL) {
            size_t nread;
            
            rc = KFileRead(kf, 0, buf, fsize, &nread);
            KFileRelease(kf);
            if (rc == 0) {
                if (nread == fsize) {
                    rc = LoadIndex((BAMFile *)self, buf, nread);
                    free(buf);
                    return rc;
                }
                rc = RC(rcAlign, rcIndex, rcReading, rcData, rcInvalid);
            }
            free(buf);
        }
        else
            rc = RC(rcAlign, rcIndex, rcReading, rcMemory, rcExhausted);
    }
    return rc;
}

LIB_EXPORT rc_t CC BAMFileOpenIndex(const BAMFile *self, const char *path)
{
    return BAMFileOpenIndexInternal(self, path);
}

LIB_EXPORT rc_t CC BAMFileOpenIndexWithVPath(const BAMFile *self, const VPath *kpath)
{
    char path[4096];
    size_t nread;
    rc_t rc = VPathReadPath(kpath, path, sizeof(path), &nread);

    if (rc == 0) {
        path[nread] = '\0';
        rc = BAMFileOpenIndexInternal(self, path);
    }
    return rc;
}

LIB_EXPORT bool CC BAMFileIsIndexed(const BAMFile *self)
{
	if (self && self->ndx)
		return true;
	return false;
}

LIB_EXPORT bool CC BAMFileIndexHasRefSeqId(const BAMFile *self, uint32_t refSeqId)
{
	if (self && self->ndx && self->ndx->refSeq[refSeqId])
		return true;
	return false;
}

static rc_t BAMFileGetAlignPos(const BAMFile *self, int64_t *beg, int64_t *end, int32_t *refSeq)
{
    const BAMAlignment *check;
    rc_t rc;
    
    rc = BAMFileRead(self, &check);
    if (rc)
        return rc;
    BAMAlignmentGetPosition(check, beg);
    BAMAlignmentGetRefSeqId(check, refSeq);
    *end = *beg + ReferenceLengthFromCIGAR(check);
    BAMAlignmentRelease(check);
    return rc;
}

LIB_EXPORT rc_t CC BAMFileSeek(const BAMFile *self, uint32_t refSeqId, uint64_t alignStart, uint64_t alignEnd)
{
    BAMFilePosition rpos = 0;
    rc_t rc;
    int64_t prev_alignPos;
    int64_t alignPos;
    int64_t alignEndPos;
    int32_t refSeq;
    
    if (self->ndx == NULL)
        return RC(rcAlign, rcFile, rcPositioning, rcIndex, rcNotFound);
    if (refSeqId >= self->refSeqs)
        return RC(rcAlign, rcFile, rcPositioning, rcData, rcNotFound);
    if (self->ndx->refSeq[refSeqId] == NULL)
        return RC(rcAlign, rcFile, rcPositioning, rcData, rcNotFound);
    if (alignStart >= self->refSeq[refSeqId].length)
        return RC(rcAlign, rcFile, rcPositioning, rcData, rcNotFound);
    if (alignEnd > self->refSeq[refSeqId].length)
        alignEnd = self->refSeq[refSeqId].length;
    
    {{
        unsigned adjust = 0;
        uint32_t ival_start = (uint32_t)(alignStart >> 14);
        uint32_t ival_end = (uint32_t)((alignEnd + 16383) >> 14);
        
        /* find the first interval >= alignStart that has an alignment */
        while (ival_start != ival_end && (rpos = self->ndx->refSeq[refSeqId][ival_start]) == 0)
            ++ival_start;
        
        if (rpos == 0)
            return RC(rcAlign, rcFile, rcPositioning, rcData, rcNotFound);
        do {
            rc = BAMFileSetPosition(self, &rpos);
            if (rc == 0)
                rc = BAMFileGetAlignPos(self, &alignPos, &alignEndPos, &refSeq);
            if (rc)
                return RC(rcAlign, rcFile, rcPositioning, rcIndex, rcInvalid);
            if (refSeq != refSeqId)
                return RC(rcAlign, rcFile, rcPositioning, rcData, rcNotFound);
            if (alignPos <= alignEnd)
                break; /* we found the interval we were looking for */
            
            /* we over-shot */
            if (++adjust >= ival_start)
                return RC(rcAlign, rcFile, rcPositioning, rcData, rcNotFound);
            if ((rpos = self->ndx->refSeq[refSeqId][ival_start - adjust]) == 0)
                return RC(rcAlign, rcFile, rcPositioning, rcData, rcNotFound);
        } while (1);
    }}
    prev_alignPos = alignPos;
    
    do {
        if (alignPos > alignEnd)
            return RC(rcAlign, rcFile, rcPositioning, rcData, rcNotFound);
        
        /* if the alignment overlaps the target range then we are done */
        if (alignPos >= alignStart || alignEndPos >= alignStart)
            return BAMFileSetPosition(self, &rpos);
        
        /* start linear scan */
        rc = BAMFileGetPosition(self, &rpos);
        if (rc)
            return rc;
        rc = BAMFileGetAlignPos(self, &alignPos, &alignEndPos, &refSeq);
        if (rc) return rc;
        if (refSeq != refSeqId)
            return RC(rcAlign, rcFile, rcPositioning, rcData, rcNotFound);
        
        /*  indexed BAM must be sorted by position
         *  so verify that we are not out of order
         *  whether this means that the index is bad
         *  or the file is bad, likely both
         *  fix the file and regenerate the index
         */
        if (prev_alignPos > alignPos)
            return RC(rcAlign, rcFile, rcPositioning, rcData, rcInvalid);
        prev_alignPos = alignPos;
    } while (1);
}

static rc_t BAMIndexWhack(const BAMIndex *cself) {
    free((void *)cself);
    return 0;
}

/* MARK: BAM Validation Stuff */

static rc_t OpenVPathRead(const KFile **fp, struct VPath const *path)
{
    char buffer[4096];
    size_t blen;
    rc_t rc = VPathReadPath(path, buffer, sizeof(buffer), &blen);
    
    if (rc == 0) {
        KDirectory *dir;
        
        rc = KDirectoryNativeDir(&dir);
        if (rc == 0) {
            rc = KDirectoryOpenFileRead(dir, fp, "%.*s", (int)blen, buffer);
            KDirectoryRelease(dir);
        }
    }
    return rc;
}

static rc_t ReadVPath(void **data, size_t *dsize, struct VPath const *path)
{
    const KFile *fp;
    rc_t rc = OpenVPathRead(&fp, path);
    
    if (rc == 0) {
        uint8_t *buff;
        uint64_t fsz;
        size_t bsz = 0;
        
        rc = KFileSize(fp, &fsz);
        if (rc == 0) {
            if ((size_t)fsz != fsz)
                return RC(rcAlign, rcFile, rcReading, rcFile, rcTooBig);
            buff = malloc(fsz);
            if (buff == NULL)
                return RC(rcAlign, rcFile, rcReading, rcMemory, rcExhausted);
            do {
                size_t nread;
                
                rc = KFileRead(fp, 0, buff + bsz, fsz - bsz, &nread);
                if (rc)
                    break;
                bsz += nread;
            } while (bsz < (size_t)fsz);
            if (rc == 0) {
                *data = buff;
                *dsize = bsz;
                return 0;
            }
            free(buff);
        }
    }
    return rc;
}

static rc_t VPath2BGZF(BGZFile *bgzf, struct VPath const *path)
{
    const KFile *fp;
    BGZFile_vt dummy;
    rc_t rc = OpenVPathRead(&fp, path);
    
    if (rc == 0) {
        rc = BGZFileInit(bgzf, fp, &dummy);
        KFileRelease(fp);
    }
    return rc;
}

struct index_data {
    uint64_t position;
    unsigned refNo;
    unsigned binNo;
    bool found;
};

struct buffer_data {
    uint64_t position;
    size_t size;
};

typedef struct BAMValidate_ctx_s BAMValidate_ctx_t;
struct BAMValidate_ctx_s {
    BAMValidateCallback callback;
    void *ctx;
    BAMValidateStats *stats;
    const uint8_t *bai;
    int32_t *refLen;
    struct index_data *position;
    uint8_t *buf;
    uint8_t *nxt;
    size_t bsize;
    size_t alloced;
    size_t dnext;
    uint32_t options;
    uint32_t lastRefId;
    uint32_t lastRefPos;
    unsigned npositions;
    unsigned mpositions;
    unsigned nrefs;
    bool cancelled;
};

static
rc_t IndexValidateStructure(const uint8_t data[], size_t dlen,
                            unsigned refNo,
                            unsigned refs,
                            enum BAMIndexStructureTypes type,
                            unsigned binNo,
                            unsigned bins,
                            unsigned elements,
                            void *Ctx)
{
    BAMValidate_ctx_t *ctx = Ctx;
    rc_t rc = 0;
    
    ctx->stats->baiFilePosition = data - ctx->bai;
    rc = ctx->callback(ctx->ctx, 0, ctx->stats);
    if (rc)
        ctx->cancelled = true;
    return rc;
}

static int CC comp_index_data(const void *A, const void *B, void *ignored)
{
    const struct index_data *a = A;
    const struct index_data *b = B;
    
    if (a->position < b->position)
        return -1;
    else if (a->position > b->position)
        return 1;
    else
        return 0;
}

static
rc_t BAMValidateLoadIndex(const uint8_t data[], size_t dlen,
                          unsigned refNo,
                          unsigned refs,
                          enum BAMIndexStructureTypes type,
                          unsigned binNo,
                          unsigned bins,
                          unsigned elements,
                          void *Ctx)
{
    BAMValidate_ctx_t *ctx = Ctx;
    unsigned const n = type == bai_16kIntervals ? elements : elements * 2;
    unsigned i;
    unsigned j;
    
    if (type == bai_StartStopPairs && binNo >= MAX_BIN)
        return 0;
    
    if (ctx->npositions + elements > ctx->mpositions) {
        void *temp;
        
        do { ctx->mpositions <<= 1; } while (ctx->npositions + elements > ctx->mpositions);
        temp = realloc(ctx->position, ctx->mpositions * sizeof(ctx->position[0]));
        if (temp == NULL)
            return RC(rcAlign, rcIndex, rcReading, rcMemory, rcExhausted);
        ctx->position = temp;
    }
    for (j = i = 0; i != n; ++i) {
        uint64_t const pos = get_pos(&data[i * 8]);
        
        if (type == bai_StartStopPairs && (i & 1) != 0)
            continue;
        
        if (pos) {
            ctx->position[ctx->npositions + j].refNo = refNo;
            ctx->position[ctx->npositions + j].binNo = binNo;
            ctx->position[ctx->npositions + j].position = pos;
            ++j;
        }
    }
    ctx->npositions += j;
    return 0;
}

static
rc_t BAMValidateHeader(const uint8_t data[],
                       unsigned dsize,
                       unsigned *header_len,
                       unsigned *refs_start,
                       unsigned *nrefs,
                       unsigned *data_start
                       )
{
    int32_t hlen;
    int32_t refs;
    unsigned i;
    unsigned cp;
    
    if (dsize < 8)
        return RC(rcAlign, rcFile, rcValidating, rcData, rcIncomplete);
    
    if (memcmp(data, "BAM\1", 4) != 0)
        return RC(rcAlign, rcFile, rcValidating, rcFormat, rcUnrecognized);
    
    hlen = LE2HI32(&data[4]);
    if (hlen < 0)
        return RC(rcAlign, rcFile, rcValidating, rcData, rcInvalid);
    
    if (dsize < hlen + 12)
        return RC(rcAlign, rcFile, rcValidating, rcData, rcIncomplete);
    
    refs = LE2HI32(&data[hlen + 8]);
    if (refs < 0)
        return RC(rcAlign, rcFile, rcValidating, rcData, rcInvalid);
    
    for (cp = hlen + 12, i = 0; i != refs; ++i) {
        int32_t nlen;
        
        if (dsize < cp + 4)
            return RC(rcAlign, rcFile, rcValidating, rcData, rcIncomplete);
        
        nlen = LE2HI32(&data[cp]);
        if (nlen < 0)
            return RC(rcAlign, rcFile, rcValidating, rcData, rcInvalid);
        
        if (dsize < cp + nlen + 4)
            return RC(rcAlign, rcFile, rcValidating, rcData, rcIncomplete);
        
        cp += nlen + 4;
    }
    
    *nrefs = refs;
    *refs_start = 12 + (*header_len = hlen);
    *data_start = cp;
    return 0;
}

static rc_t BAMValidateIndex(struct VPath const *bampath,
                             struct VPath const *baipath,
                             BAMValidateOption options,
                             BAMValidateCallback callback,
                             void *callbackContext
                             )
{
    rc_t rc = 0;
    BGZFile bam;
    uint8_t *bai = NULL;
    size_t bai_size;
    BAMValidateStats stats;
    BAMValidate_ctx_t ctx;
    uint8_t data[2 * ZLIB_BLOCK_SIZE];
    uint32_t dsize = 0;
    uint64_t pos = 0;
    uint32_t temp;
    int32_t ref = -1;
    int32_t rpos = -1;
    
    if ((options & bvo_IndexOptions) == 0)
        return callback(callbackContext, 0, &stats);

    rc = ReadVPath((void **)&bai, &bai_size, baipath);
    if (rc)
        return rc;
    
    memset(&stats, 0, sizeof(stats));
    memset(&ctx, 0, sizeof(ctx));
    
    ctx.bai = bai;
    ctx.stats = &stats;
    ctx.options = options;
    ctx.ctx = callbackContext;
    ctx.callback = callback;
    
    if ((options & bvo_IndexOptions) == bvo_IndexStructure)
        return WalkIndexStructure(bai, bai_size, IndexValidateStructure, &ctx);

    rc = VPath2BGZF(&bam, bampath);
    if (rc == 0) {
        ctx.mpositions = 1024 * 32;
        ctx.position = malloc(ctx.mpositions * sizeof(ctx.position[0]));
        if (ctx.position == NULL)
            return RC(rcAlign, rcIndex, rcReading, rcMemory, rcExhausted);
        
        rc = WalkIndexStructure(bai, bai_size, BAMValidateLoadIndex, &ctx);
        free(bai);
        if (rc) {
            stats.indexStructureIsBad = true;
            rc = callback(callbackContext, rc, &stats);
        }
        else {
            unsigned i = 0;
            
            stats.indexStructureIsGood = true;
            stats.baiFileSize = ctx.npositions;
            
            ksort(ctx.position, ctx.npositions, sizeof(ctx.position[0]), comp_index_data, 0);
            
            stats.bamFileSize = bam.fsize;
            
            while (i < ctx.npositions) {
                uint64_t const ifpos = ctx.position[i].position >> 16;
                uint16_t const bpos = (uint16_t)ctx.position[i].position;
                
                stats.baiFilePosition = i;
                if (i == 0 || ifpos != pos) {
                    stats.bamFilePosition = pos = ifpos;
                    rc = BGZFileSetPos(&bam, pos);
                    if (rc == 0)
                        rc = BGZFileRead(&bam, data, &dsize);
                    if (rc) {
                        ++stats.indexFileOffset.error;
                        do {
                            ++i;
                            if (i == ctx.npositions)
                                break;
                            if (ctx.position[i].position >> 16 != pos)
                                break;
                            ++stats.indexFileOffset.error;
                        } while (1);
                    }
                    else
                        ++stats.indexFileOffset.good;

                    rc = callback(callbackContext, rc, &stats);
                    if (rc)
                        break;
                }
                else
                    ++stats.indexFileOffset.good;
                if ((options & bvo_IndexOptions) > bvo_IndexOffsets1) {
                    int32_t rsize = 0;
                    BAMAlignment algn;
                    
                    if (bpos >= dsize)
                        goto BAD_BLOCK_OFFSET;
                    if (dsize - bpos < 4) {
                    READ_MORE:
                        if (dsize > ZLIB_BLOCK_SIZE)
                            goto BAD_BLOCK_OFFSET;

                        rc = BGZFileRead(&bam, data + dsize, &temp);
                        if (rc) {
                            ++stats.blockCompression.error;
                            goto BAD_BLOCK_OFFSET;
                        }
                        dsize += temp;
                        if (dsize - bpos < 4 || dsize - bpos < rsize)
                            goto BAD_BLOCK_OFFSET;
                    }
                    rsize = LE2HI32(data + bpos);
                    if (rsize <= 0)
                        goto BAD_BLOCK_OFFSET;
                    if (rsize > 0xFFFF) {
                        ++stats.indexBlockOffset.warning;
                        ++i;
                        continue;
                    }
                    if (dsize - bpos < rsize)
                        goto READ_MORE;
/*                    rc = BAMAlignmentParse(&algn, data + bpos + 4, rsize); */
                    if (rc)
                        goto BAD_BLOCK_OFFSET;
                    ++stats.indexBlockOffset.good;
                    if ((options & bvo_IndexOptions) > bvo_IndexOffsets2) {
                        int32_t const refSeqId = getRefSeqId(&algn);
                        uint16_t const binNo = getBin(&algn);
                        int32_t const position = getPosition(&algn);
                        
                        if (ctx.position[i].refNo == refSeqId &&
                            (ctx.position[i].binNo == binNo ||
                             ctx.position[i].binNo == ~((unsigned)0)
                        ))
                            ++stats.indexBin.good;
                        else if (ctx.position[i].refNo == refSeqId)
                            ++stats.indexBin.warning;
                        else
                            ++stats.indexBin.error;
                        
                        if (refSeqId < ref || position < rpos)
                            ++stats.inOrder.error;
                        
                        ref = refSeqId;
                        rpos = position;
                    }
                }
                if (0) {
                BAD_BLOCK_OFFSET:
                    ++stats.indexBlockOffset.error;
                }
                ++i;
            }
        }
        
        free(ctx.position);
        BGZFileWhack(&bam);
    }
    stats.bamFilePosition = stats.bamFileSize;
    return callback(callbackContext, rc, &stats);
}

static rc_t BAMValidate3(BAMValidate_ctx_t *ctx,
                         BAMAlignment const *algn
                         )
{
    rc_t rc = 0;
    uint16_t const flags = getFlags(algn);
    int32_t const refSeqId = getRefSeqId(algn);
    int32_t const refPos = getPosition(algn);
    unsigned const mapQ = getMapQual(algn);
    bool const aligned =
        ((flags & BAMFlags_SelfIsUnmapped) == 0) && 
        (refSeqId >= 0) && (refSeqId < ctx->nrefs) &&
        (refPos >= 0) && (refPos < ctx->refLen[refSeqId]) && (mapQ > 0);
    
    if (ctx->options & bvo_ExtraFields) {
    }
    if (aligned) {
        if ((ctx->options & bvo_Sorted) != 0) {
            if (ctx->lastRefId < refSeqId || (ctx->lastRefId == refSeqId && ctx->lastRefPos <= refPos))
                ++ctx->stats->inOrder.good;
            else
                ++ctx->stats->inOrder.error;
            ctx->lastRefId = refSeqId;
            ctx->lastRefPos = refPos;
        }
        if (ctx->options & bvo_CIGARConsistency) {
        }
        if (ctx->options & bvo_BinConsistency) {
        }
    }
    if (ctx->options & bvo_FlagsConsistency) {
    }
    if (ctx->options & bvo_QualityValues) {
    }
    if (ctx->options & bvo_MissingSequence) {
    }
    if (ctx->options & bvo_MissingQuality) {
    }
    if (ctx->options & bvo_FlagsStats) {
    }
    return rc;
}

static rc_t BAMValidate2(void *Ctx, const BGZFile *file,
                         rc_t rc, uint64_t fpos,
                         const zlib_block_t data, unsigned dsize)
{
    BAMValidate_ctx_t *ctx = Ctx;
    rc_t rc2;
    bool fatal = false;
    
    ctx->stats->bamFilePosition = fpos;
    if (rc) {
        if (ctx->options == bvo_BlockHeaders)
            ++ctx->stats->blockHeaders.error;
        else
            ++ctx->stats->blockCompression.error;
    }
    else if (ctx->options == bvo_BlockHeaders) {
        ++ctx->stats->blockHeaders.good;
    }
    else if (ctx->options == bvo_BlockCompression) {
        ++ctx->stats->blockHeaders.good;
        ++ctx->stats->blockCompression.good;
    }
    else if (dsize) {
        ctx->bsize += dsize;
        if (!ctx->stats->bamHeaderIsBad && !ctx->stats->bamHeaderIsGood) {
            unsigned header_len;
            unsigned refs_start;
            unsigned nrefs;
            unsigned data_start;
            
            rc2 = BAMValidateHeader(ctx->buf, ctx->bsize,
                                       &header_len, &refs_start,
                                       &nrefs, &data_start);
            
            if (rc2 == 0) {
                ctx->stats->bamHeaderIsGood = true;
                if (ctx->options & bvo_BinConsistency) {
                    ctx->refLen = malloc(nrefs * sizeof(ctx->refLen[0]));
                    if (ctx->refLen == NULL) {
                        rc = RC(rcAlign, rcFile, rcValidating, rcMemory, rcExhausted);
                        fatal = true;
                    }
                    else {
                        unsigned cp;
                        unsigned i;
                        
                        ctx->nrefs = nrefs;
                        for (i = 0, cp = refs_start; cp != data_start; ++i) {
                            int32_t len;
                            
                            memcpy(&len, &ctx->buf[cp], 4);
                            memcpy(&ctx->refLen[i], &ctx->buf[cp + 4 + len], 4);
                            cp += len + 8;
                        }
                    }
                }
                ctx->dnext = data_start;
            }
            else if (GetRCState(rc2) != rcIncomplete || GetRCObject(rc2) != rcData) {
                ctx->stats->bamHeaderIsBad = true;
                ctx->options = bvo_BlockCompression;
                rc = rc2;
            }
            else
                ctx->dnext = ctx->bsize;
        }
        if (rc == 0) {
            if (ctx->stats->bamHeaderIsGood) {
                unsigned cp = ctx->dnext;
                
                while (cp + 4 < ctx->bsize) {
                    int32_t rsize;
                    
                    rsize = LE2HI32(&ctx->buf[cp]);
                    if (rsize < 0) {
                        ++ctx->stats->blockStructure.error;
                        ctx->options = bvo_BlockStructure;
                        
                        /* throw away the rest of the current buffer */
                        if (cp >= ctx->bsize - dsize)
                            cp = ctx->bsize;
                        else
                            cp = ctx->bsize - dsize;
                        
                        rc = RC(rcAlign, rcFile, rcValidating, rcData, rcInvalid);
                        break;
                    }
                    else if (cp + 4 + rsize < ctx->bsize) {
                        if (rsize > UINT16_MAX)
                            ++ctx->stats->blockStructure.warning;
                        else
                            ++ctx->stats->blockStructure.good;
                        if (ctx->options > bvo_BlockStructure) {
                            BAMAlignment algn;
                            
/*                            rc = BAMAlignmentParse(&algn, &ctx->buf[cp + 4], rsize); */
                            if (rc == 0) {
                                ++ctx->stats->recordStructure.good;
                                if (ctx->options > bvo_RecordStructure)
                                    rc = BAMValidate3(ctx, &algn);
                            }
                            else
                                ++ctx->stats->recordStructure.error;
                        }
                        cp += 4 + rsize;
                    }
                    else
                        break;
                }
                if (&ctx->buf[cp] >= data) {
                    if (cp < ctx->bsize) {
                        ctx->bsize -= cp;
                        memmove(ctx->buf, &ctx->buf[cp], ctx->bsize);
                        cp = ctx->bsize;
                    }
                    else {
                        assert(cp == ctx->bsize);
                        cp = ctx->bsize = 0;
                    }
                }
                ctx->dnext = cp;
            }
            if (ctx->alloced < ctx->bsize + ZLIB_BLOCK_SIZE) {
                void *temp;
                
                temp = realloc(ctx->buf, ctx->alloced + ZLIB_BLOCK_SIZE);
                if (temp == NULL) {
                    rc = RC(rcAlign, rcFile, rcValidating, rcMemory, rcExhausted);
                    fatal = true;
                }
                else {
                    ctx->buf = temp;
                    ctx->alloced += ZLIB_BLOCK_SIZE;
                }
            }
            ctx->nxt = &ctx->buf[ctx->dnext];
        }
    }
    rc2 = ctx->callback(ctx->ctx, rc, ctx->stats);
    ctx->cancelled |= rc2 != 0;
    return fatal ? rc : rc2;
}

static rc_t BAMValidateBAM(struct VPath const *bampath,
                           BAMValidateOption options,
                           BAMValidateCallback callback,
                           void *callbackContext
                           )
{
    rc_t rc;
    BGZFile bam;
    BAMValidate_ctx_t ctx;
    BAMValidateStats stats;

    if (bampath == NULL)
        return RC(rcAlign, rcFile, rcValidating, rcParam, rcNull);
    
    memset(&ctx, 0, sizeof(ctx));
    memset(&stats, 0, sizeof(stats));
    
    ctx.callback = callback;
    ctx.ctx = callbackContext;
    ctx.options = options;
    ctx.stats = &stats;
    
    if (options > bvo_BlockCompression) {
        ctx.alloced = ZLIB_BLOCK_SIZE * 2;
        ctx.nxt = ctx.buf = malloc(ctx.alloced);
        
        if (ctx.buf == NULL)
            return RC(rcAlign, rcFile, rcValidating, rcMemory, rcExhausted);
    }
    
    if (options > bvo_RecordStructure)
        options = bvo_RecordStructure | (options & 0xFFF0);
    
    rc = VPath2BGZF(&bam, bampath);
    if (rc == 0) {
        stats.bamFileSize = bam.fsize;
        if ((options & 7) > bvo_BlockHeaders)
            rc = BGZFileWalkBlocks(&bam, true, (zlib_block_t *)&ctx.nxt, BAMValidate2, &ctx);
        else
            rc = BGZFileWalkBlocks(&bam, false, NULL, BAMValidate2, &ctx);
    }
    BGZFileWhack(&bam);
    return rc;
}

static rc_t CC dummy_cb(void *ctx, rc_t result, const BAMValidateStats *stats)
{
    return 0;
}

LIB_EXPORT rc_t CC BAMValidate(struct VPath const *bampath,
                               struct VPath const *baipath,
                               BAMValidateOption options,
                               BAMValidateCallback callback,
                               void *callbackContext
                               )
{
    if (callback == NULL)
        callback = dummy_cb;
    if (bampath == NULL)
        return RC(rcAlign, rcFile, rcValidating, rcParam, rcNull);
    if (baipath == NULL) {
        if (options & bvo_IndexOptions)
            return RC(rcAlign, rcFile, rcValidating, rcParam, rcNull);
        return BAMValidateBAM(bampath, options, callback, callbackContext);
    }
    return BAMValidateIndex(bampath, baipath, options, callback, callbackContext);
}
