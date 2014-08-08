/* THIS IS AN AUTO-GENERATED FILE - DO NOT EDIT */

#include "kqsh-sra.h"
#include "kqsh-priv.h"
#include <klib/rc.h>
#include <assert.h>

static const char *sra_msgs [] =
{
    "SRAMgrMakeReadWithDir",
    "SRAMgrMakeUpdate",
    "SRAMgrMakeReadWithDir",
    "SRAMgrRelease",
    "SRAMgrAddRef",
    "SRAMgrVersion",
    "SRAMgrOpenDatatypesRead",
    "SRAMgrGetSchemaRead",
    "SRAMgrUseSchemaRead",
    "SRAMgrVWritable",
    "SRANamelistAddRef",
    "SRANamelistRelease",
    "SRANamelistCount",
    "SRANamelistGet",
    "SRATableAddRef",
    "SRATableRelease",
    "SRAMgrVOpenTableRead",
    "SRATableLocked",
    "SRATableBaseCount",
    "SRATableSpotCount",
    "SRATableMinSpotId",
    "SRATableMaxSpotId",
    "SRATableGetSpotId",
    "SRATableListCol",
    "SRATableColDatatypes",
    "SRATableMetaRevision",
    "SRATableMaxMetaRevision",
    "SRATableUseMetaRevision",
    "SRATableVOpenMDataNodeRead",
    "SRATableGetSchema",
    "SRAColumnAddRef",
    "SRAColumnRelease",
    "SRATableOpenColumnRead",
    "SRAColumnDatatype",
    "SRAColumnGetRange",
    "SRAColumnRead",
    "SRAMgrMakeSRASchema",
    "VDBManagerMakeSRASchema",
    NULL
};

static union
{
    fptr_t slots [ sizeof sra_msgs / sizeof sra_msgs [ 0 ] - 2 ];

    struct
    {
        rc_t ( CC * mgr_make ) ( SRAMgr**, struct KDirectory* );
        rc_t ( CC * mgr_make_read_with_dir ) ( const SRAMgr**, struct KDirectory const* );
        rc_t ( CC * mgr_release ) ( const SRAMgr* );
        rc_t ( CC * mgr_add_ref ) ( const SRAMgr* );
        rc_t ( CC * mgr_version ) ( const SRAMgr*, uint32_t* );
        rc_t ( CC * mgr_open_datatypes_read ) ( const SRAMgr*, struct VDatatypes const** );
        rc_t ( CC * mgr_get_schema_read ) ( const SRAMgr*, struct VSchema const** );
        rc_t ( CC * mgr_use_schema_read ) ( const SRAMgr*, struct VSchema const* );
        rc_t ( CC * mgr_v_writable ) ( const SRAMgr*, const char*, va_list );
        rc_t ( CC * nmlist_add_ref ) ( const SRANamelist* );
        rc_t ( CC * nmlist_release ) ( const SRANamelist* );
        rc_t ( CC * nmlist_count ) ( const SRANamelist*, uint32_t* );
        rc_t ( CC * nmlist_get ) ( const SRANamelist*, uint32_t, const char** );
        rc_t ( CC * tbl_add_ref ) ( const SRATable* );
        rc_t ( CC * tbl_release ) ( const SRATable* );
        rc_t ( CC * mgr_v_open_table_read ) ( const SRAMgr*, const SRATable**, const char*, va_list );
        bool ( CC * tbl_locked ) ( const SRATable* );
        rc_t ( CC * tbl_base_count ) ( const SRATable*, uint64_t* );
        rc_t ( CC * tbl_spot_count ) ( const SRATable*, uint64_t* );
        rc_t ( CC * tbl_min_spot_id ) ( const SRATable*, spotid_t* );
        rc_t ( CC * tbl_max_spot_id ) ( const SRATable*, spotid_t* );
        rc_t ( CC * tbl_get_spot_id ) ( const SRATable*, spotid_t*, const char* );
        rc_t ( CC * tbl_list_col ) ( const SRATable*, SRANamelist** );
        rc_t ( CC * tbl_col_datatypes ) ( const SRATable*, const char*, uint32_t*, SRANamelist** );
        rc_t ( CC * tbl_meta_revision ) ( const SRATable*, uint32_t* );
        rc_t ( CC * tbl_max_meta_revision ) ( const SRATable*, uint32_t* );
        rc_t ( CC * tbl_use_meta_revision ) ( const SRATable*, uint32_t );
        rc_t ( CC * tbl_v_open_m_data_node_read ) ( const SRATable*, struct KMDataNode const**, const char*, va_list );
        struct VSchema const* ( CC * tbl_get_schema ) ( const SRATable* );
        rc_t ( CC * col_add_ref ) ( const SRAColumn* );
        rc_t ( CC * col_release ) ( const SRAColumn* );
        rc_t ( CC * tbl_open_column_read ) ( const SRATable*, const SRAColumn**, const char*, const char* );
        rc_t ( CC * col_datatype ) ( const SRAColumn*, struct VTypedecl*, struct VTypedef* );
        rc_t ( CC * col_get_range ) ( const SRAColumn*, spotid_t, spotid_t*, spotid_t* );
        rc_t ( CC * col_read ) ( const SRAColumn*, spotid_t, const void**, bitsz_t*, bitsz_t* );
        rc_t ( CC * mgr_make_sra_schema ) ( struct SRAMgr const*, struct VSchema** );
        rc_t ( CC * vmgr_make_sra_schema ) ( struct VDBManager const*, struct VSchema** );
    } f;

} sra_cvt;

