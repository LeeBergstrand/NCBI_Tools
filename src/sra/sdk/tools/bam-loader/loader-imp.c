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

/* #include "bam-load.vers.h" */

#include <klib/callconv.h>
#include <klib/data-buffer.h>
#include <klib/text.h>
#include <klib/log.h>
#include <klib/out.h>
#include <klib/status.h>
#include <klib/rc.h>
#include <klib/sort.h>
#include <klib/printf.h>

#include <kfs/directory.h>
#include <kfs/file.h>
#include <kfs/mmap.h>
#include <kfs/pagefile.h>
#include <kfs/pmem.h>
#include <kdb/btree.h>
#include <kdb/manager.h>
#include <kdb/database.h>
#include <kdb/table.h>
#include <kdb/meta.h>

#include <vdb/manager.h>
#include <vdb/schema.h>
#include <vdb/database.h>
#include <vdb/table.h>
#include <vdb/cursor.h>
#include <vdb/vdb-priv.h>

#include <insdc/insdc.h>
#include <insdc/sra.h>
#include <align/dna-reverse-cmpl.h>

#include <kapp/main.h>
#include <kapp/args.h>
#include <kapp/loader-file.h>
#include <kapp/loader-meta.h>
#include <kapp/log-xml.h>
#include <kapp/progressbar.h>

#include <sysalloc.h>
#include <atomic32.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#include <align/bam.h>

#include "Globals.h"
#include "sequence-writer.h"
#include "reference-writer.h"
#include "alignment-writer.h"

#define NUM_ID_SPACES (256u)

#define MMA_NUM_CHUNKS_BITS (24u)
#define MMA_NUM_SUBCHUNKS_BITS ((32u)-(MMA_NUM_CHUNKS_BITS))
#define MMA_SUBCHUNK_SIZE (1u << MMA_NUM_CHUNKS_BITS)
#define MMA_SUBCHUNK_COUNT (1u << MMA_NUM_SUBCHUNKS_BITS)

typedef struct {
    KFile *fp;
    size_t elemSize;
    uint64_t fsize;
    struct mma_map_s {
        struct mma_submap_s {
            uint8_t *base;
            KMMap *mmap;
        } submap[MMA_SUBCHUNK_COUNT];
    } map[NUM_ID_SPACES];
} MMArray;

#define FRAG_CHUNK_SIZE (128)
typedef struct {
    uint32_t primaryId[2];
    uint32_t spotId;
    uint32_t fragmentId;
    uint8_t  platform;
    uint8_t  pId_ext[2];
    uint8_t  spotId_ext;
    uint8_t  alignmentCount[2]; /* 0..254; 254: saturated max; 255: special meaning "too many" */
    uint8_t  unmated: 1,
             pcr_dup: 1,
             has_a_read: 1,
             unaligned_1: 1,
             unaligned_2: 1;
} ctx_value_t;

#define CTX_VALUE_SET_P_ID(O,N,V) do { int64_t tv = (V); (O).primaryId[N] = (uint32_t)tv; (O).pId_ext[N] = tv >> 32; } while(0);
#define CTX_VALUE_GET_P_ID(O,N) ((((int64_t)((O).pId_ext[N])) << 32) | (O).primaryId[N])

#define CTX_VALUE_SET_S_ID(O,V) do { int64_t tv = (V); (O).spotId = (uint32_t)tv; (O).spotId_ext = tv >> 32; } while(0);
#define CTX_VALUE_GET_S_ID(O) ((((int64_t)(O).spotId_ext) << 32) | (O).spotId)

typedef struct FragmentInfo {
    uint64_t ti;
    uint32_t readlen;
    uint8_t  aligned;
    uint8_t  is_bad;
    uint8_t  orientation;
    uint8_t  otherReadNo;
    uint8_t  sglen;
    uint8_t  cskey;
} FragmentInfo;

typedef struct context_t {
    const KLoadProgressbar *progress[4];
    KBTree *key2id[NUM_ID_SPACES];
    char *key2id_names;
    MMArray *id2value;
    KMemBank *fragsBoth; /*** mate will be there soon ***/
    KMemBank *fragsOne;  /*** mate may not be found soon or even show up ***/
    int64_t spotId;
    int64_t primaryId;
    int64_t secondId;
    uint64_t alignCount;
    
    uint32_t idCount[NUM_ID_SPACES];
    uint32_t key2id_hash[NUM_ID_SPACES];

    unsigned key2id_max;
    unsigned key2id_name_max;
    unsigned key2id_name_alloc;
    unsigned key2id_count;
    
    unsigned key2id_name[NUM_ID_SPACES];
    /* this array is kept in name order */
    /* this maps the names to key2id and idCount */
    unsigned key2id_oid[NUM_ID_SPACES];
    
    unsigned pass;
    bool isColorSpace;
} context_t;

static char const *Print_ctx_value_t(ctx_value_t const *const self)
{
    static char buffer[4096];
    rc_t rc = string_printf(buffer, sizeof(buffer), NULL, "pid: { %lu, %lu }, sid: %lu, fid: %u, alc: { %u, %u }, flg: %x", CTX_VALUE_GET_P_ID(*self, 0), CTX_VALUE_GET_P_ID(*self, 1), CTX_VALUE_GET_S_ID(*self), self->fragmentId, self->alignmentCount[0], self->alignmentCount[1], self->alignmentCount[2]);

    if (rc)
        return 0;
    return buffer;
}

static rc_t MMArrayMake(MMArray **rslt, KFile *fp, uint32_t elemSize)
{
    MMArray *const self = calloc(1, sizeof(*self));

    if (self == NULL)
        return RC(rcExe, rcMemMap, rcConstructing, rcMemory, rcExhausted);
    self->elemSize = (elemSize + 3) & ~(3u); /** align to 4 byte **/
    self->fp = fp;
    KFileAddRef(fp);
    *rslt = self;
    return 0;
}

#define PERF 0

static rc_t MMArrayGet(MMArray *const self, void **const value, uint64_t const element)
{
    unsigned const bin_no = element >> 32;
    unsigned const subbin = ((uint32_t)element) >> MMA_NUM_CHUNKS_BITS;
    unsigned const in_bin = (uint32_t)element & (MMA_SUBCHUNK_SIZE - 1);

    if (bin_no >= sizeof(self->map)/sizeof(self->map[0]))
        return RC(rcExe, rcMemMap, rcConstructing, rcId, rcExcessive);
    
    if (self->map[bin_no].submap[subbin].base == NULL) {
        size_t const chunk = MMA_SUBCHUNK_SIZE * self->elemSize;
        size_t const fsize = self->fsize + chunk;
        rc_t rc = KFileSetSize(self->fp, fsize);
        
        if (rc == 0) {
            KMMap *mmap;
            
            self->fsize = fsize;
            rc = KMMapMakeRgnUpdate(&mmap, self->fp, self->fsize, chunk);
            if (rc == 0) {
                void *base;
                
                rc = KMMapAddrUpdate(mmap, &base);
                if (rc == 0) {
#if PERF
                    static unsigned mapcount = 0;

                    (void)PLOGMSG(klogInfo, (klogInfo, "Number of mmaps: $(cnt)", "cnt=%u", ++mapcount));
#endif
                    self->map[bin_no].submap[subbin].mmap = mmap;
                    self->map[bin_no].submap[subbin].base = base;

                    goto GET_MAP;
                }
                KMMapRelease(mmap);
            }
        }
        return rc;
    }
GET_MAP:
    *value = &self->map[bin_no].submap[subbin].base[(size_t)in_bin * self->elemSize];
    return 0;
}

static void MMArrayWhack(MMArray *self)
{
    unsigned i;

    for (i = 0; i != sizeof(self->map)/sizeof(self->map[0]); ++i) {
        unsigned j;
        
        for (j = 0; j != sizeof(self->map[0].submap)/sizeof(self->map[0].submap[0]); ++j) {
            if (self->map[i].submap[j].mmap)
                KMMapRelease(self->map[i].submap[j].mmap);
            self->map[i].submap[j].mmap = NULL;
            self->map[i].submap[j].base = NULL;
        }
    }
    KFileRelease(self->fp);
    free(self);
}

static rc_t OpenKBTree(KBTree **const rslt, unsigned n, unsigned max)
{
    size_t const cacheSize = (((G.cache_size - (G.cache_size / 2) - (G.cache_size / 8)) / max)
                            + 0xFFFFF) & ~((size_t)0xFFFFF);
    KFile *file = NULL;
    KDirectory *dir;
    char fname[4096];
    rc_t rc;
    
    rc = KDirectoryNativeDir(&dir);
    if (rc)
        return rc;
    
    rc = string_printf(fname, sizeof(fname), NULL, "%s/key2id.%u.%u", G.tmpfs, G.pid, n); if (rc) return rc;
    rc = KDirectoryCreateFile(dir, &file, true, 0600, kcmInit, fname);
    KDirectoryRemove(dir, 0, fname);
    KDirectoryRelease(dir);
    if (rc == 0) {
        rc = KBTreeMakeUpdate(rslt, file, cacheSize,
                              false, kbtOpaqueKey,
                              1, 255, sizeof ( uint32_t ),
                              NULL
                              );
        KFileRelease(file);
#if PERF
        if (rc == 0) {
            static unsigned treecount = 0;

            (void)PLOGMSG(klogInfo, (klogInfo, "Number of trees: $(cnt)", "cnt=%u", ++treecount));
        }
#endif
    }
    return rc;
}

static rc_t GetKeyIDOld(context_t *const ctx, uint64_t *const rslt, bool *const wasInserted, char const key[], char const name[], unsigned const namelen)
{
    unsigned const keylen = strlen(key);
    rc_t rc;
    uint64_t tmpKey;

    if (ctx->key2id_count == 0) {
        rc = OpenKBTree(&ctx->key2id[0], 1, 1);
        if (rc) return rc;
        ctx->key2id_count = 1;
    }
    if (memcmp(key, name, keylen) == 0) {
        /* qname starts with read group; no append */
        tmpKey = ctx->idCount[0];
        rc = KBTreeEntry(ctx->key2id[0], &tmpKey, wasInserted, name, namelen);
    }
    else {
        char sbuf[4096];
        char *buf = sbuf;
        char *hbuf = NULL;
        size_t bsize = sizeof(sbuf);
        size_t actsize;
        
        if (keylen + namelen + 2 > bsize) {
            hbuf = malloc(bsize = keylen + namelen + 2);
            if (hbuf == NULL)
                return RC(rcExe, rcName, rcAllocating, rcMemory, rcExhausted);
            buf = hbuf;
        }
        rc = string_printf(buf, bsize, &actsize, "%s\t%.*s", key, (int)namelen, name);
        
        tmpKey = ctx->idCount[0];
        rc = KBTreeEntry(ctx->key2id[0], &tmpKey, wasInserted, buf, actsize);
        if (hbuf)
            free(hbuf);
    }
    if (rc == 0) {
        *rslt = tmpKey;
        if (*wasInserted)
            ++ctx->idCount[0];
    }
    return rc;
}

