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

#include <vfs/manager-priv.h> /* VFSManagerOpenFileReadDecrypt */
#include <vfs/manager.h> /* VFSManagerMake */
#include <vfs/path.h> /* VPathMake */

#include <kapp/main.h>
#include <kapp/args.h>
#include <klib/out.h>
#include <klib/log.h>
#include <klib/text.h>
#include <klib/rc.h>
#include <klib/namelist.h>
#include <klib/container.h>
#include <klib/debug.h>
#include <klib/data-buffer.h>
#include <klib/sort.h>

#include <kdb/manager.h>
#include <kdb/database.h>
#include <kdb/table.h>
#include <kdb/meta.h>
#include <kdb/namelist.h>
#include <kdb/consistency-check.h>
#include <kdb/kdb-priv.h> /* KTableOpenDirectoryRead */

#include <vdb/manager.h>
#include <vdb/schema.h>
#include <vdb/database.h>
#include <vdb/table.h>
#include <vdb/cursor.h>
#include <vdb/dependencies.h> /* UIError */
#include <vdb/vdb-priv.h> /* VTableOpenKTableRead */

#include <krypto/encfile.h> /* KEncFileValidate */

#include <kfs/kfs-priv.h>
#include <kfs/sra.h>
#include <kfs/tar.h>
#include <kfs/file.h> /* KFileRelease */

#include <insdc/insdc.h>
#include <insdc/sra.h>
#include <sra/srapath.h>
#include <sra/sradb.h>
#include <sra/sraschema.h>

#include <sysalloc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <common/test_assert.h>

static bool exhaustive;
static bool md5_required;
static bool ref_int_check;
static bool s_IndexOnly;

typedef struct node_s {
    int parent;
    int prvSibl;
    int nxtSibl;
    int firstChild;
    unsigned depth;
    unsigned name;
    uint32_t objType;
} node_t;
typedef struct cc_context_s {
    node_t *nodes;
    char *names;
    rc_t rc;
    unsigned num_columns;
    unsigned nextNode;
    unsigned nextName;
} cc_context_t;
static
rc_t kdbcc ( const KDBManager *mgr, char const name[], uint32_t mode,
    KPathType *pathType, bool is_file, node_t nodes[], char names[],
    INSDC_SRA_platform_id platform )
{
    rc_t rc = 0;
    cc_context_t ctx;

    uint32_t level = ( mode & 4 ) ? 3 : ( mode & 2 ) ? 1 : 0;
    if (s_IndexOnly)
        level |= CC_INDEX_ONLY;


    memset(&ctx, 0, sizeof(ctx));
    ctx.nodes = &nodes[0];
    ctx.names = &names[0];

    if (KDBManagerExists(mgr, kptDatabase, name))
        *pathType = kptDatabase;
    else if (KDBManagerExists(mgr, kptTable, name))
        *pathType = kptTable;
    else if (*pathType == kptDatabase || *pathType == kptTable)
        /* is known already: may be encrypted */ ( void) 0;
    else
    {
        rc = RC(rcExe, rcPath, rcValidating, rcType, rcUnknown);
        (void)PLOGERR(klogErr, (klogErr, rc, "Object '$(table)' "
            "has unknown type", "table=%s", name));
        return rc;
    }

    return rc;
}

typedef struct ColumnInfo_s {
    char const *name;
    union {
        void     const *vp;
        char     const *string;
        bool     const *tf;
        int8_t   const *i8;
        uint8_t  const *u8;
        int16_t  const *i16;
        uint16_t const *u16;
        int32_t  const *i32;
        uint32_t const *u32;
        int64_t  const *i64;
        uint64_t const *u64;
        float    const *f32;
        double   const *f64;
    } value;
    uint32_t idx;
    uint32_t elem_bits;
    uint32_t elem_count;
} ColumnInfo;
static rc_t EncFileReadAll(const char *name,
    char *buffer, size_t bsize, size_t *num_read)
{
    rc_t rc = 0;

    VFSManager *mgr = NULL;
    VPath *path = NULL;
    const KFile *f = NULL;

    rc = VFSManagerMake(&mgr);
    if (rc != 0)
    {   (void)LOGERR(klogErr, rc, "Failed to VFSManagerMake()"); }

    if (rc == 0) {
        rc = VPathMake(&path, name);
        if (rc != 0) {
            (void)PLOGERR(klogErr, (klogErr, rc,
                "Failed to VPathMake($(name))", "name=%s", name));
        }
    }

    if (rc == 0) {
        rc = VFSManagerOpenFileReadDecrypt(mgr, &f, path);
        if (rc != 0) {
            (void)PLOGERR(klogErr, (klogErr, rc,
                "Failed to VFSManagerOpenFileReadDecrypt($(name))",
                "name=%s", name));
        }
    }

    if (rc == 0) {
        rc = KFileReadAll(f, 0, buffer, bsize, num_read);
        if (rc != 0) {
            (void)PLOGERR(klogErr, (klogErr, rc,
                "Failed to KFileReadAll($(name))", "name=%s", name));
        }
    }

    KFileRelease(f);
    VPathRelease(path);
    VFSManagerRelease(mgr);

    return rc;
}

