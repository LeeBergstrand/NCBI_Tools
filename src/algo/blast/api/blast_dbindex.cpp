/* $Id: blast_dbindex.cpp 378769 2012-10-24 14:10:58Z camacho $
* ===========================================================================
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
* Author: Aleksandr Morgulis
*
*/

/// @file blast_dbindex.cpp
/// Functionality for indexed databases

#include <ncbi_pch.hpp>
#include <sstream>
#include <list>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbithr.hpp>
#include <algo/blast/core/blast_hits.h>
#include <algo/blast/core/blast_gapalign.h>
#include <algo/blast/core/blast_util.h>
#include <algo/blast/core/mb_indexed_lookup.h>

#include <objtools/blast/seqdb_reader/seqdbcommon.hpp>
#include <objtools/blast/seqdb_reader/seqdb.hpp>

#include <algo/blast/api/blast_dbindex.hpp>
#include <algo/blast/dbindex/dbindex.hpp>

#include "algo/blast/core/mb_indexed_lookup.h"

// Comment this out to continue with extensions.
// #define STOP_AFTER_PRESEARCH 1

// Comment this to suppress index-related tracing
// #define TRACE_DBINDEX 1

#ifdef TRACE_DBINDEX
#   define IDX_TRACE(_m) { std::cerr << _m << std::endl; }
#else
#   define IDX_TRACE(_m)
#endif

/** @addtogroup AlgoBlast
 *
 * @{
 */

extern "C" {

/** Get the seed search results for a give subject id and chunk number.

    @param idb_v          [I]   Database and index data.
    @param oid_i          [I]   Subject id.
    @param chunk_i        [I]   Chunk number.
    @param init_hitlist [I/O] Results are returned here.

    @return Word size used for search.
*/
static unsigned long s_MB_IdbGetResults(
        Int4 oid_i, Int4 chunk_i,
        BlastInitHitList * init_hitlist );

static int s_MB_IdbCheckOid( Int4 oid, Int4 * last_vol_oid );

static void s_MB_IdxEndSearchIndication( Int4 last_vol_id );
}

BEGIN_NCBI_SCOPE
BEGIN_SCOPE( blast )

USING_SCOPE( ncbi::objects );
USING_SCOPE( ncbi::blastdbindex );

/// Get the minimum acceptable word size to use with indexed search.
/// @return the minimum acceptable word size
int MinIndexWordSize() { return 16; }

/** No-op callback for setting concurrency state. 
    @sa DbIndexSetUsingThreadsFnType()
*/
static void NullSetUsingThreads( bool ) {}

/** No-op callback for setting the number of threads.
    @sa DbIndexSetNumThreadsFnType()
*/
static void NullSetNumThreads( size_t ) {}

/** No-op callback for setting query info. Used when index search is not enabled.
    @sa DbIndexSetQueryInfoFnType()
*/
static void NullSetQueryInfo(
        LookupTableWrap * , 
        CRef< CBlastSeqLocWrap > ) {}

/** No-op callback to run indexed search. Used when index search is not enabled.
    @sa DbIndexRunSearchFnType()
*/
static void NullRunSearch(
        BLAST_SequenceBlk * , 
        LookupTableOptions * , BlastInitialWordOptions * ) {}

/** Global pointer to the appropriate callback to set the state of concurrency. */
static DbIndexSetUsingThreadsFnType SetUsingThreadsFn = &NullSetUsingThreads;

/** Global pointer to the appropriate callback to set the number of threads. */
static DbIndexSetNumThreadsFnType SetNumThreadsFn = &NullSetNumThreads;

/** Global pointer to the appropriate callback to set query info, based
    on whether or not index search is enabled.
*/
static DbIndexSetQueryInfoFnType SetQueryInfoFn = &NullSetQueryInfo;

/** Global pointer to the appropriate callback to run indexed search, based
    on whether or not index search is enabled.
*/
static DbIndexRunSearchFnType RunSearchFn = &NullRunSearch;

//------------------------------------------------------------------------------
/** This class is responsible for loading indices and doing the actual
    seed search.

    It acts as a middle man between the blast engine and dbindex library.
*/
class CIndexedDb : public CObject
{
protected:

    CRef< CBlastSeqLocWrap > locs_wrap_; /**< Current set of unmasked query locations. */

public:

    static CRef< CIndexedDb > Index_Set_Instance; /**< Shared representation of 
                                                        currently loaded index volumes. */

    /** Object destructor. */
    virtual ~CIndexedDb();

    /** Check whether any results were reported for a given subject sequence.

        @param oid The subject sequence id
        @param last_vol_id The volume id checked just before oid
        @return 0 --- if oid was handled by indexed search but no seeds found;
                1 --- if oid was handled by indexed search and seeds were found;
                2 --- if oid was not handled by indexed search
    */
    virtual int CheckOid( Int4 oid, Int4 * last_vol_id ) = 0;

