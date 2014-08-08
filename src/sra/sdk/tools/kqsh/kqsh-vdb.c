/* THIS IS AN AUTO-GENERATED FILE - DO NOT EDIT */

#include "kqsh-vdb.h"
#include "kqsh-priv.h"
#include <klib/rc.h>
#include <assert.h>

static const char *vdb_msgs [] =
{
    "VDBManagerMakeRead",
    "VDBManagerMakeUpdate",
    "VDBManagerAddRef",
    "VDBManagerRelease",
    "VDBManagerVersion",
    "VDBManagerVWritable",
    "VDBManagerVAddSchemaIncludePath",
    "VDBManagerVAddLoadLibraryPath",
    "VDBManagerGetUserData",
    "VDBManagerSetUserData",
    "VDBManagerGetObjVersion",
    "VDBManagerVPathType",
    "VDatabaseAddRef",
    "VDatabaseRelease",
    "VDBManagerVOpenDBRead",
    "VDatabaseVOpenDBRead",
    "VDatabaseTypespec",
    "VDatabaseLocked",
    "VDatabaseIsAlias",
    "VDatabaseVWritable",
    "VDatabaseOpenMetadataRead",
    "VDatabaseOpenManagerRead",
    "VDatabaseOpenParentRead",
    "VDatabaseOpenSchema",
    "VDatabaseListTbl",
    "VDatabaseListDB",
    "VDatabaseGetUserData",
    "VDatabaseSetUserData",
    "VSchemaAddRef",
    "VSchemaRelease",
    "VDBManagerMakeSchema",
    "VSchemaVAddIncludePath",
    "VSchemaParseText",
    "VSchemaVParseFile",
    "VSchemaDump",
    "VSchemaIncludeFiles",
    "VSchemaVResolveTypedecl",
    "VTypedeclToText",
    "VTypedeclToSupertype",
    "VTypedeclToType",
    "VTypedeclToTypedecl",
    "VTypedeclCommonAncestor",
    "VTypedescSizeof",
    "VSchemaDescribeTypedecl",
    "VSchemaMakeRuntimeTable",
    "VSchemaRuntimeTableClose",
    "VSchemaRuntimeTableCommit",
    "VSchemaRuntimeTableVAddColumn",
    "VSchemaRuntimeTableVAddBooleanColumn",
    "VSchemaRuntimeTableVAddIntegerColumn",
    "VSchemaRuntimeTableVAddFloatColumn",
    "VSchemaRuntimeTableVAddAsciiColumn",
    "VSchemaRuntimeTableVAddUnicodeColumn",
    "VTableAddRef",
    "VTableRelease",
    "VDBManagerVOpenTableRead",
    "VDatabaseVOpenTableRead",
    "VTableTypespec",
    "VTableLocked",
    "VTableVWritable",
    "VTableOpenMetadataRead",
    "VTableVOpenIndexRead",
    "VTableListReadableColumns",
    "VTableListCol",
    "VTableListReadableDatatypes",
    "VTableColumnDatatypes",
    "VTableOpenManagerRead",
    "VTableOpenParentRead",
    "VTableOpenSchema",
    "VTableGetUserData",
    "VTableSetUserData",
    "VCursorAddRef",
    "VCursorRelease",
    "VTableCreateCursorRead",
    "VTableCreateCachedCursorRead",
    "VCursorVAddColumn",
    "VCursorVGetColumnIdx",
    "VCursorDatatype",
    "VCursorIdRange",
    "VCursorOpen",
    "VCursorRowId",
    "VCursorSetRowId",
    "VCursorOpenRow",
    "VCursorCloseRow",
    "VCursorGetBlob",
    "VCursorGetBlobDirect",
    "VCursorRead",
    "VCursorReadDirect",
    "VCursorReadBits",
    "VCursorReadBitsDirect",
    "VCursorCellData",
    "VCursorCellDataDirect",
    "VCursorOpenParentRead",
    "VCursorGetUserData",
    "VCursorSetUserData",
    NULL
};

static union
{
    fptr_t slots [ sizeof vdb_msgs / sizeof vdb_msgs [ 0 ] - 2 ];

