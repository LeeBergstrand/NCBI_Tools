/* THIS IS AN AUTO-GENERATED FILE - DO NOT EDIT */

#ifndef _h_kqsh_sra_
#define _h_kqsh_sra_

#ifndef _h_sra_sradb_
#include <sra/sradb.h>
#endif

#ifndef _h_sra_wsradb_
#include <sra/wsradb.h>
#endif

#ifndef _h_sra_sraschema_
#include <sra/sraschema.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

rc_t _SRAMgrMake ( SRAMgr **mgr, struct KDirectory *wd );
rc_t _SRAMgrMakeReadWithDir ( const SRAMgr **mgr, struct KDirectory const *wd );
rc_t _SRAMgrRelease ( const SRAMgr *self );
rc_t _SRAMgrAddRef ( const SRAMgr *self );
rc_t _SRAMgrVersion ( const SRAMgr *self, uint32_t *version );
rc_t _SRAMgrOpenDatatypesRead ( const SRAMgr *self, struct VDatatypes const **dt );
rc_t _SRAMgrGetSchemaRead ( const SRAMgr *self, struct VSchema const **schema );
rc_t _SRAMgrUseSchemaRead ( const SRAMgr *self, struct VSchema const *schema );
rc_t _SRAMgrWritable ( const SRAMgr *self, const char *path, ... );
rc_t _SRAMgrVWritable ( const SRAMgr *self, const char *path, va_list args );
rc_t _SRANamelistAddRef ( const SRANamelist *self );
rc_t _SRANamelistRelease ( const SRANamelist *self );
rc_t _SRANamelistCount ( const SRANamelist *self, uint32_t *count );
rc_t _SRANamelistGet ( const SRANamelist *self, uint32_t idx, const char **name );
rc_t _SRATableAddRef ( const SRATable *self );
rc_t _SRATableRelease ( const SRATable *self );
rc_t _SRAMgrOpenTableRead ( const SRAMgr *self, const SRATable **tbl, const char *spec, ... );
rc_t _SRAMgrVOpenTableRead ( const SRAMgr *self, const SRATable **tbl, const char *spec, va_list args );
bool _SRATableLocked ( const SRATable *self );
rc_t _SRATableBaseCount ( const SRATable *self, uint64_t *num_bases );
rc_t _SRATableSpotCount ( const SRATable *self, uint64_t *spot_count );
rc_t _SRATableMinSpotId ( const SRATable *self, spotid_t *id );
rc_t _SRATableMaxSpotId ( const SRATable *self, spotid_t *id );
rc_t _SRATableGetSpotId ( const SRATable *self, spotid_t *id, const char *spot_name );
rc_t _SRATableListCol ( const SRATable *self, SRANamelist **names );
rc_t _SRATableColDatatypes ( const SRATable *self, const char *col, uint32_t *dflt_idx, SRANamelist **typedecls );
rc_t _SRATableMetaRevision ( const SRATable *self, uint32_t *revision );
rc_t _SRATableMaxMetaRevision ( const SRATable *self, uint32_t *revision );
rc_t _SRATableUseMetaRevision ( const SRATable *self, uint32_t revision );
rc_t _SRATableOpenMDataNodeRead ( const SRATable *self, struct KMDataNode const **node, const char *path, ... );
rc_t _SRATableVOpenMDataNodeRead ( const SRATable *self, struct KMDataNode const **node, const char *path, va_list args );
struct VSchema const* _SRATableGetSchema ( const SRATable *self );
rc_t _SRAColumnAddRef ( const SRAColumn *self );
rc_t _SRAColumnRelease ( const SRAColumn *self );
rc_t _SRATableOpenColumnRead ( const SRATable *self, const SRAColumn **col, const char *name, const char *datatype );
rc_t _SRAColumnDatatype ( const SRAColumn *self, struct VTypedecl *type, struct VTypedef *def );
rc_t _SRAColumnGetRange ( const SRAColumn *self, spotid_t id, spotid_t *first, spotid_t *last );
rc_t _SRAColumnRead ( const SRAColumn *self, spotid_t id, const void **base, bitsz_t *offset, bitsz_t *size );
rc_t _SRAMgrSetMD5Mode ( SRAMgr *self, bool useMD5 );
rc_t _SRAMgrLock ( SRAMgr *self, const char *path, ... );
rc_t _SRAMgrVLock ( SRAMgr *self, const char *path, va_list args );
rc_t _SRAMgrUnlock ( SRAMgr *self, const char *path, ... );
rc_t _SRAMgrVUnlock ( SRAMgr *self, const char *path, va_list args );
rc_t _SRAMgrDropTable ( SRAMgr *self, bool force, const char *path, ... );
rc_t _SRAMgrVDropTable ( SRAMgr *self, bool force, const char *path, va_list args );
rc_t _SRAMgrCreateTable ( SRAMgr *self, SRATable **tbl, const char *typespec, const char *path, ... );
rc_t _SRAMgrVCreateTable ( SRAMgr *self, SRATable **tbl, const char *typespec, const char *path, va_list args );
rc_t _SRAMgrOpenTableUpdate ( SRAMgr *self, SRATable **tbl, const char *path, ... );
rc_t _SRAMgrVOpenTableUpdate ( SRAMgr *self, SRATable **tbl, const char *path, va_list args );
rc_t _SRATableNewSpot ( SRATable *self, spotid_t *id );
rc_t _SRATableOpenSpot ( SRATable *self, spotid_t id );
rc_t _SRATableCloseSpot ( SRATable *self );
rc_t _SRATableCommit ( SRATable *self );
rc_t _SRATableOpenColumnWrite ( SRATable *self, uint32_t *idx, SRAColumn **col, const char *name, const char *datatype );
rc_t _SRATableSetIdxColumnDefault ( SRATable *self, uint32_t idx, const void *base, bitsz_t offset, bitsz_t size );
rc_t _SRATableWriteIdxColumn ( SRATable *self, uint32_t idx, const void *base, bitsz_t offset, bitsz_t size );
rc_t _SRATableMetaFreeze ( SRATable *self );
rc_t _SRATableOpenMDataNodeUpdate ( SRATable *self, struct KMDataNode **node, const char *path, ... );
rc_t _SRATableVOpenMDataNodeUpdate ( SRATable *self, struct KMDataNode **node, const char *path, va_list args );
rc_t _SRAMgrMakeSRASchema ( struct SRAMgr const *self, struct VSchema **schema );
rc_t _VDBManagerMakeSRASchema ( struct VDBManager const *self, struct VSchema **schema );

#ifdef __cplusplus
}
#endif

#endif /* _h_kqsh_sra_ */