    /** Function used by threads to indicate that they are done with
        iterating over the database sequences.

        @param last_vol_id the last accessed volime index
    */
    virtual void EndSearchIndication( Int4 last_vol_id ) = 0;

    /** Run preliminary indexed search functionality.

        @param queries      Queries descriptor.
        @param locs         Unmasked intervals of queries.
        @param lut_options  Lookup table parameters, like target word size.
        @param word_options Contains window size of two-hits based search.
    */
    virtual void DoPreSearch( 
            BLAST_SequenceBlk * queries, 
            LookupTableOptions * lut_options,
            BlastInitialWordOptions *  word_options ) = 0;

    /** Set the current set of unmasked query segments.
        @param locs_wrap unmasked query segments
    */
    void SetQueryInfo( CRef< CBlastSeqLocWrap > locs_wrap )
    { locs_wrap_ = locs_wrap; }

    /** Return results corresponding to a given subject sequence and chunk.

        @param oid          [I]   The subject sequence id.
        @param chunk        [I]   The chunk number.
        @param init_hitlist [I/O] The results are returned here.

        @return Word size used for search.
    */
    virtual unsigned long GetResults( 
            CDbIndex::TSeqNum oid,
            CDbIndex::TSeqNum chunk,
            BlastInitHitList * init_hitlist ) const = 0;
};

//------------------------------------------------------------------------------
/** Index wrapper for old style MegaBLAST indexing functionality.
*/
class CIndexedDb_Old : public CIndexedDb
{
private:

    /** Type used to represent collections of search result sets. */
    typedef vector< CConstRef< CDbIndex::CSearchResults > > TResultSet;

    /** Type used to map loaded indices to subject ids. */
    typedef vector< CDbIndex::TSeqNum > TSeqMap;

    /** Find an index corresponding to the given subject id.

        @param oid The subject sequence id.
        @return Index of the corresponding index data in 
                \e this->indices_.
    */
    TSeqMap::size_type LocateIndex( CDbIndex::TSeqNum oid ) const
    {
        for( TSeqMap::size_type i = 0; i < seqmap_.size(); ++i ) {
            if( seqmap_[i] > oid ) return i;
        }

        assert( 0 );
        return 0;
    }

    TResultSet results_;    /**< Set of result sets, one per loaded index. */
    TSeqMap seqmap_;        /**< For each element of \e indices_ with index i
                                    seqmap_[i] contains one plus the last oid of
                                    that database index. */

    vector< string > index_names_;  /**< List of index volume names. */
    CRef< CDbIndex > index_;        /**< Currently loaded index */

public:

    /** Object constructor.
            
        @param indexname A string that is a comma separated list of index
                            file prefix, number of threads, first and
                            last chunks of the index.
    */
    explicit CIndexedDb_Old( const string & indexname );

    /** Check whether any results were reported for a given subject sequence.

        @note Overrides CIndexedDb::CheckOid()

        @param oid The subject sequence id.
        @return 0 --- if no seeds were found for oid;
                1 --- if seeds were found for oid;
    */
    virtual int CheckOid( Int4 oid, Int4 * )
    {
        TSeqMap::size_type i = LocateIndex( oid );
        const CConstRef< CDbIndex::CSearchResults > & results = results_[i];
        if( i > 0 ) oid -= seqmap_[i-1];
        return results->CheckResults( oid ) ? eHasResults : eNoResults;
    }

    /** Not used */
    virtual void EndSearchIndication( Int4 ) {}

private:

    /** Invoke the seed search procedure on each of the loaded indices.

        Each search is run in a separate thread. The function waits until
        all threads are complete before it returns.

        @param queries      Queries descriptor.
        @param locs         Unmasked intervals of queries.
        @param lut_options  Lookup table parameters, like target word size.
        @param word_options Contains window size of two-hits based search.
    */
    void PreSearch( 
            BLAST_SequenceBlk * queries, BlastSeqLoc * locs,
            LookupTableOptions * lut_options,
            BlastInitialWordOptions *  word_options );

public:

    /** Wrapper around PreSearch().

        Runs PreSearch() and then frees locs_wrap_.

        @note Overrides CIndexedDb::DoPreSearch().
    */
    virtual void DoPreSearch( 
            BLAST_SequenceBlk * queries, 
            LookupTableOptions * lut_options,
            BlastInitialWordOptions *  word_options )
    {
        PreSearch( 
                queries, locs_wrap_->getLocs(), 
                lut_options, word_options );
        locs_wrap_.Release();
    }

    /** Return results corresponding to a given subject sequence and chunk.

        @note Overrides CIndexedDb::GetResults().

        @param oid          [I]   The subject sequence id.
        @param chunk        [I]   The chunk number.
        @param init_hitlist [I/O] The results are returned here.

        @return Word size used for search.
    */
    virtual unsigned long GetResults( 
            CDbIndex::TSeqNum oid,
            CDbIndex::TSeqNum chunk,
            BlastInitHitList * init_hitlist ) const;
};