static unsigned HashKey(void const *key, unsigned keylen)
{
    /* There is nothing special about this hash. It was randomly generated. */
    static const uint8_t T1[] = {
         64, 186,  39, 203,  54, 211,  98,  32,  26,  23, 219,  94,  77,  60,  56, 184,
        129, 242,  10,  91,  84, 192,  19, 197, 231, 133, 125, 244,  48, 176, 160, 164,
         17,  41,  57, 137,  44, 196, 116, 146, 105,  40, 122,  47, 220, 226, 213, 212,
        107, 191,  52, 144,   9, 145,  81, 101, 217, 206,  85, 134, 143,  58, 128,  20,
        236, 102,  83, 149, 148, 180, 167, 163,  12, 239,  31,   0,  73, 152,   1,  15,
         75, 200,   4, 165,   5,  66,  25, 111, 255,  70, 174, 151,  96, 126, 147,  34,
        112, 161, 127, 181, 237,  78,  37,  74, 222, 123,  21, 132,  95,  51, 141,  45,
         61, 131, 193,  68,  62, 249, 178,  33,   7, 195, 228,  82,  27,  46, 254,  90,
        185, 240, 246, 124, 205, 182,  42,  22, 198,  69, 166,  92, 169, 136, 223, 245,
        118,  97, 115,  80, 252, 209,  49,  79, 221,  38,  28,  35,  36, 208, 187, 248,
        158, 201, 202, 168,   2,  18, 189, 119, 216, 214,  11,   6,  89,  16, 229, 109,
        120,  43, 162, 106, 204,   8, 199, 235, 142, 210,  86, 153, 227, 230,  24, 100,
        224, 113, 190, 243, 218, 215, 225, 173,  99, 238, 138,  59, 183, 154, 171, 232,
        157, 247, 233,  67,  88,  50, 253, 251, 140, 104, 156, 170, 150, 103, 117, 110,
        155,  72, 207, 250, 159, 194, 177, 130, 135,  87,  71, 175,  14,  55, 172, 121,
        234,  13,  30, 241,  93, 188,  53, 114,  76,  29,  65,   3, 179, 108,  63, 139
    };
    unsigned h = 0x55;
    unsigned i = keylen;
    
    do { h = T1[h ^ ((uint8_t)i)]; } while ((i >>= 8) != 0);

    for (i = 0; i != keylen; ++i)
        h = T1[h ^ ((uint8_t const *)key)[i]];

    return h;
}

static rc_t GetKeyID(context_t *const ctx, uint64_t *const rslt, bool *const wasInserted, char const key[], char const name[], unsigned const namelen)
{
    if (ctx->key2id_max == 1)
        return GetKeyIDOld(ctx, rslt, wasInserted, key, name, namelen);
    else {
        unsigned const keylen = strlen(key);
        unsigned const h = HashKey(key, keylen);
        unsigned f;
        unsigned e = ctx->key2id_count;
        uint64_t tmpKey;
        
        *rslt = 0;
        {{
            uint32_t const bucket_value = ctx->key2id_hash[h];
            unsigned const n  = (uint8_t) bucket_value;
            unsigned const i1 = (uint8_t)(bucket_value >>  8);
            unsigned const i2 = (uint8_t)(bucket_value >> 16);
            unsigned const i3 = (uint8_t)(bucket_value >> 24);
            
            if (n > 0 && strcmp(key, ctx->key2id_names + ctx->key2id_name[i1]) == 0) {
                f = i1;
                /*
                ctx->key2id_hash[h] = (i3 << 24) | (i2 << 16) | (i1 << 8) | n;
                 */
                goto GET_ID;
            }
            if (n > 1 && strcmp(key, ctx->key2id_names + ctx->key2id_name[i2]) == 0) {
                f = i2;
                ctx->key2id_hash[h] = (i3 << 24) | (i1 << 16) | (i2 << 8) | n;
                goto GET_ID;
            }
            if (n > 2 && strcmp(key, ctx->key2id_names + ctx->key2id_name[i3]) == 0) {
                f = i3;
                ctx->key2id_hash[h] = (i2 << 24) | (i1 << 16) | (i3 << 8) | n;
                goto GET_ID;
            }
        }}
        f = 0;
        while (f < e) {
            unsigned const m = (f + e) / 2;
            unsigned const oid = ctx->key2id_oid[m];
            int const diff = strcmp(key, ctx->key2id_names + ctx->key2id_name[oid]);
            
            if (diff < 0)
                e = m;
            else if (diff > 0)
                f = m + 1;
            else {
                f = oid;
                goto GET_ID;
            }
        }
        if (ctx->key2id_count < ctx->key2id_max) {
            unsigned const name_max = ctx->key2id_name_max + keylen + 1;
            KBTree *tree;
            rc_t rc = OpenKBTree(&tree, ctx->key2id_count + 1, 1); /* ctx->key2id_max); */
            
            if (rc) return rc;
            
            if (ctx->key2id_name_alloc < name_max) {
                unsigned alloc = ctx->key2id_name_alloc;
                void *tmp;
                
                if (alloc == 0)
                    alloc = 4096;
                while (alloc < name_max)
                    alloc <<= 1;
                tmp = realloc(ctx->key2id_names, alloc);
                if (tmp == NULL)
                    return RC(rcExe, rcName, rcAllocating, rcMemory, rcExhausted);
                ctx->key2id_names = tmp;
                ctx->key2id_name_alloc = alloc;
            }
            if (f < ctx->key2id_count) {
                memmove(&ctx->key2id_oid[f + 1], &ctx->key2id_oid[f], (ctx->key2id_count - f) * sizeof(ctx->key2id_oid[f]));
            }
            ctx->key2id_oid[f] = ctx->key2id_count;
            ++ctx->key2id_count;
            f = ctx->key2id_oid[f];
            ctx->key2id_name[f] = ctx->key2id_name_max;
            ctx->key2id_name_max = name_max;

            memcpy(&ctx->key2id_names[ctx->key2id_name[f]], key, keylen + 1);
            ctx->key2id[f] = tree;
            ctx->idCount[f] = 0;
            if ((uint8_t)ctx->key2id_hash[h] < 3) {
                unsigned const n = (uint8_t)ctx->key2id_hash[h] + 1;
                
                ctx->key2id_hash[h] = (((ctx->key2id_hash[h] & ~(0xFFu)) | f) << 8) | n;
            }
            else {
                /* the hash function isn't working too well
                 * keep the 3 mru
                 */
                ctx->key2id_hash[h] = (((ctx->key2id_hash[h] & ~(0xFFu)) | f) << 8) | 3;
            }
        GET_ID:
            tmpKey = ctx->idCount[f];
            rc = KBTreeEntry(ctx->key2id[f], &tmpKey, wasInserted, name, namelen);
            if (rc == 0) {
                *rslt = (((uint64_t)f) << 32) | tmpKey;
                if (*wasInserted)
                    ++ctx->idCount[f];
                assert(tmpKey < ctx->idCount[f]);
            }
            return rc;
        }
        return RC(rcExe, rcTree, rcAllocating, rcConstraint, rcViolated);
    }
}

static rc_t OpenMMapFile(context_t *const ctx, KDirectory *const dir)
{
    KFile *file = NULL;
    char fname[4096];
    rc_t rc = string_printf(fname, sizeof(fname), NULL, "%s/id2value.%u", G.tmpfs, G.pid);
    
    if (rc)
        return rc;
    
    rc = KDirectoryCreateFile(dir, &file, true, 0600, kcmInit, fname);
    KDirectoryRemove(dir, 0, fname);
    if (rc == 0)
        rc = MMArrayMake(&ctx->id2value, file, sizeof(ctx_value_t));
    KFileRelease(file);
    return rc;
}

static rc_t OpenMBankFile(context_t *const ctx, KDirectory *const dir, int which, size_t climit)
{
    KFile *file = NULL;
    char fname[4096];
    char const *const suffix = which == 1 ? "One" : "Both";
    KMemBank **const mbank = which == 1 ? &ctx->fragsOne : &ctx->fragsBoth;
    rc_t rc = string_printf(fname, sizeof(fname), NULL, "%s/frag_data%s.%u", G.tmpfs, suffix, G.pid);
    
    if (rc)
        return rc;
    
    rc = KDirectoryCreateFile(dir, &file, true, 0600, kcmInit, fname);
    KDirectoryRemove(dir, 0, fname);
    if (rc == 0) {
        KPageFile *backing;
        
        rc = KPageFileMakeUpdate(&backing, file, climit, false);
        KFileRelease(file);
        if (rc == 0) {
            rc = KMemBankMake(mbank, FRAG_CHUNK_SIZE, 0, backing);
            KPageFileRelease(backing);
        }
    }
    return rc;
}

static rc_t SetupContext(context_t *ctx, unsigned numfiles)
{
    rc_t rc = 0;

    memset(ctx, 0, sizeof(*ctx));
    
    if (G.mode == mode_Archive) {
        KDirectory *dir;
        size_t fragSizeBoth; /*** temporary hold for first side of mate pair with both sides aligned**/
        size_t fragSizeOne; /*** temporary hold for first side of mate pair with one side aligned**/

        fragSizeBoth    =   (G.cache_size / 8);
        fragSizeOne     =   (G.cache_size / 2);

        rc = KLoadProgressbar_Make(&ctx->progress[0], 0); if (rc) return rc;
        rc = KLoadProgressbar_Make(&ctx->progress[1], 0); if (rc) return rc;
        rc = KLoadProgressbar_Make(&ctx->progress[2], 0); if (rc) return rc;
        rc = KLoadProgressbar_Make(&ctx->progress[3], 0); if (rc) return rc;
        
        KLoadProgressbar_Append(ctx->progress[0], 100 * numfiles);
        
        rc = KDirectoryNativeDir(&dir);
        if (rc == 0)
            rc = OpenMMapFile(ctx, dir);
        if (rc == 0)
            rc = OpenMBankFile(ctx, dir, 0, fragSizeBoth);
        if (rc == 0)
            rc = OpenMBankFile(ctx, dir, 1, fragSizeOne);
        KDirectoryRelease(dir);
    }
    return rc;
}

