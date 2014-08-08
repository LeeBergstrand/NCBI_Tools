/* THIS IS AN AUTO-GENERATED FILE - DO NOT EDIT */

#include "kqsh-kdb.h"
#include "kqsh-priv.h"
#include <klib/rc.h>
#include <assert.h>

static const char *kdb_msgs [] =
{
    "KDBManagerMakeRead",
    "KDBManagerMakeUpdate",
    "KDBManagerAddRef",
    "KDBManagerRelease",
    "KDBManagerVersion",
    "KDBManagerVExists",
    "KDBManagerVWritable",
    "KDBManagerRunPeriodicTasks",
    "KDBManagerPathTypeVP",
    "KDBManagerVPathType",
    "KDatabaseAddRef",
    "KDatabaseRelease",
    "KDBManagerVOpenDBRead",
    "KDatabaseVOpenDBRead",
    "KDatabaseLocked",
    "KDatabaseVExists",
    "KDatabaseIsAlias",
    "KDatabaseVWritable",
    "KDatabaseOpenManagerRead",
    "KDatabaseOpenParentRead",
    "KTableAddRef",
    "KTableRelease",
    "KDBManagerVOpenTableRead",
    "KDatabaseVOpenTableRead",
    "KTableLocked",
    "KTableVExists",
    "KTableIsAlias",
    "KTableVWritable",
    "KTableOpenManagerRead",
    "KTableOpenParentRead",
    "KColumnAddRef",
    "KColumnRelease",
    "KDBManagerVOpenColumnRead",
    "KTableVOpenColumnRead",
    "KColumnLocked",
    "KColumnVersion",
    "KColumnByteOrder",
    "KColumnIdRange",
    "KColumnOpenManagerRead",
    "KColumnOpenParentRead",
    "KColumnBlobAddRef",
    "KColumnBlobRelease",
    "KColumnOpenBlobRead",
    "KColumnBlobRead",
    "KColumnBlobValidate",
    "KColumnBlobIdRange",
    "KIndexAddRef",
    "KIndexRelease",
    "KDatabaseVOpenIndexRead",
    "KTableVOpenIndexRead",
    "KIndexLocked",
    "KIndexVersion",
    "KIndexType",
    "KIndexConsistencyCheck",
    "KIndexFindText",
    "KIndexFindAllText",
    "KIndexProjectText",
    "KIndexProjectAllText",
    "KIndexFindU64",
    "KMetadataAddRef",
    "KMetadataRelease",
    "KDatabaseOpenMetadataRead",
    "KTableOpenMetadataRead",
    "KColumnOpenMetadataRead",
    "KMetadataVersion",
    "KMetadataByteOrder",
    "KMetadataRevision",
    "KMetadataMaxRevision",
    "KMetadataOpenRevision",
    "KMetadataGetSequence",
    "KMDataNodeAddRef",
    "KMDataNodeRelease",
    "KMetadataVOpenNodeRead",
    "KMDataNodeVOpenNodeRead",
    "KMDataNodeByteOrder",
    "KMDataNodeRead",
    "KMDataNodeReadB8",
    "KMDataNodeReadB16",
    "KMDataNodeReadB32",
    "KMDataNodeReadB64",
    "KMDataNodeReadB128",
    "KMDataNodeReadAsI16",
    "KMDataNodeReadAsU16",
    "KMDataNodeReadAsI32",
    "KMDataNodeReadAsU32",
    "KMDataNodeReadAsI64",
    "KMDataNodeReadAsU64",
    "KMDataNodeReadAsF64",
    "KMDataNodeReadCString",
    "KMDataNodeReadAttr",
    "KMDataNodeReadAttrAsI16",
    "KMDataNodeReadAttrAsU16",
    "KMDataNodeReadAttrAsI32",
    "KMDataNodeReadAttrAsU32",
    "KMDataNodeReadAttrAsI64",
    "KMDataNodeReadAttrAsU64",
    "KMDataNodeReadAttrAsF64",
    "KDatabaseListDB",
    "KDatabaseListTbl",
    "KDatabaseListIdx",
    "KTableListCol",
    "KTableListIdx",
    "KMDataNodeListAttr",
    "KMDataNodeListChildren",
    NULL
};

static union
{
    fptr_t slots [ sizeof kdb_msgs / sizeof kdb_msgs [ 0 ] - 2 ];

    struct
    {
        rc_t ( CC * mgr_make ) ( KDBManager**, struct KDirectory* );
        rc_t ( CC * mgr_add_ref ) ( const KDBManager* );
        rc_t ( CC * mgr_release ) ( const KDBManager* );
        rc_t ( CC * mgr_version ) ( const KDBManager*, uint32_t* );
        bool ( CC * mgr_v_exists ) ( const KDBManager*, uint32_t, const char*, va_list );
        rc_t ( CC * mgr_v_writable ) ( const KDBManager*, const char*, va_list );
        rc_t ( CC * mgr_run_periodic_tasks ) ( const KDBManager* );
        int ( CC * mgr_path_type_vp ) ( const KDBManager*, const struct VPath* );
        int ( CC * mgr_v_path_type ) ( const KDBManager*, const char*, va_list );
        rc_t ( CC * db_add_ref ) ( const KDatabase* );
        rc_t ( CC * db_release ) ( const KDatabase* );
        rc_t ( CC * mgr_v_open_db_read ) ( struct KDBManager const*, const KDatabase**, const char*, va_list );
        rc_t ( CC * db_v_open_db_read ) ( const KDatabase*, const KDatabase**, const char*, va_list );
        bool ( CC * db_locked ) ( const KDatabase* );
        bool ( CC * db_v_exists ) ( const KDatabase*, uint32_t, const char*, va_list );
        bool ( CC * db_is_alias ) ( const KDatabase*, uint32_t, char*, size_t, const char* );
        rc_t ( CC * db_v_writable ) ( const KDatabase*, uint32_t, const char*, va_list );
        rc_t ( CC * db_open_manager_read ) ( const KDatabase*, struct KDBManager const** );
        rc_t ( CC * db_open_parent_read ) ( const KDatabase*, const KDatabase** );
        rc_t ( CC * tbl_add_ref ) ( const KTable* );
        rc_t ( CC * tbl_release ) ( const KTable* );
        rc_t ( CC * mgr_v_open_table_read ) ( struct KDBManager const*, const KTable**, const char*, va_list );
        rc_t ( CC * db_v_open_table_read ) ( struct KDatabase const*, const KTable**, const char*, va_list );
        bool ( CC * tbl_locked ) ( const KTable* );
        bool ( CC * tbl_v_exists ) ( const KTable*, uint32_t, const char*, va_list );
        bool ( CC * tbl_is_alias ) ( const KTable*, uint32_t, char*, size_t, const char* );
        rc_t ( CC * tbl_v_writable ) ( const KTable*, uint32_t, const char*, va_list );
        rc_t ( CC * tbl_open_manager_read ) ( const KTable*, struct KDBManager const** );
        rc_t ( CC * tbl_open_parent_read ) ( const KTable*, struct KDatabase const** );
        rc_t ( CC * col_add_ref ) ( const KColumn* );
        rc_t ( CC * col_release ) ( const KColumn* );
        rc_t ( CC * mgr_v_open_column_read ) ( struct KDBManager const*, const KColumn**, const char*, va_list );
        rc_t ( CC * tbl_v_open_column_read ) ( struct KTable const*, const KColumn**, const char*, va_list );
        bool ( CC * col_locked ) ( const KColumn* );
        rc_t ( CC * col_version ) ( const KColumn*, uint32_t* );
        rc_t ( CC * col_byte_order ) ( const KColumn*, bool* );
        rc_t ( CC * col_id_range ) ( const KColumn*, int64_t*, uint64_t* );
        rc_t ( CC * col_open_manager_read ) ( const KColumn*, struct KDBManager const** );
        rc_t ( CC * col_open_parent_read ) ( const KColumn*, struct KTable const** );
        rc_t ( CC * col_blob_add_ref ) ( const KColumnBlob* );
        rc_t ( CC * col_blob_release ) ( const KColumnBlob* );
        rc_t ( CC * col_open_blob_read ) ( const KColumn*, const KColumnBlob**, int64_t );
        rc_t ( CC * col_blob_read ) ( const KColumnBlob*, size_t, void*, size_t, size_t*, size_t* );
        rc_t ( CC * col_blob_validate ) ( const KColumnBlob* );
        rc_t ( CC * col_blob_id_range ) ( const KColumnBlob*, int64_t*, uint32_t* );
        rc_t ( CC * idx_add_ref ) ( const KIndex* );
        rc_t ( CC * idx_release ) ( const KIndex* );
        rc_t ( CC * db_v_open_index_read ) ( struct KDatabase const*, const KIndex**, const char*, va_list );
        rc_t ( CC * tbl_v_open_index_read ) ( struct KTable const*, const KIndex**, const char*, va_list );
        bool ( CC * idx_locked ) ( const KIndex* );
        rc_t ( CC * idx_version ) ( const KIndex*, uint32_t* );
        rc_t ( CC * idx_type ) ( const KIndex*, KIdxType* );
        rc_t ( CC * idx_consistency_check ) ( const KIndex*, uint32_t, int64_t*, uint64_t*, uint64_t*, uint64_t*, uint64_t* );
        rc_t ( CC * idx_find_text ) ( const KIndex*, const char*, int64_t*, uint64_t*, int ( CC * ) ( const void *item, struct PBSTNode const *n, void *data ), void* );
        rc_t ( CC * idx_find_all_text ) ( const KIndex*, const char*, rc_t ( CC * ) ( int64_t start_id, uint64_t id_count, void *data ), void* );
        rc_t ( CC * idx_project_text ) ( const KIndex*, int64_t, int64_t*, uint64_t*, char*, size_t, size_t* );
        rc_t ( CC * idx_project_all_text ) ( const KIndex*, int64_t, rc_t ( CC * ) ( int64_t start_id, uint64_t id_count, const char *key, void *data ), void* );
        rc_t ( CC * idx_find_u64 ) ( const KIndex*, uint64_t, uint64_t*, uint64_t*, int64_t*, uint64_t* );
        rc_t ( CC * meta_add_ref ) ( const KMetadata* );
        rc_t ( CC * meta_release ) ( const KMetadata* );
        rc_t ( CC * db_open_metadata_read ) ( struct KDatabase const*, const KMetadata** );
        rc_t ( CC * tbl_open_metadata_read ) ( struct KTable const*, const KMetadata** );
        rc_t ( CC * col_open_metadata_read ) ( struct KColumn const*, const KMetadata** );
        rc_t ( CC * meta_version ) ( const KMetadata*, uint32_t* );
        rc_t ( CC * meta_byte_order ) ( const KMetadata*, bool* );
        rc_t ( CC * meta_revision ) ( const KMetadata*, uint32_t* );
        rc_t ( CC * meta_max_revision ) ( const KMetadata*, uint32_t* );
        rc_t ( CC * meta_open_revision ) ( const KMetadata*, const KMetadata**, uint32_t );
        rc_t ( CC * meta_get_sequence ) ( const KMetadata*, const char*, int64_t* );
        rc_t ( CC * node_add_ref ) ( const KMDataNode* );
        rc_t ( CC * node_release ) ( const KMDataNode* );
        rc_t ( CC * meta_v_open_node_read ) ( const KMetadata*, const KMDataNode**, const char*, va_list );
        rc_t ( CC * node_v_open_node_read ) ( const KMDataNode*, const KMDataNode**, const char*, va_list );
        rc_t ( CC * node_byte_order ) ( const KMDataNode*, bool* );
        rc_t ( CC * node_read ) ( const KMDataNode*, size_t, void*, size_t, size_t*, size_t* );
        rc_t ( CC * node_read_b8 ) ( const KMDataNode*, void* );
        rc_t ( CC * node_read_b16 ) ( const KMDataNode*, void* );
        rc_t ( CC * node_read_b32 ) ( const KMDataNode*, void* );
        rc_t ( CC * node_read_b64 ) ( const KMDataNode*, void* );
        rc_t ( CC * node_read_b128 ) ( const KMDataNode*, void* );
        rc_t ( CC * node_read_as_i16 ) ( const KMDataNode*, int16_t* );
        rc_t ( CC * node_read_as_u16 ) ( const KMDataNode*, uint16_t* );
        rc_t ( CC * node_read_as_i32 ) ( const KMDataNode*, int32_t* );
        rc_t ( CC * node_read_as_u32 ) ( const KMDataNode*, uint32_t* );
        rc_t ( CC * node_read_as_i64 ) ( const KMDataNode*, int64_t* );
        rc_t ( CC * node_read_as_u64 ) ( const KMDataNode*, uint64_t* );
        rc_t ( CC * node_read_as_f64 ) ( const KMDataNode*, double* );
        rc_t ( CC * node_read_c_string ) ( const KMDataNode*, char*, size_t, size_t* );
        rc_t ( CC * node_read_attr ) ( const KMDataNode*, const char*, char*, size_t, size_t* );
        rc_t ( CC * node_read_attr_as_i16 ) ( const KMDataNode*, const char*, int16_t* );
        rc_t ( CC * node_read_attr_as_u16 ) ( const KMDataNode*, const char*, uint16_t* );
        rc_t ( CC * node_read_attr_as_i32 ) ( const KMDataNode*, const char*, int32_t* );
        rc_t ( CC * node_read_attr_as_u32 ) ( const KMDataNode*, const char*, uint32_t* );
        rc_t ( CC * node_read_attr_as_i64 ) ( const KMDataNode*, const char*, int64_t* );
        rc_t ( CC * node_read_attr_as_u64 ) ( const KMDataNode*, const char*, uint64_t* );
        rc_t ( CC * node_read_attr_as_f64 ) ( const KMDataNode*, const char*, double* );
        rc_t ( CC * db_list_db ) ( struct KDatabase const*, struct KNamelist** );
        rc_t ( CC * db_list_tbl ) ( struct KDatabase const*, struct KNamelist** );
        rc_t ( CC * db_list_idx ) ( struct KDatabase const*, struct KNamelist** );
        rc_t ( CC * tbl_list_col ) ( struct KTable const*, struct KNamelist** );
        rc_t ( CC * tbl_list_idx ) ( struct KTable const*, struct KNamelist** );
        rc_t ( CC * node_list_attr ) ( struct KMDataNode const*, struct KNamelist** );
        rc_t ( CC * node_list_children ) ( struct KMDataNode const*, struct KNamelist** );
    } f;

} kdb_cvt;