#if 0
static rc_t verify_encryption(const KDirectory *dir, const char *name,
    bool *enc, bool *sra)
{
    rc_t rc = 0;
    const KFile *f = NULL;
    char buffer[24];
    size_t num_read = ~0;

    assert(enc && sra);
    *enc = false;
    *sra = true;

    rc = KDirectoryOpenFileRead(dir, &f, name);
    if (rc == 0)
    {   rc = KFileReadAll(f, 0, &buffer, sizeof buffer, &num_read); }

    if (rc == 0) {
        size_t sz = num_read < 8 ? num_read : 8;
        assert(num_read <= sizeof buffer);
        rc = KFileIsEnc(buffer, sz);
        if (rc == 0)
        {
            (void)PLOGMSG(klogInfo, (klogInfo, "File '$(f)' is encrypted",
                "f=%s", name));
            *enc = true;
            rc = KEncFileValidate(f);
            if (rc != 0) {
                (void)PLOGERR(klogErr, (klogErr, rc, "Failure of encryption "
                    "validation of file '$(name)'", "name=%s", name));
            }
            else {
                (void)PLOGMSG(klogInfo, (klogInfo, "File '$(f)': encryption OK",
                    "f=%s", name));
                rc = EncFileReadAll(name, buffer, sizeof buffer, &num_read);
            }
        }
        else {
            rc = 0;
            (void)PLOGMSG(klogInfo, (klogInfo, "File '$(f)' is not encrypted",
                "f=%s", name));
        }
    }

    if (rc == 0) {
        rc = KFileIsSRA(buffer, num_read);
        if (rc == 0) {
            *sra = true;
        }
        else if (rc
            == SILENT_RC(rcFS, rcFile, rcIdentifying, rcBuffer, rcWrongType))
        {
            rc = 0;
            *sra = false;
        }
        else {
            (void)PLOGERR(klogErr, (klogErr, rc,
                "Failed to KFileIsSRA($(name))", "name=%s", name));
        }
    }

    KFileRelease(f);

    return rc;
}
#endif

static rc_t init_dbcc(KDirectory const *dir, char const name[], bool is_file,
    node_t **nodes, char **names, KPathType *pathType)
{
    KDirectory const *obj;
    rc_t rc = 0;

    assert(pathType);

    if (is_file) {
        rc = KDirectoryOpenSraArchiveRead_silent(dir, &obj, false, name);
        if (rc != 0)
        {   rc = KDirectoryOpenTarArchiveRead_silent(dir, &obj, false, name); }
        if (rc != 0) {
            const VDBManager *mgr = NULL;
            const VTable *tbl = NULL;
            VSchema *sra_schema = NULL;
            rc = VDBManagerMakeRead(&mgr, dir);
            if (rc != 0)
            {   return rc; }
            for ( ; rc == 0; ) {
                rc = VDBManagerOpenTableRead(mgr, &tbl, sra_schema, name);
                VSchemaRelease(sra_schema);
                if (rc == 0) {
                    const KTable *ktbl = NULL;
                    rc = VTableOpenKTableRead(tbl, &ktbl);
                    if (rc == 0)
                    {   rc = KTableOpenDirectoryRead(ktbl, &obj); }
                    KTableRelease(ktbl);
                    *pathType = kptTable;
                    break;
                }
                else if (GetRCState(rc) == rcNotFound
                    && GetRCObject(rc) == rcSchema && sra_schema == NULL)
                {
                     rc = VDBManagerMakeSRASchema(mgr, &sra_schema);
                }
                else {
                    const VDatabase *db = NULL;
                    const KDatabase *kdb = NULL;
                    rc = VDBManagerOpenDBRead(mgr, &db, NULL, name);
                    if (rc == 0) {
                        rc = VDatabaseOpenKDatabaseRead(db, &kdb);
                    }
                    if (rc == 0)
                    {   rc = KDatabaseOpenDirectoryRead(kdb, &obj); }
                    if (rc == 0)
                    {   *pathType = kptDatabase; }
                    KDatabaseRelease(kdb);
                    VDatabaseRelease(db);
                    break;
                }
            }
            VTableRelease(tbl);
            VDBManagerRelease(mgr);
        }
    }
    else {
        rc = KDirectoryOpenDirRead(dir, &obj, false, name);
    }
    if (rc)
        return rc;
    KDirectoryRelease(obj);

    return rc;
}