//------------------------------------------------------------------------------
/** Index wrapper for new style MegaBLAST indexing functionality.

    Each leaf volume of the BLAST database is indexed separately (with
    possibly multiple index volumes) or not indexed at all. For the database
    volumes that are not indexed processing is delegated back to the default
    BLAST search.
*/
class CIndexedDb_New : public CIndexedDb
{
private:

    /** Alias for a vector os strings. */
    typedef std::vector< std::string > TStrVec;

    /** Information about one leaf index volume. */
    struct SVolumeDescriptor
    {
        SIZE_TYPE start_oid;    ///< OId of the first sequence of the volume.
        SIZE_TYPE n_oids;       ///< Number of sequences in the volume.
        std::string name;       ///< Fully qualified name of the volume.
        bool has_index;         ///< 'true' if the volume is indexed.

        /** Volumes are compared by their starting ordinal ids. */
        friend bool operator<( 
                const SVolumeDescriptor & a, const SVolumeDescriptor & b )
        {
            return a.start_oid < b.start_oid;
        }

        /** This is only used for debug tracing. Print out information
            about the volume.
        */
        friend std::ostream & operator<<( 
                std::ostream & os, const SVolumeDescriptor & vd )
        {
            os << vd.name << '[' << vd.start_oid << ',' << vd.n_oids << ',' 
               << vd.has_index << ']';
            return os;
        }
    };

    /// List of leaf index volumes.
    typedef std::vector< SVolumeDescriptor > TVolList;

    /// This type captures the seeds found by search of an index volume.
    typedef CConstRef< CDbIndex::CSearchResults > TVolResults;

    /** Reference count for the volume results.

        Holds results for a given volume only while there is a search
        thread potentially in need of those results.
    */
    struct SVolResults
    {
        SVolResults() : ref_count( 0 ) {}

        TVolResults res; ///< Seed set or null.
        int ref_count;   ///< How many threads still need the result set.
    };

    /// List of reference counted result holders.
    typedef std::vector< SVolResults > TResultsHolder;

    /** Generate a list of BLAST database names from a single string.

        @param db_spec string containing space separated list of names
        @param db_names [out] resulting list of database names
    */
    static void ParseDBNames( const std::string db_spec, TStrVec & db_names );

    /** Generate a list of leaf database volumes from a list of
        database names.

        @param db_names BLAST database names
        @param db_vols [out] resulting list of leaf database volume names
    */
    static void EnumerateDbVolumes( 
            const TStrVec & db_names, TStrVec & db_vols );

    /** This is only used for debugging output. */
    static void TraceNames( const TStrVec & names )
    {
#ifdef TRACE_DBINDEX
        ITERATE( TStrVec, i, names ) { IDX_TRACE( "\t" << *i ); }
#endif
    }

    /** This is only used for debugging output. */
    void TraceVolumes( void )
    {
#ifdef TRACE_DBINDEX
        ITERATE( TVolList, i, volumes_ ) { IDX_TRACE( "\t" << *i ); }
#endif
    }

    /** Auxiliary function thet returns the oid value
        that is one more than the largest oid used so far.
    */
    SIZE_TYPE GetNextUnusedOID( void ) const;

    /** Update the seed sets, if necessary.

        If oid belongs to the volume at vol_idx, then does nothing.
        Otherwise finds the index of the volume containing oid and
        saves it in *vol_idx. Updates the reference counts of all
        volumes between the old and new values of *vol_idx, releasing
        the result sets if necessary. If the results for new volume
        are not yet available, searches the new volume and stores
        the results.

        @param oid ordinal id of the subject sequence
        @param vol_idx [in/out] index of the volume containing ordinal
                       id of the sequence last accessed by this thread;
                       updated to the index of the volume containing
                       oid
    */
    void UpdateIndex( Int4 oid, Int4 * vol_idx );

    /* Add index volumes corresponding to the given database volume.

       If an index exists for the given database volume, adds information
       about all corresponding index volumes to volumes_. Otherwise a single
       entry is added to volumes_ with has_index flag set to 'false'.

       @param vol_name database volume name
       @param idx_not_resolved [out] returns 'true' if the database volume
                               has no associated index
    */
    void AddIndexInfo( const std::string & vol_name, bool & idx_not_resolved );

    /** Find a volume containing the given subject ordinal id. */
    TVolList::const_iterator FindVolume( Int4 oid ) const
    {
        SVolumeDescriptor s = { oid };
        TVolList::const_iterator r(
                std::upper_bound( volumes_.begin(), volumes_.end(), s ) );
        ASSERT( r != volumes_.begin() );
        return --r;
    }

    TVolList volumes_;                  ///< index volume descriptors
    TResultsHolder results_holder_;     ///< reference counted seed set holders
    CFastMutex mtx_;                    ///< mutex used for thread sync
    BLAST_SequenceBlk * queries_;       ///< query data (from BLAST)
    CDbIndex::SSearchOptions sopt_;     ///< common search parameters
    bool multiple_threads_;             /**< flag indicating that multithreading
                                             is in effect */
    size_t n_threads_;                  ///< number of search threads running

public:

    /** Object constructor.
            
        If all database indices were resolved successfully, then 'false' is
        returned in partial; otherwise 'true' is returned.

        @param indexname MegaBLAST database name (can be a space separated
                         list of databases)
        @param partial [O] returns 'true' if not all database indices were
                       resolved
    */
    explicit CIndexedDb_New( const string & indexname, bool & partial );

    /** Object destructor.
    */
    virtual ~CIndexedDb_New();

    /** Check whether any results were reported for a given subject sequence.

        @note Overrides CIndexedDb::CheckOid().

        @param oid The subject sequence id.
        @param last_vol_id The volume id checked just before oid
        @return 0 --- if oid was handled by indexed search but no seeds found;
                1 --- if oid was handled by indexed search and seeds were found;
                2 --- if oid was not handled by indexed search
    */
    virtual int CheckOid( Int4 oid, Int4 * last_vol_id );

    /** Function used by threads to indicate that they are done with
        iterating over the database sequences.

        @param last_vol_id the last accessed volime index
    */
    virtual void EndSearchIndication( Int4 last_vol_id );

    /** Run preliminary indexed search functionality.

        @note Overrides CIndexedDb::DoPreSearch().

        @param queries      Queries descriptor.
        @param locs         Unmasked intervals of queries.
        @param lut_options  Lookup table parameters, like target word size.
        @param word_options Contains window size of two-hits based search.
    */
    virtual void DoPreSearch( 
            BLAST_SequenceBlk * queries, 
            LookupTableOptions * lut_options,
            BlastInitialWordOptions *  word_options );

    /** Return results corresponding to a given subject sequence and chunk.

        @note Overrides CIndexedDb::GetResults().

        @param oid          [I]   The subject sequence id.
        @param chunk        [I]   The chunk number.
        @param init_hitlist [I/O] The results are returned here.

        @return Word size used for search.
    */
    virtual unsigned long GetResults( 
            CDbIndex::TSeqNum oid,
            CDbIndex::TSeqNum chunk,
            BlastInitHitList * init_hitlist ) const;

    /** Set the concurrency status.

        @param multiple_threads 'true' if concurrent search is being performed;
                                'false' otherwise
    */
    void SetMultipleThreads( bool multiple_threads )
    { 
        IDX_TRACE( "setting multiple threads to " << 
                   (multiple_threads ? "true" : "false") );
        multiple_threads_ = multiple_threads;
        if( multiple_threads_ ) n_threads_ = 0;
    }

    /** Set the number of threads used for concurrent search.

        @param n_threads number of search threads.
    */
    void SetNumThreads( size_t n_threads )
    {
        ASSERT( n_threads > 1 );
        IDX_TRACE( "setting number of search threads to " << n_threads );
        n_threads_ = n_threads;
    }
};

//------------------------------------------------------------------------------
CRef< CIndexedDb > CIndexedDb::Index_Set_Instance;

//------------------------------------------------------------------------------
/// Run indexed search.
/// @param queries query data
/// @param lut_options lookup table parameters
/// @param word_options word parameters
static void IndexedDbRunSearch(
        BLAST_SequenceBlk * queries, 
        LookupTableOptions * lut_options, 
        BlastInitialWordOptions * word_options )
{
    CIndexedDb * idb( CIndexedDb::Index_Set_Instance.GetPointerOrNull() );
    if( idb == 0 ) return;
    idb->DoPreSearch( queries, lut_options, word_options );
}

//------------------------------------------------------------------------------
/// Set state of concurrency in the index structure.
/// @param multiple_threads 'true' if multiple search threads are used;
///                         'false' otherwise
static void IndexedDbSetUsingThreads( bool multiple_threads )
{
    CIndexedDb * idb( CIndexedDb::Index_Set_Instance.GetPointerOrNull() );
    if( idb == 0 ) return;
    CIndexedDb_New * idbn( dynamic_cast< CIndexedDb_New * >( idb ) );
    if ( idbn == 0 ) return;
    ASSERT( idbn != 0 );
    idbn->SetMultipleThreads( multiple_threads );
}

//------------------------------------------------------------------------------
/// Set the number of concurrent search threads in the index structure.
/// @param n_threads number of concurrent search threads.
static void IndexedDbSetNumThreads( size_t n_threads )
{
    CIndexedDb * idb( CIndexedDb::Index_Set_Instance.GetPointerOrNull() );
    if( idb == 0 ) return;
    CIndexedDb_New * idbn( dynamic_cast< CIndexedDb_New * >( idb ) );
    if ( idbn == 0 ) return;
    ASSERT( idbn != 0 );
    idbn->SetNumThreads( n_threads );
}