static const char *kdb_wmsgs [] =
{
    "KDBManagerVLock",
    "KDBManagerVUnlock",
    "KDBManagerVDrop",
    "KDBManagerVCreateDB",
    "KDatabaseVCreateDB",
    "KDBManagerVOpenDBUpdate",
    "KDatabaseVOpenDBUpdate",
    "KDatabaseVLock",
    "KDatabaseVUnlock",
    "KDatabaseRenameDB",
    "KDatabaseRenameTable",
    "KDatabaseRenameIndex",
    "KDatabaseAliasDB",
    "KDatabaseAliasTable",
    "KDatabaseAliasIndex",
    "KDatabaseVDropDB",
    "KDatabaseVDropTable",
    "KDatabaseVDropIndex",
    "KDatabaseOpenManagerUpdate",
    "KDatabaseOpenParentUpdate",
    "KDBManagerVCreateTable",
    "KDatabaseVCreateTable",
    "KDBManagerVOpenTableUpdate",
    "KDatabaseVOpenTableUpdate",
    "KTableVLock",
    "KTableVUnlock",
    "KTableRenameColumn",
    "KTableRenameIndex",
    "KTableAliasColumn",
    "KTableAliasIndex",
    "KTableVDropColumn",
    "KTableVDropIndex",
    "KTableReindex",
    "KTableOpenManagerUpdate",
    "KTableOpenParentUpdate",
    "KDBManagerVCreateColumn",
    "KTableVCreateColumn",
    "KDBManagerVOpenColumnUpdate",
    "KTableVOpenColumnUpdate",
    "KColumnReindex",
    "KColumnCommitFreq",
    "KColumnSetCommitFreq",
    "KColumnOpenManagerUpdate",
    "KColumnOpenParentUpdate",
    "KColumnCreateBlob",
    "KColumnOpenBlobUpdate",
    "KColumnBlobAppend",
    "KColumnBlobAssignRange",
    "KColumnBlobCommit",
    "KDatabaseVCreateIndex",
    "KTableVCreateIndex",
    "KDatabaseVOpenIndexUpdate",
    "KTableVOpenIndexUpdate",
    "KIndexCommit",
    "KIndexInsertText",
    "KIndexDeleteText",
    "KIndexInsertU64",
    "KIndexDeleteU64",
    "KDatabaseOpenMetadataUpdate",
    "KTableOpenMetadataUpdate",
    "KColumnOpenMetadataUpdate",
    "KMetadataCommit",
    "KMetadataFreeze",
    "KMetadataSetSequence",
    "KMetadataNextSequence",
    "KMetadataVOpenNodeUpdate",
    "KMDataNodeVOpenNodeUpdate",
    "KMDataNodeWrite",
    "KMDataNodeAppend",
    "KMDataNodeWriteB8",
    "KMDataNodeWriteB16",
    "KMDataNodeWriteB32",
    "KMDataNodeWriteB64",
    "KMDataNodeWriteB128",
    "KMDataNodeWriteCString",
    "KMDataNodeWriteAttr",
    "KMDataNodeDropAll",
    "KMDataNodeDropAttr",
    "KMDataNodeVDropChild",
    "KMDataNodeRenameAttr",
    "KMDataNodeRenameChild",
    NULL
};

static union
{
    fptr_t slots [ sizeof kdb_wmsgs / sizeof kdb_wmsgs [ 0 ] - 1 ];