    struct
    {
        rc_t ( CC * mgr_make ) ( VDBManager**, struct KDirectory* );
        rc_t ( CC * mgr_add_ref ) ( const VDBManager* );
        rc_t ( CC * mgr_release ) ( const VDBManager* );
        rc_t ( CC * mgr_version ) ( const VDBManager*, uint32_t* );
        rc_t ( CC * mgr_v_writable ) ( const VDBManager*, const char*, va_list );
        rc_t ( CC * mgr_v_add_schema_include_path ) ( const VDBManager*, const char*, va_list );
        rc_t ( CC * mgr_v_add_load_library_path ) ( const VDBManager*, const char*, va_list );
        rc_t ( CC * mgr_get_user_data ) ( const VDBManager*, void** );
        rc_t ( CC * mgr_set_user_data ) ( const VDBManager*, void*, void ( CC * ) ( void *data ) );
        rc_t ( CC * mgr_get_obj_version ) ( const VDBManager*, ver_t*, const char* );
        int ( CC * mgr_v_path_type ) ( const VDBManager*, const char*, va_list );
        rc_t ( CC * db_add_ref ) ( const VDatabase* );
        rc_t ( CC * db_release ) ( const VDatabase* );
        rc_t ( CC * mgr_v_open_db_read ) ( struct VDBManager const*, const VDatabase**, struct VSchema const*, const char*, va_list );
        rc_t ( CC * db_v_open_db_read ) ( const VDatabase*, const VDatabase**, const char*, va_list );
        rc_t ( CC * db_typespec ) ( const VDatabase*, char*, size_t );
        bool ( CC * db_locked ) ( const VDatabase* );
        bool ( CC * db_is_alias ) ( const VDatabase*, uint32_t, char*, size_t, const char* );
        rc_t ( CC * db_v_writable ) ( const VDatabase*, uint32_t, const char*, va_list );
        rc_t ( CC * db_open_metadata_read ) ( const VDatabase*, struct KMetadata const** );
        rc_t ( CC * db_open_manager_read ) ( const VDatabase*, struct VDBManager const** );
        rc_t ( CC * db_open_parent_read ) ( const VDatabase*, const VDatabase** );
        rc_t ( CC * db_open_schema ) ( const VDatabase*, struct VSchema const** );
        rc_t ( CC * db_list_tbl ) ( const VDatabase*, KNamelist** );
        rc_t ( CC * db_list_db ) ( const VDatabase*, KNamelist** );
        rc_t ( CC * db_get_user_data ) ( const VDatabase*, void** );
        rc_t ( CC * db_set_user_data ) ( const VDatabase*, void*, void ( CC * ) ( void *data ) );
        rc_t ( CC * schema_add_ref ) ( const VSchema* );
        rc_t ( CC * schema_release ) ( const VSchema* );
        rc_t ( CC * mgr_make_schema ) ( struct VDBManager const*, VSchema** );
        rc_t ( CC * schema_v_add_include_path ) ( VSchema*, const char*, va_list );
        rc_t ( CC * schema_parse_text ) ( VSchema*, const char*, const char*, size_t );
        rc_t ( CC * schema_v_parse_file ) ( VSchema*, const char*, va_list );
        rc_t ( CC * schema_dump ) ( const VSchema*, uint32_t, const char*, rc_t ( CC * ) ( void *dst, const void *buffer, size_t bsize ), void* );
        rc_t ( CC * schema_include_files ) ( const VSchema*, struct KNamelist const** );
        rc_t ( CC * schema_v_resolve_typedecl ) ( const VSchema*, VTypedecl*, const char*, va_list );
        rc_t ( CC * td_to_text ) ( const VTypedecl*, const VSchema*, char*, size_t );
        bool ( CC * td_to_supertype ) ( const VTypedecl*, const VSchema*, VTypedecl* );
        bool ( CC * td_to_type ) ( const VTypedecl*, const VSchema*, uint32_t, VTypedecl*, uint32_t* );
        bool ( CC * td_to_typedecl ) ( const VTypedecl*, const VSchema*, const VTypedecl*, VTypedecl*, uint32_t* );
        bool ( CC * td_common_ancestor ) ( const VTypedecl*, const VSchema*, const VTypedecl*, VTypedecl*, uint32_t* );
        uint32_t ( CC * tdesc_sizeof ) ( const VTypedesc* );
        rc_t ( CC * schema_describe_typedecl ) ( const VSchema*, VTypedesc*, const VTypedecl* );
        rc_t ( CC * schema_make_runtime_table ) ( VSchema*, VSchemaRuntimeTable**, const char*, const char* );
        rc_t ( CC * srtt_close ) ( VSchemaRuntimeTable* );
        rc_t ( CC * srtt_commit ) ( VSchemaRuntimeTable* );
        rc_t ( CC * srtt_v_add_column ) ( VSchemaRuntimeTable*, const VTypedecl*, const char*, const char*, va_list );
        rc_t ( CC * srtt_v_add_boolean_column ) ( VSchemaRuntimeTable*, const char*, va_list );
        rc_t ( CC * srtt_v_add_integer_column ) ( VSchemaRuntimeTable*, uint32_t, bool, const char*, va_list );
        rc_t ( CC * srtt_v_add_float_column ) ( VSchemaRuntimeTable*, uint32_t, uint32_t, const char*, va_list );
        rc_t ( CC * srtt_v_add_ascii_column ) ( VSchemaRuntimeTable*, const char*, va_list );
        rc_t ( CC * srtt_v_add_unicode_column ) ( VSchemaRuntimeTable*, uint32_t, const char*, va_list );
        rc_t ( CC * tbl_add_ref ) ( const VTable* );
        rc_t ( CC * tbl_release ) ( const VTable* );
        rc_t ( CC * mgr_v_open_table_read ) ( struct VDBManager const*, const VTable**, struct VSchema const*, const char*, va_list );
        rc_t ( CC * db_v_open_table_read ) ( struct VDatabase const*, const VTable**, const char*, va_list );
        rc_t ( CC * tbl_typespec ) ( const VTable*, char*, size_t );
        bool ( CC * tbl_locked ) ( const VTable* );
        rc_t ( CC * tbl_v_writable ) ( const VTable*, uint32_t, const char*, va_list );
        rc_t ( CC * tbl_open_metadata_read ) ( const VTable*, struct KMetadata const** );
        rc_t ( CC * tbl_v_open_index_read ) ( const VTable*, const KIndex**, const char*, va_list );
        rc_t ( CC * tbl_list_readable_columns ) ( const VTable*, struct KNamelist** );
        rc_t ( CC * tbl_list_col ) ( const VTable*, struct KNamelist** );
        rc_t ( CC * tbl_list_readable_datatypes ) ( const VTable*, const char*, uint32_t*, struct KNamelist** );
        rc_t ( CC * tbl_column_datatypes ) ( const VTable*, const char*, uint32_t*, struct KNamelist** );
        rc_t ( CC * tbl_open_manager_read ) ( const VTable*, struct VDBManager const** );
        rc_t ( CC * tbl_open_parent_read ) ( const VTable*, struct VDatabase const** );
        rc_t ( CC * tbl_open_schema ) ( const VTable*, struct VSchema const** );
        rc_t ( CC * tbl_get_user_data ) ( const VTable*, void** );
        rc_t ( CC * tbl_set_user_data ) ( const VTable*, void*, void ( CC * ) ( void *data ) );
        rc_t ( CC * curs_add_ref ) ( const VCursor* );
        rc_t ( CC * curs_release ) ( const VCursor* );
        rc_t ( CC * tbl_create_cursor_read ) ( struct VTable const*, const VCursor** );
        rc_t ( CC * tbl_create_cached_cursor_read ) ( struct VTable const*, const VCursor**, size_t );
        rc_t ( CC * curs_v_add_column ) ( const VCursor*, uint32_t*, const char*, va_list );
        rc_t ( CC * curs_v_get_column_idx ) ( const VCursor*, uint32_t*, const char*, va_list );
        rc_t ( CC * curs_datatype ) ( const VCursor*, uint32_t, struct VTypedecl*, struct VTypedesc* );
        rc_t ( CC * curs_id_range ) ( const VCursor*, uint32_t, int64_t*, uint64_t* );
        rc_t ( CC * curs_open ) ( const VCursor* );
        rc_t ( CC * curs_row_id ) ( const VCursor*, int64_t* );
        rc_t ( CC * curs_set_row_id ) ( const VCursor*, int64_t );
        rc_t ( CC * curs_open_row ) ( const VCursor* );
        rc_t ( CC * curs_close_row ) ( const VCursor* );
        rc_t ( CC * curs_get_blob ) ( const VCursor*, struct VBlob const**, uint32_t );
        rc_t ( CC * curs_get_blob_direct ) ( const VCursor*, struct VBlob const**, int64_t, uint32_t );
        rc_t ( CC * curs_read ) ( const VCursor*, uint32_t, uint32_t, void*, uint32_t, uint32_t* );
        rc_t ( CC * curs_read_direct ) ( const VCursor*, int64_t, uint32_t, uint32_t, void*, uint32_t, uint32_t* );
        rc_t ( CC * curs_read_bits ) ( const VCursor*, uint32_t, uint32_t, uint32_t, void*, uint32_t, uint32_t, uint32_t*, uint32_t* );
        rc_t ( CC * curs_read_bits_direct ) ( const VCursor*, int64_t, uint32_t, uint32_t, uint32_t, void*, uint32_t, uint32_t, uint32_t*, uint32_t* );
        rc_t ( CC * curs_cell_data ) ( const VCursor*, uint32_t, uint32_t*, const void**, uint32_t*, uint32_t* );
        rc_t ( CC * curs_cell_data_direct ) ( const VCursor*, int64_t, uint32_t, uint32_t*, const void**, uint32_t*, uint32_t* );
        rc_t ( CC * curs_open_parent_read ) ( const VCursor*, struct VTable const** );
        rc_t ( CC * curs_get_user_data ) ( const VCursor*, void** );
        rc_t ( CC * curs_set_user_data ) ( const VCursor*, void*, void ( CC * ) ( void *data ) );
    } f;

} vdb_cvt;

static const char *vdb_wmsgs [] =
{
    "VDBManagerVLock",
    "VDBManagerVUnlock",
    "VDBManagerVDrop",
    "VDBManagerVCreateDB",
    "VDatabaseVCreateDB",
    "VDatabaseVDropDB",
    "VDatabaseVDropTable",
    "VDBManagerVOpenDBUpdate",
    "VDatabaseVOpenDBUpdate",
    "VDatabaseVLock",
    "VDatabaseVUnlock",
    "VDatabaseOpenMetadataUpdate",
    "VDatabaseColumnCreateParams",
    "VDatabaseOpenManagerUpdate",
    "VDatabaseOpenParentUpdate",
    "VDBManagerVCreateTable",
    "VDatabaseVCreateTable",
    "VDBManagerVOpenTableUpdate",
    "VDatabaseVOpenTableUpdate",
    "VTableVLock",
    "VTableVUnlock",
    "VTableOpenMetadataUpdate",
    "VTableColumnCreateParams",
    "VTableVCreateIndex",
    "VTableVOpenIndexUpdate",
    "VTableListWritableColumns",
    "VTableListWritableDatatypes",
    "VTableReindex",
    "VTableOpenManagerUpdate",
    "VTableOpenParentUpdate",
    "VTableCreateCursorWrite",
    "VCursorCommitRow",
    "VCursorRepeatRow",
    "VCursorFlushPage",
    "VCursorDefault",
    "VCursorWrite",
    "VCursorCommit",
    "VCursorOpenParentUpdate",
    NULL
};

static union
{
    fptr_t slots [ sizeof vdb_wmsgs / sizeof vdb_wmsgs [ 0 ] - 1 ];

