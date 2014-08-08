/*==============================================================================
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
*/
#include <align/extern.h>

#include <kapp/main.h>
#include <klib/log.h>
#include <klib/rc.h>
#include <klib/sort.h>
#include <klib/data-buffer.h>
#include <klib/container.h>
#include <klib/checksum.h>
#include <klib/text.h>
#include <kfs/mmap.h>
#include <kfs/file.h>
#include <kdb/manager.h>
#include <vdb/database.h>
#include <vdb/table.h>
#include <vdb/cursor.h>
#include <vdb/manager.h>
#include <vdb/vdb-priv.h>
#include <sra/sradb.h>

#include <align/writer-reference.h>
#include <align/writer-refseq.h>
#include <align/refseq-mgr.h>
#include "refseq-mgr-priv.h"
#include "writer-ref.h"
#include "reader-cmn.h"
#include "reference-cmn.h"
#include "debug.h"
#include <os-native.h>
#include <sysalloc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/*
 * ReferenceSeq objects:
 *  ReferenceSeq objects may be unattached, i.e. they might not yet represent an
 *  actual sequence.
 *
 *  ReferenceSeq objects may be attached, i.e. they represent a sequence from
 *  either RefSeq or a fasta file.
 *
 *  A ReferenceSeq object may be refered to by more than one id, but a
 *  ReferenceSeq object has only one canonical id.
 *
 *  More than one ReferenceSeq object may be associated with the same seqId.
 *
 *  More than one ReferenceSeq object may be attached to the same sequence.
 *  This will cause the REFERENCE table to have more than one copy of the
 *  sequence.
 *
 *  ReferenceSeq objects may be created from the config file.  These objects
 *  will have an id, a seqId, but no fastaSeqId.  These are unattached.
 *
 *  ReferenceSeq objects may be created from explicit fasta files.  These
 *  objects will have a fastaSeqId, but *** NO id OR seqId ***. These are
 *  attached.
 *
 *  ReferenceSeq objects may be created on the fly by requesting an id that
 *  isn't already in the collection.  These objects will have the requested id.
 *
 * When a reference is requested (by id):
 *  Resolve the id to a ReferenceSeq object.
 *  If the object is unattached, attach it to a sequence.
 *  If the sequence is not yet written to the REFERENCE table, write it to the
 *  REFERENCE table.  NAME gets id; SEQID gets seqId unless seqId is null, then
 *  SEQID gets id.
 *
 * Resolving id's to ReferenceSeq objects:
 *  Search the id index and if the found object is attached, return it.
 *  Search the identifiers in the fastaSeqIds or seqIds.
 *  If different objects were found from both searches, use sequence length and
 *  MD5 to break the tie (if both match then use RefSeq).  If no sequence length
 *  or MD5 then fail.
 *  If no objects were found from either search, then create a new unattached
 *  ReferenceSeq object.
 *  If the object's id is null, set it to the id.
 *  It the object was not found in the id index, add it.
 *
 * Attaching ReferenceSeq objects to sequences:
 *  Search RefSeq for seqId.
 *  Else search RefSeq for id.
 *  Else search for seqId in the loaded fasta files.
 *  Else search data directory for id.fasta or id.fa; load it or fail.
 *  Else search data directory for seqId.fasta or seqId.fa; load it or fail.
 *  Else fail.
 *  If failed, mark the object as dead.
 *
 *
 *
 * Config file:
 *  The config file consists of lines containing whitespace (preferably tab)
 *  seperated fields.  The fields are:
 *      NAME (unique)
 *      SEQID
 *      extra (optional)
 *
 *  There is one ReferenceSeq object created per record in the config file.
 *  NAME is stored in id; SEQID is stored in seqId; if extra contains the word
 *  'circular' (case-insensitive), true is stored in circular.  These
 *  ReferenceSeq object are created in the unattached state, i.e. not attached
 *  to a fasta file or a RefSeq library object.
 *
 * Fasta files:
 *  Fasta file consists of one of more sequences.  A sequence in a fasta file
 *  consists of a seqid line followed by lines containing the bases of the
 *  sequence.  A seqid line starts with '>' and the next word (whitespace
 *  delimited) is the seqid.  The seqid may consist of '|' delimited identifiers
 *  (this is purposely vague).  The fasta seqid is stored in fastaSeqId.
 *
 * Fasta files may be loaded explicitly:
 *  When a fasta file is loaded explicitly, a new ReferenceSeq object is created
 *  (with id == NULL) for each sequence found in the file.
 *
 * Fasta files may be loaded implicitly:
 *  When an id can't be found in the set of ReferenceSeq objects and can't be
 *  found as an accession by RefSeq, an attempt is made to load a fasta file
 *  named <id>.fasta or <id>.fa in the directory given to the constructor.  If
 *  this succeeds, a new ReferenceSeq object with the given id is attached to
 *  the sequence.  In this situation, to avoid ambiquity, there can be only one
 *  sequence in the fasta file.
 *
 */

enum ReferenceSeqType {
    rst_unattached,
    rst_local,
    rst_refSeqById,
    rst_refSeqBySeqId,
    rst_dead
};

struct ReferenceSeq {
    ReferenceMgr *mgr;
    char *id;
    char *seqId;
    char *fastaSeqId;
    /* ref table position */
    int64_t start_rowid;
    /* total reference length */
    INSDC_coord_len seq_len;
    int type;
    bool circular;
    uint8_t md5[16];
    union {
        struct {
            KDataBuffer buf;
            uint8_t const *data;
        } local;
        RefSeq const *refseq;
    } u;
};

struct ReferenceMgr {
    TableWriterRef const *writer;
    KDirectory const *dir;
    RefSeqMgr const *rmgr;
    VDatabase *db;
    ReferenceSeq *refSeq;
    
    int64_t ref_rowid;
    
    size_t cache;
    
    uint32_t options;
    uint32_t num_open_max;
    uint32_t num_open;
    uint32_t max_seq_len;
    
    KDataBuffer compress;
    KDataBuffer seq;
    KDataBuffer refSeqs;
    KDataBuffer refSeqsById;
};

typedef struct key_id_t {
    char const *key;
    int64_t id;
} key_id_t;

static
int CC key_id_cmp(const void *arg1, const void *arg2, void *data)
{
    key_id_t const *a = arg1;
    key_id_t const *b = arg2;
    
    return strcmp(a->key, b->key);
}

static
void CC ReferenceSeq_Whack(ReferenceSeq *self)
{
    if (self->type == rst_local) {
        KDataBufferWhack(&self->u.local.buf);
    }
    else if (self->type == rst_refSeqById || self->type == rst_refSeqBySeqId) {
        RefSeq_Release(self->u.refseq);
    }
    free(self->id);
    free(self->seqId);
    free(self->fastaSeqId);
}

struct OpenConfigFile_ctx {
    char const *name;
    KDirectory const *dir;
    KFile const **kfp;
    rc_t rc;
};

static
bool OpenConfigFile(char const server[], char const volume[], void *Ctx)
{
    struct OpenConfigFile_ctx *ctx = Ctx;
    KDirectory const *dir;
    
    if( volume == NULL ) {
        ctx->rc = KDirectoryOpenDirRead(ctx->dir, &dir, false, "%s", server);
    } else {
        ctx->rc = KDirectoryOpenDirRead(ctx->dir, &dir, false, "%s/%s", server, volume);
    }
    if (ctx->rc == 0) {
        ctx->rc = KDirectoryOpenFileRead(dir, ctx->kfp, ctx->name);
        KDirectoryRelease(dir);
        if (ctx->rc == 0) {
            return true;
        }
    }
    return false;
}

static
rc_t FindAndOpenConfigFile(RefSeqMgr const *const rmgr,
                           KDirectory const *const dir,
                           KFile const **const kfp, char const conf[])
{
    rc_t rc = KDirectoryOpenFileRead(dir, kfp, conf);
    
    if (rc) {
        struct OpenConfigFile_ctx ctx;

        ctx.name = conf;
        ctx.dir = dir;
        ctx.kfp = kfp;
        ctx.rc = 0;
        
        rc = RefSeqMgr_ForEachVolume(rmgr, OpenConfigFile, &ctx);
        if (rc == 0 && *kfp == NULL) {
            rc = RC(rcAlign, rcIndex, rcConstructing, rcFile, rcNotFound);
        }
    }
    return rc;
}

enum comparison_weights {
    no_match        = 0,
    substring_match = (1u << 0),
    expected_prefix = (1u << 1),
    exact_match     = (1u << 2),
    seq_len_match   = (1u << 3),
    md5_match       = (1u << 4)
};

static
unsigned str_weight(char const str[], char const qry[], unsigned const qry_len)
{
    char const *const fnd = strcasestr(str, qry);
    unsigned wt = no_match;
    
    if (fnd) {
        unsigned const fnd_len = strlen(fnd);
        unsigned const fndlen = (fnd_len > qry_len && fnd[qry_len] == '|') ? qry_len : fnd_len;
        
        if (fndlen == qry_len && (fnd == str || fnd[-1] == '|')) {
            wt |= substring_match;
            
            if (fnd == str) {
                if (fnd[fndlen] == '\0')
                    wt |= exact_match;
            }
            else {
                /* check for expected prefices */
                char const *ns = fnd - 1;
                
                while (ns != str && ns[-1] != '|')
                    --ns;
                
                if (   memcmp(ns, "ref|", 4) == 0
                    || memcmp(ns, "emb|", 4) == 0
                    || memcmp(ns, "dbg|", 4) == 0
                    || memcmp(ns, "tpg|", 4) == 0
                    || memcmp(ns, "tpe|", 4) == 0
                    || memcmp(ns, "tpd|", 4) == 0
                    || memcmp(ns, "gpp|", 4) == 0
                    || memcmp(ns,  "gb|", 3) == 0
                    )
                {
                    wt |= expected_prefix;
                }
            }
        }
    }
    return wt;
}

static
rc_t ReferenceMgr_AddId(ReferenceMgr *const self, char const ID[],
                        ReferenceSeq const *const obj)
{
    char *id = string_dup( ID, string_size( ID ) );
    unsigned const last_id = self->refSeqsById.elem_count;
    
    if (id) {
        rc_t rc = KDataBufferResize(&self->refSeqsById, last_id + 1);
        
        if (rc == 0) {
            key_id_t *kid = &((key_id_t *)self->refSeqsById.base)[last_id];
            
            kid->key = id;
            kid->id = obj - self->refSeq;
            
            ksort(self->refSeqsById.base, self->refSeqsById.elem_count, sizeof(key_id_t), key_id_cmp, NULL);
        }
        else
            free(id);
        
        return rc;
    }
    else
        return RC(rcAlign, rcIndex, rcInserting, rcMemory, rcExhausted);
}

static
key_id_t *ReferenceMgr_FindId(ReferenceMgr const *const self, char const id[])
{
    key_id_t qry;
    
    qry.key = id;
    qry.id = 0;
    
    return kbsearch(&qry, self->refSeqsById.base,
                    self->refSeqsById.elem_count,
                    sizeof(qry), key_id_cmp, NULL);
}

static
rc_t ReferenceMgr_NewReferenceSeq(ReferenceMgr *const self, ReferenceSeq **const rslt)
{
    unsigned const last_rs = self->refSeqs.elem_count;
    rc_t rc = KDataBufferResize(&self->refSeqs, last_rs + 1);
    
    if (rc) return rc;
    self->refSeq = self->refSeqs.base;
    
    *rslt = &self->refSeq[last_rs];
    memset(&self->refSeq[last_rs], 0, sizeof(self->refSeq[0]));
    self->refSeq[last_rs].mgr = self;

    return 0;
}

static
int CC config_cmp(void const *A, void const *B, void *Data)
{
    struct {
        unsigned id;
        unsigned seqId;
        unsigned extra;
        unsigned extralen;
    } const *const a = A;
    struct {
        unsigned id;
        unsigned seqId;
        unsigned extra;
        unsigned extralen;
    } const *const b = B;
    char const *const data = Data;
    
    return strcmp(&data[a->id], &data[b->id]);
}