static const char *sra_wmsgs [] =
{
    "SRAMgrSetMD5Mode",
    "SRAMgrVLock",
    "SRAMgrVUnlock",
    "SRAMgrVDropTable",
    "SRAMgrVCreateTable",
    "SRAMgrVOpenTableUpdate",
    "SRATableNewSpot",
    "SRATableOpenSpot",
    "SRATableCloseSpot",
    "SRATableCommit",
    "SRATableOpenColumnWrite",
    "SRATableSetIdxColumnDefault",
    "SRATableWriteIdxColumn",
    "SRATableMetaFreeze",
    "SRATableVOpenMDataNodeUpdate",
    NULL
};

static union
{
    fptr_t slots [ sizeof sra_wmsgs / sizeof sra_wmsgs [ 0 ] - 1 ];

    struct
    {
        rc_t ( CC * mgr_set_m_d5_mode ) ( SRAMgr*, bool );
        rc_t ( CC * mgr_v_lock ) ( SRAMgr*, const char*, va_list );
        rc_t ( CC * mgr_v_unlock ) ( SRAMgr*, const char*, va_list );
        rc_t ( CC * mgr_v_drop_table ) ( SRAMgr*, bool, const char*, va_list );
        rc_t ( CC * mgr_v_create_table ) ( SRAMgr*, SRATable**, const char*, const char*, va_list );
        rc_t ( CC * mgr_v_open_table_update ) ( SRAMgr*, SRATable**, const char*, va_list );
        rc_t ( CC * tbl_new_spot ) ( SRATable*, spotid_t* );
        rc_t ( CC * tbl_open_spot ) ( SRATable*, spotid_t );
        rc_t ( CC * tbl_close_spot ) ( SRATable* );
        rc_t ( CC * tbl_commit ) ( SRATable* );
        rc_t ( CC * tbl_open_column_write ) ( SRATable*, uint32_t*, SRAColumn**, const char*, const char* );
        rc_t ( CC * tbl_set_idx_column_default ) ( SRATable*, uint32_t, const void*, bitsz_t, bitsz_t );
        rc_t ( CC * tbl_write_idx_column ) ( SRATable*, uint32_t, const void*, bitsz_t, bitsz_t );
        rc_t ( CC * tbl_meta_freeze ) ( SRATable* );
        rc_t ( CC * tbl_v_open_m_data_node_update ) ( SRATable*, struct KMDataNode**, const char*, va_list );
    } f;

} sra_wvt;

kqsh_libdata sra_data =
{
    NULL, NULL,
    sra_msgs, sra_cvt . slots,
    sra_wmsgs, sra_wvt . slots
};

