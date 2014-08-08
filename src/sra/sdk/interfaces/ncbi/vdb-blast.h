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

#ifndef _h_ncbi_vdb_blast_
#define _h_ncbi_vdb_blast_

#ifndef _h_vdb_extern_
#include <vdb/extern.h>
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * defines
 */
#ifdef __cplusplus
#define VDB_BLAST_DECLARE( cls ) \
    struct cls
#else
#define VDB_BLAST_DECLARE( cls ) \
    typedef struct cls cls
#endif


/*--------------------------------------------------------------------------
 * status codes
 *  NB - TO BE COMPLETED DURING DEVELOPMENT OF TESTS AND INITIAL CODE
 */
enum
{
    eVdbBlastNoErr,
    eVdbBlastErr,
    eVdbBlastMemErr,
    eVdbBlastRunErr,
    eVdbBlastEndOfSequence,
    eVdbBlastChunkedSequence, /* can't hand out direct pointer into cache */
    eVdbBlastTooExpensive
};


/*------------------------------------------------------------------------------
 * VdbBlastMgr
 */
VDB_BLAST_DECLARE ( VdbBlastMgr );


/* Init
 *  initialize library
 *
 *  "status" [ OUT ] - return parameter for status code
 */
VDB_EXTERN VdbBlastMgr* CC VdbBlastInit ( uint32_t *status );


/* AddRef
 *  attach a reference to existing object
 * Release
 *  drop reference to object
 *  deletes object when last reference is gone
 */
VDB_EXTERN VdbBlastMgr* CC VdbBlastMgrAddRef ( VdbBlastMgr *self );
VDB_EXTERN void CC VdbBlastMgrRelease ( VdbBlastMgr *self );


/* FUNCTIONS TO CONTROL VDB-LIBRARIES LOG GENERATION */

/* KLogLevelSet
 * set log level of VDB-libraries
 *
 * returns status code
 */
VDB_EXTERN uint32_t CC VdbBlastMgrKLogLevelSetInfo ( const VdbBlastMgr *self );
VDB_EXTERN uint32_t CC VdbBlastMgrKLogLevelSetWarn ( const VdbBlastMgr *self );

/* for level description see klib/log.h */
VDB_EXTERN uint32_t CC VdbBlastMgrKLogLevelSet ( const VdbBlastMgr *self,
    uint32_t level );

VDB_EXTERN void CC VdbBlastMgrKStsLevelSet ( const VdbBlastMgr *self,
    uint32_t level );

/* KLogHandlerSet, KLogLibHandlerSet, KStsHandlerSet, KOutHandlerSet
 * set logging output handlers for different channels
 *
 * returns status code
 */
VDB_EXTERN uint32_t CC VdbBlastMgrKLogHandlerSetStdOut
    ( const VdbBlastMgr *self );
VDB_EXTERN uint32_t CC VdbBlastMgrKLogLibHandlerSetStdOut
    ( const VdbBlastMgr *self );
VDB_EXTERN uint32_t CC VdbBlastMgrKStsHandlerSetStdOut
    ( const VdbBlastMgr *self );
VDB_EXTERN uint32_t CC VdbBlastMgrKOutHandlerSetStdOut
    ( const VdbBlastMgr *self );

VDB_EXTERN uint32_t CC VdbBlastMgrKLogHandlerSetStdErr
    ( const VdbBlastMgr *self );
VDB_EXTERN uint32_t CC VdbBlastMgrKLogLibHandlerSetStdErr
    ( const VdbBlastMgr *self );
VDB_EXTERN uint32_t CC VdbBlastMgrKStsHandlerSetStdErr
    ( const VdbBlastMgr *self );
VDB_EXTERN uint32_t CC VdbBlastMgrKOutHandlerSetStdErr
    ( const VdbBlastMgr *self );


/*  upon success, the writer will return code 0 */
typedef uint32_t ( CC * VdbBlastKWrtWriter )
    ( void * self, const char * buffer, size_t bufsize, size_t * num_writ );