typedef struct id_pair_s {
    int64_t first;
    int64_t second;
} id_pair_t;
#include <kfg/config.h>
#include <vfs/resolver.h>
#define rcResolver   rcTree
static bool NotFoundByResolver(rc_t rc) {
    if (GetRCModule(rc) == rcVFS) {
        if (GetRCTarget(rc) == rcResolver) {
            if (GetRCContext(rc) == rcResolving) {
                if (GetRCState(rc) == rcNotFound) {
                    return true;
                }
            }
        }
    }
    return false;
}
static rc_t resolver(const VDatabase *db, const char *name) {
    bool existing = true;
    VFSManager* mgr = NULL;
    KConfig *cfg = NULL;
    const VDBDependencies* dep = NULL;
    VResolver* resolver = NULL;
    uint32_t count = 0;
    rc_t rc = VFSManagerMake(&mgr);
    assert(name);
    if (rc == 0) {
        rc = KConfigMake(&cfg, NULL);
    }
    if (rc == 0) {
        rc = VFSManagerMakeResolver(mgr, &resolver, cfg);
    }
    if (name[0] != 'S' && db != NULL) {
        if (rc == 0) {
            rc = VDatabaseListDependencies(db, &dep, false);
        }
        if (rc == 0) {
            rc = VDBDependenciesCount(dep, &count);
        }
    }
    else {
        existing = false;
        count = 1;
    }
    if (rc == 0) {
        uint32_t i = 0;
        for (i = 0; i < count && rc == 0; ++i) {
            VPath* acc = NULL;
            const VPath* local = NULL;
            const VPath* remote = NULL;
            const VPath* cache = NULL;
            bool isLocal = false;
            const char* dSeqId = NULL;
            const char* dLocal = NULL;
            const char* dRemote = NULL;
            const char* dCache = NULL;
            const String *sLocal = NULL;
            const String *sRemote = NULL;
            const String *sCache = NULL;
            bool remoteNotFound = false;
            if (existing) {
                rc = VDBDependenciesLocal(dep, &isLocal, i);
                if (rc == 0) {
                    rc = VDBDependenciesSeqId(dep, &dSeqId, i);
                }
                if (rc == 0) {
                    OUTMSG(("%d: SeqId=%s\n", i, dSeqId));
                }
            }
            else {
                isLocal = false;
                dSeqId = name;
            }
            if (!isLocal && rc == 0) {
                rc = VPathMake(&acc, dSeqId);
                if (existing) {
                    if (rc == 0) {
                        rc = VDBDependenciesPath(dep, &dLocal, i);
                    }
                    if (rc == 0) {
                        OUTMSG(("%d: Local=%s\n", i, dLocal));
                    }
                    if (rc == 0) {
                        rc = VDBDependenciesPathRemote(dep, &dRemote, i);
                    }
                    if (rc == 0) {
                        OUTMSG(("%d: Remote=%s\n", i, dRemote));
                    }
                    if (rc == 0) {
                        rc = VDBDependenciesPathCache(dep, &dCache, i);
                    }
                    if (rc == 0) {
                        OUTMSG(("%d: Cache=%s\n", i, dCache));
                    }
                }
            }
            if (rc == 0) {
                rc = VResolverLocal(resolver, acc, &local);
                if (rc == 0) {
                    rc = VPathMakeString(local, &sLocal);
                    if (rc == 0) {
                        OUTMSG(("local = %.*s\n", sLocal->size, sLocal->addr));
                    }
                }
                else if (NotFoundByResolver(rc)) {
                    OUTMSG(("local not found\n"));
                    rc = 0;
                }
            }
            if (rc == 0) {
                rc = VResolverRemote(resolver, acc, &remote, NULL);
                if (rc == 0) {
                    rc = VPathMakeString(remote, &sRemote);
                    if (rc == 0) {
                        OUTMSG(("remote = %.*s\n",
                            sRemote->size, sRemote->addr));
                    }
                }
                else if (NotFoundByResolver(rc)) {
                    remoteNotFound = true;
                    OUTMSG(("remote not found\n"));
                    rc = 0;
                }
            }
            if (rc == 0) {
                if (remoteNotFound) {
                    OUTMSG(("remote not found: skipped VResolverCache call\n"));
                }
                else {
                    uint64_t file_size = 0;
                    rc = VResolverCache(resolver, remote, &cache, file_size);
                    if (rc == 0) {
                        rc = VPathMakeString(cache, &sCache);
                    }
                    if (rc == 0) {
                        OUTMSG(("cache = %.*s\n", sCache->size, sCache->addr));
                    }
                }
            }
            free((void*)sCache);
            free((void*)sRemote);
            free((void*)sLocal);
            VPathRelease(cache);
            VPathRelease(remote);
            VPathRelease(local);
            VPathRelease(acc);
        }
    }
    OUTMSG(("in resolver: %R\n", rc));
    VDBDependenciesRelease(dep);
    VResolverRelease(resolver);
    KConfigRelease(cfg);
    VFSManagerRelease(mgr);
    return rc;
}