//------------------------------------------------------------------------------
/// Set information about unmasked query segments.
/// @param lt_wrap lookup table information to update
/// @param locs_wrap set of unmasked query segments
static void IndexedDbSetQueryInfo(
        LookupTableWrap * lt_wrap, 
        CRef< CBlastSeqLocWrap > locs_wrap )
{
    CIndexedDb * idb( CIndexedDb::Index_Set_Instance.GetPointerOrNull() );
    if( idb == 0 ) return;
    lt_wrap->read_indexed_db = (void *)(&s_MB_IdbGetResults);
    lt_wrap->check_index_oid = (void *)(&s_MB_IdbCheckOid);
    lt_wrap->end_search_indication = (void *)(&s_MB_IdxEndSearchIndication);
    idb->SetQueryInfo( locs_wrap );
}

//------------------------------------------------------------------------------
void CIndexedDb_New::ParseDBNames( 
        const std::string db_spec, TStrVec & db_names )
{
    static const char * SEP = " ";

    string::size_type pos( 0 ), pos1( 0 );

    while( pos1 != string::npos ) {
        pos1 = db_spec.find_first_of( SEP, pos );
        db_names.push_back( db_spec.substr( pos, pos1 - pos ) );
        pos = pos1 + 1;
    }
}

//------------------------------------------------------------------------------
void CIndexedDb_New::EnumerateDbVolumes( 
        const TStrVec & db_names, TStrVec & db_vols )
{
    CSeqDB db( db_names, CSeqDB::eNucleotide, 0, 0, false );
    db.FindVolumePaths( db_vols, true );
}

//------------------------------------------------------------------------------
SIZE_TYPE CIndexedDb_New::GetNextUnusedOID( void ) const
{
    if( !volumes_.empty() ) {
        const SVolumeDescriptor & vd( *volumes_.rbegin() );
        return vd.start_oid + vd.n_oids;
    }
    else return 0;
}

//------------------------------------------------------------------------------
void CIndexedDb_New::AddIndexInfo( 
        const std::string & vol_name, bool & partial )
{
    bool idx_not_resolved( false );
    CSeqDB db( vol_name, CSeqDB::eNucleotide, 0, 0, false );
    size_t dbnseq( (size_t)db.GetNumOIDs() );
    CRef< CIndexSuperHeader_Base > shdr;
    
    try {
        shdr.Reset( GetIndexSuperHeader( vol_name + ".shd" ) );
    }
    catch( CException & e ) {
        ERR_POST( 
            Info << "index superheader for volume " << vol_name 
                 << " was not loaded (" << e.what() << ")" );
        idx_not_resolved = true;
    }

    if( !idx_not_resolved && shdr->GetNumSeq() != dbnseq ) {
        ERR_POST( 
                Error << "numbers of OIDs reported by the database and "
                      << "by the index do not match. Index for volume " 
                      << vol_name << " will not be used" );
        idx_not_resolved = true;
    }

    if( !idx_not_resolved ) {
        size_t curr_vols_size( volumes_.size() );
        size_t total_idxvol_oids( 0 );

        for( size_t i( 0 ), e( shdr->GetNumVol() ); i < e; ++i ) {
            std::string name( SeqDB_ResolveDbPath(
                        CIndexSuperHeader_Base::GenerateIndexVolumeName( 
                            vol_name, i ) ) );

            if( name.empty() ) {
                ERR_POST( 
                        Error << "index volume " << name
                              << " not resolved; index will not be used for "
                              << vol_name );
                idx_not_resolved = true;
            }

            if( !idx_not_resolved ) {
                size_t idxvol_oids( GetIdxVolNumOIDs( name ) );

                if( idxvol_oids == 0 ) {
                    idx_not_resolved = true;
                    ERR_POST(
                            Error << "index volume " << name
                                  << " reports no sequences; index will "
                                  << "not be used for " << vol_name );
                }
                else {
                    SVolumeDescriptor vd = {
                        GetNextUnusedOID(), idxvol_oids, name, true };
                    volumes_.push_back( vd );
                    total_idxvol_oids += idxvol_oids;
                }
            }
            
            if( idx_not_resolved ) {
                volumes_.resize( curr_vols_size );
                break;
            }
        }

        if( !idx_not_resolved && dbnseq != total_idxvol_oids ) {
            ERR_POST(
                    Error << "total of oids reported by index volumes ("
                          << total_idxvol_oids << ") does not match "
                          << "the number of oids reported by the superheader ("
                          << dbnseq << "); index will not be used for "
                          << vol_name );
            volumes_.resize( curr_vols_size );
            idx_not_resolved = true;
        }
    }

    partial = (partial || idx_not_resolved);

    if( idx_not_resolved ) {
        SVolumeDescriptor vd = { GetNextUnusedOID(), dbnseq, vol_name, false };
        volumes_.push_back( vd );
        return;
    }
}

