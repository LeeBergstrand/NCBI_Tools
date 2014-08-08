/* THIS IS AN AUTO-GENERATED FILE - DO NOT EDIT */

#ifndef _h_kqsh_kdb_
#define _h_kqsh_kdb_

#ifndef _h_kdb_manager_
#include <kdb/manager.h>
#endif

#ifndef _h_kdb_database_
#include <kdb/database.h>
#endif

#ifndef _h_kdb_table_
#include <kdb/table.h>
#endif

#ifndef _h_kdb_column_
#include <kdb/column.h>
#endif

#ifndef _h_kdb_index_
#include <kdb/index.h>
#endif

#ifndef _h_kdb_meta_
#include <kdb/meta.h>
#endif

#ifndef _h_kdb_namelist_
#include <kdb/namelist.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

rc_t _KDBManagerMake ( KDBManager **mgr, struct KDirectory *wd );
rc_t _KDBManagerAddRef ( const KDBManager *self );
rc_t _KDBManagerRelease ( const KDBManager *self );
rc_t _KDBManagerVersion ( const KDBManager *self, uint32_t *version );
bool _KDBManagerExists ( const KDBManager *self, uint32_t type, const char *name, ... );
bool _KDBManagerVExists ( const KDBManager *self, uint32_t type, const char *name, va_list args );
rc_t _KDBManagerWritable ( const KDBManager *self, const char *path, ... );
rc_t _KDBManagerVWritable ( const KDBManager *self, const char *path, va_list args );
rc_t _KDBManagerLock ( KDBManager *self, const char *path, ... );
rc_t _KDBManagerVLock ( KDBManager *self, const char *path, va_list args );
rc_t _KDBManagerUnlock ( KDBManager *self, const char *path, ... );
rc_t _KDBManagerVUnlock ( KDBManager *self, const char *path, va_list args );
rc_t _KDBManagerDrop ( KDBManager *self, uint32_t obj_type, const char *path, ... );
rc_t _KDBManagerVDrop ( KDBManager *self, uint32_t obj_type, const char *path, va_list args );
rc_t _KDBManagerRunPeriodicTasks ( const KDBManager *self );
int _KDBManagerPathTypeVP ( const KDBManager * self, const struct VPath * path );
int _KDBManagerPathType ( const KDBManager * self, const char *path, ... );
int _KDBManagerVPathType ( const KDBManager * self, const char *path, va_list args );
rc_t _KDatabaseAddRef ( const KDatabase *self );
rc_t _KDatabaseRelease ( const KDatabase *self );
rc_t _KDBManagerCreateDB ( struct KDBManager *self, KDatabase **db, KCreateMode cmode, const char *path, ... );
rc_t _KDatabaseCreateDB ( KDatabase *self, KDatabase **db, KCreateMode cmode, const char *name, ... );
rc_t _KDBManagerVCreateDB ( struct KDBManager *self, KDatabase **db, KCreateMode cmode, const char *path, va_list args );
rc_t _KDatabaseVCreateDB ( KDatabase *self, KDatabase **db, KCreateMode cmode, const char *name, va_list args );
rc_t _KDBManagerOpenDBRead ( struct KDBManager const *self, const KDatabase **db, const char *path, ... );
rc_t _KDatabaseOpenDBRead ( const KDatabase *self, const KDatabase **db, const char *name, ... );
rc_t _KDBManagerVOpenDBRead ( struct KDBManager const *self, const KDatabase **db, const char *path, va_list args );
rc_t _KDatabaseVOpenDBRead ( const KDatabase *self, const KDatabase **db, const char *name, va_list args );
rc_t _KDBManagerOpenDBUpdate ( struct KDBManager *self, KDatabase **db, const char *path, ... );
rc_t _KDatabaseOpenDBUpdate ( KDatabase *self, KDatabase **db, const char *name, ... );
rc_t _KDBManagerVOpenDBUpdate ( struct KDBManager *self, KDatabase **db, const char *path, va_list args );
rc_t _KDatabaseVOpenDBUpdate ( KDatabase *self, KDatabase **db, const char *name, va_list args );
bool _KDatabaseLocked ( const KDatabase *self );
bool _KDatabaseExists ( const KDatabase *self, uint32_t type, const char *name, ... );
bool _KDatabaseVExists ( const KDatabase *self, uint32_t type, const char *name, va_list args );
bool _KDatabaseIsAlias ( const KDatabase *self, uint32_t type, char *resolved, size_t rsize, const char *name );
rc_t _KDatabaseWritable ( const KDatabase *self, uint32_t type, const char *name, ... );
rc_t _KDatabaseVWritable ( const KDatabase *self, uint32_t type, const char *name, va_list args );
rc_t _KDatabaseLock ( KDatabase *self, uint32_t type, const char *name, ... );
rc_t _KDatabaseVLock ( KDatabase *self, uint32_t type, const char *name, va_list args );
rc_t _KDatabaseUnlock ( KDatabase *self, uint32_t type, const char *name, ... );
rc_t _KDatabaseVUnlock ( KDatabase *self, uint32_t type, const char *name, va_list args );
rc_t _KDatabaseRenameDB ( KDatabase *self, bool force, const char *from, const char *to );
rc_t _KDatabaseRenameTable ( KDatabase *self, bool force, const char *from, const char *to );
rc_t _KDatabaseRenameIndex ( KDatabase *self, bool force, const char *from, const char *to );
rc_t _KDatabaseAliasDB ( KDatabase *self, const char *obj, const char *alias );
rc_t _KDatabaseAliasTable ( KDatabase *self, const char *obj, const char *alias );
rc_t _KDatabaseAliasIndex ( KDatabase *self, const char *obj, const char *alias );
rc_t _KDatabaseDropDB ( KDatabase *self, const char *name, ... );
rc_t _KDatabaseDropTable ( KDatabase *self, const char *name, ... );
rc_t _KDatabaseDropIndex ( KDatabase *self, const char *name, ... );
rc_t _KDatabaseVDropDB ( KDatabase *self, const char *name, va_list args );
rc_t _KDatabaseVDropTable ( KDatabase *self, const char *name, va_list args );
rc_t _KDatabaseVDropIndex ( KDatabase *self, const char *name, va_list args );
rc_t _KDatabaseOpenManagerRead ( const KDatabase *self, struct KDBManager const **mgr );
rc_t _KDatabaseOpenManagerUpdate ( KDatabase *self, struct KDBManager **mgr );
rc_t _KDatabaseOpenParentRead ( const KDatabase *self, const KDatabase **par );
rc_t _KDatabaseOpenParentUpdate ( KDatabase *self, KDatabase **par );
rc_t _KTableAddRef ( const KTable *self );
rc_t _KTableRelease ( const KTable *self );
rc_t _KDBManagerCreateTable ( struct KDBManager *self, KTable **tbl, KCreateMode cmode, const char *path, ... );
rc_t _KDatabaseCreateTable ( struct KDatabase *self, KTable **tbl, KCreateMode cmode, const char *name, ... );
rc_t _KDBManagerVCreateTable ( struct KDBManager *self, KTable **tbl, KCreateMode cmode, const char *path, va_list args );
rc_t _KDatabaseVCreateTable ( struct KDatabase *self, KTable **tbl, KCreateMode cmode, const char *name, va_list args );
rc_t _KDBManagerOpenTableRead ( struct KDBManager const *self, const KTable **tbl, const char *path, ... );
rc_t _KDatabaseOpenTableRead ( struct KDatabase const *self, const KTable **tbl, const char *name, ... );
rc_t _KDBManagerVOpenTableRead ( struct KDBManager const *self, const KTable **tbl, const char *path, va_list args );
rc_t _KDatabaseVOpenTableRead ( struct KDatabase const *self, const KTable **tbl, const char *name, va_list args );
rc_t _KDBManagerOpenTableUpdate ( struct KDBManager *self, KTable **tbl, const char *path, ... );
rc_t _KDatabaseOpenTableUpdate ( struct KDatabase *self, KTable **tbl, const char *name, ... );
rc_t _KDBManagerVOpenTableUpdate ( struct KDBManager *self, KTable **tbl, const char *path, va_list args );
rc_t _KDatabaseVOpenTableUpdate ( struct KDatabase *self, KTable **tbl, const char *name, va_list args );
bool _KTableLocked ( const KTable *self );
bool _KTableExists ( const KTable *self, uint32_t type, const char *name, ... );
bool _KTableVExists ( const KTable *self, uint32_t type, const char *name, va_list args );
bool _KTableIsAlias ( const KTable *self, uint32_t type, char *resolved, size_t rsize, const char *name );
rc_t _KTableWritable ( const KTable *self, uint32_t type, const char *name, ... );
rc_t _KTableVWritable ( const KTable *self, uint32_t type, const char *name, va_list args );
rc_t _KTableLock ( KTable *self, uint32_t type, const char *name, ... );
rc_t _KTableVLock ( KTable *self, uint32_t type, const char *name, va_list args );
rc_t _KTableUnlock ( KTable *self, uint32_t type, const char *name, ... );
rc_t _KTableVUnlock ( KTable *self, uint32_t type, const char *name, va_list args );
rc_t _KTableRenameColumn ( KTable *self, bool force, const char *from, const char *to );
rc_t _KTableRenameIndex ( KTable *self, bool force, const char *from, const char *to );
rc_t _KTableAliasColumn ( KTable *self, const char *path, const char *alias );
rc_t _KTableAliasIndex ( KTable *self, const char *name, const char *alias );
rc_t _KTableDropColumn ( KTable *self, const char *name, ... );
rc_t _KTableDropIndex ( KTable *self, const char *name, ... );
rc_t _KTableVDropColumn ( KTable *self, const char *name, va_list args );
rc_t _KTableVDropIndex ( KTable *self, const char *name, va_list args );
rc_t _KTableReindex ( KTable *self );
rc_t _KTableOpenManagerRead ( const KTable *self, struct KDBManager const **mgr );
rc_t _KTableOpenManagerUpdate ( KTable *self, struct KDBManager **mgr );
rc_t _KTableOpenParentRead ( const KTable *self, struct KDatabase const **db );
rc_t _KTableOpenParentUpdate ( KTable *self, struct KDatabase **db );
rc_t _KColumnAddRef ( const KColumn *self );
rc_t _KColumnRelease ( const KColumn *self );
rc_t _KDBManagerCreateColumn ( struct KDBManager *self, KColumn **col, KCreateMode cmode, KChecksum checksum, size_t pgsize, const char *path, ... );
rc_t _KTableCreateColumn ( struct KTable *self, KColumn **col, KCreateMode cmode, KChecksum checksum, size_t pgsize, const char *path, ... );
rc_t _KDBManagerVCreateColumn ( struct KDBManager *self, KColumn **col, KCreateMode cmode, KChecksum checksum, size_t pgsize, const char *path, va_list args );
rc_t _KTableVCreateColumn ( struct KTable *self, KColumn **col, KCreateMode cmode, KChecksum checksum, size_t pgsize, const char *path, va_list args );
rc_t _KDBManagerOpenColumnRead ( struct KDBManager const *self, const KColumn **col, const char *path, ... );
rc_t _KTableOpenColumnRead ( struct KTable const *self, const KColumn **col, const char *path, ... );
rc_t _KDBManagerVOpenColumnRead ( struct KDBManager const *self, const KColumn **col, const char *path, va_list args );
rc_t _KTableVOpenColumnRead ( struct KTable const *self, const KColumn **col, const char *path, va_list args );
rc_t _KDBManagerOpenColumnUpdate ( struct KDBManager *self, KColumn **col, const char *path, ... );
rc_t _KTableOpenColumnUpdate ( struct KTable *self, KColumn **col, const char *path, ... );
rc_t _KDBManagerVOpenColumnUpdate ( struct KDBManager *self, KColumn **col, const char *path, va_list args );
rc_t _KTableVOpenColumnUpdate ( struct KTable *self, KColumn **col, const char *path, va_list args );
bool _KColumnLocked ( const KColumn *self );
rc_t _KColumnVersion ( const KColumn *self, uint32_t *version );
rc_t _KColumnByteOrder ( const KColumn *self, bool *reversed );
rc_t _KColumnIdRange ( const KColumn *self, int64_t *first, uint64_t *count );
rc_t _KColumnReindex ( KColumn *self );
rc_t _KColumnCommitFreq ( KColumn *self, uint32_t *freq );
rc_t _KColumnSetCommitFreq ( KColumn *self, uint32_t freq );
rc_t _KColumnOpenManagerRead ( const KColumn *self, struct KDBManager const **mgr );
rc_t _KColumnOpenManagerUpdate ( KColumn *self, struct KDBManager **mgr );
rc_t _KColumnOpenParentRead ( const KColumn *self, struct KTable const **tbl );
rc_t _KColumnOpenParentUpdate ( KColumn *self, struct KTable **tbl );
rc_t _KColumnBlobAddRef ( const KColumnBlob *self );
rc_t _KColumnBlobRelease ( const KColumnBlob *self );
rc_t _KColumnCreateBlob ( KColumn *self, KColumnBlob **blob );
rc_t _KColumnOpenBlobRead ( const KColumn *self, const KColumnBlob **blob, int64_t id );
rc_t _KColumnOpenBlobUpdate ( KColumn *self, KColumnBlob **blob, int64_t id );
rc_t _KColumnBlobRead ( const KColumnBlob *self, size_t offset, void *buffer, size_t bsize, size_t *num_read, size_t *remaining );
rc_t _KColumnBlobAppend ( KColumnBlob *self, const void *buffer, size_t size );
rc_t _KColumnBlobValidate ( const KColumnBlob *self );
rc_t _KColumnBlobIdRange ( const KColumnBlob *self, int64_t *first, uint32_t *count );
rc_t _KColumnBlobAssignRange ( KColumnBlob *self, int64_t first, uint32_t count );
rc_t _KColumnBlobCommit ( KColumnBlob *self );
rc_t _KIndexAddRef ( const KIndex *self );
rc_t _KIndexRelease ( const KIndex *self );
rc_t _KDatabaseCreateIndex ( struct KDatabase *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, ... );
rc_t _KTableCreateIndex ( struct KTable *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, ... );
rc_t _KDatabaseVCreateIndex ( struct KDatabase *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, va_list args );
rc_t _KTableVCreateIndex ( struct KTable *self, KIndex **idx, KIdxType type, KCreateMode cmode, const char *name, va_list args );
rc_t _KDatabaseOpenIndexRead ( struct KDatabase const *self, const KIndex **idx, const char *name, ... );
rc_t _KTableOpenIndexRead ( struct KTable const *self, const KIndex **idx, const char *name, ... );
rc_t _KDatabaseVOpenIndexRead ( struct KDatabase const *self, const KIndex **idx, const char *name, va_list args );
rc_t _KTableVOpenIndexRead ( struct KTable const *self, const KIndex **idx, const char *name, va_list args );
rc_t _KDatabaseOpenIndexUpdate ( struct KDatabase *self, KIndex **idx, const char *name, ... );
rc_t _KTableOpenIndexUpdate ( struct KTable *self, KIndex **idx, const char *name, ... );
rc_t _KDatabaseVOpenIndexUpdate ( struct KDatabase *self, KIndex **idx, const char *name, va_list args );
rc_t _KTableVOpenIndexUpdate ( struct KTable *self, KIndex **idx, const char *name, va_list args );
bool _KIndexLocked ( const KIndex *self );
rc_t _KIndexVersion ( const KIndex *self, uint32_t *version );
rc_t _KIndexType ( const KIndex *self, KIdxType *type );
rc_t _KIndexCommit ( KIndex *self );
rc_t _KIndexConsistencyCheck ( const KIndex *self, uint32_t level, int64_t *start_id, uint64_t *id_range, uint64_t *num_keys, uint64_t *num_rows, uint64_t *num_holes );
rc_t _KIndexInsertText ( KIndex *self, bool unique, const char *key, int64_t id );
rc_t _KIndexDeleteText ( KIndex *self, const char *key );
rc_t _KIndexFindText ( const KIndex *self, const char *key, int64_t *start_id, uint64_t *id_count, int ( CC * custom_cmp ) ( const void *item, struct PBSTNode const *n, void *data ), void *data );
rc_t _KIndexFindAllText ( const KIndex *self, const char *key, rc_t ( CC * f ) ( int64_t start_id, uint64_t id_count, void *data ), void *data );
rc_t _KIndexProjectText ( const KIndex *self, int64_t id, int64_t *start_id, uint64_t *id_count, char *key, size_t kmax, size_t *actsize );
rc_t _KIndexProjectAllText ( const KIndex *self, int64_t id, rc_t ( CC * f ) ( int64_t start_id, uint64_t id_count, const char *key, void *data ), void *data );
rc_t _KIndexInsertU64 ( KIndex *self, bool unique, uint64_t key, uint64_t key_size, int64_t start_id, uint64_t id_count );
rc_t _KIndexDeleteU64 ( KIndex *self, uint64_t key );
rc_t _KIndexFindU64 ( const KIndex *self, uint64_t offset, uint64_t *key, uint64_t *key_size, int64_t *start_id, uint64_t *id_count );
rc_t _KMetadataAddRef ( const KMetadata *self );
rc_t _KMetadataRelease ( const KMetadata *self );
rc_t _KDatabaseOpenMetadataRead ( struct KDatabase const *self, const KMetadata **meta );
rc_t _KTableOpenMetadataRead ( struct KTable const *self, const KMetadata **meta );
rc_t _KColumnOpenMetadataRead ( struct KColumn const *self, const KMetadata **meta );
rc_t _KDatabaseOpenMetadataUpdate ( struct KDatabase *self, KMetadata **meta );
rc_t _KTableOpenMetadataUpdate ( struct KTable *self, KMetadata **meta );
rc_t _KColumnOpenMetadataUpdate ( struct KColumn *self, KMetadata **meta );
rc_t _KMetadataVersion ( const KMetadata *self, uint32_t *version );
rc_t _KMetadataByteOrder ( const KMetadata *self, bool *reversed );
rc_t _KMetadataRevision ( const KMetadata *self, uint32_t *revision );
rc_t _KMetadataMaxRevision ( const KMetadata *self, uint32_t *revision );
rc_t _KMetadataCommit ( KMetadata *self );
rc_t _KMetadataFreeze ( KMetadata *self );
rc_t _KMetadataOpenRevision ( const KMetadata *self, const KMetadata **meta, uint32_t revision );
rc_t _KMetadataGetSequence ( const KMetadata *self, const char *seq, int64_t *val );
rc_t _KMetadataSetSequence ( KMetadata *self, const char *seq, int64_t val );
rc_t _KMetadataNextSequence ( KMetadata *self, const char *seq, int64_t *val );
rc_t _KMDataNodeAddRef ( const KMDataNode *self );
rc_t _KMDataNodeRelease ( const KMDataNode *self );
rc_t _KMetadataOpenNodeRead ( const KMetadata *self, const KMDataNode **node, const char *path, ... );
rc_t _KMDataNodeOpenNodeRead ( const KMDataNode *self, const KMDataNode **node, const char *path, ... );
rc_t _KMetadataVOpenNodeRead ( const KMetadata *self, const KMDataNode **node, const char *path, va_list args );
rc_t _KMDataNodeVOpenNodeRead ( const KMDataNode *self, const KMDataNode **node, const char *path, va_list args );
rc_t _KMetadataOpenNodeUpdate ( KMetadata *self, KMDataNode **node, const char *path, ... );
rc_t _KMDataNodeOpenNodeUpdate ( KMDataNode *self, KMDataNode **node, const char *path, ... );
rc_t _KMetadataVOpenNodeUpdate ( KMetadata *self, KMDataNode **node, const char *path, va_list args );
rc_t _KMDataNodeVOpenNodeUpdate ( KMDataNode *self, KMDataNode **node, const char *path, va_list args );
rc_t _KMDataNodeByteOrder ( const KMDataNode *self, bool *reversed );
rc_t _KMDataNodeRead ( const KMDataNode *self, size_t offset, void *buffer, size_t bsize, size_t *num_read, size_t *remaining );
rc_t _KMDataNodeWrite ( KMDataNode *self, const void *buffer, size_t size );
rc_t _KMDataNodeAppend ( KMDataNode *self, const void *buffer, size_t size );
rc_t _KMDataNodeReadB8 ( const KMDataNode *self, void *b8 );
rc_t _KMDataNodeReadB16 ( const KMDataNode *self, void *b16 );
rc_t _KMDataNodeReadB32 ( const KMDataNode *self, void *b32 );
rc_t _KMDataNodeReadB64 ( const KMDataNode *self, void *b64 );
rc_t _KMDataNodeReadB128 ( const KMDataNode *self, void *b128 );
rc_t _KMDataNodeReadAsI16 ( const KMDataNode *self, int16_t *i );
rc_t _KMDataNodeReadAsU16 ( const KMDataNode *self, uint16_t *u );
rc_t _KMDataNodeReadAsI32 ( const KMDataNode *self, int32_t *i );
rc_t _KMDataNodeReadAsU32 ( const KMDataNode *self, uint32_t *u );
rc_t _KMDataNodeReadAsI64 ( const KMDataNode *self, int64_t *i );
rc_t _KMDataNodeReadAsU64 ( const KMDataNode *self, uint64_t *u );
rc_t _KMDataNodeReadAsF64 ( const KMDataNode *self, double *f );
rc_t _KMDataNodeReadCString ( const KMDataNode *self, char *buffer, size_t bsize, size_t *size );
rc_t _KMDataNodeWriteB8 ( KMDataNode *self, const void *b8 );
rc_t _KMDataNodeWriteB16 ( KMDataNode *self, const void *b16 );
rc_t _KMDataNodeWriteB32 ( KMDataNode *self, const void *b32 );
rc_t _KMDataNodeWriteB64 ( KMDataNode *self, const void *b64 );
rc_t _KMDataNodeWriteB128 ( KMDataNode *self, const void *b128 );
rc_t _KMDataNodeWriteCString ( KMDataNode *self, const char *str );
rc_t _KMDataNodeReadAttr ( const KMDataNode *self, const char *name, char *buffer, size_t bsize, size_t *size );
rc_t _KMDataNodeWriteAttr ( KMDataNode *self, const char *name, const char *value );
rc_t _KMDataNodeReadAttrAsI16 ( const KMDataNode *self, const char *attr, int16_t *i );
rc_t _KMDataNodeReadAttrAsU16 ( const KMDataNode *self, const char *attr, uint16_t *u );
rc_t _KMDataNodeReadAttrAsI32 ( const KMDataNode *self, const char *attr, int32_t *i );
rc_t _KMDataNodeReadAttrAsU32 ( const KMDataNode *self, const char *attr, uint32_t *u );
rc_t _KMDataNodeReadAttrAsI64 ( const KMDataNode *self, const char *attr, int64_t *i );
rc_t _KMDataNodeReadAttrAsU64 ( const KMDataNode *self, const char *attr, uint64_t *u );
rc_t _KMDataNodeReadAttrAsF64 ( const KMDataNode *self, const char *attr, double *f );
rc_t _KMDataNodeDropAll ( KMDataNode *self );
rc_t _KMDataNodeDropAttr ( KMDataNode *self, const char *attr );
rc_t _KMDataNodeDropChild ( KMDataNode *self, const char *path, ... );
rc_t _KMDataNodeVDropChild ( KMDataNode *self, const char *path, va_list args );
rc_t _KMDataNodeRenameAttr ( KMDataNode *self, const char *from, const char *to );
rc_t _KMDataNodeRenameChild ( KMDataNode *self, const char *from, const char *to );
rc_t _KDatabaseListDB ( struct KDatabase const *self, struct KNamelist **names );
rc_t _KDatabaseListTbl ( struct KDatabase const *self, struct KNamelist **names );
rc_t _KDatabaseListIdx ( struct KDatabase const *self, struct KNamelist **names );
rc_t _KTableListCol ( struct KTable const *self, struct KNamelist **names );
rc_t _KTableListIdx ( struct KTable const *self, struct KNamelist **names );
rc_t _KMDataNodeListAttr ( struct KMDataNode const *self, struct KNamelist **names );
rc_t _KMDataNodeListChildren ( struct KMDataNode const *self, struct KNamelist **names );

#ifdef __cplusplus
}
#endif

#endif /* _h_kqsh_kdb_ */