static rc_t verify_mgr_database(VDBManager const *mgr,
    char const name[], node_t const nodes[], char const names[])
{
    VDatabase const *child;
    rc_t rc = VDBManagerOpenDBRead(mgr, &child, NULL, name);
    
    if (rc == 0) {
        rc = resolver(child, name);
        VDatabaseRelease(child);
    }

    return rc;
}

static rc_t sra_dbcc(const VDBManager *mgr,
    char const name[], node_t const nodes[], char const names[])
{
    return verify_mgr_database(mgr, name, nodes, names);
}

typedef struct vdb_validate_params vdb_validate_params;
struct vdb_validate_params
{
    const KDirectory *wd;
    const KDBManager *kmgr;
    const VDBManager *vmgr;

    bool md5_chk;
    bool md5_chk_explicit;
    bool blob_crc;
    bool index_chk;
};

static
rc_t dbcc ( const vdb_validate_params *pb, const char *path, bool is_file )
{
    char *names;
    KPathType pathType;
    node_t *nodes = NULL;
    const char *obj_type, *obj_name;

    rc_t rc = init_dbcc ( pb -> wd, path, is_file, & nodes, & names, & pathType );
    if ( rc == 0 )
    {
        /* construct mode */
        uint32_t mode = ( pb -> md5_chk ? 1 : 0 )
                      | ( pb -> blob_crc ? 2 : 0 )
                      | ( pb -> index_chk ? 4 : 0 )
                      ;

        INSDC_SRA_platform_id platform = SRA_PLATFORM_UNDEFINED;

        /* check as kdb object */
        rc = kdbcc ( pb -> kmgr, path, mode, & pathType, is_file, nodes, names, platform );
        if ( rc == 0 && pathType == kptDatabase) {
            rc = sra_dbcc ( pb -> vmgr, path, nodes, names );
        }
    }

    obj_type = ( pathType == kptDatabase ) ? "Database" : "Table";
    obj_name = strrchr ( path, '/' );
    if ( obj_name ++ == NULL )
        obj_name = path;

    if ( rc != 0 )
    {
        PLOGERR ( klogErr, ( klogErr, rc,
                             "$(objType) '$(objName)' check failed"
                             , "objType=%s,objName=%s"
                             , obj_type, obj_name ) );
    }
    else
    {
        PLOGMSG ( klogInfo, ( klogInfo,
                              "$(objType) '$(objName)' is consistent"
                             , "objType=%s,objName=%s"
                             , obj_type, obj_name ) );
    }

    free ( nodes );
    return rc;
}