static
rc_t ReferenceMgr_ProcessConf(ReferenceMgr *const self, char Data[], unsigned const len)
{
    rc_t rc;
    unsigned i;
    unsigned j;
    struct {
        unsigned id;
        unsigned seqId;
        unsigned extra;
        unsigned extralen;
    } *data, tmp;
    KDataBuffer buf;
    
    memset(&buf, 0, sizeof(buf));
    buf.elem_bits = sizeof(data[0]) * 8;
    
    for (j = i = 0; i < len; ++j) {
        unsigned lineEnd;
        unsigned id;
        unsigned acc;
        unsigned ii;
        
        for (lineEnd = i; lineEnd != len; ++lineEnd) {
            int const ch = Data[lineEnd];
            
            if (ch == '\n')
                break;
            if (ch == '\r')
                break;
        }
        if (i == lineEnd) {
            ++i;
            continue;
        }
        Data[lineEnd] = '\0';
        for (id = i; id != lineEnd; ++id) {
            int const ch = Data[id];
            
            if (!isspace(ch))
                break;
        }
        for (ii = id; ii != lineEnd; ++ii) {
            int const ch = Data[ii];
            
            if (isspace(ch)) {
                Data[ii++] = '\0';
                break;
            }
        }
        for (acc = ii; acc < lineEnd; ++acc) {
            int const ch = Data[acc];
            
            if (!isspace(ch))
                break;
        }
        if (acc >= lineEnd)
            return RC(rcAlign, rcFile, rcReading, rcFormat, rcInvalid);
        
        for (ii = acc; ii != lineEnd; ++ii) {
            int const ch = Data[ii];
            
            if (isspace(ch)) {
                Data[ii++] = '\0';
                break;
            }
        }
        tmp.id = id;
        tmp.seqId = acc;
        tmp.extra = ii;
        tmp.extralen = lineEnd > ii ? lineEnd - ii : 0;
        
        if ((rc = KDataBufferResize(&buf, buf.elem_count + 1)) != 0) return rc;
        data = buf.base;
        
        data[buf.elem_count-1] = tmp;
        i = lineEnd + 1;
    }
    
    /* check unique */
    ksort(data, buf.elem_count, sizeof(data[0]), config_cmp, Data);
    for (i = 1; i < buf.elem_count; ++i) {
        if (strcmp(&Data[data[i-1].id], &Data[data[i].id]) == 0)
            return RC(rcAlign, rcIndex, rcConstructing, rcItem, rcExists);
    }

    for (i = 0; i != buf.elem_count; ++i) {
        unsigned const extralen = data[i].extralen;
        char const *const id    = &Data[data[i].id];
        char const *const seqId = &Data[data[i].seqId];
        char const *const extra = extralen ? &Data[data[i].extra] : NULL;
        bool circular = false;
        ReferenceSeq *rs;
        
        if (extra && extralen >= 8) {
            char const *const circ = strcasestr(extra, "circular");
            
            circular = circ && (circ == extra || isspace(circ[-1])) &&
                       (circ[8] == '\0' || isspace(circ[8]));
        }
        if ((rc = ReferenceMgr_NewReferenceSeq(self, &rs)) != 0) return rc;
        
        rs->id = string_dup( id, string_size( id ) );
        if ( rs->id == NULL )
            return RC(rcAlign, rcFile, rcReading, rcMemory, rcExhausted);
        
        rs->seqId = string_dup( seqId, string_size( seqId ) );
        if ( rs->seqId == NULL )
            return RC(rcAlign, rcFile, rcReading, rcMemory, rcExhausted);
        
        rs->circular = circular;
    }
    KDataBufferWhack(&buf);
    return 0;
}

static
rc_t ReferenceMgr_Conf(ReferenceMgr *const self, char const conf[])
{
    rc_t rc;
    const KFile* kf = NULL;

    if (conf == NULL)
        return 0;
    
    rc = FindAndOpenConfigFile(self->rmgr, self->dir, &kf, conf);
    if (rc == 0) {
        uint64_t sz;
        KDataBuffer buf;
        
        rc = KFileSize(kf, &sz);
        assert(rc == 0);
        if (sz == 0)
            (void)PLOGMSG(klogWarn, (klogWarn, "Configuration file '$(file)' is empty", "file=%s", conf));
        else {
            rc = KDataBufferMakeBytes(&buf, sz + 1);
            if (rc == 0) {
                size_t nread;
                
                rc = KFileReadAll(kf, 0, buf.base, sz, &nread);
                if (rc == 0) {
                    assert(nread == sz);
                    ((char *)buf.base)[sz] = '\n'; /* make sure that last line is terminated */
                    rc = ReferenceMgr_ProcessConf(self, buf.base, sz + 1);
                }
                KDataBufferWhack(&buf);
            }
        }
        KFileRelease(kf);
    }
    return rc;
}

static
rc_t ReferenceMgr_FastaFile_GetSeqIds(KDataBuffer *const buf, char const data[], uint64_t const len)
{
    uint64_t pos;
    int st = 0;
    
    for (pos = 0; pos < len; ++pos) {
        int const ch = data[pos];
        
        if (st == 0) {
            if (ch == '>') {
                uint64_t const n = buf->elem_count;
                rc_t rc = KDataBufferResize(buf, n + 1);
                
                if (rc)
                    return rc;
                ((uint64_t *)buf->base)[n] = pos;
                st = 1;
            }
        }
        else if (ch == '\r' || ch == '\n') 
            st = 0;
    }
    return 0;
}

static
rc_t ReferenceMgr_ImportFasta(ReferenceMgr *const self, ReferenceSeq *obj, KDataBuffer *const buf)
{
    unsigned seqId;
    unsigned seqIdLen;
    unsigned ln;
    unsigned src;
    unsigned dst;
    unsigned start=0;
    char *const data = buf->base;
    unsigned const len = buf->elem_count;
    rc_t rc;
    MD5State mds;
    
    memset(obj, 0, sizeof(*obj));
    obj->mgr = self;
    
    if (len == 0)
        return 0;
    assert(data[0] == '>');
    
    for (ln = 1; ln != len; ++ln) {
        int const ch = data[ln];
        
        if (ch == '\r' || ch == '\n') {
            data[ln] = '\0';
            start = ln + 1;
            break;
        }
    }
    for (seqId = 1; seqId != ln; ++seqId) {
        if (!isspace(data[seqId]))
            break;
    }
    for (seqIdLen = 0; seqId + seqIdLen < ln; ++seqIdLen) {
        if (isspace(data[seqId + seqIdLen])) {
            ln = seqId + seqIdLen;
            data[ln] = '\0';
            break;
        }
    }
    if (seqIdLen == 0)
        return RC(rcAlign, rcFile, rcReading, rcData, rcInvalid);
    
    obj->fastaSeqId = string_dup( &data[ seqId ], string_size( &data[ seqId ] ) );
    if ( obj->fastaSeqId == NULL )
        return RC(rcAlign, rcFile, rcReading, rcMemory, rcExhausted);
    
    MD5StateInit(&mds);
    for (dst = src = start; src != len; ++src) {
        int const ch = toupper(data[src]);
        
        if (isspace(ch))
            continue;
        
        if (strchr(INSDC_4na_map_CHARSET, ch) == NULL && ch != 'X')
            return RC(rcAlign, rcFile, rcReading, rcData, rcInvalid);
        
        data[dst] = ch == 'X' ? 'N' : ch;
        MD5StateAppend(&mds, data + dst, 1);
        ++dst;
    }
    MD5StateFinish(&mds, obj->md5);
    rc = KDataBufferSub(buf, &obj->u.local.buf, start, dst - start);
    if (rc == 0) {
        obj->type = rst_local;
        obj->seq_len = dst - start;
    }
    else
        obj->type = rst_dead;
    return rc;
}

#define READ_CHUNK_SIZE (1024 * 1024)

static
rc_t ReferenceMgr_ImportFastaFile(ReferenceMgr *const self, KFile const *kf,
                                  ReferenceSeq *rslt)
{
    uint64_t file_size;
    rc_t rc = KFileSize(kf, &file_size);
    
    if (rc == 0) {
        KDataBuffer fbuf;
        
        rc = KDataBufferMake(&fbuf, 8, file_size);
        if (rc == 0) {
            fbuf.elem_count = 0;
            do {
                size_t const readable = file_size - fbuf.elem_count;
                size_t const to_read = readable > READ_CHUNK_SIZE ? READ_CHUNK_SIZE : readable;
                size_t nread = 0;
                
                rc = KFileRead(kf, fbuf.elem_count, &((uint8_t *)fbuf.base)[fbuf.elem_count], to_read, &nread);
                if (rc != 0 || nread == 0)
                    break;
                fbuf.elem_count += nread;
            } while (fbuf.elem_count < file_size);
            if (rc == 0) {
                char const *const base = fbuf.base;
                KDataBuffer seqIdBuf;
                
                memset(&seqIdBuf, 0, sizeof(seqIdBuf));
                seqIdBuf.elem_bits = sizeof(file_size) * 8;
                
                rc = ReferenceMgr_FastaFile_GetSeqIds(&seqIdBuf, base, file_size);
                if (rc == 0) {
                    uint64_t const *const seqIdOffset = seqIdBuf.base;
                    unsigned const seqIds = seqIdBuf.elem_count;
                    unsigned i;
                    KDataBuffer sub;
                    
                    if (rslt) {
                        if (seqIds > 1)
                            rc = RC(rcAlign, rcFile, rcReading, rcItem, rcUnexpected);
                        
                        memset(&sub, 0, sizeof(sub));
                        KDataBufferSub(&fbuf, &sub, seqIdOffset[0], file_size - seqIdOffset[0]);
                        rc = ReferenceMgr_ImportFasta(self, rslt, &sub);
                        KDataBufferWhack(&sub);
                    }
                    else
                        for (i = 0; i != seqIds; ++i) {
                            uint64_t const ofs = seqIdOffset[i];
                            uint64_t const nxt = (i < seqIds - 1) ? seqIdOffset[i + 1] : file_size;
                            uint64_t const len = nxt - ofs;
                            ReferenceSeq tmp;
                            ReferenceSeq *new_seq;
                            
                            memset(&sub, 0, sizeof(sub));
                            KDataBufferSub(&fbuf, &sub, ofs, len);
                            rc = ReferenceMgr_ImportFasta(self, &tmp, &sub);
                            KDataBufferWhack(&sub);
                            if (rc) break;
                            
                            rc = ReferenceMgr_NewReferenceSeq(self, &new_seq);
                            if (rc) break;
                            
                            *new_seq = tmp;
                        }
                }
                KDataBufferWhack(&seqIdBuf);
            }
            KDataBufferWhack(&fbuf);
        }
    }
    return rc;
}

static
rc_t OpenFastaFile(KFile const **const kf,
                   KDirectory const *const dir,
                   char const base[],
                   unsigned const len)
{
    char fname_a[4096];
    char *fname_h = NULL;
    char *fname = fname_a;
    rc_t rc;
    
    if (len + 7 >= sizeof(fname_a)) {
        fname_h = malloc(len + 7);
        if (fname_h)
            fname = fname_h;
        else
            return RC(rcAlign, rcFile, rcOpening, rcMemory, rcExhausted);
    }
    memcpy(fname, base, len);
    memcpy(fname + len, ".fasta", 7);
    
    rc = KDirectoryOpenFileRead(dir, kf, fname);
    if (rc) {
        fname[len + 3] = '\0'; /* base.fasta -> base.fa */
        
        rc = KDirectoryOpenFileRead(dir, kf, fname);
    }
    free(fname_h);
    return rc;
}