    struct
    {
        rc_t ( CC * mgr_v_lock ) ( KDBManager*, const char*, va_list );
        rc_t ( CC * mgr_v_unlock ) ( KDBManager*, const char*, va_list );
        rc_t ( CC * mgr_v_drop ) ( KDBManager*, uint32_t, const char*, va_list );
        rc_t ( CC * mgr_v_create_db ) ( struct KDBManager*, KDatabase**, KCreateMode, const char*, va_list );
        rc_t ( CC * db_v_create_db ) ( KDatabase*, KDatabase**, KCreateMode, const char*, va_list );
        rc_t ( CC * mgr_v_open_db_update ) ( struct KDBManager*, KDatabase**, const char*, va_list );
        rc_t ( CC * db_v_open_db_update ) ( KDatabase*, KDatabase**, const char*, va_list );
        rc_t ( CC * db_v_lock ) ( KDatabase*, uint32_t, const char*, va_list );
        rc_t ( CC * db_v_unlock ) ( KDatabase*, uint32_t, const char*, va_list );
        rc_t ( CC * db_rename_db ) ( KDatabase*, bool, const char*, const char* );
        rc_t ( CC * db_rename_table ) ( KDatabase*, bool, const char*, const char* );
        rc_t ( CC * db_rename_index ) ( KDatabase*, bool, const char*, const char* );
        rc_t ( CC * db_alias_db ) ( KDatabase*, const char*, const char* );
        rc_t ( CC * db_alias_table ) ( KDatabase*, const char*, const char* );
        rc_t ( CC * db_alias_index ) ( KDatabase*, const char*, const char* );
        rc_t ( CC * db_v_drop_db ) ( KDatabase*, const char*, va_list );
        rc_t ( CC * db_v_drop_table ) ( KDatabase*, const char*, va_list );
        rc_t ( CC * db_v_drop_index ) ( KDatabase*, const char*, va_list );
        rc_t ( CC * db_open_manager_update ) ( KDatabase*, struct KDBManager** );
        rc_t ( CC * db_open_parent_update ) ( KDatabase*, KDatabase** );
        rc_t ( CC * mgr_v_create_table ) ( struct KDBManager*, KTable**, KCreateMode, const char*, va_list );
        rc_t ( CC * db_v_create_table ) ( struct KDatabase*, KTable**, KCreateMode, const char*, va_list );
        rc_t ( CC * mgr_v_open_table_update ) ( struct KDBManager*, KTable**, const char*, va_list );
        rc_t ( CC * db_v_open_table_update ) ( struct KDatabase*, KTable**, const char*, va_list );
        rc_t ( CC * tbl_v_lock ) ( KTable*, uint32_t, const char*, va_list );
        rc_t ( CC * tbl_v_unlock ) ( KTable*, uint32_t, const char*, va_list );
        rc_t ( CC * tbl_rename_column ) ( KTable*, bool, const char*, const char* );
        rc_t ( CC * tbl_rename_index ) ( KTable*, bool, const char*, const char* );
        rc_t ( CC * tbl_alias_column ) ( KTable*, const char*, const char* );
        rc_t ( CC * tbl_alias_index ) ( KTable*, const char*, const char* );
        rc_t ( CC * tbl_v_drop_column ) ( KTable*, const char*, va_list );
        rc_t ( CC * tbl_v_drop_index ) ( KTable*, const char*, va_list );
        rc_t ( CC * tbl_reindex ) ( KTable* );
        rc_t ( CC * tbl_open_manager_update ) ( KTable*, struct KDBManager** );
        rc_t ( CC * tbl_open_parent_update ) ( KTable*, struct KDatabase** );
        rc_t ( CC * mgr_v_create_column ) ( struct KDBManager*, KColumn**, KCreateMode, KChecksum, size_t, const char*, va_list );
        rc_t ( CC * tbl_v_create_column ) ( struct KTable*, KColumn**, KCreateMode, KChecksum, size_t, const char*, va_list );
        rc_t ( CC * mgr_v_open_column_update ) ( struct KDBManager*, KColumn**, const char*, va_list );
        rc_t ( CC * tbl_v_open_column_update ) ( struct KTable*, KColumn**, const char*, va_list );
        rc_t ( CC * col_reindex ) ( KColumn* );
        rc_t ( CC * col_commit_freq ) ( KColumn*, uint32_t* );
        rc_t ( CC * col_set_commit_freq ) ( KColumn*, uint32_t );
        rc_t ( CC * col_open_manager_update ) ( KColumn*, struct KDBManager** );
        rc_t ( CC * col_open_parent_update ) ( KColumn*, struct KTable** );
        rc_t ( CC * col_create_blob ) ( KColumn*, KColumnBlob** );
        rc_t ( CC * col_open_blob_update ) ( KColumn*, KColumnBlob**, int64_t );
        rc_t ( CC * col_blob_append ) ( KColumnBlob*, const void*, size_t );
        rc_t ( CC * col_blob_assign_range ) ( KColumnBlob*, int64_t, uint32_t );
        rc_t ( CC * col_blob_commit ) ( KColumnBlob* );
        rc_t ( CC * db_v_create_index ) ( struct KDatabase*, KIndex**, KIdxType, KCreateMode, const char*, va_list );
        rc_t ( CC * tbl_v_create_index ) ( struct KTable*, KIndex**, KIdxType, KCreateMode, const char*, va_list );
        rc_t ( CC * db_v_open_index_update ) ( struct KDatabase*, KIndex**, const char*, va_list );
        rc_t ( CC * tbl_v_open_index_update ) ( struct KTable*, KIndex**, const char*, va_list );
        rc_t ( CC * idx_commit ) ( KIndex* );
        rc_t ( CC * idx_insert_text ) ( KIndex*, bool, const char*, int64_t );
        rc_t ( CC * idx_delete_text ) ( KIndex*, const char* );
        rc_t ( CC * idx_insert_u64 ) ( KIndex*, bool, uint64_t, uint64_t, int64_t, uint64_t );
        rc_t ( CC * idx_delete_u64 ) ( KIndex*, uint64_t );
        rc_t ( CC * db_open_metadata_update ) ( struct KDatabase*, KMetadata** );
        rc_t ( CC * tbl_open_metadata_update ) ( struct KTable*, KMetadata** );
        rc_t ( CC * col_open_metadata_update ) ( struct KColumn*, KMetadata** );
        rc_t ( CC * meta_commit ) ( KMetadata* );
        rc_t ( CC * meta_freeze ) ( KMetadata* );
        rc_t ( CC * meta_set_sequence ) ( KMetadata*, const char*, int64_t );
        rc_t ( CC * meta_next_sequence ) ( KMetadata*, const char*, int64_t* );
        rc_t ( CC * meta_v_open_node_update ) ( KMetadata*, KMDataNode**, const char*, va_list );
        rc_t ( CC * node_v_open_node_update ) ( KMDataNode*, KMDataNode**, const char*, va_list );
        rc_t ( CC * node_write ) ( KMDataNode*, const void*, size_t );
        rc_t ( CC * node_append ) ( KMDataNode*, const void*, size_t );
        rc_t ( CC * node_write_b8 ) ( KMDataNode*, const void* );
        rc_t ( CC * node_write_b16 ) ( KMDataNode*, const void* );
        rc_t ( CC * node_write_b32 ) ( KMDataNode*, const void* );
        rc_t ( CC * node_write_b64 ) ( KMDataNode*, const void* );
        rc_t ( CC * node_write_b128 ) ( KMDataNode*, const void* );
        rc_t ( CC * node_write_c_string ) ( KMDataNode*, const char* );
        rc_t ( CC * node_write_attr ) ( KMDataNode*, const char*, const char* );
        rc_t ( CC * node_drop_all ) ( KMDataNode* );
        rc_t ( CC * node_drop_attr ) ( KMDataNode*, const char* );
        rc_t ( CC * node_v_drop_child ) ( KMDataNode*, const char*, va_list );
        rc_t ( CC * node_rename_attr ) ( KMDataNode*, const char*, const char* );
        rc_t ( CC * node_rename_child ) ( KMDataNode*, const char*, const char* );
    } f;

} kdb_wvt;

kqsh_libdata kdb_data =
{
    NULL, NULL,
    kdb_msgs, kdb_cvt . slots,
    kdb_wmsgs, kdb_wvt . slots
};

rc_t _KDBManagerMake ( KDBManager **mgr, struct KDirectory *wd )
{
    if ( sizeof kdb_cvt . slots != sizeof kdb_cvt . f ||
         sizeof kdb_wvt . slots != sizeof kdb_wvt . f )
    {
        * mgr = NULL;
        return RC ( rcExe, rcMgr, rcConstructing, rcInterface, rcCorrupt );
    }

    assert ( kdb_cvt . f . mgr_make != NULL );
    return ( * kdb_cvt . f . mgr_make ) ( mgr, wd );
}

rc_t _KDBManagerAddRef ( const KDBManager *self )
{
    assert ( kdb_cvt . f . mgr_add_ref != NULL );
    return ( * kdb_cvt . f . mgr_add_ref ) ( self );
}

rc_t _KDBManagerRelease ( const KDBManager *self )
{
    assert ( kdb_cvt . f . mgr_release != NULL );
    return ( * kdb_cvt . f . mgr_release ) ( self );
}

rc_t _KDBManagerVersion ( const KDBManager *self, uint32_t *version )
{
    assert ( kdb_cvt . f . mgr_version != NULL );
    return ( * kdb_cvt . f . mgr_version ) ( self, version );
}