static
const char *generate_relpath ( const vdb_validate_params *pb, const KDirectory *dir,
    char *buffer, size_t bsize, const char *path )
{
    if ( dir != pb -> wd )
    {
        char full [ 4096 ];
        rc_t rc = KDirectoryResolvePath ( dir, true, full, sizeof full, path );
        if ( rc == 0 )
        {
            rc = KDirectoryResolvePath ( pb -> wd, false, buffer, bsize, full );
            if ( rc == 0 )
                path = buffer;
        }
    }

    return path;
}

static
rc_t vdb_validate_file ( const vdb_validate_params *pb, const KDirectory *dir, const char *path )
{
    char buffer [ 4096 ];
    const char *relpath = generate_relpath ( pb, dir, buffer, sizeof buffer, path );

    const KFile *f;
    rc_t rc = KDirectoryOpenFileRead ( dir, & f, path );
    if ( rc != 0 )
        PLOGERR ( klogErr, ( klogErr, rc, "File '$(fname)' could not be opened", "fname=%s", relpath ) );
    else
    {
        bool is_sra = false;
        bool encrypted = false;

        size_t num_read;
        char buffer [ 4096 ];
        rc = KFileReadAll ( f, 0, buffer, sizeof buffer, & num_read );
        if ( rc != 0 )
            PLOGERR ( klogErr, ( klogErr, rc, "File '$(fname)' could not be read", "fname=%s", relpath ) );
        else
        {
            /* special kludge to prevent code from looking too far at header */
            size_t hdr_bytes = num_read;
            if ( num_read > 8 )
                hdr_bytes = 8;

            /* check for encrypted file */
            if ( KFileIsEnc ( buffer, hdr_bytes ) == 0 )
            {
                encrypted = true;

                PLOGMSG ( klogInfo, ( klogInfo, "Validating encrypted file '$(fname)'...", "fname=%s", relpath ) );
                rc = KEncFileValidate ( f );
                if ( rc != 0 )
                {
                    PLOGERR ( klogErr, ( klogErr, rc, "Encrypted file '$(fname)' could not be validated", "fname=%s", relpath ) );
                }
                else
                {
                    PLOGMSG ( klogInfo, ( klogInfo, "Encrypted file '$(fname)' appears valid", "fname=%s", relpath ) );

                    rc = EncFileReadAll ( relpath, buffer, sizeof buffer, & num_read );
                    if ( rc == 0 && KFileIsSRA ( buffer, num_read ) == 0 )
                        is_sra = true;
                }
            }
            else if ( KFileIsSRA ( buffer, num_read ) == 0 )
            {
                is_sra = true;
            }
        }

        KFileRelease ( f );

        if ( rc == 0 && is_sra )
            rc = dbcc ( pb, relpath, true );
    }

    return rc;
}

static
rc_t vdb_validate_database ( const vdb_validate_params *pb, const KDirectory *dir, const char *path )
{
    char buffer [ 4096 ];
    const char *relpath = generate_relpath ( pb, dir, buffer, sizeof buffer, path );
    return dbcc ( pb, relpath, false );
}

static
rc_t vdb_validate_table ( const vdb_validate_params *pb, const KDirectory *dir, const char *path )
{
    char buffer [ 4096 ];
    const char *relpath = generate_relpath ( pb, dir, buffer, sizeof buffer, path );
    return dbcc ( pb, relpath, false );
}

static
KPathType vdb_subdir_type ( const vdb_validate_params *pb, const KDirectory *dir, const char *name )
{
    char full [ 4096 ];
    rc_t rc = KDirectoryResolvePath ( dir, true, full, sizeof full, name );
    if ( rc == 0 )
    {
        switch ( KDBManagerPathType ( pb -> kmgr, full ) )
        {
        case kptDatabase:
            return kptDatabase;
        case kptTable:
            return kptTable;
        }
    }
    return kptDir;
}

static
rc_t CC vdb_validate_dir ( const KDirectory *dir, uint32_t type, const char *name, void *data )
{
    switch ( type & ~ kptAlias )
    {
    case kptFile:
        return vdb_validate_file ( data, dir, name );
    case kptDir:
        switch ( vdb_subdir_type ( data, dir, name ) )
        {
        case kptDatabase:
            return vdb_validate_database ( data, dir, name );
        case kptTable:
            return vdb_validate_table ( data, dir, name );
        default:
            return KDirectoryVisit ( dir, false, vdb_validate_dir, data, name );
        }
    }

    return 0;
}