#if 1
void ReferenceSeq_Dump(ReferenceSeq const *const rs, unsigned const i, key_id_t const *const key_id_array, unsigned const m)
{
    static char const *types[] = {
        "'unattached'",
        "'fasta'",
        "'RefSeq-by-id'",
        "'RefSeq-by-seqid'",
        "'dead'"
    };
    unsigned j;
    
    ALIGN_CF_DBGF(("{ "));
    ALIGN_CF_DBGF(("type: %s, ", (rs->type < 0 || rs->type > rst_dead) ? "null" : types[rs->type]));
    
    if (rs->id)
        ALIGN_CF_DBGF(("id: '%s', ", rs->id));
    else
        ALIGN_CF_DBGF(("id: null, "));
    
    if (rs->seqId)
        ALIGN_CF_DBGF(("seqId: '%s', ", rs->seqId));
    else
        ALIGN_CF_DBGF(("seqId: null, "));
    
    if (rs->fastaSeqId)
        ALIGN_CF_DBGF(("fastaSeqId: '%s', ", rs->fastaSeqId));
    else
        ALIGN_CF_DBGF(("fastaSeqId: null, "));
    
    ALIGN_CF_DBGF(("seq-len: %u, ", rs->seq_len));
    ALIGN_CF_DBGF(("circular: %s, ", rs->circular ? "true" : "false"));
    
    if (rs->md5) {
        ALIGN_CF_DBGF(("md5: '"));
        for (j = 0; j != 16; ++j)
            ALIGN_CF_DBGF(("%02X", rs->md5[j]));
        ALIGN_CF_DBGF(("', "));
    }
    else
        ALIGN_CF_DBGF(("md5: null, "));
    
    
    ALIGN_CF_DBGF(("keys: [ "));
    for (j = 0; j != m; ++j) {
        key_id_t const *const kid = &key_id_array[j];
        
        if (kid->id == i)
            ALIGN_CF_DBGF(("'%s', ", kid->key));
    }
    ALIGN_CF_DBGF(("] }"));
}

void ReferenceMgr_DumpConfig(ReferenceMgr const *const self)
{
    unsigned const n = self->refSeqs.elem_count;
    unsigned const m = self->refSeqsById.elem_count;
    key_id_t const *const key_id_array = self->refSeqsById.base;
    unsigned i;
    
    ALIGN_CF_DBGF(("config: [\n"));
    for (i = 0; i != n; ++i) {
        ALIGN_CF_DBGF(("\t"));
        ReferenceSeq_Dump(&self->refSeq[i], i, key_id_array, m);
        ALIGN_CF_DBGF((",\n"));
    }
    ALIGN_CF_DBGF(("]\n"));
}
#endif

static
rc_t ReferenceMgr_TryFasta(ReferenceMgr *const self, ReferenceSeq *const seq,
                           char const id[], unsigned const idLen)
{
    KFile const *kf = NULL;
    rc_t rc;
    
    rc = OpenFastaFile(&kf, self->dir, id, idLen);
    
    if (rc == 0) {
        rc = ReferenceMgr_ImportFastaFile(self, kf, seq);
        KFileRelease(kf);
    }
    return rc;
}

static
rc_t ReferenceSeq_GetRefSeqInfo(ReferenceSeq *const self)
{
    rc_t rc;
    uint8_t const *md5;
    
    assert(self != NULL);
    assert(self->type == rst_refSeqById || self->type == rst_refSeqBySeqId);
    
    if ((rc = RefSeq_Circular(self->u.refseq, &self->circular)) != 0)
        return rc;
    if ((rc = RefSeq_SeqLength(self->u.refseq, &self->seq_len)) != 0)
        return rc;
    if ((rc = RefSeq_MD5(self->u.refseq, &md5)) != 0)
        return rc;
    
    memcpy(self->md5, md5, 16);
    return 0;
}

static
rc_t ReferenceSeq_Attach(ReferenceMgr *const self, ReferenceSeq *const rs)
{
    unsigned const seqid_len = rs->seqId ? strlen(rs->seqId) : 0;
    unsigned const id_len = rs->id ? strlen(rs->id) : 0;
    rc_t rc = 0;
    KFile const *kf = NULL;
    
    assert(rs->type == rst_unattached);
    assert(id_len != 0 || seqid_len != 0);
    
    if (seqid_len) {
        if (RefSeqMgr_Exists(self->rmgr, rs->seqId, seqid_len, NULL) == 0) {
            rc = RefSeqMgr_GetSeq(self->rmgr, &rs->u.refseq, rs->seqId, seqid_len);
            if (rc == 0) {
                rs->type = rst_refSeqBySeqId;
                rc = ReferenceSeq_GetRefSeqInfo(rs);
            }
            return rc;
        }
    }
    if (id_len) {
        if (RefSeqMgr_Exists(self->rmgr, rs->id, id_len, NULL) == 0) {
            rc = RefSeqMgr_GetSeq(self->rmgr, &rs->u.refseq, rs->id, id_len);
            if (rc == 0) {
                rs->type = rst_refSeqById;
                rc = ReferenceSeq_GetRefSeqInfo(rs);
            }
            return rc;
        }
    }
    if (id_len) {
        rc = OpenFastaFile(&kf, self->dir, rs->id, id_len);
        if (rc && seqid_len)
            rc = OpenFastaFile(&kf, self->dir, rs->seqId, seqid_len);
    }
    else
        rc = OpenFastaFile(&kf, self->dir, rs->seqId, seqid_len);
    
    if (kf) {
        ReferenceSeq tmp;
        
        rc = ReferenceMgr_ImportFastaFile(self, kf, &tmp);
        KFileRelease(kf);
        if (rc == 0) {
            tmp.id = rs->id;
            tmp.seqId = rs->seqId;
            tmp.circular = rs->circular;
            
            *rs = tmp;
        }
        return rc;
    }
    return 0;
}

static
rc_t ReferenceMgr_OpenSeq(ReferenceMgr *const self, ReferenceSeq **const rslt,
                          char const id[],
                          unsigned const seq_len,
                          uint8_t const md5[16])
{
    unsigned const idLen = strlen(id);
    key_id_t const *const fnd = ReferenceMgr_FindId(self, id);
    
    assert(rslt != NULL);
    *rslt = NULL;
    if (fnd) {
        if (self->refSeq[fnd->id].type == rst_dead)
            return RC(rcAlign, rcIndex, rcSearching, rcItem, rcInvalid);
        *rslt = &self->refSeq[fnd->id];
        return 0;
    }
    else {
        unsigned const n = self->refSeqs.elem_count;
        unsigned i;
        ReferenceSeq *seq = NULL;
        rc_t rc = 0;
        
        /* try to find by id; this should work most of the time */
        for (i = 0; i != n; ++i) {
            ReferenceSeq *const rs = &self->refSeq[i];

            if (rs->type == rst_dead)
                continue;
            
            if (rs->id && strcmp(rs->id, id) == 0) {
                seq = rs;
                break;
            }
        }
        if (seq == NULL) {
            /* try to find by seqId */
            for (i = 0; i != n; ++i) {
                ReferenceSeq *const rs = &self->refSeq[i];
                
                if (rs->type == rst_dead)
                    continue;
                
                if (rs->seqId && strcasecmp(rs->seqId, id) == 0) {
                    seq = rs;
                    break;
                }
            }
        }
        if (seq == NULL) {
            /* try to find id within fasta seqIds */
            unsigned best_wt = 0;
            unsigned best = n;
            
            for (i = 0; i != n; ++i) {
                ReferenceSeq const *const rs = &self->refSeq[i];
                
                if (rs->fastaSeqId) {
                    unsigned wt = str_weight(rs->fastaSeqId, id, idLen);
                    
                    if (wt != no_match) {
                        if (seq_len && rs->seq_len == seq_len)
                            wt |= seq_len_match;
                        if (md5 && memcmp(rs->md5, md5, 16) == 0)
                            wt |= md5_match;
                    }
                    if (best_wt < wt) {
                        best_wt = wt;
                        best = i;
                    }
                }
            }
            if (best < n)
                seq = &self->refSeq[best];
        }
        if (seq == NULL) {
            /* try id.fasta or id.fa */
            rc = ReferenceMgr_NewReferenceSeq(self, &seq);
            if (rc) return rc;
            rc = ReferenceMgr_TryFasta(self, seq, id, idLen);
            if (GetRCState(rc) == rcNotFound && GetRCObject(rc) == rcPath)
                rc = 0;
            else if (rc) return rc;
        }
        else if (seq->type == rst_unattached) {
            /* expect to get here most of the time
             *
             * ReferenceSeq_Attach tries to get reference:
             *  from RefSeqMgr:
             *   by seqId
             *   by id
             *  from self->dir (data directory)
             *   id.fasta
             *   id.fa
             *   seqId.fasta
             *   seqId.fa
             */
            rc = ReferenceSeq_Attach(self, seq);
            if (rc) return rc;
            
            if (seq->type == rst_unattached && seq->seqId != NULL) {
                /* attach didn't work for id; try to find seqId within fasta seqIds */
                unsigned const seqIdLen = strlen(seq->seqId);
                unsigned best_wt = 0;
                unsigned best = n;
                
                for (i = 0; i != n; ++i) {
                    ReferenceSeq const *const rs = &self->refSeq[i];
                    
                    if (rs->type == rst_local && rs->fastaSeqId) {
                        unsigned wt = str_weight(rs->fastaSeqId, seq->seqId, seqIdLen);
                        
                        if (wt != no_match) {
                            if (seq_len && rs->seq_len == seq_len)
                                wt |= seq_len_match;
                            if (md5 && memcmp(rs->md5, md5, 16) == 0)
                                wt |= md5_match;
                        }
                        if (best_wt < wt) {
                            best_wt = wt;
                            best = i;
                        }
                    }
                }
                if (best < n) {
                    char *const tmp_id = seq->id;
                    char *const tmp_seqId = seq->seqId;
                    bool const tmp_circ = seq->circular;
                    
                    *seq = self->refSeq[best];
                    seq->id = tmp_id;
                    seq->seqId = tmp_seqId;
                    seq->fastaSeqId = NULL;
                    seq->circular = tmp_circ;

                    /* add another reference to the data buffer */
                    rc = KDataBufferSub(&self->refSeq[best].u.local.buf, &seq->u.local.buf, 0, 0);
                    if (rc) return rc;
                }
            }
        }
        assert(seq != NULL);
        if (seq->type == rst_unattached) {
            /* nothing has worked and nothing left to try */
            seq->type = rst_dead;
            rc = RC(rcAlign, rcFile, rcConstructing, rcId, rcNotFound);
        }
        else {
            ReferenceSeq *alt = NULL;
            
            /* perform ambiguity check
             *
             * This search follows the same pattern as the main search but has
             * more stringent conditions.  One hopes that it fails to find
             * anything.
             */
            for (i = 0; i != n; ++i) {
                ReferenceSeq *const rs = &self->refSeq[i];
                
                if (   rs->type != rst_dead
                    && rs->type != rst_unattached
                    && rs != seq
                    && rs->id != NULL
                    && strcmp(id, rs->id) == 0
                    && (seq_len == 0 || seq_len == rs->seq_len))
                {
                    alt = rs;
                    break;
                }
            }
            if (alt == NULL) {
                for (i = 0; i != n; ++i) {
                    ReferenceSeq *const rs = &self->refSeq[i];
                    
                    if (   rs->type != rst_dead
                        && rs->type != rst_unattached
                        && rs != seq
                        && rs->seqId != NULL
                        && strcasecmp(id, rs->seqId) == 0
                        && (seq_len == 0 || seq_len == rs->seq_len))
                    {
                        alt = rs;
                        break;
                    }
                }
            }
            if (alt == NULL) {
                unsigned best_wt = 0;
                unsigned best = n;
                
                for (i = 0; i != n; ++i) {
                    ReferenceSeq const *const rs = &self->refSeq[i];
                    
                    if (rs != seq && rs->fastaSeqId && rs != seq) {
                        unsigned wt = str_weight(rs->fastaSeqId, id, idLen);
                        
                        if (wt != no_match) {
                            if (seq_len && rs->seq_len == seq_len)
                                wt |= seq_len_match;
                            if (md5 && memcmp(rs->md5, md5, 16) == 0)
                                wt |= md5_match;
                        }
                        if (best_wt < wt) {
                            best_wt = wt;
                            best = i;
                        }
                    }
                }
                if (best < n)
                    alt = &self->refSeq[best];
            }
            /* try to knock the alternative out of consideration
             * if it survives length and md5 tests, it is *really* likely to be
             * a duplicate.
             */
            if (alt != NULL && seq_len != 0 && seq_len != alt->seq_len)
                alt = NULL;
            if (alt != NULL && md5 != NULL && memcmp(md5, alt->md5, 16) != 0)
                alt = NULL;
            if (alt != NULL) {
                seq->type = rst_dead;
                rc = RC(rcAlign, rcFile, rcConstructing, rcId, rcAmbiguous);
            }
        }
        if ( seq->id == NULL )
        {
            seq->id = string_dup( id, string_size( id ) );
            if ( seq->id == NULL )
                return RC(rcAlign, rcFile, rcConstructing, rcMemory, rcExhausted);
        }
        /* finally, associate the id with the object and put it in the index */
        {{
            rc_t rc2 = ReferenceMgr_AddId(self, id, seq);
            if (rc == 0)
                rc = rc2;
        }}
        if (rc == 0)
            *rslt = seq;
        return rc;
    }
}