//------------------------------------------------------------------------------
CIndexedDb_New::CIndexedDb_New( const string & indexname, bool & partial )
    : queries_( 0 ), multiple_threads_( false ), n_threads_( 1 )
{
    // ENABLE_IDX_TRACE;
    IDX_TRACE( "creating new style CIndexedDb object" );
    partial = false;

    // Enumerate the databases.
    //
    IDX_TRACE( "db spec given: " << indexname );
    TStrVec db_names;
    ParseDBNames( indexname, db_names );
    IDX_TRACE( "list of databases:" );
    TraceNames( db_names );

    // Enumerate primitive database volumes.
    //
    TStrVec db_vol_names;
    EnumerateDbVolumes( db_names, db_vol_names );
    IDX_TRACE( "list of database volumes in order:" );
    TraceNames( db_vol_names );

    // Populate volume information for each resolved database volume.
    //
    ITERATE( TStrVec, dbvi, db_vol_names ) { AddIndexInfo( *dbvi, partial ); }
    IDX_TRACE( "final index volume list:" );
    TraceVolumes();

    // Check if any volume has index. If not, do not use indexing.
    //
    {
        bool has_index( false );

        ITERATE( TVolList, i, volumes_ ) 
        { 
            if( i->has_index ) {
                has_index = true;
                break;
            }
        }

        if( !has_index ) {
            NCBI_THROW( CDbIndex_Exception, eBadOption,
                        "no database volume has an index" );
        }
    }

    // Initialize the results contexts.
    //
    results_holder_.resize( volumes_.size() );
}

//------------------------------------------------------------------------------
CIndexedDb_New::~CIndexedDb_New()
{
    IDX_TRACE( "destroying new style CIndexedDb object" );
}

//------------------------------------------------------------------------------
void CIndexedDb_New::UpdateIndex( Int4 oid, Int4 * vol_idx_p )
{
    Int4 & vol_idx( *vol_idx_p );
    Int4 new_vol_idx;
    bool find_volume( true );

    if( vol_idx != LAST_VOL_IDX_INIT ) {
        const SVolumeDescriptor & vd( volumes_[vol_idx] );
        if( vd.start_oid + vd.n_oids > (SIZE_TYPE)oid ) find_volume = false;
    }

    if( !find_volume ) return;
    TVolList::const_iterator vi( FindVolume( oid ) );
    new_vol_idx = vi - volumes_.begin();
    if( !vi->has_index ) { vol_idx = new_vol_idx; return; }
    CFastMutexGuard lock( mtx_ );
    SVolResults & res( results_holder_[new_vol_idx] );
    Int4 min_vol_idx( vol_idx == -1 ? 0 : vol_idx );

    if( res.ref_count <= 0 ) {
        res.ref_count += n_threads_;
        IDX_TRACE( "loading volume "  << new_vol_idx << ": " << vi->name );
        ASSERT( vi->has_index );
        CRef< CDbIndex > index( CDbIndex::Load( vi->name ) );
        
        if( index == 0 ) {
            std::ostringstream os;
            os << "CIndexedDb: could not load index volume: " << vi->name;
            NCBI_THROW( CIndexedDbException, eIndexInitError, os.str() );
        }

        IDX_TRACE( "searching volume " << vi->name );
        res.res = index->Search( queries_, locs_wrap_->getLocs(), sopt_ );
        IDX_TRACE( "results loaded for " << vi->name );
    }

    for( ; min_vol_idx < new_vol_idx; ++min_vol_idx ) {
        if( --results_holder_[min_vol_idx].ref_count == 0 ) {
            results_holder_[min_vol_idx].res.Reset( 0 );
            IDX_TRACE( "unloaded results for volume " << 
                       volumes_[min_vol_idx].name );
        }
    }

    vol_idx = new_vol_idx;
}

//------------------------------------------------------------------------------
int CIndexedDb_New::CheckOid( Int4 oid, Int4 * last_vol_idx )
{
    if( *last_vol_idx == LAST_VOL_IDX_NULL ) {
        TVolList::const_iterator vi( FindVolume( oid ) );
        if( vi->has_index ) return eHasResults;
        else return eNotIndexed;
    }

    UpdateIndex( oid, last_vol_idx );
    TVolList::const_iterator vi( volumes_.begin() + *last_vol_idx );
    if( !vi->has_index ) return eNotIndexed;
    oid -= vi->start_oid;
    return results_holder_[*last_vol_idx].res->CheckResults( oid ) ?
                eHasResults : eNoResults;
}

//------------------------------------------------------------------------------
void CIndexedDb_New::EndSearchIndication( Int4 last_vol_idx )
{
    CFastMutexGuard lock( mtx_ );
    if( last_vol_idx == LAST_VOL_IDX_INIT ) last_vol_idx = 0;

    for( Int4 i( last_vol_idx ); i < (Int4)volumes_.size(); ++i ) {
        if( --results_holder_[i].ref_count == 0 ) {
            results_holder_[i].res.Reset( 0 );
            IDX_TRACE( "unloaded results for volume " << volumes_[i].name );
        }
    }
}