static void ContextReleaseMemBank(context_t *ctx)
{
    KMemBankRelease(ctx->fragsOne);
    ctx->fragsOne = NULL;
    KMemBankRelease(ctx->fragsBoth);
    ctx->fragsBoth = NULL;
}

static void ContextRelease(context_t *ctx)
{
    KLoadProgressbar_Release(ctx->progress[0], true);
    KLoadProgressbar_Release(ctx->progress[1], true);
    KLoadProgressbar_Release(ctx->progress[2], true);
    KLoadProgressbar_Release(ctx->progress[3], true);
    MMArrayWhack(ctx->id2value);
}

static
void COPY_QUAL(uint8_t D[], uint8_t const S[], unsigned const L, bool const R) 
{
    if (R) {
        unsigned i;
        unsigned j;
        
        for (i = 0, j = L - 1; i != L; ++i, --j)
            D[i] = S[j];
    }
    else
        memcpy(D, S, L);
}

static
void COPY_READ(INSDC_dna_text D[], INSDC_dna_text const S[], unsigned const L, bool const R)
{
    static INSDC_dna_text const compl[] = {
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 , '.',  0 , 
        '0', '1', '2', '3',  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 , 'T', 'V', 'G', 'H',  0 ,  0 , 'C', 
        'D',  0 ,  0 , 'M',  0 , 'K', 'N',  0 , 
         0 ,  0 , 'Y', 'S', 'A', 'A', 'B', 'W', 
         0 , 'R',  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 , 'T', 'V', 'G', 'H',  0 ,  0 , 'C', 
        'D',  0 ,  0 , 'M',  0 , 'K', 'N',  0 , 
         0 ,  0 , 'Y', 'S', 'A', 'A', 'B', 'W', 
         0 , 'R',  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0
    };
    if (R) {
        unsigned i;
        unsigned j;
        
        for (i = 0, j = L - 1; i != L; ++i, --j)
            D[i] = compl[((uint8_t const *)S)[j]];
    }
    else
        memcpy(D, S, L);
}

static rc_t OpenBAM(const BAMFile **bam, VDatabase *db, const char bamFile[])
{
    rc_t rc = BAMFileMakeWithHeader(bam, G.headerText, bamFile);
    if (rc) {
        (void)PLOGERR(klogErr, (klogErr, rc, "Failed to open '$(file)'", "file=%s", bamFile));
    }
    else if (db) {
        KMetadata *dbmeta;
        
        rc = VDatabaseOpenMetadataUpdate(db, &dbmeta);
        if (rc == 0) {
            KMDataNode *node;
            
            rc = KMetadataOpenNodeUpdate(dbmeta, &node, "BAM_HEADER");
            KMetadataRelease(dbmeta);
            if (rc == 0) {
                char const *header;
                size_t size;
                
                rc = BAMFileGetHeaderText(*bam, &header, &size);
                if (rc == 0) {
                    rc = KMDataNodeWrite(node, header, size);
                }
                KMDataNodeRelease(node);
            }
        }
    }

    return rc;
}

static rc_t VerifyReferences(BAMFile const *bam, Reference const *ref)
{
    rc_t rc = 0;
    uint32_t n;
    unsigned i;
    
    BAMFileGetRefSeqCount(bam, &n);
    for (i = 0; i != n; ++i) {
        BAMRefSeq const *refSeq;
        
        BAMFileGetRefSeq(bam, i, &refSeq);
        if (G.refFilter && strcmp(refSeq->name, G.refFilter) != 0)
            continue;
        
        rc = ReferenceVerify(ref, refSeq->name, refSeq->length, refSeq->checksum);
        if (rc) {
            if (GetRCObject(rc) == rcChecksum && GetRCState(rc) == rcUnequal) {
#if NCBI
                (void)PLOGMSG(klogWarn, (klogWarn, "Reference: '$(name)', Length: $(len); checksums do not match", "name=%s,len=%u", refSeq->name, (unsigned)refSeq->length));
#endif
            }
            else
            if (GetRCObject(rc) == rcSize && GetRCState(rc) == rcUnequal) {
                (void)PLOGMSG(klogWarn, (klogWarn, "Reference: '$(name)', Length: $(len); lengths do not match", "name=%s,len=%u", refSeq->name, (unsigned)refSeq->length));
            }
            else if (GetRCObject(rc) == rcSize && GetRCState(rc) == rcEmpty) {
                (void)PLOGMSG(klogWarn, (klogWarn, "Reference: '$(name)', Length: $(len); fasta file is empty", "name=%s,len=%u", refSeq->name, (unsigned)refSeq->length));
            }
            else if (GetRCObject(rc) == rcId && GetRCState(rc) == rcNotFound) {
                (void)PLOGMSG(klogWarn, (klogWarn, "Reference: '$(name)', Length: $(len); no match found", "name=%s,len=%u", refSeq->name, (unsigned)refSeq->length));
            }
            else {
                (void)PLOGERR(klogWarn, (klogWarn, rc, "Reference: '$(name)', Length: $(len); error", "name=%s,len=%u", refSeq->name, (unsigned)refSeq->length));
            }
        }
        else if (G.onlyVerifyReferences) {
            (void)PLOGMSG(klogInfo, (klogInfo, "Reference: '$(name)', Length: $(len); match found", "name=%s,len=%u", refSeq->name, (unsigned)refSeq->length));
        }
    }
    return 0;
}

static uint8_t GetMapQ(BAMAlignment const *rec)
{
    uint8_t mapQ;
    
    BAMAlignmentGetMapQuality(rec, &mapQ);
    return mapQ;
}

static void EditAlignedQualities(uint8_t qual[], bool const hasMismatch[], unsigned readlen)
{
    unsigned i;
    
    for (i = 0; i < readlen; ++i) {
        uint8_t const q = hasMismatch[i] ? G.alignedQualValue : qual[i];
        
        qual[i] = q;
    }
}

static void EditUnalignedQualities(uint8_t qual[], bool const hasMismatch[], unsigned readlen)
{
    unsigned i;
    
    for (i = 0; i < readlen; ++i) {
        uint8_t const q = (qual[i] & 0x7F) | (hasMismatch[i] ? 0x80 : 0);
        
        qual[i] = q;
    }
}

static void AlignmentRecordInit(AlignmentRecord *self, void *buffer, unsigned readlen, char **endp)
{
    memset(self, 0, sizeof(*self));

    self->data.seq_read_id.buffer = &self->read_id;
    self->data.seq_read_id.elements = 1;
    self->data.ref_id.buffer = &self->ref_id;
    self->data.ref_id.elements = 1;
    if (G.expectUnsorted) {
        self->data.ref_start.buffer = &self->ref_start;
        self->data.ref_start.elements = 1;
    }
    else {
        self->data.global_ref_start.buffer = &self->global_ref_start;
        self->data.global_ref_start.elements = 1;
    }
    self->data.ref_orientation.buffer = &self->ref_orientation;
    self->data.ref_orientation.elements = 1;
    self->data.mapq.buffer = &self->mapq;
    self->data.mapq.elements = 1;
    self->data.tmp_key_id.buffer = &self->tmp_key_id;
    self->data.tmp_key_id.elements = 1;
    
    self->data.read_start.buffer = &self->read_start;
    self->data.read_start.elements = 1;
    self->data.read_len.buffer = &self->read_len;
    self->data.read_len.elements = 1;
    
    self->data.mate_ref_orientation.buffer = &self->mate_ref_orientation;
    self->data.mate_ref_orientation.elements = 1;
    self->data.mate_ref_id.buffer = &self->mate_ref_id;
    self->data.mate_ref_id.elements = 1;
    self->data.mate_ref_pos.buffer = &self->mate_ref_pos;
    self->data.mate_ref_pos.elements = 1;
    self->data.mate_align_id.buffer = &self->mate_align_id;
    self->data.mate_align_id.elements = 1;
    self->data.template_len.buffer = &self->template_len;
    self->data.template_len.elements = 1;
    
    self->data.ref_offset.buffer = (int32_t *)buffer;
    self->data.has_mismatch.buffer = (bool *)&AR_OFFSET(*self)[readlen];
    self->data.has_mismatch.elements = readlen;
    self->data.has_ref_offset.buffer = &AR_HAS_MISMATCH(*self)[readlen];
    self->data.has_ref_offset.elements = readlen;
    self->data.mismatch.buffer = (char *)&AR_HAS_OFFSET(*self)[readlen];
    
    *endp = (char *)&AR_MISMATCH(*self)[readlen];
}

static bool platform_cmp(char const platform[], char const test[])
{
    unsigned i;

    for (i = 0; ; ++i) {
        int ch1 = test[i];
        int ch2 = toupper(platform[i]);
        
        if (ch1 != ch2)
            break;
        if (ch1 == 0)
            return true;
    }
    return false;
}

static
INSDC_SRA_platform_id GetINSDCPlatform(BAMFile const *bam, char const name[]) {
    if (name) {
        BAMReadGroup const *rg;

        BAMFileGetReadGroupByName(bam, name, &rg);
        if (rg && rg->platform) {
            switch (toupper(rg->platform[0])) {
            case 'C':
                if (platform_cmp(rg->platform, "COMPLETE GENOMICS"))
                    return SRA_PLATFORM_COMPLETE_GENOMICS;
                if (platform_cmp(rg->platform, "CAPILLARY"))
                    return SRA_PLATFORM_SANGER;
                break;
            case 'H':
                if (platform_cmp(rg->platform, "HELICOS"))
                    return SRA_PLATFORM_HELICOS;
                break;
            case 'I':
                if (platform_cmp(rg->platform, "ILLUMINA"))
                    return SRA_PLATFORM_ILLUMINA;
                if (platform_cmp(rg->platform, "IONTORRENT"))
                    return SRA_PLATFORM_ION_TORRENT;
                break;
            case 'L':
                if (platform_cmp(rg->platform, "LS454"))
                    return SRA_PLATFORM_454;
                break;
            case 'P':
                if (platform_cmp(rg->platform, "PACBIO"))
                    return SRA_PLATFORM_PACBIO_SMRT;
                break;
            case 'S':
                if (platform_cmp(rg->platform, "SOLID"))
                    return SRA_PLATFORM_ABSOLID;
                break;
            default:
                break;
            }
        }
    }
    return SRA_PLATFORM_UNDEFINED;
}

static
rc_t CheckLimitAndLogError(void)
{
    ++G.errCount;
    if (G.maxErrCount > 0 && G.errCount > G.maxErrCount) {
        (void)PLOGERR(klogErr, (klogErr, RC(rcAlign, rcFile, rcReading, rcError, rcExcessive), "Number of errors $(cnt) exceeds limit of $(max): Exiting", "cnt=%u,max=%u", G.errCount, G.maxErrCount));
        return RC(rcAlign, rcFile, rcReading, rcError, rcExcessive);
    }
    return 0;
}