VDB_EXTERN uint32_t CC VdbBlastMgrKLogHandlerSet ( const VdbBlastMgr *self,
    VdbBlastKWrtWriter writer, void *data );
VDB_EXTERN uint32_t CC VdbBlastMgrKLogLibHandlerSet ( const VdbBlastMgr *self,
    VdbBlastKWrtWriter writer, void *data );
VDB_EXTERN uint32_t CC VdbBlastMgrKOutHandlerSet ( const VdbBlastMgr *self,
    VdbBlastKWrtWriter writer, void *data );


/* KDbgSetString 
 * set a string used to turn on debug messages
 *
 * returns status code
 */
VDB_EXTERN uint32_t CC VdbBlastMgrKDbgSetString ( const VdbBlastMgr *self,
    const char *string );

/* KConfigPrint
 * print current configuration to the output handler
 *
 * returns status code
 */
VDB_EXTERN uint32_t VdbBlastMgrKConfigPrint ( const VdbBlastMgr *self );

/* OTHER FUNCTIONS FOR CONFIGURING SESSION BEHAVIOR */


/*------------------------------------------------------------------------------
 * VdbBlastRunSet
 */
VDB_BLAST_DECLARE ( VdbBlastRunSet );


/* MakeRunSet
 *  create an empty run set
 *
 *  "status" [ OUT ] - return parameter for status code
 *
 *  "min_read_length" [ IN ] - min-length parameter for filtering
 *
 *  "protein" [ IN ] - if true, run set will contain only protein sequences
 *
 *  returns NULL upon any error
 */
VDB_EXTERN VdbBlastRunSet* CC VdbBlastMgrMakeRunSet ( const VdbBlastMgr *self,
    uint32_t *status, uint32_t min_read_length, bool protein );


/* AddRef
 *  attach a reference to existing object
 * Release
 *  drop reference to object
 *  deletes object when last reference is gone
 */
VDB_EXTERN VdbBlastRunSet* CC VdbBlastRunSetAddRef ( VdbBlastRunSet *self );
VDB_EXTERN void CC VdbBlastRunSetRelease ( VdbBlastRunSet *self );


/* AddRun
 *  add a run by accession or path
 *
 *  "rundesc" [ IN ] - NUL-terminated UTF-8 string
 *  giving run accession or path
 *
 *  returns status code
 */
VDB_EXTERN uint32_t CC VdbBlastRunSetAddRun ( VdbBlastRunSet *self,
    const char *rundesc );


/* GetNumSequences
 *  returns the total number of biological sequences in run set
 * GetNumSequencesApprox
 *  returns a calculated estimate of the number of biological
 *  sequences in run set. may be used if the exact count function
 *  returns "eVdbBlastTooExpensive".
 */
VDB_EXTERN uint64_t CC VdbBlastRunSetGetNumSequences
    ( const VdbBlastRunSet *self, uint32_t *status );
VDB_EXTERN uint64_t CC VdbBlastRunSetGetNumSequencesApprox
    ( const VdbBlastRunSet *self );


/* GetTotalLength
 *  returns the total number of bases in run set
 * GetTotalLengthApprox
 *  returns a calculated estimate of the total number
 *  of bases in run set. may be used if the exact length function
 *  returns "eVdbBlastTooExpensive".
 */
VDB_EXTERN uint64_t CC VdbBlastRunSetGetTotalLength
    ( const VdbBlastRunSet *self, uint32_t *status );
VDB_EXTERN uint64_t CC VdbBlastRunSetGetTotalLengthApprox
    ( const VdbBlastRunSet *self );


/* GetMinSeqLen
 * GetMaxSeqLen
 * GetAvgSeqLen
 *  returns metrics of biological reads
 *  may be the result of estimated calculations ( see above )
 */
VDB_EXTERN uint64_t CC VdbBlastRunSetGetMaxSeqLen
    ( const VdbBlastRunSet *self );
VDB_EXTERN uint64_t CC VdbBlastRunSetGetMinSeqLen
    ( const VdbBlastRunSet *self );
VDB_EXTERN uint64_t CC VdbBlastRunSetGetAvgSeqLen
    ( const VdbBlastRunSet *self );