bool _KDBManagerExists ( const KDBManager *self, uint32_t type, const char *name, ... )
{
    bool ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_cvt . f . mgr_v_exists != NULL );
    ret = ( * kdb_cvt . f . mgr_v_exists ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

bool _KDBManagerVExists ( const KDBManager *self, uint32_t type, const char *name, va_list args )
{
    assert ( kdb_cvt . f . mgr_v_exists != NULL );
    return ( * kdb_cvt . f . mgr_v_exists ) ( self, type, name, args );
}

rc_t _KDBManagerWritable ( const KDBManager *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_cvt . f . mgr_v_writable != NULL );
    ret = ( * kdb_cvt . f . mgr_v_writable ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVWritable ( const KDBManager *self, const char *path, va_list args )
{
    assert ( kdb_cvt . f . mgr_v_writable != NULL );
    return ( * kdb_cvt . f . mgr_v_writable ) ( self, path, args );
}

rc_t _KDBManagerLock ( KDBManager *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . mgr_v_lock != NULL );
    ret = ( * kdb_wvt . f . mgr_v_lock ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVLock ( KDBManager *self, const char *path, va_list args )
{
    assert ( kdb_wvt . f . mgr_v_lock != NULL );
    return ( * kdb_wvt . f . mgr_v_lock ) ( self, path, args );
}

rc_t _KDBManagerUnlock ( KDBManager *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . mgr_v_unlock != NULL );
    ret = ( * kdb_wvt . f . mgr_v_unlock ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVUnlock ( KDBManager *self, const char *path, va_list args )
{
    assert ( kdb_wvt . f . mgr_v_unlock != NULL );
    return ( * kdb_wvt . f . mgr_v_unlock ) ( self, path, args );
}

rc_t _KDBManagerDrop ( KDBManager *self, uint32_t obj_type, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . mgr_v_drop != NULL );
    ret = ( * kdb_wvt . f . mgr_v_drop ) ( self, obj_type, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVDrop ( KDBManager *self, uint32_t obj_type, const char *path, va_list args )
{
    assert ( kdb_wvt . f . mgr_v_drop != NULL );
    return ( * kdb_wvt . f . mgr_v_drop ) ( self, obj_type, path, args );
}

rc_t _KDBManagerRunPeriodicTasks ( const KDBManager *self )
{
    assert ( kdb_cvt . f . mgr_run_periodic_tasks != NULL );
    return ( * kdb_cvt . f . mgr_run_periodic_tasks ) ( self );
}

int _KDBManagerPathTypeVP ( const KDBManager * self, const struct VPath * path )
{
    assert ( kdb_cvt . f . mgr_path_type_vp != NULL );
    return ( * kdb_cvt . f . mgr_path_type_vp ) ( self, path );
}

int _KDBManagerPathType ( const KDBManager * self, const char *path, ... )
{
    int ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_cvt . f . mgr_v_path_type != NULL );
    ret = ( * kdb_cvt . f . mgr_v_path_type ) ( self, path, args );
    va_end ( args );
    return ret;
}

int _KDBManagerVPathType ( const KDBManager * self, const char *path, va_list args )
{
    assert ( kdb_cvt . f . mgr_v_path_type != NULL );
    return ( * kdb_cvt . f . mgr_v_path_type ) ( self, path, args );
}

rc_t _KDatabaseAddRef ( const KDatabase *self )
{
    assert ( kdb_cvt . f . db_add_ref != NULL );
    return ( * kdb_cvt . f . db_add_ref ) ( self );
}

rc_t _KDatabaseRelease ( const KDatabase *self )
{
    assert ( kdb_cvt . f . db_release != NULL );
    return ( * kdb_cvt . f . db_release ) ( self );
}

rc_t _KDBManagerCreateDB ( struct KDBManager *self, KDatabase **db, KCreateMode cmode, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . mgr_v_create_db != NULL );
    ret = ( * kdb_wvt . f . mgr_v_create_db ) ( self, db, cmode, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseCreateDB ( KDatabase *self, KDatabase **db, KCreateMode cmode, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . db_v_create_db != NULL );
    ret = ( * kdb_wvt . f . db_v_create_db ) ( self, db, cmode, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVCreateDB ( struct KDBManager *self, KDatabase **db, KCreateMode cmode, const char *path, va_list args )
{
    assert ( kdb_wvt . f . mgr_v_create_db != NULL );
    return ( * kdb_wvt . f . mgr_v_create_db ) ( self, db, cmode, path, args );
}

rc_t _KDatabaseVCreateDB ( KDatabase *self, KDatabase **db, KCreateMode cmode, const char *name, va_list args )
{
    assert ( kdb_wvt . f . db_v_create_db != NULL );
    return ( * kdb_wvt . f . db_v_create_db ) ( self, db, cmode, name, args );
}

rc_t _KDBManagerOpenDBRead ( struct KDBManager const *self, const KDatabase **db, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_cvt . f . mgr_v_open_db_read != NULL );
    ret = ( * kdb_cvt . f . mgr_v_open_db_read ) ( self, db, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseOpenDBRead ( const KDatabase *self, const KDatabase **db, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_cvt . f . db_v_open_db_read != NULL );
    ret = ( * kdb_cvt . f . db_v_open_db_read ) ( self, db, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVOpenDBRead ( struct KDBManager const *self, const KDatabase **db, const char *path, va_list args )
{
    assert ( kdb_cvt . f . mgr_v_open_db_read != NULL );
    return ( * kdb_cvt . f . mgr_v_open_db_read ) ( self, db, path, args );
}

rc_t _KDatabaseVOpenDBRead ( const KDatabase *self, const KDatabase **db, const char *name, va_list args )
{
    assert ( kdb_cvt . f . db_v_open_db_read != NULL );
    return ( * kdb_cvt . f . db_v_open_db_read ) ( self, db, name, args );
}

rc_t _KDBManagerOpenDBUpdate ( struct KDBManager *self, KDatabase **db, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . mgr_v_open_db_update != NULL );
    ret = ( * kdb_wvt . f . mgr_v_open_db_update ) ( self, db, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseOpenDBUpdate ( KDatabase *self, KDatabase **db, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . db_v_open_db_update != NULL );
    ret = ( * kdb_wvt . f . db_v_open_db_update ) ( self, db, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVOpenDBUpdate ( struct KDBManager *self, KDatabase **db, const char *path, va_list args )
{
    assert ( kdb_wvt . f . mgr_v_open_db_update != NULL );
    return ( * kdb_wvt . f . mgr_v_open_db_update ) ( self, db, path, args );
}

rc_t _KDatabaseVOpenDBUpdate ( KDatabase *self, KDatabase **db, const char *name, va_list args )
{
    assert ( kdb_wvt . f . db_v_open_db_update != NULL );
    return ( * kdb_wvt . f . db_v_open_db_update ) ( self, db, name, args );
}

bool _KDatabaseLocked ( const KDatabase *self )
{
    assert ( kdb_cvt . f . db_locked != NULL );
    return ( * kdb_cvt . f . db_locked ) ( self );
}

bool _KDatabaseExists ( const KDatabase *self, uint32_t type, const char *name, ... )
{
    bool ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_cvt . f . db_v_exists != NULL );
    ret = ( * kdb_cvt . f . db_v_exists ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

bool _KDatabaseVExists ( const KDatabase *self, uint32_t type, const char *name, va_list args )
{
    assert ( kdb_cvt . f . db_v_exists != NULL );
    return ( * kdb_cvt . f . db_v_exists ) ( self, type, name, args );
}

bool _KDatabaseIsAlias ( const KDatabase *self, uint32_t type, char *resolved, size_t rsize, const char *name )
{
    assert ( kdb_cvt . f . db_is_alias != NULL );
    return ( * kdb_cvt . f . db_is_alias ) ( self, type, resolved, rsize, name );
}

rc_t _KDatabaseWritable ( const KDatabase *self, uint32_t type, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_cvt . f . db_v_writable != NULL );
    ret = ( * kdb_cvt . f . db_v_writable ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseVWritable ( const KDatabase *self, uint32_t type, const char *name, va_list args )
{
    assert ( kdb_cvt . f . db_v_writable != NULL );
    return ( * kdb_cvt . f . db_v_writable ) ( self, type, name, args );
}

rc_t _KDatabaseLock ( KDatabase *self, uint32_t type, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . db_v_lock != NULL );
    ret = ( * kdb_wvt . f . db_v_lock ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseVLock ( KDatabase *self, uint32_t type, const char *name, va_list args )
{
    assert ( kdb_wvt . f . db_v_lock != NULL );
    return ( * kdb_wvt . f . db_v_lock ) ( self, type, name, args );
}

rc_t _KDatabaseUnlock ( KDatabase *self, uint32_t type, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . db_v_unlock != NULL );
    ret = ( * kdb_wvt . f . db_v_unlock ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseVUnlock ( KDatabase *self, uint32_t type, const char *name, va_list args )
{
    assert ( kdb_wvt . f . db_v_unlock != NULL );
    return ( * kdb_wvt . f . db_v_unlock ) ( self, type, name, args );
}

rc_t _KDatabaseRenameDB ( KDatabase *self, bool force, const char *from, const char *to )
{
    assert ( kdb_wvt . f . db_rename_db != NULL );
    return ( * kdb_wvt . f . db_rename_db ) ( self, force, from, to );
}

rc_t _KDatabaseRenameTable ( KDatabase *self, bool force, const char *from, const char *to )
{
    assert ( kdb_wvt . f . db_rename_table != NULL );
    return ( * kdb_wvt . f . db_rename_table ) ( self, force, from, to );
}

rc_t _KDatabaseRenameIndex ( KDatabase *self, bool force, const char *from, const char *to )
{
    assert ( kdb_wvt . f . db_rename_index != NULL );
    return ( * kdb_wvt . f . db_rename_index ) ( self, force, from, to );
}

rc_t _KDatabaseAliasDB ( KDatabase *self, const char *obj, const char *alias )
{
    assert ( kdb_wvt . f . db_alias_db != NULL );
    return ( * kdb_wvt . f . db_alias_db ) ( self, obj, alias );
}

rc_t _KDatabaseAliasTable ( KDatabase *self, const char *obj, const char *alias )
{
    assert ( kdb_wvt . f . db_alias_table != NULL );
    return ( * kdb_wvt . f . db_alias_table ) ( self, obj, alias );
}

rc_t _KDatabaseAliasIndex ( KDatabase *self, const char *obj, const char *alias )
{
    assert ( kdb_wvt . f . db_alias_index != NULL );
    return ( * kdb_wvt . f . db_alias_index ) ( self, obj, alias );
}

rc_t _KDatabaseDropDB ( KDatabase *self, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . db_v_drop_db != NULL );
    ret = ( * kdb_wvt . f . db_v_drop_db ) ( self, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseDropTable ( KDatabase *self, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . db_v_drop_table != NULL );
    ret = ( * kdb_wvt . f . db_v_drop_table ) ( self, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseDropIndex ( KDatabase *self, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . db_v_drop_index != NULL );
    ret = ( * kdb_wvt . f . db_v_drop_index ) ( self, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseVDropDB ( KDatabase *self, const char *name, va_list args )
{
    assert ( kdb_wvt . f . db_v_drop_db != NULL );
    return ( * kdb_wvt . f . db_v_drop_db ) ( self, name, args );
}

rc_t _KDatabaseVDropTable ( KDatabase *self, const char *name, va_list args )
{
    assert ( kdb_wvt . f . db_v_drop_table != NULL );
    return ( * kdb_wvt . f . db_v_drop_table ) ( self, name, args );
}

rc_t _KDatabaseVDropIndex ( KDatabase *self, const char *name, va_list args )
{
    assert ( kdb_wvt . f . db_v_drop_index != NULL );
    return ( * kdb_wvt . f . db_v_drop_index ) ( self, name, args );
}

rc_t _KDatabaseOpenManagerRead ( const KDatabase *self, struct KDBManager const **mgr )
{
    assert ( kdb_cvt . f . db_open_manager_read != NULL );
    return ( * kdb_cvt . f . db_open_manager_read ) ( self, mgr );
}

rc_t _KDatabaseOpenManagerUpdate ( KDatabase *self, struct KDBManager **mgr )
{
    assert ( kdb_wvt . f . db_open_manager_update != NULL );
    return ( * kdb_wvt . f . db_open_manager_update ) ( self, mgr );
}

rc_t _KDatabaseOpenParentRead ( const KDatabase *self, const KDatabase **par )
{
    assert ( kdb_cvt . f . db_open_parent_read != NULL );
    return ( * kdb_cvt . f . db_open_parent_read ) ( self, par );
}

rc_t _KDatabaseOpenParentUpdate ( KDatabase *self, KDatabase **par )
{
    assert ( kdb_wvt . f . db_open_parent_update != NULL );
    return ( * kdb_wvt . f . db_open_parent_update ) ( self, par );
}

rc_t _KTableAddRef ( const KTable *self )
{
    assert ( kdb_cvt . f . tbl_add_ref != NULL );
    return ( * kdb_cvt . f . tbl_add_ref ) ( self );
}

rc_t _KTableRelease ( const KTable *self )
{
    assert ( kdb_cvt . f . tbl_release != NULL );
    return ( * kdb_cvt . f . tbl_release ) ( self );
}

rc_t _KDBManagerCreateTable ( struct KDBManager *self, KTable **tbl, KCreateMode cmode, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . mgr_v_create_table != NULL );
    ret = ( * kdb_wvt . f . mgr_v_create_table ) ( self, tbl, cmode, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseCreateTable ( struct KDatabase *self, KTable **tbl, KCreateMode cmode, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . db_v_create_table != NULL );
    ret = ( * kdb_wvt . f . db_v_create_table ) ( self, tbl, cmode, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVCreateTable ( struct KDBManager *self, KTable **tbl, KCreateMode cmode, const char *path, va_list args )
{
    assert ( kdb_wvt . f . mgr_v_create_table != NULL );
    return ( * kdb_wvt . f . mgr_v_create_table ) ( self, tbl, cmode, path, args );
}

rc_t _KDatabaseVCreateTable ( struct KDatabase *self, KTable **tbl, KCreateMode cmode, const char *name, va_list args )
{
    assert ( kdb_wvt . f . db_v_create_table != NULL );
    return ( * kdb_wvt . f . db_v_create_table ) ( self, tbl, cmode, name, args );
}

rc_t _KDBManagerOpenTableRead ( struct KDBManager const *self, const KTable **tbl, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_cvt . f . mgr_v_open_table_read != NULL );
    ret = ( * kdb_cvt . f . mgr_v_open_table_read ) ( self, tbl, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseOpenTableRead ( struct KDatabase const *self, const KTable **tbl, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_cvt . f . db_v_open_table_read != NULL );
    ret = ( * kdb_cvt . f . db_v_open_table_read ) ( self, tbl, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVOpenTableRead ( struct KDBManager const *self, const KTable **tbl, const char *path, va_list args )
{
    assert ( kdb_cvt . f . mgr_v_open_table_read != NULL );
    return ( * kdb_cvt . f . mgr_v_open_table_read ) ( self, tbl, path, args );
}

rc_t _KDatabaseVOpenTableRead ( struct KDatabase const *self, const KTable **tbl, const char *name, va_list args )
{
    assert ( kdb_cvt . f . db_v_open_table_read != NULL );
    return ( * kdb_cvt . f . db_v_open_table_read ) ( self, tbl, name, args );
}

rc_t _KDBManagerOpenTableUpdate ( struct KDBManager *self, KTable **tbl, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . mgr_v_open_table_update != NULL );
    ret = ( * kdb_wvt . f . mgr_v_open_table_update ) ( self, tbl, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseOpenTableUpdate ( struct KDatabase *self, KTable **tbl, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . db_v_open_table_update != NULL );
    ret = ( * kdb_wvt . f . db_v_open_table_update ) ( self, tbl, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVOpenTableUpdate ( struct KDBManager *self, KTable **tbl, const char *path, va_list args )
{
    assert ( kdb_wvt . f . mgr_v_open_table_update != NULL );
    return ( * kdb_wvt . f . mgr_v_open_table_update ) ( self, tbl, path, args );
}

rc_t _KDatabaseVOpenTableUpdate ( struct KDatabase *self, KTable **tbl, const char *name, va_list args )
{
    assert ( kdb_wvt . f . db_v_open_table_update != NULL );
    return ( * kdb_wvt . f . db_v_open_table_update ) ( self, tbl, name, args );
}

bool _KTableLocked ( const KTable *self )
{
    assert ( kdb_cvt . f . tbl_locked != NULL );
    return ( * kdb_cvt . f . tbl_locked ) ( self );
}

bool _KTableExists ( const KTable *self, uint32_t type, const char *name, ... )
{
    bool ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_cvt . f . tbl_v_exists != NULL );
    ret = ( * kdb_cvt . f . tbl_v_exists ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

bool _KTableVExists ( const KTable *self, uint32_t type, const char *name, va_list args )
{
    assert ( kdb_cvt . f . tbl_v_exists != NULL );
    return ( * kdb_cvt . f . tbl_v_exists ) ( self, type, name, args );
}

bool _KTableIsAlias ( const KTable *self, uint32_t type, char *resolved, size_t rsize, const char *name )
{
    assert ( kdb_cvt . f . tbl_is_alias != NULL );
    return ( * kdb_cvt . f . tbl_is_alias ) ( self, type, resolved, rsize, name );
}

rc_t _KTableWritable ( const KTable *self, uint32_t type, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_cvt . f . tbl_v_writable != NULL );
    ret = ( * kdb_cvt . f . tbl_v_writable ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _KTableVWritable ( const KTable *self, uint32_t type, const char *name, va_list args )
{
    assert ( kdb_cvt . f . tbl_v_writable != NULL );
    return ( * kdb_cvt . f . tbl_v_writable ) ( self, type, name, args );
}

rc_t _KTableLock ( KTable *self, uint32_t type, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . tbl_v_lock != NULL );
    ret = ( * kdb_wvt . f . tbl_v_lock ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _KTableVLock ( KTable *self, uint32_t type, const char *name, va_list args )
{
    assert ( kdb_wvt . f . tbl_v_lock != NULL );
    return ( * kdb_wvt . f . tbl_v_lock ) ( self, type, name, args );
}

rc_t _KTableUnlock ( KTable *self, uint32_t type, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . tbl_v_unlock != NULL );
    ret = ( * kdb_wvt . f . tbl_v_unlock ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _KTableVUnlock ( KTable *self, uint32_t type, const char *name, va_list args )
{
    assert ( kdb_wvt . f . tbl_v_unlock != NULL );
    return ( * kdb_wvt . f . tbl_v_unlock ) ( self, type, name, args );
}

rc_t _KTableRenameColumn ( KTable *self, bool force, const char *from, const char *to )
{
    assert ( kdb_wvt . f . tbl_rename_column != NULL );
    return ( * kdb_wvt . f . tbl_rename_column ) ( self, force, from, to );
}

rc_t _KTableRenameIndex ( KTable *self, bool force, const char *from, const char *to )
{
    assert ( kdb_wvt . f . tbl_rename_index != NULL );
    return ( * kdb_wvt . f . tbl_rename_index ) ( self, force, from, to );
}

rc_t _KTableAliasColumn ( KTable *self, const char *path, const char *alias )
{
    assert ( kdb_wvt . f . tbl_alias_column != NULL );
    return ( * kdb_wvt . f . tbl_alias_column ) ( self, path, alias );
}

rc_t _KTableAliasIndex ( KTable *self, const char *name, const char *alias )
{
    assert ( kdb_wvt . f . tbl_alias_index != NULL );
    return ( * kdb_wvt . f . tbl_alias_index ) ( self, name, alias );
}

rc_t _KTableDropColumn ( KTable *self, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . tbl_v_drop_column != NULL );
    ret = ( * kdb_wvt . f . tbl_v_drop_column ) ( self, name, args );
    va_end ( args );
    return ret;
}

rc_t _KTableDropIndex ( KTable *self, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . tbl_v_drop_index != NULL );
    ret = ( * kdb_wvt . f . tbl_v_drop_index ) ( self, name, args );
    va_end ( args );
    return ret;
}

rc_t _KTableVDropColumn ( KTable *self, const char *name, va_list args )
{
    assert ( kdb_wvt . f . tbl_v_drop_column != NULL );
    return ( * kdb_wvt . f . tbl_v_drop_column ) ( self, name, args );
}

rc_t _KTableVDropIndex ( KTable *self, const char *name, va_list args )
{
    assert ( kdb_wvt . f . tbl_v_drop_index != NULL );
    return ( * kdb_wvt . f . tbl_v_drop_index ) ( self, name, args );
}

rc_t _KTableReindex ( KTable *self )
{
    assert ( kdb_wvt . f . tbl_reindex != NULL );
    return ( * kdb_wvt . f . tbl_reindex ) ( self );
}

rc_t _KTableOpenManagerRead ( const KTable *self, struct KDBManager const **mgr )
{
    assert ( kdb_cvt . f . tbl_open_manager_read != NULL );
    return ( * kdb_cvt . f . tbl_open_manager_read ) ( self, mgr );
}

rc_t _KTableOpenManagerUpdate ( KTable *self, struct KDBManager **mgr )
{
    assert ( kdb_wvt . f . tbl_open_manager_update != NULL );
    return ( * kdb_wvt . f . tbl_open_manager_update ) ( self, mgr );
}

rc_t _KTableOpenParentRead ( const KTable *self, struct KDatabase const **db )
{
    assert ( kdb_cvt . f . tbl_open_parent_read != NULL );
    return ( * kdb_cvt . f . tbl_open_parent_read ) ( self, db );
}

rc_t _KTableOpenParentUpdate ( KTable *self, struct KDatabase **db )
{
    assert ( kdb_wvt . f . tbl_open_parent_update != NULL );
    return ( * kdb_wvt . f . tbl_open_parent_update ) ( self, db );
}

rc_t _KColumnAddRef ( const KColumn *self )
{
    assert ( kdb_cvt . f . col_add_ref != NULL );
    return ( * kdb_cvt . f . col_add_ref ) ( self );
}

rc_t _KColumnRelease ( const KColumn *self )
{
    assert ( kdb_cvt . f . col_release != NULL );
    return ( * kdb_cvt . f . col_release ) ( self );
}

rc_t _KDBManagerCreateColumn ( struct KDBManager *self, KColumn **col, KCreateMode cmode, KChecksum checksum, size_t pgsize, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . mgr_v_create_column != NULL );
    ret = ( * kdb_wvt . f . mgr_v_create_column ) ( self, col, cmode, checksum, pgsize, path, args );
    va_end ( args );
    return ret;
}

rc_t _KTableCreateColumn ( struct KTable *self, KColumn **col, KCreateMode cmode, KChecksum checksum, size_t pgsize, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . tbl_v_create_column != NULL );
    ret = ( * kdb_wvt . f . tbl_v_create_column ) ( self, col, cmode, checksum, pgsize, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVCreateColumn ( struct KDBManager *self, KColumn **col, KCreateMode cmode, KChecksum checksum, size_t pgsize, const char *path, va_list args )
{
    assert ( kdb_wvt . f . mgr_v_create_column != NULL );
    return ( * kdb_wvt . f . mgr_v_create_column ) ( self, col, cmode, checksum, pgsize, path, args );
}

rc_t _KTableVCreateColumn ( struct KTable *self, KColumn **col, KCreateMode cmode, KChecksum checksum, size_t pgsize, const char *path, va_list args )
{
    assert ( kdb_wvt . f . tbl_v_create_column != NULL );
    return ( * kdb_wvt . f . tbl_v_create_column ) ( self, col, cmode, checksum, pgsize, path, args );
}

rc_t _KDBManagerOpenColumnRead ( struct KDBManager const *self, const KColumn **col, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_cvt . f . mgr_v_open_column_read != NULL );
    ret = ( * kdb_cvt . f . mgr_v_open_column_read ) ( self, col, path, args );
    va_end ( args );
    return ret;
}

rc_t _KTableOpenColumnRead ( struct KTable const *self, const KColumn **col, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_cvt . f . tbl_v_open_column_read != NULL );
    ret = ( * kdb_cvt . f . tbl_v_open_column_read ) ( self, col, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVOpenColumnRead ( struct KDBManager const *self, const KColumn **col, const char *path, va_list args )
{
    assert ( kdb_cvt . f . mgr_v_open_column_read != NULL );
    return ( * kdb_cvt . f . mgr_v_open_column_read ) ( self, col, path, args );
}

rc_t _KTableVOpenColumnRead ( struct KTable const *self, const KColumn **col, const char *path, va_list args )
{
    assert ( kdb_cvt . f . tbl_v_open_column_read != NULL );
    return ( * kdb_cvt . f . tbl_v_open_column_read ) ( self, col, path, args );
}

rc_t _KDBManagerOpenColumnUpdate ( struct KDBManager *self, KColumn **col, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . mgr_v_open_column_update != NULL );
    ret = ( * kdb_wvt . f . mgr_v_open_column_update ) ( self, col, path, args );
    va_end ( args );
    return ret;
}

rc_t _KTableOpenColumnUpdate ( struct KTable *self, KColumn **col, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . tbl_v_open_column_update != NULL );
    ret = ( * kdb_wvt . f . tbl_v_open_column_update ) ( self, col, path, args );
    va_end ( args );
    return ret;
}

rc_t _KDBManagerVOpenColumnUpdate ( struct KDBManager *self, KColumn **col, const char *path, va_list args )
{
    assert ( kdb_wvt . f . mgr_v_open_column_update != NULL );
    return ( * kdb_wvt . f . mgr_v_open_column_update ) ( self, col, path, args );
}

rc_t _KTableVOpenColumnUpdate ( struct KTable *self, KColumn **col, const char *path, va_list args )
{
    assert ( kdb_wvt . f . tbl_v_open_column_update != NULL );
    return ( * kdb_wvt . f . tbl_v_open_column_update ) ( self, col, path, args );
}

bool _KColumnLocked ( const KColumn *self )
{
    assert ( kdb_cvt . f . col_locked != NULL );
    return ( * kdb_cvt . f . col_locked ) ( self );
}

rc_t _KColumnVersion ( const KColumn *self, uint32_t *version )
{
    assert ( kdb_cvt . f . col_version != NULL );
    return ( * kdb_cvt . f . col_version ) ( self, version );
}

rc_t _KColumnByteOrder ( const KColumn *self, bool *reversed )
{
    assert ( kdb_cvt . f . col_byte_order != NULL );
    return ( * kdb_cvt . f . col_byte_order ) ( self, reversed );
}

rc_t _KColumnIdRange ( const KColumn *self, int64_t *first, uint64_t *count )
{
    assert ( kdb_cvt . f . col_id_range != NULL );
    return ( * kdb_cvt . f . col_id_range ) ( self, first, count );
}

rc_t _KColumnReindex ( KColumn *self )
{
    assert ( kdb_wvt . f . col_reindex != NULL );
    return ( * kdb_wvt . f . col_reindex ) ( self );
}

rc_t _KColumnCommitFreq ( KColumn *self, uint32_t *freq )
{
    assert ( kdb_wvt . f . col_commit_freq != NULL );
    return ( * kdb_wvt . f . col_commit_freq ) ( self, freq );
}

rc_t _KColumnSetCommitFreq ( KColumn *self, uint32_t freq )
{
    assert ( kdb_wvt . f . col_set_commit_freq != NULL );
    return ( * kdb_wvt . f . col_set_commit_freq ) ( self, freq );
}

rc_t _KColumnOpenManagerRead ( const KColumn *self, struct KDBManager const **mgr )
{
    assert ( kdb_cvt . f . col_open_manager_read != NULL );
    return ( * kdb_cvt . f . col_open_manager_read ) ( self, mgr );
}

rc_t _KColumnOpenManagerUpdate ( KColumn *self, struct KDBManager **mgr )
{
    assert ( kdb_wvt . f . col_open_manager_update != NULL );
    return ( * kdb_wvt . f . col_open_manager_update ) ( self, mgr );
}

rc_t _KColumnOpenParentRead ( const KColumn *self, struct KTable const **tbl )
{
    assert ( kdb_cvt . f . col_open_parent_read != NULL );
    return ( * kdb_cvt . f . col_open_parent_read ) ( self, tbl );
}

rc_t _KColumnOpenParentUpdate ( KColumn *self, struct KTable **tbl )
{
    assert ( kdb_wvt . f . col_open_parent_update != NULL );
    return ( * kdb_wvt . f . col_open_parent_update ) ( self, tbl );
}

rc_t _KColumnBlobAddRef ( const KColumnBlob *self )
{
    assert ( kdb_cvt . f . col_blob_add_ref != NULL );
    return ( * kdb_cvt . f . col_blob_add_ref ) ( self );
}

rc_t _KColumnBlobRelease ( const KColumnBlob *self )
{
    assert ( kdb_cvt . f . col_blob_release != NULL );
    return ( * kdb_cvt . f . col_blob_release ) ( self );
}

rc_t _KColumnCreateBlob ( KColumn *self, KColumnBlob **blob )
{
    assert ( kdb_wvt . f . col_create_blob != NULL );
    return ( * kdb_wvt . f . col_create_blob ) ( self, blob );
}

rc_t _KColumnOpenBlobRead ( const KColumn *self, const KColumnBlob **blob, int64_t id )
{
    assert ( kdb_cvt . f . col_open_blob_read != NULL );
    return ( * kdb_cvt . f . col_open_blob_read ) ( self, blob, id );
}

rc_t _KColumnOpenBlobUpdate ( KColumn *self, KColumnBlob **blob, int64_t id )
{
    assert ( kdb_wvt . f . col_open_blob_update != NULL );
    return ( * kdb_wvt . f . col_open_blob_update ) ( self, blob, id );
}

rc_t _KColumnBlobRead ( const KColumnBlob *self, size_t offset, void *buffer, size_t bsize, size_t *num_read, size_t *remaining )
{
    assert ( kdb_cvt . f . col_blob_read != NULL );
    return ( * kdb_cvt . f . col_blob_read ) ( self, offset, buffer, bsize, num_read, remaining );
}

rc_t _KColumnBlobAppend ( KColumnBlob *self, const void *buffer, size_t size )
{
    assert ( kdb_wvt . f . col_blob_append != NULL );
    return ( * kdb_wvt . f . col_blob_append ) ( self, buffer, size );
}

rc_t _KColumnBlobValidate ( const KColumnBlob *self )
{
    assert ( kdb_cvt . f . col_blob_validate != NULL );
    return ( * kdb_cvt . f . col_blob_validate ) ( self );
}

rc_t _KColumnBlobIdRange ( const KColumnBlob *self, int64_t *first, uint32_t *count )
{
    assert ( kdb_cvt . f . col_blob_id_range != NULL );
    return ( * kdb_cvt . f . col_blob_id_range ) ( self, first, count );
}

rc_t _KColumnBlobAssignRange ( KColumnBlob *self, int64_t first, uint32_t count )
{
    assert ( kdb_wvt . f . col_blob_assign_range != NULL );
    return ( * kdb_wvt . f . col_blob_assign_range ) ( self, first, count );
}

rc_t _KColumnBlobCommit ( KColumnBlob *self )
{
    assert ( kdb_wvt . f . col_blob_commit != NULL );
    return ( * kdb_wvt . f . col_blob_commit ) ( self );
}

rc_t _KIndexAddRef ( const KIndex *self )
{
    assert ( kdb_cvt . f . idx_add_ref != NULL );
    return ( * kdb_cvt . f . idx_add_ref ) ( self );
}

rc_t _KIndexRelease ( const KIndex *self )
{
    assert ( kdb_cvt . f . idx_release != NULL );
    return ( * kdb_cvt . f . idx_release ) ( self );
}

rc_t _KDatabaseCreateIndex ( struct KDatabase *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . db_v_create_index != NULL );
    ret = ( * kdb_wvt . f . db_v_create_index ) ( self, idx, type, cmode, name, args );
    va_end ( args );
    return ret;
}

rc_t _KTableCreateIndex ( struct KTable *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . tbl_v_create_index != NULL );
    ret = ( * kdb_wvt . f . tbl_v_create_index ) ( self, idx, type, cmode, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseVCreateIndex ( struct KDatabase *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, va_list args )
{
    assert ( kdb_wvt . f . db_v_create_index != NULL );
    return ( * kdb_wvt . f . db_v_create_index ) ( self, idx, type, cmode, name, args );
}

rc_t _KTableVCreateIndex ( struct KTable *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, va_list args )
{
    assert ( kdb_wvt . f . tbl_v_create_index != NULL );
    return ( * kdb_wvt . f . tbl_v_create_index ) ( self, idx, type, cmode, name, args );
}

rc_t _KDatabaseOpenIndexRead ( struct KDatabase const *self, const KIndex **idx, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_cvt . f . db_v_open_index_read != NULL );
    ret = ( * kdb_cvt . f . db_v_open_index_read ) ( self, idx, name, args );
    va_end ( args );
    return ret;
}

rc_t _KTableOpenIndexRead ( struct KTable const *self, const KIndex **idx, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_cvt . f . tbl_v_open_index_read != NULL );
    ret = ( * kdb_cvt . f . tbl_v_open_index_read ) ( self, idx, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseVOpenIndexRead ( struct KDatabase const *self, const KIndex **idx, const char *name, va_list args )
{
    assert ( kdb_cvt . f . db_v_open_index_read != NULL );
    return ( * kdb_cvt . f . db_v_open_index_read ) ( self, idx, name, args );
}

rc_t _KTableVOpenIndexRead ( struct KTable const *self, const KIndex **idx, const char *name, va_list args )
{
    assert ( kdb_cvt . f . tbl_v_open_index_read != NULL );
    return ( * kdb_cvt . f . tbl_v_open_index_read ) ( self, idx, name, args );
}

rc_t _KDatabaseOpenIndexUpdate ( struct KDatabase *self, KIndex **idx, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . db_v_open_index_update != NULL );
    ret = ( * kdb_wvt . f . db_v_open_index_update ) ( self, idx, name, args );
    va_end ( args );
    return ret;
}

rc_t _KTableOpenIndexUpdate ( struct KTable *self, KIndex **idx, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( kdb_wvt . f . tbl_v_open_index_update != NULL );
    ret = ( * kdb_wvt . f . tbl_v_open_index_update ) ( self, idx, name, args );
    va_end ( args );
    return ret;
}

rc_t _KDatabaseVOpenIndexUpdate ( struct KDatabase *self, KIndex **idx, const char *name, va_list args )
{
    assert ( kdb_wvt . f . db_v_open_index_update != NULL );
    return ( * kdb_wvt . f . db_v_open_index_update ) ( self, idx, name, args );
}

rc_t _KTableVOpenIndexUpdate ( struct KTable *self, KIndex **idx, const char *name, va_list args )
{
    assert ( kdb_wvt . f . tbl_v_open_index_update != NULL );
    return ( * kdb_wvt . f . tbl_v_open_index_update ) ( self, idx, name, args );
}

bool _KIndexLocked ( const KIndex *self )
{
    assert ( kdb_cvt . f . idx_locked != NULL );
    return ( * kdb_cvt . f . idx_locked ) ( self );
}

rc_t _KIndexVersion ( const KIndex *self, uint32_t *version )
{
    assert ( kdb_cvt . f . idx_version != NULL );
    return ( * kdb_cvt . f . idx_version ) ( self, version );
}

rc_t _KIndexType ( const KIndex *self, KIdxType *type )
{
    assert ( kdb_cvt . f . idx_type != NULL );
    return ( * kdb_cvt . f . idx_type ) ( self, type );
}

rc_t _KIndexCommit ( KIndex *self )
{
    assert ( kdb_wvt . f . idx_commit != NULL );
    return ( * kdb_wvt . f . idx_commit ) ( self );
}

rc_t _KIndexConsistencyCheck ( const KIndex *self, uint32_t level, int64_t *start_id, uint64_t *id_range, uint64_t *num_keys, uint64_t *num_rows, uint64_t *num_holes )
{
    assert ( kdb_cvt . f . idx_consistency_check != NULL );
    return ( * kdb_cvt . f . idx_consistency_check ) ( self, level, start_id, id_range, num_keys, num_rows, num_holes );
}

rc_t _KIndexInsertText ( KIndex *self, bool unique, const char *key, int64_t id )
{
    assert ( kdb_wvt . f . idx_insert_text != NULL );
    return ( * kdb_wvt . f . idx_insert_text ) ( self, unique, key, id );
}

rc_t _KIndexDeleteText ( KIndex *self, const char *key )
{
    assert ( kdb_wvt . f . idx_delete_text != NULL );
    return ( * kdb_wvt . f . idx_delete_text ) ( self, key );
}

rc_t _KIndexFindText ( const KIndex *self, const char *key, int64_t *start_id, uint64_t *id_count, int ( CC * custom_cmp ) ( const void *item, struct PBSTNode const *n, void *data ), void *data )
{
    assert ( kdb_cvt . f . idx_find_text != NULL );
    return ( * kdb_cvt . f . idx_find_text ) ( self, key, start_id, id_count, custom_cmp, data );
}

rc_t _KIndexFindAllText ( const KIndex *self, const char *key, rc_t ( CC * f ) ( int64_t start_id, uint64_t id_count, void *data ), void *data )
{
    assert ( kdb_cvt . f . idx_find_all_text != NULL );
    return ( * kdb_cvt . f . idx_find_all_text ) ( self, key, f, data );
}

rc_t _KIndexProjectText ( const KIndex *self, int64_t id, int64_t *start_id, uint64_t *id_count, char *key, size_t kmax, size_t *actsize )
{
    assert ( kdb_cvt . f . idx_project_text != NULL );
    return ( * kdb_cvt . f . idx_project_text ) ( self, id, start_id, id_count, key, kmax, actsize );
}

rc_t _KIndexProjectAllText ( const KIndex *self, int64_t id, rc_t ( CC * f ) ( int64_t start_id, uint64_t id_count, const char *key, void *data ), void *data )
{
    assert ( kdb_cvt . f . idx_project_all_text != NULL );
    return ( * kdb_cvt . f . idx_project_all_text ) ( self, id, f, data );
}

rc_t _KIndexInsertU64 ( KIndex *self, bool unique, uint64_t key, uint64_t key_size, int64_t start_id, uint64_t id_count )
{
    assert ( kdb_wvt . f . idx_insert_u64 != NULL );
    return ( * kdb_wvt . f . idx_insert_u64 ) ( self, unique, key, key_size, start_id, id_count );
}

rc_t _KIndexDeleteU64 ( KIndex *self, uint64_t key )
{
    assert ( kdb_wvt . f . idx_delete_u64 != NULL );
    return ( * kdb_wvt . f . idx_delete_u64 ) ( self, key );
}

rc_t _KIndexFindU64 ( const KIndex *self, uint64_t offset, uint64_t *key, uint64_t *key_size, int64_t *start_id, uint64_t *id_count )
{
    assert ( kdb_cvt . f . idx_find_u64 != NULL );
    return ( * kdb_cvt . f . idx_find_u64 ) ( self, offset, key, key_size, start_id, id_count );
}

rc_t _KMetadataAddRef ( const KMetadata *self )
{
    assert ( kdb_cvt . f . meta_add_ref != NULL );
    return ( * kdb_cvt . f . meta_add_ref ) ( self );
}

rc_t _KMetadataRelease ( const KMetadata *self )
{
    assert ( kdb_cvt . f . meta_release != NULL );
    return ( * kdb_cvt . f . meta_release ) ( self );
}

rc_t _KDatabaseOpenMetadataRead ( struct KDatabase const *self, const KMetadata **meta )
{
    assert ( kdb_cvt . f . db_open_metadata_read != NULL );
    return ( * kdb_cvt . f . db_open_metadata_read ) ( self, meta );
}

rc_t _KTableOpenMetadataRead ( struct KTable const *self, const KMetadata **meta )
{
    assert ( kdb_cvt . f . tbl_open_metadata_read != NULL );
    return ( * kdb_cvt . f . tbl_open_metadata_read ) ( self, meta );
}

rc_t _KColumnOpenMetadataRead ( struct KColumn const *self, const KMetadata **meta )
{
    assert ( kdb_cvt . f . col_open_metadata_read != NULL );
    return ( * kdb_cvt . f . col_open_metadata_read ) ( self, meta );
}

rc_t _KDatabaseOpenMetadataUpdate ( struct KDatabase *self, KMetadata **meta )
{
    assert ( kdb_wvt . f . db_open_metadata_update != NULL );
    return ( * kdb_wvt . f . db_open_metadata_update ) ( self, meta );
}

rc_t _KTableOpenMetadataUpdate ( struct KTable *self, KMetadata **meta )
{
    assert ( kdb_wvt . f . tbl_open_metadata_update != NULL );
    return ( * kdb_wvt . f . tbl_open_metadata_update ) ( self, meta );
}

rc_t _KColumnOpenMetadataUpdate ( struct KColumn *self, KMetadata **meta )
{
    assert ( kdb_wvt . f . col_open_metadata_update != NULL );
    return ( * kdb_wvt . f . col_open_metadata_update ) ( self, meta );
}

rc_t _KMetadataVersion ( const KMetadata *self, uint32_t *version )
{
    assert ( kdb_cvt . f . meta_version != NULL );
    return ( * kdb_cvt . f . meta_version ) ( self, version );
}

rc_t _KMetadataByteOrder ( const KMetadata *self, bool *reversed )
{
    assert ( kdb_cvt . f . meta_byte_order != NULL );
    return ( * kdb_cvt . f . meta_byte_order ) ( self, reversed );
}

rc_t _KMetadataRevision ( const KMetadata *self, uint32_t *revision )
{
    assert ( kdb_cvt . f . meta_revision != NULL );
    return ( * kdb_cvt . f . meta_revision ) ( self, revision );
}

rc_t _KMetadataMaxRevision ( const KMetadata *self, uint32_t *revision )
{
    assert ( kdb_cvt . f . meta_max_revision != NULL );
    return ( * kdb_cvt . f . meta_max_revision ) ( self, revision );
}

rc_t _KMetadataCommit ( KMetadata *self )
{
    assert ( kdb_wvt . f . meta_commit != NULL );
    return ( * kdb_wvt . f . meta_commit ) ( self );
}

rc_t _KMetadataFreeze ( KMetadata *self )
{
    assert ( kdb_wvt . f . meta_freeze != NULL );
    return ( * kdb_wvt . f . meta_freeze ) ( self );
}

rc_t _KMetadataOpenRevision ( const KMetadata *self, const KMetadata **meta, uint32_t revision )
{
    assert ( kdb_cvt . f . meta_open_revision != NULL );
    return ( * kdb_cvt . f . meta_open_revision ) ( self, meta, revision );
}

rc_t _KMetadataGetSequence ( const KMetadata *self, const char *seq, int64_t *val )
{
    assert ( kdb_cvt . f . meta_get_sequence != NULL );
    return ( * kdb_cvt . f . meta_get_sequence ) ( self, seq, val );
}

rc_t _KMetadataSetSequence ( KMetadata *self, const char *seq, int64_t val )
{
    assert ( kdb_wvt . f . meta_set_sequence != NULL );
    return ( * kdb_wvt . f . meta_set_sequence ) ( self, seq, val );
}

rc_t _KMetadataNextSequence ( KMetadata *self, const char *seq, int64_t *val )
{
    assert ( kdb_wvt . f . meta_next_sequence != NULL );
    return ( * kdb_wvt . f . meta_next_sequence ) ( self, seq, val );
}

rc_t _KMDataNodeAddRef ( const KMDataNode *self )
{
    assert ( kdb_cvt . f . node_add_ref != NULL );
    return ( * kdb_cvt . f . node_add_ref ) ( self );
}

rc_t _KMDataNodeRelease ( const KMDataNode *self )
{
    assert ( kdb_cvt . f . node_release != NULL );
    return ( * kdb_cvt . f . node_release ) ( self );
}

rc_t _KMetadataOpenNodeRead ( const KMetadata *self, const KMDataNode **node, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_cvt . f . meta_v_open_node_read != NULL );
    ret = ( * kdb_cvt . f . meta_v_open_node_read ) ( self, node, path, args );
    va_end ( args );
    return ret;
}

rc_t _KMDataNodeOpenNodeRead ( const KMDataNode *self, const KMDataNode **node, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_cvt . f . node_v_open_node_read != NULL );
    ret = ( * kdb_cvt . f . node_v_open_node_read ) ( self, node, path, args );
    va_end ( args );
    return ret;
}

rc_t _KMetadataVOpenNodeRead ( const KMetadata *self, const KMDataNode **node, const char *path, va_list args )
{
    assert ( kdb_cvt . f . meta_v_open_node_read != NULL );
    return ( * kdb_cvt . f . meta_v_open_node_read ) ( self, node, path, args );
}

rc_t _KMDataNodeVOpenNodeRead ( const KMDataNode *self, const KMDataNode **node, const char *path, va_list args )
{
    assert ( kdb_cvt . f . node_v_open_node_read != NULL );
    return ( * kdb_cvt . f . node_v_open_node_read ) ( self, node, path, args );
}

rc_t _KMetadataOpenNodeUpdate ( KMetadata *self, KMDataNode **node, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . meta_v_open_node_update != NULL );
    ret = ( * kdb_wvt . f . meta_v_open_node_update ) ( self, node, path, args );
    va_end ( args );
    return ret;
}

rc_t _KMDataNodeOpenNodeUpdate ( KMDataNode *self, KMDataNode **node, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . node_v_open_node_update != NULL );
    ret = ( * kdb_wvt . f . node_v_open_node_update ) ( self, node, path, args );
    va_end ( args );
    return ret;
}

rc_t _KMetadataVOpenNodeUpdate ( KMetadata *self, KMDataNode **node, const char *path, va_list args )
{
    assert ( kdb_wvt . f . meta_v_open_node_update != NULL );
    return ( * kdb_wvt . f . meta_v_open_node_update ) ( self, node, path, args );
}

rc_t _KMDataNodeVOpenNodeUpdate ( KMDataNode *self, KMDataNode **node, const char *path, va_list args )
{
    assert ( kdb_wvt . f . node_v_open_node_update != NULL );
    return ( * kdb_wvt . f . node_v_open_node_update ) ( self, node, path, args );
}

rc_t _KMDataNodeByteOrder ( const KMDataNode *self, bool *reversed )
{
    assert ( kdb_cvt . f . node_byte_order != NULL );
    return ( * kdb_cvt . f . node_byte_order ) ( self, reversed );
}

rc_t _KMDataNodeRead ( const KMDataNode *self, size_t offset, void *buffer, size_t bsize, size_t *num_read, size_t *remaining )
{
    assert ( kdb_cvt . f . node_read != NULL );
    return ( * kdb_cvt . f . node_read ) ( self, offset, buffer, bsize, num_read, remaining );
}

rc_t _KMDataNodeWrite ( KMDataNode *self, const void *buffer, size_t size )
{
    assert ( kdb_wvt . f . node_write != NULL );
    return ( * kdb_wvt . f . node_write ) ( self, buffer, size );
}

rc_t _KMDataNodeAppend ( KMDataNode *self, const void *buffer, size_t size )
{
    assert ( kdb_wvt . f . node_append != NULL );
    return ( * kdb_wvt . f . node_append ) ( self, buffer, size );
}

rc_t _KMDataNodeReadB8 ( const KMDataNode *self, void *b8 )
{
    assert ( kdb_cvt . f . node_read_b8 != NULL );
    return ( * kdb_cvt . f . node_read_b8 ) ( self, b8 );
}

rc_t _KMDataNodeReadB16 ( const KMDataNode *self, void *b16 )
{
    assert ( kdb_cvt . f . node_read_b16 != NULL );
    return ( * kdb_cvt . f . node_read_b16 ) ( self, b16 );
}

rc_t _KMDataNodeReadB32 ( const KMDataNode *self, void *b32 )
{
    assert ( kdb_cvt . f . node_read_b32 != NULL );
    return ( * kdb_cvt . f . node_read_b32 ) ( self, b32 );
}

rc_t _KMDataNodeReadB64 ( const KMDataNode *self, void *b64 )
{
    assert ( kdb_cvt . f . node_read_b64 != NULL );
    return ( * kdb_cvt . f . node_read_b64 ) ( self, b64 );
}

rc_t _KMDataNodeReadB128 ( const KMDataNode *self, void *b128 )
{
    assert ( kdb_cvt . f . node_read_b128 != NULL );
    return ( * kdb_cvt . f . node_read_b128 ) ( self, b128 );
}

rc_t _KMDataNodeReadAsI16 ( const KMDataNode *self, int16_t *i )
{
    assert ( kdb_cvt . f . node_read_as_i16 != NULL );
    return ( * kdb_cvt . f . node_read_as_i16 ) ( self, i );
}

rc_t _KMDataNodeReadAsU16 ( const KMDataNode *self, uint16_t *u )
{
    assert ( kdb_cvt . f . node_read_as_u16 != NULL );
    return ( * kdb_cvt . f . node_read_as_u16 ) ( self, u );
}

rc_t _KMDataNodeReadAsI32 ( const KMDataNode *self, int32_t *i )
{
    assert ( kdb_cvt . f . node_read_as_i32 != NULL );
    return ( * kdb_cvt . f . node_read_as_i32 ) ( self, i );
}

rc_t _KMDataNodeReadAsU32 ( const KMDataNode *self, uint32_t *u )
{
    assert ( kdb_cvt . f . node_read_as_u32 != NULL );
    return ( * kdb_cvt . f . node_read_as_u32 ) ( self, u );
}

rc_t _KMDataNodeReadAsI64 ( const KMDataNode *self, int64_t *i )
{
    assert ( kdb_cvt . f . node_read_as_i64 != NULL );
    return ( * kdb_cvt . f . node_read_as_i64 ) ( self, i );
}

rc_t _KMDataNodeReadAsU64 ( const KMDataNode *self, uint64_t *u )
{
    assert ( kdb_cvt . f . node_read_as_u64 != NULL );
    return ( * kdb_cvt . f . node_read_as_u64 ) ( self, u );
}

rc_t _KMDataNodeReadAsF64 ( const KMDataNode *self, double *f )
{
    assert ( kdb_cvt . f . node_read_as_f64 != NULL );
    return ( * kdb_cvt . f . node_read_as_f64 ) ( self, f );
}

rc_t _KMDataNodeReadCString ( const KMDataNode *self, char *buffer, size_t bsize, size_t *size )
{
    assert ( kdb_cvt . f . node_read_c_string != NULL );
    return ( * kdb_cvt . f . node_read_c_string ) ( self, buffer, bsize, size );
}

rc_t _KMDataNodeWriteB8 ( KMDataNode *self, const void *b8 )
{
    assert ( kdb_wvt . f . node_write_b8 != NULL );
    return ( * kdb_wvt . f . node_write_b8 ) ( self, b8 );
}

rc_t _KMDataNodeWriteB16 ( KMDataNode *self, const void *b16 )
{
    assert ( kdb_wvt . f . node_write_b16 != NULL );
    return ( * kdb_wvt . f . node_write_b16 ) ( self, b16 );
}

rc_t _KMDataNodeWriteB32 ( KMDataNode *self, const void *b32 )
{
    assert ( kdb_wvt . f . node_write_b32 != NULL );
    return ( * kdb_wvt . f . node_write_b32 ) ( self, b32 );
}

rc_t _KMDataNodeWriteB64 ( KMDataNode *self, const void *b64 )
{
    assert ( kdb_wvt . f . node_write_b64 != NULL );
    return ( * kdb_wvt . f . node_write_b64 ) ( self, b64 );
}

rc_t _KMDataNodeWriteB128 ( KMDataNode *self, const void *b128 )
{
    assert ( kdb_wvt . f . node_write_b128 != NULL );
    return ( * kdb_wvt . f . node_write_b128 ) ( self, b128 );
}

rc_t _KMDataNodeWriteCString ( KMDataNode *self, const char *str )
{
    assert ( kdb_wvt . f . node_write_c_string != NULL );
    return ( * kdb_wvt . f . node_write_c_string ) ( self, str );
}

rc_t _KMDataNodeReadAttr ( const KMDataNode *self, const char *name, char *buffer, size_t bsize, size_t *size )
{
    assert ( kdb_cvt . f . node_read_attr != NULL );
    return ( * kdb_cvt . f . node_read_attr ) ( self, name, buffer, bsize, size );
}

rc_t _KMDataNodeWriteAttr ( KMDataNode *self, const char *name, const char *value )
{
    assert ( kdb_wvt . f . node_write_attr != NULL );
    return ( * kdb_wvt . f . node_write_attr ) ( self, name, value );
}

rc_t _KMDataNodeReadAttrAsI16 ( const KMDataNode *self, const char *attr, int16_t *i )
{
    assert ( kdb_cvt . f . node_read_attr_as_i16 != NULL );
    return ( * kdb_cvt . f . node_read_attr_as_i16 ) ( self, attr, i );
}

rc_t _KMDataNodeReadAttrAsU16 ( const KMDataNode *self, const char *attr, uint16_t *u )
{
    assert ( kdb_cvt . f . node_read_attr_as_u16 != NULL );
    return ( * kdb_cvt . f . node_read_attr_as_u16 ) ( self, attr, u );
}

rc_t _KMDataNodeReadAttrAsI32 ( const KMDataNode *self, const char *attr, int32_t *i )
{
    assert ( kdb_cvt . f . node_read_attr_as_i32 != NULL );
    return ( * kdb_cvt . f . node_read_attr_as_i32 ) ( self, attr, i );
}

rc_t _KMDataNodeReadAttrAsU32 ( const KMDataNode *self, const char *attr, uint32_t *u )
{
    assert ( kdb_cvt . f . node_read_attr_as_u32 != NULL );
    return ( * kdb_cvt . f . node_read_attr_as_u32 ) ( self, attr, u );
}

rc_t _KMDataNodeReadAttrAsI64 ( const KMDataNode *self, const char *attr, int64_t *i )
{
    assert ( kdb_cvt . f . node_read_attr_as_i64 != NULL );
    return ( * kdb_cvt . f . node_read_attr_as_i64 ) ( self, attr, i );
}

rc_t _KMDataNodeReadAttrAsU64 ( const KMDataNode *self, const char *attr, uint64_t *u )
{
    assert ( kdb_cvt . f . node_read_attr_as_u64 != NULL );
    return ( * kdb_cvt . f . node_read_attr_as_u64 ) ( self, attr, u );
}

rc_t _KMDataNodeReadAttrAsF64 ( const KMDataNode *self, const char *attr, double *f )
{
    assert ( kdb_cvt . f . node_read_attr_as_f64 != NULL );
    return ( * kdb_cvt . f . node_read_attr_as_f64 ) ( self, attr, f );
}

rc_t _KMDataNodeDropAll ( KMDataNode *self )
{
    assert ( kdb_wvt . f . node_drop_all != NULL );
    return ( * kdb_wvt . f . node_drop_all ) ( self );
}

rc_t _KMDataNodeDropAttr ( KMDataNode *self, const char *attr )
{
    assert ( kdb_wvt . f . node_drop_attr != NULL );
    return ( * kdb_wvt . f . node_drop_attr ) ( self, attr );
}

rc_t _KMDataNodeDropChild ( KMDataNode *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( kdb_wvt . f . node_v_drop_child != NULL );
    ret = ( * kdb_wvt . f . node_v_drop_child ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _KMDataNodeVDropChild ( KMDataNode *self, const char *path, va_list args )
{
    assert ( kdb_wvt . f . node_v_drop_child != NULL );
    return ( * kdb_wvt . f . node_v_drop_child ) ( self, path, args );
}

rc_t _KMDataNodeRenameAttr ( KMDataNode *self, const char *from, const char *to )
{
    assert ( kdb_wvt . f . node_rename_attr != NULL );
    return ( * kdb_wvt . f . node_rename_attr ) ( self, from, to );
}

rc_t _KMDataNodeRenameChild ( KMDataNode *self, const char *from, const char *to )
{
    assert ( kdb_wvt . f . node_rename_child != NULL );
    return ( * kdb_wvt . f . node_rename_child ) ( self, from, to );
}

rc_t _KDatabaseListDB ( struct KDatabase const *self, struct KNamelist **names )
{
    assert ( kdb_cvt . f . db_list_db != NULL );
    return ( * kdb_cvt . f . db_list_db ) ( self, names );
}

rc_t _KDatabaseListTbl ( struct KDatabase const *self, struct KNamelist **names )
{
    assert ( kdb_cvt . f . db_list_tbl != NULL );
    return ( * kdb_cvt . f . db_list_tbl ) ( self, names );
}

rc_t _KDatabaseListIdx ( struct KDatabase const *self, struct KNamelist **names )
{
    assert ( kdb_cvt . f . db_list_idx != NULL );
    return ( * kdb_cvt . f . db_list_idx ) ( self, names );
}

rc_t _KTableListCol ( struct KTable const *self, struct KNamelist **names )
{
    assert ( kdb_cvt . f . tbl_list_col != NULL );
    return ( * kdb_cvt . f . tbl_list_col ) ( self, names );
}

rc_t _KTableListIdx ( struct KTable const *self, struct KNamelist **names )
{
    assert ( kdb_cvt . f . tbl_list_idx != NULL );
    return ( * kdb_cvt . f . tbl_list_idx ) ( self, names );
}

rc_t _KMDataNodeListAttr ( struct KMDataNode const *self, struct KNamelist **names )
{
    assert ( kdb_cvt . f . node_list_attr != NULL );
    return ( * kdb_cvt . f . node_list_attr ) ( self, names );
}

rc_t _KMDataNodeListChildren ( struct KMDataNode const *self, struct KNamelist **names )
{
    assert ( kdb_cvt . f . node_list_children != NULL );
    return ( * kdb_cvt . f . node_list_children ) ( self, names );
}