LIB_EXPORT rc_t CC ReferenceMgr_SetCache(ReferenceMgr const *const self, size_t cache, uint32_t num_open)
{
    return RefSeqMgr_SetCache(self->rmgr, cache, num_open);
}

static
rc_t OpenDataDirectory(KDirectory const **rslt, char const path[])
{
    KDirectory *dir;
    rc_t rc = KDirectoryNativeDir(&dir);
    
    if (rc == 0) {
        if (path) {
            rc = KDirectoryOpenDirRead(dir, rslt, false, path);
            KDirectoryRelease(dir);
        }
        else
            *rslt = dir;
    }
    return rc;
}

LIB_EXPORT rc_t CC ReferenceMgr_Make(ReferenceMgr const **cself, VDatabase *db,
                                     VDBManager const* vmgr,
                                     const uint32_t options, char const conf[], char const path[],
                                     uint32_t max_seq_len, size_t cache, uint32_t num_open)
{
    rc_t rc;
    ReferenceMgr *self;
    uint32_t wopt = 0;
    
    if (cself == NULL)
        return RC(rcAlign, rcIndex, rcConstructing, rcParam, rcNull);
    
    wopt |= (options & ewrefmgr_co_allREADs) ? ewref_co_SaveRead : 0;
    wopt |= (options & ewrefmgr_co_Coverage) ? ewref_co_Coverage : 0;

    if (max_seq_len == 0)
        max_seq_len = TableWriterRefSeq_MAX_SEQ_LEN;
    
    self = calloc(1, sizeof(*self));
    if (self) {
        rc = KDataBufferMakeBytes(&self->seq, max_seq_len);
        if (rc == 0) {
            self->compress.elem_bits = 32;
            self->refSeqs.elem_bits = sizeof(ReferenceSeq) * 8;
            self->refSeqsById.elem_bits = sizeof(key_id_t) * 8;
            
            self->options = options;
            self->cache = cache;
            self->num_open_max = num_open;
            self->max_seq_len = max_seq_len;
            if (db) VDatabaseAddRef(self->db = db);
            rc = OpenDataDirectory(&self->dir, path);
            if (rc == 0) {
                rc = RefSeqMgr_Make(&self->rmgr, vmgr, 0, cache, num_open);
                if (rc == 0) {
                    rc = ReferenceMgr_Conf(self, conf);
                    if (rc == 0) {
                        *cself = self;
                        ALIGN_DBG("conf %s, local path '%s'", conf ? conf : "", path ? path : "");
                        return 0;
                    }
                    (void)PLOGERR(klogErr, (klogErr, rc, "failed to open configuration $(file)", "file=%s/%s", path ? path : ".", conf));
                }
            }
        }
        ReferenceMgr_Release(self, false, NULL, false);
    }
    else
        rc = RC(rcAlign, rcIndex, rcConstructing, rcMemory, rcExhausted);

    ALIGN_DBGERR(rc);

    return rc;
}

#define ID_CHUNK_SZ 256

typedef struct TChunk32_struct {
    struct TChunk32_struct* next;
    uint32_t id[ID_CHUNK_SZ]; /*** will only work with positive ids **/
} TChunk32;

typedef struct AlignId32List_struct {
	TChunk32* head;
	TChunk32* tail;
	uint32_t  tail_qty;  /** number elements in the last chunk **/
	uint32_t  chunk_qty; /** number of chunks */
} AlignId32List;
typedef struct AlignIdList_struct {
	AlignId32List **sub_list;
	uint32_t        sub_list_count; 
} AlignIdList;

static uint64_t AlignId32ListCount(AlignId32List *l)
{ return (l->chunk_qty>0)?ID_CHUNK_SZ*(l->chunk_qty-1)+ l->tail_qty:0;}

static uint64_t AlignIdListCount(AlignIdList *l)
{
	uint64_t ret=0;
	if(l){
		uint32_t i;
		for(i=0;i<l->sub_list_count;i++){
			if(l->sub_list[i]){
				ret += AlignId32ListCount(l->sub_list[i]);
			}
		}
	}
	return ret;
}
static int int64_compare(const void *A, const void *B, void *ignore)
{
	return *(int64_t*)A -  *(int64_t*)B;
}
static uint64_t AlignIdListFlatCopy(AlignIdList *l,int64_t *buf,uint64_t num_elem,bool do_sort)
{
	uint64_t res=0;
	uint32_t i,j;
	AlignId32List* cl;
	assert(l!=0);

	if((cl = l->sub_list[0])!=NULL){
		TChunk32* head  = cl->head;
		while(head !=  cl->tail){
			for(i=0;i<ID_CHUNK_SZ && res < num_elem;i++,res++){
				buf[res] = head->id[i];
			}
			head = head->next;
		}
		for(i=0;i<cl->tail_qty && res < num_elem;i++,res++){
			buf[res] = head->id[i];
		}
	}
	for(j = 1; j< l->sub_list_count && res < num_elem;j++){
		if((cl = l->sub_list[j])!=NULL){
			TChunk32* head  = cl->head;
			uint64_t  hi = ((uint64_t)j) << 32;
			while(head !=  cl->tail){
				for(i=0;i<ID_CHUNK_SZ && res < num_elem;i++,res++){
					buf[res] = hi | head->id[i];
				}
				head = head->next;
			}
			for(i=0;i<cl->tail_qty && res < num_elem;i++,res++){
				buf[res] = hi | head->id[i];
			}
		}
	}
	if(do_sort && res > 1) 
		ksort(buf,res,sizeof(*buf),int64_compare,NULL);
	return res;
}

static rc_t AlignId32ListAddId(AlignId32List *l,const uint32_t id)
{
	if(l->tail  == NULL){
		l->head = l->tail = malloc(sizeof(*l->tail));
		if(l->tail == NULL) return RC(rcAlign, rcTable, rcCommitting, rcMemory, rcExhausted);
		l->chunk_qty =1;
		l->tail_qty = 0;	
	}
	if(l->tail_qty == ID_CHUNK_SZ){/** chunk is full **/
		l->tail->next = malloc(sizeof(*l->tail));
		if(l->tail == NULL) return RC(rcAlign, rcTable, rcCommitting, rcMemory, rcExhausted);
		l->tail = l->tail->next;
		l->chunk_qty ++;
		l->tail_qty = 0;	
	}
	l->tail->id[l->tail_qty++]=id;
	return 0;
}

static rc_t AlignIdListAddId(AlignIdList *l,const int64_t id)
{
	uint32_t  sub_id,id32;
	if(id < 0) return RC(rcAlign, rcTable, rcCommitting, rcId, rcOutofrange);
	id32 = (uint32_t) id;
	sub_id = id >> 32;
	if(sub_id >= l->sub_list_count) return RC(rcAlign, rcTable, rcCommitting, rcId, rcOutofrange);
	if(l->sub_list[sub_id] == NULL){
		l->sub_list[sub_id] = calloc(1,sizeof(AlignId32List));
		if(l->sub_list[sub_id] == NULL) return RC(rcAlign, rcTable, rcCommitting, rcMemory, rcExhausted);
	}
	return AlignId32ListAddId(l->sub_list[sub_id],id32);
}

static void AlignId32ListRelease(AlignId32List *l)
{
	if(l){
		while(l->head != l->tail){
			TChunk32* head = l->head;
			l->head = l->head->next;
			free(head);
		}
		free(l->head);
		free(l);
	}
}
static void AlignIdListRelease(AlignIdList *l)
{
        if(l){
		uint32_t i;
		for(i=0;i<l->sub_list_count;i++){
			AlignId32ListRelease(l->sub_list[i]);
		}
		free(l->sub_list);
		free(l);
        }
}


typedef struct {
    AlignIdList*	idlist;
    ReferenceSeqCoverage cover;
    INSDC_coord_len bin_seq_len;
} TCover;

static
void ReferenceMgr_TCoverRelease(TCover* c)
{
	if(c){
		AlignIdListRelease(c->idlist);
		c->idlist = NULL;
	}
}
static 
rc_t ReferenceMgr_TCoverSetMaxId(TCover* c,int64_t id)
{
	uint32_t  sub_id;
	if(id < 0) return RC(rcAlign, rcTable, rcCommitting, rcId, rcOutofrange);
	sub_id = id >> 32;
	if(c->idlist == NULL){
		c->idlist = calloc(1,sizeof(AlignIdList));
		if(c->idlist==NULL) return RC(rcAlign, rcTable, rcCommitting, rcMemory, rcExhausted);
		c->idlist->sub_list_count = sub_id+1;
		c->idlist->sub_list = calloc(c->idlist->sub_list_count,sizeof(c->idlist->sub_list[0]));
		if( c->idlist->sub_list == NULL) return RC(rcAlign, rcTable, rcCommitting, rcMemory, rcExhausted);
	} else {
		return RC(rcAlign, rcTable, rcCommitting, rcParam, rcUnexpected);
	}
	return 0;
}

static
rc_t CoverageGetSeqLen(ReferenceMgr const *const mgr, TCover data[], uint64_t const rows)
{
    TableReaderColumn acols[] =
    {
        {0, "(INSDC:coord:len)SEQ_LEN", {NULL}, 0, 0},
        {0, NULL, {NULL}, 0, 0}
    };
    VTable const *tbl;
    rc_t rc = VDatabaseOpenTableRead(mgr->db, &tbl, "REFERENCE");
    
    if (rc == 0) {
        TableReader const *reader;
        
        rc = TableReader_Make(&reader, tbl, acols, 0);
        if (rc == 0) {
            uint64_t i;
            
            for (i = 0; i != rows; ++i) {
                rc = TableReader_ReadRow(reader, i + 1);
                if (rc == 0 && acols->len > 0)
                    data[i].bin_seq_len = acols->base.coord_len[0];
            }
            TableReader_Whack(reader);
        }
        VTableRelease(tbl);
    }
    return rc;
}