/* GetName
 *  returns the total number of bytes needed for name
 *  which, if > bsize, means insufficient buffer error
 *  and if == bsize, means the name is NOT NUL-terminated
 *  otherwise, the buffer is NUL-terminated as well.
 */
VDB_EXTERN size_t CC VdbBlastRunSetGetName ( const VdbBlastRunSet *self,
    uint32_t *status, char *name_buffer, size_t bsize );


/* IsProtein
 *  return the boolean value given when the set was created
 */
VDB_EXTERN bool CC VdbBlastRunSetIsProtein ( const VdbBlastRunSet *self );


#if NOT_DEFERRED
/* LastUpdatedDate
 *  request the modification date of the SEQUENCE data
 *  or the READ DESCRIPTOR data, whichever is more recent
 */
time_t VdbBlastRunSetLastUpdatedDate ( const VdbBlastRunSet *self );
#endif

/* GetReadName
 */
VDB_EXTERN size_t CC VdbBlastRunSetGetReadName ( const VdbBlastRunSet *self,
    uint64_t read_id, char *name_buffer, size_t bsize );

/* GetReadId
 * Inverse function to GetReadName
 *
 *  returns status code
 */
VDB_EXTERN uint32_t CC VdbBlastRunSetGetReadId ( const VdbBlastRunSet *self,
    const char *name_buffer, size_t bsize, uint64_t *read_id );

#if NOT_DEFERRED
/* GetReadLength
 *  this API bothers me, because it has the potential of blowing up the cache
 *  and subverts the guarantee of the readers exclusive use of caches.
 *  Note that by getting a read and its length using the "Data" functions
 *  below, you can get the length as cheaply as you would here, at least in
 *  VDB-2.
 */
uint64_t VdbBlastRunSetGetReadLength ( const VdbBlastRunSet *self,
    uint64_t read_id );
#endif


/*------------------------------------------------------------------------------
 * VdbBlast2naReader
 */
VDB_BLAST_DECLARE ( VdbBlast2naReader );


/* Make2naReader
 *  create a reader onto the run set
 *
 *  "status" [ OUT ] - return parameter for status code
 *
 *  "initial_read_id" [ IN ] - starting position for reader
 */
VDB_EXTERN VdbBlast2naReader* CC VdbBlastRunSetMake2naReader
    ( const VdbBlastRunSet *self, uint32_t *status, uint64_t initial_read_id );


/* AddRef
 *  attach a reference to existing object
 * Release
 *  drop reference to object
 *  deletes object when last reference is gone
 */
VDB_EXTERN VdbBlast2naReader* CC VdbBlast2naReaderAddRef
    ( VdbBlast2naReader *self );
VDB_EXTERN void CC VdbBlast2naReaderRelease ( VdbBlast2naReader *self );


/* Read
 *  read a single sequence in packed 2na
 *
 *  "status" [ OUT ] - return parameter for status code
 *
 *  "read_id" [ OUT ] - id of read that was read
 *
 *  "buffer" [ OUT ] and "buffer_size" [ IN ] - externally provided buffer
 *  for single sequence in packed 2na, where buffer_size is in bytes.
 *
 *  returns number of bases read or 0 if no further bases could be read
 */
VDB_EXTERN uint64_t CC VdbBlast2naReaderRead ( const VdbBlast2naReader *self,
    uint32_t *status, uint64_t *read_id,
    uint8_t *buffer, size_t buffer_size );


/* Data
 *  read as many sequences as are available
 *
 *  "status" [ OUT ] - return parameter for status code
 *
 *  "buffer" [ OUT ] and "buffer_len" [ IN ] - externally provided buffer
 *  for biological read sequences
 *
 *  returns number of sequences available in "buffer"
 *  or 0 if nothing could be read
 */
typedef struct Packed2naRead Packed2naRead;
struct Packed2naRead
{
    uint64_t read_id;
    void *starting_byte;
    uint32_t offset_to_first_bit;
    uint32_t length_in_bases;
};