    struct
    {
        rc_t ( CC * mgr_v_lock ) ( VDBManager*, const char*, va_list );
        rc_t ( CC * mgr_v_unlock ) ( VDBManager*, const char*, va_list );
        rc_t ( CC * mgr_v_drop ) ( VDBManager*, uint32_t, const char*, va_list );
        rc_t ( CC * mgr_v_create_db ) ( struct VDBManager*, VDatabase**, struct VSchema const*, const char*, KCreateMode, const char*, va_list );
        rc_t ( CC * db_v_create_db ) ( VDatabase*, VDatabase**, const char*, KCreateMode, const char*, va_list );
        rc_t ( CC * db_v_drop_db ) ( VDatabase*, const char*, va_list );
        rc_t ( CC * db_v_drop_table ) ( VDatabase*, const char*, va_list );
        rc_t ( CC * mgr_v_open_db_update ) ( struct VDBManager*, VDatabase**, struct VSchema const*, const char*, va_list );
        rc_t ( CC * db_v_open_db_update ) ( VDatabase*, VDatabase**, const char*, va_list );
        rc_t ( CC * db_v_lock ) ( VDatabase*, uint32_t, const char*, va_list );
        rc_t ( CC * db_v_unlock ) ( VDatabase*, uint32_t, const char*, va_list );
        rc_t ( CC * db_open_metadata_update ) ( VDatabase*, struct KMetadata** );
        rc_t ( CC * db_column_create_params ) ( VDatabase*, KCreateMode, KChecksum, size_t );
        rc_t ( CC * db_open_manager_update ) ( VDatabase*, struct VDBManager** );
        rc_t ( CC * db_open_parent_update ) ( VDatabase*, VDatabase** );
        rc_t ( CC * mgr_v_create_table ) ( struct VDBManager*, VTable**, struct VSchema const*, const char*, KCreateMode, const char*, va_list );
        rc_t ( CC * db_v_create_table ) ( struct VDatabase*, VTable**, const char*, KCreateMode, const char*, va_list );
        rc_t ( CC * mgr_v_open_table_update ) ( struct VDBManager*, VTable**, struct VSchema const*, const char*, va_list );
        rc_t ( CC * db_v_open_table_update ) ( struct VDatabase*, VTable**, const char*, va_list );
        rc_t ( CC * tbl_v_lock ) ( VTable*, uint32_t, const char*, va_list );
        rc_t ( CC * tbl_v_unlock ) ( VTable*, uint32_t, const char*, va_list );
        rc_t ( CC * tbl_open_metadata_update ) ( VTable*, struct KMetadata** );
        rc_t ( CC * tbl_column_create_params ) ( VTable*, KCreateMode, KChecksum, size_t );
        rc_t ( CC * tbl_v_create_index ) ( VTable*, KIndex**, KIdxType, KCreateMode, const char*, va_list );
        rc_t ( CC * tbl_v_open_index_update ) ( VTable*, KIndex**, const char*, va_list );
        rc_t ( CC * tbl_list_writable_columns ) ( VTable*, struct KNamelist** );
        rc_t ( CC * tbl_list_writable_datatypes ) ( VTable*, const char*, struct KNamelist** );
        rc_t ( CC * tbl_reindex ) ( VTable* );
        rc_t ( CC * tbl_open_manager_update ) ( VTable*, struct VDBManager** );
        rc_t ( CC * tbl_open_parent_update ) ( VTable*, struct VDatabase** );
        rc_t ( CC * tbl_create_cursor_write ) ( struct VTable*, VCursor**, KCreateMode );
        rc_t ( CC * curs_commit_row ) ( VCursor* );
        rc_t ( CC * curs_repeat_row ) ( VCursor*, uint64_t );
        rc_t ( CC * curs_flush_page ) ( VCursor* );
        rc_t ( CC * curs_default ) ( VCursor*, uint32_t, bitsz_t, const void*, bitsz_t, uint64_t );
        rc_t ( CC * curs_write ) ( VCursor*, uint32_t, bitsz_t, const void*, bitsz_t, uint64_t );
        rc_t ( CC * curs_commit ) ( VCursor* );
        rc_t ( CC * curs_open_parent_update ) ( VCursor*, struct VTable** );
    } f;

} vdb_wvt;

kqsh_libdata vdb_data =
{
    NULL, NULL,
    vdb_msgs, vdb_cvt . slots,
    vdb_wmsgs, vdb_wvt . slots
};

rc_t _VDBManagerMake ( VDBManager **mgr, struct KDirectory *wd )
{
    if ( sizeof vdb_cvt . slots != sizeof vdb_cvt . f ||
         sizeof vdb_wvt . slots != sizeof vdb_wvt . f )
    {
        * mgr = NULL;
        return RC ( rcExe, rcMgr, rcConstructing, rcInterface, rcCorrupt );
    }

    assert ( vdb_cvt . f . mgr_make != NULL );
    return ( * vdb_cvt . f . mgr_make ) ( mgr, wd );
}

rc_t _VDBManagerAddRef ( const VDBManager *self )
{
    assert ( vdb_cvt . f . mgr_add_ref != NULL );
    return ( * vdb_cvt . f . mgr_add_ref ) ( self );
}

rc_t _VDBManagerRelease ( const VDBManager *self )
{
    assert ( vdb_cvt . f . mgr_release != NULL );
    return ( * vdb_cvt . f . mgr_release ) ( self );
}

rc_t _VDBManagerVersion ( const VDBManager *self, uint32_t *version )
{
    assert ( vdb_cvt . f . mgr_version != NULL );
    return ( * vdb_cvt . f . mgr_version ) ( self, version );
}