static
rc_t ReferenceMgr_ReCover(const ReferenceMgr* cself, uint64_t ref_rows)
{
    rc_t rc = 0;
    uint64_t new_rows = 0;
    const TableWriterRefCoverage* cover_writer = NULL;
    
    TableReaderColumn acols[] =
    {
        {0, "REF_ID", {NULL}, 0, 0},
        {0, "REF_START", {NULL}, 0, 0},
        {0, "CIGAR_LONG",{NULL}, 0, 0},
        {0, "REF_POS",{NULL}, 0, 0},
        {0, NULL, {NULL}, 0, 0}
    };
    const int64_t** al_ref_id = &acols[0].base.i64;
    const INSDC_coord_zero** al_ref_start = &acols[1].base.coord0;
    const TableReaderColumn* cigar =  &acols[2];
    const INSDC_coord_zero** al_ref_pos = &acols[3].base.coord0;
    /* order is important see ReferenceSeqCoverage struct */
    struct {
        const char* nm;
	    const char* col;
        bool ids_only;
    } tbls[] = { {"PRIMARY_ALIGNMENT", "PRIMARY_ALIGNMENT_IDS",false},
        {"SECONDARY_ALIGNMENT", "SECONDARY_ALIGNMENT_IDS",false},
        {"EVIDENCE_INTERVAL", "EVIDENCE_INTERVAL_IDS", true} };
	int tbls_qty=(sizeof(tbls)/sizeof(tbls[0]));
    rc_t rc1 = 0;
    int64_t rr;
    uint32_t i;
    uint8_t* hilo=NULL;
    TCover* data = NULL;
    
    /* allocate mem for ref_rows of reference coverage*/
    if((data = calloc(ref_rows, (sizeof(*data) + cself->max_seq_len))) == NULL ) {
		rc = RC(rcAlign, rcTable, rcCommitting, rcMemory, rcExhausted);
    } else {
		/** allocation for both data and hilo was done in 1 shot ***/
		hilo = (uint8_t *)&data[ref_rows];
        rc = CoverageGetSeqLen(cself, data, ref_rows);
    }
    /* grep through tables for coverage data */
    ALIGN_R_DBG("covering REFERENCE rowid range [1:%ld]",ref_rows);
    for(i = 0; rc == 0 && i < tbls_qty; i++) { /* TABLE LOOP STARTS */
        const VTable* table = NULL;
        const TableReader* reader = NULL;
        int64_t al_from;
        uint64_t al_qty;
        
        ALIGN_R_DBG("covering REFERENCE with %s", tbls[i].nm);
        if( (rc = VDatabaseOpenTableRead(cself->db, &table, tbls[i].nm)) != 0 ) {
            if( GetRCState(rc) == rcNotFound ) {
                ALIGN_R_DBG("table %s was not found, ignored", tbls[i].nm);
                rc = 0;
                continue;
            } else {
                break;
            }
        }
        if( (rc = TableReader_Make(&reader, table, acols, cself->cache)) == 0 &&
           (rc = TableReader_IdRange(reader, &al_from, &al_qty)) == 0 ) {
            int64_t al_rowid;
            
            for(al_rowid = al_from; rc == 0 && al_rowid < al_from + al_qty; al_rowid++) {
                if( (rc = TableReader_ReadRow(reader, al_rowid)) != 0 ) {
                    break;
                }
                rr    = **al_ref_id-1;
                /**** Record ALIGNMENT_IDS ***/
                if(   data[rr].idlist == NULL 
                   && (rc = ReferenceMgr_TCoverSetMaxId(data+rr,al_from + al_qty))!=0){
                    break; /*** out-of-memory ***/
                }
                if((rc = AlignIdListAddId(data[rr].idlist,al_rowid))!=0){
                    break; /*** out-of-memory ***/
                }
                /**** Done alignment ids ***/
                if( !tbls[i].ids_only ) { /*** work on statistics ***/
                    char const *c = cigar->base.str;
                    const char *c_end = c + cigar->len;
                    int64_t const global_ref_pos = rr*cself->max_seq_len + **al_ref_start; /** global_ref_start **/
                    int64_t const global_refseq_start = global_ref_pos -  **al_ref_pos;   /** global_ref_start of current reference **/
                    unsigned const bin_no = global_ref_pos / cself->max_seq_len;
                    TCover *const bin = &data[bin_no];
                    uint8_t *const cov = &hilo[global_ref_pos];
                    int64_t ref_offset = 0;
                    int64_t max_ref_offset = 0;
                    int64_t min_ref_offset = 0;
                    int64_t j;

                    while (rc == 0 && c < c_end) {
                        int op_len = strtol(c, (char **)&c, 10);
                        int const op = *c++;
                        
                        switch (op){
                        case 'I':/* extra bases in the read **/
                            ++bin->cover.indels;
                        case 'S':/* skip in the read */
                            break;
                        case 'B':/* back up in the sequence */
                            if (ref_offset > op_len)
                                ref_offset -= op_len;
                            else
                                ref_offset = 0;
                            break;
                        case 'D': /** delete in the reference ***/
                            ++bin->cover.indels;
                        case 'N': /** expected skip in the reference ***/
                            ref_offset += op_len;
                            break;
                        case 'X':
                            bin->cover.mismatches += op_len;
                        case '=':
                            ref_offset += op_len;
                            break;
                        default:
                            rc = RC(rcAlign, rcTable, rcCommitting, rcData, rcUnrecognized);
                        }
                        if (min_ref_offset > ref_offset)
                            min_ref_offset = ref_offset;
                        if (max_ref_offset < ref_offset)
                            max_ref_offset = ref_offset;
                    }
                    for (j = min_ref_offset; j < max_ref_offset; ++j) {
                        unsigned const hl = cov[j];
                        
                        if (hl < UINT8_MAX)
                            cov[j] = hl + 1;
                    }
                    /*** check if OVERLAPS are needed ***/
                    {
                        int64_t min_rr = (global_ref_pos + min_ref_offset)/cself->max_seq_len;
                        int64_t max_rr = (global_ref_pos + max_ref_offset)/cself->max_seq_len;
                        
                        if(min_rr < 0) min_rr = 0;
                        if(max_rr >= ref_rows) max_rr = ref_rows -1;
                        
                        assert(min_rr<= max_rr);
                        
                        if(min_rr < max_rr){
                            int64_t  overlap_ref_pos; /** relative the beginning of the reference **/
                            uint32_t overlap_ref_len = (global_ref_pos + max_ref_offset) % cself->max_seq_len ;
                            
                            min_rr++;
                            if (global_ref_pos + min_ref_offset > global_refseq_start) {
                                overlap_ref_pos = global_ref_pos + min_ref_offset - global_refseq_start;
                            }
                            else {
                                overlap_ref_pos = 1;
                            }
                            for ( ; min_rr < max_rr; ++min_rr) {
                                if (   data[min_rr].cover.overlap_ref_pos[i] == 0 /*** NOT SET***/
                                    || overlap_ref_pos < data[min_rr].cover.overlap_ref_pos[i])
                                {
									data[min_rr].cover.overlap_ref_pos[i] = overlap_ref_pos;
                                }
                                data[min_rr].cover.overlap_ref_len[i] = cself->max_seq_len; /*** in between chunks get full length of overlap **/
                            }
                            if (   data[min_rr].cover.overlap_ref_pos[i] == 0
                                || overlap_ref_pos < data[min_rr].cover.overlap_ref_pos[i])
                            {
								data[min_rr].cover.overlap_ref_pos[i] = overlap_ref_pos;
                            }
                            if (overlap_ref_len > data[min_rr].cover.overlap_ref_len[i])
								data[min_rr].cover.overlap_ref_len[i] = overlap_ref_len;
                        }
                    }
                } /**** DONE WITH WORK ON STATISTICS ***/
                ALIGN_DBGERR(rc);
                rc = rc ? rc : Quitting();
            }
		    /*** HAVE TO RELEASE **/
		    TableReader_Whack(reader);
		    VTableRelease(table);
		    /*** NOW SAVE AND RELEASE THE COLUMN ***/
		    if( (rc = TableWriterRefCoverage_MakeIds(&cover_writer, cself->db, tbls[i].col)) == 0 ) {
                for(rr=0; rc ==0 &&  rr < ref_rows; rr ++){
                    uint64_t num_elem = AlignIdListCount( data[rr].idlist);
                    uint64_t num_elem_copied = 0;
                    if(num_elem > 0 ){
#define BUF_STACK_COUNT 128 * 1024
                        int64_t buf_stack[BUF_STACK_COUNT];
                        int64_t *buf_alloc = NULL;
                        int64_t *buf = buf_stack;
                        if(num_elem > BUF_STACK_COUNT){
                            buf=buf_alloc=malloc(num_elem*sizeof(buf[0]));
                            if(buf_alloc == NULL) 
                                rc = RC(rcAlign, rcTable, rcCommitting, rcMemory, rcExhausted);
                        }
                        if(rc == 0){
                            num_elem_copied = AlignIdListFlatCopy(data[rr].idlist,buf,num_elem,true);
                            assert(num_elem == num_elem_copied);
                        }
                        ReferenceMgr_TCoverRelease(data+rr); /** release memory ***/
                        if(rc == 0){
                            rc = TableWriterRefCoverage_WriteIds(cover_writer, rr+1, buf,num_elem);
                        }
                        if(buf_alloc) free(buf_alloc);
                    } else {
                        rc = TableWriterRefCoverage_WriteIds(cover_writer, rr+1, NULL,0);
                    }
                }
                if(rc == 0){
                    rc = TableWriterRefCoverage_Whack(cover_writer, rc == 0, &new_rows);
                    if( rc == 0  && ref_rows != new_rows ) {
                        rc = RC(rcAlign, rcTable, rcCommitting, rcData, rcInconsistent);
                    }
                }
                ALIGN_DBGERR(rc);
		    }
		} else {
			TableReader_Whack(reader);
			VTableRelease(table);
		}
	}/* TABLE LOOP ENDS **/
    /* prep and write coverage data */
	if(rc == 0) {
        uint64_t k;
        
		rc = TableWriterRefCoverage_MakeCoverage(&cover_writer, cself->db, 0);
		for (rr = 0, k = 0; rc == 0 && rr != ref_rows; ++rr, k += cself->max_seq_len) {
            unsigned hi = 0;
            unsigned lo = 255;
            
		    for (i = 0; i != data[rr].bin_seq_len; ++i) {
                unsigned const depth = hilo[k + i];
                
                if (hi < depth) hi = depth;
                if (lo > depth) lo = depth;
		    }
            data[rr].cover.high = hi;
            data[rr].cover.low  = lo;
		    rc = TableWriterRefCoverage_WriteCoverage(cover_writer,rr+1, &data[rr].cover);
		}
		free(data);
		rc1 = TableWriterRefCoverage_Whack(cover_writer, rc == 0, &new_rows);
		rc = rc ? rc : rc1;
		if( rc == 0 && ref_rows != new_rows ) {
		    rc = RC(rcAlign, rcTable, rcCommitting, rcData, rcInconsistent);
		}
	}
    ALIGN_DBGERR(rc);
	return rc;
}

LIB_EXPORT rc_t CC ReferenceMgr_Release(const ReferenceMgr *cself,
                                        bool commit,
                                        uint64_t *const Rows,
                                        bool build_coverage)
{
    rc_t rc = 0;

    if (cself != NULL) {
        ReferenceMgr *const self = (ReferenceMgr *)cself;
        uint64_t rows;
        unsigned i;
        
        rc = TableWriterRef_Whack(self->writer, commit, &rows);
        if (Rows) *Rows = rows;
        KDirectoryRelease(self->dir);
        
        for (i = 0; i != self->refSeqsById.elem_count; ++i)
            free((void *)((key_id_t *)self->refSeqsById.base)[i].key);
        
        for (i = 0; i != self->refSeqs.elem_count; ++i)
            ReferenceSeq_Whack(&self->refSeq[i]);
        
        KDataBufferWhack(&self->compress);
        KDataBufferWhack(&self->seq);
        KDataBufferWhack(&self->refSeqs);
        KDataBufferWhack(&self->refSeqsById);
        
        if (rc == 0 && build_coverage && commit)
            rc = ReferenceMgr_ReCover(cself, rows);
#if 0 
        {
            VTable* t = NULL;
            
            if (VDatabaseOpenTableUpdate(self->db, &t, "SECONDARY_ALIGNMENT") == 0) {
                VTableDropColumn(t, "TMP_MISMATCH");
                VTableDropColumn(t, "TMP_HAS_MISMATCH");
            }
            VTableRelease(t);
        }
#endif
        VDatabaseRelease(self->db);
        RefSeqMgr_Release(self->rmgr);
        free(self);
    }
    return rc;
}