static
rc_t vdb_validate ( const vdb_validate_params *pb, const char *path )
{
    rc_t rc;

    /* what type of thing is this path? */
    KPathType pt = KDirectoryPathType ( pb -> wd, path );
    switch ( pt & ~ kptAlias )
    {
    case kptNotFound:
        rc = resolver(NULL, path);
        break;
    case kptBadPath:
        rc = RC ( rcExe, rcPath, rcValidating, rcPath, rcInvalid );
        break;
    case kptFile:
        return vdb_validate_file ( pb, pb -> wd, path );
    case kptDir:
        switch ( KDBManagerPathType ( pb -> kmgr, path ) )
        {
        case kptDatabase:
            return vdb_validate_database ( pb, pb -> wd, path );
        case kptTable:
        case kptPrereleaseTbl:
            return vdb_validate_table ( pb, pb -> wd, path );
        case kptIndex:
        case kptColumn:
            rc = RC ( rcExe, rcPath, rcValidating, rcType, rcUnsupported );
            break;
        default:
            return KDirectoryVisit ( pb -> wd, false, vdb_validate_dir, ( void* ) pb, path );
        }
        break;

    default:
        return 0;
    }

    PLOGMSG ( klogWarn, ( klogWarn, "Path '$(fname)' could not be validated", "fname=%s", path ) );

    return 0;
}

static char const* const defaultLogLevel = 
#if _DEBUGGING
"debug5";
#else
"info";
#endif

/******************************************************************************
 * Usage
 ******************************************************************************/
const char UsageDefaultName[] = "vdb-validate";

rc_t CC UsageSummary(const char *prog_name)
{
    return KOutMsg ( "Usage: %s [options] path [ path... ]\n"
                     "\n"
                     , prog_name );
}

static char const *help_text[] = 
{
    "Check components md5s if present, "
            "fail unless other checks are requested (default: yes)", NULL,
    "Check blobs CRC32 (default: no)", NULL,
    "Check 'skey' index (default: no)", NULL,
    "Continue checking object for all possible errors (default: no)", NULL,
    "Check data referential integrity for databases (default: no)", NULL,
    "Check index-only with blobs CRC32 (default: no)", NULL
};

static OptDef options [] = 
{
    { "md5"       , "5", NULL, &help_text[0], 1, false, false }
  , { "blob-crc"  , "b", NULL, &help_text[2], 1, false, false }
#if CHECK_INDEX
  , { "index"     , "i", NULL, &help_text[4], 1, false, false }
#endif
  , { "exhaustive", "x", NULL, &help_text[6], 1, false, false }
  , { "referential-integrity", "d", NULL, &help_text[8], 1, false, false }

    /* should be the last ones here: not printed by --help */
  , { "dri"       , NULL, NULL, &help_text[8], 1, false, false }
  , { "index-only",NULL, NULL, &help_text[10], 1, false, false }
};

#define NUM_SILENT_TRAILING_OPTIONS 2

static const char *option_params [] =
{
    NULL
  , NULL
#if CHECK_INDEX
  , NULL
#endif
  , NULL
  , NULL
  , NULL
  , NULL
};

rc_t CC Usage ( const Args * args )
{
    uint32_t i;
    const char *progname, *fullpath;
    rc_t rc = ArgsProgram ( args, & fullpath, & progname );
    if ( rc != 0 )
        progname = fullpath = UsageDefaultName;
    
    UsageSummary ( progname );
    
    KOutMsg ( "  Examine directories, files and VDB objects,\n"
              "  reporting any problems that can be detected.\n"
              "\n"
              "Options:\n"
        );

#define NUM_LISTABLE_OPTIONS \
    ( sizeof options / sizeof options [ 0 ] - NUM_SILENT_TRAILING_OPTIONS )

    for ( i = 0; i < NUM_LISTABLE_OPTIONS; ++i )
    {
        HelpOptionLine ( options [ i ] . aliases, options [ i ] . name,
            option_params [ i ], options [ i ] . help );
    }
    
    HelpOptionsStandard ();
    
    HelpVersion ( fullpath, KAppVersion () );
    
    return 0;
}

uint32_t CC KAppVersion(void) { return 0; }