rc_t _VDBManagerWritable ( const VDBManager *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_cvt . f . mgr_v_writable != NULL );
    ret = ( * vdb_cvt . f . mgr_v_writable ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVWritable ( const VDBManager *self, const char *path, va_list args )
{
    assert ( vdb_cvt . f . mgr_v_writable != NULL );
    return ( * vdb_cvt . f . mgr_v_writable ) ( self, path, args );
}

rc_t _VDBManagerLock ( VDBManager *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_wvt . f . mgr_v_lock != NULL );
    ret = ( * vdb_wvt . f . mgr_v_lock ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVLock ( VDBManager *self, const char *path, va_list args )
{
    assert ( vdb_wvt . f . mgr_v_lock != NULL );
    return ( * vdb_wvt . f . mgr_v_lock ) ( self, path, args );
}

rc_t _VDBManagerUnlock ( VDBManager *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_wvt . f . mgr_v_unlock != NULL );
    ret = ( * vdb_wvt . f . mgr_v_unlock ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVUnlock ( VDBManager *self, const char *path, va_list args )
{
    assert ( vdb_wvt . f . mgr_v_unlock != NULL );
    return ( * vdb_wvt . f . mgr_v_unlock ) ( self, path, args );
}

rc_t _VDBManagerDrop ( VDBManager *self, uint32_t obj_type, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_wvt . f . mgr_v_drop != NULL );
    ret = ( * vdb_wvt . f . mgr_v_drop ) ( self, obj_type, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVDrop ( VDBManager *self, uint32_t obj_type, const char *path, va_list args )
{
    assert ( vdb_wvt . f . mgr_v_drop != NULL );
    return ( * vdb_wvt . f . mgr_v_drop ) ( self, obj_type, path, args );
}

rc_t _VDBManagerAddSchemaIncludePath ( const VDBManager *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_cvt . f . mgr_v_add_schema_include_path != NULL );
    ret = ( * vdb_cvt . f . mgr_v_add_schema_include_path ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVAddSchemaIncludePath ( const VDBManager *self, const char *path, va_list args )
{
    assert ( vdb_cvt . f . mgr_v_add_schema_include_path != NULL );
    return ( * vdb_cvt . f . mgr_v_add_schema_include_path ) ( self, path, args );
}

rc_t _VDBManagerAddLoadLibraryPath ( const VDBManager *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_cvt . f . mgr_v_add_load_library_path != NULL );
    ret = ( * vdb_cvt . f . mgr_v_add_load_library_path ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVAddLoadLibraryPath ( const VDBManager *self, const char *path, va_list args )
{
    assert ( vdb_cvt . f . mgr_v_add_load_library_path != NULL );
    return ( * vdb_cvt . f . mgr_v_add_load_library_path ) ( self, path, args );
}

rc_t _VDBManagerGetUserData ( const VDBManager *self, void **data )
{
    assert ( vdb_cvt . f . mgr_get_user_data != NULL );
    return ( * vdb_cvt . f . mgr_get_user_data ) ( self, data );
}

rc_t _VDBManagerSetUserData ( const VDBManager *self, void *data, void ( CC * destroy ) ( void *data ) )
{
    assert ( vdb_cvt . f . mgr_set_user_data != NULL );
    return ( * vdb_cvt . f . mgr_set_user_data ) ( self, data, destroy );
}

rc_t _VDBManagerGetObjVersion ( const VDBManager *self, ver_t * version, const char *path )
{
    assert ( vdb_cvt . f . mgr_get_obj_version != NULL );
    return ( * vdb_cvt . f . mgr_get_obj_version ) ( self, version, path );
}

int _VDBManagerPathType ( const VDBManager * self, const char *path, ... )
{
    int ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_cvt . f . mgr_v_path_type != NULL );
    ret = ( * vdb_cvt . f . mgr_v_path_type ) ( self, path, args );
    va_end ( args );
    return ret;
}

int _VDBManagerVPathType ( const VDBManager * self, const char *path, va_list args )
{
    assert ( vdb_cvt . f . mgr_v_path_type != NULL );
    return ( * vdb_cvt . f . mgr_v_path_type ) ( self, path, args );
}

rc_t _VDatabaseAddRef ( const VDatabase *self )
{
    assert ( vdb_cvt . f . db_add_ref != NULL );
    return ( * vdb_cvt . f . db_add_ref ) ( self );
}

rc_t _VDatabaseRelease ( const VDatabase *self )
{
    assert ( vdb_cvt . f . db_release != NULL );
    return ( * vdb_cvt . f . db_release ) ( self );
}

rc_t _VDBManagerCreateDB ( struct VDBManager *self, VDatabase **db, struct VSchema const *schema, const char *typespec, KCreateMode cmode, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_wvt . f . mgr_v_create_db != NULL );
    ret = ( * vdb_wvt . f . mgr_v_create_db ) ( self, db, schema, typespec, cmode, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVCreateDB ( struct VDBManager *self, VDatabase **db, struct VSchema const *schema, const char *typespec, KCreateMode cmode, const char *path, va_list args )
{
    assert ( vdb_wvt . f . mgr_v_create_db != NULL );
    return ( * vdb_wvt . f . mgr_v_create_db ) ( self, db, schema, typespec, cmode, path, args );
}

rc_t _VDatabaseCreateDB ( VDatabase *self, VDatabase **db, const char *member, KCreateMode cmode, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . db_v_create_db != NULL );
    ret = ( * vdb_wvt . f . db_v_create_db ) ( self, db, member, cmode, name, args );
    va_end ( args );
    return ret;
}

rc_t _VDatabaseVCreateDB ( VDatabase *self, VDatabase **db, const char *member, KCreateMode cmode, const char *name, va_list args )
{
    assert ( vdb_wvt . f . db_v_create_db != NULL );
    return ( * vdb_wvt . f . db_v_create_db ) ( self, db, member, cmode, name, args );
}

rc_t _VDatabaseDropDB ( VDatabase *self, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . db_v_drop_db != NULL );
    ret = ( * vdb_wvt . f . db_v_drop_db ) ( self, name, args );
    va_end ( args );
    return ret;
}

rc_t _VDatabaseVDropDB ( VDatabase *self, const char *name, va_list args )
{
    assert ( vdb_wvt . f . db_v_drop_db != NULL );
    return ( * vdb_wvt . f . db_v_drop_db ) ( self, name, args );
}

rc_t _VDatabaseDropTable ( VDatabase *self, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . db_v_drop_table != NULL );
    ret = ( * vdb_wvt . f . db_v_drop_table ) ( self, name, args );
    va_end ( args );
    return ret;
}

rc_t _VDatabaseVDropTable ( VDatabase *self, const char *name, va_list args )
{
    assert ( vdb_wvt . f . db_v_drop_table != NULL );
    return ( * vdb_wvt . f . db_v_drop_table ) ( self, name, args );
}

rc_t _VDBManagerOpenDBRead ( struct VDBManager const *self, const VDatabase **db, struct VSchema const *schema, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_cvt . f . mgr_v_open_db_read != NULL );
    ret = ( * vdb_cvt . f . mgr_v_open_db_read ) ( self, db, schema, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVOpenDBRead ( struct VDBManager const *self, const VDatabase **db, struct VSchema const *schema, const char *path, va_list args )
{
    assert ( vdb_cvt . f . mgr_v_open_db_read != NULL );
    return ( * vdb_cvt . f . mgr_v_open_db_read ) ( self, db, schema, path, args );
}

rc_t _VDBManagerOpenDBUpdate ( struct VDBManager *self, VDatabase **db, struct VSchema const *schema, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_wvt . f . mgr_v_open_db_update != NULL );
    ret = ( * vdb_wvt . f . mgr_v_open_db_update ) ( self, db, schema, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVOpenDBUpdate ( struct VDBManager *self, VDatabase **db, struct VSchema const *schema, const char *path, va_list args )
{
    assert ( vdb_wvt . f . mgr_v_open_db_update != NULL );
    return ( * vdb_wvt . f . mgr_v_open_db_update ) ( self, db, schema, path, args );
}

rc_t _VDatabaseOpenDBRead ( const VDatabase *self, const VDatabase **db, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . db_v_open_db_read != NULL );
    ret = ( * vdb_cvt . f . db_v_open_db_read ) ( self, db, name, args );
    va_end ( args );
    return ret;
}

rc_t _VDatabaseVOpenDBRead ( const VDatabase *self, const VDatabase **db, const char *name, va_list args )
{
    assert ( vdb_cvt . f . db_v_open_db_read != NULL );
    return ( * vdb_cvt . f . db_v_open_db_read ) ( self, db, name, args );
}

rc_t _VDatabaseOpenDBUpdate ( VDatabase *self, VDatabase **db, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . db_v_open_db_update != NULL );
    ret = ( * vdb_wvt . f . db_v_open_db_update ) ( self, db, name, args );
    va_end ( args );
    return ret;
}

rc_t _VDatabaseVOpenDBUpdate ( VDatabase *self, VDatabase **db, const char *name, va_list args )
{
    assert ( vdb_wvt . f . db_v_open_db_update != NULL );
    return ( * vdb_wvt . f . db_v_open_db_update ) ( self, db, name, args );
}

rc_t _VDatabaseTypespec ( const VDatabase *self, char *ts_buff, size_t ts_size )
{
    assert ( vdb_cvt . f . db_typespec != NULL );
    return ( * vdb_cvt . f . db_typespec ) ( self, ts_buff, ts_size );
}

bool _VDatabaseLocked ( const VDatabase *self )
{
    assert ( vdb_cvt . f . db_locked != NULL );
    return ( * vdb_cvt . f . db_locked ) ( self );
}

bool _VDatabaseIsAlias ( const VDatabase *self, uint32_t type, char *resolved, size_t rsize, const char *name )
{
    assert ( vdb_cvt . f . db_is_alias != NULL );
    return ( * vdb_cvt . f . db_is_alias ) ( self, type, resolved, rsize, name );
}

rc_t _VDatabaseWritable ( const VDatabase *self, uint32_t type, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . db_v_writable != NULL );
    ret = ( * vdb_cvt . f . db_v_writable ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _VDatabaseVWritable ( const VDatabase *self, uint32_t type, const char *name, va_list args )
{
    assert ( vdb_cvt . f . db_v_writable != NULL );
    return ( * vdb_cvt . f . db_v_writable ) ( self, type, name, args );
}

rc_t _VDatabaseLock ( VDatabase *self, uint32_t type, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . db_v_lock != NULL );
    ret = ( * vdb_wvt . f . db_v_lock ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _VDatabaseVLock ( VDatabase *self, uint32_t type, const char *name, va_list args )
{
    assert ( vdb_wvt . f . db_v_lock != NULL );
    return ( * vdb_wvt . f . db_v_lock ) ( self, type, name, args );
}

rc_t _VDatabaseUnlock ( VDatabase *self, uint32_t type, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . db_v_unlock != NULL );
    ret = ( * vdb_wvt . f . db_v_unlock ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _VDatabaseVUnlock ( VDatabase *self, uint32_t type, const char *name, va_list args )
{
    assert ( vdb_wvt . f . db_v_unlock != NULL );
    return ( * vdb_wvt . f . db_v_unlock ) ( self, type, name, args );
}

rc_t _VDatabaseOpenMetadataRead ( const VDatabase *self, struct KMetadata const **meta )
{
    assert ( vdb_cvt . f . db_open_metadata_read != NULL );
    return ( * vdb_cvt . f . db_open_metadata_read ) ( self, meta );
}

rc_t _VDatabaseOpenMetadataUpdate ( VDatabase *self, struct KMetadata **meta )
{
    assert ( vdb_wvt . f . db_open_metadata_update != NULL );
    return ( * vdb_wvt . f . db_open_metadata_update ) ( self, meta );
}

rc_t _VDatabaseColumnCreateParams ( VDatabase *self, KCreateMode cmode, KChecksum checksum, size_t pgsize )
{
    assert ( vdb_wvt . f . db_column_create_params != NULL );
    return ( * vdb_wvt . f . db_column_create_params ) ( self, cmode, checksum, pgsize );
}

rc_t _VDatabaseOpenManagerRead ( const VDatabase *self, struct VDBManager const **mgr )
{
    assert ( vdb_cvt . f . db_open_manager_read != NULL );
    return ( * vdb_cvt . f . db_open_manager_read ) ( self, mgr );
}

rc_t _VDatabaseOpenManagerUpdate ( VDatabase *self, struct VDBManager **mgr )
{
    assert ( vdb_wvt . f . db_open_manager_update != NULL );
    return ( * vdb_wvt . f . db_open_manager_update ) ( self, mgr );
}

rc_t _VDatabaseOpenParentRead ( const VDatabase *self, const VDatabase **par )
{
    assert ( vdb_cvt . f . db_open_parent_read != NULL );
    return ( * vdb_cvt . f . db_open_parent_read ) ( self, par );
}

rc_t _VDatabaseOpenParentUpdate ( VDatabase *self, VDatabase **par )
{
    assert ( vdb_wvt . f . db_open_parent_update != NULL );
    return ( * vdb_wvt . f . db_open_parent_update ) ( self, par );
}

rc_t _VDatabaseOpenSchema ( const VDatabase *self, struct VSchema const **schema )
{
    assert ( vdb_cvt . f . db_open_schema != NULL );
    return ( * vdb_cvt . f . db_open_schema ) ( self, schema );
}

rc_t _VDatabaseListTbl ( const VDatabase *self, KNamelist **names )
{
    assert ( vdb_cvt . f . db_list_tbl != NULL );
    return ( * vdb_cvt . f . db_list_tbl ) ( self, names );
}

rc_t _VDatabaseListDB ( const VDatabase *self, KNamelist **names )
{
    assert ( vdb_cvt . f . db_list_db != NULL );
    return ( * vdb_cvt . f . db_list_db ) ( self, names );
}

rc_t _VDatabaseGetUserData ( const VDatabase *self, void **data )
{
    assert ( vdb_cvt . f . db_get_user_data != NULL );
    return ( * vdb_cvt . f . db_get_user_data ) ( self, data );
}

rc_t _VDatabaseSetUserData ( const VDatabase *self, void *data, void ( CC * destroy ) ( void *data ) )
{
    assert ( vdb_cvt . f . db_set_user_data != NULL );
    return ( * vdb_cvt . f . db_set_user_data ) ( self, data, destroy );
}

rc_t _VSchemaAddRef ( const VSchema *self )
{
    assert ( vdb_cvt . f . schema_add_ref != NULL );
    return ( * vdb_cvt . f . schema_add_ref ) ( self );
}

rc_t _VSchemaRelease ( const VSchema *self )
{
    assert ( vdb_cvt . f . schema_release != NULL );
    return ( * vdb_cvt . f . schema_release ) ( self );
}

rc_t _VDBManagerMakeSchema ( struct VDBManager const *self, VSchema **schema )
{
    assert ( vdb_cvt . f . mgr_make_schema != NULL );
    return ( * vdb_cvt . f . mgr_make_schema ) ( self, schema );
}

rc_t _VSchemaAddIncludePath ( VSchema *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_cvt . f . schema_v_add_include_path != NULL );
    ret = ( * vdb_cvt . f . schema_v_add_include_path ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _VSchemaVAddIncludePath ( VSchema *self, const char *path, va_list args )
{
    assert ( vdb_cvt . f . schema_v_add_include_path != NULL );
    return ( * vdb_cvt . f . schema_v_add_include_path ) ( self, path, args );
}

rc_t _VSchemaParseText ( VSchema *self, const char *name, const char *text, size_t bytes )
{
    assert ( vdb_cvt . f . schema_parse_text != NULL );
    return ( * vdb_cvt . f . schema_parse_text ) ( self, name, text, bytes );
}

rc_t _VSchemaParseFile ( VSchema *self, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . schema_v_parse_file != NULL );
    ret = ( * vdb_cvt . f . schema_v_parse_file ) ( self, name, args );
    va_end ( args );
    return ret;
}

rc_t _VSchemaVParseFile ( VSchema *self, const char *name, va_list args )
{
    assert ( vdb_cvt . f . schema_v_parse_file != NULL );
    return ( * vdb_cvt . f . schema_v_parse_file ) ( self, name, args );
}

rc_t _VSchemaDump ( const VSchema *self, uint32_t mode, const char *decl, rc_t ( CC * flush ) ( void *dst, const void *buffer, size_t bsize ), void *dst )
{
    assert ( vdb_cvt . f . schema_dump != NULL );
    return ( * vdb_cvt . f . schema_dump ) ( self, mode, decl, flush, dst );
}

rc_t _VSchemaIncludeFiles ( const VSchema *self, struct KNamelist const **list )
{
    assert ( vdb_cvt . f . schema_include_files != NULL );
    return ( * vdb_cvt . f . schema_include_files ) ( self, list );
}

rc_t _VSchemaResolveTypedecl ( const VSchema *self, VTypedecl *resolved, const char *typedecl, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, typedecl );
    assert ( vdb_cvt . f . schema_v_resolve_typedecl != NULL );
    ret = ( * vdb_cvt . f . schema_v_resolve_typedecl ) ( self, resolved, typedecl, args );
    va_end ( args );
    return ret;
}

rc_t _VSchemaVResolveTypedecl ( const VSchema *self, VTypedecl *resolved, const char *typedecl, va_list args )
{
    assert ( vdb_cvt . f . schema_v_resolve_typedecl != NULL );
    return ( * vdb_cvt . f . schema_v_resolve_typedecl ) ( self, resolved, typedecl, args );
}

rc_t _VTypedeclToText ( const VTypedecl *self, const VSchema *schema, char *buffer, size_t bsize )
{
    assert ( vdb_cvt . f . td_to_text != NULL );
    return ( * vdb_cvt . f . td_to_text ) ( self, schema, buffer, bsize );
}

bool _VTypedeclToSupertype ( const VTypedecl *self, const VSchema *schema, VTypedecl *cast )
{
    assert ( vdb_cvt . f . td_to_supertype != NULL );
    return ( * vdb_cvt . f . td_to_supertype ) ( self, schema, cast );
}

bool _VTypedeclToType ( const VTypedecl *self, const VSchema *schema,  uint32_t ancestor, VTypedecl *cast, uint32_t *distance )
{
    assert ( vdb_cvt . f . td_to_type != NULL );
    return ( * vdb_cvt . f . td_to_type ) ( self, schema, ancestor, cast, distance );
}

bool _VTypedeclToTypedecl ( const VTypedecl *self, const VSchema *schema, const VTypedecl *ancestor, VTypedecl *cast, uint32_t *distance )
{
    assert ( vdb_cvt . f . td_to_typedecl != NULL );
    return ( * vdb_cvt . f . td_to_typedecl ) ( self, schema, ancestor, cast, distance );
}

bool _VTypedeclCommonAncestor ( const VTypedecl *self, const VSchema *schema, const VTypedecl *peer, VTypedecl *ancestor, uint32_t *distance )
{
    assert ( vdb_cvt . f . td_common_ancestor != NULL );
    return ( * vdb_cvt . f . td_common_ancestor ) ( self, schema, peer, ancestor, distance );
}

uint32_t _VTypedescSizeof ( const VTypedesc *self )
{
    assert ( vdb_cvt . f . tdesc_sizeof != NULL );
    return ( * vdb_cvt . f . tdesc_sizeof ) ( self );
}

rc_t _VSchemaDescribeTypedecl ( const VSchema *self, VTypedesc *desc, const VTypedecl *td )
{
    assert ( vdb_cvt . f . schema_describe_typedecl != NULL );
    return ( * vdb_cvt . f . schema_describe_typedecl ) ( self, desc, td );
}

rc_t _VSchemaMakeRuntimeTable ( VSchema *self, VSchemaRuntimeTable **tbl, const char *type_name, const char *supertype_spec )
{
    assert ( vdb_cvt . f . schema_make_runtime_table != NULL );
    return ( * vdb_cvt . f . schema_make_runtime_table ) ( self, tbl, type_name, supertype_spec );
}

rc_t _VSchemaRuntimeTableClose ( VSchemaRuntimeTable *self )
{
    assert ( vdb_cvt . f . srtt_close != NULL );
    return ( * vdb_cvt . f . srtt_close ) ( self );
}

rc_t _VSchemaRuntimeTableCommit ( VSchemaRuntimeTable *self )
{
    assert ( vdb_cvt . f . srtt_commit != NULL );
    return ( * vdb_cvt . f . srtt_commit ) ( self );
}

rc_t _VSchemaRuntimeTableAddColumn ( VSchemaRuntimeTable *self, const VTypedecl *td, const char *encoding, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . srtt_v_add_column != NULL );
    ret = ( * vdb_cvt . f . srtt_v_add_column ) ( self, td, encoding, name, args );
    va_end ( args );
    return ret;
}

rc_t _VSchemaRuntimeTableVAddColumn ( VSchemaRuntimeTable *self, const VTypedecl *td, const char *encoding, const char *name, va_list args )
{
    assert ( vdb_cvt . f . srtt_v_add_column != NULL );
    return ( * vdb_cvt . f . srtt_v_add_column ) ( self, td, encoding, name, args );
}

rc_t _VSchemaRuntimeTableAddBooleanColumn ( VSchemaRuntimeTable *self, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . srtt_v_add_boolean_column != NULL );
    ret = ( * vdb_cvt . f . srtt_v_add_boolean_column ) ( self, name, args );
    va_end ( args );
    return ret;
}

rc_t _VSchemaRuntimeTableAddIntegerColumn ( VSchemaRuntimeTable *self, uint32_t bits, bool has_sign,  const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . srtt_v_add_integer_column != NULL );
    ret = ( * vdb_cvt . f . srtt_v_add_integer_column ) ( self, bits, has_sign, name, args );
    va_end ( args );
    return ret;
}

rc_t _VSchemaRuntimeTableAddFloatColumn ( VSchemaRuntimeTable *self, uint32_t bits, uint32_t significant_mantissa_bits,  const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . srtt_v_add_float_column != NULL );
    ret = ( * vdb_cvt . f . srtt_v_add_float_column ) ( self, bits, significant_mantissa_bits, name, args );
    va_end ( args );
    return ret;
}

rc_t _VSchemaRuntimeTableAddAsciiColumn ( VSchemaRuntimeTable *self, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . srtt_v_add_ascii_column != NULL );
    ret = ( * vdb_cvt . f . srtt_v_add_ascii_column ) ( self, name, args );
    va_end ( args );
    return ret;
}