static
rc_t ReferenceSeq_ReadDirect(ReferenceSeq *self,
                             int offset,
                             unsigned const len,
                             bool read_circular,
                             uint8_t buffer[],
                             unsigned* written,
                             bool force_linear)
{
    *written = 0;
    if (len == 0)
        return 0;
    
    if (read_circular || self->circular) {
        if (offset < 0) {
            unsigned const n = (-offset) / self->seq_len;
            offset = (self->seq_len * (n + 1) + offset) % self->seq_len;
        }
        else if (offset > self->seq_len)
            offset %= self->seq_len;
    }
    else if (offset >= self->seq_len)
        return RC(rcAlign, rcType, rcReading, rcOffset, rcOutofrange);
    
    if (self->type == rst_local) {
        uint8_t const *const src = self->u.local.buf.base;
        unsigned dst_off = 0;
        
        while (dst_off < len) {
            unsigned const writable = len - dst_off;
            unsigned const readable = self->seq_len - offset;
            unsigned const to_write = readable < writable ? readable : writable;
            
            memcpy(&buffer[dst_off], &src[offset], to_write);
            *written += to_write;
            if (!read_circular)
                break;
            offset = 0;
            dst_off += to_write;
        }
        return 0;
    }
    else if (self->type == rst_refSeqById || self->type == rst_refSeqBySeqId) {
        unsigned to_write = len;
        
        if (!self->circular || force_linear) {
            unsigned const readable = self->seq_len - offset;
            
            if (to_write > readable)
                to_write = readable;
        }
        return RefSeq_Read(self->u.refseq, offset, to_write, buffer, written);
    }
    return RC(rcAlign, rcType, rcReading, rcType, rcInvalid);
}

static
rc_t ReferenceMgr_LoadSeq(ReferenceMgr *const self, ReferenceSeq *obj)
{
    KDataBuffer readBuf;
    rc_t rc = KDataBufferMake(&readBuf, 8, self->max_seq_len);
    
    if (rc == 0) {
        char const *const id = obj->id;
        char const *const seqId = obj->seqId ? obj->seqId : id;
        TableWriterRefData data;
        INSDC_coord_zero offset = 0;
        
        obj->start_rowid = self->ref_rowid + 1;
        data.name.buffer = id;
        data.name.elements = strlen(id);
        data.read.buffer = readBuf.base;
        data.seq_id.buffer = seqId;
        data.seq_id.elements = strlen(seqId);
        data.force_READ_write = obj->type == rst_local || (self->options & ewrefmgr_co_allREADs);
        data.circular = obj->circular;
        
        if (self->writer == NULL) {
            uint32_t wopt = 0;
            
            wopt |= (self->options & ewrefmgr_co_allREADs) ? ewref_co_SaveRead : 0;
            wopt |= (self->options & ewrefmgr_co_Coverage) ? ewref_co_Coverage : 0;
            if( (rc = TableWriterRef_Make(&self->writer, self->db, wopt)) == 0 ) {
                TableWriterData mlen;
                
                mlen.buffer = &self->max_seq_len;
                mlen.elements = 1;
                rc = TableWriterRef_WriteDefaultData(self->writer, ewrefseq_cn_MAX_SEQ_LEN, &mlen);
            }
        }
        while (rc == 0 && offset < obj->seq_len) {
            unsigned row_len;
            
            rc = ReferenceSeq_ReadDirect(obj, offset, self->max_seq_len, false,
                                         readBuf.base, &row_len, true);
            if (rc != 0 || row_len == 0) break;
            
            data.read.elements = row_len;
            rc = TableWriterRef_Write(self->writer, &data, NULL);
            offset += row_len;
            ++self->ref_rowid;
        }
        KDataBufferWhack(&readBuf);
    }
    return rc;
}

LIB_EXPORT rc_t CC ReferenceMgr_GetSeq(ReferenceMgr const *const cself,
                                       ReferenceSeq const **const seq,
                                       char const id[])
{
    ReferenceMgr *const self = (ReferenceMgr *)cself;
    
    if  (self == NULL || seq == NULL || id == NULL )
        return RC(rcAlign, rcFile, rcConstructing, rcParam, rcNull);
    
    *seq = NULL;
    {
        ReferenceSeq *obj;
        rc_t rc = ReferenceMgr_OpenSeq(self, &obj, id, 0, NULL);
        
        if (rc) return rc;
        if (obj->start_rowid == 0) {
            rc = ReferenceMgr_LoadSeq(self, obj);
            if (rc) return rc;
        }
        *seq = obj;
    }
    return 0;
}

LIB_EXPORT rc_t CC ReferenceMgr_Verify(const ReferenceMgr* cself, const char* id, INSDC_coord_len seq_len, const uint8_t* md5)
{
#if 0
    rc_t rc = 0;
    ReferenceSeq* rseq;
    const KFile* kf;
    
    if( cself == NULL || id == NULL ) {
        rc = RC(rcAlign, rcFile, rcConstructing, rcParam, rcNull);
    } else if( (rc = ReferenceMgr_Find(cself, id, &rseq, &kf)) == 0 ) {
        if( rseq == NULL ) {
            uint64_t size = 0;
            if( (rc = KFileSize(kf, &size)) == 0 && size == 0 ) {
                rc = RC(rcAlign, rcTable, rcValidating, rcSize, rcEmpty);
            }
            KFileRelease(kf);
        } else if( rseq->local ) {
            if( rseq->seq_len != seq_len ) {
                rc = RC(rcAlign, rcFile, rcValidating, rcSize, rcUnequal);
                ALIGN_DBGERRP("%s->%s SEQ_LEN verification", rc, id, rseq->accession);
            }
            if( rc == 0 && md5 != NULL && memcmp(md5, rseq->u.local.md5, sizeof(rseq->u.local.md5)) != 0 ) {
                unsigned i;
                rc = RC(rcAlign, rcTable, rcValidating, rcChecksum, rcUnequal);
                ALIGN_DBGERRP("%s->%s MD5 verification", rc, id, rseq->accession);
                ALIGN_DBGF((" local '"));
                for(i = 0; i < sizeof(rseq->u.local.md5); i++) {
                    ALIGN_DBGF(("%02hx", rseq->u.local.md5[i]));
                }
                ALIGN_DBGF(("'  <> match '"));
                for(i = 0; i < sizeof(rseq->u.local.md5); i++) {
                    ALIGN_DBGF(("%02hx", md5[i]));
                }
                ALIGN_DBGF(("'\n"));
            } else {
                ALIGN_DBG("%s->%s MD5 verification ok", id, rseq->accession);
            }
        } else {
            const RefSeq* tmp = rseq->u.refseq.o;
            INSDC_coord_len o_len;
            const uint8_t* o_md5;

            if( tmp == NULL ) {
                if( (rc = RefSeqMgr_GetSeq(cself->rmgr, &tmp, rseq->accession, strlen(rseq->accession))) != 0 ||
                    (rc = RefSeq_SeqLength(tmp, &o_len)) != 0 ) {
                    ALIGN_DBGERRP("%s->%s verification", rc, id, rseq->accession);
                }
            } else {
                o_len = rseq->seq_len;
            }
            if( rc == 0 ) {
                if( seq_len != 0 && o_len != seq_len ) {
                    rc = RC(rcAlign, rcTable, rcValidating, rcSize, rcUnequal);
                    ALIGN_DBGERRP("%s->%s SEQ_LEN verification", rc, id, rseq->accession);
                } else {
                    ALIGN_DBG("%s->%s SEQ_LEN verification ok", id, rseq->accession);
                }
            }
            if( rc == 0 && (rc = RefSeq_MD5(tmp, &o_md5)) == 0 ) {
                if( md5 != NULL && o_md5 != NULL && memcmp(md5, o_md5, sizeof(rseq->u.local.md5)) != 0 ) {
                    rc = RC(rcAlign, rcTable, rcValidating, rcChecksum, rcUnequal);
                    ALIGN_DBGERRP("%s->%s MD5 verification", rc, id, rseq->accession);
                } else {
                    ALIGN_DBG("%s->%s MD5 verification ok", id, rseq->accession);
                }
            }
            if( tmp != rseq->u.refseq.o ) {
                RefSeq_Release(tmp);
            }
        }
    }
    if( rc == 0 ) {
        ALIGN_DBG("%s verification ok", id);
    } else {
        ALIGN_DBGERRP("%s verification", rc, id);
    }
    return rc;
#else
    if (cself == NULL || id == NULL)
        return RC(rcAlign, rcFile, rcValidating, rcParam, rcNull);
    {
        ReferenceMgr *self = (ReferenceMgr *)cself;
        ReferenceSeq *rseq;
        rc_t rc = ReferenceMgr_OpenSeq(self, &rseq, id, seq_len, md5);
        
        if (rc) return rc;
        if (rseq->seq_len != seq_len) {
            rc = RC(rcAlign, rcFile, rcValidating, rcSize, rcUnequal);
            ALIGN_DBGERRP("%s->%s SEQ_LEN verification", rc, id, rseq->seqId);
        }
        if (md5 && memcmp(md5, rseq->md5, sizeof(rseq->md5)) != 0 ) {
            unsigned i;
            
            rc = RC(rcAlign, rcTable, rcValidating, rcChecksum, rcUnequal);
            ALIGN_DBGERRP("%s->%s MD5 verification", rc, id, rseq->seqId);
            ALIGN_DBGF((" found '"));
            for(i = 0; i < sizeof(rseq->md5); i++) {
                ALIGN_DBGF(("%02hx", rseq->md5[i]));
            }
            ALIGN_DBGF(("'  != requested '"));
            for(i = 0; i < sizeof(rseq->md5); i++) {
                ALIGN_DBGF(("%02hx", md5[i]));
            }
            ALIGN_DBGF(("'\n"));
        } else {
            ALIGN_DBG("%s->%s MD5 verification ok", id, rseq->seqId);
        }
        if( rc == 0 ) {
            ALIGN_DBG("%s verification ok", id);
        } else {
            ALIGN_DBGERRP("%s verification", rc, id);
        }
        return rc;
    }
#endif
}

LIB_EXPORT rc_t CC ReferenceMgr_FastaPath(const ReferenceMgr* cself, const char* fasta_path)
{
    rc_t rc = 0;
    KDirectory* dir;

    if( cself == NULL || fasta_path == NULL ) {
        rc = RC(rcAlign, rcFile, rcConstructing, rcParam, rcNull);
    } else if( (rc = KDirectoryNativeDir(&dir)) == 0 ) {
        const KFile* kf;
        if( (rc = KDirectoryOpenFileRead(dir, &kf, "%s", fasta_path)) == 0 ) {
            rc = ReferenceMgr_FastaFile(cself, kf);
            KFileRelease(kf);
        }
        KDirectoryRelease(dir);
    }
    ALIGN_DBGERRP("from file %s", rc, fasta_path);
    return rc;
}

LIB_EXPORT rc_t CC ReferenceMgr_FastaFile(const ReferenceMgr* cself, const KFile* file)
{
    if( cself == NULL || file == NULL ) {
        return RC(rcAlign, rcFile, rcConstructing, rcParam, rcNull);
    }
    return ReferenceMgr_ImportFastaFile((ReferenceMgr *)cself, file, NULL);
}

static
int extrace_cigar_op(char *op,
                     uint32_t *op_len,
                     char const *const cigar,
                     uint32_t const cigar_len,
                     uint32_t const offset,
                     uint32_t const options,
                     unsigned *const bin)
{
    static char const op_txt[] = "MIDNSHP=XB??????";
    uint32_t i = offset;
    
    if (options & ewrefmgr_cmp_Binary) {
        uint32_t c;
        
        memcpy(&c, cigar + i * 4, 4);
        *bin = c & 0x0F;
        *op = op_txt[c & 0x0F];
        *op_len = c >> 4;
    }
    else {
        uint32_t len = 0;
        int ch;

        while(i < cigar_len && (ch = cigar[i]) != '\0' && isdigit(ch)) {
            len *= 10;
            len += ch - '0';
            ++i;
        }
        *bin = *op = ch;
        *op_len = len;
    }
    return i - offset;
}