rc_t _SRAMgrMake ( SRAMgr **mgr, struct KDirectory *wd )
{
    if ( sizeof sra_cvt . slots != sizeof sra_cvt . f ||
         sizeof sra_wvt . slots != sizeof sra_wvt . f )
    {
        * mgr = NULL;
        return RC ( rcExe, rcMgr, rcConstructing, rcInterface, rcCorrupt );
    }

    assert ( sra_cvt . f . mgr_make != NULL );
    return ( * sra_cvt . f . mgr_make ) ( mgr, wd );
}

rc_t _SRAMgrMakeReadWithDir ( const SRAMgr **mgr, struct KDirectory const *wd )
{
    assert ( sra_cvt . f . mgr_make_read_with_dir != NULL );
    return ( * sra_cvt . f . mgr_make_read_with_dir ) ( mgr, wd );
}

rc_t _SRAMgrRelease ( const SRAMgr *self )
{
    assert ( sra_cvt . f . mgr_release != NULL );
    return ( * sra_cvt . f . mgr_release ) ( self );
}

rc_t _SRAMgrAddRef ( const SRAMgr *self )
{
    assert ( sra_cvt . f . mgr_add_ref != NULL );
    return ( * sra_cvt . f . mgr_add_ref ) ( self );
}

rc_t _SRAMgrVersion ( const SRAMgr *self, uint32_t *version )
{
    assert ( sra_cvt . f . mgr_version != NULL );
    return ( * sra_cvt . f . mgr_version ) ( self, version );
}

rc_t _SRAMgrOpenDatatypesRead ( const SRAMgr *self, struct VDatatypes const **dt )
{
    assert ( sra_cvt . f . mgr_open_datatypes_read != NULL );
    return ( * sra_cvt . f . mgr_open_datatypes_read ) ( self, dt );
}

rc_t _SRAMgrGetSchemaRead ( const SRAMgr *self, struct VSchema const **schema )
{
    assert ( sra_cvt . f . mgr_get_schema_read != NULL );
    return ( * sra_cvt . f . mgr_get_schema_read ) ( self, schema );
}

rc_t _SRAMgrUseSchemaRead ( const SRAMgr *self, struct VSchema const *schema )
{
    assert ( sra_cvt . f . mgr_use_schema_read != NULL );
    return ( * sra_cvt . f . mgr_use_schema_read ) ( self, schema );
}