rc_t _VSchemaRuntimeTableAddUnicodeColumn ( VSchemaRuntimeTable *self, uint32_t bits, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . srtt_v_add_unicode_column != NULL );
    ret = ( * vdb_cvt . f . srtt_v_add_unicode_column ) ( self, bits, name, args );
    va_end ( args );
    return ret;
}

rc_t _VSchemaRuntimeTableVAddBooleanColumn ( VSchemaRuntimeTable *self, const char *name, va_list args )
{
    assert ( vdb_cvt . f . srtt_v_add_boolean_column != NULL );
    return ( * vdb_cvt . f . srtt_v_add_boolean_column ) ( self, name, args );
}

rc_t _VSchemaRuntimeTableVAddIntegerColumn ( VSchemaRuntimeTable *self, uint32_t bits, bool has_sign,  const char *name, va_list args )
{
    assert ( vdb_cvt . f . srtt_v_add_integer_column != NULL );
    return ( * vdb_cvt . f . srtt_v_add_integer_column ) ( self, bits, has_sign, name, args );
}

rc_t _VSchemaRuntimeTableVAddFloatColumn ( VSchemaRuntimeTable *self, uint32_t bits, uint32_t significant_mantissa_bits,  const char *name, va_list args )
{
    assert ( vdb_cvt . f . srtt_v_add_float_column != NULL );
    return ( * vdb_cvt . f . srtt_v_add_float_column ) ( self, bits, significant_mantissa_bits, name, args );
}