static
rc_t cigar2offset(const uint32_t options, const void* cigar, uint32_t cigar_len, int32_t* out, uint32_t out_sz, uint32_t out_used,
                  INSDC_coord_len* seq_len, INSDC_coord_len* ref_len, INSDC_coord_len* max_ref_len)
{
    rc_t rc = 0;
    uint32_t i;
    uint32_t last_match = 0;
    uint32_t op_len;
    uint32_t last_soft_len = 0;
    uint32_t b_len = 0;
    uint32_t last_op = cigar_len - 1;
    char op;
    unsigned op_bin;
    
    if ((options & ewrefmgr_cmp_Binary) == 0) {
        for (i = 0; i < cigar_len; last_op = ++i) {
            i += extrace_cigar_op(&op, &op_len, cigar, cigar_len, i, options, &op_bin);
        }
    }
    
#if _DEBUGGING
    ALIGN_C_DBGF(("%s:%u: ", __func__, __LINE__));
    if( options & ewrefmgr_cmp_Binary ) {
        ALIGN_C_DBGF(("bin cigar '"));
        for(i = 0; i < cigar_len; i++) {
            extrace_cigar_op(&op, &op_len, cigar, cigar_len, i, options, &op_bin);
            ALIGN_C_DBGF(("%u%c", op_len, op));
        }
    } else {
        ALIGN_C_DBGF(("cigar '%.*s", cigar_len, cigar));
    }
    ALIGN_C_DBGF(("'[%u]\n", cigar_len));
#endif
    
    *seq_len = 0;
    *ref_len = 0;
    *max_ref_len = 0;
    memset(out, 0, out_used * sizeof(*out) * 2);
    for(i = 0; rc == 0 && i < cigar_len; i++) {
        i += extrace_cigar_op(&op, &op_len, cigar, cigar_len, i, options, &op_bin);
        if (op == 'P') {
            continue;
        }
        if( op != 'S' && op != 'I' ) {
            last_soft_len = 0;
        }
        switch(op) {
        case 'M':
        case '=':
        case 'X':
            *seq_len += op_len;
            *ref_len += op_len;
            *max_ref_len += op_len;
            if(b_len > op_len) {
                b_len -= op_len;
            }
            else {
                b_len=0;
                last_match = *seq_len;
            }
            break;
        case 'S':
        case 'I':
            last_soft_len += op_len;
            *max_ref_len += op_len;
            *seq_len += op_len;
            if (last_match < out_sz) {
                if (i < last_op) {
                    if (b_len > 0) {/** not right soft clip **/
                        assert(out[last_match] < 0);
                        out[last_match-out[last_match]] = 0x80000000;
                    }
                    out[last_match] += -op_len;
                }
                if (b_len == 0) last_match = *seq_len;
            } else {
                rc = RC(rcAlign, rcFile, rcProcessing, rcData, rcInconsistent);
            }
            
            break;
        case 'B':
            /* Complete Genomics CIGAR style specific:
             overlap between consecutive reads
             ex: sequence 6 bases: ACACTG, reference 2 bases: ACTG,
             cigar will be: 2M2B2M
             no need to move sequence position
             */
            if( last_match < out_sz ) {
                if(b_len > 0 && i < cigar_len -1){/** not right soft clip **/
                    assert(out[last_match] < 0);
                    out[last_match-out[last_match]] = 0x80000000;
                }
                out[last_match] -= op_len;
            } else {
                rc = RC(rcAlign, rcFile, rcProcessing, rcData, rcInconsistent);
            }
            if( *ref_len < op_len ) {
                *ref_len = 0;
            } else {
                *ref_len -= op_len;
            }
            *max_ref_len += op_len;
            b_len += op_len;
            break;
        case 'D':
        case 'N':
            if( last_match < out_sz ) {
                out[last_match] += op_len;
            } else if (last_match == out_sz && last_match > 0 ){ /*** delete at the end ***/
                out[last_match-1] += op_len;
            } else {
                rc = RC(rcAlign, rcFile, rcProcessing, rcData, rcInconsistent);
            }
            *ref_len += op_len;
            *max_ref_len += op_len;
            break;
        case 'H':
            if ((options & ewrefmgr_co_AcceptHardClip) == 0)
            {
                rc = RC(rcAlign, rcFile, rcProcessing, rcData, rcInvalid);
                (void)LOGERR(klogErr, rc, "Hard clipping of sequence data is not allowed");
            }
            break;
        default:
            rc = RC(rcAlign, rcFile, rcProcessing, rcData, rcUnrecognized);
            if ((options & ewrefmgr_cmp_Binary) == 0)
                (void)PLOGERR(klogErr, (klogErr, rc, "Invalid or unrecognized CIGAR operation '$(opcode)'", "opcode=%c", op));
            else if (op != '?')
                (void)PLOGERR(klogErr, (klogErr, rc, "Invalid or unrecognized CIGAR operation '$(opcode)' (binary code: $(opbin))", "opcode=%c,opbin=%u", op, op_bin));
            else
                (void)PLOGERR(klogErr, (klogErr, rc, "Invalid or unrecognized CIGAR operation (binary code: $(opbin))", "opbin=%u", op_bin));
            break;
        }
    }
    if( !(options & ewrefmgr_cmp_Exact) && last_soft_len > 0 ) {
        ALIGN_C_DBG("trailing soft clip %u @ %u", last_soft_len, last_match);
        out[last_match] += last_soft_len;
    }
#if _DEBUGGING
    ALIGN_C_DBGF(("%s:%u: offsets: ", __func__, __LINE__));
    last_match = last_match > *seq_len ? last_match : *seq_len;
    for(i = 0; i <= last_match; i++) {
        if( i == *seq_len ) {
            ALIGN_C_DBGF((" | "));
        }
        ALIGN_C_DBGF(("%i%c", out[i], ((i + 1) % 5) ? ',' : ' '));
    }
    ALIGN_C_DBGF((" cigar derived seq_len %u, ref_len %u, max_ref_len %u\n", *seq_len, *ref_len, *max_ref_len));
#endif
    return rc;
}

static
void ReferenceSeq_TranslateOffset_int(ReferenceSeq const *const cself,
                                      INSDC_coord_zero const offset,
                                      int64_t *const ref_id,
                                      INSDC_coord_zero *const ref_start,
                                      uint64_t *const global_ref_start)
{
    if (ref_id)
        *ref_id = cself->start_rowid + offset / cself->mgr->max_seq_len;
    
    if (ref_start)
        *ref_start = offset % cself->mgr->max_seq_len;
    
    if (global_ref_start)
        *global_ref_start = (cself->start_rowid - 1) * cself->mgr->max_seq_len + offset;
}

LIB_EXPORT rc_t CC ReferenceMgr_Compress(const ReferenceMgr* cself, uint32_t options,
                                         const char* id, INSDC_coord_zero offset,
                                         const char* seq, INSDC_coord_len seq_len,
                                         const void* cigar, uint32_t cigar_len,
                                         INSDC_coord_zero allele_offset, const char* allele, INSDC_coord_len allele_len,INSDC_coord_zero offset_in_allele,
                                         const void* allele_cigar, uint32_t allele_cigar_len,
                                         TableWriterAlgnData* data)
{
    rc_t rc = 0;
    const ReferenceSeq* refseq;

    if( cself == NULL || id == NULL ) {
        rc = RC(rcAlign, rcFile, rcProcessing, rcParam, rcNull);
    } else if( (rc = ReferenceMgr_GetSeq(cself, &refseq, id)) == 0 ) {
        rc = ReferenceSeq_Compress(refseq, options, offset, seq, seq_len, cigar, cigar_len,
                                   allele_offset, allele, allele_len,offset_in_allele,
				   allele_cigar, allele_cigar_len, data);
        ReferenceSeq_Release(refseq);
    }
    ALIGN_C_DBGERR(rc);
    return rc;
}