static
void RecordNoMatch(char const readName[], char const refName[], uint32_t const refPos)
{
    if (G.noMatchLog) {
        static uint64_t lpos = 0;
        char logbuf[256];
        size_t len;
        
        if (string_printf(logbuf, sizeof(logbuf), &len, "%s\t%s\t%u\n", readName, refName, refPos) == 0) {
            KFileWrite(G.noMatchLog, lpos, logbuf, len, NULL);
            lpos += len;
        }
    }
}

static
rc_t LogNoMatch(char const readName[], char const refName[], unsigned rpos, unsigned matches)
{
    rc_t const rc = CheckLimitAndLogError();
    static unsigned count = 0;
    
    ++count;
    if (rc) {
        (void)PLOGMSG(klogInfo, (klogInfo, "This is the last warning; this class of warning occurred $(occurred) times",
                                 "occurred=%u", count));
        (void)PLOGMSG(klogWarn, (klogWarn, "Spot '$(name)' contains too few ($(count)) matching bases to reference '$(ref)' at $(pos)",
                                 "name=%s,ref=%s,pos=%u,count=%u", readName, refName, rpos, matches));
    }
    else if (G.maxWarnCount_NoMatch == 0 || count < G.maxWarnCount_NoMatch)
        (void)PLOGMSG(klogWarn, (klogWarn, "Spot '$(name)' contains too few ($(count)) matching bases to reference '$(ref)' at $(pos)",
                                 "name=%s,ref=%s,pos=%u,count=%u", readName, refName, rpos, matches));
    return rc;
}

static
rc_t LogDupConflict(char const readName[])
{
    rc_t const rc = CheckLimitAndLogError();
    static unsigned count = 0;
    
    ++count;
    if (rc) {
        (void)PLOGMSG(klogInfo, (klogInfo, "This is the last warning; this class of warning occurred $(occurred) times",
                                 "occurred=%u", count));
        (void)PLOGERR(klogWarn, (klogWarn, RC(rcApp, rcFile, rcReading, rcData, rcInconsistent),
                                 "Spot '$(name)' is both a duplicate and NOT a duplicate!",
                                 "name=%s", readName));
    }
    else if (G.maxWarnCount_DupConflict == 0 || count < G.maxWarnCount_DupConflict)
        (void)PLOGERR(klogWarn, (klogWarn, RC(rcApp, rcFile, rcReading, rcData, rcInconsistent),
                                 "Spot '$(name)' is both a duplicate and NOT a duplicate!",
                                 "name=%s", readName));
    return rc;
}