rc_t _VSchemaRuntimeTableVAddAsciiColumn ( VSchemaRuntimeTable *self, const char *name, va_list args )
{
    assert ( vdb_cvt . f . srtt_v_add_ascii_column != NULL );
    return ( * vdb_cvt . f . srtt_v_add_ascii_column ) ( self, name, args );
}

rc_t _VSchemaRuntimeTableVAddUnicodeColumn ( VSchemaRuntimeTable *self, uint32_t bits, const char *name, va_list args )
{
    assert ( vdb_cvt . f . srtt_v_add_unicode_column != NULL );
    return ( * vdb_cvt . f . srtt_v_add_unicode_column ) ( self, bits, name, args );
}

rc_t _VTableAddRef ( const VTable *self )
{
    assert ( vdb_cvt . f . tbl_add_ref != NULL );
    return ( * vdb_cvt . f . tbl_add_ref ) ( self );
}

rc_t _VTableRelease ( const VTable *self )
{
    assert ( vdb_cvt . f . tbl_release != NULL );
    return ( * vdb_cvt . f . tbl_release ) ( self );
}

rc_t _VDBManagerCreateTable ( struct VDBManager *self, VTable **tbl, struct VSchema const *schema, const char *typespec, KCreateMode cmode, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_wvt . f . mgr_v_create_table != NULL );
    ret = ( * vdb_wvt . f . mgr_v_create_table ) ( self, tbl, schema, typespec, cmode, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVCreateTable ( struct VDBManager *self, VTable **tbl, struct VSchema const *schema, const char *typespec, KCreateMode cmode, const char *path, va_list args )
{
    assert ( vdb_wvt . f . mgr_v_create_table != NULL );
    return ( * vdb_wvt . f . mgr_v_create_table ) ( self, tbl, schema, typespec, cmode, path, args );
}

rc_t _VDatabaseCreateTable ( struct VDatabase *self, VTable **tbl, const char *member, KCreateMode cmode, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . db_v_create_table != NULL );
    ret = ( * vdb_wvt . f . db_v_create_table ) ( self, tbl, member, cmode, name, args );
    va_end ( args );
    return ret;
}

rc_t _VDatabaseVCreateTable ( struct VDatabase *self, VTable **tbl, const char *member, KCreateMode cmode, const char *name, va_list args )
{
    assert ( vdb_wvt . f . db_v_create_table != NULL );
    return ( * vdb_wvt . f . db_v_create_table ) ( self, tbl, member, cmode, name, args );
}

rc_t _VDBManagerOpenTableRead ( struct VDBManager const *self, const VTable **tbl, struct VSchema const *schema, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_cvt . f . mgr_v_open_table_read != NULL );
    ret = ( * vdb_cvt . f . mgr_v_open_table_read ) ( self, tbl, schema, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVOpenTableRead ( struct VDBManager const *self, const VTable **tbl, struct VSchema const *schema, const char *path, va_list args )
{
    assert ( vdb_cvt . f . mgr_v_open_table_read != NULL );
    return ( * vdb_cvt . f . mgr_v_open_table_read ) ( self, tbl, schema, path, args );
}

rc_t _VDBManagerOpenTableUpdate ( struct VDBManager *self, VTable **tbl, struct VSchema const *schema, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( vdb_wvt . f . mgr_v_open_table_update != NULL );
    ret = ( * vdb_wvt . f . mgr_v_open_table_update ) ( self, tbl, schema, path, args );
    va_end ( args );
    return ret;
}

rc_t _VDBManagerVOpenTableUpdate ( struct VDBManager *self, VTable **tbl, struct VSchema const *schema, const char *path, va_list args )
{
    assert ( vdb_wvt . f . mgr_v_open_table_update != NULL );
    return ( * vdb_wvt . f . mgr_v_open_table_update ) ( self, tbl, schema, path, args );
}

rc_t _VDatabaseOpenTableRead ( struct VDatabase const *self, const VTable **tbl, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . db_v_open_table_read != NULL );
    ret = ( * vdb_cvt . f . db_v_open_table_read ) ( self, tbl, name, args );
    va_end ( args );
    return ret;
}

rc_t _VDatabaseVOpenTableRead ( struct VDatabase const *self, const VTable **tbl, const char *name, va_list args )
{
    assert ( vdb_cvt . f . db_v_open_table_read != NULL );
    return ( * vdb_cvt . f . db_v_open_table_read ) ( self, tbl, name, args );
}

rc_t _VDatabaseOpenTableUpdate ( struct VDatabase *self, VTable **tbl, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . db_v_open_table_update != NULL );
    ret = ( * vdb_wvt . f . db_v_open_table_update ) ( self, tbl, name, args );
    va_end ( args );
    return ret;
}

rc_t _VDatabaseVOpenTableUpdate ( struct VDatabase *self, VTable **tbl, const char *name, va_list args )
{
    assert ( vdb_wvt . f . db_v_open_table_update != NULL );
    return ( * vdb_wvt . f . db_v_open_table_update ) ( self, tbl, name, args );
}

rc_t _VTableTypespec ( const VTable *self, char *ts_buff, size_t ts_size )
{
    assert ( vdb_cvt . f . tbl_typespec != NULL );
    return ( * vdb_cvt . f . tbl_typespec ) ( self, ts_buff, ts_size );
}

bool _VTableLocked ( const VTable *self )
{
    assert ( vdb_cvt . f . tbl_locked != NULL );
    return ( * vdb_cvt . f . tbl_locked ) ( self );
}

