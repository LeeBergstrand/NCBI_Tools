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

#ifndef __TRACE_OS_UTILS_H__
#define __TRACE_OS_UTILS_H__
#include <tracedb.h>
#include <tracedata.h>
#include <os_mtutils.h>

#include <sra/sradb.h>
#include <vdb/manager.h>
#include <sra/srapath.h>
#include <klib/writer.h>
#include <klib/log.h>

#define	GET_TRACE_DATA_INFO	0x0001
#define	GET_TRACE_DATA_BASECALL	0x0002
#define	GET_TRACE_DATA_QSCORES	0x0004
#define	GET_TRACE_DATA_IMAGE	0x0008
#define	GET_TRACE_DATA_PEAK	0x0010
#define	GET_TRACE_DATA_COMMENT	0x0020
#define	GET_TRACE_DATA_MATE	0x0040
#define	GET_TRACE_DATA_ALT	0x8000

#define IMAGE_SOURCE_DB		1
#define IMAGE_SOURCE_FILE	2

#define MACRO_SRV_XFERDATA(DIR,TYPE) if(srv_xferdata(spp,DIR,TYPE)!=CS_SUCCEED) {status |= SRV_DONE_ERROR; goto DONE;}

/*******/
typedef struct TIintroStruct {
        Uint4		ti;
        CS_INT		replaced_by;
	CS_CHAR		trace_name[251];
	CS_CHAR		center_name[51];
	CS_CHAR		species_code[101];
	CS_CHAR		trace_type_code[51];
	CS_CHAR		accession[31];
	CS_INT		basecall_len;
	CS_INT		trace_len;
	CS_SMALLINT	trace_table_type;
} TIintro, PNTR TIintroPtr;

typedef struct TraceFieldStruct {
        CS_CHAR                 name[51];
        CS_CHAR                 field_prim[51];
        CS_BIT                  is_searchable;
        CS_BIT                  is_static;
        struct TraceFieldStruct PNTR     next;
} TraceField, PNTR TraceFieldPtr;

typedef struct TraceQueryStruct {
        CS_CHAR                 sql[1025];
	CS_INT			page_size;
	CS_INT			page_number;
	CS_INT			rowcount;
} TraceQuery, PNTR TraceQueryPtr;

typedef struct TraceLocationStruct {
	char			srvname[50];	
	char			dbname[50];	
	char			path[256];
	BalancedServerPtr	bsrv;
	unsigned int		ti_start;
	unsigned int		ti_stop;
} TraceLocation, PNTR TraceLocationPtr;


typedef struct TraceLocationSetStruct {
	int			numsrv_img;
	TraceLocationPtr	trloc_img;
	CharPtr			trloc_img_tab;
	int			numsrv_data;
	TraceLocationPtr	trloc_data;
	CharPtr			trloc_data_tab;
	int			numsrv_alt;
	TraceLocationPtr        trloc_alt;
	CharPtr			trloc_alt_tab;
	ValNodePtr		vn_bsrv_list;
} TraceLocationSet, PNTR TraceLocationSetPtr;

typedef struct TraceOSStruct {
    OpenServerInfo  osi;
    SRV_PROC PNTR   service_thread;
    SRV_PROC PNTR   service_thread_data;
    SRV_PROC PNTR   service_thread_img;
    Boolean         service_is_sleeping;
    BalancedServer  iqconn;
	BalancedServer	trmainconn;
    ServerInfoPtr   iqservers;
    ServerInfoPtr   servers;
    Int2            numconn;/* specifies how many pre-initialized connections are in use */
                            /* if 0 gets a connection for every login */
    Int4            numusers;
    Int4            packetsize;
    Boolean         restart_is_running;
    KLogLevel       debug_level;
	TraceLocationSet trlocs;
	TraceFieldPtr	trace_fields;
	TraceFieldPtr	trace_fields_old;
	FILE*           fplog;
	SRV_OBJID       log_mutex;  /* to guard log file */
	TraceDB		    tracedb_data;
	TraceDB		    tracedb_img;
	char*           sracfgfile;
	const SRAMgr*   sradb;
    char*           ftp_prefix;
    int             ftp_prefix_len;
    char*           aspera_prefix;
    int             aspera_prefix_len;
} TraceOS, PNTR TraceOSPtr;

typedef struct TraceDataRequestStruct {
	Int4	ti;
	CharPtr dbname;
	Int4	basecall_len;
	Char	basecall[2048];
	Char	qualscore[2048];
	Char	peakidx[2048];
} TraceDataRequest,PNTR TraceDataRequestPtr;

CS_RETCODE TraceOS_start_handler(SRV_SERVER *spp);
CS_RETCODE TraceOS_service_handler(SRV_SERVER *spp);
CS_RETCODE TraceOS_filedb_data_maint_handler(SRV_SERVER *spp);
CS_RETCODE TraceOS_filedb_img_maint_handler(SRV_SERVER *spp);
CS_RETCODE TraceOS_sradb_maint_handler(SRV_SERVER *spp);
CS_RETCODE TraceOS_lang_handler(SRV_SERVER *spp);
CS_VOID TraceOS_signal_handler(int signo);

Boolean TraceLocationsInit(TraceLocationPtr PNTR trlocp,Int4Ptr numsrvp, CharPtr table_name,CS_COMMAND PNTR ctcmd,ValNodePtr PNTR vn_bsrv_list);
Boolean TraceOS_new_request_log(CharPtr newfile);
Boolean TraceOS_log_request(unsigned int ti_start,unsigned int ti_stop,int outfmt);


/*********** Stored procedures handlers *****************/
CS_RETCODE TraceOS_show_image_locations(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_show_data_locations(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_show_altdata_locations(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_show_fields(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_usp_get_trace_data(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_sra_get_run_data(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_sra_get_signal(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_sra_get_run_files(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_sra_get_sequence(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_sra_find_spot(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_sra_find_seq(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_sra_find_group(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_get_trace_data_by_ti(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_trace_query(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_show_pools(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_id_get_asnprop(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_tracemain_gateway(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_usp_range_by_submission_id(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_usp_reset_pending_status(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_usp_show_reset_pending_status(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_usp_sid_show_log(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_usp_tracking(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_usp_list_species_pager(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_sra_flush(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_sra_list_references(SRV_PROC PNTR spp);
CS_RETCODE TraceOS_shutdown(SRV_PROC PNTR spp);



/*********** Helper functions ***************************/
CS_RETCODE TraceOS_sra_get_asnprop(SRV_PROC PNTR spp,int sat_key);
CS_RETCODE TraceOS_show_locations_func(SRV_PROC PNTR spp,TraceLocationPtr trloc,int cnt);
CS_RETCODE TraceOS_ti_intro_output_func(SRV_PROC PNTR spp,TIintroPtr tiip);
CS_RETCODE TraceOS_get_trace_image(SRV_PROC PNTR spp, UserThreadDataPtr tdatap,TIintroPtr tiip,Int4Ptr im_source);
CS_INT TraceOS_tracedb_find_data(SRV_PROC PNTR spp,Uint4 ti,Int4 outform,TraceData ** tdp);




/************** Callbacks to os_ExecOnBestConnection **************/
CS_RETCODE TraceLocationSetInit(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg);
CS_RETCODE TraceFieldsInit(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg);
CS_RETCODE TraceOS_get_alt_data_cb(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg);
CS_RETCODE TraceOS_get_table_type_cb(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg);
CS_RETCODE TraceOS_get_trace_intro_cb(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg);
CS_RETCODE TraceOS_trace_query_cb(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg);

#endif