static rc_t ProcessBAM(char const bamFile[], context_t *ctx, VDatabase *db,
                       Reference *ref, Sequence *seq, Alignment *align,
                       bool *had_alignments, bool *had_sequences)
{
    const BAMFile *bam;
    const BAMAlignment *rec;
    KDataBuffer buf;
    KDataBuffer fragBuf;
    KDataBuffer cigBuf;
    rc_t rc;
    int32_t lastRefSeqId = -1;
    size_t rsize;
    uint64_t keyId = 0;
    uint64_t reccount = 0;
    SequenceRecord srec;
    char spotGroup[512];
    size_t namelen;
    unsigned progress = 0;
    unsigned warned = 0;
    long     fcountBoth=0;
    long     fcountOne=0;
    int skipRefSeqID = -1;
    uint64_t recordsProcessed = 0;
    uint64_t filterFlagConflictRecords=0; /*** counts number of conflicts between flags 0x400 and 0x200 ***/
#define MAX_WARNINGS_FLAG_CONFLICT 10000 /*** maximum errors to report ***/

    bool isColorSpace = false;
    bool isNotColorSpace = G.noColorSpace;
    char alignGroup[32];
    size_t alignGroupLen;
    
    rc = OpenBAM(&bam, db, bamFile);
    if (rc) return rc;
    if (!G.noVerifyReferences && ref != NULL) {
        rc = VerifyReferences(bam, ref);
        if (G.onlyVerifyReferences) {
            BAMFileRelease(bam);
            return rc;
        }
    }
    if (ctx->key2id_max == 0) {
        uint32_t rgcount;
        unsigned rgi;
        
        BAMFileGetReadGroupCount(bam, &rgcount);
        if (rgcount > (sizeof(ctx->key2id)/sizeof(ctx->key2id[0]) - 1))
            ctx->key2id_max = 1;
        else
            ctx->key2id_max = sizeof(ctx->key2id)/sizeof(ctx->key2id[0]);
        
        for (rgi = 0; rgi != rgcount; ++rgi) {
            BAMReadGroup const *rg;
            
            BAMFileGetReadGroup(bam, rgi, &rg);
            if (rg && rg->platform && platform_cmp(rg->platform, "CAPILLARY")) {
                G.hasTI = true;
                break;
            }
        }
    }
    memset(&srec, 0, sizeof(srec));
    
    rc = KDataBufferMake(&cigBuf, 32, 0);
    if (rc)
        return rc;
    
    rc = KDataBufferMake(&fragBuf, 8, FRAG_CHUNK_SIZE);
    if (rc)
        return rc;
    
    rc = KDataBufferMake(&buf, (sizeof(int32_t) + sizeof(bool) * 2 + sizeof(char) * 2 + sizeof(uint8_t))*8, 0);
    if (rc)
        return rc;
    
    if (rc == 0) {
        (void)PLOGMSG(klogInfo, (klogInfo, "Loading '$(file)'", "file=%s", bamFile));
    }
    while (rc == 0 && (rc = Quitting()) == 0) {
        bool aligned;
        AlignmentRecord data;
        uint32_t readlen;
        uint16_t flags;
        int64_t rpos=0;
        char *seqDNA;
        const BAMRefSeq *refSeq;
        ctx_value_t *value;
        bool wasInserted;
        int32_t refSeqId=-1;
        uint8_t *qual;
        bool mated;
        const char *name;
        char cskey = 0;
        bool originally_aligned;
        bool isPrimary;
        uint32_t opCount;
        bool hasCG;
        uint64_t ti = 0;
        uint32_t csSeqLen = 0;

        rc = BAMFileRead(bam, &rec);
        if (rc) {
            if (GetRCModule(rc) == rcAlign && GetRCObject(rc) == rcRow && GetRCState(rc) == rcNotFound)
                rc = 0;
            break;
        }
        if ((unsigned)(BAMFileGetProportionalPosition(bam) * 100.0) > progress) {
            unsigned new_value = BAMFileGetProportionalPosition(bam) * 100.0;
            KLoadProgressbar_Process(ctx->progress[0], new_value - progress, false);
            progress = new_value;
        }


        /**************************************************************/
        if (!G.noColorSpace) {
            if (BAMAlignmentHasColorSpace(rec)) {/*BAM*/
                if (isNotColorSpace) {
                MIXED_BASE_AND_COLOR:
                    rc = RC(rcApp, rcFile, rcReading, rcData, rcInconsistent);  
                    (void)PLOGERR(klogErr, (klogErr, rc, "File '$(file)' contains base space and color space", "file=%s", bamFile));
                    goto LOOP_END;
                }
                ctx->isColorSpace = isColorSpace = true;
            }
            else if (isColorSpace)
                goto MIXED_BASE_AND_COLOR;
            else
                isNotColorSpace = true;
        }
        hasCG = BAMAlignmentHasCGData(rec);/*BAM*/
        if (hasCG) {
            BAMAlignmentGetCigarCount(rec, &opCount);/*BAM*/
            rc = KDataBufferResize(&cigBuf, opCount * 2 + 5);
            if (rc) {
                (void)LOGERR(klogErr, rc, "Failed to resize CIGAR buffer");
                goto LOOP_END;
            }
            
            rc = KDataBufferResize(&buf, readlen = 35);
            if (rc) {
                (void)LOGERR(klogErr, rc, "Failed to resize record buffer");
                goto LOOP_END;
            }
            
            AlignmentRecordInit(&data, buf.base, readlen, &seqDNA);
            qual = (uint8_t *)&seqDNA[readlen];
        }
        else {
            uint32_t const *tmp;
            
            BAMAlignmentGetRawCigar(rec, &tmp, &opCount);/*BAM*/
            rc = KDataBufferResize(&cigBuf, opCount);
            if (rc) {
                (void)LOGERR(klogErr, rc, "Failed to resize CIGAR buffer");
                goto LOOP_END;
            }
            memcpy(cigBuf.base, tmp, opCount * sizeof(uint32_t));
            
            BAMAlignmentGetReadLength(rec, &readlen);/*BAM*/
            if (isColorSpace) {
                BAMAlignmentGetCSSeqLen(rec, &csSeqLen);
                if (readlen != csSeqLen && readlen != 0) {
                    rc = RC(rcAlign, rcRow, rcReading, rcData, rcInconsistent);
                    (void)LOGERR(klogErr, rc, "Sequence length and CS Sequence length are not equal");
                    goto LOOP_END;
                }
            }
            else if (readlen == 0) {
            }
            rc = KDataBufferResize(&buf, readlen | csSeqLen);
            if (rc) {
                (void)LOGERR(klogErr, rc, "Failed to resize record buffer");
                goto LOOP_END;
            }
            
            AlignmentRecordInit(&data, buf.base, readlen | csSeqLen, &seqDNA);
            qual = (uint8_t *)&seqDNA[readlen | csSeqLen];
        }
        BAMAlignmentGetSequence(rec, seqDNA);/*BAM*/
        if (G.useQUAL) {
            uint8_t const *squal;
            
            BAMAlignmentGetQuality(rec, &squal);/*BAM*/
            memcpy(qual, squal, readlen);
        }
        else {
            uint8_t const *squal;
            uint8_t qoffset = 0;
            unsigned i;
            
            rc = BAMAlignmentGetQuality2(rec, &squal, &qoffset);/*BAM*/
            if (rc) {
                (void)PLOGERR(klogErr, (klogErr, rc, "Spot '$(name)': length of original quality does not match sequence", "name=%s", name));
                goto LOOP_END;
            }
            if (qoffset) {
                for (i = 0; i != readlen; ++i)
                    qual[i] = squal[i] - qoffset;
            }
            else
                memcpy(qual, squal, readlen);
        }
        if (hasCG) {
            rc = BAMAlignmentGetCGSeqQual(rec, seqDNA, qual);/*BAM*/
            if (rc == 0)
                rc = BAMAlignmentGetCGCigar(rec, cigBuf.base, cigBuf.elem_count, &opCount);/*BAM*/
            if (rc) {
                (void)LOGERR(klogErr, rc, "Failed to read CG data");
                goto LOOP_END;
            }
        }
        if (G.hasTI) {
            rc = BAMAlignmentGetTI(rec, &ti);/*BAM*/
            if (rc)
                ti = 0;
            rc = 0;
        }
        data.data.align_group.buffer = alignGroup;
        if (BAMAlignmentGetCGAlignGroup(rec, alignGroup, sizeof(alignGroup), &alignGroupLen) == 0)/*BAM*/
            data.data.align_group.elements = alignGroupLen;
        else
            data.data.align_group.elements = 0;

        AR_MAPQ(data) = GetMapQ(rec);
        BAMAlignmentGetFlags(rec, &flags);/*BAM*/
        BAMAlignmentGetReadName2(rec, &name, &namelen);/*BAM*/
        {{
            char const *rgname;

            BAMAlignmentGetReadGroupName(rec, &rgname);/*BAM*/
            if (rgname)
                strcpy(spotGroup, rgname);
            else
                spotGroup[0] = '\0';
        }}        
        AR_REF_ORIENT(data) = (flags & BAMFlags_SelfIsReverse) == 0 ? false : true;/*BAM*/
        isPrimary = (flags & BAMFlags_IsNotPrimary) == 0 ? true : false;/*BAM*/
        if (G.noSecondary && !isPrimary)
            goto LOOP_END;
        originally_aligned = (flags & BAMFlags_SelfIsUnmapped) == 0;/*BAM*/
        aligned = originally_aligned && (AR_MAPQ(data) >= G.minMapQual);
        
        if (aligned && align == NULL) {
            rc = RC(rcApp, rcFile, rcReading, rcData, rcInconsistent);
            (void)PLOGERR(klogErr, (klogErr, rc, "File '$(file)' contains aligned records", "file=%s", bamFile));
            goto LOOP_END;
        }
        while (aligned) {
            BAMAlignmentGetPosition(rec, &rpos);/*BAM*/
            BAMAlignmentGetRefSeqId(rec, &refSeqId);/*BAM*/
            if (rpos >= 0 && refSeqId >= 0) {
                if (refSeqId == skipRefSeqID)
                    goto LOOP_END;
                if (refSeqId == lastRefSeqId)
                    break;
                refSeq = NULL;
                BAMFileGetRefSeqById(bam, refSeqId, &refSeq);/*BAM*/
                if (refSeq == NULL) {
                    rc = RC(rcApp, rcFile, rcReading, rcData, rcInconsistent);
                    (void)PLOGERR(klogWarn, (klogWarn, rc, "File '$(file)': Spot '$(name)' refers to an unknown Reference number $(refSeqId)", "file=%s,refSeqId=%i,name=%s", bamFile, (int)refSeqId, name));
                    rc = CheckLimitAndLogError();
                    goto LOOP_END;
                }
                else {
                    if (G.refFilter && strcmp(G.refFilter, refSeq->name) != 0) {
                        (void)PLOGMSG(klogInfo, (klogInfo, "Skipping Reference '$(name)'", "name=%s", refSeq->name));
                        skipRefSeqID = refSeqId;
                        goto LOOP_END;
                    }
                    
                    rc = ReferenceSetFile(ref, refSeq->name, refSeq->length, refSeq->checksum);
                    if (rc == 0) {
                        lastRefSeqId = refSeqId;
                        break;
                    }
                    if (GetRCObject(rc) == rcConstraint && GetRCState(rc) == rcViolated) {
                        int const level = G.limit2config ? klogWarn : klogErr;
                        
                        (void)PLOGMSG(level, (level, "Could not find a Reference to match { name: '$(name)', length: $(rlen) }", "name=%s,rlen=%u", refSeq->name, (unsigned)refSeq->length));
                    }
                    else if (!G.limit2config)
                        (void)PLOGERR(klogErr, (klogErr, rc, "File '$(file)': Spot '$(sname)' refers to an unknown Reference '$(rname)'", "file=%s,rname=%s,sname=%s", bamFile, refSeq->name, name));
                    if (G.limit2config)
                        rc = 0;
                    goto LOOP_END;
                }
            }
            else if (refSeqId < 0) {
                (void)PLOGMSG(klogWarn, (klogWarn, "Spot '$(name)' was marked aligned, but reference id = $(id) is invalid", "name=%.*s,id=%i", namelen, name, refSeqId));
                if ((rc = CheckLimitAndLogError()) != 0) goto LOOP_END;
            }
            else {
                (void)PLOGMSG(klogWarn, (klogWarn, "Spot '$(name)' was marked aligned, but reference position = $(pos) is invalid", "name=%.*s,pos=%i", namelen, name, rpos));
                if ((rc = CheckLimitAndLogError()) != 0) goto LOOP_END;
            }

            aligned = false;
        }
        if (!aligned && (G.refFilter != NULL || G.limit2config))
            goto LOOP_END;
        
        rc = GetKeyID(ctx, &keyId, &wasInserted, spotGroup, name, namelen);
        if (rc) {
            (void)PLOGERR(klogErr, (klogErr, rc, "KBTreeEntry: failed on key '$(key)'", "key=%.*s", namelen, name));
            goto LOOP_END;
        }
        rc = MMArrayGet(ctx->id2value, (void **)&value, keyId);
        if (rc) {
            (void)PLOGERR(klogErr, (klogErr, rc, "MMArrayGet: failed on id '$(id)'", "id=%u", keyId));
            goto LOOP_END;
        }
        
        AR_KEY(data) = keyId;
        
        mated = false;
        if (flags & BAMFlags_WasPaired) {/*BAM*/
            if ((flags & BAMFlags_IsFirst) != 0)/*BAM*/
                AR_READNO(data) |= 1;
            if ((flags & BAMFlags_IsSecond) != 0)/*BAM*/
                AR_READNO(data) |= 2;
            switch (AR_READNO(data)) {
            case 1:
            case 2:
                mated = true;
                break;
            case 0:
                if ((warned & 1) == 0) {
                    (void)LOGMSG(klogWarn, "Spots without fragment info have been encountered");
                    warned |= 1;
                }
                break;
            case 3:
                if ((warned & 2) == 0) {
                    (void)LOGMSG(klogWarn, "Spots with more than two fragments have been encountered");
                    warned |= 2;
                }
                break;
            }
        }
        if (!mated)
            AR_READNO(data) = 1;
        
        if (wasInserted) {
            memset(value, 0, sizeof(*value));
            value->unmated = !mated;
            value->pcr_dup = (flags & BAMFlags_IsDuplicate) == 0 ? 0 : 1;/*BAM*/
            value->platform = GetINSDCPlatform(bam, spotGroup);
        }
        else {
            if (!G.acceptBadDups && value->pcr_dup != ((flags & BAMFlags_IsDuplicate) == 0 ? 0 : 1)) {/*BAM*/
                rc = LogDupConflict(name);
                goto LOOP_END; /* TODO: is this correct? */
            }
            value->pcr_dup &= (flags & BAMFlags_IsDuplicate) == 0 ? 0 : 1;/*BAM*/
            if (mated && value->unmated) {
                (void)PLOGERR(klogWarn, (klogWarn, RC(rcApp, rcFile, rcReading, rcData, rcInconsistent),
                                         "Spot '$(name)', which was first seen without mate info, now has mate info",
                                         "name=%s", name));
                rc = CheckLimitAndLogError();
                goto LOOP_END;
            }
            else if (!mated && !value->unmated) {
                (void)PLOGERR(klogWarn, (klogWarn, RC(rcApp, rcFile, rcReading, rcData, rcInconsistent),
                                         "Spot '$(name)', which was first seen with mate info, now has no mate info",
                                         "name=%s", name));
                rc = CheckLimitAndLogError();
                goto LOOP_END;
            }
        }
        
        ++recordsProcessed;

        if (isPrimary) {
            switch (AR_READNO(data)) {
            case 1:
                if (CTX_VALUE_GET_P_ID(*value, 0) != 0)
                    isPrimary = false;
                else if (aligned && value->unaligned_1) {
                    (void)PLOGMSG(klogWarn, (klogWarn, "Read 1 of spot '$(name)', which was unmapped, is now being mapped at position $(pos) on reference '$(ref)'; this alignment will be considered as secondary", "name=%s,ref=%s,pos=%u", name, refSeq->name, rpos));
                    isPrimary = false;
                }
                break;
            case 2:
                if (CTX_VALUE_GET_P_ID(*value, 1) != 0)
                    isPrimary = false;
                else if (aligned && value->unaligned_2) {
                    (void)PLOGMSG(klogWarn, (klogWarn, "Read 2 of spot '$(name)', which was unmapped, is now being mapped at position $(pos) on reference '$(ref)'; this alignment will be considered as secondary", "name=%s,ref=%s,pos=%u", name, refSeq->name, rpos));
                    isPrimary = false;
                }
                break;
            default:
                break;
            }
        }
        data.isPrimary = isPrimary;
        if (aligned) {
            uint32_t matches = 0;
            
            rc = ReferenceRead(ref, &data, rpos, cigBuf.base, opCount, seqDNA, readlen, &matches);
            if (rc) {
                aligned = false;
                
                if (   (GetRCState(rc) == rcViolated  && GetRCObject(rc) == rcConstraint)
                    || (GetRCState(rc) == rcExcessive && GetRCObject(rc) == rcRange))
                {
                    RecordNoMatch(name, refSeq->name, rpos);
                }
                if (GetRCState(rc) == rcViolated && GetRCObject(rc) == rcConstraint) {
                    rc = LogNoMatch(name, refSeq->name, (unsigned)rpos, (unsigned)matches);
                }
                else if (GetRCObject(rc) == rcData && GetRCState(rc) == rcInvalid) {
                    (void)PLOGERR(klogWarn, (klogWarn, rc, "Spot '$(name)': bad alignment to reference '$(ref)' at $(pos)", "name=%s,ref=%s,pos=%u", name, refSeq->name, rpos));
                    CheckLimitAndLogError();
                }
                else if (GetRCObject(rc) == rcData) {
                    (void)PLOGERR(klogWarn, (klogWarn, rc, "Spot '$(name)': bad alignment to reference '$(ref)' at $(pos)", "name=%s,ref=%s,pos=%u", name, refSeq->name, rpos));
                    rc = CheckLimitAndLogError();
                }
                else {
                    (void)PLOGERR(klogWarn, (klogWarn, rc, "Spot '$(name)': error reading reference '$(ref)' at $(pos)", "name=%s,ref=%s,pos=%u", name, refSeq->name, rpos));
                    rc = CheckLimitAndLogError();
                }
                if (rc) goto LOOP_END;
            }
        }
        if (isColorSpace) {
            /* must be after ReferenceRead */
            BAMAlignmentGetCSKey(rec, &cskey);/*BAM*/
            BAMAlignmentGetCSSequence(rec, seqDNA, csSeqLen);/*BAM*/
            if (!aligned && !G.useQUAL) {
                uint8_t const *squal;
                uint8_t qoffset = 0;
                
                rc = BAMAlignmentGetCSQuality(rec, &squal, &qoffset);/*BAM*/
                if (rc) {
                    (void)PLOGERR(klogErr, (klogErr, rc, "Spot '$(name)': length of colorspace quality does not match sequence", "name=%s", name));
                    goto LOOP_END;
                }
                if (qoffset) {
                    unsigned i;
                    
                    for (i = 0; i < csSeqLen; ++i)
                        qual[i] = squal[i] - qoffset;
                }
                else
                    memcpy(qual, squal, csSeqLen);
                readlen = csSeqLen;
            }
        }
        
        if (aligned) {
            if (G.editAlignedQual ) EditAlignedQualities  (qual, AR_HAS_MISMATCH(data), readlen);
            if (G.keepMismatchQual) EditUnalignedQualities(qual, AR_HAS_MISMATCH(data), readlen);
        }
        else if (isPrimary) {
            switch (AR_READNO(data)) {
            case 1:
                value->unaligned_1 = 1;
                break;
            case 2:
                value->unaligned_2 = 1;
                break;
            default:
                break;
            }
        }
        if (isPrimary) {
            switch (AR_READNO(data)) {
            case 1:
                if (CTX_VALUE_GET_P_ID(*value, 0) == 0 && aligned) {
                    data.alignId = ++ctx->primaryId;
                    CTX_VALUE_SET_P_ID(*value, 0, data.alignId);
                }
                break;
            case 2:
                if (CTX_VALUE_GET_P_ID(*value, 1) == 0 && aligned) {
                    data.alignId = ++ctx->primaryId;
                    CTX_VALUE_SET_P_ID(*value, 1, data.alignId);
                }
                break;
            default:
                break;
            }
        }
        if (mated) {
            if (isPrimary || !originally_aligned) {
                if (CTX_VALUE_GET_S_ID(*value) != 0) {
                    (void)PLOGMSG(klogWarn, (klogWarn, "Spot '$(name)' has already been assigned a spot id", "name=%.*s", namelen, name));
                }
                else if (!value->has_a_read) {
                    /* new mated fragment - do spot assembly */
                    unsigned sz;
                    uint64_t    fragmentId;
                    FragmentInfo fi;
                    KMemBank *frags;
                    int32_t mate_refSeqId = -1;
                    int64_t pnext = 0;
                    
                    memset(&fi, 0, sizeof(fi));
                    fi.aligned = aligned;
                    fi.ti = ti;
                    fi.orientation = AR_REF_ORIENT(data);
                    fi.otherReadNo = AR_READNO(data);
                    fi.sglen   = strlen(spotGroup);
                    fi.readlen = readlen;
                    fi.cskey = cskey;
                    fi.is_bad = (flags & BAMFlags_IsLowQuality) != 0;/*BAM*/
                    sz = sizeof(fi) + 2*fi.readlen + fi.sglen;
                    if (align) {
                        BAMAlignmentGetMateRefSeqId(rec, &mate_refSeqId);/*BAM*/
                        BAMAlignmentGetMatePosition(rec, &pnext);/*BAM*/
                    }
                    if(align && mate_refSeqId == refSeqId && pnext > 0 && pnext!=rpos /*** weird case in some bams**/){ 
                        frags = ctx->fragsBoth;
                        rc = KMemBankAlloc(frags, &fragmentId, sz, 0);
                        value->fragmentId = fragmentId*2;
                        fcountBoth++;
                    } else {
                        frags = ctx->fragsOne;
                        rc = KMemBankAlloc(frags, &fragmentId, sz, 0);
                        value->fragmentId = fragmentId*2+1;
                        fcountOne++;
                    }
                    if (rc) {
                        (void)LOGERR(klogErr, rc, "KMemBankAlloc failed");
                        goto LOOP_END;
                    }
                    /*printf("IN:%10d\tcnt2=%ld\tcnt1=%ld\n",value->fragmentId,fcountBoth,fcountOne);*/
                    
                    rc = KDataBufferResize(&fragBuf, sz);
                    if (rc) {
                        (void)LOGERR(klogErr, rc, "Failed to resize fragment buffer");
                        goto LOOP_END;
                    }
                    {{
                        uint8_t *dst = (uint8_t*) fragBuf.base;
                        memcpy(dst,&fi,sizeof(fi));
                        dst += sizeof(fi);
                        COPY_READ((char *)dst, seqDNA, fi.readlen, (isColorSpace && !aligned) ? 0 : fi.orientation);
                        dst += fi.readlen;
                        COPY_QUAL(dst, qual, fi.readlen, (isColorSpace && !aligned) ? 0 : fi.orientation);
                        dst += fi.readlen;
                        memcpy(dst,spotGroup,fi.sglen);
                    }}
                    rc = KMemBankWrite(frags, fragmentId, 0, fragBuf.base, sz, &rsize);
                    if (rc) {
                        (void)PLOGERR(klogErr, (klogErr, rc, "KMemBankWrite failed writing fragment $(id)", "id=%u", fragmentId));
                        goto LOOP_END;
                    }
                    value->has_a_read = 1;
                }
                else if (value->fragmentId != 0 ) {
                    /* might be second fragment */
                    uint64_t sz;
                    FragmentInfo *fip;
                    KMemBank *frags;
                    
                    if(value->fragmentId & 1) frags = ctx->fragsOne;
                    else               frags = ctx->fragsBoth; 
                    
                    rc=KMemBankSize(frags, value->fragmentId>>1, &sz);
                    if (rc) {
                        (void)PLOGERR(klogErr, (klogErr, rc, "KMemBankSize failed on fragment $(id)", "id=%u", value->fragmentId>>1));
                        goto LOOP_END;
                    }
                    rc=KDataBufferResize(&fragBuf, (size_t)sz);
                    if (rc) {
                        (void)PLOGERR(klogErr, (klogErr, rc, "Failed to resize fragment buffer", ""));
                        goto LOOP_END;
                    }
                    rc=KMemBankRead(frags, value->fragmentId>>1, 0, fragBuf.base, sz, &rsize);
                    if (rc) {
                        (void)PLOGERR(klogErr, (klogErr, rc, "KMemBankRead failed on fragment $(id)", "id=%u", value->fragmentId>>1));
                        goto LOOP_END;
                    }
                    
                    assert( rsize == sz );
                    fip = (FragmentInfo *) fragBuf.base;
                    if(AR_READNO(data) != fip->otherReadNo) {
                        /* mate found */
                        unsigned readLen[2];
                        unsigned read1 = 0;
                        unsigned read2 = 1;
                        uint8_t  *src  = (uint8_t*) fip + sizeof(*fip);
                        
                        if (AR_READNO(data) < fip->otherReadNo) {
                            read1 = 1;
                            read2 = 0;
                        }
                        readLen[read1] = fip->readlen;
                        readLen[read2] = readlen;
                        rc = SequenceRecordInit(&srec, 2, readLen);
                        if (rc) {
                            (void)PLOGERR(klogErr, (klogErr, rc, "Failed resizing sequence record buffer", ""));
                            goto LOOP_END;
                        }
                        srec.ti[read1] = fip->ti;
                        srec.aligned[read1] = fip->aligned;
                        srec.is_bad[read1] = fip->is_bad;
                        srec.orientation[read1] = fip->orientation;
                        srec.cskey[read1] = fip->cskey;
                        memcpy(srec.seq + srec.readStart[read1], src, fip->readlen);
                        src += fip->readlen;
                        memcpy(srec.qual + srec.readStart[read1], src, fip->readlen);
                        src += fip->readlen;
                        
                        srec.orientation[read2] = AR_REF_ORIENT(data);
                        COPY_READ(srec.seq + srec.readStart[read2], seqDNA, srec.readLen[read2], (isColorSpace && !aligned) ? 0 : srec.orientation[read2]);
                        COPY_QUAL(srec.qual + srec.readStart[read2], qual, srec.readLen[read2],  (isColorSpace && !aligned) ? 0 : srec.orientation[read2]);

                        srec.keyId = keyId;
                        srec.is_bad[read2] = (flags & BAMFlags_IsLowQuality) != 0;
                        srec.aligned[read2] = aligned;
                        srec.cskey[read2] = cskey;
                        srec.ti[read2] = ti;
                        
                        srec.spotGroup = spotGroup;
                        srec.spotGroupLen = strlen(spotGroup);
                        if (value->pcr_dup && (srec.is_bad[0] || srec.is_bad[1])) {
                            filterFlagConflictRecords++;
                            if(filterFlagConflictRecords < MAX_WARNINGS_FLAG_CONFLICT){
                                (void)PLOGMSG(klogWarn, (klogWarn, "Spot '$(name)': both 0x400 and 0x200 flag bits set, only 0x400 will be saved", "name=%s", name));
                            } else if(filterFlagConflictRecords == MAX_WARNINGS_FLAG_CONFLICT){
                                (void)PLOGMSG(klogWarn, (klogWarn, "Last reported warning: Spot '$(name)': both 0x400 and 0x200 flag bits set, only 0x400 will be saved", "name=%s", name));
                            }
                        }
                        rc = SequenceWriteRecord(seq, &srec, isColorSpace, value->pcr_dup, value->platform);
                        if (rc) {
                            (void)LOGERR(klogErr, rc, "SequenceWriteRecord failed");
                            goto LOOP_END;
                        }
                        CTX_VALUE_SET_S_ID(*value, ++ctx->spotId);
                        if(value->fragmentId & 1){
                            fcountOne--;
                        } else {
                            fcountBoth--;
                        }
                        /*	printf("OUT:%9d\tcnt2=%ld\tcnt1=%ld\n",value->fragmentId,fcountBoth,fcountOne);*/
                        rc = KMemBankFree(frags, value->fragmentId>>1);
                        if (rc) {
                            (void)PLOGERR(klogErr, (klogErr, rc, "KMemBankFree failed on fragment $(id)", "id=%u", value->fragmentId>>1));
                            goto LOOP_END;
                        }
                        value->fragmentId = 0;
                    }
                }
            }
            if (!isPrimary && aligned) {
                int32_t bam_mrid;
                int64_t mpos;
                int64_t mrid;
                int64_t tlen;
                
                BAMAlignmentGetMatePosition(rec, &mpos);/*BAM*/
                BAMAlignmentGetMateRefSeqId(rec, &bam_mrid);/*BAM*/
                BAMAlignmentGetInsertSize(rec, &tlen);/*BAM*/
                
                if (mpos >= 0 && bam_mrid >= 0 && tlen != 0) {
                    BAMRefSeq const *mref;/*BAM*/
                    
                    BAMFileGetRefSeq(bam, bam_mrid, &mref);/*BAM*/
                    if (mref) {
                        rc_t rc_temp = ReferenceGet1stRow(ref, &mrid, mref->name);
                        if (rc_temp == 0) {
                            data.mate_ref_pos = mpos;
                            data.template_len = tlen;
                            data.mate_ref_orientation = (flags & BAMFlags_MateIsReverse) ? 1 : 0;
                        }
                        else {
                            (void)PLOGERR(klogWarn, (klogWarn, rc_temp, "Failed to get refID for $(name)", "name=%s", mref->name));
                            mrid = 0;
                        }
                        data.mate_ref_id = mrid;
                    }
                }
            }
        }
        else if (wasInserted & (isPrimary || !originally_aligned)) {
            /* new unmated fragment - no spot assembly */
            unsigned readLen[1];
            
            readLen[0] = readlen;
            rc = SequenceRecordInit(&srec, 1, readLen);
            if (rc) {
                (void)PLOGERR(klogErr, (klogErr, rc, "Failed resizing sequence record buffer", ""));
                goto LOOP_END;
            }
            srec.ti[0] = ti;
            srec.aligned[0] = aligned;
            srec.is_bad[0] = (flags & BAMFlags_IsLowQuality) != 0;
            srec.orientation[0] = AR_REF_ORIENT(data);
            srec.cskey[0] = cskey;
            COPY_READ(srec.seq  + srec.readStart[0], seqDNA, readlen, (isColorSpace && !aligned) ? 0 : srec.orientation[0]);
            COPY_QUAL(srec.qual + srec.readStart[0], qual, readlen, (isColorSpace && !aligned) ? 0 : srec.orientation[0]);
	     
            srec.keyId = keyId;
            
            srec.spotGroup = spotGroup;
            srec.spotGroupLen = strlen(spotGroup);
            if (value->pcr_dup && srec.is_bad[0]) {
                filterFlagConflictRecords++;
                if (filterFlagConflictRecords < MAX_WARNINGS_FLAG_CONFLICT) {
                    (void)PLOGMSG(klogWarn, (klogWarn, "Spot '$(name)': both 0x400 and 0x200 flag bits set, only 0x400 will be saved", "name=%s", name));
                }
                else if (filterFlagConflictRecords == MAX_WARNINGS_FLAG_CONFLICT) {
                    (void)PLOGMSG(klogWarn, (klogWarn, "Last reported warning: Spot '$(name)': both 0x400 and 0x200 flag bits set, only 0x400 will be saved", "name=%s", name));
		}
            }
            rc = SequenceWriteRecord(seq, &srec, isColorSpace, value->pcr_dup, value->platform);
            if (rc) {
                (void)PLOGERR(klogErr, (klogErr, rc, "SequenceWriteRecord failed", ""));
                goto LOOP_END;
            }
            CTX_VALUE_SET_S_ID(*value, ++ctx->spotId);
            value->fragmentId = 0;
        }
        
        if (aligned) {
            if (value->alignmentCount[AR_READNO(data) - 1] < 254)
                ++value->alignmentCount[AR_READNO(data) - 1];
            ++ctx->alignCount;
            
            assert(keyId >> 32 < ctx->key2id_count);
            assert((uint32_t)keyId < ctx->idCount[keyId >> 32]);
            
            rc = AlignmentWriteRecord(align, &data);
            if (rc == 0) {
                if (!isPrimary)
                    data.alignId = ++ctx->secondId;
                
                rc = ReferenceAddAlignId(ref, data.alignId, isPrimary);
                if (rc) {
                    (void)PLOGERR(klogErr, (klogErr, rc, "ReferenceAddAlignId failed", ""));
                }
                else {
                    *had_alignments = true;
                }
            }
            else {
                (void)PLOGERR(klogErr, (klogErr, rc, "AlignmentWriteRecord failed", ""));
            }
        }
        /**************************************************************/
        
    LOOP_END:
        BAMAlignmentRelease(rec);
        ++reccount;
        if (G.maxAlignCount > 0 && reccount >= G.maxAlignCount)
            break;
        if (rc == 0)
            *had_sequences = true;
    }
    if (filterFlagConflictRecords > 0) {
        (void)PLOGMSG(klogWarn, (klogWarn, "$(cnt1) out of $(cnt2) records contained warning : both 0x400 and 0x200 flag bits set, only 0x400 will be saved", "cnt1=%lu,cnt2=%lu", filterFlagConflictRecords,recordsProcessed));
    }
    if (rc == 0 && recordsProcessed == 0) {
        (void)LOGMSG(klogWarn, (G.limit2config || G.refFilter != NULL) ? 
                     "All records from the file were filtered out" :
                     "The file contained no records that were processed.");
        rc = RC(rcAlign, rcFile, rcReading, rcData, rcEmpty);
    }
    BAMFileRelease(bam);
    KDataBufferWhack(&buf);
    KDataBufferWhack(&fragBuf);
    KDataBufferWhack(&srec.storage);
    KDataBufferWhack(&cigBuf);
    return rc;
}