//------------------------------------------------------------------------------
void CIndexedDb_New::DoPreSearch( 
        BLAST_SequenceBlk * queries, LookupTableOptions * lut_options, 
        BlastInitialWordOptions * word_options )
{
    queries_ = queries;
    sopt_.word_size = lut_options->word_size;
    sopt_.two_hits = word_options->window_size;
    IDX_TRACE( "set word size to " << sopt_.word_size );
    IDX_TRACE( "set two_hits to " << sopt_.two_hits );
}

//------------------------------------------------------------------------------
unsigned long CIndexedDb_New::GetResults( 
        CDbIndex::TSeqNum oid, CDbIndex::TSeqNum chunk, 
        BlastInitHitList * init_hitlist ) const
{
    TVolList::const_iterator vi( FindVolume( oid ) );
    ASSERT( vi->start_oid <= oid );
    ASSERT( vi->start_oid + vi->n_oids > oid );
    ASSERT( vi->has_index );
    oid -= vi->start_oid;
    BlastInitHitList * res( 0 );
    const TVolResults & vr( results_holder_[vi - volumes_.begin()].res );
    ASSERT( vr != 0 );

    if( (res = vr->GetResults( oid, chunk )) != 0 ) {
        BlastInitHitListMove( init_hitlist, res );
        return vr->GetWordSize();
    }
    else {
        BlastInitHitListReset( init_hitlist );
        return 0;
    }
}

//------------------------------------------------------------------------------
CIndexedDb_Old::CIndexedDb_Old( const string & indexnames )
{
    if( !indexnames.empty() ) {
        vector< string > dbnames;
        string::size_type start = 0, end = 0;

        // Interpret indexname as a space separated list of database names.
        //
        while( start != string::npos ) {
            end = indexnames.find_first_of( " ", start );
            dbnames.push_back( indexnames.substr( start, end - start ) );
            start = indexnames.find_first_not_of( " ", end );
        }

        std::sort( dbnames.begin(), dbnames.end(), &SeqDB_CompareVolume );

        for( vector< string >::const_iterator dbni = dbnames.begin();
                dbni != dbnames.end(); ++dbni ) {
            const string & indexname = *dbni;

            // Parse the indexname as a comma separated list
            unsigned long start_vol = 0, stop_vol = 99;
            start = 0;
            end = indexname.find_first_of( ",", start );
            string index_base = indexname.substr( start, end );
            start = end + 1;
    
            if( start < indexname.length() && end != string::npos ) {
                end = indexname.find_first_of( ",", start );
                start = end + 1;
    
                if( start < indexname.length() && end != string::npos ) {
                    end = indexname.find_first_of( ",", start );
                    string start_vol_str = 
                        indexname.substr( start, end - start );
    
                    if( !start_vol_str.empty() ) {
                        start_vol = atoi( start_vol_str.c_str() );
                    }
    
                    start = end + 1;
    
                    if( start < indexname.length() && end != string::npos ) {
                        end = indexname.find_first_of( ",", start );
                        string stop_vol_str = 
                            indexname.substr( start, end - start);
    
                        if( !stop_vol_str.empty() ) {
                            stop_vol = atoi( stop_vol_str.c_str() );
                        }
                    }
                }
            }
    
            if( start_vol <= stop_vol ) {
                long last_i = -1;
    
                for( long i = start_vol; (unsigned long)i <= stop_vol; ++i ) {
                    ostringstream os;
                    os << index_base << "." << setw( 2 ) << setfill( '0' )
                       << i << ".idx";
                    string name = SeqDB_ResolveDbPath( os.str() );
    
                    if( !name.empty() ){
                        if( i - last_i > 1 ) {
                            for( long j = last_i + 1; j < i; ++j ) {
                                ERR_POST( Error << "Index volume " 
                                                << j << " not resolved." );
                            }
                        }
    
                        index_names_.push_back( name );
                        last_i = i;
                    }
                }
            }
        }
    }

    if( index_names_.empty() ) {
        string msg("no index file specified or index '");
        msg += indexnames + "*' not found.";
        NCBI_THROW(CDbIndex_Exception, eBadOption, msg);
    }
}

//------------------------------------------------------------------------------
CIndexedDb::~CIndexedDb()
{
}

//------------------------------------------------------------------------------
void CIndexedDb_Old::PreSearch( 
        BLAST_SequenceBlk * queries, BlastSeqLoc * locs,
        LookupTableOptions * lut_options , 
        BlastInitialWordOptions * word_options )
{
    CDbIndex::SSearchOptions sopt;
    sopt.word_size = lut_options->word_size;
    sopt.two_hits = word_options->window_size;

    for( vector< string >::size_type v = 0; 
            v < index_names_.size(); v += 1 ) {
        CRef< CDbIndex > index;
        string result;

        try { index = CDbIndex::Load( index_names_[v] ); }
        catch( CException & e ) { result = e.what(); }

        if( index == 0 ) { 
            NCBI_THROW( CIndexedDbException, eIndexInitError,
                    string( "CIndexedDb: could not load index" ) +
                    index_names_[v] + ": " + result );
        }

        index_ = index;
        results_.push_back( CConstRef< CDbIndex::CSearchResults >( null ) );
        CDbIndex::TSeqNum s = seqmap_.empty() ? 0 : *seqmap_.rbegin();
        seqmap_.push_back( s + (index->StopSeq() - index->StartSeq()) );
        CConstRef< CDbIndex::CSearchResults > & results = results_[v];
        results = index_->Search( queries, locs, sopt );
    }
}

