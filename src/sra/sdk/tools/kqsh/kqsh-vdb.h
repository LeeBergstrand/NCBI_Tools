/* THIS IS AN AUTO-GENERATED FILE - DO NOT EDIT */

#ifndef _h_kqsh_vdb_
#define _h_kqsh_vdb_

#ifndef _h_vdb_manager_
#include <vdb/manager.h>
#endif

#ifndef _h_vdb_database_
#include <vdb/database.h>
#endif

#ifndef _h_vdb_schema_
#include <vdb/schema.h>
#endif

#ifndef _h_vdb_table_
#include <vdb/table.h>
#endif

#ifndef _h_vdb_cursor_
#include <vdb/cursor.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

rc_t _VDBManagerMake ( VDBManager **mgr, struct KDirectory *wd );
rc_t _VDBManagerAddRef ( const VDBManager *self );
rc_t _VDBManagerRelease ( const VDBManager *self );
rc_t _VDBManagerVersion ( const VDBManager *self, uint32_t *version );
rc_t _VDBManagerWritable ( const VDBManager *self, const char *path, ... );
rc_t _VDBManagerVWritable ( const VDBManager *self, const char *path, va_list args );
rc_t _VDBManagerLock ( VDBManager *self, const char *path, ... );
rc_t _VDBManagerVLock ( VDBManager *self, const char *path, va_list args );
rc_t _VDBManagerUnlock ( VDBManager *self, const char *path, ... );
rc_t _VDBManagerVUnlock ( VDBManager *self, const char *path, va_list args );
rc_t _VDBManagerDrop ( VDBManager *self, uint32_t obj_type, const char *path, ... );
rc_t _VDBManagerVDrop ( VDBManager *self, uint32_t obj_type, const char *path, va_list args );
rc_t _VDBManagerAddSchemaIncludePath ( const VDBManager *self, const char *path, ... );
rc_t _VDBManagerVAddSchemaIncludePath ( const VDBManager *self, const char *path, va_list args );
rc_t _VDBManagerAddLoadLibraryPath ( const VDBManager *self, const char *path, ... );
rc_t _VDBManagerVAddLoadLibraryPath ( const VDBManager *self, const char *path, va_list args );
rc_t _VDBManagerGetUserData ( const VDBManager *self, void **data );
rc_t _VDBManagerSetUserData ( const VDBManager *self, void *data, void ( CC * destroy ) ( void *data ) );
rc_t _VDBManagerGetObjVersion ( const VDBManager *self, ver_t * version, const char *path );
int _VDBManagerPathType ( const VDBManager * self, const char *path, ... );
int _VDBManagerVPathType ( const VDBManager * self, const char *path, va_list args );
rc_t _VDatabaseAddRef ( const VDatabase *self );
rc_t _VDatabaseRelease ( const VDatabase *self );
rc_t _VDBManagerCreateDB ( struct VDBManager *self, VDatabase **db, struct VSchema const *schema, const char *typespec, KCreateMode cmode, const char *path, ... );
rc_t _VDBManagerVCreateDB ( struct VDBManager *self, VDatabase **db, struct VSchema const *schema, const char *typespec, KCreateMode cmode, const char *path, va_list args );
rc_t _VDatabaseCreateDB ( VDatabase *self, VDatabase **db, const char *member, KCreateMode cmode, const char *name, ... );
rc_t _VDatabaseVCreateDB ( VDatabase *self, VDatabase **db, const char *member, KCreateMode cmode, const char *name, va_list args );
rc_t _VDatabaseDropDB ( VDatabase *self, const char *name, ... );
rc_t _VDatabaseVDropDB ( VDatabase *self, const char *name, va_list args );
rc_t _VDatabaseDropTable ( VDatabase *self, const char *name, ... );
rc_t _VDatabaseVDropTable ( VDatabase *self, const char *name, va_list args );
rc_t _VDBManagerOpenDBRead ( struct VDBManager const *self, const VDatabase **db, struct VSchema const *schema, const char *path, ... );
rc_t _VDBManagerVOpenDBRead ( struct VDBManager const *self, const VDatabase **db, struct VSchema const *schema, const char *path, va_list args );
rc_t _VDBManagerOpenDBUpdate ( struct VDBManager *self, VDatabase **db, struct VSchema const *schema, const char *path, ... );
rc_t _VDBManagerVOpenDBUpdate ( struct VDBManager *self, VDatabase **db, struct VSchema const *schema, const char *path, va_list args );
rc_t _VDatabaseOpenDBRead ( const VDatabase *self, const VDatabase **db, const char *name, ... );
rc_t _VDatabaseVOpenDBRead ( const VDatabase *self, const VDatabase **db, const char *name, va_list args );
rc_t _VDatabaseOpenDBUpdate ( VDatabase *self, VDatabase **db, const char *name, ... );
rc_t _VDatabaseVOpenDBUpdate ( VDatabase *self, VDatabase **db, const char *name, va_list args );
rc_t _VDatabaseTypespec ( const VDatabase *self, char *ts_buff, size_t ts_size );
bool _VDatabaseLocked ( const VDatabase *self );
bool _VDatabaseIsAlias ( const VDatabase *self, uint32_t type, char *resolved, size_t rsize, const char *name );
rc_t _VDatabaseWritable ( const VDatabase *self, uint32_t type, const char *name, ... );
rc_t _VDatabaseVWritable ( const VDatabase *self, uint32_t type, const char *name, va_list args );
rc_t _VDatabaseLock ( VDatabase *self, uint32_t type, const char *name, ... );
rc_t _VDatabaseVLock ( VDatabase *self, uint32_t type, const char *name, va_list args );
rc_t _VDatabaseUnlock ( VDatabase *self, uint32_t type, const char *name, ... );
rc_t _VDatabaseVUnlock ( VDatabase *self, uint32_t type, const char *name, va_list args );
rc_t _VDatabaseOpenMetadataRead ( const VDatabase *self, struct KMetadata const **meta );
rc_t _VDatabaseOpenMetadataUpdate ( VDatabase *self, struct KMetadata **meta );
rc_t _VDatabaseColumnCreateParams ( VDatabase *self, KCreateMode cmode, KChecksum checksum, size_t pgsize );
rc_t _VDatabaseOpenManagerRead ( const VDatabase *self, struct VDBManager const **mgr );
rc_t _VDatabaseOpenManagerUpdate ( VDatabase *self, struct VDBManager **mgr );
rc_t _VDatabaseOpenParentRead ( const VDatabase *self, const VDatabase **par );
rc_t _VDatabaseOpenParentUpdate ( VDatabase *self, VDatabase **par );
rc_t _VDatabaseOpenSchema ( const VDatabase *self, struct VSchema const **schema );
rc_t _VDatabaseListTbl ( const VDatabase *self, KNamelist **names );
rc_t _VDatabaseListDB ( const VDatabase *self, KNamelist **names );
rc_t _VDatabaseGetUserData ( const VDatabase *self, void **data );
rc_t _VDatabaseSetUserData ( const VDatabase *self, void *data, void ( CC * destroy ) ( void *data ) );
rc_t _VSchemaAddRef ( const VSchema *self );
rc_t _VSchemaRelease ( const VSchema *self );
rc_t _VDBManagerMakeSchema ( struct VDBManager const *self, VSchema **schema );
rc_t _VSchemaAddIncludePath ( VSchema *self, const char *path, ... );
rc_t _VSchemaVAddIncludePath ( VSchema *self, const char *path, va_list args );
rc_t _VSchemaParseText ( VSchema *self, const char *name, const char *text, size_t bytes );
rc_t _VSchemaParseFile ( VSchema *self, const char *name, ... );
rc_t _VSchemaVParseFile ( VSchema *self, const char *name, va_list args );
rc_t _VSchemaDump ( const VSchema *self, uint32_t mode, const char *decl, rc_t ( CC * flush ) ( void *dst, const void *buffer, size_t bsize ), void *dst );
rc_t _VSchemaIncludeFiles ( const VSchema *self, struct KNamelist const **list );
rc_t _VSchemaResolveTypedecl ( const VSchema *self, VTypedecl *resolved, const char *typedecl, ... );
rc_t _VSchemaVResolveTypedecl ( const VSchema *self, VTypedecl *resolved, const char *typedecl, va_list args );
rc_t _VTypedeclToText ( const VTypedecl *self, const VSchema *schema, char *buffer, size_t bsize );
bool _VTypedeclToSupertype ( const VTypedecl *self, const VSchema *schema, VTypedecl *cast );
bool _VTypedeclToType ( const VTypedecl *self, const VSchema *schema,  uint32_t ancestor, VTypedecl *cast, uint32_t *distance );
bool _VTypedeclToTypedecl ( const VTypedecl *self, const VSchema *schema, const VTypedecl *ancestor, VTypedecl *cast, uint32_t *distance );
bool _VTypedeclCommonAncestor ( const VTypedecl *self, const VSchema *schema, const VTypedecl *peer, VTypedecl *ancestor, uint32_t *distance );
uint32_t _VTypedescSizeof ( const VTypedesc *self );
rc_t _VSchemaDescribeTypedecl ( const VSchema *self, VTypedesc *desc, const VTypedecl *td );
rc_t _VSchemaMakeRuntimeTable ( VSchema *self, VSchemaRuntimeTable **tbl, const char *type_name, const char *supertype_spec );
rc_t _VSchemaRuntimeTableClose ( VSchemaRuntimeTable *self );
rc_t _VSchemaRuntimeTableCommit ( VSchemaRuntimeTable *self );
rc_t _VSchemaRuntimeTableAddColumn ( VSchemaRuntimeTable *self, const VTypedecl *td, const char *encoding, const char *name, ... );
rc_t _VSchemaRuntimeTableVAddColumn ( VSchemaRuntimeTable *self, const VTypedecl *td, const char *encoding, const char *name, va_list args );
rc_t _VSchemaRuntimeTableAddBooleanColumn ( VSchemaRuntimeTable *self, const char *name, ... );
rc_t _VSchemaRuntimeTableAddIntegerColumn ( VSchemaRuntimeTable *self, uint32_t bits, bool has_sign,  const char *name, ... );
rc_t _VSchemaRuntimeTableAddFloatColumn ( VSchemaRuntimeTable *self, uint32_t bits, uint32_t significant_mantissa_bits,  const char *name, ... );
rc_t _VSchemaRuntimeTableAddAsciiColumn ( VSchemaRuntimeTable *self, const char *name, ... );
rc_t _VSchemaRuntimeTableAddUnicodeColumn ( VSchemaRuntimeTable *self, uint32_t bits, const char *name, ... );
rc_t _VSchemaRuntimeTableVAddBooleanColumn ( VSchemaRuntimeTable *self, const char *name, va_list args );
rc_t _VSchemaRuntimeTableVAddIntegerColumn ( VSchemaRuntimeTable *self, uint32_t bits, bool has_sign,  const char *name, va_list args );
rc_t _VSchemaRuntimeTableVAddFloatColumn ( VSchemaRuntimeTable *self, uint32_t bits, uint32_t significant_mantissa_bits,  const char *name, va_list args );
rc_t _VSchemaRuntimeTableVAddAsciiColumn ( VSchemaRuntimeTable *self, const char *name, va_list args );
rc_t _VSchemaRuntimeTableVAddUnicodeColumn ( VSchemaRuntimeTable *self, uint32_t bits, const char *name, va_list args );
rc_t _VTableAddRef ( const VTable *self );
rc_t _VTableRelease ( const VTable *self );
rc_t _VDBManagerCreateTable ( struct VDBManager *self, VTable **tbl, struct VSchema const *schema, const char *typespec, KCreateMode cmode, const char *path, ... );
rc_t _VDBManagerVCreateTable ( struct VDBManager *self, VTable **tbl, struct VSchema const *schema, const char *typespec, KCreateMode cmode, const char *path, va_list args );
rc_t _VDatabaseCreateTable ( struct VDatabase *self, VTable **tbl, const char *member, KCreateMode cmode, const char *name, ... );
rc_t _VDatabaseVCreateTable ( struct VDatabase *self, VTable **tbl, const char *member, KCreateMode cmode, const char *name, va_list args );
rc_t _VDBManagerOpenTableRead ( struct VDBManager const *self, const VTable **tbl, struct VSchema const *schema, const char *path, ... );
rc_t _VDBManagerVOpenTableRead ( struct VDBManager const *self, const VTable **tbl, struct VSchema const *schema, const char *path, va_list args );
rc_t _VDBManagerOpenTableUpdate ( struct VDBManager *self, VTable **tbl, struct VSchema const *schema, const char *path, ... );
rc_t _VDBManagerVOpenTableUpdate ( struct VDBManager *self, VTable **tbl, struct VSchema const *schema, const char *path, va_list args );
rc_t _VDatabaseOpenTableRead ( struct VDatabase const *self, const VTable **tbl, const char *name, ... );
rc_t _VDatabaseVOpenTableRead ( struct VDatabase const *self, const VTable **tbl, const char *name, va_list args );
rc_t _VDatabaseOpenTableUpdate ( struct VDatabase *self, VTable **tbl, const char *name, ... );
rc_t _VDatabaseVOpenTableUpdate ( struct VDatabase *self, VTable **tbl, const char *name, va_list args );
rc_t _VTableTypespec ( const VTable *self, char *ts_buff, size_t ts_size );
bool _VTableLocked ( const VTable *self );
rc_t _VTableWritable ( const VTable *self, uint32_t type, const char * name, ... );
rc_t _VTableVWritable ( const VTable *self, uint32_t type, const char * name, va_list args );
rc_t _VTableLock ( VTable *self, uint32_t type, const char * name, ... );
rc_t _VTableVLock ( VTable *self, uint32_t type, const char * name, va_list args );
rc_t _VTableUnlock ( VTable *self, uint32_t type, const char * name, ... );
rc_t _VTableVUnlock ( VTable *self, uint32_t type, const char * name, va_list args );
rc_t _VTableOpenMetadataRead ( const VTable *self, struct KMetadata const **meta );
rc_t _VTableOpenMetadataUpdate ( VTable *self, struct KMetadata **meta );
rc_t _VTableColumnCreateParams ( VTable *self, KCreateMode cmode, KChecksum checksum, size_t pgsize );
rc_t _VTableCreateIndex ( VTable *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, ... );
rc_t _VTableVCreateIndex ( VTable *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, va_list args );
rc_t _VTableOpenIndexRead ( const VTable *self, const KIndex **idx, const char *name, ... );
rc_t _VTableVOpenIndexRead ( const VTable *self, const KIndex **idx, const char *name, va_list args );
rc_t _VTableOpenIndexUpdate ( VTable *self, KIndex **idx, const char *name, ... );
rc_t _VTableVOpenIndexUpdate ( VTable *self, KIndex **idx, const char *name, va_list args );
rc_t _VTableListReadableColumns ( const VTable *self, struct KNamelist **names );
rc_t _VTableListWritableColumns ( VTable *self, struct KNamelist **names );
rc_t _VTableListCol ( const VTable *self, struct KNamelist **names );
rc_t _VTableListReadableDatatypes ( const VTable *self, const char *col, uint32_t *dflt_idx, struct KNamelist **typedecls );
rc_t _VTableListWritableDatatypes ( VTable *self, const char *col, struct KNamelist **typedecls );
rc_t _VTableColumnDatatypes ( const VTable *self, const char *col, uint32_t *dflt_idx, struct KNamelist **typedecls );
rc_t _VTableReindex ( VTable *self );
rc_t _VTableOpenManagerRead ( const VTable *self, struct VDBManager const **mgr );
rc_t _VTableOpenManagerUpdate ( VTable *self, struct VDBManager **mgr );
rc_t _VTableOpenParentRead ( const VTable *self, struct VDatabase const **db );
rc_t _VTableOpenParentUpdate ( VTable *self, struct VDatabase **db );
rc_t _VTableOpenSchema ( const VTable *self, struct VSchema const **schema );
rc_t _VTableGetUserData ( const VTable *self, void **data );
rc_t _VTableSetUserData ( const VTable *self, void *data, void ( CC * destroy ) ( void *data ) );
rc_t _VCursorAddRef ( const VCursor *self );
rc_t _VCursorRelease ( const VCursor *self );
rc_t _VTableCreateCursorRead ( struct VTable const *self, const VCursor **curs );
rc_t _VTableCreateCursorWrite ( struct VTable *self, VCursor **curs, KCreateMode mode );
rc_t _VTableCreateCachedCursorRead ( struct VTable const *self, const VCursor **curs, size_t capacity );
rc_t _VCursorAddColumn ( const VCursor *self, uint32_t *idx, const char *name, ... );
rc_t _VCursorVAddColumn ( const VCursor *self, uint32_t *idx, const char *name, va_list args );
rc_t _VCursorGetColumnIdx ( const VCursor *self, uint32_t *idx, const char *name, ... );
rc_t _VCursorVGetColumnIdx ( const VCursor *self, uint32_t *idx, const char *name, va_list args );
rc_t _VCursorDatatype ( const VCursor *self, uint32_t idx, struct VTypedecl *type, struct VTypedesc *desc );
rc_t _VCursorIdRange ( const VCursor *self, uint32_t idx, int64_t *first, uint64_t *count );
rc_t _VCursorOpen ( const VCursor *self );
rc_t _VCursorRowId ( const VCursor *self, int64_t *row_id );
rc_t _VCursorSetRowId ( const VCursor *self, int64_t row_id );
rc_t _VCursorOpenRow ( const VCursor *self );
rc_t _VCursorCommitRow ( VCursor *self );
rc_t _VCursorRepeatRow ( VCursor *self, uint64_t count );
rc_t _VCursorCloseRow ( const VCursor *self );
rc_t _VCursorFlushPage ( VCursor *self );
rc_t _VCursorGetBlob ( const VCursor *self, struct VBlob const **blob, uint32_t col_idx );
rc_t _VCursorGetBlobDirect ( const VCursor *self, struct VBlob const **blob, int64_t row_id, uint32_t col_idx );
rc_t _VCursorRead ( const VCursor *self, uint32_t col_idx, uint32_t elem_bits, void *buffer, uint32_t blen, uint32_t *row_len );
rc_t _VCursorReadDirect ( const VCursor *self, int64_t row_id, uint32_t col_idx, uint32_t elem_bits, void *buffer, uint32_t blen, uint32_t *row_len );
rc_t _VCursorReadBits ( const VCursor *self, uint32_t col_idx, uint32_t elem_bits, uint32_t start, void *buffer, uint32_t boff, uint32_t blen, uint32_t *num_read, uint32_t *remaining );
rc_t _VCursorReadBitsDirect ( const VCursor *self, int64_t row_id, uint32_t col_idx, uint32_t elem_bits, uint32_t start, void *buffer, uint32_t boff, uint32_t blen, uint32_t *num_read, uint32_t *remaining );
rc_t _VCursorCellData ( const VCursor *self, uint32_t col_idx, uint32_t *elem_bits, const void **base, uint32_t *boff, uint32_t *row_len );
rc_t _VCursorCellDataDirect ( const VCursor *self, int64_t row_id, uint32_t col_idx, uint32_t *elem_bits, const void **base, uint32_t *boff, uint32_t *row_len );
rc_t _VCursorDefault ( VCursor *self, uint32_t col_idx, bitsz_t elem_bits, const void *buffer, bitsz_t boff, uint64_t row_len );
rc_t _VCursorWrite ( VCursor *self, uint32_t col_idx, bitsz_t elem_bits, const void *buffer, bitsz_t boff, uint64_t count );
rc_t _VCursorCommit ( VCursor *self );
rc_t _VCursorOpenParentRead ( const VCursor *self, struct VTable const **tbl );
rc_t _VCursorOpenParentUpdate ( VCursor *self, struct VTable **tbl );
rc_t _VCursorGetUserData ( const VCursor *self, void **data );
rc_t _VCursorSetUserData ( const VCursor *self, void *data, void ( CC * destroy ) ( void *data ) );

#ifdef __cplusplus
}
#endif

#endif /* _h_kqsh_vdb_ */
