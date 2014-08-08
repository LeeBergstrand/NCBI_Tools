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

#ifndef __TROS_SRAUTILS_H__
#define __TROS_SRAUTILS_H__
#include <sra/sradb.h>
#include <vdb/table.h>
#include <vdb/database.h>
#include <sra/sradb-priv.h>
#include <search/nucstrstr.h>
#include <sra/types.h>
#include <os_mtutils.h>
#include <klib/rc.h>
#include <kfs/directory.h>


#define SRA_OUTPUT_META_BIT	1
#define	SRA_OUTPUT_NAME_BIT	2
#define SRA_OUTPUT_BASECALL_BIT 4
#define SRA_OUTPUT_QUALITY_BIT	8
#define SRA_OUTPUT_POSITION_BIT	16
#define SRA_OUTPUT_SIGNAL_BIT	32
#define SRA_OUTPUT_COORD_BIT	64
#define SRA_OUTPUT_CLIP_QUALITY_RIGHT_BIT	128
#define SRA_OUTPUT_INTENSITY_BIT   	256
#define SRA_OUTPUT_NOISE_BIT   		512
#define SRA_OUTPUT_QUALITY4_BIT	1024
#define SRA_OUTPUT_COLOR_SPACE_BIT 2048
#define SRA_OUTPUT_FILTER_BIT 4096
#define SRA_OUTPUT_SPOTGROUP_BIT 8192
#define SRA_OUTPUT_READ_TYPE_BIT 16384
#define SRA_OUTPUT_READ_LEN_BIT  32768
#define SRA_OUTPUT_READ_LABEL_BIT  65536 

typedef struct SRASpotCoord{
    uint32_t x, y, tile;
        uint32_t  lane;
        spotid_t id;

    /* prefix part of spotname */
        int32_t platename_len;

}SRASpotCoord;



typedef struct  SraColumnData_struct{
	const SRAColumn	*sc;
	bool		not_present;	
	CS_BYTE*	data;
	bitsz_t		bitoffset;
	bitsz_t		bitlen;
	CS_INT          len;
	CS_SMALLINT     ind;
	/*** substructured reads ****/
	uint8_t		outcol[16];
} SraColumnData;

typedef struct SraUserData_struct{
	char		acc[256];
	const SRATable *run;
	const VTable   *vtbl;
	const VDatabase   *vdb;
	/*** const SRATableData    *tdata;***/
	spotid_t	run_spot_first, run_spot_last;
/****** Coordinates replacement for SRATable ****/
	SRASpotCoord	coord;
/****** META DATA replacement for SRATable ***/
	uint8_t		platform;
	char		platform_str[100];
	uint32_t	num_reads;
	uint32_t	read_mask_bio;
	uint32_t	fixed_spot_len;
	uint64_t	base_count;
	int		plate_outcol;
/****** EXTRA META DATA ******/
	char		key_seq[20];
	int		key_seq_len;
	char		flow_seq[4*1024];
	int		flow_seq_len;
	char            linker_seq[100];
        int             linker_seq_len;
	char		cs_key[32];
	
/********** may be obsolete ??? ******/
	int		num_sig_ch;
	int		num_int_ch;
	int		num_nse_ch;
	int		qs4_present;
	int		cqr_present;
	int		cql_present;
	int		spot_group_present;
	int		rd_filter_present;
	char		*sig_typedecl;
/********** COLUMNS *****************/
	SraColumnData	bc;	/*** basecalls: READ: insdc_fasta_t**/
	SraColumnData	cs;	/*** abi color space: CSREAD: insdc_csfasta_t**/
	SraColumnData	rd;	/*** basecalls: READ: ncbi_2na_t**/
	SraColumnData	rd_cs;	/*** abi color space: READ: ncbi_2cs_t**/
	SraColumnData	len;	/**** read structure: READ_LEN ***/
	SraColumnData	name;	/*** spot name:   NAME**/
        SraColumnData	rdesc;	/*** READ_DESC (read label) ***/
	SraColumnData	qs1;	/*** quality scores: QUALITY:ncbi_qual1_t **/
	SraColumnData	qs4;	/*** quality scores: QUALITY:ncbi_qual4_t **/
	SraColumnData	pos;	/*** peak posititon: POSITION  ***/
	SraColumnData	sig;	/*** processed signal graphs: SIGNAL ***/
	SraColumnData	rint;   /*** raw intensity graphs: INTENSITY ***/
	SraColumnData	nse;	/*** noise graphs: NOISE ***/
	SraColumnData	cqr;	/*** clip quality right: CLIP_QUALITY_RIGHT ***/
	SraColumnData	cql;	/*** clip quality right: CLIP_QUALITY_LEFT ***/
        SraColumnData	filt;   /*** bad read filter: READ_FILTER:sra_read_filter_t ***/
        SraColumnData	sgrp;   /*** spot group SPOT_GROUP:ascii ***/
	SraColumnData   trd;	/*** read types ***/
	SraColumnData   lbl;	/*** label string ***/
	SraColumnData   lbls;	/*** label start ***/
	SraColumnData   lbll;	/*** label len ***/
/****** pacbio qualities ********/
	SraColumnData   delq;   /*** deletion qualities **/
	SraColumnData   delv;   /*** deletion tag  **/
	SraColumnData   subq;   /*** substitution qualities **/
	SraColumnData   subv;   /*** substitution tag  **/
	SraColumnData   insq;   /*** insertion qualities **/
/****** pacbio signal ************/
	SraColumnData   frw;	/*** width in frames ***/
	SraColumnData   frp;	/*** pre-base in frames ***/
/******** Current position ****/
	spotid_t	spotid;
	uint16_t	clip_quality_right;
	uint16_t	clip_quality_left;
} SraUserData;

typedef struct SraFiles_struct {
    CS_INT id;
    CS_INT parent;
    KPathType type;
    char name[256 * 5];
    CS_INT mtime;
    CS_INT size;
} SraFiles;


void SraUserDataDelete(SraUserData *data);
void SraUserDataReset(SraUserData *data);
rc_t SraUserDataSetRunAccession(const SRAMgr *sradb,SraUserData *data,char *acc,char **errstr);
rc_t SraUserDataOpen(SraUserData *data,spotid_t id,int format_mask,int read_mask);
rc_t SraColumnDataOpen(SraColumnData *c,const SRATable *t,char* name,char* type,spotid_t id);
rc_t SraGetFileList(const char* server, const char* rpath, int parent, SraFiles* files, int* max_qty, bool signal, bool names);

/******************* OUTPUT FUNCTIONS *******/
CS_RETCODE SraUserSendMetadata(SRV_PROC *spp);
CS_RETCODE SraUserSpotDataPrepare(SRV_PROC *spp,SraUserData *sd,int format_mask,int read_mask);
CS_RETCODE SraUserSpotDataSend(SRV_PROC *spp,SraUserData *sd,int format_mask,int read_mask,int do_clip);
CS_RETCODE SraUserSignalSend(SRV_PROC *spp,SraUserData *sd);
CS_RETCODE SraUserSequenceSend(SRV_PROC *spp,SraUserData *sd);
CS_RETCODE Sra_send_asnprop(SRV_PROC *spp);

#endif