static rc_t WriteSoloFragments(context_t *ctx, Sequence *seq)
{
    uint32_t i;
    unsigned j;
    uint32_t fcountOne  = 0;
    uint32_t fcountBoth = 0;
    uint64_t idCount = 0;
    rc_t rc;
    KDataBuffer fragBuf;
    SequenceRecord srec;
    
    ++ctx->pass;
    memset(&srec, 0, sizeof(srec));
    
    rc = KDataBufferMake(&fragBuf, 8, 0);
    if (rc) {
        (void)LOGERR(klogErr, rc, "KDataBufferMake failed");
        return rc;
    }
    for (idCount = 0, j = 0; j < ctx->key2id_count; ++j) {
        idCount += ctx->idCount[j];
    }
    KLoadProgressbar_Append(ctx->progress[ctx->pass - 1], idCount);
    
    for (idCount = 0, j = 0; j < ctx->key2id_count; ++j) {
        for (i = 0; i != ctx->idCount[j]; ++i, ++idCount) {
            uint64_t const keyId = ((uint64_t)j << 32) | i;
            ctx_value_t *value;
            size_t rsize;
            uint64_t id;
            uint64_t sz;
            unsigned readLen[2];
            unsigned read = 0;
            FragmentInfo const *fip;
            uint8_t const *src;
            KMemBank *frags;
            
            rc = MMArrayGet(ctx->id2value, (void **)&value, keyId);
            if (rc)
                break;
            KLoadProgressbar_Process(ctx->progress[ctx->pass - 1], 1, false);
            if (value->fragmentId == 0)
                continue;
            if (value->fragmentId & 1) {
                frags = ctx->fragsOne;
                fcountOne++;
            }
            else {
                frags = ctx->fragsBoth; 
                fcountBoth++;
            }
            id = value->fragmentId >> 1;
            
            rc = KMemBankSize(frags, id, &sz);
            if (rc) {
                (void)LOGERR(klogErr, rc, "KMemBankSize failed");
                break;
            }
            rc = KDataBufferResize(&fragBuf, (size_t)sz);
            if (rc) {
                (void)LOGERR(klogErr, rc, "KDataBufferResize failed");
                break;
            }
            rc = KMemBankRead(frags, id, 0, fragBuf.base, sz, &rsize);
            if (rc) {
                (void)LOGERR(klogErr, rc, "KMemBankRead failed");
                break;
            }
            assert( rsize == sz );
            fip = (FragmentInfo const *)fragBuf.base;
            src = (uint8_t const *)&fip[1];
            
            readLen[0] = readLen[1] = 0;
            if (!value->unmated && (   (fip->aligned && CTX_VALUE_GET_P_ID(*value, 0) == 0)
                                    || (value->unaligned_2)))
            {
                read = 1;
            }
            
            readLen[read] = fip->readlen;
            rc = SequenceRecordInit(&srec, value->unmated ? 1 : 2, readLen);
            if (rc) {
                (void)LOGERR(klogErr, rc, "SequenceRecordInit failed");
                break;
            }
            
            srec.ti[read] = fip->ti;
            srec.aligned[read] = fip->aligned;
            srec.is_bad[read] = fip->is_bad;
            srec.orientation[read] = fip->orientation;
            srec.cskey[read] = fip->cskey;
            memcpy(srec.seq + srec.readStart[read], src, srec.readLen[read]);
            src += fip->readlen;
            memcpy(srec.qual + srec.readStart[read], src, srec.readLen[read]);
            src += fip->readlen;
            srec.spotGroup = (char *)src;
            srec.spotGroupLen = fip->sglen;
            srec.keyId = keyId;
            
            rc = SequenceWriteRecord(seq, &srec, ctx->isColorSpace, value->pcr_dup, value->platform);
            if (rc) {
                (void)LOGERR(klogErr, rc, "SequenceWriteRecord failed");
                break;
            }
            /*rc = KMemBankFree(frags, id);*/
            CTX_VALUE_SET_S_ID(*value, ++ctx->spotId);
        }
    }
    /*printf("DONE_SOLO:\tcnt2=%d\tcnt1=%d\n",fcountBoth,fcountOne);*/
    KDataBufferWhack(&fragBuf);
    KDataBufferWhack(&srec.storage);
    return rc;
}