rc_t _VTableWritable ( const VTable *self, uint32_t type, const char * name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . tbl_v_writable != NULL );
    ret = ( * vdb_cvt . f . tbl_v_writable ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _VTableVWritable ( const VTable *self, uint32_t type, const char * name, va_list args )
{
    assert ( vdb_cvt . f . tbl_v_writable != NULL );
    return ( * vdb_cvt . f . tbl_v_writable ) ( self, type, name, args );
}

rc_t _VTableLock ( VTable *self, uint32_t type, const char * name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . tbl_v_lock != NULL );
    ret = ( * vdb_wvt . f . tbl_v_lock ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _VTableVLock ( VTable *self, uint32_t type, const char * name, va_list args )
{
    assert ( vdb_wvt . f . tbl_v_lock != NULL );
    return ( * vdb_wvt . f . tbl_v_lock ) ( self, type, name, args );
}

rc_t _VTableUnlock ( VTable *self, uint32_t type, const char * name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . tbl_v_unlock != NULL );
    ret = ( * vdb_wvt . f . tbl_v_unlock ) ( self, type, name, args );
    va_end ( args );
    return ret;
}

rc_t _VTableVUnlock ( VTable *self, uint32_t type, const char * name, va_list args )
{
    assert ( vdb_wvt . f . tbl_v_unlock != NULL );
    return ( * vdb_wvt . f . tbl_v_unlock ) ( self, type, name, args );
}

rc_t _VTableOpenMetadataRead ( const VTable *self, struct KMetadata const **meta )
{
    assert ( vdb_cvt . f . tbl_open_metadata_read != NULL );
    return ( * vdb_cvt . f . tbl_open_metadata_read ) ( self, meta );
}

rc_t _VTableOpenMetadataUpdate ( VTable *self, struct KMetadata **meta )
{
    assert ( vdb_wvt . f . tbl_open_metadata_update != NULL );
    return ( * vdb_wvt . f . tbl_open_metadata_update ) ( self, meta );
}

rc_t _VTableColumnCreateParams ( VTable *self, KCreateMode cmode, KChecksum checksum, size_t pgsize )
{
    assert ( vdb_wvt . f . tbl_column_create_params != NULL );
    return ( * vdb_wvt . f . tbl_column_create_params ) ( self, cmode, checksum, pgsize );
}

rc_t _VTableCreateIndex ( VTable *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . tbl_v_create_index != NULL );
    ret = ( * vdb_wvt . f . tbl_v_create_index ) ( self, idx, type, cmode, name, args );
    va_end ( args );
    return ret;
}

rc_t _VTableVCreateIndex ( VTable *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, va_list args )
{
    assert ( vdb_wvt . f . tbl_v_create_index != NULL );
    return ( * vdb_wvt . f . tbl_v_create_index ) ( self, idx, type, cmode, name, args );
}

rc_t _VTableOpenIndexRead ( const VTable *self, const KIndex **idx, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . tbl_v_open_index_read != NULL );
    ret = ( * vdb_cvt . f . tbl_v_open_index_read ) ( self, idx, name, args );
    va_end ( args );
    return ret;
}

rc_t _VTableVOpenIndexRead ( const VTable *self, const KIndex **idx, const char *name, va_list args )
{
    assert ( vdb_cvt . f . tbl_v_open_index_read != NULL );
    return ( * vdb_cvt . f . tbl_v_open_index_read ) ( self, idx, name, args );
}

rc_t _VTableOpenIndexUpdate ( VTable *self, KIndex **idx, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_wvt . f . tbl_v_open_index_update != NULL );
    ret = ( * vdb_wvt . f . tbl_v_open_index_update ) ( self, idx, name, args );
    va_end ( args );
    return ret;
}

rc_t _VTableVOpenIndexUpdate ( VTable *self, KIndex **idx, const char *name, va_list args )
{
    assert ( vdb_wvt . f . tbl_v_open_index_update != NULL );
    return ( * vdb_wvt . f . tbl_v_open_index_update ) ( self, idx, name, args );
}

rc_t _VTableListReadableColumns ( const VTable *self, struct KNamelist **names )
{
    assert ( vdb_cvt . f . tbl_list_readable_columns != NULL );
    return ( * vdb_cvt . f . tbl_list_readable_columns ) ( self, names );
}

rc_t _VTableListWritableColumns ( VTable *self, struct KNamelist **names )
{
    assert ( vdb_wvt . f . tbl_list_writable_columns != NULL );
    return ( * vdb_wvt . f . tbl_list_writable_columns ) ( self, names );
}

rc_t _VTableListCol ( const VTable *self, struct KNamelist **names )
{
    assert ( vdb_cvt . f . tbl_list_col != NULL );
    return ( * vdb_cvt . f . tbl_list_col ) ( self, names );
}

rc_t _VTableListReadableDatatypes ( const VTable *self, const char *col, uint32_t *dflt_idx, struct KNamelist **typedecls )
{
    assert ( vdb_cvt . f . tbl_list_readable_datatypes != NULL );
    return ( * vdb_cvt . f . tbl_list_readable_datatypes ) ( self, col, dflt_idx, typedecls );
}

rc_t _VTableListWritableDatatypes ( VTable *self, const char *col, struct KNamelist **typedecls )
{
    assert ( vdb_wvt . f . tbl_list_writable_datatypes != NULL );
    return ( * vdb_wvt . f . tbl_list_writable_datatypes ) ( self, col, typedecls );
}

rc_t _VTableColumnDatatypes ( const VTable *self, const char *col, uint32_t *dflt_idx, struct KNamelist **typedecls )
{
    assert ( vdb_cvt . f . tbl_column_datatypes != NULL );
    return ( * vdb_cvt . f . tbl_column_datatypes ) ( self, col, dflt_idx, typedecls );
}

rc_t _VTableReindex ( VTable *self )
{
    assert ( vdb_wvt . f . tbl_reindex != NULL );
    return ( * vdb_wvt . f . tbl_reindex ) ( self );
}

rc_t _VTableOpenManagerRead ( const VTable *self, struct VDBManager const **mgr )
{
    assert ( vdb_cvt . f . tbl_open_manager_read != NULL );
    return ( * vdb_cvt . f . tbl_open_manager_read ) ( self, mgr );
}

rc_t _VTableOpenManagerUpdate ( VTable *self, struct VDBManager **mgr )
{
    assert ( vdb_wvt . f . tbl_open_manager_update != NULL );
    return ( * vdb_wvt . f . tbl_open_manager_update ) ( self, mgr );
}

rc_t _VTableOpenParentRead ( const VTable *self, struct VDatabase const **db )
{
    assert ( vdb_cvt . f . tbl_open_parent_read != NULL );
    return ( * vdb_cvt . f . tbl_open_parent_read ) ( self, db );
}

rc_t _VTableOpenParentUpdate ( VTable *self, struct VDatabase **db )
{
    assert ( vdb_wvt . f . tbl_open_parent_update != NULL );
    return ( * vdb_wvt . f . tbl_open_parent_update ) ( self, db );
}

rc_t _VTableOpenSchema ( const VTable *self, struct VSchema const **schema )
{
    assert ( vdb_cvt . f . tbl_open_schema != NULL );
    return ( * vdb_cvt . f . tbl_open_schema ) ( self, schema );
}

rc_t _VTableGetUserData ( const VTable *self, void **data )
{
    assert ( vdb_cvt . f . tbl_get_user_data != NULL );
    return ( * vdb_cvt . f . tbl_get_user_data ) ( self, data );
}

rc_t _VTableSetUserData ( const VTable *self, void *data, void ( CC * destroy ) ( void *data ) )
{
    assert ( vdb_cvt . f . tbl_set_user_data != NULL );
    return ( * vdb_cvt . f . tbl_set_user_data ) ( self, data, destroy );
}

rc_t _VCursorAddRef ( const VCursor *self )
{
    assert ( vdb_cvt . f . curs_add_ref != NULL );
    return ( * vdb_cvt . f . curs_add_ref ) ( self );
}

rc_t _VCursorRelease ( const VCursor *self )
{
    assert ( vdb_cvt . f . curs_release != NULL );
    return ( * vdb_cvt . f . curs_release ) ( self );
}

rc_t _VTableCreateCursorRead ( struct VTable const *self, const VCursor **curs )
{
    assert ( vdb_cvt . f . tbl_create_cursor_read != NULL );
    return ( * vdb_cvt . f . tbl_create_cursor_read ) ( self, curs );
}

rc_t _VTableCreateCursorWrite ( struct VTable *self, VCursor **curs, KCreateMode mode )
{
    assert ( vdb_wvt . f . tbl_create_cursor_write != NULL );
    return ( * vdb_wvt . f . tbl_create_cursor_write ) ( self, curs, mode );
}

rc_t _VTableCreateCachedCursorRead ( struct VTable const *self, const VCursor **curs, size_t capacity )
{
    assert ( vdb_cvt . f . tbl_create_cached_cursor_read != NULL );
    return ( * vdb_cvt . f . tbl_create_cached_cursor_read ) ( self, curs, capacity );
}