rc_t _SRAMgrWritable ( const SRAMgr *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( sra_cvt . f . mgr_v_writable != NULL );
    ret = ( * sra_cvt . f . mgr_v_writable ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _SRAMgrVWritable ( const SRAMgr *self, const char *path, va_list args )
{
    assert ( sra_cvt . f . mgr_v_writable != NULL );
    return ( * sra_cvt . f . mgr_v_writable ) ( self, path, args );
}

rc_t _SRANamelistAddRef ( const SRANamelist *self )
{
    assert ( sra_cvt . f . nmlist_add_ref != NULL );
    return ( * sra_cvt . f . nmlist_add_ref ) ( self );
}

rc_t _SRANamelistRelease ( const SRANamelist *self )
{
    assert ( sra_cvt . f . nmlist_release != NULL );
    return ( * sra_cvt . f . nmlist_release ) ( self );
}

rc_t _SRANamelistCount ( const SRANamelist *self, uint32_t *count )
{
    assert ( sra_cvt . f . nmlist_count != NULL );
    return ( * sra_cvt . f . nmlist_count ) ( self, count );
}

rc_t _SRANamelistGet ( const SRANamelist *self, uint32_t idx, const char **name )
{
    assert ( sra_cvt . f . nmlist_get != NULL );
    return ( * sra_cvt . f . nmlist_get ) ( self, idx, name );
}

rc_t _SRATableAddRef ( const SRATable *self )
{
    assert ( sra_cvt . f . tbl_add_ref != NULL );
    return ( * sra_cvt . f . tbl_add_ref ) ( self );
}

rc_t _SRATableRelease ( const SRATable *self )
{
    assert ( sra_cvt . f . tbl_release != NULL );
    return ( * sra_cvt . f . tbl_release ) ( self );
}

rc_t _SRAMgrOpenTableRead ( const SRAMgr *self, const SRATable **tbl, const char *spec, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, spec );
    assert ( sra_cvt . f . mgr_v_open_table_read != NULL );
    ret = ( * sra_cvt . f . mgr_v_open_table_read ) ( self, tbl, spec, args );
    va_end ( args );
    return ret;
}

rc_t _SRAMgrVOpenTableRead ( const SRAMgr *self, const SRATable **tbl, const char *spec, va_list args )
{
    assert ( sra_cvt . f . mgr_v_open_table_read != NULL );
    return ( * sra_cvt . f . mgr_v_open_table_read ) ( self, tbl, spec, args );
}

bool _SRATableLocked ( const SRATable *self )
{
    assert ( sra_cvt . f . tbl_locked != NULL );
    return ( * sra_cvt . f . tbl_locked ) ( self );
}

rc_t _SRATableBaseCount ( const SRATable *self, uint64_t *num_bases )
{
    assert ( sra_cvt . f . tbl_base_count != NULL );
    return ( * sra_cvt . f . tbl_base_count ) ( self, num_bases );
}

rc_t _SRATableSpotCount ( const SRATable *self, uint64_t *spot_count )
{
    assert ( sra_cvt . f . tbl_spot_count != NULL );
    return ( * sra_cvt . f . tbl_spot_count ) ( self, spot_count );
}

rc_t _SRATableMinSpotId ( const SRATable *self, spotid_t *id )
{
    assert ( sra_cvt . f . tbl_min_spot_id != NULL );
    return ( * sra_cvt . f . tbl_min_spot_id ) ( self, id );
}

rc_t _SRATableMaxSpotId ( const SRATable *self, spotid_t *id )
{
    assert ( sra_cvt . f . tbl_max_spot_id != NULL );
    return ( * sra_cvt . f . tbl_max_spot_id ) ( self, id );
}

rc_t _SRATableGetSpotId ( const SRATable *self, spotid_t *id, const char *spot_name )
{
    assert ( sra_cvt . f . tbl_get_spot_id != NULL );
    return ( * sra_cvt . f . tbl_get_spot_id ) ( self, id, spot_name );
}

rc_t _SRATableListCol ( const SRATable *self, SRANamelist **names )
{
    assert ( sra_cvt . f . tbl_list_col != NULL );
    return ( * sra_cvt . f . tbl_list_col ) ( self, names );
}

rc_t _SRATableColDatatypes ( const SRATable *self, const char *col, uint32_t *dflt_idx, SRANamelist **typedecls )
{
    assert ( sra_cvt . f . tbl_col_datatypes != NULL );
    return ( * sra_cvt . f . tbl_col_datatypes ) ( self, col, dflt_idx, typedecls );
}

rc_t _SRATableMetaRevision ( const SRATable *self, uint32_t *revision )
{
    assert ( sra_cvt . f . tbl_meta_revision != NULL );
    return ( * sra_cvt . f . tbl_meta_revision ) ( self, revision );
}

rc_t _SRATableMaxMetaRevision ( const SRATable *self, uint32_t *revision )
{
    assert ( sra_cvt . f . tbl_max_meta_revision != NULL );
    return ( * sra_cvt . f . tbl_max_meta_revision ) ( self, revision );
}

rc_t _SRATableUseMetaRevision ( const SRATable *self, uint32_t revision )
{
    assert ( sra_cvt . f . tbl_use_meta_revision != NULL );
    return ( * sra_cvt . f . tbl_use_meta_revision ) ( self, revision );
}

rc_t _SRATableOpenMDataNodeRead ( const SRATable *self, struct KMDataNode const **node, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( sra_cvt . f . tbl_v_open_m_data_node_read != NULL );
    ret = ( * sra_cvt . f . tbl_v_open_m_data_node_read ) ( self, node, path, args );
    va_end ( args );
    return ret;
}

rc_t _SRATableVOpenMDataNodeRead ( const SRATable *self, struct KMDataNode const **node, const char *path, va_list args )
{
    assert ( sra_cvt . f . tbl_v_open_m_data_node_read != NULL );
    return ( * sra_cvt . f . tbl_v_open_m_data_node_read ) ( self, node, path, args );
}

struct VSchema const* _SRATableGetSchema ( const SRATable *self )
{
    assert ( sra_cvt . f . tbl_get_schema != NULL );
    return ( * sra_cvt . f . tbl_get_schema ) ( self );
}

rc_t _SRAColumnAddRef ( const SRAColumn *self )
{
    assert ( sra_cvt . f . col_add_ref != NULL );
    return ( * sra_cvt . f . col_add_ref ) ( self );
}

rc_t _SRAColumnRelease ( const SRAColumn *self )
{
    assert ( sra_cvt . f . col_release != NULL );
    return ( * sra_cvt . f . col_release ) ( self );
}

rc_t _SRATableOpenColumnRead ( const SRATable *self, const SRAColumn **col, const char *name, const char *datatype )
{
    assert ( sra_cvt . f . tbl_open_column_read != NULL );
    return ( * sra_cvt . f . tbl_open_column_read ) ( self, col, name, datatype );
}

rc_t _SRAColumnDatatype ( const SRAColumn *self, struct VTypedecl *type, struct VTypedef *def )
{
    assert ( sra_cvt . f . col_datatype != NULL );
    return ( * sra_cvt . f . col_datatype ) ( self, type, def );
}

rc_t _SRAColumnGetRange ( const SRAColumn *self, spotid_t id, spotid_t *first, spotid_t *last )
{
    assert ( sra_cvt . f . col_get_range != NULL );
    return ( * sra_cvt . f . col_get_range ) ( self, id, first, last );
}

rc_t _SRAColumnRead ( const SRAColumn *self, spotid_t id, const void **base, bitsz_t *offset, bitsz_t *size )
{
    assert ( sra_cvt . f . col_read != NULL );
    return ( * sra_cvt . f . col_read ) ( self, id, base, offset, size );
}

rc_t _SRAMgrSetMD5Mode ( SRAMgr *self, bool useMD5 )
{
    assert ( sra_wvt . f . mgr_set_m_d5_mode != NULL );
    return ( * sra_wvt . f . mgr_set_m_d5_mode ) ( self, useMD5 );
}

rc_t _SRAMgrLock ( SRAMgr *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( sra_wvt . f . mgr_v_lock != NULL );
    ret = ( * sra_wvt . f . mgr_v_lock ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _SRAMgrVLock ( SRAMgr *self, const char *path, va_list args )
{
    assert ( sra_wvt . f . mgr_v_lock != NULL );
    return ( * sra_wvt . f . mgr_v_lock ) ( self, path, args );
}

rc_t _SRAMgrUnlock ( SRAMgr *self, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( sra_wvt . f . mgr_v_unlock != NULL );
    ret = ( * sra_wvt . f . mgr_v_unlock ) ( self, path, args );
    va_end ( args );
    return ret;
}

rc_t _SRAMgrVUnlock ( SRAMgr *self, const char *path, va_list args )
{
    assert ( sra_wvt . f . mgr_v_unlock != NULL );
    return ( * sra_wvt . f . mgr_v_unlock ) ( self, path, args );
}

rc_t _SRAMgrDropTable ( SRAMgr *self, bool force, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( sra_wvt . f . mgr_v_drop_table != NULL );
    ret = ( * sra_wvt . f . mgr_v_drop_table ) ( self, force, path, args );
    va_end ( args );
    return ret;
}

rc_t _SRAMgrVDropTable ( SRAMgr *self, bool force, const char *path, va_list args )
{
    assert ( sra_wvt . f . mgr_v_drop_table != NULL );
    return ( * sra_wvt . f . mgr_v_drop_table ) ( self, force, path, args );
}

rc_t _SRAMgrCreateTable ( SRAMgr *self, SRATable **tbl, const char *typespec, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( sra_wvt . f . mgr_v_create_table != NULL );
    ret = ( * sra_wvt . f . mgr_v_create_table ) ( self, tbl, typespec, path, args );
    va_end ( args );
    return ret;
}

rc_t _SRAMgrVCreateTable ( SRAMgr *self, SRATable **tbl, const char *typespec, const char *path, va_list args )
{
    assert ( sra_wvt . f . mgr_v_create_table != NULL );
    return ( * sra_wvt . f . mgr_v_create_table ) ( self, tbl, typespec, path, args );
}

rc_t _SRAMgrOpenTableUpdate ( SRAMgr *self, SRATable **tbl, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( sra_wvt . f . mgr_v_open_table_update != NULL );
    ret = ( * sra_wvt . f . mgr_v_open_table_update ) ( self, tbl, path, args );
    va_end ( args );
    return ret;
}

rc_t _SRAMgrVOpenTableUpdate ( SRAMgr *self, SRATable **tbl, const char *path, va_list args )
{
    assert ( sra_wvt . f . mgr_v_open_table_update != NULL );
    return ( * sra_wvt . f . mgr_v_open_table_update ) ( self, tbl, path, args );
}

rc_t _SRATableNewSpot ( SRATable *self, spotid_t *id )
{
    assert ( sra_wvt . f . tbl_new_spot != NULL );
    return ( * sra_wvt . f . tbl_new_spot ) ( self, id );
}

rc_t _SRATableOpenSpot ( SRATable *self, spotid_t id )
{
    assert ( sra_wvt . f . tbl_open_spot != NULL );
    return ( * sra_wvt . f . tbl_open_spot ) ( self, id );
}

rc_t _SRATableCloseSpot ( SRATable *self )
{
    assert ( sra_wvt . f . tbl_close_spot != NULL );
    return ( * sra_wvt . f . tbl_close_spot ) ( self );
}

rc_t _SRATableCommit ( SRATable *self )
{
    assert ( sra_wvt . f . tbl_commit != NULL );
    return ( * sra_wvt . f . tbl_commit ) ( self );
}

rc_t _SRATableOpenColumnWrite ( SRATable *self, uint32_t *idx, SRAColumn **col, const char *name, const char *datatype )
{
    assert ( sra_wvt . f . tbl_open_column_write != NULL );
    return ( * sra_wvt . f . tbl_open_column_write ) ( self, idx, col, name, datatype );
}

rc_t _SRATableSetIdxColumnDefault ( SRATable *self, uint32_t idx, const void *base, bitsz_t offset, bitsz_t size )
{
    assert ( sra_wvt . f . tbl_set_idx_column_default != NULL );
    return ( * sra_wvt . f . tbl_set_idx_column_default ) ( self, idx, base, offset, size );
}

rc_t _SRATableWriteIdxColumn ( SRATable *self, uint32_t idx, const void *base, bitsz_t offset, bitsz_t size )
{
    assert ( sra_wvt . f . tbl_write_idx_column != NULL );
    return ( * sra_wvt . f . tbl_write_idx_column ) ( self, idx, base, offset, size );
}

rc_t _SRATableMetaFreeze ( SRATable *self )
{
    assert ( sra_wvt . f . tbl_meta_freeze != NULL );
    return ( * sra_wvt . f . tbl_meta_freeze ) ( self );
}

rc_t _SRATableOpenMDataNodeUpdate ( SRATable *self, struct KMDataNode **node, const char *path, ... )
{
    rc_t ret;
    va_list args;
    va_start ( args, path );
    assert ( sra_wvt . f . tbl_v_open_m_data_node_update != NULL );
    ret = ( * sra_wvt . f . tbl_v_open_m_data_node_update ) ( self, node, path, args );
    va_end ( args );
    return ret;
}

rc_t _SRATableVOpenMDataNodeUpdate ( SRATable *self, struct KMDataNode **node, const char *path, va_list args )
{
    assert ( sra_wvt . f . tbl_v_open_m_data_node_update != NULL );
    return ( * sra_wvt . f . tbl_v_open_m_data_node_update ) ( self, node, path, args );
}

rc_t _SRAMgrMakeSRASchema ( struct SRAMgr const *self, struct VSchema **schema )
{
    assert ( sra_cvt . f . mgr_make_sra_schema != NULL );
    return ( * sra_cvt . f . mgr_make_sra_schema ) ( self, schema );
}

rc_t _VDBManagerMakeSRASchema ( struct VDBManager const *self, struct VSchema **schema )
{
    assert ( sra_cvt . f . vmgr_make_sra_schema != NULL );
    return ( * sra_cvt . f . vmgr_make_sra_schema ) ( self, schema );
}