VDB_EXTERN uint32_t CC VdbBlast2naReaderData ( VdbBlast2naReader *self,
    uint32_t *status, Packed2naRead *buffer, uint32_t buffer_length );


/*------------------------------------------------------------------------------
 * VdbBlast4naReader
 */
VDB_BLAST_DECLARE ( VdbBlast4naReader );


/* Make4naReader
 *  create a reader onto the run set
 *
 *  "status" [ OUT ] - return parameter for status code
 */
VDB_EXTERN VdbBlast4naReader* CC VdbBlastRunSetMake4naReader
    ( const VdbBlastRunSet *self, uint32_t *status );


/* AddRef
 *  attach a reference to existing object
 * Release
 *  drop reference to object
 *  deletes object when last reference is gone
 */
VDB_EXTERN VdbBlast4naReader* CC VdbBlast4naReaderAddRef
    ( VdbBlast4naReader *self );
VDB_EXTERN void CC VdbBlast4naReaderRelease ( VdbBlast4naReader *self );


/* Read
 *  read a single sequence in unpacked, std 4na
 *
 *  "status" [ OUT ] - return parameter for status code
 *
 *  "read_id" [ IN ] - id of read to be read
 *
 *  "starting_base" [ IN ] - zero-based starting index into the sequence
 *
 *  "buffer" [ OUT ] and "buffer_len" [ IN ] - externally provided buffer
 *  for single sequence in unpacked 4na
 *
 *  returns number of bases read or 0 if no further bases could be read
 */
VDB_EXTERN size_t CC VdbBlast4naReaderRead ( const VdbBlast4naReader *self,
    uint32_t *status, uint64_t read_id, size_t starting_base,
    uint8_t *buffer, size_t buffer_length );


/* Data
 *  access cached data
 *
 *  "status" [ OUT ] - return parameter for status code
 *
 *  "read_id" [ IN ] - id of read to be accessed
 *
 *  "length" [ OUT ] - return parameter for sequence length
 *
 *  returns pointer to data or NULL upon error
 */
VDB_EXTERN const uint8_t* CC VdbBlast4naReaderData
    ( const VdbBlast4naReader *self,
      uint32_t *status, uint64_t read_id, size_t *length );


/*------------------------------------------------------------------------------
 * VdbBlastStdaaReader
 */
VDB_BLAST_DECLARE ( VdbBlastStdaaReader );


/* MakeStdaaReader
 *  create a reader onto the run set
 */
VDB_EXTERN VdbBlastStdaaReader* CC VdbBlastRunSetMakeStdaaReader
    ( const VdbBlastRunSet *self, uint32_t *status );


/* AddRef
 *  attach a reference to existing object
 * Release
 *  drop reference to object
 *  deletes object when last reference is gone
 */
VDB_EXTERN VdbBlastStdaaReader* CC VdbBlastStdaaReaderAddRef
    ( VdbBlastStdaaReader *self );
VDB_EXTERN void CC VdbBlastStdaaReaderRelease ( VdbBlastStdaaReader *self );


/* Read
 *  read a single sequence in stdaa
 *
 *  "status" [ OUT ] - return parameter for status code
 *
 *  "pig" [ IN ] - protein id
 *
 *  "buffer" [ OUT ] and "buffer_len" [ IN ] - externally provided buffer
 *  for single sequence in unpacked Stdaa
 *
 *  returns number of residues read or 0 upon error
 */
VDB_EXTERN size_t CC VdbBlastStdaaReaderRead ( const VdbBlastStdaaReader *self,
    uint32_t *status, uint64_t pig, uint8_t *buffer, size_t buffer_length );


/* Data
 *  access cached data
 *
 *  "status" [ OUT ] - return parameter for status code
 *
 *  "length" [ OUT ] - return parameter for sequence length
 *
 *  returns pointer to data or NULL upon error
 */
VDB_EXTERN const uint8_t* CC VdbBlastStdaaReaderData
    ( const VdbBlastStdaaReader *self,
      uint32_t *status, uint64_t pig, size_t *length );


#ifdef __cplusplus
}
#endif

#endif /* _h_ncbi_vdb_blast_ */