rc_t _VCursorAddColumn ( const VCursor *self, uint32_t *idx, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . curs_v_add_column != NULL );
    ret = ( * vdb_cvt . f . curs_v_add_column ) ( self, idx, name, args );
    va_end ( args );
    return ret;
}

rc_t _VCursorVAddColumn ( const VCursor *self, uint32_t *idx, const char *name, va_list args )
{
    assert ( vdb_cvt . f . curs_v_add_column != NULL );
    return ( * vdb_cvt . f . curs_v_add_column ) ( self, idx, name, args );
}

rc_t _VCursorGetColumnIdx ( const VCursor *self, uint32_t *idx, const char *name, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, name );
    assert ( vdb_cvt . f . curs_v_get_column_idx != NULL );
    ret = ( * vdb_cvt . f . curs_v_get_column_idx ) ( self, idx, name, args );
    va_end ( args );
    return ret;
}

rc_t _VCursorVGetColumnIdx ( const VCursor *self, uint32_t *idx, const char *name, va_list args )
{
    assert ( vdb_cvt . f . curs_v_get_column_idx != NULL );
    return ( * vdb_cvt . f . curs_v_get_column_idx ) ( self, idx, name, args );
}

rc_t _VCursorDatatype ( const VCursor *self, uint32_t idx, struct VTypedecl *type, struct VTypedesc *desc )
{
    assert ( vdb_cvt . f . curs_datatype != NULL );
    return ( * vdb_cvt . f . curs_datatype ) ( self, idx, type, desc );
}

rc_t _VCursorIdRange ( const VCursor *self, uint32_t idx, int64_t *first, uint64_t *count )
{
    assert ( vdb_cvt . f . curs_id_range != NULL );
    return ( * vdb_cvt . f . curs_id_range ) ( self, idx, first, count );
}

rc_t _VCursorOpen ( const VCursor *self )
{
    assert ( vdb_cvt . f . curs_open != NULL );
    return ( * vdb_cvt . f . curs_open ) ( self );
}

rc_t _VCursorRowId ( const VCursor *self, int64_t *row_id )
{
    assert ( vdb_cvt . f . curs_row_id != NULL );
    return ( * vdb_cvt . f . curs_row_id ) ( self, row_id );
}

rc_t _VCursorSetRowId ( const VCursor *self, int64_t row_id )
{
    assert ( vdb_cvt . f . curs_set_row_id != NULL );
    return ( * vdb_cvt . f . curs_set_row_id ) ( self, row_id );
}

rc_t _VCursorOpenRow ( const VCursor *self )
{
    assert ( vdb_cvt . f . curs_open_row != NULL );
    return ( * vdb_cvt . f . curs_open_row ) ( self );
}

rc_t _VCursorCommitRow ( VCursor *self )
{
    assert ( vdb_wvt . f . curs_commit_row != NULL );
    return ( * vdb_wvt . f . curs_commit_row ) ( self );
}

rc_t _VCursorRepeatRow ( VCursor *self, uint64_t count )
{
    assert ( vdb_wvt . f . curs_repeat_row != NULL );
    return ( * vdb_wvt . f . curs_repeat_row ) ( self, count );
}

rc_t _VCursorCloseRow ( const VCursor *self )
{
    assert ( vdb_cvt . f . curs_close_row != NULL );
    return ( * vdb_cvt . f . curs_close_row ) ( self );
}

rc_t _VCursorFlushPage ( VCursor *self )
{
    assert ( vdb_wvt . f . curs_flush_page != NULL );
    return ( * vdb_wvt . f . curs_flush_page ) ( self );
}

rc_t _VCursorGetBlob ( const VCursor *self, struct VBlob const **blob, uint32_t col_idx )
{
    assert ( vdb_cvt . f . curs_get_blob != NULL );
    return ( * vdb_cvt . f . curs_get_blob ) ( self, blob, col_idx );
}

rc_t _VCursorGetBlobDirect ( const VCursor *self, struct VBlob const **blob, int64_t row_id, uint32_t col_idx )
{
    assert ( vdb_cvt . f . curs_get_blob_direct != NULL );
    return ( * vdb_cvt . f . curs_get_blob_direct ) ( self, blob, row_id, col_idx );
}

rc_t _VCursorRead ( const VCursor *self, uint32_t col_idx, uint32_t elem_bits, void *buffer, uint32_t blen, uint32_t *row_len )
{
    assert ( vdb_cvt . f . curs_read != NULL );
    return ( * vdb_cvt . f . curs_read ) ( self, col_idx, elem_bits, buffer, blen, row_len );
}

rc_t _VCursorReadDirect ( const VCursor *self, int64_t row_id, uint32_t col_idx, uint32_t elem_bits, void *buffer, uint32_t blen, uint32_t *row_len )
{
    assert ( vdb_cvt . f . curs_read_direct != NULL );
    return ( * vdb_cvt . f . curs_read_direct ) ( self, row_id, col_idx, elem_bits, buffer, blen, row_len );
}

rc_t _VCursorReadBits ( const VCursor *self, uint32_t col_idx, uint32_t elem_bits, uint32_t start, void *buffer, uint32_t boff, uint32_t blen, uint32_t *num_read, uint32_t *remaining )
{
    assert ( vdb_cvt . f . curs_read_bits != NULL );
    return ( * vdb_cvt . f . curs_read_bits ) ( self, col_idx, elem_bits, start, buffer, boff, blen, num_read, remaining );
}

rc_t _VCursorReadBitsDirect ( const VCursor *self, int64_t row_id, uint32_t col_idx, uint32_t elem_bits, uint32_t start, void *buffer, uint32_t boff, uint32_t blen, uint32_t *num_read, uint32_t *remaining )
{
    assert ( vdb_cvt . f . curs_read_bits_direct != NULL );
    return ( * vdb_cvt . f . curs_read_bits_direct ) ( self, row_id, col_idx, elem_bits, start, buffer, boff, blen, num_read, remaining );
}

rc_t _VCursorCellData ( const VCursor *self, uint32_t col_idx, uint32_t *elem_bits, const void **base, uint32_t *boff, uint32_t *row_len )
{
    assert ( vdb_cvt . f . curs_cell_data != NULL );
    return ( * vdb_cvt . f . curs_cell_data ) ( self, col_idx, elem_bits, base, boff, row_len );
}

rc_t _VCursorCellDataDirect ( const VCursor *self, int64_t row_id, uint32_t col_idx, uint32_t *elem_bits, const void **base, uint32_t *boff, uint32_t *row_len )
{
    assert ( vdb_cvt . f . curs_cell_data_direct != NULL );
    return ( * vdb_cvt . f . curs_cell_data_direct ) ( self, row_id, col_idx, elem_bits, base, boff, row_len );
}

rc_t _VCursorDefault ( VCursor *self, uint32_t col_idx, bitsz_t elem_bits, const void *buffer, bitsz_t boff, uint64_t row_len )
{
    assert ( vdb_wvt . f . curs_default != NULL );
    return ( * vdb_wvt . f . curs_default ) ( self, col_idx, elem_bits, buffer, boff, row_len );
}

rc_t _VCursorWrite ( VCursor *self, uint32_t col_idx, bitsz_t elem_bits, const void *buffer, bitsz_t boff, uint64_t count )
{
    assert ( vdb_wvt . f . curs_write != NULL );
    return ( * vdb_wvt . f . curs_write ) ( self, col_idx, elem_bits, buffer, boff, count );
}

rc_t _VCursorCommit ( VCursor *self )
{
    assert ( vdb_wvt . f . curs_commit != NULL );
    return ( * vdb_wvt . f . curs_commit ) ( self );
}

rc_t _VCursorOpenParentRead ( const VCursor *self, struct VTable const **tbl )
{
    assert ( vdb_cvt . f . curs_open_parent_read != NULL );
    return ( * vdb_cvt . f . curs_open_parent_read ) ( self, tbl );
}

rc_t _VCursorOpenParentUpdate ( VCursor *self, struct VTable **tbl )
{
    assert ( vdb_wvt . f . curs_open_parent_update != NULL );
    return ( * vdb_wvt . f . curs_open_parent_update ) ( self, tbl );
}

rc_t _VCursorGetUserData ( const VCursor *self, void **data )
{
    assert ( vdb_cvt . f . curs_get_user_data != NULL );
    return ( * vdb_cvt . f . curs_get_user_data ) ( self, data );
}

rc_t _VCursorSetUserData ( const VCursor *self, void *data, void ( CC * destroy ) ( void *data ) )
{
    assert ( vdb_cvt . f . curs_set_user_data != NULL );
    return ( * vdb_cvt . f . curs_set_user_data ) ( self, data, destroy );
}