static
rc_t parse_args ( vdb_validate_params *pb, Args *args )
{
    rc_t rc;
    uint32_t cnt;

    pb -> md5_chk = true;

    rc = ArgsOptionCount ( args, "md5", & cnt );
    if ( rc != 0 )
        return rc;
    pb -> md5_chk_explicit = md5_required = cnt != 0;

    rc = ArgsOptionCount ( args, "blob-crc", & cnt );
    if ( rc != 0 )
        return rc;
    pb -> blob_crc = cnt != 0;

    rc = ArgsOptionCount ( args, "exhaustive", & cnt );
    if ( rc != 0 )
        return rc;
    exhaustive = cnt != 0;

    rc = ArgsOptionCount ( args, "referential-integrity", & cnt );
    if ( rc != 0 )
        return rc;
    ref_int_check = cnt != 0;

    rc = ArgsOptionCount ( args, "dri", & cnt );
    if ( rc != 0 )
        return rc;
    ref_int_check = cnt != 0;

#if CHECK_INDEX
    rc = ArgsOptionCount ( args, "index", & cnt );
    if ( rc != 0 )
        return rc;
    pb -> index_chk = cnt != 0;
#endif

    rc = ArgsOptionCount ( args, "index-only", & cnt );
    if ( rc != 0 )
        return rc;
    if ( cnt != 0 )
        s_IndexOnly = pb -> blob_crc = true;

    if ( pb -> blob_crc || pb -> index_chk )
        pb -> md5_chk = pb -> md5_chk_explicit;

    return 0;
}

static
void vdb_validate_params_whack ( vdb_validate_params *pb )
{
    VDBManagerRelease ( pb -> vmgr );
    KDBManagerRelease ( pb -> kmgr );
    KDirectoryRelease ( pb -> wd );
    memset ( pb, 0, sizeof * pb );
}

static
rc_t vdb_validate_params_init ( vdb_validate_params *pb )
{
    rc_t rc;
    KDirectory *wd;

    memset ( pb, 0, sizeof * pb );

    rc = KDirectoryNativeDir ( & wd );
    if ( rc == 0 )
    {
        pb -> wd = wd;
        rc = VDBManagerMakeRead ( & pb -> vmgr, wd );
        if ( rc == 0 )
        {
            rc = VDBManagerOpenKDBManagerRead ( pb -> vmgr, & pb -> kmgr );
            if ( rc == 0 )
                return 0;
        }
    }

    vdb_validate_params_whack ( pb );
    return rc;
}

rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;
    rc_t rc = ArgsMakeAndHandle ( & args, argc, argv, 1,
        options, sizeof options / sizeof options [ 0 ] );

    if ( rc != 0 )
        LOGERR ( klogErr, rc, "Failed to parse command line" );
    else
    {
        uint32_t pcount;
        rc = ArgsParamCount ( args, & pcount );
        if ( rc != 0 )
            LOGERR ( klogErr, rc, "Failed to count command line parameters" );
        else if ( argc <= 1 )
        {
            rc = RC ( rcExe, rcPath, rcValidating, rcParam, rcInsufficient );
            MiniUsage ( args );
        }
        else if ( pcount == 0 )
        {
            rc = RC ( rcExe, rcPath, rcValidating, rcParam, rcInsufficient );
            LOGERR ( klogErr, rc, "No paths to validate" );
            MiniUsage ( args );
        }
        else
        {
            vdb_validate_params pb;
            rc = vdb_validate_params_init ( & pb );
            if ( rc != 0 )
                LOGERR ( klogErr, rc, "Failed to initialize internal managers" );
            else
            {
                rc = parse_args ( & pb, args );
                if ( rc != 0 )
                    LOGERR ( klogErr, rc, "Failed to extract command line options" );
                else
                {
                    rc = KLogLevelSet ( klogInfo );
                    if ( rc != 0 )
                        LOGERR ( klogErr, rc, "Failed to set log level" );
                    else
                    {
                        uint32_t i;
                        for ( i = 0; i < pcount; ++ i )
                        {
                            rc_t rc2;
                            const char *path;
                            rc = ArgsParamValue ( args, i, & path );
                            if ( rc != 0 )
                            {
                                LOGERR ( klogErr, rc, "Failed to extract command line options" );
                                break;
                            }

                            rc2 = vdb_validate ( & pb, path );
                            if ( rc == 0 )
                                rc = rc2;
                        }
                    }
                }

                vdb_validate_params_whack ( & pb );
            }
        }

        ArgsWhack ( args );
    }

    return rc;
}