static rc_t SequenceUpdateAlignInfo(context_t *ctx, Sequence *seq)
{
    rc_t rc = 0;
    uint64_t row;
    const ctx_value_t *value;
    uint64_t keyId;
    
    ++ctx->pass;
    KLoadProgressbar_Append(ctx->progress[ctx->pass - 1], ctx->spotId + 1);
    
    for (row = 1; row <= ctx->spotId; ++row) {
        rc = SequenceReadKey(seq, row, &keyId);
        if (rc) {
            (void)PLOGERR(klogErr, (klogErr, rc, "Failed to get key for row $(row)", "row=%u", (unsigned)row));
            break;
        }
        rc = MMArrayGet(ctx->id2value, (void **)&value, keyId);
        if (rc) {
            (void)PLOGERR(klogErr, (klogErr, rc, "Failed to read info for row $(row), index $(idx)", "row=%u,idx=%u", (unsigned)row, (unsigned)keyId));
            break;
        }
        if (row != CTX_VALUE_GET_S_ID(*value)) {
            rc = RC(rcApp, rcTable, rcWriting, rcData, rcUnexpected);
            (void)PLOGMSG(klogErr, (klogErr, "Unexpected spot id $(spotId) for row $(row), index $(idx)", "spotId=%u,row=%u,idx=%u", (unsigned)CTX_VALUE_GET_S_ID(*value), (unsigned)row, (unsigned)keyId));
            break;
        }
        {{
            int64_t primaryId[2];
            uint64_t const spotId = CTX_VALUE_GET_S_ID(*value);
            
            primaryId[0] = CTX_VALUE_GET_P_ID(*value, 0);
            primaryId[1] = CTX_VALUE_GET_P_ID(*value, 1);
            
            rc = SequenceUpdateAlignData(seq, row, value->unmated ? 1 : 2,
                                         primaryId,
                                         value->alignmentCount);
        }}
        if (rc) {
            (void)LOGERR(klogErr, rc, "Failed updating Alignment data in sequence table");
            break;
        }
        KLoadProgressbar_Process(ctx->progress[ctx->pass - 1], 1, false);
    }
    return rc;
}