//------------------------------------------------------------------------------
unsigned long CIndexedDb_Old::GetResults( 
        CDbIndex::TSeqNum oid, CDbIndex::TSeqNum chunk, 
        BlastInitHitList * init_hitlist ) const
{
    BlastInitHitList * res = 0;
    TSeqMap::size_type i = LocateIndex( oid );
    const CConstRef< CDbIndex::CSearchResults > & results = results_[i];
    if( i > 0 ) oid -= seqmap_[i-1];

    if( (res = results->GetResults( oid, chunk )) != 0 ) {
        BlastInitHitListMove( init_hitlist, res );
        return results->GetWordSize();
    }else {
        BlastInitHitListReset( init_hitlist );
        return 0;
    }
}

//------------------------------------------------------------------------------
std::string DbIndexInit( 
        const string & indexname, bool old_style, bool & partial )
{
    std::string result;
    partial = false;

    if( !old_style ) {
        try {
            ERR_POST( Info << "trying to load new style index at " 
                           << indexname );
            CIndexedDb::Index_Set_Instance.Reset(
                    new CIndexedDb_New( indexname, partial ) );

            if( CIndexedDb::Index_Set_Instance != 0 ) {
                ERR_POST( Info << "new style index loaded" );

                if( partial ) {
                    ERR_POST( Info << "some volumes are not resolved" );
                }

                return "";
            }
            else return "index allocation error";
        }
        catch( CException & e ) {
            ERR_POST( Info << "new style index failed to load" );
            result = e.what();
        }
    }

    try{
        ERR_POST( Info << "trying to load old style index at "
                       << indexname );
        CIndexedDb::Index_Set_Instance.Reset(
                new CIndexedDb_Old( indexname ) );

        if( CIndexedDb::Index_Set_Instance != 0 ) {
            ERR_POST( Info << "old style index loaded" );
            return "";
        }
        else return "index allocation error";
    }
    catch( CException & e ) {
        ERR_POST( Info << "old style index failed to load" );
        result += '\n' + e.what();
    }

    return result;
}

//------------------------------------------------------------------------------
void SetUpDbIndexCallbacks( void )
{
    SetUsingThreadsFn = &IndexedDbSetUsingThreads;
    SetNumThreadsFn = &IndexedDbSetNumThreads;
    SetQueryInfoFn = &IndexedDbSetQueryInfo;
    RunSearchFn = &IndexedDbRunSearch;
}

//------------------------------------------------------------------------------
void ClearDbIndexCallbacks( void )
{
    SetUsingThreadsFn = NullSetUsingThreads;
    SetNumThreadsFn = NullSetNumThreads;
    SetQueryInfoFn = NullSetQueryInfo;
    RunSearchFn = NullRunSearch;
}

//------------------------------------------------------------------------------
DbIndexSetUsingThreadsFnType GetDbIndexSetUsingThreadsFn() 
{ return SetUsingThreadsFn; }

DbIndexSetNumThreadsFnType GetDbIndexSetNumThreadsFn() 
{ return SetNumThreadsFn; }

DbIndexSetQueryInfoFnType GetDbIndexSetQueryInfoFn() { return SetQueryInfoFn; }
DbIndexRunSearchFnType GetDbIndexRunSearchFn() { return RunSearchFn; }

END_SCOPE( blast )
END_NCBI_SCOPE

USING_SCOPE( ncbi );
USING_SCOPE( ncbi::blast );

extern "C" {

//------------------------------------------------------------------------------
static void s_MB_IdxEndSearchIndication( Int4 last_vol_id )
{
    return CIndexedDb::Index_Set_Instance->EndSearchIndication( last_vol_id );
}

//------------------------------------------------------------------------------
static int s_MB_IdbCheckOid( Int4 oid, Int4 * last_vol_id )
{
    _ASSERT( oid >= 0 );
    return CIndexedDb::Index_Set_Instance->CheckOid( oid, last_vol_id );
}

//------------------------------------------------------------------------------
static unsigned long s_MB_IdbGetResults(
        Int4 oid_i, Int4 chunk_i,
        BlastInitHitList * init_hitlist )
{
    _ASSERT( oid_i >= 0 );
    _ASSERT( chunk_i >= 0 );
    _ASSERT( init_hitlist != 0 );

    CDbIndex::TSeqNum oid = (CDbIndex::TSeqNum)oid_i;
    CDbIndex::TSeqNum chunk = (CDbIndex::TSeqNum)chunk_i;

    return CIndexedDb::Index_Set_Instance->GetResults( 
            oid, chunk, init_hitlist );
}

} /* extern "C" */

/* @} */