LIB_EXPORT rc_t CC ReferenceSeq_Compress(const ReferenceSeq* cself, const uint32_t options, INSDC_coord_zero offset,
                                         const char* seq, INSDC_coord_len seq_len,
                                         const void* cigar, uint32_t cigar_len,
                                         INSDC_coord_zero allele_offset, const char* allele,
					 INSDC_coord_len allele_len,
					 INSDC_coord_zero offset_in_allele,
                                         const void* allele_cigar, uint32_t allele_cigar_len,
                                         TableWriterAlgnData* data)
{
    rc_t rc = 0;

    if( cself == NULL || seq == NULL || cigar == NULL || cigar_len == 0 || data == NULL ||
        (!(allele == NULL && allele_len == 0 && allele_cigar == NULL && allele_cigar_len == 0) &&
         !(allele != NULL && allele_cigar != NULL && allele_cigar_len != 0)) ) {
        return RC(rcAlign, rcFile, rcProcessing, rcParam, rcInvalid);
    }
    if (seq_len * 2 > cself->mgr->compress.elem_count) {
        rc = KDataBufferResize(&cself->mgr->compress, seq_len * 2);
        if (rc) return rc;
    }
    if ((rc = ReferenceSeq_ReOffset(cself->circular, cself->seq_len, &offset)) == 0 ) {
        INSDC_coord_len i, seq_pos = 0, allele_ref_end = 0, ref_len = 0, rl = 0, max_rl = 0;
        INSDC_coord_zero* read_start = &((INSDC_coord_zero*)(data->read_start.buffer))[data->ploidy];
        INSDC_coord_len* read_len = &((INSDC_coord_len*)(data->read_len.buffer))[data->ploidy];
        bool* has_ref_offset, *has_mismatch;
        int32_t* ref_offset;
        uint8_t* mismatch; 
        uint8_t sref_buf[64 * 1024];
        void *href_buf = NULL;
        uint8_t *ref_buf = sref_buf;
	int32_t  allele_off_buf[1024];
#if _DEBUGGING
        uint64_t i_ref_offset_elements, i_mismatch_elements;
        char x[4096];
#endif

        if( data->ploidy == 0 ) {
            data->has_ref_offset.elements = seq_len;
            data->ref_offset.elements = 0;
            data->has_mismatch.elements = seq_len;
            data->mismatch.elements = 0;
            *read_start = 0;
        }
        else {
            data->has_ref_offset.elements += seq_len;
            data->has_mismatch.elements += seq_len;
            *read_start = read_start[-1] + read_len[-1];
        }
        *read_len = seq_len;
        has_ref_offset = &((bool*)(data->has_ref_offset.buffer))[*read_start];
        ref_offset = (int32_t*)(data->ref_offset.buffer);
        has_mismatch = &((bool*)(data->has_mismatch.buffer))[*read_start];
        mismatch = (uint8_t*)(data->mismatch.buffer);

#if _DEBUGGING
        i_ref_offset_elements = data->ref_offset.elements;
        i_mismatch_elements = data->mismatch.elements;
        ALIGN_C_DBG("align%s '%.*s'[%u] to '%s:%s' at %i", (options & ewrefmgr_cmp_Exact) ? " EXACT" : "",
                    seq_len, seq, seq_len, cself->id, cself->seqId, offset);
#endif
        if( allele != NULL ) {
            /* determine length of reference for subst by allele */
            ALIGN_C_DBG("apply allele %.*s[%u] at %i w/cigar below",
                        allele_len, allele, allele_len, allele_offset);
            rc = cigar2offset(options, allele_cigar, allele_cigar_len,
                    allele_off_buf, sizeof(allele_off_buf)/sizeof(*allele_off_buf)/2,
                    allele_len, &seq_pos, &allele_ref_end, &max_rl);
            /* where allele ends on reference */
            allele_ref_end += allele_offset;
        }
        if( rc == 0 ) {
            rc = cigar2offset(options, cigar, cigar_len,
                              (int32_t*)(cself->mgr->compress.base), cself->mgr->compress.elem_count / 2,
                             seq_len, &seq_pos, &rl, &max_rl);
        }
        if( allele != NULL ) {
            if( allele_offset + allele_ref_end < offset || allele_offset >= offset + rl ) {
                (void)PLOGMSG(klogWarn, (klogWarn,
                    "allele $(a) offset $(ao) $(ac) is not within referenced region in $(id) at offset $(ro) $(rc)",
                    "a=%.*s,ao=%i,ac=%.*s,id=%s,ro=%i,rc=%.*s",
                    allele_len, allele, allele_offset, (options & ewrefmgr_cmp_Binary) ? 0 : allele_cigar_len, allele_cigar,
                    cself->seqId, offset, (options & ewrefmgr_cmp_Binary) ? 0 : cigar_len, cigar));
                allele = NULL;
            }
        }
        if( rc == 0 ) {
            if( !(options & ewrefmgr_cmp_Exact) ) {
                /* do not write leading deletes just move reference offset */
                int32_t *const compress_buf = cself->mgr->compress.base;
                if( compress_buf[0] > 0 ) {
                    offset += compress_buf[0];
                    rl -= compress_buf[0];
                    max_rl -= compress_buf[0];
                    compress_buf[0] = 0;
                    ALIGN_C_DBG("adjusted offset %i", offset);
                }
            }
            ref_len = rl;
            if( (offset + max_rl) > cself->seq_len && !cself->circular ) {
                max_rl = cself->seq_len - offset;
                if( max_rl < rl ) {
                    /* ref_len used for compression cannot be shorter than ref_len derived from cigar,
                       if there is a shortage it will fail later here */
                    max_rl = rl;
                }
                ALIGN_C_DBG("max_ref_len truncated to %u cause it goes beyond refseq length %lu at offset %i",
                             max_rl, cself->seq_len, offset);
            }
            ALIGN_C_DBG("chosen REF_LEN %u, ref len for match %u", ref_len, max_rl);

            assert(seq_len == seq_pos);
            if (max_rl > sizeof(sref_buf)) {
                if (href_buf)
                    free(href_buf);
                href_buf = malloc(max_rl);
                if (href_buf == NULL)
                    rc = RC(rcAlign, rcFile, rcProcessing, rcMemory, rcExhausted);
                ref_buf = href_buf;
            }
            if (rc == 0) {
                int64_t ref_pos;

                if( allele != NULL ) {
                    /* subst allele in reference */
                    if( allele_offset <= offset ) {
                        /* move allele start inside referenced chunk */
			    allele     += offset_in_allele;
			    allele_len -= offset_in_allele;
                            rl = 0;
                    } else {
                        /* fetch portion of reference which comes before allele */
                        rl = allele_offset - offset;
                        rc = ReferenceSeq_ReadDirect((ReferenceSeq*)cself, offset, rl, true, ref_buf, &i, false);
                        if( rc == 0 && rl != i ) {
                            /* here we need to test it otherwise excessive portion of allele could be fetch in next if */
                            rc = RC(rcAlign, rcFile, rcProcessing, rcRange, rcExcessive);
                        }
                    }
                    if( rc == 0 && allele_len < (max_rl - rl) ) {
                        memcpy(&ref_buf[rl], allele, allele_len);
                        rl += allele_len;
                        /* append tail of actual reference */
                        rc = ReferenceSeq_ReadDirect((ReferenceSeq*)cself, allele_ref_end, max_rl - rl, true, &ref_buf[rl], &i, false);
                        rl += i;
                    } else if( rc == 0 ) {
                        /* allele is longer than needed */
                        memcpy(&ref_buf[rl], allele, max_rl - rl);
                        rl = max_rl;
                    }
                } else {
                    rc = ReferenceSeq_ReadDirect((ReferenceSeq*)cself, offset, max_rl, true, ref_buf, &rl, false);
                }
                if( rc != 0 || max_rl != rl ) {
                    rc = rc ? rc : RC(rcAlign, rcFile, rcProcessing, rcRange, rcExcessive);
                    ALIGN_C_DBGERRP("refseq is shorter: at offset %i need %u bases", rc, offset, max_rl);
                } else {
                    int32_t *const compress_buf = cself->mgr->compress.base;
                    for(seq_pos = 0, ref_pos = 0; seq_pos < seq_len; seq_pos++, ref_pos++) {
                        has_ref_offset[seq_pos] = compress_buf[seq_pos] != 0;
                        if( has_ref_offset[seq_pos]){
                            if(compress_buf[seq_pos] == 0x80000000 ){
                                compress_buf[seq_pos] = 0; /** the hack has served its purpose **/
                            } else {
                                ref_offset[data->ref_offset.elements++] = compress_buf[seq_pos];
                                ref_pos += compress_buf[seq_pos];
                            }
                        }
                        if( (ref_pos < 0) || (ref_pos >= max_rl) || 
                            ((toupper(ref_buf[ref_pos]) != toupper(seq[seq_pos])) && (seq[seq_pos] != '=')) ) {
                            has_mismatch[seq_pos] = true;
                            mismatch[data->mismatch.elements++] = seq[seq_pos];
                        } else {
                            /* match! */
                            has_mismatch[seq_pos] = false;
                        }
                    }
                }
            }
        }
#if _DEBUGGING
        if(rc == 0 ) {
            int32_t j;
            memset(x, '-', sizeof(x) - 1);
            x[sizeof(x) - 2] = '\0';

            ALIGN_C_DBG("ref: %.*s [%u]", max_rl, ref_buf, max_rl);
            ALIGN_C_DBGF(("%s:%u: ref: ", __func__, __LINE__));
            for(seq_pos = 0, j = 0, rl = 0, i = 0; seq_pos < seq_len; seq_pos++, j++) {
                if( has_ref_offset[seq_pos] ) {
                    if( ref_offset[i_ref_offset_elements + rl] > 0 ) {
                        ALIGN_C_DBGF(("%.*s", (uint32_t)(ref_offset[i_ref_offset_elements + rl]), &ref_buf[j]));
                    } else {
                        i = -ref_offset[i_ref_offset_elements + rl];
                    }
                    j += ref_offset[i_ref_offset_elements + rl];
                    rl++;
                }
                ALIGN_C_DBGF(("%c", (j < 0 || j >= max_rl) ? '-' : (i > 0) ? tolower(ref_buf[j]) : ref_buf[j]));
                if( i > 0  ) {
                    i--;
                }
            }
            ALIGN_C_DBGF(("\n%s:%u: seq: ", __func__, __LINE__));
            for(i = 0, j = 0; i < seq_len; i++) {
                if( has_ref_offset[i] && ref_offset[i_ref_offset_elements + j++] > 0 ) {
                    ALIGN_C_DBGF(("%.*s", ref_offset[i_ref_offset_elements + j - 1], x));
                }
                ALIGN_C_DBGF(("%c", seq[i]));
            }
            ALIGN_C_DBGF((" [%u]\n", seq_len));
            ALIGN_C_DBGF(("%s:%u: hro: ", __func__, __LINE__));
            for(i = 0, j = 0; i < seq_len; i++) {
                if( has_ref_offset[i] && ref_offset[i_ref_offset_elements + j++] > 0 ) {
                    ALIGN_C_DBGF(("%.*s", ref_offset[i_ref_offset_elements + j - 1], x));
                }
                ALIGN_C_DBGF(("%c", has_ref_offset[i] + '0'));
            }
            ALIGN_C_DBGF((", ro:"));
            for(i = i_ref_offset_elements; i < data->ref_offset.elements; i++) {
                ALIGN_C_DBGF((" %i,", ref_offset[i]));
            }
            ALIGN_C_DBGF(("[%u]\n", data->ref_offset.elements - i_ref_offset_elements));
            ALIGN_C_DBGF(("%s:%u: hmm: ", __func__, __LINE__));
            for(i = 0, j = 0; i < seq_len; i++) {
                if( has_ref_offset[i] && ref_offset[i_ref_offset_elements + j++] > 0 ) {
                    ALIGN_C_DBGF(("%.*s", ref_offset[i_ref_offset_elements + j - 1], x));
                }
                ALIGN_C_DBGF(("%c", has_mismatch[i] + '0'));
            }
            ALIGN_C_DBGF((", mm: '%.*s'[%u]\n", (int)(data->mismatch.elements - i_mismatch_elements),
                &mismatch[i_mismatch_elements], data->mismatch.elements - i_mismatch_elements));
        }
#endif
        if( rc == 0 ) {
            if( data->ploidy == 0 ) {
                int64_t *const ref_id = (int64_t *)data->ref_id.buffer;
                INSDC_coord_zero *const ref_start = (INSDC_coord_zero *)data->ref_start.buffer;
                uint64_t *const global_ref_start = (uint64_t *)data->global_ref_start.buffer;
                
                data->ref_1st_row_id = cself->start_rowid;
                data->effective_offset = offset;
                data->ref_len = ref_len;
                ALIGN_C_DBGF(("%s:%u: reference 1st ROW_ID %li OFFSET %i REF_LEN %u",
                    __func__, __LINE__, data->ref_1st_row_id, data->effective_offset, data->ref_len));
                
                ReferenceSeq_TranslateOffset_int(cself, offset, ref_id, ref_start, global_ref_start);
                
                if (ref_id) {
                    data->ref_id.elements = 1;
                    ALIGN_C_DBGF((" REF_ID %li", ref_id[0]));
                }
                if (ref_start) {
                    data->ref_start.elements = 1;
                    ALIGN_C_DBGF((" REF_START %i", ref_start[0]));
                }
                if (global_ref_start) {
                    data->global_ref_start.elements = 1;
                    ALIGN_C_DBGF((" GLOBAL_REF_START %lu", global_ref_start[0]));
                }
                ALIGN_C_DBGF(("\n"));
            } else {
                if( data->ref_1st_row_id != cself->start_rowid || data->effective_offset != offset ) {
                    rc = RC(rcAlign, rcFile, rcProcessing, rcData, rcInconsistent);
                    (void)PLOGERR(klogErr, (klogErr, rc,
                        "all reads in alignment record must align to same refseq at same location $(r1)@$(o1) <> $(r2):$(a2)@$(o2)",
                        "r1=%li,o1=%i,r2=%s,a2=%s,o2=%i", data->ref_1st_row_id, data->effective_offset, cself->id, cself->seqId, offset));
                } else if( data->ref_len != ref_len ) {
                    rc = RC(rcAlign, rcFile, rcProcessing, rcData, rcInconsistent);
                    (void)PLOGERR(klogErr, (klogErr, rc,
                        "all reads in alignment record must have same size projection on refseq $(rl1) <> $(rl2) $(r):$(a)@$(o)",
                        "rl1=%u,rl2=%u,r=%s,a=%s,o=%i", data->ref_len, ref_len, cself->id, cself->seqId, offset));
                }
            }
        }
        if( rc == 0 ) {
            data->ploidy++;
            data->read_start.elements = data->ploidy;
            data->read_len.elements = data->ploidy;
        }
        if (href_buf)
            free(href_buf);
    }
    ALIGN_C_DBGERR(rc);
    return rc;
}

LIB_EXPORT rc_t CC ReferenceSeq_Read(const ReferenceSeq* cself, INSDC_coord_zero offset, INSDC_coord_len len,
                                     uint8_t* buffer, INSDC_coord_len* ref_len)
{
    rc_t rc = 0;

    if( cself == NULL || buffer == NULL || ref_len == NULL ) {
        rc = RC(rcAlign, rcFile, rcReading, rcParam, rcNull);
    } else {
        rc = ReferenceSeq_ReadDirect((ReferenceSeq*)cself, offset, len, true, buffer, ref_len, false);
    }
    ALIGN_DBGERR(rc);
    return rc;
}

LIB_EXPORT rc_t CC ReferenceSeq_Get1stRow(const ReferenceSeq* cself, int64_t* row_id)
{
    rc_t rc = 0;

    if( cself == NULL || row_id == NULL ) {
        rc = RC(rcAlign, rcFile, rcReading, rcParam, rcNull);
    } else {
        *row_id = cself->start_rowid;
    }
    return rc;
}

LIB_EXPORT rc_t CC ReferenceSeq_AddCoverage(const ReferenceSeq* cself, INSDC_coord_zero offset, const ReferenceSeqCoverage* data)
{
    rc_t rc = 0;

    if( cself == NULL || data == NULL) {
        rc = RC(rcAlign, rcFile, rcReading, rcParam, rcNull);
    } else if( !(cself->mgr->options & ewrefmgr_co_Coverage) ) {
        rc = RC( rcAlign, rcType, rcWriting, rcData, rcUnexpected);
        ALIGN_R_DBGERRP("coverage %s", rc, "data");
    } else if( (rc = ReferenceSeq_ReOffset(cself->circular, cself->seq_len, &offset)) == 0 ) {
        rc = TableWriterRef_WriteCoverage(cself->mgr->writer, cself->start_rowid, offset, data);
    }
    ALIGN_DBGERR(rc);
    return rc;
}

LIB_EXPORT rc_t CC ReferenceSeq_Release(const ReferenceSeq *cself)
{
    return 0;
}