static rc_t AlignmentUpdateSpotInfo(context_t *ctx, Alignment *align)
{
    rc_t rc;
    uint64_t keyId;
    
    ++ctx->pass;

    KLoadProgressbar_Append(ctx->progress[ctx->pass - 1], ctx->alignCount);

    rc = AlignmentStartUpdatingSpotIds(align);
    while (rc == 0 && (rc = Quitting()) == 0) {
        ctx_value_t const *value;
        
        rc = AlignmentGetSpotKey(align, &keyId);
        if (rc) {
            if (GetRCObject(rc) == rcRow && GetRCState(rc) == rcNotFound)
                rc = 0;
            break;
        }
        assert(keyId >> 32 < ctx->key2id_count);
        assert((uint32_t)keyId < ctx->idCount[keyId >> 32]);
        rc = MMArrayGet(ctx->id2value, (void **)&value, keyId);
        if (rc == 0) {
            int64_t const spotId = CTX_VALUE_GET_S_ID(*value);
            
            if (spotId == 0) {
                (void)PLOGMSG(klogWarn, (klogWarn, "Spot '$(id)' was never assigned a spot id, probably has no primary alignments", "id=%lx", keyId));
                /* (void)PLOGMSG(klogWarn, (klogWarn, "Spot #$(i): { $(s) }", "i=%lu,s=%s", keyId, Print_ctx_value_t(value))); */
            }
            rc = AlignmentWriteSpotId(align, spotId);
        }
        KLoadProgressbar_Process(ctx->progress[ctx->pass - 1], 1, false);
    }
    return rc;
}

static rc_t ArchiveBAM(VDBManager *mgr, VDatabase *db,
                       unsigned bamFiles, char const *bamFile[],
                       unsigned seqFiles, char const *seqFile[],
                       bool *has_alignments)
{
    rc_t rc = 0;
    rc_t rc2;
    Reference ref;
    Sequence seq;
    Alignment *align;
    context_t ctx;
    bool has_sequences = false;
    unsigned i;
    
    *has_alignments = false;
    rc = ReferenceInit(&ref, mgr, db);
    if (rc)
        return rc;
    
    if (G.onlyVerifyReferences) {
        for (i = 0; i < bamFiles && rc == 0; ++i) {
            rc = ProcessBAM(bamFile[i], NULL, db, &ref, NULL, NULL, NULL, NULL);
        }
        ReferenceWhack(&ref, false);
        return rc;
    }
    SequenceInit(&seq, db);
    align = AlignmentMake(db);
    
    rc = SetupContext(&ctx, bamFiles + seqFiles);
    if (rc)
        return rc;
    
    ++ctx.pass;
    for (i = 0; i < bamFiles && rc == 0; ++i) {
        bool this_has_alignments = false;
        bool this_has_sequences = false;
        
        rc = ProcessBAM(bamFile[i], &ctx, db, &ref, &seq, align, &this_has_alignments, &this_has_sequences);
        *has_alignments |= this_has_alignments;
        has_sequences |= this_has_sequences;
    }
    for (i = 0; i < seqFiles && rc == 0; ++i) {
        bool this_has_alignments = false;
        bool this_has_sequences = false;
        
        rc = ProcessBAM(seqFile[i], &ctx, db, &ref, &seq, align, &this_has_alignments, &this_has_sequences);
        *has_alignments |= this_has_alignments;
        has_sequences |= this_has_sequences;
    }
/*** No longer need memory for key2id ***/
    for (i = 0; i != ctx.key2id_count; ++i) {
        KBTreeDropBacking(ctx.key2id[i]);
        KBTreeRelease(ctx.key2id[i]);
        ctx.key2id[i] = NULL;
    }
    free(ctx.key2id_names);
    ctx.key2id_names = NULL;
/*******************/

    if (has_sequences) {
        if (rc == 0 && (rc = Quitting()) == 0) {
            (void)LOGMSG(klogInfo, "Writing unpaired sequences");
            rc = WriteSoloFragments(&ctx, &seq);
            ContextReleaseMemBank(&ctx);
            if (rc == 0) {
                rc = SequenceDoneWriting(&seq);
                if (rc == 0) {
                    (void)LOGMSG(klogInfo, "Updating sequence alignment info");
                    rc = SequenceUpdateAlignInfo(&ctx, &seq);
                }
            }
        }
    }
    
    if (*has_alignments && rc == 0 && (rc = Quitting()) == 0) {
        (void)LOGMSG(klogInfo, "Writing alignment spot ids");
        rc = AlignmentUpdateSpotInfo(&ctx, align);
    }
    rc2 = AlignmentWhack(align, *has_alignments && rc == 0 && (rc = Quitting()) == 0);
    if (rc == 0)
        rc = rc2;

    rc2 = ReferenceWhack(&ref, *has_alignments && rc == 0 && (rc = Quitting()) == 0);
    if (rc == 0)
        rc = rc2;
    
    SequenceWhack(&seq, rc == 0);
    
    ContextRelease(&ctx);

    if (rc == 0) {
        (void)LOGMSG(klogInfo, "Successfully loaded all files");
    }
    return rc;
}

rc_t WriteLoaderSignature(KMetadata *meta, char const progName[])
{
    KMDataNode *node;
    rc_t rc = KMetadataOpenNodeUpdate(meta, &node, "/");
    
    if (rc == 0) {
        rc = KLoaderMeta_Write(node, progName, __DATE__, "BAM", KAppVersion());
        KMDataNodeRelease(node);
    }
    if (rc) {
        (void)LOGERR(klogErr, rc, "Cannot update loader meta");
    }
    return rc;
}

rc_t OpenPath(char const path[], KDirectory **dir)
{
    KDirectory *p;
    rc_t rc = KDirectoryNativeDir(&p);
    
    if (rc == 0) {
        rc = KDirectoryOpenDirUpdate(p, dir, false, path);
        KDirectoryRelease(p);
    }
    return rc;
}

static
rc_t ConvertDatabaseToUnmapped(VDatabase *db)
{
    VTable* tbl;
    rc_t rc = VDatabaseOpenTableUpdate(db, &tbl, "SEQUENCE");
    if (rc == 0) 
    {
        VTableRenameColumn(tbl, false, "CMP_ALTREAD", "ALTREAD");
        VTableRenameColumn(tbl, false, "CMP_READ", "READ");
        VTableRenameColumn(tbl, false, "CMP_ALTCSREAD", "ALTCSREAD");
        VTableRenameColumn(tbl, false, "CMP_CSREAD", "CSREAD");
        rc = VTableRelease(tbl);
    }
    return rc;
}
rc_t run(char const progName[],
         unsigned bamFiles, char const *bamFile[],
         unsigned seqFiles, char const *seqFile[])
{
    VDBManager *mgr;
    rc_t rc;
    rc_t rc2;
    char const *db_type = G.expectUnsorted ? "NCBI:align:db:alignment_unsorted" : "NCBI:align:db:alignment_sorted";
    
    rc = VDBManagerMakeUpdate(&mgr, NULL);
    if (rc) {
        (void)LOGERR (klogErr, rc, "failed to create VDB Manager!");
    }
    else {
        bool has_alignments = false;
            
        if (G.onlyVerifyReferences) {
            rc = ArchiveBAM(mgr, NULL, bamFiles, bamFile, 0, NULL, &has_alignments);
        }
        else {
            VSchema *schema;
        
            rc = VDBManagerMakeSchema(mgr, &schema);
            if (rc) {
                (void)LOGERR (klogErr, rc, "failed to create schema");
            }
            else {
                (void)(rc = VSchemaAddIncludePath(schema, G.schemaIncludePath));
                rc = VSchemaParseFile(schema, G.schemaPath);
                if (rc) {
                    (void)PLOGERR(klogErr, (klogErr, rc, "failed to parse schema file $(file)", "file=%s", G.schemaPath));
                }
                else {
                    VDatabase *db;
                    
                    rc = VDBManagerCreateDB(mgr, &db, schema, db_type,
                                            kcmInit + kcmMD5, G.outpath);
                    rc2 = VSchemaRelease(schema);
                    if (rc2)
                        (void)LOGERR(klogWarn, rc2, "Failed to release schema");
                    if (rc == 0)
                        rc = rc2;
                    if (rc == 0) {
                        rc = ArchiveBAM(mgr, db, bamFiles, bamFile, seqFiles, seqFile, &has_alignments);
                        if (rc == 0 && !has_alignments) {
                            rc = ConvertDatabaseToUnmapped(db);
                        }
                        
                        rc2 = VDatabaseRelease(db);
                        if (rc2)
                            (void)LOGERR(klogWarn, rc2, "Failed to close database");
                        if (rc == 0)
                            rc = rc2;
                        
                        if (rc == 0) {
                            KMetadata *meta;
                            KDBManager *kmgr;
                            
                            rc = VDBManagerOpenKDBManagerUpdate(mgr, &kmgr);
                            if (rc == 0) {
                                KDatabase *kdb;
                                
                                rc = KDBManagerOpenDBUpdate(kmgr, &kdb, G.outpath);
                                if (rc == 0) {
                                    rc = KDatabaseOpenMetadataUpdate(kdb, &meta);
                                    KDatabaseRelease(kdb);
                                }
                                KDBManagerRelease(kmgr);
                            }
                            if (rc == 0) {
                                rc = WriteLoaderSignature(meta, progName);
                                KMetadataRelease(meta);
                            }
                        }
                    }
                }
            }
        }
        rc2 = VDBManagerRelease(mgr);
        if (rc2)
            (void)LOGERR(klogWarn, rc2, "Failed to release VDB Manager");
        if (rc == 0)
            rc = rc2;
    }
    return rc;
}
