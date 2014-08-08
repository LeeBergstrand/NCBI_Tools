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

#include <tracedb.h>
#include <tracedata.h>
#include <TraceOSutils.h>
#include <time.h>
#include <sys/signal.h>
#include <tros_srautils.h>
#include <vdb/cursor.h>
#include <align/iterator.h>
#include <align/reference.h>
#include <sra/impl.h>

#define ERR_CODE_WITHDRAWN 22001
#define ERR_CODE_NOTFOUND  22002
#define ERR_CODE_NOTLOADED 22003
#define ERR_CODE_UNKNOWN   22004
#define ERR_CODE_REPLACED  22005

#define DEFAULT_PASSWORD  "ShutItDown"

#define SERVICEFREQ     30      /* time period for service thread*/
#define IQCOMMIT_MULT   10      /* SERVICEFREQ*IQCOMMIT_MULT = time between commits */


extern CS_INT   DebugLevel;
extern TraceOS	tros;

static unsigned char dummy_buf[128000]; /*** keep zeroed out data for some weird cases ***/

/*****************************************/
/************* HANDLERS       ************/
/*****************************************/
CS_RETCODE
TraceOS_start_handler(SRV_SERVER *spp)
{
    SRV_PROC   *regthreadp;                 /* Thread to use for registering procedure. */
    CS_INT     int_default;
	CS_SMALLINT	smallint_default;
	CS_TINYINT	tinyint_default;

	if((regthreadp = srv_createproc(spp))){
		if (srv_regdefine(regthreadp,"shutdown",CS_NULLTERM,TraceOS_shutdown) == CS_FAIL) return CS_FAIL;
		if (srv_regparam(regthreadp,"@passwd",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL) return CS_FAIL;
		if (srv_regcreate(regthreadp,NULL) == CS_FAIL) return CS_FAIL;

                /*** show_image_locations ***/
                if (srv_regdefine(regthreadp,"show_image_locations",CS_NULLTERM,TraceOS_show_image_locations) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;
                /*** show_data_locations ***/
                if (srv_regdefine(regthreadp,"show_data_locations",CS_NULLTERM,TraceOS_show_data_locations) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;
                /*** show_altdata_locations ***/
                if (srv_regdefine(regthreadp,"show_altdata_locations",CS_NULLTERM,TraceOS_show_altdata_locations) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;
		/*** show fields ****/
		if (srv_regdefine(regthreadp,"show_fields",CS_NULLTERM,TraceOS_show_fields) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;

                /*** show_pools ***/
                if (srv_regdefine(regthreadp,"show_pools",CS_NULLTERM,TraceOS_show_pools) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;
		/*** retrieve trace data ***/
		if(tros.trmainconn.is_online){
			if (srv_regdefine(regthreadp,"usp_get_trace_data",CS_NULLTERM,TraceOS_usp_get_trace_data) == CS_FAIL)
				return CS_FAIL;
			int_default=0;
			if (srv_regparam(regthreadp,"@first",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			if (srv_regparam(regthreadp,"@last",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			smallint_default=1;
			if (srv_regparam(regthreadp,"@form",CS_NULLTERM,CS_INT_TYPE,sizeof(smallint_default),(CS_BYTE*)&smallint_default) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;
			/*** another way to retrieve trace data ***/
			if (srv_regdefine(regthreadp,"get_trace_data_by_ti",CS_NULLTERM,TraceOS_get_trace_data_by_ti) == CS_FAIL)
				return CS_FAIL;
			int_default=0;
			if (srv_regparam(regthreadp,"@ti",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			tinyint_default=21;
			if (srv_regparam(regthreadp,"@mask",CS_NULLTERM,CS_TINYINT_TYPE,sizeof(tinyint_default),(CS_BYTE*)&tinyint_default) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;

		}
		/**** trace_query *****/
		if(tros.iqconn.is_online){
			if (srv_regdefine(regthreadp,"trace_query",CS_NULLTERM,TraceOS_trace_query) == CS_FAIL)
				return CS_FAIL;
			if (srv_regparam(regthreadp,"@sql",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
				return CS_FAIL;
			int_default=10;
			if (srv_regparam(regthreadp,"@page_size",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			int_default=0;
			if (srv_regparam(regthreadp,"@page_number",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;
		}

		/*** proc needed by PubSeqOS ***/
                if (srv_regdefine(regthreadp,"id_get_asnprop",CS_NULLTERM,TraceOS_id_get_asnprop) == CS_FAIL)
                        return CS_FAIL;
                int_default=0;
                if (srv_regparam(regthreadp,"@sat_key",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@feat_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		smallint_default=0;
                if (srv_regparam(regthreadp,"@sat",CS_NULLTERM,CS_SMALLINT_TYPE,sizeof(smallint_default),(CS_BYTE*)&smallint_default) == CS_FAIL)
                        return CS_FAIL;
		if (srv_regparam(regthreadp,"@username",CS_NULLTERM,CS_CHAR_TYPE,255,(CS_BYTE*)"") == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;
/******** passthrough procs *****************
usp_list_arrivals
usp_list_centers
usp_list_strategy
usp_list_species
usp_list_retrievals
usp_get_searchable_fields
usp_list_submission_type
usp_list_common_fields
usp_tracking
usp_list_species_pager
usp_reset_pending_status
usp_sid_show_log 
usp_show_reset_pending_status 
*********************************************/
		if(tros.trmainconn.is_online){
			if (srv_regdefine(regthreadp,"usp_range_by_submission_id",CS_NULLTERM,TraceOS_usp_range_by_submission_id) == CS_FAIL)
				return CS_FAIL;
			int_default=0;
			if (srv_regparam(regthreadp,"@sid",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;

			if (srv_regdefine(regthreadp,"usp_reset_pending_status",CS_NULLTERM,TraceOS_usp_reset_pending_status) == CS_FAIL)
				return CS_FAIL;
			int_default=0;
			if (srv_regparam(regthreadp,"@sid",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			if (srv_regparam(regthreadp,"@user_name",CS_NULLTERM,CS_CHAR_TYPE,64,(CS_BYTE*)"") == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;

			if (srv_regdefine(regthreadp,"usp_show_reset_pending_status",CS_NULLTERM,TraceOS_usp_show_reset_pending_status) == CS_FAIL)
				return CS_FAIL;
			int_default=0;
			if (srv_regparam(regthreadp,"@sid",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			if (srv_regparam(regthreadp,"@user_name",CS_NULLTERM,CS_CHAR_TYPE,64,(CS_BYTE*)"") == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;

			if (srv_regdefine(regthreadp,"usp_sid_show_log",CS_NULLTERM,TraceOS_usp_sid_show_log) == CS_FAIL)
				return CS_FAIL;
			int_default=0;
			if (srv_regparam(regthreadp,"@sid",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;


			if (srv_regdefine(regthreadp,"usp_tracking",CS_NULLTERM,TraceOS_usp_tracking) == CS_FAIL)
				return CS_FAIL;
			int_default=10;
			if (srv_regparam(regthreadp,"@page_size",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			int_default=1;
			if (srv_regparam(regthreadp,"@page_number",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			if (srv_regparam(regthreadp,"@filter",CS_NULLTERM,CS_CHAR_TYPE,255,(CS_BYTE*)"") == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;

			if (srv_regdefine(regthreadp,"usp_list_species_pager",CS_NULLTERM,TraceOS_usp_list_species_pager) == CS_FAIL)
				return CS_FAIL;
			if (srv_regparam(regthreadp,"@letter",CS_NULLTERM,CS_CHAR_TYPE,1,(CS_BYTE*)"A") == CS_FAIL)
				return CS_FAIL;
			int_default=0;
			if (srv_regparam(regthreadp,"@page_number",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			int_default=40;
			if (srv_regparam(regthreadp,"@page_size",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;


			if (srv_regdefine(regthreadp,"usp_list_arrivals",CS_NULLTERM,TraceOS_tracemain_gateway) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;
			if (srv_regdefine(regthreadp,"usp_list_centers",CS_NULLTERM,TraceOS_tracemain_gateway) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;
			if (srv_regdefine(regthreadp,"usp_list_strategy",CS_NULLTERM,TraceOS_tracemain_gateway) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;
			if (srv_regdefine(regthreadp,"usp_list_species",CS_NULLTERM,TraceOS_tracemain_gateway) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;

			if (srv_regdefine(regthreadp,"usp_list_retrievals",CS_NULLTERM,TraceOS_tracemain_gateway) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;
			if (srv_regdefine(regthreadp,"usp_list_submission_type",CS_NULLTERM,TraceOS_tracemain_gateway) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;
			if (srv_regdefine(regthreadp,"usp_list_common_fields",CS_NULLTERM,TraceOS_tracemain_gateway) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;
			if (srv_regdefine(regthreadp,"usp_get_searchable_fields",CS_NULLTERM,TraceOS_tracemain_gateway) == CS_FAIL)
				return CS_FAIL;
			if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
				return CS_FAIL;
		}
/************** SRA PROCS *************************/
	        if (srv_regdefine(regthreadp,"sra_get_run_data",CS_NULLTERM,TraceOS_sra_get_run_data) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
		        int_default=1;
                if (srv_regparam(regthreadp,"@spot_first",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0x7fffffff;
                if (srv_regparam(regthreadp,"@spot_last",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0x7fffffff;
                if (srv_regparam(regthreadp,"@spot_cnt",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0x7fffffff;
                if (srv_regparam(regthreadp,"@format_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0x7fffffff;
                if (srv_regparam(regthreadp,"@read_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0;
                if (srv_regparam(regthreadp,"@do_clip",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;

                if (srv_regdefine(regthreadp,"sra_get_signal",CS_NULLTERM,TraceOS_sra_get_signal) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@spotid",CS_NULLTERM,CS_INT_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;

                if (srv_regdefine(regthreadp,"sra_get_sequence",CS_NULLTERM,TraceOS_sra_get_sequence) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@spotid",CS_NULLTERM,CS_INT_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;

		    if (srv_regdefine(regthreadp,"sra_find_spot",CS_NULLTERM,TraceOS_sra_find_spot) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@spotname",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0;
                if (srv_regparam(regthreadp,"@range_x",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0;
                if (srv_regparam(regthreadp,"@range_y",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=SRA_OUTPUT_BASECALL_BIT|SRA_OUTPUT_NAME_BIT|SRA_OUTPUT_COORD_BIT;
                if (srv_regparam(regthreadp,"@format_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0x7fffffff;
                if (srv_regparam(regthreadp,"@read_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0;
                if (srv_regparam(regthreadp,"@do_clip",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0x7fffffff;
                if (srv_regparam(regthreadp,"@pagesize",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=1;
                if (srv_regparam(regthreadp,"@pageno",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;

		if (srv_regdefine(regthreadp,"sra_find_seq",CS_NULLTERM,TraceOS_sra_find_seq) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@seq",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
                int_default=SRA_OUTPUT_BASECALL_BIT|SRA_OUTPUT_NAME_BIT|SRA_OUTPUT_COORD_BIT;
                if (srv_regparam(regthreadp,"@format_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                int_default=0x7fffffff;
                if (srv_regparam(regthreadp,"@read_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                int_default=0;
                if (srv_regparam(regthreadp,"@do_clip",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0x7fffffff;
                if (srv_regparam(regthreadp,"@pagesize",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=1;
                if (srv_regparam(regthreadp,"@pageno",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0x7fffffff;
                if (srv_regparam(regthreadp,"@read_mask_search",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;

		if (srv_regdefine(regthreadp,"sra_find_group",CS_NULLTERM,TraceOS_sra_find_group) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@group",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
                int_default=SRA_OUTPUT_BASECALL_BIT|SRA_OUTPUT_NAME_BIT|SRA_OUTPUT_COORD_BIT;
                if (srv_regparam(regthreadp,"@format_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                int_default=0x7fffffff;
                if (srv_regparam(regthreadp,"@read_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                int_default=0;
                if (srv_regparam(regthreadp,"@do_clip",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                        int_default=0x7fffffff;
                if (srv_regparam(regthreadp,"@pagesize",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                        int_default=1;
                if (srv_regparam(regthreadp,"@pageno",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;


	        if (srv_regdefine(regthreadp,"sra_flush",CS_NULLTERM,TraceOS_sra_flush) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;

	        if (srv_regdefine(regthreadp,"sra_get_run_files",CS_NULLTERM,TraceOS_sra_get_run_files) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
        		int_default=0;
                if (srv_regparam(regthreadp,"@signal",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		        int_default=0;
                if (srv_regparam(regthreadp,"@names",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
        		int_default=0;
                /* 0 - all,  1 - remote, 2 - local */
                if (srv_regparam(regthreadp,"@paths",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;


		if (srv_regdefine(regthreadp,"sra_list_references",CS_NULLTERM,TraceOS_sra_list_references) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,SRV_NODEFAULT,NULL) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regparam(regthreadp,"@name",CS_NULLTERM,CS_CHAR_TYPE,0,(CS_BYTE*)"") == CS_FAIL)
                        return CS_FAIL;
		int_default=0;
                if (srv_regparam(regthreadp,"@max_seq_len",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
		int_default=0;
		if (srv_regparam(regthreadp,"@num_bins",CS_NULLTERM,CS_INT_TYPE,sizeof(int_default),(CS_BYTE*)&int_default) == CS_FAIL)
                        return CS_FAIL;
                if (srv_regcreate(regthreadp,NULL) == CS_FAIL)
                        return CS_FAIL;


/**************************************************/


		/**** some common procs ****/		
		if(os_register_common_procs(regthreadp) == CS_FAIL)
			return CS_FAIL;

/* Terminate the thread created for procedure registration. */
                srv_termproc(regthreadp);
        } else {
                return CS_FAIL;
        }


        /*Install event handlers.*/
        if(!srv_handle(spp,SRV_ATTENTION,os_attention_handler))
                return CS_FAIL;
        if(!srv_handle(spp,SRV_CONNECT,os_conn_handler))
                return CS_FAIL;
        if(!srv_handle(spp,SRV_DISCONNECT,os_disconn_handler))
                return CS_FAIL;
        if(!srv_handle(spp,SRV_LANGUAGE,TraceOS_lang_handler))
                return CS_FAIL;

	
        if(!srv_spawn(&tros.service_thread,SRV_DEFAULT_STACKSIZE,
			(SRV_SERVICETHREAD_FUNC)TraceOS_service_handler,&tros,SRV_LOWPRIORITY)){
                return CS_FAIL;
        }
        if(!srv_spawn(&tros.service_thread_data,SRV_DEFAULT_STACKSIZE,
			(SRV_SERVICETHREAD_FUNC)TraceOS_filedb_data_maint_handler,&tros,SRV_LOWPRIORITY)){
                return CS_FAIL;
        }
        if(!srv_spawn(&tros.service_thread_img,SRV_DEFAULT_STACKSIZE,
			(SRV_SERVICETHREAD_FUNC)TraceOS_filedb_img_maint_handler,&tros,SRV_LOWPRIORITY)){
                return CS_FAIL;
        }
#if 1
        if(!srv_spawn(&tros.service_thread_img,SRV_DEFAULT_STACKSIZE,
			(SRV_SERVICETHREAD_FUNC)TraceOS_sradb_maint_handler,&tros,SRV_LOWPRIORITY)){
                return CS_FAIL;
        }
#endif
        return CS_TRUE;
}
CS_VOID
TraceOS_signal_handler(int signo)
{
        if(tros.service_is_sleeping){
                srv_wakeup(tros.service_thread,SRV_M_WAKE_INTR|SRV_M_WAKE_FIRST,0,0);
        }
        return;
}
static CS_BOOL
TraceOS_check_service_threads(ServerInfoPtr head)
{
        CS_INT          intinfo;
        ServerInfoPtr   srvip;
        CS_CHAR         logmsg[200];

        for(srvip=head;srvip;srvip=srvip->next){
                if(srvip->conn){
                        if(ct_con_props(srvip->conn,CS_GET,CS_CON_STATUS,&intinfo,CS_UNUSED,NULL) == CS_FAIL)
                                return CS_FALSE;
                        if( intinfo & CS_CONSTAT_DEAD){
                                sprintf(logmsg,"Service|%-15s|Connection is marked dead",
                                        srvip->clu.srv);
                                os_ErrPost(NULL,NULL,logmsg,0,0,CS_FALSE,0);
                                ct_close(srvip->conn,CS_FORCE_CLOSE);
                                ct_con_drop(srvip->conn);
                                srvip->conn=NULL;
                                srvip->p_cpu = srvip->p_io = srvip->p_idle = 0;
                                srvip->d_idle = 0;
                        }
                }
                if(!srvip->conn){
                        if(!CTLibConnect(&srvip->clu,NULL)){
                                sprintf(logmsg,"Service|%-15s|Unable to connect will try later",
                                        srvip->clu.srv);
                                os_ErrPost(NULL,NULL,logmsg,0,0,CS_FALSE,0);
                                continue;
                        }
                        srvip->ctcmd = srvip->clu.ctcmd;
                        srvip->conn = srvip->clu.connection;
                        srvip->do_log = FALSE;
                        sprintf(logmsg,"Service|%-15s|Reconnected successfully",srvip->clu.srv);
                        os_ErrPost(NULL,NULL,logmsg,0,0,CS_FALSE,0);
                }
		/* get statistics */
                CTlibSingleValueSelect(srvip->ctcmd,
                        "select convert(int,convert(float,property('ProcessCPU'))*100)",
                        &intinfo,sizeof(intinfo));
                srvip->p_cpu=(intinfo - srvip->cpu_busy) / 100.0; /**** delta in seconds ***/
                srvip->cpu_busy=intinfo;

                /* issue_commits */
		CTLibSimpleSQL_Ex(srvip->ctcmd, "commit");
                sprintf(logmsg,"Service|%-15s|Commit is done",srvip->clu.srv);
		os_ErrPost(NULL,NULL,logmsg,0,0,CS_FALSE,100);
        }
        return CS_TRUE;
}
/* bubble sort algorithm */
#ifdef BALANCED_CONN_MGR
#define MACRO_ServerOrder_CMP(I,J) (bsp->m_Dbs[bsp->load_order[I]]->m_Stat.m_p_cpu < bsp->m_Dbs[bsp->load_order[J]]->m_Stat.m_p_cpu)
#else
#define MACRO_ServerOrder_CMP(I,J) (bsp->server[bsp->load_order[I]]->p_cpu < bsp->server[bsp->load_order[J]]->p_cpu)
#endif
#define MACRO_ServerOrder_SWAP(I,J) {t = bsp->load_order[I];bsp->load_order[I] = bsp->load_order[J];bsp->load_order[J] = t;}

static void
IQ_BalancedServerOrder(BalancedServerPtr bsp)
{
        Int2    t,i,j;

        for(i=0;i<bsp->numservers-1;i++){
                for(j=bsp->numservers-1;j>i;j--){
                        if(MACRO_ServerOrder_CMP(j,j-1)) MACRO_ServerOrder_SWAP(j,j-1);
                }
        }
}

CS_RETCODE
TraceOS_service_handler(CS_VOID PNTR arg)
{
    CS_INT  i,spid,need_commit=0;
    CS_CHAR ustate[]="SERVICE THREAD";
    SRV_PROC PNTR   spp;
    ValNodePtr	vnp;
    BalancedServerPtr	bsp;
    UserThreadData  tdata;

	srv_props(tros.osi.context,CS_GET,SRV_S_CURTHREAD,&spp,sizeof(spp),NULL);
        srv_thread_props(spp,CS_GET,SRV_T_SPID,(CS_VOID PNTR)&spid,sizeof(spid),NULL);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)ustate,sizeof(ustate),NULL);
	memset(&tdata,0,sizeof(tdata));
        srv_thread_props(spp,CS_SET,SRV_T_USERDATA,(CS_VOID PNTR)&tdata,CS_SIZEOF(&tdata),NULL);
        srv_thread_props(spp,CS_GET,SRV_T_SPID,(CS_VOID PNTR)&tdata.spid,sizeof(tdata.spid),NULL);

        while(1){
				/**** Some threads are waiting for broken connection to be restored ****/
		if(!os_get_server_statistics(tros.servers)){
                        goto FATAL;
                }
                if(tros.iqconn.is_online){
			TraceOS_check_service_threads(tros.iqservers);
			BalancedServerConnectEx(&tros.iqconn);/**** Will re-establish broken connections ****/
			srv_wakeup((CS_VOID PNTR)&tros.iqconn,SRV_M_WAKE_FIRST,0,0);
			IQ_BalancedServerOrder(&tros.iqconn);
		}
		if(tros.trmainconn.is_online){
			BalancedServerConnectEx(&tros.trmainconn);
			srv_wakeup((CS_VOID PNTR)&tros.trmainconn,SRV_M_WAKE_FIRST,0,0);
			BalancedServerOrder(&tros.trmainconn);

			for(vnp=tros.trlocs.vn_bsrv_list;vnp;vnp=vnp->next){
				bsp=(BalancedServerPtr)vnp->data.ptrvalue;
				BalancedServerConnectEx(bsp);
				srv_wakeup((CS_VOID PNTR)bsp,SRV_M_WAKE_FIRST,0,0);
				BalancedServerOrder(bsp);

			}
		}
		
		/*** issue commits on IQ connection ***/
		if(need_commit==IQCOMMIT_MULT){
			if(tros.iqconn.is_online){
#ifdef BALANCED_CONN_MGR
                for( i = 0; i < tros.iqconn.numservers; i++)
                {
                    SBalancedSrvConnection* Conn = 
                        BalancedServerConnect( &tros.iqconn, spid, i);
                    if( Conn == NULL) continue;
                    CTLibSimpleSQL_Ex( Conn->m_CTCmd, "commit");
                    os_ErrPost( tros.osi.context, NULL, "Commit is done", 0, 0, CS_FALSE, 100);
                }
#else
				for(i=0;i<tros.iqconn.numconn*tros.iqconn.numservers;i++){
					if(BalancedServerLockConnection(&tros.iqconn,i,spid)==CS_SUCCEED){
						CTLibSimpleSQL_Ex(tros.iqconn.ctcmd[i], "commit");
						os_ErrPost(tros.osi.context,NULL,"Commit is done",0,0,CS_FALSE,100);
						BalancedServerUnlockConnection(&tros.iqconn,i,spid);
					}
				}
#endif
			}
			need_commit=0;
			if(tros.trmainconn.is_online){
				/******************************************************/
				/**** Refresh Location Table **************************/
				/******************************************************/
				os_ExecOnBestConnection(spp,&tdata,&tros.trmainconn,&tros.trlocs,TraceLocationSetInit,CS_FALSE);
				/******************************************************/
				/**** Refresh Table of Fields *************************/
				/******************************************************/
				os_ExecOnBestConnection(spp,&tdata,&tros.trmainconn,&tros.trlocs,TraceFieldsInit,CS_FALSE);
			}
		}else{
			need_commit++;
		}
                tros.service_is_sleeping=TRUE;
		sleep(SERVICEFREQ);
                tros.service_is_sleeping=FALSE;
        }
FATAL:
        if(srv_event(tros.service_thread,SRV_STOP,NULL) == CS_FAIL) {
                exit(1);
        }
        return CS_FAIL;
}
CS_RETCODE
TraceOS_filedb_img_maint_handler(CS_VOID PNTR arg)
{
    CS_INT  spid;
    CS_CHAR ustate[]="IMAGE MAINT";
	SRV_PROC PNTR   spp;
	UserThreadData  tdata;


	srv_props(tros.osi.context,CS_GET,SRV_S_CURTHREAD,&spp,sizeof(spp),NULL);
        srv_thread_props(spp,CS_GET,SRV_T_SPID,(CS_VOID PNTR)&spid,sizeof(spid),NULL);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)ustate,sizeof(ustate),NULL);
	memset(&tdata,0,sizeof(tdata));
        srv_thread_props(spp,CS_SET,SRV_T_USERDATA,(CS_VOID PNTR)&tdata,CS_SIZEOF(&tdata),NULL);
        srv_thread_props(spp,CS_GET,SRV_T_SPID,(CS_VOID PNTR)&tdata.spid,sizeof(tdata.spid),NULL);

	while(1){
		if(TraceDBInit(&tros.tracedb_img,300,1000000000)==0){
			break;
		}
		os_ErrPost(tros.osi.context,spp,"Failed to start Trace Data Database ... sleeping",0,0,CS_FALSE,0);
		sleep(60);
	}
	/*tros.tracedb_img.verbose=10;	*/
        while(1){
		os_ErrPost(tros.osi.context,spp,"Running image filedb maintenance",0,0,CS_FALSE,100);
		TraceDBRunGCTasks ( &tros.tracedb_img, 2000 );
		sleep(2);
        }
        if(srv_event(tros.service_thread,SRV_STOP,NULL) == CS_FAIL) {
                exit(1);
        }
        return CS_FAIL;
}
CS_RETCODE
TraceOS_filedb_data_maint_handler(CS_VOID PNTR arg)
{
    CS_INT  spid;
    CS_CHAR ustate[]="DATA MAINT";
	SRV_PROC PNTR   spp;
	UserThreadData  tdata;


	srv_props(tros.osi.context,CS_GET,SRV_S_CURTHREAD,&spp,sizeof(spp),NULL);
        srv_thread_props(spp,CS_GET,SRV_T_SPID,(CS_VOID PNTR)&spid,sizeof(spid),NULL);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)ustate,sizeof(ustate),NULL);
	memset(&tdata,0,sizeof(tdata));
        srv_thread_props(spp,CS_SET,SRV_T_USERDATA,(CS_VOID PNTR)&tdata,CS_SIZEOF(&tdata),NULL);
        srv_thread_props(spp,CS_GET,SRV_T_SPID,(CS_VOID PNTR)&tdata.spid,sizeof(tdata.spid),NULL);

	while(1){
		if(TraceDBInit(&tros.tracedb_data,300,1000000000)==0){
			TraceDBInitDecompression(&tros.tracedb_data);
			break;
		}
		os_ErrPost(tros.osi.context,spp,"Failed to start Trace Image  Database ... sleeping",0,0,CS_FALSE,0);
		sleep(60);
	}
	/*tros.tracedb_data.verbose=10;	*/
        while(1){
		os_ErrPost(tros.osi.context,spp,"Running data filedb maintenance",0,0,CS_FALSE,100);
		TraceDBRunGCTasks ( &tros.tracedb_data, 2000 );
		sleep(2);
        }
        if(srv_event(tros.service_thread,SRV_STOP,NULL) == CS_FAIL) {
                exit(1);
        }
        return CS_FAIL;
}
/*** typedef rc_t (* KWrtWriter ) (void * self, const char * buffer, size_t bufsize, size_t * num_writ); ***/
static
rc_t ErrPostForSRA(void *self,const char *buffer, size_t bytes, size_t * num_writ )
{
	if(buffer && bytes > 0){
		char *buf=(char*)buffer;
		srv_log(tros.osi.serverp,CS_TRUE,buf,bytes);
		if(num_writ) *num_writ=bytes;
	}
	return 0;
}

CS_RETCODE
TraceOS_sradb_maint_handler(CS_VOID PNTR arg)
{
    CS_INT spid;
    CS_CHAR ustate[] = "SRA MAINT";
    SRV_PROC PNTR spp;
    UserThreadData tdata;

    srv_props(tros.osi.context,CS_GET,SRV_S_CURTHREAD,&spp,sizeof(spp),NULL);
    srv_thread_props(spp,CS_GET,SRV_T_SPID,(CS_VOID PNTR)&spid,sizeof(spid),NULL);
    srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)ustate,sizeof(ustate),NULL);
    memset(&tdata,0,sizeof(tdata));
    srv_thread_props(spp,CS_SET,SRV_T_USERDATA,(CS_VOID PNTR)&tdata,CS_SIZEOF(&tdata),NULL);
    srv_thread_props(spp,CS_GET,SRV_T_SPID,(CS_VOID PNTR)&tdata.spid,sizeof(tdata.spid),NULL);

    while(tros.sradb == NULL)
    {
        char logmsg[2048];
        FILE *fp = NULL;
        rc_t rc = 0;

        if(tros.sradb == NULL)
        {
            SRAPath *path_mgr;
            rc = SRAMgrMakeRead(&tros.sradb);
            if(rc != 0)
            {
                size_t x = sprintf(logmsg, "SRADB_INIT: FAILED to create Manager: ");
                RCExplain(rc, logmsg + x, sizeof(logmsg) - x, &x);
                os_ErrPost(tros.osi.context, spp, logmsg, 0, 0, CS_FALSE, 0);
                return CS_FAIL;
            }
            KLogHandlerSet(ErrPostForSRA, NULL);
            KLogLevelSet(klogInfo);

            rc = SRAPathMakeImpl ( & path_mgr, NULL );
            if ( rc == 0 )
            {
                SRAMgrUseSRAPath (tros.sradb, path_mgr);
                SRAPathRelease ( path_mgr );
            }
        }

        if(tros.sracfgfile) {
            fp = fopen(tros.sracfgfile, "r");
            if(!fp) {
                os_ErrPost(tros.osi.context,spp,"Failed to open SRA Configuration file",0,0,CS_FALSE,0);
            }
        }
        if(fp) {
            char p[3][1024];
            int	cnt = 0;
	    

            if(rc != 0){
                size_t x = sprintf(logmsg, "SRADB_INIT: FAILED to create Manager: ");
                RCExplain(rc, logmsg + x, sizeof(logmsg) - x, &x);
                os_ErrPost(tros.osi.context, spp, logmsg, 0, 0, CS_FALSE, 0);
                return CS_FAIL;
            }
            while( (cnt = fscanf(fp, "%1024s %1024s %1024s\n", p[0], p[1], p[2]) ) >= 0) {
                if(cnt == 3) {
                    /*** create SRAPath ***/
                    if(strcasecmp(p[0], "SRAPath") == 0) {
#if 0
                        char libpath[2050];
                        sprintf(libpath,"%s/%s",p[1],p[2]);
                        rc=SRAMgrInitPathLibrary (libpath);
                        if(rc != 0) {
                            size_t x = sprintf(logmsg, "SRADB_INIT: FAILED to init SRAPath: ");
                            RCExplain(rc, logmsg + x, sizeof(logmsg) - x, &x);
                            os_ErrPost(tros.osi.context, spp, logmsg, 0, 0, CS_FALSE, 0);
                            return CS_FAIL;
                        }
#endif
                    } else if(strcasecmp(p[0], "SRAManager") == 0) {
#if 0 /*** IGNORE ***/
                        if(strcmp(p[1], "-")) {
                            rc = SRAPathAddRepPath(tros.srapath, p[1]);
                            if(rc == 0) {
                                sprintf(logmsg, "SRADB_INIT: added path <%s>", p[1]);
                            } else {
                                size_t x = sprintf(logmsg, "SRADB_INIT: FAILED to add path <%s>: ", p[1]);
                                RCExplain(rc, logmsg + x, sizeof(logmsg) - x, &x);
                            }
                            os_ErrPost(tros.osi.context, spp, logmsg, 0, 0, CS_FALSE, 0);
                        }
                        if(strcmp(p[2], "-")) {
                            rc = SRAPathAddVolPath(tros.srapath, p[2]);
                            if(rc == 0) {
                                sprintf(logmsg, "SRADB_INIT: added volume <%s>", p[2]);
                            } else {
                                size_t x = sprintf(logmsg, "SRADB_INIT: FAILED to add volume <%s>: ", p[2]);
                                RCExplain(rc, logmsg + x, sizeof(logmsg) - x, &x);
                            }
                            os_ErrPost(tros.osi.context, spp, logmsg, 0, 0, CS_FALSE, 0);
                        }
#endif
                    } else if(strcasecmp(p[0], "FTP") == 0 || strcasecmp(p[0], "Aspera") == 0) {
                        char* a = malloc(strlen(p[1]) + strlen(p[2]) + 1);
                        if( a == NULL ) {
                            sprintf(logmsg, "SRADB_INIT: memory allocation error");
                            os_ErrPost(tros.osi.context, spp, logmsg, 0, 0, CS_FALSE, 0);
                            return CS_FAIL;
                        }
                        strcpy(a, p[1]);
                        strcat(a, p[2]);
                        if(strcasecmp(p[0], "FTP") == 0) { 
                            tros.ftp_prefix = a;
                            tros.ftp_prefix_len = strlen(tros.ftp_prefix);
                        } else {
                            tros.aspera_prefix = a;
                            tros.aspera_prefix_len = strlen(tros.aspera_prefix);
                        }
                        sprintf(logmsg, "SRADB_INIT: Param %s: %s", p[0], a);
                        os_ErrPost(tros.osi.context, spp, logmsg, 0, 0, CS_FALSE, 0);
                    }
                }
            }
            fclose(fp);
            fp = NULL;
        }
        if( tros.sradb == NULL ) {
            os_ErrPost(tros.osi.context,spp,"Failed to start SRA Database ... sleeping",0,0,CS_FALSE,0);
            sleep(60);
        }
    }
    while(1) {
        os_ErrPost(tros.osi.context,spp,"Running SRA filedb maintenance",0,0,CS_FALSE,100);
        SRAMgrRunBGTasks(tros.sradb);
        sleep(10);
    }
    if(srv_event(tros.service_thread, SRV_STOP, NULL) == CS_FAIL) {
        exit(1);
    }
    return CS_FAIL;
}

#define MACRO_GET_TI(TI,I_STR) {long tmpti; tmpti=atol(I_STR); if(tmpti > 0x7fffffff)TI=tmpti-0x100000000; else TI=tmpti;}

CS_RETCODE TraceOS_lang_handler(SRV_PROC PNTR spp)
{
	CS_CHAR		langbuf[4*1024]; /*** copy of original language request ***/
	CS_CHAR         langmsg[1024];     /*** this part will be tokenized *****/
	CS_CHAR		logmsg[2048];
	CS_CHAR PNTR	langptr=NULL;
	CS_CHAR PNTR	lang_alloc_ptr=NULL;
        CS_RETCODE      retcode=CS_SUCCEED;
        UserThreadDataPtr tdatap=NULL;
	CS_CHAR         *tokens[100]; /* tokens of language query */
	CS_INT		numtokens;
	CS_BOOL         iodead=CS_FALSE;
	CS_INT          status=SRV_DONE_FINAL; /* srv_senddone status.         */
	CS_INT		lang_len=0;
	int		nt;
	CS_INT          intinfo=0;


        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
                return CS_FAIL;
        }
	if((lang_len = srv_langlen(spp)) < 0){
		retcode=CS_FAIL;
                goto    EXIT;
	}
	sprintf(logmsg,"lang_len=%d",lang_len);
	os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,100);
	if(lang_len < sizeof(langbuf)){
		langptr=langbuf;
	} else {
		langptr=lang_alloc_ptr=srv_alloc(lang_len+1);
	}
        /* Copy the language query into the allocated buffer.           */
        if(srv_langcpy(spp,0,-1,langptr) < 0){
                retcode=CS_FAIL;
                goto    EXIT;
        }
	strncpy(langmsg,langptr,sizeof(langmsg)-1);
	os_ErrPost(NULL,spp,langmsg,0,0,CS_FALSE,100);
#define FIELDS_LANG "SET ROWCOUNT 0 select name,field_prim,searchable,static from DBA.Main order by name"
	if(   tros.trace_fields
	   && !strncasecmp(langmsg,FIELDS_LANG,sizeof(FIELDS_LANG)-1)){
		os_ErrPost(NULL,spp,"Redirecting to show_fields",0,0,CS_FALSE,0);
		srv_reginit(spp,"show_fields",CS_NULLTERM,SRV_M_PNOTIFYALL);
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        }


	numtokens=os_language_tokenizer(langmsg,tokens,sizeof(tokens)/sizeof(*tokens));
        if(numtokens==0){
                goto  DONE;
        }
	if(!strcmp(tokens[0],"use")){
                goto DONE;
        }
	if(!strcmp(tokens[0],"shutdown")){
                srv_reginit(spp,"shutdown",CS_NULLTERM,SRV_M_PNOTIFYALL);
                if(numtokens > 1){
                        srv_regparam(spp,"@passwd",CS_NULLTERM,CS_CHAR_TYPE,
                                strlen(tokens[1])+1,(CS_BYTE PNTR)tokens[1]);
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        }
    if(!strcmp(tokens[0],"set")) {
        if(numtokens > 1) {
            if(!strcmp(tokens[1],"debug_level")) {
                tros.debug_level = (numtokens > 2) ? (KLogLevel)atoi(tokens[2]) : klogInfo;
                DebugLevel = tros.debug_level;
                KLogLevelSet(tros.debug_level);
                sprintf(logmsg, "Debug level set to %d", tros.debug_level);
                os_ErrPost(NULL, spp, logmsg, 0, 0, CS_TRUE, 0);
                goto DONE;
            } else if(numtokens > 2 && !strcmp(tokens[1],"rowcount")) {
                tdatap->rowcount = atoi(tokens[2]);
                goto DONE;
            }
        }
    }
	if(!strcmp(tokens[0],"start_new_request_log")){
		CharPtr	newfile=NULL;
		CS_INT	intinfo;
		if(numtokens > 1){
			newfile=tokens[1];
		}
		srv_lockmutex(tros.log_mutex,SRV_M_WAIT,&intinfo);
		if(!TraceOS_new_request_log(newfile)){
			status|=SRV_DONE_ERROR;
		}
		srv_unlockmutex(tros.log_mutex);
		goto DONE;
	}

	
        if(!strcmp(tokens[0],"exec") || !strcmp(tokens[0],"execute")){/* remove exec from the line */
                nt = 1;
                numtokens--;
        } else {
                nt = 0;
        }
/* Find the name of the proc */

	switch(os_common_procs_as_lang(spp,tokens[nt])){
		case CS_FAIL:
			status |= SRV_DONE_ERROR;
                        goto DONE;
		case CS_SUCCEED:
			goto EXIT;
		case CS_CONTINUE:
		default:
			/*** nothing to do ***/
			break;
	}

	if(   !strcmp(tokens[nt],"usp_list_arrivals")
	   || !strcmp(tokens[nt],"usp_list_centers")
	   || !strcmp(tokens[nt],"usp_list_strategy")
	   || !strcmp(tokens[nt],"usp_list_species")
	   || !strcmp(tokens[nt],"usp_list_retrievals")
	   || !strcmp(tokens[nt],"usp_get_searchable_fields")
	   || !strcmp(tokens[nt],"usp_list_submission_type")
	   || !strcmp(tokens[nt],"usp_list_common_fields")){
		srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;

	}

	
	if(   !strcmp(tokens[nt],"show_image_locations")
           || !strcmp(tokens[nt],"show_data_locations")
           || !strcmp(tokens[nt],"show_altdata_locations")
           || !strcmp(tokens[nt],"show_fields")){
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        } else if(!strcmp(tokens[nt],"show_pools")){
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        } else if(!strcmp(tokens[nt],"usp_get_trace_data")){
                CS_INT  first=0,last=0;
                CS_INT outform=0;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
			MACRO_GET_TI(first,tokens[nt]);
                        srv_regparam(spp,"@first",CS_NULLTERM,CS_INT_TYPE,sizeof(first),(CS_BYTE PNTR)&first);
                        nt++;
                        numtokens--;
                        if(numtokens > 0){
				MACRO_GET_TI(last,tokens[nt]);
                                srv_regparam(spp,"@last",CS_NULLTERM,CS_INT_TYPE,sizeof(last),(CS_BYTE PNTR)&last);
                                nt++;
                                numtokens--;
                                if(numtokens > 0){
                                        outform = atoi(tokens[nt]);
                                        srv_regparam(spp,"@form",CS_NULLTERM,CS_INT_TYPE,sizeof(outform),(CS_BYTE PNTR)&outform);
                                        nt++;
                                        numtokens--;
                                }
                        }
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        } else if(!strcmp(tokens[nt],"trace_query")){
                CS_INT  page_size=0,page_number=0;
                CS_CHAR PNTR sql=0;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
			sql=tokens[nt];
                        srv_regparam(spp,"@sql",CS_NULLTERM,CS_CHAR_TYPE,strlen(sql),(CS_BYTE PNTR)sql);
                        nt++;
                        numtokens--;
                        if(numtokens > 0){
                                page_size = atoi(tokens[nt]);
                                srv_regparam(spp,"@page_size",CS_NULLTERM,CS_INT_TYPE,sizeof(page_size),(CS_BYTE PNTR)&page_size);
                                nt++;
                                numtokens--;
                                if(numtokens > 0){
                                        page_number = atoi(tokens[nt]);
                                        srv_regparam(spp,"@page_number",CS_NULLTERM,CS_INT_TYPE,sizeof(page_number),(CS_BYTE PNTR)&page_number);
                                        nt++;
                                        numtokens--;
                                }
                        }
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        }  else if(!strcmp(tokens[nt],"sra_flush")){
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
			CS_CHAR	*acc=tokens[nt];
			CS_INT	acclen=strlen(acc);
                        srv_regparam(spp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,acclen,(CS_BYTE PNTR)acc);
                        nt++;
                        numtokens--;
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        }  else if(!strcmp(tokens[nt],"sra_list_references")){
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
                        CS_CHAR *acc=tokens[nt];
                        CS_INT  acclen=strlen(acc);
                        srv_regparam(spp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,acclen,(CS_BYTE PNTR)acc);
                        nt++;
                        numtokens--;
			if(numtokens > 0){
				CS_CHAR *name=tokens[nt];
	                        CS_INT  namelen=strlen(name);
                        	srv_regparam(spp,"@name",CS_NULLTERM,CS_CHAR_TYPE,namelen,(CS_BYTE PNTR)name);
				nt++;
				numtokens--;
				if(numtokens > 0){
                                        CS_INT intval = atoi(tokens[nt]);
                                        srv_regparam(spp,"@max_seq_len",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
                                        nt++;
                                        numtokens--;
					if(numtokens > 0){
						CS_INT intval = atoi(tokens[nt]);
						srv_regparam(spp,"@num_bins",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
						nt++;
						numtokens--;

					}
                                }
			}
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        }  else if(!strcmp(tokens[nt],"sra_get_run_data")){
		CS_INT	intval;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
			CS_CHAR	*acc=tokens[nt];
			CS_INT	acclen=strlen(acc);
                        srv_regparam(spp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,acclen,(CS_BYTE PNTR)acc);
                        nt++;
                        numtokens--;
			if(numtokens > 0){
				intval = atoi(tokens[nt]);
				srv_regparam(spp,"@spot_first",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
				nt++;
				numtokens--;
				if(numtokens > 0){
					intval = atoi(tokens[nt]);
					srv_regparam(spp,"@spot_last",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
					nt++;
					numtokens--;
					if(numtokens > 0){
						intval = atoi(tokens[nt]);
						srv_regparam(spp,"@spot_cnt",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
						nt++;
						numtokens--;
						if(numtokens > 0){
							intval = atoi(tokens[nt]);
							srv_regparam(spp,"@format_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
							nt++;
							numtokens--;
							if(numtokens > 0){
								intval = atoi(tokens[nt]);
								srv_regparam(spp,"@read_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
								nt++;
								numtokens--;
								if(numtokens > 0){
									intval = atoi(tokens[nt]);
									srv_regparam(spp,"@do_clip",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
									nt++;
									numtokens--;
								}
							}
						}
					}
				}
			}
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        } else if(!strcmp(tokens[nt],"sra_find_spot")){
		CS_INT	intval;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
			CS_CHAR	*acc=tokens[nt];
			CS_INT	acclen=strlen(acc);
                        srv_regparam(spp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,acclen,(CS_BYTE PNTR)acc);
                        nt++;
                        numtokens--;
			if(numtokens > 0){
				CS_CHAR *sn=tokens[nt];
				CS_INT	sn_len=strlen(sn);
				srv_regparam(spp,"@spotname",CS_NULLTERM,CS_CHAR_TYPE,sn_len,(CS_BYTE PNTR)sn);
				nt++;
				numtokens--;
				if(numtokens > 0){
					intval = atoi(tokens[nt]);
					srv_regparam(spp,"@range_x",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
					nt++;
					numtokens--;
					if(numtokens > 0){
						intval = atoi(tokens[nt]);
						srv_regparam(spp,"@range_y",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
						nt++;
						numtokens--;
						if(numtokens > 0){
							intval = atoi(tokens[nt]);
							srv_regparam(spp,"@format_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
							nt++;
							numtokens--;
							if(numtokens > 0){
								intval = atoi(tokens[nt]);
								srv_regparam(spp,"@read_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
								nt++;
								numtokens--;
								if(numtokens > 0){
									intval = atoi(tokens[nt]);
									srv_regparam(spp,"@do_clip",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
									nt++;
									numtokens--;
									if(numtokens > 0){
										intval = atoi(tokens[nt]);
										srv_regparam(spp,"@pagesize",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
										nt++;
										numtokens--;
										if(numtokens > 0){
											intval = atoi(tokens[nt]);
											srv_regparam(spp,"@pageno",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
											nt++;
											numtokens--;

										}
									}
								}
							}
						}
					}
				}
			}
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        }  else if(!strcmp(tokens[nt],"sra_get_run_files")){
            CS_INT	intval;
            srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
            nt++;
            numtokens--;
            if(numtokens > 0){
                CS_CHAR	*acc=tokens[nt];
                CS_INT	acclen=strlen(acc);
                srv_regparam(spp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,acclen,(CS_BYTE PNTR)acc);
                nt++;
                numtokens--;
                if(numtokens > 0){
                    intval = atoi(tokens[nt]);
                    srv_regparam(spp,"@signal",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
                    nt++;
                    numtokens--;
                    if(numtokens > 0){
                        intval = atoi(tokens[nt]);
                        srv_regparam(spp,"@names",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
                        nt++;
                        numtokens--;
                        if(numtokens > 0){
                            intval = atoi(tokens[nt]);
                            srv_regparam(spp,"@paths",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
                            nt++;
                            numtokens--;
                        }
                    }
                }
            }
            if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
            }
            goto EXIT;
        } else if(!strcmp(tokens[nt],"sra_find_seq")){
		CS_INT	intval;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
			CS_CHAR	*acc=tokens[nt];
			CS_INT	acclen=strlen(acc);
                        srv_regparam(spp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,acclen,(CS_BYTE PNTR)acc);
                        nt++;
                        numtokens--;
			if(numtokens > 0){
				CS_CHAR *sn=tokens[nt];
				CS_INT	sn_len=strlen(sn);
				srv_regparam(spp,"@seq",CS_NULLTERM,CS_CHAR_TYPE,sn_len,(CS_BYTE PNTR)sn);
				nt++;
				numtokens--;
				if(numtokens > 0){
					intval = atoi(tokens[nt]);
					srv_regparam(spp,"@format_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
					nt++;
					numtokens--;
					if(numtokens > 0){
						intval = atoi(tokens[nt]);
						srv_regparam(spp,"@read_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
						nt++;
						numtokens--;
						if(numtokens > 0){
							intval = atoi(tokens[nt]);
							srv_regparam(spp,"@do_clip",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
							nt++;
							numtokens--;
							if(numtokens > 0){
								intval = atoi(tokens[nt]);
								srv_regparam(spp,"@pagesize",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
								nt++;
								numtokens--;
								if(numtokens > 0){
									intval = atoi(tokens[nt]);
									srv_regparam(spp,"@pageno",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
									nt++;
									numtokens--;
									if(numtokens > 0){
										intval = atoi(tokens[nt]);
										srv_regparam(spp,"@read_mask_search",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
										nt++;
										numtokens--;
									}
								}
							}
						}
					}
				}
			}
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        } else if(!strcmp(tokens[nt],"sra_find_group")){
		CS_INT	intval;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
			CS_CHAR	*acc=tokens[nt];
			CS_INT	acclen=strlen(acc);
                        srv_regparam(spp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,acclen,(CS_BYTE PNTR)acc);
                        nt++;
                        numtokens--;
			if(numtokens > 0){
				CS_CHAR *sn=tokens[nt];
				CS_INT	sn_len=strlen(sn);
				srv_regparam(spp,"@group",CS_NULLTERM,CS_CHAR_TYPE,sn_len,(CS_BYTE PNTR)sn);
				nt++;
				numtokens--;
				if(numtokens > 0){
					intval = atoi(tokens[nt]);
					srv_regparam(spp,"@format_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
					nt++;
					numtokens--;
					if(numtokens > 0){
						intval = atoi(tokens[nt]);
						srv_regparam(spp,"@read_mask",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
						nt++;
						numtokens--;
						if(numtokens > 0){
							intval = atoi(tokens[nt]);
							srv_regparam(spp,"@do_clip",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
							nt++;
							numtokens--;
							if(numtokens > 0){
								intval = atoi(tokens[nt]);
								srv_regparam(spp,"@pagesize",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
								nt++;
								numtokens--;
								if(numtokens > 0){
									intval = atoi(tokens[nt]);
									srv_regparam(spp,"@pageno",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
									nt++;
									numtokens--;
								}
							}
						}
					}
				}
			}
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        } else if(!strcmp(tokens[nt],"sra_get_signal") || !strcmp(tokens[nt],"sra_get_sequence")){
                CS_INT  intval;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
                        CS_CHAR *acc=tokens[nt];
                        CS_INT  acclen=strlen(acc);
                        srv_regparam(spp,"@acc",CS_NULLTERM,CS_CHAR_TYPE,acclen,(CS_BYTE PNTR)acc);
                        nt++;
                        numtokens--;
                        if(numtokens > 0){
                                intval = atoi(tokens[nt]);
                                srv_regparam(spp,"@spotid",CS_NULLTERM,CS_INT_TYPE,sizeof(intval),(CS_BYTE PNTR)&intval);
                                nt++;
                                numtokens--;
                        }
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        }  else if(!strcmp(tokens[nt],"get_trace_data_by_ti")){
                CS_INT  ti=0;
		CS_TINYINT mask=0;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
			MACRO_GET_TI(ti,tokens[nt]);
                        srv_regparam(spp,"@ti",CS_NULLTERM,CS_INT_TYPE,sizeof(ti),(CS_BYTE PNTR)&ti);
                        nt++;
                        numtokens--;
                        if(numtokens > 0){
                                mask = atoi(tokens[nt]);
                                srv_regparam(spp,"@mask",CS_NULLTERM,CS_TINYINT_TYPE,sizeof(mask),(CS_BYTE PNTR)&mask);
                                nt++;
                                numtokens--;
                        }
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
	} else if(!strcmp(tokens[nt],"id_get_asnprop")){
                CS_INT  sat_key=0;
		CS_SMALLINT sat=28;/***TRACE**/
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
			MACRO_GET_TI(sat_key,tokens[nt]);
                        srv_regparam(spp,"@sat_key",CS_NULLTERM,CS_INT_TYPE,sizeof(sat_key),(CS_BYTE PNTR)&sat_key);
                        nt++;
                        numtokens--;
			if(numtokens >0){
				nt++;
				numtokens--;
				if(numtokens >0){
					sat=atoi(tokens[nt]);
					srv_regparam(spp,"@sat",CS_NULLTERM,CS_SMALLINT_TYPE,sizeof(sat_key),(CS_BYTE PNTR)&sat);
					nt++;
		                        numtokens--;
				}
			}
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        } else if(   !strcmp(tokens[nt],"usp_range_by_submission_id")
		  || !strcmp(tokens[nt],"usp_sid_show_log")
		){
                CS_INT  sid=0;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
                        sid = atoi(tokens[nt]);
                        srv_regparam(spp,"@sid",CS_NULLTERM,CS_INT_TYPE,sizeof(sid),(CS_BYTE PNTR)&sid);
                        nt++;
                        numtokens--;
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        }  else if(   !strcmp(tokens[nt],"usp_show_reset_pending_status")
		   || !strcmp(tokens[nt],"usp_reset_pending_status")){
                CS_INT  sid=0;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
                        sid = atoi(tokens[nt]);
                        srv_regparam(spp,"@sid",CS_NULLTERM,CS_INT_TYPE,sizeof(sid),(CS_BYTE PNTR)&sid);
                        nt++;
                        numtokens--;
			if(numtokens > 0){
				srv_regparam(spp,"@user_name",CS_NULLTERM,CS_CHAR_TYPE,strlen(tokens[nt]),(CS_BYTE PNTR)tokens[nt]);
				nt++;
				numtokens--;
			}
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        }else if(!strcmp(tokens[nt],"usp_tracking")){
                CS_INT  page=0;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
                if(numtokens > 0){
                        page = atoi(tokens[nt]);
                        srv_regparam(spp,"@page_size",CS_NULLTERM,CS_INT_TYPE,sizeof(page),(CS_BYTE PNTR)&page);
                        nt++;
                        numtokens--;
			if(numtokens > 0){
				page = atoi(tokens[nt]);
				srv_regparam(spp,"@page_number",CS_NULLTERM,CS_INT_TYPE,sizeof(page),(CS_BYTE PNTR)&page);
				nt++;
				numtokens--;
				if(numtokens > 0){
					srv_regparam(spp,"@filter",CS_NULLTERM,CS_CHAR_TYPE,strlen(tokens[nt]),(CS_BYTE PNTR)tokens[nt]);
					nt++;
					numtokens--;

				}
			}
                }
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        } else if(!strcmp(tokens[nt],"usp_list_species_pager")){
                CS_INT  page=0;
                srv_reginit(spp,tokens[nt],CS_NULLTERM,SRV_M_PNOTIFYALL);
                nt++;
                numtokens--;
		if(numtokens > 0){
			srv_regparam(spp,"@letter",CS_NULLTERM,CS_CHAR_TYPE,strlen(tokens[nt]),(CS_BYTE PNTR)tokens[nt]);
			nt++;
			numtokens--;
			if(numtokens > 0){
				page = atoi(tokens[nt]);
				srv_regparam(spp,"@page_number",CS_NULLTERM,CS_INT_TYPE,sizeof(page),(CS_BYTE PNTR)&page);
				nt++;
				numtokens--;
				if(numtokens > 0){
					page = atoi(tokens[nt]);
					srv_regparam(spp,"@page_size",CS_NULLTERM,CS_INT_TYPE,sizeof(page),(CS_BYTE PNTR)&page);
					nt++;
					numtokens--;
				}
			}
		}
                if(srv_regexec(spp,&intinfo)!=CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
                goto EXIT;
        }

	os_ErrPost(NULL,spp,langptr,0,0,CS_FALSE,0);
	if(tros.iqconn.is_online){
		if(os_ExecOnBestConnection(spp,tdatap,&tros.iqconn,langptr,
		       (1/*tdatap->byteorder != tros.osi.byteorderi*/)?os_gateway_lang_func_cb:os_passthrough_lang_func_cb
			       ,CS_FALSE) != CS_SUCCEED){
			retcode=CS_FAIL;
		}
		goto EXIT;
	} else {
		os_ErrPost(NULL,spp,"IQ connection is missing - language call was not recognized",0,10,CS_TRUE,0);
	}
DONE:
	if(srv_thread_props(spp,CS_GET,SRV_T_IODEAD,&iodead,sizeof(iodead),NULL)!=CS_SUCCEED && iodead){
                os_ErrPost(NULL,spp,"IO channel is dead",0,0,CS_FALSE,0);
                return CS_FAIL;
        }
        if (srv_senddone(spp,status,CS_TRAN_UNDEFINED,0) == CS_FAIL) {
                sprintf(logmsg,"%s: srv_senddone() failed... ",tros.osi.srvname);
                os_ErrPost(NULL,spp,logmsg,0,10,CS_TRUE,0);
        }

EXIT:
	if(lang_alloc_ptr) srv_free(lang_alloc_ptr);
        return(retcode);
}
/*****************************************/
/************* STORED PROCS   ************/
/*****************************************/
CS_RETCODE
TraceOS_shutdown(SRV_PROC PNTR spp)
{
        CS_INT          numparams;
        CS_CHAR         pwd_shutdown[51];
        CS_INT          pwd_len = sizeof(pwd_shutdown);
        CS_DATAFMT      cs_passwd_fmt = {"@passwd",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,50,CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_SMALLINT     outlen;
        CS_INT          status = SRV_DONE_FINAL;

        if (srv_numparams(spp,&numparams) != CS_SUCCEED || numparams < 1){
                os_ErrPost(NULL,spp,MSG_NO_PASSWORD,ERR_NO_PASSWORD,10,CS_TRUE,0);
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_passwd_fmt,(CS_BYTE *)pwd_shutdown,&pwd_len,&outlen) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        MACRO_SRV_XFERDATA(CS_GET,SRV_RPCDATA);
        if(strcmp(pwd_shutdown,DEFAULT_PASSWORD)){
                os_ErrPost(NULL,spp,MSG_INVALID_PASSWORD,ERR_INVALID_PASSWORD,10,CS_TRUE,0);
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        os_ErrPost(NULL,spp,"Shutting down...",0,0,CS_TRUE,0);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"SHUTTING DOWN",sizeof("SHUTTING DOWN"),NULL);
        /****while(ct_exit(lnos.osi.context,CS_UNUSED)!=CS_SUCCEED){}****/
        if(srv_event(spp, SRV_STOP, (CS_VOID *) NULL) != SRV_STOP){
                status |= SRV_DONE_ERROR;
        }
DONE:
        srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-100:0);
        srv_senddone(spp,status,CS_TRAN_COMPLETED,0);
        return CS_SUCCEED;
}


CS_RETCODE
TraceOS_tracemain_gateway(SRV_PROC PNTR spp)
{
	UserThreadDataPtr tdatap=NULL;
	CS_INT	len;
	CS_CHAR	sql[100];
	CS_CHAR PNTR np;

        srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL);
	np=srv_rpcname(spp,&len);
	if(len<=0 || np==NULL) return CS_FAIL;
	strncpy(sql,np,len);
	sql[len]='\0';
	return os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,sql,
	   (1/*tdatap->byteorder != tros.osi.byteorder*/)?os_gateway_lang_func_cb:os_passthrough_lang_func_cb,CS_FALSE);
}

CS_RETCODE
TraceOS_show_image_locations(SRV_PROC PNTR spp)
{
	return TraceOS_show_locations_func(spp,tros.trlocs.trloc_img,tros.trlocs.numsrv_img);
}
CS_RETCODE
TraceOS_show_data_locations(SRV_PROC PNTR spp)
{
        return TraceOS_show_locations_func(spp,tros.trlocs.trloc_data,tros.trlocs.numsrv_data);
}
CS_RETCODE
TraceOS_show_altdata_locations(SRV_PROC PNTR spp)
{
        return TraceOS_show_locations_func(spp,tros.trlocs.trloc_alt,tros.trlocs.numsrv_alt);
}
CS_RETCODE
TraceOS_show_fields(SRV_PROC PNTR spp)
{
    CS_INT          status = SRV_DONE_FINAL;
    CS_CHAR         name[51],field_prim[51];
	CS_BIT		    is_searchable,is_static;
	TraceFieldPtr	trf_run;
    UserThreadDataPtr tdatap=NULL;
    CS_DATAFMT      cs_searchable_fmt =
            {"searchable",CS_NULLTERM,CS_BIT_TYPE,CS_FMT_UNUSED,sizeof(CS_BIT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_DATAFMT      cs_static_fmt =
            {"static",CS_NULLTERM,CS_BIT_TYPE,CS_FMT_UNUSED,sizeof(CS_BIT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_DATAFMT      cs_name_fmt =
            {"name",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,50,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_DATAFMT      cs_field_prim_fmt =
            {"field_prim",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,50,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};

	CS_INT          bitlen=sizeof(CS_BIT),namelen,field_primlen;
	CS_INT		    rowcount=0;

	if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
        status |= SRV_DONE_ERROR;
        goto DONE;
    }
	
 	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_name_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_name_fmt,(CS_BYTE*)name,&namelen,NULL)!=CS_SUCCEED
 	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,2,&cs_field_prim_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,2,&cs_field_prim_fmt,(CS_BYTE*)field_prim,&field_primlen,NULL)!=CS_SUCCEED
           || srv_descfmt(spp,CS_SET,SRV_ROWDATA,3,&cs_searchable_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,3,&cs_searchable_fmt,(CS_BYTE*)&is_searchable,&bitlen,NULL)!=CS_SUCCEED
           || srv_descfmt(spp,CS_SET,SRV_ROWDATA,4,&cs_static_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,4,&cs_static_fmt,(CS_BYTE*)&is_static,&bitlen,NULL)!=CS_SUCCEED
                ){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
	for(trf_run=tros.trace_fields;trf_run;trf_run=trf_run->next,rowcount++){
		is_searchable=trf_run->is_searchable;
		is_static=trf_run->is_static;
		strcpy(name,trf_run->name); namelen=strlen(name);
		strcpy(field_prim,trf_run->field_prim); field_primlen=strlen(field_prim);
		if(srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED){
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
	}
DONE:
	/*srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?100:0);*/
	srv_senddone(spp,status,CS_TRAN_UNDEFINED,rowcount);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
        return CS_SUCCEED;
}


CS_RETCODE
os_output_image_data(SRV_PROC PNTR spp,CS_CHAR PNTR name,CS_BOOL is_text,CS_BYTE PNTR data,CS_INT len)
{
        CS_DATAFMT      cs_fmt= {"",CS_NULLTERM,CS_TEXT_TYPE,CS_FMT_UNUSED,1,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,1,0,NULL};
	CS_IODESC       iodesc;

	memset(&iodesc,0,sizeof(iodesc));
	iodesc.iotype=CS_IODATA;
	strcpy(iodesc.name,name?name:"noname");
	iodesc.namelen=CS_NULLTERM;
	iodesc.timestamplen=CS_TS_SIZE;
	iodesc.textptrlen  =CS_TP_SIZE;
	iodesc.datatype = (is_text?CS_TEXT_TYPE:CS_IMAGE_TYPE);
	iodesc.total_txtlen = len;

	strcpy(cs_fmt.name,name?name:"noname");
	cs_fmt.datatype = (is_text?CS_TEXT_TYPE:CS_IMAGE_TYPE);
	cs_fmt.maxlength = len;

	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_fmt) != CS_SUCCEED
	   || srv_text_info(spp,CS_SET,1,&iodesc) != CS_SUCCEED
	   || srv_send_text(spp,data,len) != CS_SUCCEED){
		char logmsg[1024];
		sprintf(logmsg,"os_output_image_data| failed to send <%d> bytes for column <%s>",iodesc.total_txtlen,iodesc.name);
		os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
		return CS_FAIL;
	}
  	return srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,1);
}



CS_RETCODE
TraceOS_usp_get_trace_data(SRV_PROC PNTR spp)
{
        CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_DATAFMT      cs_smallint_fmt = {"",CS_NULLTERM,CS_SMALLINT_TYPE,CS_FMT_UNUSED,sizeof(CS_SMALLINT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
	CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_bin_fmt = {"",CS_NULLTERM,CS_BINARY_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_INT          status = SRV_DONE_FINAL;
        CS_CHAR         logmsg[512],sql[1024],buf[256];
	CS_CHAR PNTR	sqlptr;
        CS_INT          int_len = sizeof(CS_INT),smallint_len=sizeof(CS_SMALLINT);
	CS_INT		char_len;
        UserThreadDataPtr tdatap=NULL;
	Uint4		ti,ti_first,ti_last;
	Uint4		ti_msg_first=0,ti_msg_last=0;
	Int8		last_replaced_by=0;
	CS_INT		outform,form;
	int		i_data=0,i_alt=0,i;
	CS_SMALLINT	order_id;
	Int4		im_src=0;
	CS_INT		errcode=0;

#define MACRO_OUTPUT_INTEGER(NAME,INTVAL) {\
	CS_INT	val=INTVAL;\
	strcpy(cs_int_fmt.name,#NAME);\
	srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_int_fmt); \
	srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_int_fmt,(CS_BYTE*)&val,&int_len,NULL); \
	srv_xferdata(spp,CS_SET,SRV_ROWDATA); \
	srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,1);}



        if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_int_fmt,(CS_BYTE *)&ti_first,NULL,NULL) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_int_fmt,(CS_BYTE *)&ti_last,NULL,NULL) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,3,&cs_int_fmt,(CS_BYTE *)&form,NULL,NULL) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }

	sprintf(logmsg,"Calling usp_get_trace_data  %d,%d,%d",ti_first,ti_last,form);
	os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
	TraceOS_log_request(ti_first,ti_last,form);

	if(ti_first > ti_last || form==0){
os_ErrPost(NULL,spp,"    ",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"Usage 'usp_get_trace_data first,last[,form]'",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                          first,last - positive integer trace identifiers",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                          form       - integer output format qualifyer",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"    ",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"Format valid values are:  1 - information",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                          2 - base calls",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                          4 - quality scores",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                          8 - trace image",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                         16 - peak positions",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                         32 - comment",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                         64 - mate pairs",0,5,CS_TRUE,100);
		status |= SRV_DONE_ERROR;
                goto DONE;
        }

	tdatap->got_attention=0;
	for(ti=ti_first;ti <= ti_last && !tdatap->got_attention;ti++){
		TIintro	tii;
		TraceData *tdp;
		
		memset(&tii,0,sizeof(tii));
		tii.ti=ti;
		tii.replaced_by=-256;
		if(os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,&tii,TraceOS_get_trace_intro_cb,CS_FALSE) != CS_SUCCEED){
                        status |= SRV_DONE_ERROR;
                        goto DONE;
                }
		if(tii.replaced_by != last_replaced_by){
			if(last_replaced_by < 0 && last_replaced_by >=-256){
				switch(last_replaced_by){
				 case -256:
					sprintf(logmsg,"TIs <%d:%d> are not found",ti_msg_first,ti_msg_last);
					errcode=ERR_CODE_NOTFOUND;
					break;
				 case -100:
				 case -10:
					sprintf(logmsg,"TIs <%d:%d> are withdrawn",ti_msg_first,ti_msg_last);
					errcode=ERR_CODE_WITHDRAWN;
					break;
				 case -1:
					sprintf(logmsg,"TIs <%d:%d> are not completely loaded",ti_msg_first,ti_msg_last);
					errcode=ERR_CODE_NOTLOADED;
                                        break;
				 default:
					sprintf(logmsg,"TIs <%d:%d> have unexpected status",ti_msg_first,ti_msg_last);
					errcode=ERR_CODE_UNKNOWN;
                                        break;
				}
				os_ErrPost(NULL,spp,logmsg,errcode,10,CS_TRUE,0);
			}
			ti_msg_first=ti_msg_last=ti;
			last_replaced_by=tii.replaced_by;
		} else {
			ti_msg_last=ti;
		}
		
		if(tii.replaced_by==-256) continue;

		if(ti_first != ti_last && tii.replaced_by != 0){
			if(tii.replaced_by > 0){
				sprintf(logmsg,"TI <%d> has been replaced by <%d>",ti,tii.replaced_by);
				os_ErrPost(NULL,spp,logmsg,ERR_CODE_REPLACED,10,CS_TRUE,0);
			} 
                        continue;
		}
		if(TraceOS_ti_intro_output_func(spp,&tii)!=CS_SUCCEED){
			status |= SRV_DONE_ERROR;
                        goto DONE;
		}
		sqlptr=sql;
		outform=form;
		/*** header ***/
		if(outform & GET_TRACE_DATA_INFO){
			sqlptr += sprintf(sqlptr,"exec usp_tros_get_trace_info %d ",ti);
			outform &= ~GET_TRACE_DATA_INFO;
		}
		if((outform & GET_TRACE_DATA_MATE) || (outform & GET_TRACE_DATA_BASECALL)){
			sqlptr += sprintf(sqlptr,"exec usp_tros_get_trace_mate %d,%d,10 ",ti,ti);
			outform &= ~GET_TRACE_DATA_MATE;
		}
		if(sqlptr != sql && os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,sql,os_gateway_lang_func_cb_results_only,CS_FALSE) != CS_SUCCEED){
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
		sqlptr=sql;

		if(outform & GET_TRACE_DATA_IMAGE){
			if(TraceOS_get_trace_image(spp,tdatap,&tii,&im_src) != CS_SUCCEED){
				status |= SRV_DONE_ERROR;
				goto DONE;
			}
			outform &= ~GET_TRACE_DATA_IMAGE;
		}
		if((outform&GET_TRACE_DATA_ALT) && (outform != GET_TRACE_DATA_ALT)){
				TraceDataRequest	tdr;
				while(ti > tros.trlocs.trloc_alt[i_alt].ti_stop  && tros.trlocs.trloc_alt[i_alt].ti_stop != 0xFFFFFFFF){
					i_alt++;
					if(i_alt >  tros.trlocs.numsrv_alt){
						os_ErrPost(NULL,spp,"AltData location cache is corrupted...",0,10,CS_TRUE,0);
						status |= SRV_DONE_ERROR;
						goto DONE;
					}
				}	
				tdr.ti=ti;
				tdr.dbname=tros.trlocs.trloc_alt[i_alt].dbname;
				if(os_ExecOnBestConnection(spp,tdatap,tros.trlocs.trloc_alt[i_alt].bsrv,&tdr,TraceOS_get_alt_data_cb,CS_FALSE) != CS_SUCCEED){
					status |= SRV_DONE_ERROR;
					goto DONE;
				}
				if(tdr.basecall_len > 0){
					MACRO_OUTPUT_INTEGER(alt_basecall_len,tdr.basecall_len);
					if(outform & GET_TRACE_DATA_BASECALL){
						strcpy(cs_smallint_fmt.name,"border_id");
						srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_smallint_fmt); 
						srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_smallint_fmt,(CS_BYTE*)&order_id,&smallint_len,NULL); 
						strcpy(cs_char_fmt.name,"base_call");
						srv_descfmt(spp,CS_SET,SRV_ROWDATA,2,&cs_char_fmt);
						srv_bind(spp,CS_SET,SRV_ROWDATA,2,&cs_char_fmt,(CS_BYTE*)buf,&char_len,NULL);
						for(i=0,order_id=0;i<tdr.basecall_len;order_id++){
							char_len=tdr.basecall_len-i;
							if(char_len > 254) char_len=254;
							memcpy(buf,tdr.basecall+i,char_len);
							i+=char_len;
							srv_xferdata(spp,CS_SET,SRV_ROWDATA);
						}
						srv_senddone(spp,SRV_DONE_MORE,CS_TRAN_UNDEFINED,order_id);
						MACRO_OUTPUT_INTEGER(alternative,1);
						outform &= ~GET_TRACE_DATA_BASECALL;
					}	
					if(outform & GET_TRACE_DATA_QSCORES){
						strcpy(cs_smallint_fmt.name,"qorder_id");
						srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_smallint_fmt); 
						srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_smallint_fmt,(CS_BYTE*)&order_id,&smallint_len,NULL); 
						strcpy(cs_bin_fmt.name,"qual_score");
						srv_descfmt(spp,CS_SET,SRV_ROWDATA,2,&cs_bin_fmt);
						srv_bind(spp,CS_SET,SRV_ROWDATA,2,&cs_bin_fmt,(CS_BYTE*)buf,&char_len,NULL);
						for(i=0,order_id=0;i<tdr.basecall_len;order_id++){
							char_len=tdr.basecall_len-i;
							if(char_len > 254) char_len=254;
							memcpy(buf,tdr.qualscore+i,char_len);
							buf[char_len]='\255';
							i+=char_len;
							char_len++;
							srv_xferdata(spp,CS_SET,SRV_ROWDATA);
						}
						srv_senddone(spp,SRV_DONE_MORE,CS_TRAN_UNDEFINED,order_id);
						MACRO_OUTPUT_INTEGER(alternative,1);
						outform &= ~GET_TRACE_DATA_QSCORES;
					}
					if(outform & GET_TRACE_DATA_PEAK){
						strcpy(cs_smallint_fmt.name,"order_id");
						srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_smallint_fmt); 
						srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_smallint_fmt,(CS_BYTE*)&order_id,&smallint_len,NULL); 
						strcpy(cs_bin_fmt.name,"qual_score");
						srv_descfmt(spp,CS_SET,SRV_ROWDATA,2,&cs_bin_fmt);
						srv_bind(spp,CS_SET,SRV_ROWDATA,2,&cs_bin_fmt,(CS_BYTE*)buf,&char_len,NULL);
						for(i=0,order_id=0;i<tdr.basecall_len;order_id++){
							char_len=tdr.basecall_len-i;
							if(char_len > 254) char_len=254;
							memcpy(buf,tdr.peakidx+i,char_len);
							buf[char_len]='\255';
							i+=char_len;
							char_len++;
							srv_xferdata(spp,CS_SET,SRV_ROWDATA);
						}
						srv_senddone(spp,SRV_DONE_MORE,CS_TRAN_UNDEFINED,order_id);
						MACRO_OUTPUT_INTEGER(alternative,1);
						outform &= ~GET_TRACE_DATA_PEAK;
					}
				}
				outform &= ~GET_TRACE_DATA_ALT;
		}
		if(outform > 0){ /**** only trace_data left ***/
			if((i_data=TraceOS_tracedb_find_data(spp,tii.ti,outform,&tdp)) < 0){
                                status |= SRV_DONE_ERROR;
                                goto DONE;
                        }
			if(tdp){
				if(outform & GET_TRACE_DATA_BASECALL){
					size_t    bc_len=0;
					CharPtr  bc=NULL;
					bc=(CharPtr)TraceDataGetBasecall(tdp,&bc_len);
#ifdef _DEBUG
					sprintf(logmsg,"TI=%d|bc_len=%d ",ti,bc_len);
                                        os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
#endif
					if( bc && bc_len){
						if(os_output_image_data(spp,"base_call_full",TRUE,(CS_BYTE PNTR)bc,bc_len)==CS_SUCCEED){
							outform &= ~GET_TRACE_DATA_BASECALL;
						}
					} else {
						sprintf(logmsg,"TI=%d| Failed to retrieve basecalls",ti);
						os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
					}
				}
				if(outform & GET_TRACE_DATA_QSCORES){
                                        size_t    qs_len=0;
                                        const void *  qs=NULL;
                                        qs=TraceDataGetQualscore(tdp,&qs_len);
#ifdef _DEBUG
					sprintf(logmsg,"TI=%d|qs_len=%d ",ti,qs_len);
                                        os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
#endif
                                        if( qs && qs_len){
                                                if(os_output_image_data(spp,"qual_score_full",FALSE,(CS_BYTE PNTR)qs,qs_len)==CS_SUCCEED){
                                                        outform &= ~GET_TRACE_DATA_QSCORES;
                                                }
                                        } else {
                                                sprintf(logmsg,"TI=%d| Failed to retrieve quality scores",ti);
                                                os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
                                        }
                                }
				if(outform & GET_TRACE_DATA_PEAK){
                                        size_t    pp_len=0;
                                        const void *  pp=NULL;
                                        pp=TraceDataGetPeakindex(tdp,&pp_len);
#ifdef _DEBUG
					sprintf(logmsg,"TI=%d|pp_len=%d ",ti,pp_len);
                                        os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
#endif
					if( pp_len != tii.basecall_len*4){
						sprintf(logmsg,"TI=%d|bad length of peak_index <%d> expected <%d>",ti,(int)pp_len,tii.basecall_len*4);
	                                        os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
						if(pp_len < tii.basecall_len*4){
							pp=dummy_buf;
						}
						pp_len=tii.basecall_len*4;
					}
                                        if( pp && pp_len){
                                                if(os_output_image_data(spp,"peak_index_full",FALSE,(CS_BYTE PNTR)pp,pp_len)==CS_SUCCEED){
                                                        outform &= ~GET_TRACE_DATA_PEAK;
                                                }
                                        } else {
                                                sprintf(logmsg,"TI=%d| Failed to retrieve peak positions",ti);
                                                os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
                                        }
                                }
				if(outform & GET_TRACE_DATA_COMMENT){
                                        size_t    cm_len=0;
                                        const void *  cm=NULL;
                                        cm=TraceDataGetComment(tdp,&cm_len);
#ifdef _DEBUG
					sprintf(logmsg,"TI=%d|cm_len=%d ",ti,cm_len);
                                        os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
#endif

                                        if( cm && cm_len){
                                                os_output_image_data(spp,"trace_comment_full",TRUE,(CS_BYTE PNTR)cm,cm_len);
                                        }
					cm=TraceDataGetExtended(tdp,&cm_len);
#ifdef _DEBUG
					sprintf(logmsg,"TI=%d|xd_len=%d ",ti,cm_len);
                                        os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
#endif
                                        if( cm && cm_len){
                                                os_output_image_data(spp,"extended_data_full",TRUE,(CS_BYTE PNTR)cm,cm_len);
                                        }
					outform &= ~GET_TRACE_DATA_COMMENT;
                                }
				TraceDataWhack(tdp);
			} 
			if(outform > 0 && tros.trlocs.trloc_data[i_data].dbname && tros.trlocs.trloc_data[i_data].dbname[0]){
				sprintf(sql,"exec %s..usp_get_data %d,%d",tros.trlocs.trloc_data[i_data].dbname,ti,outform);
				if(os_ExecOnBestConnection(spp,tdatap,tros.trlocs.trloc_data[i_data].bsrv,sql,
						os_gateway_lang_func_cb_results_only,CS_FALSE) != CS_SUCCEED){
					return CS_FAIL;
				}
				outform &= ~GET_TRACE_DATA_QSCORES;
				outform &= ~GET_TRACE_DATA_BASECALL;
				outform &= ~GET_TRACE_DATA_PEAK;
				outform &= ~GET_TRACE_DATA_COMMENT;
			}
		}
		MACRO_OUTPUT_INTEGER(all_done,im_src==IMAGE_SOURCE_FILE?0:1);
	}
	if(last_replaced_by < 0){
		switch(last_replaced_by){
		 case -1000:
			sprintf(logmsg,"TIs <%d:%d> are not found",ti_msg_first,ti_msg_last);
			errcode=ERR_CODE_NOTFOUND;
			break;
		 case -100:
		 case -10:
			sprintf(logmsg,"TIs <%d:%d> are withdrawn",ti_msg_first,ti_msg_last);
			errcode=ERR_CODE_WITHDRAWN;
			break;
		 case -1:
			sprintf(logmsg,"TIs <%d:%d> are not completely loaded",ti_msg_first,ti_msg_last);
			errcode=ERR_CODE_NOTLOADED;
			break;
		 default:
			sprintf(logmsg,"TIs <%d:%d> have unexpected status",ti_msg_first,ti_msg_last);
			errcode=ERR_CODE_UNKNOWN;
			break;
		}
		os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,10);
	}




DONE:
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:0);
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,0);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
        os_ErrPost(NULL,spp,"usp_get_trace_data has successfully completed",0,0,CS_FALSE,10);
        return CS_SUCCEED;

}
CS_RETCODE
TraceOS_id_get_asnprop(SRV_PROC PNTR spp)
{
        CS_INT          sat_key;
	CS_SMALLINT	sat;
        CS_DATAFMT      cs_int_fmt = {"@sat_key",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_DATAFMT      cs_smallint_fmt = {"@sat",CS_NULLTERM,CS_SMALLINT_TYPE,CS_FMT_UNUSED,sizeof(CS_SMALLINT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_CHAR         sql[1024];
        UserThreadDataPtr tdatap=NULL;


        if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_int_fmt,(CS_BYTE *)&sat_key,NULL,NULL) != CS_SUCCEED){
		return CS_FAIL;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,3,&cs_smallint_fmt,(CS_BYTE *)&sat,NULL,NULL) != CS_SUCCEED){
		return CS_FAIL;
        }
        if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
		return CS_FAIL;
        }
        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
		return CS_FAIL;
        }
	if(sat==44/*** SRA ***/){
		return TraceOS_sra_get_asnprop(spp,sat_key);
	}

	sprintf(sql,"exec id_get_asnprop %d",sat_key);
	if(tros.trmainconn.is_online){
		return os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,sql,
		   (1/*tdatap->byteorder != tros.osi.byteorder*/)?os_gateway_lang_func_cb:os_passthrough_lang_func_cb,CS_FALSE);
	} else {
		srv_sendstatus(spp,0);
		srv_senddone(spp,SRV_DONE_FINAL,CS_TRAN_UNDEFINED,0);
		return CS_SUCCEED;
	}
}
CS_RETCODE
TraceOS_usp_range_by_submission_id(SRV_PROC PNTR spp)
{
        CS_INT          sid;
        CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_CHAR         sql[1024];
        UserThreadDataPtr tdatap=NULL;


        if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_int_fmt,(CS_BYTE *)&sid,NULL,NULL) != CS_SUCCEED){
		return CS_FAIL;
        }
        if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
		return CS_FAIL;
        }
        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
		return CS_FAIL;
        }

	sprintf(sql,"exec usp_range_by_submission_id %d",sid);
	return os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,sql,
	   (1/*tdatap->byteorder != tros.osi.byteorder*/)?os_gateway_lang_func_cb:os_passthrough_lang_func_cb,CS_FALSE);
}

CS_RETCODE
TraceOS_usp_reset_pending_status(SRV_PROC PNTR spp)
{
        CS_INT          sid;
        CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,65,CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_CHAR         sql[1024];
	CS_CHAR		user_name[65]="";
        UserThreadDataPtr tdatap=NULL;

        if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_int_fmt,(CS_BYTE *)&sid,NULL,NULL) != CS_SUCCEED){
                return CS_FAIL;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_char_fmt,(CS_BYTE *)user_name,NULL,NULL) != CS_SUCCEED){
                return CS_FAIL;
        }
        if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
                return CS_FAIL;
        }
        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
                return CS_FAIL;
        }
        sprintf(sql,"exec STraceSubmission..usp_reset_pending_status @sid=%d,@user_name=\"%s\"",sid,user_name);
        return os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,sql,
           (1/*tdatap->byteorder != tros.osi.byteorder*/)?os_gateway_lang_func_cb:os_passthrough_lang_func_cb,CS_FALSE);
}

CS_RETCODE
TraceOS_usp_sid_show_log(SRV_PROC PNTR spp)
{
        CS_INT          sid;
        CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_CHAR         sql[1024];
        UserThreadDataPtr tdatap=NULL;

        if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_int_fmt,(CS_BYTE *)&sid,NULL,NULL) != CS_SUCCEED){
		return CS_FAIL;
        }
        if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
		return CS_FAIL;
        }
        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
		return CS_FAIL;
        }
	sprintf(sql,"exec STraceSubmission..usp_sid_show_log %d",sid);
	return os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,sql,
	   (1/*tdatap->byteorder != tros.osi.byteorder*/)?os_gateway_lang_func_cb:os_passthrough_lang_func_cb,CS_FALSE);
}

CS_RETCODE
TraceOS_usp_show_reset_pending_status(SRV_PROC PNTR spp)
{
        CS_INT          sid;
        CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,65,CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_CHAR         sql[1024];
	CS_CHAR		user_name[65]="";
        UserThreadDataPtr tdatap=NULL;

        if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_int_fmt,(CS_BYTE *)&sid,NULL,NULL) != CS_SUCCEED){
                return CS_FAIL;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_char_fmt,(CS_BYTE *)user_name,NULL,NULL) != CS_SUCCEED){
                return CS_FAIL;
        }
        if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
                return CS_FAIL;
        }
        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
                return CS_FAIL;
        }
        sprintf(sql,"exec STraceSubmission..usp_show_reset_pending_status @sid=%d,@user_name=\"%s\"",sid,user_name);
        return os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,sql,
           (1/*tdatap->byteorder != tros.osi.byteorder*/)?os_gateway_lang_func_cb:os_passthrough_lang_func_cb,CS_FALSE);
}

CS_RETCODE
TraceOS_usp_tracking(SRV_PROC PNTR spp)
{
        CS_INT          page_size,page_number;
	CS_CHAR		filter[256];
        CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,255,CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_CHAR         sql[1024];
        UserThreadDataPtr tdatap=NULL;


        if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_int_fmt,(CS_BYTE *)&page_size,NULL,NULL) != CS_SUCCEED){
		return CS_FAIL;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_int_fmt,(CS_BYTE *)&page_number,NULL,NULL) != CS_SUCCEED){
		return CS_FAIL;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,3,&cs_char_fmt,(CS_BYTE *)filter,NULL,NULL) != CS_SUCCEED){
		return CS_FAIL;
        }
        if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
		return CS_FAIL;
        }
        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
		return CS_FAIL;
        }

	sprintf(sql,"exec STraceSubmission..usp_tracking %d,%d,\"%s\"",page_size,page_number,filter);
	return os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,sql,
	   (1/*tdatap->byteorder != tros.osi.byteorder*/)?os_gateway_lang_func_cb:os_passthrough_lang_func_cb,CS_FALSE);
}
CS_RETCODE
TraceOS_usp_list_species_pager(SRV_PROC PNTR spp)
{
        CS_INT          page_size,page_number;
	CS_CHAR		letter[256];
        CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,255,CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_CHAR         sql[1024];
        UserThreadDataPtr tdatap=NULL;


        if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_char_fmt,(CS_BYTE *)letter,NULL,NULL) != CS_SUCCEED){
		return CS_FAIL;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_int_fmt,(CS_BYTE *)&page_number,NULL,NULL) != CS_SUCCEED){
		return CS_FAIL;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,3,&cs_int_fmt,(CS_BYTE *)&page_size,NULL,NULL) != CS_SUCCEED){
		return CS_FAIL;
        }
        if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
		return CS_FAIL;
        }
        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
		return CS_FAIL;
        }

	sprintf(sql,"exec TraceMain..usp_list_species_pager \"%s\",%d,%d",letter,page_number,page_size);
	return os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,sql,
	   (1/*tdatap->byteorder != tros.osi.byteorder*/)?os_gateway_lang_func_cb:os_passthrough_lang_func_cb,CS_FALSE);
}
/*************************************************SRA ************************************/

#define MACRO_CHECK4SRAERROR(RCC, ACC, PROC) \
if(RCC != 0) { \
    char mac_buffer[1024], mac_msg[1024]; \
    size_t mac_x = 0; \
	RCExplain(RCC, mac_buffer, sizeof(mac_buffer), &mac_x); \
	snprintf(mac_msg, sizeof(mac_msg), \
             "<ERROR procedure=\"%s\" number=\"%d\" public=\"yes\" msg=\"%.*s\">Failed to get data for %s</ERROR>", \
             #PROC, RCC, (int)mac_x, mac_buffer, ACC); \
	os_ErrPost(NULL, spp, mac_msg, 0, 0, CS_TRUE, 0); \
	status |= SRV_DONE_ERROR; \
	goto DONE; \
}

CS_RETCODE
TraceOS_sra_flush(SRV_PROC PNTR spp)
{
	CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,200,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_INT          status = SRV_DONE_FINAL;
    CS_CHAR         acc[255];
	CS_INT		    acclen=0,rc=0;
    UserThreadDataPtr tdatap=NULL;

    if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_char_fmt,(CS_BYTE *)acc,&acclen,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	acc[acclen]=0;
    srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"SRA_FLUSH",sizeof("SRA_FLUSH"),NULL);

	if(acc[0]=='/'){
		rc=SRAMgrFlushRepPath(tros.sradb,acc);
		MACRO_CHECK4SRAERROR(rc, acc, SRAMgrFlushRepPath);
	} else if(acclen==9 && acc[0]=='S' && acc[1]=='R' && acc[2]=='R'){
		rc=SRAMgrFlushRun(tros.sradb,acc);
		MACRO_CHECK4SRAERROR(rc, acc, SRAMgrFlushRun);
	} else  {
		rc=SRAMgrFlushVolPath(tros.sradb,acc);
		MACRO_CHECK4SRAERROR(rc, acc, SRAMgrFlushVolPath);
	}
	
DONE:	
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:0);
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,0);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
        return CS_SUCCEED;
}

CS_RETCODE
TraceOS_sra_list_references(SRV_PROC PNTR spp)
{
    CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,200,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_DATAFMT      cs_int_fmt  = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,4,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_INT          status = SRV_DONE_FINAL;
    CS_CHAR         acc[255];
    CS_CHAR         rname[255];
    CS_CHAR	    logmsg[2048];
    CS_INT	    acclen=0,rnamelen=0,rc=0;
    CS_INT	    max_seq_len=0;
    CS_INT	    num_bins=0;
    UserThreadDataPtr tdatap=NULL;
    SraUserData	   *sra_data=NULL;
	CS_INT	   i=0, rowcount=0;
	char		*errstr=NULL;
	bool	    need_describe;

	if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
		status |= SRV_DONE_ERROR;
		goto DONE;
	}
	sra_data=tdatap->userdata;
	if(!sra_data){
		tdatap->userdata=sra_data=malloc(sizeof(*sra_data));
		memset(sra_data,0,sizeof(*sra_data));
		tdatap->userdatafree=(void (*)(void*))SraUserDataDelete;
	}

    if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_char_fmt,(CS_BYTE *)acc,&acclen,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_char_fmt,(CS_BYTE *)rname,&rnamelen,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,3,&cs_int_fmt,(CS_BYTE *)&max_seq_len,NULL,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,4,&cs_int_fmt,(CS_BYTE *)&num_bins,NULL,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    acc[acclen]=0;
    rname[rnamelen]=0;
    srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"SRA_LIST_REF",sizeof("SRA_LIST_REF"),NULL);

	rc=SraUserDataSetRunAccession(tros.sradb,sra_data,acc,&errstr);
        if(errstr != 0) os_ErrPost(NULL,spp,errstr,0,0,CS_TRUE,0);
        MACRO_CHECK4SRAERROR(rc, acc, SraUserDataSetRunAccession);

	if(sra_data->vdb == 0){
		sprintf(logmsg,"SRA Run %s does not contain references",acc);
		os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
		status |= SRV_DONE_ERROR;
	} else {
CS_DATAFMT      cs_name_fmt  = {"name", CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
CS_DATAFMT      cs_seqid_fmt = {"seqid",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
CS_DATAFMT      cs_path_fmt  = {"external_path", CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
CS_DATAFMT      cs_len_fmt   = {"length", CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
CS_DATAFMT      cs_id_fmt   = {"id", CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
CS_DATAFMT      cs_bin_fmt   = {"bin", CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
CS_DATAFMT      cs_cir_fmt   = {"is_circular", CS_NULLTERM,CS_TINYINT_TYPE,CS_FMT_UNUSED,sizeof(CS_TINYINT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
CS_DATAFMT      cs_ext_fmt   = {"is_external", CS_NULLTERM,CS_TINYINT_TYPE,CS_FMT_UNUSED,sizeof(CS_TINYINT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
		const ReferenceList *reflist;
		uint32_t    count;
		const char *name;
		const char *seqid;
		INSDC_coord_len len;
		int64_t	    id;
		CS_INT	    length,refid;
		uint32_t    bin;
		bool	    is_circular;
		bool	    is_external;
		char	   *external_path = NULL;
		char	   seqid_buf[1024];
		CS_TINYINT  cir,ext;
		CS_SMALLINT pathind;

		rc=ReferenceList_MakeDatabase(&reflist,sra_data->vdb,0/*options*/,10,(rname[0])?rname:NULL,num_bins);
		if(rc){
			if(rname[0]==0) sprintf(logmsg,"SRA Run %s does not contain references",acc);
			else            sprintf(logmsg,"SRA Run %s does not contain reference %s",acc,rname);
			os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
		rc=ReferenceList_Count(reflist,&count);
		MACRO_CHECK4SRAERROR(rc, acc, ReferenceListCount);





		for(i=0,need_describe = true;i < count; i++){
			const ReferenceObj  *ref;

			if(need_describe){
				if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_id_fmt)!=CS_SUCCEED
				   || srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_id_fmt,(CS_BYTE*)&refid,NULL,NULL)!=CS_SUCCEED
				   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,2,&cs_bin_fmt)!=CS_SUCCEED
				   || srv_bind(spp,CS_SET,SRV_ROWDATA,2,&cs_bin_fmt,(CS_BYTE*)&bin,NULL,NULL)!=CS_SUCCEED
				   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,3,&cs_name_fmt)!=CS_SUCCEED
				   || srv_bind(spp,CS_SET,SRV_ROWDATA,3,&cs_name_fmt,(CS_BYTE*)name,NULL,NULL)!=CS_SUCCEED
				   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,4,&cs_seqid_fmt)!=CS_SUCCEED
				   || srv_bind(spp,CS_SET,SRV_ROWDATA,4,&cs_seqid_fmt,(CS_BYTE*)seqid_buf,NULL,NULL)!=CS_SUCCEED
				   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,5,&cs_len_fmt)!=CS_SUCCEED
				   || srv_bind(spp,CS_SET,SRV_ROWDATA,5,&cs_len_fmt,(CS_BYTE*)&length,NULL,NULL)!=CS_SUCCEED
				   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,6,&cs_cir_fmt)!=CS_SUCCEED
				   || srv_bind(spp,CS_SET,SRV_ROWDATA,6,&cs_cir_fmt,(CS_BYTE*)&cir,NULL,NULL)!=CS_SUCCEED
				   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,7,&cs_ext_fmt)!=CS_SUCCEED
				   || srv_bind(spp,CS_SET,SRV_ROWDATA,7,&cs_ext_fmt,(CS_BYTE*)&ext,NULL,NULL)!=CS_SUCCEED
				   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,8,&cs_path_fmt)!=CS_SUCCEED
				   || srv_bind(spp,CS_SET,SRV_ROWDATA,8,&cs_path_fmt,(CS_BYTE*)external_path,NULL,&pathind)!=CS_SUCCEED
				   ){
					status |= SRV_DONE_ERROR;
					goto DONE;
				}
				need_describe = false;
			}
				
			rc=ReferenceList_Get(reflist,&ref,i);
			MACRO_CHECK4SRAERROR(rc, acc, ReferenceListCount);
			rc=ReferenceObj_IdRange(ref,&id,NULL);
			MACRO_CHECK4SRAERROR(rc, acc, ReferenceIdRange);
			rc=ReferenceObj_Bin(ref,&bin);
			MACRO_CHECK4SRAERROR(rc, acc, ReferenceBin);
			rc=ReferenceObj_Name(ref,&name);
			MACRO_CHECK4SRAERROR(rc, acc, ReferenceName);
			rc=ReferenceObj_SeqId(ref,&seqid);
			MACRO_CHECK4SRAERROR(rc, acc, ReferenceSeqId);
			rc=ReferenceObj_SeqLength(ref,&len);
			MACRO_CHECK4SRAERROR(rc, acc, ReferenceSeqLength);
			rc=ReferenceObj_Circular(ref,&is_circular);
			MACRO_CHECK4SRAERROR(rc, acc, ReferenceCircular);
			rc=ReferenceObj_External(ref, &is_external, NULL);
			MACRO_CHECK4SRAERROR(rc, acc, ReferenceExternal);

			ext=is_external;
			cir=is_circular;
			if(is_external && acc[0] != '/'){
				strcpy(seqid_buf,seqid);
			} else {
				sprintf(seqid_buf,"gnl|SRA|%s/%s",acc,name);
			}
            pathind = CS_NULLDATA;
#if 0 /* always null now */
			if(is_external){
				pathind = CS_GOODDATA;
			}
#endif
			length = (CS_INT)len;
			refid  = (CS_INT)id;
			/* sprintf(logmsg,"%s\t%s\t%d\t%d\t%d\t%s",name,seqid,(int)len,is_circular,is_external,is_external?external_path:"");*/
			/* os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);*/
			 if(   srv_bind(spp,CS_SET,SRV_ROWDATA,3,&cs_name_fmt,(CS_BYTE*)name,NULL,NULL)!=CS_SUCCEED
			    || srv_bind(spp,CS_SET,SRV_ROWDATA,8,&cs_path_fmt,(CS_BYTE*)external_path,NULL,&pathind)!=CS_SUCCEED
			   ){
				status |= SRV_DONE_ERROR;
				goto DONE;
			}
			if(srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED){
				status |= SRV_DONE_ERROR;
				goto DONE;
			}
			rowcount++;
			if(max_seq_len != 0 ) { /*** only in this case output the sequence ***/
				int64_t offset = 0;
				int32_t maxlen = max_seq_len;
				if(maxlen < 0) maxlen = length;
				srv_senddone(spp,SRV_DONE_MORE| SRV_DONE_COUNT,CS_TRAN_UNDEFINED,rowcount);
				rowcount=0;
				while(offset < maxlen){
					INSDC_coord_len bytes,buflen;
					uint8_t	buf[100000];
					buflen=sizeof(buf);
					if(buflen > maxlen - offset){
						buflen = maxlen - offset;
					}
					rc = ReferenceObj_Read(ref, offset,buflen,buf,&bytes);
					if(rc) break;
					if(os_output_image_data(spp,"sequence",CS_TRUE,buf,bytes)!= CS_SUCCEED) break;
					offset += bytes;
					rowcount++;
				}
				srv_senddone(spp,SRV_DONE_MORE| SRV_DONE_COUNT,CS_TRAN_UNDEFINED,rowcount);
				rowcount = 0;
				need_describe=true;
			}
			ReferenceObj_Release(ref);
		}
		ReferenceList_Release(reflist);
	} 
	
DONE:	
	if(!need_describe) srv_senddone(spp,SRV_DONE_MORE| SRV_DONE_COUNT,CS_TRAN_UNDEFINED,rowcount);
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:0);
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,0);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
	SraUserDataReset(sra_data);
        return CS_SUCCEED;
}
CS_RETCODE
TraceOS_sra_get_run_data(SRV_PROC PNTR spp)
{
	CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_INT          status = SRV_DONE_FINAL;
    CS_CHAR         logmsg[2048],acc[255];
	CS_INT		acclen=0,rc=0,rowcount=0;
    UserThreadDataPtr tdatap=NULL;
	CS_INT		dsize;
	CS_INT		spot_first,spot_last,spot_cnt;
	SraUserData	*sra_data=NULL;
	int		format_mask,read_mask,do_clip;
	char		*errstr=NULL;


    if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	sra_data=tdatap->userdata;
	if(!sra_data){
		tdatap->userdata=sra_data=malloc(sizeof(*sra_data));
		memset(sra_data,0,sizeof(*sra_data));
		tdatap->userdatafree=(void (*)(void*))SraUserDataDelete;
	}

    if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_char_fmt,(CS_BYTE *)acc,&acclen,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_int_fmt,(CS_BYTE *)&spot_first,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,3,&cs_int_fmt,(CS_BYTE *)&spot_last,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,4,&cs_int_fmt,(CS_BYTE *)&spot_cnt,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,5,&cs_int_fmt,(CS_BYTE *)&format_mask,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,6,&cs_int_fmt,(CS_BYTE *)&read_mask,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,7,&cs_int_fmt,(CS_BYTE *)&do_clip,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
	}
    if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	acc[acclen]=0;
    srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"SRA_DATA",sizeof("SRA_DATA"),NULL);
	if(spot_first <=0) spot_first=1;
	if(spot_last <=0) spot_last=0x7fffffff;
	if(spot_cnt <=0) spot_cnt=0x7fffffff;

	rc=SraUserDataSetRunAccession(tros.sradb,sra_data,acc,&errstr);
	if(errstr != 0) os_ErrPost(NULL,spp,errstr,0,0,CS_TRUE,0);
	MACRO_CHECK4SRAERROR(rc, acc, SraUserDataSetRunAccession);
	
	if(format_mask&SRA_OUTPUT_META_BIT) {
		if(SraUserSendMetadata(spp)!=CS_SUCCEED){
			sprintf(logmsg,"SRA_RUN|%s|SraUserSendMetadata failed",acc);
			os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
		if(format_mask==SRA_OUTPUT_META_BIT) goto DONE;
	}

        rowcount=0;

	if(SraUserSpotDataPrepare(spp,sra_data,format_mask,read_mask)!=CS_SUCCEED){
		status |= SRV_DONE_ERROR;
		goto DONE;
	}
	if(spot_first > sra_data->run_spot_last){
		sprintf(logmsg,"SRA_RUN|%s|Spot_first %d is outside the range %d:%d",acc,spot_first,sra_data->run_spot_first,sra_data->run_spot_last);
                os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
                status |= SRV_DONE_ERROR;
                goto DONE;
	}
	if(spot_last > sra_data->run_spot_last){
		spot_last=sra_data->run_spot_last;
	}
	for(rowcount=0;spot_first <= spot_last && spot_cnt > 0;spot_first++,spot_cnt--,rowcount++){
		rc=SraUserDataOpen(sra_data,spot_first,format_mask|(do_clip?SRA_OUTPUT_CLIP_QUALITY_RIGHT_BIT:0),read_mask);
		MACRO_CHECK4SRAERROR(rc, acc, SraUserDataOpen);

		if(SraUserSpotDataSend(spp,sra_data,format_mask,read_mask,do_clip)!=CS_SUCCEED){
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
	}
	srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,rowcount);
	
DONE:
	
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:0);
	
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,rowcount);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
	SraUserDataReset(sra_data);
        return CS_SUCCEED;

}

CS_RETCODE
TraceOS_sra_find_spot(SRV_PROC PNTR spp)
{
	CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_INT          status = SRV_DONE_FINAL;
    CS_CHAR         logmsg[2048],acc[255];
	CS_INT		acclen=0,rc=0,rowcount=0;
    UserThreadDataPtr tdatap=NULL;
	CS_INT		dsize,range_x=0,range_y=0;
	SraUserData	*sra_data=NULL;
	CS_INT		format_mask,read_mask,do_clip,pagesize,pageno,rescount=0;
	spotid_t	spot_first,spot_last;
	uint32_t	x,y,tile;
	char		*errstr=NULL;
	CS_CHAR		spotname[200];
	CS_INT		spotnamelen;


    if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	sra_data=tdatap->userdata;
	if(!sra_data){
		tdatap->userdata=sra_data=malloc(sizeof(*sra_data));
		memset(sra_data,0,sizeof(*sra_data));
		tdatap->userdatafree=(void (*)(void*))SraUserDataDelete;
	}

    if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_char_fmt,(CS_BYTE *)acc,&acclen,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_char_fmt,(CS_BYTE *)spotname,&spotnamelen,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,3,&cs_int_fmt,(CS_BYTE *)&range_x,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,4,&cs_int_fmt,(CS_BYTE *)&range_y,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,5,&cs_int_fmt,(CS_BYTE *)&format_mask,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,6,&cs_int_fmt,(CS_BYTE *)&read_mask,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,7,&cs_int_fmt,(CS_BYTE *)&do_clip,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,8,&cs_int_fmt,(CS_BYTE *)&pagesize,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
	}
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,9,&cs_int_fmt,(CS_BYTE *)&pageno,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
	}
    if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	acc[acclen]=0;
	spotname[spotnamelen]=0;
    srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"SRA_FIND_SPOT",sizeof("SRA_FIND_SPOT"),NULL);
	
	rc=SraUserDataSetRunAccession(tros.sradb,sra_data,acc,&errstr);
	if(errstr != 0) os_ErrPost(NULL,spp,errstr,0,0,CS_TRUE,0);
	MACRO_CHECK4SRAERROR(rc, acc, SraUserDataSetRunAccession);

	if(format_mask&SRA_OUTPUT_META_BIT) {
		if(SraUserSendMetadata(spp)!=CS_SUCCEED){
			sprintf(logmsg,"SRA_FIND|%s|SraUserSendMetadata failed",acc);
			os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
		if(format_mask==SRA_OUTPUT_META_BIT) goto DONE;
	}

	if(SraUserSpotDataPrepare(spp,sra_data,format_mask,read_mask)!=CS_SUCCEED){
		status |= SRV_DONE_ERROR;
		goto DONE;
	}
	rc=SRATableGetSpotId(sra_data->run,&sra_data->spotid,spotname);
	if(rc) goto DONE;
	/**** spotid is valid here, starting output ***/
	rc=SraUserDataOpen(sra_data,sra_data->spotid,format_mask|(do_clip?SRA_OUTPUT_CLIP_QUALITY_RIGHT_BIT:0),read_mask);
	MACRO_CHECK4SRAERROR(rc, acc, SraUserDataOpen);
	spot_first=spot_last=sra_data->spotid; /*** disabling search by range ***/
	rescount++;
	if(rescount > (pageno-1)*pagesize && rescount <= pageno*pagesize){
		if(SraUserSpotDataSend(spp,sra_data,format_mask,read_mask,do_clip)!=CS_SUCCEED){
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
		rowcount++;
	}
	if(range_x > 0 || range_y > 0){
		spotid_t	self=sra_data->spotid;
		x=sra_data->coord.x;
		y=sra_data->coord.y;
		tile=sra_data->coord.tile;
		for(;spot_first <= spot_last;spot_first++){
			if(spot_first==self) continue;
			rc=SraUserDataOpen(sra_data,spot_first,SRA_OUTPUT_NAME_BIT,0);
			MACRO_CHECK4SRAERROR(rc, acc, SraUserDataOpen(NAME));
			if(    tile == sra_data->coord.tile
			   &&  x + range_x >= sra_data->coord.x && x  <= sra_data->coord.x + range_x 
			   &&  y + range_y >= sra_data->coord.y && y  <= sra_data->coord.y + range_y ){
				rescount++;
				if(rescount > (pageno-1)*pagesize && rescount <= pageno*pagesize){
					rc=SraUserDataOpen(sra_data,spot_first,format_mask|(do_clip?SRA_OUTPUT_CLIP_QUALITY_RIGHT_BIT:0),read_mask);
					MACRO_CHECK4SRAERROR(rc, acc, SraUserDataOpen(MASK));
					if(SraUserSpotDataSend(spp,sra_data,format_mask,read_mask,do_clip)!=CS_SUCCEED){
						status |= SRV_DONE_ERROR;
						goto DONE;
					}
					rowcount++;
				}
			}
		}
	}
	srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,rowcount);
	
DONE:
	
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:rescount);
	
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,rowcount);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
	SraUserDataReset(sra_data);
        return CS_SUCCEED;

}
CS_RETCODE
TraceOS_sra_find_seq(SRV_PROC PNTR spp)
{
	CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_INT          status = SRV_DONE_FINAL;
    CS_CHAR         logmsg[2048],acc[256],seq_in[256],seq_buf[256];
	CS_INT		acclen=0,rc=0,rowcount=0;
    UserThreadDataPtr tdatap=NULL;
	unsigned int	i;
	CS_INT		dsize,seqlen;
	CS_INT		pagesize,pageno,rescount=0;
	SraUserData	*sra_data=NULL;
	CS_INT		format_mask,read_mask,do_clip,read_mask_search;
	size_t		spotid;
	char		*errstr=NULL;
	uint32_t	clip_quality_right;
	uint32_t	*len;
	NucStrstr	*q_str=NULL;
	char		*seq;
	char		*read_type;
	char		*read_name;
	SraColumnData   *rd_src;


    if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	sra_data=tdatap->userdata;
	tdatap->got_attention=0;
	if(!sra_data){
		tdatap->userdata=sra_data=malloc(sizeof(*sra_data));
		memset(sra_data,0,sizeof(*sra_data));
		tdatap->userdatafree=(void (*)(void*))SraUserDataDelete;
	}
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_char_fmt,(CS_BYTE *)acc,&acclen,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_char_fmt,(CS_BYTE *)seq_in,&seqlen,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,3,&cs_int_fmt,(CS_BYTE *)&format_mask,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,4,&cs_int_fmt,(CS_BYTE *)&read_mask,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,5,&cs_int_fmt,(CS_BYTE *)&do_clip,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,6,&cs_int_fmt,(CS_BYTE *)&pagesize,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
	}
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,7,&cs_int_fmt,(CS_BYTE *)&pageno,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
	}
	if (srv_bind(spp,CS_GET,SRV_RPCDATA,8,&cs_int_fmt,(CS_BYTE *)&read_mask_search,&dsize,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	
	acc[acclen]=0;
	seq_in[seqlen]=0;
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"SRA_FIND",sizeof("SRA_FIND"),NULL);

	rc=SraUserDataSetRunAccession(tros.sradb,sra_data,acc,&errstr);
	if(errstr != 0) os_ErrPost(NULL,spp,errstr,0,0,CS_TRUE,0);
	MACRO_CHECK4SRAERROR(rc, acc, SraUserDataSetRunAccession);

	if(read_mask_search==0x7fffffff){
		read_mask_search = read_mask & sra_data->read_mask_bio;
	}
	
	if(format_mask&SRA_OUTPUT_META_BIT) {
		if(SraUserSendMetadata(spp)!=CS_SUCCEED){
			sprintf(logmsg,"SRA_FIND|%s|SraUserSendMetadata failed",acc);
			os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
		if(format_mask==SRA_OUTPUT_META_BIT) goto DONE;
	}

	if(SraUserSpotDataPrepare(spp,sra_data,format_mask,read_mask)!=CS_SUCCEED){
		status |= SRV_DONE_ERROR;
		goto DONE;
	}
	if(sra_data->platform==SRA_PLATFORM_ABSOLID){
		static const uint8_t cm[5][5] = {
			{ 0, 1, 2, 3, 4 },
			{ 1, 0, 3, 2, 4 },
			{ 2, 3, 0, 1, 4 },
			{ 3, 2, 1, 0, 4 },
			{ 4, 4, 4, 4, 4 },
		};
		uint8_t	prev,cur;
		int i;

		read_type="INSDC:2cs:packed";
		read_name="CSREAD";
		rd_src=&sra_data->rd_cs;
		switch(seq_in[0]){
		 case 'A': prev=0; break;
		 case 'C': prev=1; break;
		 case 'G': prev=2; break;
		 case 'T': prev=3; break;
		 default:  prev=4; break;
		}
		for(i=0,--seqlen;i<seqlen;++i){
			switch(seq_in[i+1]){
			 case 'A': cur=0; break;
			 case 'C': cur=1; break;
			 case 'G': cur=2; break;
			 case 'T': cur=3; break;
			 default:  cur=4; break;
			}
			seq_buf[i]=  "ACGTN"[cm[prev][cur]];
			prev=cur;
		}
		seq=seq_buf;
	} else {
		seq=seq_in;
		read_type="INSDC:2na:packed";
		read_name="READ";
		rd_src=&sra_data->rd;
	}
	rc=NucStrstrMake(&q_str,0,seq,seqlen);
	if(rc != 0){
		os_ErrPost(NULL,spp,"Invalid Query Sequence",15,15,CS_TRUE,0);
		goto DONE;
	}
  	for(rowcount=0,spotid=sra_data->run_spot_first;spotid<=sra_data->run_spot_last && !tdatap->got_attention;spotid++){
		int pos=0,len_b,offset_b;
		rc=SraColumnDataOpen(rd_src,sra_data->run,read_name,read_type,spotid);
                MACRO_CHECK4SRAERROR(rc, acc, SraColumnDataOpen_READ2NA);
		if(rd_src->not_present){
			os_ErrPost(NULL,spp,"Cannot open needed read format",15,15,CS_TRUE,0);
                	goto DONE;
		}
		if(do_clip && sra_data->cqr_present){ 
			rc=SraUserDataOpen(sra_data,spotid,SRA_OUTPUT_CLIP_QUALITY_RIGHT_BIT,0);
                	MACRO_CHECK4SRAERROR(rc, acc, SraUserDataOpen);
			clip_quality_right=*(uint32_t*)sra_data->cqr.data;
		} else {
			clip_quality_right=0;
		}
		offset_b=rd_src->bitoffset/2;
		len_b=rd_src->bitlen/2;
		if(clip_quality_right > 0 && len_b > clip_quality_right) len_b=clip_quality_right;
		
		if(read_mask_search ==0){
			pos=NucStrstrSearch(q_str,rd_src->data,offset_b,len_b,NULL);
		} else {
			rc=SraColumnDataOpen(&sra_data->len,sra_data->run,"READ_LEN",vdb_uint32_t,spotid);
			MACRO_CHECK4SRAERROR(rc, acc, SraColumnDataOpen_READ_LEN);
			len=(uint32_t*)sra_data->len.data;
			for(i=0;i<sra_data->num_reads && pos==0 && len_b > 0;i++){
				if(read_mask_search & (1<<i)){
					pos=NucStrstrSearch(q_str,rd_src->data,offset_b,(len[i]>len_b)?len_b:len[i],NULL);
				}
				offset_b+=len[i];
				len_b-=len[i];
			}
		}
		if(pos>0){
			rescount++;
			if(rescount > (pageno-1)*pagesize && rescount <= pageno*pagesize){
				rc=SraUserDataOpen(sra_data,spotid,format_mask,read_mask);
				MACRO_CHECK4SRAERROR(rc, acc, SraUserDataOpen);
				if(SraUserSpotDataSend(spp,sra_data,format_mask,read_mask,do_clip)!=CS_SUCCEED){
					status |= SRV_DONE_ERROR;
					goto DONE;
				}
				rowcount++;
			}
		}
        }
	srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,rowcount);
	
DONE:
	if(q_str) NucStrstrWhack(q_str);
	
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:rescount);
	
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,rowcount);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
	SraUserDataReset(sra_data);
        return CS_SUCCEED;

}
CS_RETCODE
TraceOS_sra_find_group(SRV_PROC PNTR spp)
{
	CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_INT          status = SRV_DONE_FINAL;
	CS_CHAR         logmsg[2048],acc[256],group[256];
	CS_INT		acclen=0,rc=0,rowcount=0;
	UserThreadDataPtr tdatap=NULL;
	CS_INT		dsize,grouplen;
	CS_INT		pagesize,pageno,rescount=0;
	SraUserData	*sra_data=NULL;
	CS_INT		format_mask,read_mask,do_clip;
	size_t		spotid;
	char		*errstr=NULL;


	if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
	    status |= SRV_DONE_ERROR;
	    goto DONE;
	}
	sra_data=tdatap->userdata;
	tdatap->got_attention=0;
	if(!sra_data){
		tdatap->userdata=sra_data=malloc(sizeof(*sra_data));
		memset(sra_data,0,sizeof(*sra_data));
		tdatap->userdatafree=(void (*)(void*))SraUserDataDelete;
	}
	if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_char_fmt,(CS_BYTE *)acc,&acclen,NULL) != CS_SUCCEED){
	    status |= SRV_DONE_ERROR;
	    goto DONE;
	}
	if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_char_fmt,(CS_BYTE *)group,&grouplen,NULL) != CS_SUCCEED){
	    status |= SRV_DONE_ERROR;
	    goto DONE;
	}
	if (srv_bind(spp,CS_GET,SRV_RPCDATA,3,&cs_int_fmt,(CS_BYTE *)&format_mask,&dsize,NULL) != CS_SUCCEED){
	    status |= SRV_DONE_ERROR;
	    goto DONE;
	}
	if (srv_bind(spp,CS_GET,SRV_RPCDATA,4,&cs_int_fmt,(CS_BYTE *)&read_mask,&dsize,NULL) != CS_SUCCEED){
	    status |= SRV_DONE_ERROR;
	    goto DONE;
	}
	if (srv_bind(spp,CS_GET,SRV_RPCDATA,5,&cs_int_fmt,(CS_BYTE *)&do_clip,&dsize,NULL) != CS_SUCCEED){
	    status |= SRV_DONE_ERROR;
	    goto DONE;
	}
	if (srv_bind(spp,CS_GET,SRV_RPCDATA,6,&cs_int_fmt,(CS_BYTE *)&pagesize,&dsize,NULL) != CS_SUCCEED){
	    status |= SRV_DONE_ERROR;
	    goto DONE;
	}
	if (srv_bind(spp,CS_GET,SRV_RPCDATA,7,&cs_int_fmt,(CS_BYTE *)&pageno,&dsize,NULL) != CS_SUCCEED){
	    status |= SRV_DONE_ERROR;
	    goto DONE;
	}
	if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
	    status |= SRV_DONE_ERROR;
	    goto DONE;
	}
	
	acc[acclen]=0;
	group[grouplen]=0;
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"SRA_FIND",sizeof("SRA_FIND"),NULL);

	rc=SraUserDataSetRunAccession(tros.sradb,sra_data,acc,&errstr);
	if(errstr != 0) os_ErrPost(NULL,spp,errstr,0,0,CS_TRUE,0);
	MACRO_CHECK4SRAERROR(rc, acc, SraUserDataSetRunAccession);

	
	if(format_mask&SRA_OUTPUT_META_BIT) {
		if(SraUserSendMetadata(spp)!=CS_SUCCEED){
			sprintf(logmsg,"SRA_FIND|%s|SraUserSendMetadata failed",acc);
			os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
		if(format_mask==SRA_OUTPUT_META_BIT) goto DONE;
	}
	if(!sra_data->spot_group_present) goto DONE;

	if(SraUserSpotDataPrepare(spp,sra_data,format_mask,read_mask)!=CS_SUCCEED){
		status |= SRV_DONE_ERROR;
		goto DONE;
	}
  	for(rowcount=0,spotid=sra_data->run_spot_first;spotid<=sra_data->run_spot_last && !tdatap->got_attention;spotid++){
		rc=SraColumnDataOpen(&sra_data->sgrp,sra_data->run,"SPOT_GROUP",vdb_ascii_t,spotid);
		MACRO_CHECK4SRAERROR(rc, acc, SraColumnDataOpen);
		{/* SPOT_GROUP may be stored with extra '\0' at the end */
			int len=strnlen((const char*)sra_data->sgrp.data,sra_data->sgrp.len);
			sra_data->sgrp.len=len;
		}
		if(grouplen==sra_data->sgrp.len && !strncasecmp((char*)sra_data->sgrp.data,group,grouplen)){
			rescount++;
			if(rescount > (pageno-1)*pagesize && rescount <= pageno*pagesize){
				rc=SraUserDataOpen(sra_data,spotid,format_mask,read_mask);
				MACRO_CHECK4SRAERROR(rc, acc, SraUserDataOpen);
				if(SraUserSpotDataSend(spp,sra_data,format_mask,read_mask,do_clip)!=CS_SUCCEED){
					status |= SRV_DONE_ERROR;
					goto DONE;
				}
				rowcount++;
			}
		}
        }
	srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,rowcount);
	
DONE:
	
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:rescount);
	
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,rowcount);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
	SraUserDataReset(sra_data);
        return CS_SUCCEED;

}
CS_RETCODE
TraceOS_sra_get_signal(SRV_PROC PNTR spp)
{
	CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_INT          status = SRV_DONE_FINAL;
    CS_CHAR         logmsg[2048],acc[255];
	CS_INT		acclen=0,rc=0,rowcount=0;
    UserThreadDataPtr tdatap=NULL;
	CS_INT		spotid,dummy;
	SraUserData	*sra_data=NULL;
	char		*errstr=NULL;
	int		format_mask=SRA_OUTPUT_QUALITY_BIT|SRA_OUTPUT_QUALITY4_BIT|SRA_OUTPUT_POSITION_BIT|SRA_OUTPUT_INTENSITY_BIT|SRA_OUTPUT_NOISE_BIT|SRA_OUTPUT_SIGNAL_BIT;


    if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	sra_data=tdatap->userdata;
	if(!sra_data){
		tdatap->userdata=sra_data=malloc(sizeof(*sra_data));
		memset(sra_data,0,sizeof(*sra_data));
		tdatap->userdatafree=(void (*)(void*))SraUserDataDelete;
	}

    if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_char_fmt,(CS_BYTE *)acc,&acclen,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_int_fmt,(CS_BYTE *)&spotid,&dummy,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	acc[acclen]=0;
    srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"SRA_SIGNAL",sizeof("SRA_SIGNAL"),NULL);
	
	rc=SraUserDataSetRunAccession(tros.sradb,sra_data,acc,&errstr);
	if(errstr != 0) os_ErrPost(NULL,spp,errstr,0,0,CS_TRUE,0);
	MACRO_CHECK4SRAERROR(rc, acc, SraUserDataSetRunAccession);
	if(sra_data->platform==SRA_PLATFORM_ABSOLID){
		format_mask|=SRA_OUTPUT_COLOR_SPACE_BIT;
	} else {
		format_mask|=SRA_OUTPUT_BASECALL_BIT;
	}
	
    rowcount=0;

	if(spotid > sra_data->run_spot_last || spotid < sra_data->run_spot_first){
		sprintf(logmsg,"SRA_RUN|%s|Spot_first %d is outside the range %d:%d",acc,spotid,sra_data->run_spot_first,sra_data->run_spot_last);
                os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
                status |= SRV_DONE_ERROR;
                goto DONE;
	}
	rc=SraUserDataOpen(sra_data,spotid,format_mask,0);
	MACRO_CHECK4SRAERROR(rc, acc, SraUserDataOpen);

	if(SraUserSignalSend(spp,sra_data)!=CS_SUCCEED){
		status |= SRV_DONE_ERROR;
                goto DONE;
	}
DONE:
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:0);
	
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,rowcount);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
	SraUserDataReset(sra_data);
        return CS_SUCCEED;

}

CS_RETCODE
TraceOS_sra_get_run_files(SRV_PROC PNTR spp)
{
    CS_DATAFMT cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_DATAFMT cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};

    CS_CHAR acc[255];
    CS_INT dsize = 0, acclen = 0, signal = 0, names = 0, paths = 0;
    CS_INT status = SRV_DONE_FINAL;
    SRAPath	*srapath;

    UserThreadDataPtr tdatap = NULL;

    rc_t rc = 0;
    char server[2048], rpath[2048];
    SraFiles sra_files[1000];
    int nfiles = sizeof(sra_files) / sizeof(SraFiles);

    if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL) {
        status |= SRV_DONE_ERROR;
        goto DONE;
    }

    if(srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_char_fmt,(CS_BYTE *)acc,&acclen,NULL) != CS_SUCCEED) {
        status |= SRV_DONE_ERROR;
        goto DONE;
    }
    if(srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_int_fmt,(CS_BYTE *)&signal,&dsize,NULL) != CS_SUCCEED) {
        status |= SRV_DONE_ERROR;
        goto DONE;
    }
    if(srv_bind(spp,CS_GET,SRV_RPCDATA,3,&cs_int_fmt,(CS_BYTE *)&names,&dsize,NULL) != CS_SUCCEED) {
        status |= SRV_DONE_ERROR;
        goto DONE;
    }
    if(srv_bind(spp,CS_GET,SRV_RPCDATA,4,&cs_int_fmt,(CS_BYTE *)&paths,&dsize,NULL) != CS_SUCCEED) {
        status |= SRV_DONE_ERROR;
        goto DONE;
    }
    if(srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED) {
        status |= SRV_DONE_ERROR;
        goto DONE;
    }
    acc[acclen] = 0;
    srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"SRA_DATA",sizeof("SRA_DATA"),NULL);

    rc=SRAMgrGetSRAPath ( tros.sradb, &srapath );
    if( rc == 0){
	rc = SRAPathFind(srapath, acc,  rpath, sizeof(rpath));
	if( rc == 0 ) {
		rc = SraGetFileList(server, rpath, 0, sra_files, &nfiles, signal == 1, names == 1);
	}
    }

    {{
        CS_DATAFMT cs_accession = {"accession",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
        CS_DATAFMT cs_status    = {"status",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
        CS_DATAFMT cs_path      = {"path",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT cs_repserver = {"nfs",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
        CS_DATAFMT cs_ftppfx    = {"ftp",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
        CS_DATAFMT cs_asperapfx = {"aspera",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
        CS_SMALLINT x = CS_GOODDATA;
        CS_INT server_len = strlen(server), rpath_len = strlen(rpath);
        char msg[256] = "Ok";
        int msg_len = 2;
        int outcol = 1;

        if( rc != 0 ) {
		size_t ml;
            const char* const label = "Error: ";
            const int label_len = sizeof(label) - 1;
            strcpy(msg, label);
            RCExplain(rc, &msg[label_len], sizeof(msg) - label_len, &ml);
            msg_len = ml+label_len;
            os_ErrPost(NULL, spp, msg, 0, 0, CS_TRUE, 0);
        }

        /* first row set */

	    if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol,&cs_accession) != CS_SUCCEED ||
            srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_accession,(CS_BYTE*)acc,&acclen,&x)!=CS_SUCCEED) {
            status |= SRV_DONE_ERROR;
            goto DONE;
        }
        outcol++;
	    if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol,&cs_status) != CS_SUCCEED ||
            srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_status,(CS_BYTE*)msg,&msg_len,&x)!=CS_SUCCEED) {
            status |= SRV_DONE_ERROR;
            goto DONE;
        }
        outcol++;
        if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol,&cs_path) != CS_SUCCEED ||
            srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_path,(CS_BYTE*)rpath,&rpath_len,&x)!=CS_SUCCEED) {
            status |= SRV_DONE_ERROR;
            goto DONE;
        }
        if( paths != 1 ) {
            outcol++;
	        if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol,&cs_repserver) != CS_SUCCEED ||
                srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_repserver,(CS_BYTE*)server,&server_len,&x)!=CS_SUCCEED) {
                status |= SRV_DONE_ERROR;
                goto DONE;
            }
        }
        if( paths != 2 ) {
            outcol++;
	        if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol,&cs_ftppfx) != CS_SUCCEED ||
                srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_ftppfx,(CS_BYTE*)tros.ftp_prefix,&tros.ftp_prefix_len,&x)!=CS_SUCCEED) {
                status |= SRV_DONE_ERROR;
                goto DONE;
            }
            outcol++;
	        if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol,&cs_asperapfx) != CS_SUCCEED ||
                srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_asperapfx,(CS_BYTE*)tros.aspera_prefix,&tros.aspera_prefix_len,&x)!=CS_SUCCEED) {
                status |= SRV_DONE_ERROR;
                goto DONE;
            }
        }
        MACRO_SRV_XFERDATA(CS_SET, SRV_ROWDATA);
    }}
    dsize = 1;
    srv_senddone(spp, SRV_DONE_MORE | SRV_DONE_COUNT, CS_TRAN_UNDEFINED, dsize);

    if( rc == 0 ) {
        /* 2nd rowset with files */
        CS_DATAFMT cs_id        = {"id",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT cs_parent    = {"parent",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT cs_type      = {"type",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,4,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
        CS_DATAFMT cs_name      = {"name",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
        CS_DATAFMT cs_timestamp = {"timestamp",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT cs_size      = {"size",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_SMALLINT x = CS_GOODDATA;

        int outcol = 1;
        CS_INT zz = 0;
        if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_id) != CS_SUCCEED ||
            srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_parent) != CS_SUCCEED ||
            srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_type) != CS_SUCCEED ||
            srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_name) != CS_SUCCEED ||
            srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_timestamp) != CS_SUCCEED ||
            srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_size) != CS_SUCCEED /*||
            srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_md5) != CS_SUCCEED*/ ) {
            status |= SRV_DONE_ERROR;
            goto DONE;
        }
        dsize = sizeof(sra_files) / sizeof(SraFiles) - nfiles;
        /*fprintf(stderr, "# files: '%d'\n", dsize);*/
        for(; nfiles < sizeof(sra_files) / sizeof(SraFiles); nfiles++) {
		int name_len,type_len;
            /*fprintf(stderr, "O: '%s'\n", sra_files[nfiles].name);*/

		outcol = 1;
		name_len = strlen(sra_files[nfiles].name);
		type_len = sra_files[nfiles].type == kptDir ? 3 : 4;
	        if( srv_bind(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_id,(CS_BYTE*)&sra_files[nfiles].id,&zz,&x) != CS_SUCCEED ||
                srv_bind(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_parent,(CS_BYTE*)&sra_files[nfiles].parent,&zz,&x) != CS_SUCCEED ||
                srv_bind(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_type,(CS_BYTE*)(sra_files[nfiles].type == kptDir ? "dir" : "file"),&type_len,&x) != CS_SUCCEED ||
                srv_bind(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_name,(CS_BYTE*)&sra_files[nfiles].name,&name_len,&x) != CS_SUCCEED ||
                srv_bind(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_timestamp,(CS_BYTE*)&sra_files[nfiles].mtime,&zz,&x) != CS_SUCCEED ||
                srv_bind(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_size,(CS_BYTE*)&sra_files[nfiles].size,&zz,&x) != CS_SUCCEED /*||
                srv_bind(spp,CS_SET,SRV_ROWDATA,outcol++,&cs_md5,NULL,NULL,NULL) != CS_SUCCEED */) {
                status |= SRV_DONE_ERROR;
                goto DONE;
            }
            MACRO_SRV_XFERDATA(CS_SET, SRV_ROWDATA);
        }
        srv_senddone(spp, SRV_DONE_MORE | SRV_DONE_COUNT, CS_TRAN_UNDEFINED, dsize);
    }

DONE:

    srv_sendstatus(spp, ((status & SRV_DONE_ERROR) > 0) ? -6 : 0);
    srv_senddone(spp, status, CS_TRAN_UNDEFINED, dsize);
    srv_thread_props(spp, CS_SET, SRV_T_USTATE, (CS_VOID PNTR)"AWAITING COMMAND", sizeof("AWAITING COMMAND"), NULL);
    return CS_SUCCEED;
}

CS_RETCODE
TraceOS_sra_get_sequence(SRV_PROC PNTR spp)
{
	CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
    CS_INT          status = SRV_DONE_FINAL;
    CS_CHAR         logmsg[2048],acc[255];
	CS_INT		acclen=0,rc=0,rowcount=0;
    UserThreadDataPtr tdatap=NULL;
	CS_INT		spotid,dummy;
	SraUserData	*sra_data=NULL;
	int		format_mask=SRA_OUTPUT_BASECALL_BIT|SRA_OUTPUT_QUALITY_BIT|SRA_OUTPUT_QUALITY4_BIT|SRA_OUTPUT_POSITION_BIT|SRA_OUTPUT_READ_LEN_BIT|SRA_OUTPUT_READ_TYPE_BIT;
	char		*errstr=NULL;


    if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	sra_data=tdatap->userdata;
	if(!sra_data){
		tdatap->userdata=sra_data=malloc(sizeof(*sra_data));
		memset(sra_data,0,sizeof(*sra_data));
		tdatap->userdatafree=(void (*)(void*))SraUserDataDelete;
	}

    if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_char_fmt,(CS_BYTE *)acc,&acclen,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_int_fmt,(CS_BYTE *)&spotid,&dummy,NULL) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
    if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	acc[acclen]=0;
    srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"SRA_SEQUENCE",sizeof("SRA_SEQUENCE"),NULL);
	
	rc=SraUserDataSetRunAccession(tros.sradb,sra_data,acc,&errstr);
	if(errstr != 0) os_ErrPost(NULL,spp,errstr,0,0,CS_TRUE,0);
	MACRO_CHECK4SRAERROR(rc, acc, SraUserDataSetRunAccession);
	if(sra_data->platform==SRA_PLATFORM_ABSOLID){
		format_mask|=SRA_OUTPUT_COLOR_SPACE_BIT;
	} else {
		format_mask|=SRA_OUTPUT_BASECALL_BIT;
	}
	
    rowcount=0;

	if(spotid > sra_data->run_spot_last || spotid < sra_data->run_spot_first){
		sprintf(logmsg,"SRA_RUN|%s|Spot_first %d is outside the range %d:%d",acc,spotid,sra_data->run_spot_first,sra_data->run_spot_last);
                os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,0);
                status |= SRV_DONE_ERROR;
                goto DONE;
	}
	rc=SraUserDataOpen(sra_data,spotid,format_mask,0);
	MACRO_CHECK4SRAERROR(rc, acc, SraUserDataOpen);

	if(SraUserSequenceSend(spp,sra_data)!=CS_SUCCEED){
		status |= SRV_DONE_ERROR;
                goto DONE;
	}
DONE:
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:0);
	
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,rowcount);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
	SraUserDataReset(sra_data);
        return CS_SUCCEED;

}
CS_RETCODE
TraceOS_sra_get_asnprop(SRV_PROC PNTR spp,int sat_key)
{
    CS_INT      status = SRV_DONE_FINAL;
    CS_CHAR     acc[255];
	CS_INT		acclen=0,rc=0,rowcount=0;
    UserThreadDataPtr tdatap=NULL;
	SraUserData	*sra_data=NULL;
	char		*errstr=NULL;


    if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
            status |= SRV_DONE_ERROR;
            goto DONE;
    }
	sat_key/=1000;
	if(sat_key < 1000000){
		acclen=sprintf(acc,"SRR%06d",sat_key);
	}else if (sat_key < 2000000){
		acclen=sprintf(acc,"ERR%06d",sat_key-1000000);
	}else {
		acclen=0;
	}

	sra_data=tdatap->userdata;
	if(!sra_data){
		tdatap->userdata=sra_data=malloc(sizeof(*sra_data));
		memset(sra_data,0,sizeof(*sra_data));
		tdatap->userdatafree=(void (*)(void*))SraUserDataDelete;
	}

	rc=SraUserDataSetRunAccession(tros.sradb,sra_data,acc,&errstr);
	if(errstr != 0) os_ErrPost(NULL,spp,errstr,0,0,CS_TRUE,0);
	MACRO_CHECK4SRAERROR(rc, acc, SraUserDataSetRunAccession);

	Sra_send_asnprop(spp);
	
DONE:
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:0);
	
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,rowcount);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
	SraUserDataReset(sra_data);
        return CS_SUCCEED;

}
CS_RETCODE
TraceOS_trace_query(SRV_PROC PNTR spp)
{
        CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
	CS_DATAFMT      cs_char_fmt = {"",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,1024,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_INT          status = SRV_DONE_FINAL|SRV_DONE_COUNT;
        CS_CHAR         logmsg[2048];
	CS_INT		outlen;
        UserThreadDataPtr tdatap=NULL;
	TraceQuery	tq;


	tq.rowcount=0;
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_char_fmt,(CS_BYTE *)tq.sql,&outlen,NULL) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_int_fmt,(CS_BYTE *)&tq.page_size,NULL,NULL) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
	}
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,3,&cs_int_fmt,(CS_BYTE *)&tq.page_number,NULL,NULL) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
	}
        if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
	tq.sql[outlen]='0';
        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"TRACE_QUERY",sizeof("TRACE_QUERY"),NULL);
	sprintf(logmsg,"TRACE_QUERY: <%s> PS:<%d> PN:<%d>",tq.sql,tq.page_size,tq.page_number);
        os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
	os_ExecOnBestConnection(spp,tdatap,&tros.iqconn,&tq,TraceOS_trace_query_cb,CS_FALSE);
DONE:
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:0);
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,tq.rowcount);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
	sprintf(logmsg,"TRACE_QUERY: <%d> tis returned",tq.rowcount);
        os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
        return CS_SUCCEED;

}
CS_RETCODE
TraceOS_get_trace_data_by_ti(SRV_PROC PNTR spp)
{
        CS_DATAFMT      cs_int_fmt = {"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_DATAFMT      cs_tinyint_fmt = {"",CS_NULLTERM,CS_TINYINT_TYPE,CS_FMT_UNUSED,sizeof(CS_TINYINT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
        CS_INT          status = SRV_DONE_FINAL;
        CS_CHAR         logmsg[512],sql[1024];
	CS_CHAR PNTR	sqlptr;
        UserThreadDataPtr tdatap=NULL;
	CS_INT		ti;
	CS_SMALLINT	outform;
	CS_TINYINT	mask;
	int		i_data=0;


        if (srv_bind(spp,CS_GET,SRV_RPCDATA,1,&cs_int_fmt,(CS_BYTE *)&ti,NULL,NULL) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        if (srv_bind(spp,CS_GET,SRV_RPCDATA,2,&cs_tinyint_fmt,(CS_BYTE *)&mask,NULL,NULL) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        if (srv_xferdata(spp,CS_GET,SRV_RPCDATA) != CS_SUCCEED){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }

	sprintf(logmsg,"Calling get_trace_data_by_ti  %d,%d",ti,mask);
	os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);

	if(ti==0 || mask==0){
os_ErrPost(NULL,spp,"    ",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"Usage 'get_trace_data_by_ti ti[,mask=21]'",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                          ti      - positive integer trace identifiers",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                          mask    - bitmask of requested information",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"    ",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"Bit values are:           1 - base calls",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                          2 - peak positions",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                          4 - quality scores",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                          8 - trace image",0,5,CS_TRUE,100);
os_ErrPost(NULL,spp,"                         16 - descriptive fields",0,5,CS_TRUE,100);
		status |= SRV_DONE_ERROR;
                goto DONE;
        }

	tdatap->got_attention=0;
	outform=mask;
	/*** header ***/
	if(outform & 16){
		sprintf(sql,"exec usp_tros_get_trace_info_by_ti %d",ti);
		outform &= ~16;
		if(os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,sql,os_gateway_lang_func_cb_results_only,CS_FALSE) != CS_SUCCEED){
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
	}
	
	if(outform & 7 ){ /**** basecall,peak,quality  ***/
		CS_INT outform2=0;
		TraceData PNTR  tdp=NULL;
		if(outform & 1)  outform2 |=GET_TRACE_DATA_BASECALL;
		if(outform & 2)  outform2 |=GET_TRACE_DATA_PEAK;
		if(outform & 4)  outform2 |=GET_TRACE_DATA_QSCORES;

		if((i_data=TraceOS_tracedb_find_data(spp,ti,outform2,&tdp)) < 0){
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
		if(tdp){
			size_t    bc_len=0,qs_len=0,pp_len=0;
                        CharPtr  bc=NULL;
			const void *  qs=NULL;
			const void *  pp=NULL;

                        bc=(CharPtr)TraceDataGetBasecall(tdp,&bc_len);
			qs=TraceDataGetQualscore(tdp,&qs_len);
			pp=TraceDataGetPeakindex(tdp,&pp_len);
			if( pp_len > 0 && bc_len > 0 && pp_len != bc_len*4){
				sprintf(logmsg,"TI=%d|bad length of peak_index <%d> expected <%d>",ti,(int)pp_len,(int)bc_len*4);
				os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
				if(pp_len < bc_len*4){
					pp=dummy_buf;
				}
				pp_len=bc_len*4;
			}
			

#ifdef _DEBUG
			sprintf(logmsg,"TI=%d|bc_len=%d|pp_len=%d|qs_len=%d ",ti,bc_len,pp_len,qs_len);
			os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
#endif
			if(1 || ((bc && bc_len) > 0)){
				CS_SMALLINT	bc_ind,qs_ind,pp_ind;
       				CS_DATAFMT      cs_bc_fmt=
			 {"base_call_full",CS_NULLTERM,CS_TEXT_TYPE,CS_FMT_UNUSED,1,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,1,0,NULL};
       				CS_DATAFMT      cs_qs_fmt=
			 {"qual_score_full",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,1,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,1,0,NULL};
       				CS_DATAFMT      cs_pp_fmt=
			 {"peak_index_full",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,1,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,1,0,NULL};
				CS_IODESC	bc_io;
				CS_IODESC	qs_io;
				CS_IODESC	pp_io;

				cs_bc_fmt.maxlength=bc_len;
				memset(&bc_io,sizeof(bc_io),0);
				bc_io.iotype=CS_IODATA;
				strcpy(bc_io.name,cs_bc_fmt.name);
				bc_io.namelen=CS_NULLTERM;
				bc_io.timestamplen=CS_TS_SIZE;
				bc_io.textptrlen  =CS_TP_SIZE;
				bc_io.datatype = cs_bc_fmt.datatype;
				bc_io.total_txtlen = bc_len;
				bc_ind=(bc_len > 0)?CS_GOODDATA:CS_NULLDATA;

				cs_qs_fmt.maxlength=qs_len;
				memset(&qs_io,sizeof(qs_io),0);
				qs_io.iotype=CS_IODATA;
				strcpy(qs_io.name,cs_qs_fmt.name);
				qs_io.namelen=CS_NULLTERM;
				qs_io.timestamplen=CS_TS_SIZE;
				qs_io.textptrlen  =CS_TP_SIZE;
				qs_io.datatype = cs_qs_fmt.datatype;
				qs_io.total_txtlen = qs_len;
				qs_ind=(qs_len > 0)?CS_GOODDATA:CS_NULLDATA;

				cs_pp_fmt.maxlength=pp_len;
				memset(&pp_io,sizeof(pp_io),0);
				pp_io.iotype=CS_IODATA;
				strcpy(pp_io.name,cs_pp_fmt.name);
				pp_io.namelen=CS_NULLTERM;
				pp_io.timestamplen=CS_TS_SIZE;
				pp_io.textptrlen  =CS_TP_SIZE;
				pp_io.datatype = cs_pp_fmt.datatype;
				pp_io.total_txtlen = pp_len;
				pp_ind=(pp_len > 0)?CS_GOODDATA:CS_NULLDATA;


				if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_bc_fmt) == CS_SUCCEED	
				   && srv_descfmt(spp,CS_SET,SRV_ROWDATA,2,&cs_pp_fmt) == CS_SUCCEED	
				   && srv_descfmt(spp,CS_SET,SRV_ROWDATA,3,&cs_qs_fmt) == CS_SUCCEED	
				   && srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_bc_fmt,(CS_BYTE*)bc,(CS_INT*)&bc_len,&bc_ind) == CS_SUCCEED
				   && srv_bind(spp,CS_SET,SRV_ROWDATA,2,&cs_pp_fmt,(CS_BYTE*)pp,(CS_INT*)&pp_len,&pp_ind) == CS_SUCCEED
				   && srv_bind(spp,CS_SET,SRV_ROWDATA,3,&cs_qs_fmt,(CS_BYTE*)qs,(CS_INT*)&qs_len,&qs_ind) == CS_SUCCEED
				   && srv_text_info(spp,CS_SET,1,&bc_io) == CS_SUCCEED
				   && srv_text_info(spp,CS_SET,2,&pp_io) == CS_SUCCEED
				   && srv_text_info(spp,CS_SET,3,&qs_io) == CS_SUCCEED
				   && srv_xferdata(spp,CS_SET,SRV_ROWDATA) == CS_SUCCEED
				   && srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,1)){
					outform &= ~7;
				}
			}
			TraceDataWhack(tdp);
		}
		if(outform & 7 && tros.trlocs.trloc_data[i_data].dbname && tros.trlocs.trloc_data[i_data].dbname[0]){

			sqlptr=sql;
			sqlptr += sprintf(sqlptr,"select order_id 'order_id'");
			if(outform & 1)  sqlptr += sprintf(sqlptr,",basecall 'base_call'");
			if(outform & 2)  sqlptr += sprintf(sqlptr,",peakindex 'peak_index'");
			if(outform & 4)  sqlptr += sprintf(sqlptr,",qualscore 'qual_score'");

			sqlptr += sprintf(sqlptr," from %s..TraceData where ti=%d order by order_id at isolation 0",tros.trlocs.trloc_data[i_data].dbname,ti);
			if(os_ExecOnBestConnection(spp,tdatap,tros.trlocs.trloc_data[i_data].bsrv,sql,os_gateway_lang_func_cb_results_only,CS_FALSE) != CS_SUCCEED){
				status |= SRV_DONE_ERROR;
				goto DONE;
			}
			outform &= ~7;
		}
	}
	if(outform & 8){
		TIintro	tii;
		memset(&tii,0,sizeof(tii));
		tii.ti=ti;
		if(TraceOS_get_trace_image(spp,tdatap,&tii,NULL) != CS_SUCCEED){
			status |= SRV_DONE_ERROR;
			goto DONE;
		}
                outform &= ~8;
        }




DONE:
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?-6:0);
        srv_senddone(spp,status,CS_TRAN_UNDEFINED,0);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
        os_ErrPost(NULL,spp,"get_trace_data_by_ti has successfully completed",0,0,CS_FALSE,10);
        return CS_SUCCEED;

}

/*****************************************/
/************* CALLBACKS       ************/
/*****************************************/

CS_RETCODE
TraceLocationSetInit(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg)
{
	TraceLocationSetPtr trlocs=(TraceLocationSetPtr)arg;
	
	return (
		TraceLocationsInit(&trlocs->trloc_img,&trlocs->numsrv_img,trlocs->trloc_img_tab,ctcmd,&trlocs->vn_bsrv_list)
	     && TraceLocationsInit(&trlocs->trloc_data,&trlocs->numsrv_data,trlocs->trloc_data_tab,ctcmd,&trlocs->vn_bsrv_list)
	     && TraceLocationsInit(&trlocs->trloc_alt,&trlocs->numsrv_alt,trlocs->trloc_alt_tab,ctcmd,&trlocs->vn_bsrv_list)
	)?CS_SUCCEED:CS_FAIL;
}
CS_RETCODE
TraceFieldsInit(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg)
{
	TraceFieldPtr	trace_fields_new=NULL,trf_run,trf_tail=NULL;
	CS_RETCODE      retcode;
        CS_INT          restype,count,outlen;
        CS_CHAR         sqltext[]="select name,field_prim,searchable,static from TraceMain..Main order by name";

        retcode=ct_command(ctcmd,CS_LANG_CMD,sqltext,CS_NULLTERM,CS_UNUSED);
        retcode=ct_send(ctcmd);
        while((retcode=ct_results(ctcmd,&restype))==CS_SUCCEED){
                switch(restype){
                 case CS_ROW_RESULT:
                        while((retcode=ct_fetch(ctcmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&count))==CS_SUCCEED){
				trf_run=MemNew(sizeof(*trf_run));
				ct_get_data(ctcmd,1,trf_run->name,sizeof(trf_run->name)-1,&outlen);
				trf_run->name[outlen]='\0';
				ct_get_data(ctcmd,2,trf_run->field_prim,sizeof(trf_run->field_prim)-1,&outlen);
				trf_run->field_prim[outlen]='\0';
				ct_get_data(ctcmd,3,&trf_run->is_searchable,sizeof(trf_run->is_searchable),&outlen);
				ct_get_data(ctcmd,4,&trf_run->is_static,sizeof(trf_run->is_static),&outlen);
				if(trace_fields_new==NULL){
					trf_tail=trace_fields_new=trf_run;
				} else {
					trf_tail->next=trf_run;
					trf_tail=trf_run;
				}
                        }
                        if(retcode==CS_ROW_FAIL){
                                goto FATAL;
                        } else if(retcode == CS_CANCELED){
                        }
                        break;
                 case CS_STATUS_RESULT:
                        while((retcode=ct_fetch(ctcmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&count))==CS_SUCCEED){}
                        if(retcode == CS_CANCELED){
                        }
                        break;
                 case CS_CMD_SUCCEED:
                 case CS_CMD_DONE:
                        break;
                 default:
                        goto FATAL;
                }
        }
        switch(retcode){
         case CS_END_RESULTS:
                goto DONE; 
         case CS_CANCELED:
                goto FATAL;
         default:
                goto FATAL;
        }
DONE:
	while(tros.trace_fields_old != NULL){/** free old data **/
		trf_run=tros.trace_fields_old->next;
		MemFree(tros.trace_fields_old);
		tros.trace_fields_old=trf_run;
	}
	tros.trace_fields_old=tros.trace_fields; /** Save current data - someone may be using it now ***/
	tros.trace_fields=trace_fields_new;
	return CS_SUCCEED;
FATAL:
	while(trace_fields_new != NULL){
		trf_run=trace_fields_new->next;
		MemFree(trace_fields_new);
		trace_fields_new=trf_run;
	}
        return CS_FAIL;
}
CS_RETCODE
TraceOS_get_trace_intro_cb(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg)
{
	TIintro	PNTR	tiip=(TIintroPtr)arg;
	CS_RETCODE      retcode;
        CS_INT          restype,count,outlen;
        CS_CHAR         procname[100]="usp_tros_get_trace_intro";
        CS_DATAFMT      cs_start_fmt = {"@ti_start",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,CS_INPUTVALUE,0,0,NULL};
        CS_DATAFMT      cs_stop_fmt = {"@ti_stop",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,CS_INPUTVALUE,0,0,NULL};

	if(tiip==NULL) return CS_SUCCEED;

        retcode=ct_command(ctcmd,CS_RPC_CMD,procname,CS_NULLTERM,CS_UNUSED);
        retcode=ct_param(ctcmd,&cs_start_fmt,&tiip->ti,sizeof(tiip->ti),0);
        retcode=ct_param(ctcmd,&cs_stop_fmt,&tiip->ti,sizeof(tiip->ti),0);
        retcode=ct_send(ctcmd);
        while((retcode=ct_results(ctcmd,&restype))==CS_SUCCEED){
                switch(restype){
                 case CS_ROW_RESULT:
			while((retcode=ct_fetch(ctcmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&count))==CS_SUCCEED){
				ct_get_data(ctcmd,2,&tiip->replaced_by,sizeof(tiip->replaced_by),&outlen);
				ct_get_data(ctcmd,3,tiip->trace_name,sizeof(tiip->trace_name)-1,&outlen);
				tiip->trace_name[outlen]='\0';
				ct_get_data(ctcmd,4,tiip->center_name,sizeof(tiip->center_name)-1,&outlen);
				tiip->center_name[outlen]='\0';
				ct_get_data(ctcmd,5,tiip->species_code,sizeof(tiip->species_code)-1,&outlen);
				tiip->species_code[outlen]='\0';
				ct_get_data(ctcmd,6,tiip->trace_type_code,sizeof(tiip->trace_type_code)-1,&outlen);
				tiip->accession[outlen]='\0';
				ct_get_data(ctcmd,7,tiip->accession,sizeof(tiip->accession)-1,&outlen);
				tiip->accession[outlen]='\0';
				ct_get_data(ctcmd,8,&tiip->basecall_len,sizeof(tiip->basecall_len),&outlen);
				ct_get_data(ctcmd,9,&tiip->trace_len,sizeof(tiip->trace_len),&outlen);
				ct_get_data(ctcmd,10,&tiip->trace_table_type,sizeof(tiip->trace_table_type),&outlen);
                        }
                        if(retcode==CS_ROW_FAIL){
                                goto FATAL;
                        } else if(retcode == CS_CANCELED){
                        }
                        break;
                 case CS_STATUS_RESULT:
                        while((retcode=ct_fetch(ctcmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&count))==CS_SUCCEED){}
                        if(retcode == CS_CANCELED){
                        }
                        break;
                 case CS_CMD_SUCCEED:
                 case CS_CMD_DONE:
                        break;
                 default:
                        goto FATAL;
                }
        }
        switch(retcode){
         case CS_END_RESULTS:
                return CS_SUCCEED;
         case CS_CANCELED:
                return CS_CANCELED;
         default:
                goto FATAL;
        }
FATAL:
        return CS_FAIL;
}
CS_RETCODE
TraceOS_get_alt_data_cb(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg)
{
	TraceDataRequestPtr	tdrp=(TraceDataRequestPtr)arg;
	CS_RETCODE      retcode;
        CS_INT          restype,count,outlen,numcol;
        CS_CHAR         procname[100]="usp_tros_get_alt_data";
        CS_DATAFMT      cs_int_fmt = {"@ti",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,CS_INPUTVALUE,0,0,NULL};
	int		i_bc=0,i_qs=0,i_pi=0;

	tdrp->basecall_len=0;
	sprintf(procname,"%s..usp_tros_get_alt_data",tdrp->dbname);
        retcode=ct_command(ctcmd,CS_RPC_CMD,procname,CS_NULLTERM,CS_UNUSED);
        retcode=ct_param(ctcmd,&cs_int_fmt,&tdrp->ti,sizeof(tdrp->ti),0);
        retcode=ct_send(ctcmd);
        while((retcode=ct_results(ctcmd,&restype))==CS_SUCCEED){
                switch(restype){
                 case CS_ROW_RESULT:
			retcode=ct_res_info(ctcmd,CS_NUMDATA,(CS_VOID PNTR)&numcol,CS_UNUSED,&outlen);
                        while((retcode=ct_fetch(ctcmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&count))==CS_SUCCEED){
				if(numcol==1){
					ct_get_data(ctcmd,1,&tdrp->basecall_len,sizeof(tdrp->basecall_len),&outlen);
				} else {
					ct_get_data(ctcmd,2,tdrp->basecall+i_bc,255,&outlen);i_bc+=outlen;
					ct_get_data(ctcmd,3,tdrp->qualscore+i_qs,255,&outlen);i_qs+=outlen-1;
					ct_get_data(ctcmd,4,tdrp->peakidx+i_pi,255,&outlen);i_pi+=outlen;
				}
                        }
                        if(retcode==CS_ROW_FAIL){
                                goto FATAL;
                        } else if(retcode == CS_CANCELED){
                        }
                        break;
                 case CS_STATUS_RESULT:
                        while((retcode=ct_fetch(ctcmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&count))==CS_SUCCEED){}
                        if(retcode == CS_CANCELED){
                        }
                        break;
                 case CS_CMD_SUCCEED:
                 case CS_CMD_DONE:
                        break;
                 default:
                        goto FATAL;
                }
        }
        switch(retcode){
         case CS_END_RESULTS:
                return CS_SUCCEED;
         case CS_CANCELED:
                return CS_CANCELED;
         default:
                goto FATAL;
        }
FATAL:
        return CS_FAIL;
}
CS_RETCODE
TraceOS_trace_query_cb(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg)
{
	TraceQueryPtr	tqp=(TraceQueryPtr)arg;
	CS_RETCODE      retcode;
	CS_INT		    ti;
	CS_INT		    ti_len=sizeof(ti);
    CS_INT          restype,count,outlen;
    CS_CHAR         procname[]="dba.trace_query";
    CS_DATAFMT      cs_sql_fmt = {"@sql",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,1024,CS_DEF_SCALE,CS_DEF_PREC,CS_INPUTVALUE,0,0,NULL};
    CS_DATAFMT      cs_ps_fmt = {"@page_size",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,CS_INPUTVALUE,0,0,NULL};
    CS_DATAFMT      cs_pn_fmt = {"@page_number",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,CS_INPUTVALUE,0,0,NULL};
	CS_DATAFMT      cs_ti_fmt = {"ti",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,1,0,NULL};
	UserThreadDataPtr   tdatap=NULL;


	if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
        goto FATAL;
    }

	tdatap->got_attention=CS_FALSE;
        retcode=ct_command(ctcmd,CS_RPC_CMD,procname,CS_NULLTERM,CS_UNUSED);
        retcode=ct_param(ctcmd,&cs_sql_fmt,tqp->sql,strlen(tqp->sql),0);
        retcode=ct_param(ctcmd,&cs_ps_fmt,&tqp->page_size,sizeof(CS_INT),0);
        retcode=ct_param(ctcmd,&cs_pn_fmt,&tqp->page_number,sizeof(CS_INT),0);
        retcode=ct_send(ctcmd);
	srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_ti_fmt);	
	srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_ti_fmt,(CS_BYTE*)&ti,&ti_len,NULL);
	tqp->rowcount=0;
        while((retcode=ct_results(ctcmd,&restype))==CS_SUCCEED){
                switch(restype){
                 case CS_ROW_RESULT:
                        while((retcode=ct_fetch(ctcmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&count))==CS_SUCCEED){
				ct_get_data(ctcmd,1,&ti,sizeof(ti),&outlen);
				if(srv_xferdata(spp,CS_SET,SRV_ROWDATA) != CS_SUCCEED){
					ct_cancel(NULL,ctcmd,CS_CANCEL_ALL);
				} else {
					tqp->rowcount++;
				}
                        }
                        if(retcode==CS_ROW_FAIL){
                                goto FATAL;
                        } else if(retcode == CS_CANCELED){
                        }
			if(tdatap->got_attention){
				ct_cancel(NULL,ctcmd,CS_CANCEL_ALL);
			}
                        break;
                 case CS_STATUS_RESULT:
                        while((retcode=ct_fetch(ctcmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&count))==CS_SUCCEED){}
                        if(retcode == CS_CANCELED){
                        }
                        break;
                 case CS_CMD_SUCCEED:
                 case CS_CMD_DONE:
                        break;
                 default:
                        goto FATAL;
                }
        }
        switch(retcode){
         case CS_END_RESULTS:
                return CS_SUCCEED;
         case CS_CANCELED:
                return CS_CANCELED;
         default:
                goto FATAL;
        }
FATAL:
        return CS_FAIL;
}

CS_RETCODE
TraceOS_get_table_type_cb(SRV_PROC PNTR spp,CS_COMMAND PNTR ctcmd,VoidPtr arg)
{
	CS_INT	PNTR	type = (CS_INT PNTR)arg;
	CS_RETCODE      retcode;
        CS_INT          restype,count,outlen;
        CS_CHAR         sql[1024]="usp_tros_get_alt_data";

	sprintf(sql,"select convert(int,trace_table_type) from Trace where ti=%d at isolation 0",*type);
	*type=-1;
        retcode=ct_command(ctcmd,CS_LANG_CMD,sql,CS_NULLTERM,CS_UNUSED);
        retcode=ct_send(ctcmd);
        while((retcode=ct_results(ctcmd,&restype))==CS_SUCCEED){
                switch(restype){
                 case CS_ROW_RESULT:
                        while((retcode=ct_fetch(ctcmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&count))==CS_SUCCEED){
				ct_get_data(ctcmd,1,type,sizeof(*type),&outlen);
			}
                        if(retcode==CS_ROW_FAIL){
                                goto FATAL;
                        } else if(retcode == CS_CANCELED){
                        }
                        break;
                 case CS_STATUS_RESULT:
                        while((retcode=ct_fetch(ctcmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&count))==CS_SUCCEED){}
                        if(retcode == CS_CANCELED){
                        }
                        break;
                 case CS_CMD_SUCCEED:
                 case CS_CMD_DONE:
                        break;
                 default:
                        goto FATAL;
                }
        }
        switch(retcode){
         case CS_END_RESULTS:
                return CS_SUCCEED;
         case CS_CANCELED:
                return CS_CANCELED;
         default:
                goto FATAL;
        }
FATAL:
        return CS_FAIL;
}


/******************************************/
/****** HELPER FUNCTIONS ******************/
CS_RETCODE
TraceOS_get_trace_image(SRV_PROC PNTR spp, UserThreadDataPtr tdatap,TIintroPtr tiip,Int4Ptr im_src)
{
	CS_INT	i_img=0;
	CS_CHAR sql[256];
	CS_CHAR	logmsg[1024];
	CS_RETCODE	retcode;
	Uint4		ti=tiip->ti;
	while(ti > tros.trlocs.trloc_img[i_img].ti_stop  && tros.trlocs.trloc_img[i_img].ti_stop != 0xFFFFFFFF){
		i_img++;
		if(i_img >  tros.trlocs.numsrv_img){
			os_ErrPost(NULL,spp,"Image location cache is corrupted...",0,10,CS_TRUE,0);
			return CS_FAIL;
		}
	}
/*********** check if can do it from file cache ****/
	if(tros.trlocs.trloc_img[i_img].path[0]){
		int		rc;
		size_t	bytes_read,offset,bytes_left;
		Char	buf[128*1024];	

		offset=0;
		if((rc=TraceDBRead(&tros.tracedb_img,tros.trlocs.trloc_img[i_img].path,ti,buf,sizeof(buf),
									offset,&bytes_read,&bytes_left))!= 0 ){
			bytes_read=bytes_left=0;
		}
		sprintf(logmsg,"bytes_read: %d; bytes_left: %d; ret_code: %d; file=<%s>",(int)bytes_read,(int)bytes_left,rc,tros.trlocs.trloc_img[i_img].path);
		os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,10);

		if(bytes_read > 0){
			CS_DATAFMT      cs_trace_fmt = {"trace",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,10,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,1,0,NULL};
			CS_IODESC	iodesc;

			srv_bzero(&iodesc,sizeof(iodesc));
			iodesc.iotype=CS_IODATA;
			iodesc.datatype=CS_IMAGE_TYPE;
			iodesc.total_txtlen = bytes_read + bytes_left;
			cs_trace_fmt.maxlength=iodesc.total_txtlen;
			iodesc.namelen=CS_NULLTERM;
			iodesc.timestamplen=CS_TS_SIZE;
			iodesc.textptrlen  =CS_TP_SIZE;

			retcode=srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_trace_fmt);
			retcode=srv_text_info(spp,CS_SET,1,&iodesc);
			if(srv_send_text(spp,(CS_BYTE PNTR)buf,bytes_read) != CS_SUCCEED){
				os_ErrPost(NULL,spp,"srv_send_text failed",0,10,CS_TRUE,0);
				return CS_FAIL;
			}
			for(offset=bytes_read; bytes_left > 0; offset+=bytes_read){
				if((rc=TraceDBRead(&tros.tracedb_img,tros.trlocs.trloc_img[i_img].path,ti,buf,sizeof(buf),
						offset,&bytes_read,&bytes_left))!= 0 ){
					bytes_read=bytes_left=0;
				}
				sprintf(logmsg,"bytes_read: %d; bytes_left: %d; ret_code: %d",(int)bytes_read,(int)bytes_left,rc);
                		os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,10);

				if(bytes_read <=0 ) return CS_FAIL;
				if(srv_send_text(spp,(CS_BYTE PNTR)buf,bytes_read) != CS_SUCCEED){
	                                os_ErrPost(NULL,spp,"srv_send_text failed",0,10,CS_TRUE,0);
        	                        return CS_FAIL;
                	        }
			}
			if(im_src) *im_src=IMAGE_SOURCE_FILE;
			srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,1);
			return CS_SUCCEED;
		} else {
			sprintf(logmsg,"TI=%d| Failed to retrieve image from <%s>",ti,tros.trlocs.trloc_img[i_img].path);
			os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
		}

	}
	if(tros.trlocs.trloc_img[i_img].srvname[0]=='\0'){
		sprintf(logmsg,"TI=%d| Failed to retrieve trace image data",ti);
                os_ErrPost(NULL,spp,logmsg,0,10,CS_TRUE,0);
		return CS_FAIL;
	}
	if(   tiip->trace_table_type==0){
		tiip->trace_table_type=ti; /**** will be used in callback ****/ 
	   	if(os_ExecOnBestConnection(spp,tdatap,&tros.trmainconn,&tiip->trace_table_type,TraceOS_get_table_type_cb,CS_FALSE) != CS_SUCCEED){
			return CS_FAIL;
		}
		if(tiip->trace_table_type <= 0 || tiip->trace_table_type > 2 ){
			sprintf(logmsg,"TI=%d|Bad table <%d> type was returned trying to reset it to 1",ti,tiip->trace_table_type);
                        os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
			tiip->trace_table_type=1;
		}
	}
	sprintf(sql,"exec %s..usp_get_image_data %d,%d",tros.trlocs.trloc_img[i_img].dbname,ti,tiip->trace_table_type);
	if(os_ExecOnBestConnection(spp,tdatap,tros.trlocs.trloc_img[i_img].bsrv,sql,
			   (tiip->trace_table_type==1)?os_gateway_lang_func_cb_single_image:os_gateway_lang_func_cb_results_only,CS_FALSE) != CS_SUCCEED){
		return CS_FAIL;
	}
	if(im_src) *im_src=IMAGE_SOURCE_DB;
	return CS_SUCCEED;
}
CS_INT
TraceOS_tracedb_find_data(SRV_PROC PNTR spp,Uint4 ti,Int4 outform,TraceData ** tdp)
{
	CS_INT	i_data=0;
	CS_CHAR	logmsg[1024];


	*tdp=NULL;
	while(ti > tros.trlocs.trloc_data[i_data].ti_stop  && tros.trlocs.trloc_data[i_data].ti_stop != 0xFFFFFFFF){
		i_data++;
		if(i_data >  tros.trlocs.numsrv_data){
			os_ErrPost(NULL,spp,"Data location cache is corrupted...",0,10,CS_TRUE,0);
			return -1;
		}
	}
/*********** check if can do it from file cache ****/
	if(tros.trlocs.trloc_data[i_data].path[0]){
		int		rc=0;
		unsigned int fields=0;

		if(outform & GET_TRACE_DATA_QSCORES) fields |= td_qualscore;
		if(outform & GET_TRACE_DATA_BASECALL)fields |= td_basecall;
		if(outform & GET_TRACE_DATA_PEAK)    fields |= td_peakindex;
		if(outform & GET_TRACE_DATA_COMMENT) fields |= td_comment | td_extended;

		rc=TraceDBGetTraceData(&tros.tracedb_data,tros.trlocs.trloc_data[i_data].path,(ti_t)ti,fields,tdp);
		if(rc==0 && *tdp!=NULL){
#ifdef _DEBUG
			sprintf(logmsg,"TI=%d| Found in <%s>",ti,tros.trlocs.trloc_data[i_data].path);
			os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
#endif
			return i_data;
		} else {
			sprintf(logmsg,"TI=%d| Not found in <%s> | rc=%d",ti,tros.trlocs.trloc_data[i_data].path,rc);
			os_ErrPost(NULL,spp,logmsg,0,0,CS_FALSE,0);
			*tdp=NULL;
			return i_data;
		}
	} else {
		return i_data;
	}
	return -1; /*** impossible ***/
}
CS_RETCODE
TraceOS_ti_intro_output_func(SRV_PROC PNTR spp,TIintroPtr tiip)
{
        CS_INT          status = SRV_DONE_MORE;
        UserThreadDataPtr tdatap=NULL;
        CS_DATAFMT      cs_ti_fmt = {"ti",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_replaced_by_fmt = {"replaced_by",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_trace_name_fmt = {"trace_name",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,250,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_center_name_fmt = {"center_name",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,50,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_species_code_fmt = {"species_code",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,100,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_trace_type_code_fmt = {"trace_type_code",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,50,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_accession_fmt = {"accession",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,30,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
        CS_DATAFMT      cs_basecall_len_fmt = {"basecall_len",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_trace_len_fmt = {"trace_len",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_type_fmt = {"trace_table_type",CS_NULLTERM,CS_SMALLINT_TYPE,CS_FMT_UNUSED,sizeof(CS_SMALLINT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};

	CS_INT          intlen=sizeof(CS_INT),smallint_len=sizeof(CS_SMALLINT);
	CS_INT		trace_name_len,center_name_len,species_code_len,trace_type_code_len,accession_len;
	CS_SMALLINT	accind=0;

	if(tiip==NULL){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
	if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }

	
 	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_ti_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_ti_fmt,(CS_BYTE*)&tiip->ti,&intlen,NULL)!=CS_SUCCEED
 	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,2,&cs_replaced_by_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,2,&cs_replaced_by_fmt,(CS_BYTE*)&tiip->replaced_by,&intlen,NULL)!=CS_SUCCEED
 	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,3,&cs_trace_name_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,3,&cs_trace_name_fmt,(CS_BYTE*)tiip->trace_name,&trace_name_len,NULL)!=CS_SUCCEED
 	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,4,&cs_center_name_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,4,&cs_center_name_fmt,(CS_BYTE*)tiip->center_name,&center_name_len,NULL)!=CS_SUCCEED
 	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,5,&cs_species_code_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,5,&cs_species_code_fmt,(CS_BYTE*)tiip->species_code,&species_code_len,NULL)!=CS_SUCCEED
 	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,6,&cs_trace_type_code_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,6,&cs_trace_type_code_fmt,(CS_BYTE*)tiip->trace_type_code,&trace_type_code_len,NULL)!=CS_SUCCEED
 	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,7,&cs_accession_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,7,&cs_accession_fmt,(CS_BYTE*)tiip->accession,&accession_len,&accind)!=CS_SUCCEED
 	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,8,&cs_basecall_len_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,8,&cs_basecall_len_fmt,(CS_BYTE*)&tiip->basecall_len,&intlen,NULL)!=CS_SUCCEED
 	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,9,&cs_trace_len_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,9,&cs_trace_len_fmt,(CS_BYTE*)&tiip->trace_len,&intlen,NULL)!=CS_SUCCEED
 	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,10,&cs_type_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,10,&cs_type_fmt,(CS_BYTE*)&tiip->trace_table_type,&smallint_len,NULL)!=CS_SUCCEED
                ){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
	trace_name_len=strlen(tiip->trace_name);
	center_name_len=strlen(tiip->center_name);
	species_code_len=strlen(tiip->species_code);
	trace_type_code_len=strlen(tiip->trace_type_code);
	if(tiip->accession[0]=='\0' || tiip->accession[0]==' '){
		accession_len=0;
		accind=-1;
	} else {
		accession_len=strlen(tiip->accession);
		accind=0;
 	}	
	if(srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED){
		status |= SRV_DONE_ERROR;
		goto DONE;
	}

DONE:
	srv_senddone(spp,status,CS_TRAN_UNDEFINED,1);
        return (status&SRV_DONE_ERROR)?CS_FAIL:CS_SUCCEED;
}
CS_RETCODE
TraceOS_show_locations_func(SRV_PROC PNTR spp,TraceLocationPtr trloc,int cnt)
{
        CS_INT          status = SRV_DONE_FINAL;
        CS_INT     	ti_start,ti_stop;
        CS_CHAR		logmsg[200];
        CS_CHAR         srv[21],db[21],path[256];
        UserThreadDataPtr tdatap=NULL;
        CS_DATAFMT      cs_start_fmt =
                {"ti_start",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_stop_fmt =
                {"ti_stop",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_srv_fmt =
                {"srv",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,20,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
        CS_DATAFMT      cs_db_fmt =
                {"db",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,20,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
        CS_DATAFMT      cs_path_fmt =
                {"path",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};

	CS_INT          intlen=sizeof(CS_INT),srvlen,dblen,i=0,pathlen;
	CS_SMALLINT	pathind=0,dbind=0,srvind=0;

	if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        sprintf(logmsg,"OPEN SERVER NAME: %s",tros.osi.srvname);
	os_ErrPost(tros.osi.context,spp,logmsg,0,0,CS_TRUE,500);
	
 	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_start_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_start_fmt,(CS_BYTE*)&ti_start,&intlen,NULL)!=CS_SUCCEED
 	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,2,&cs_stop_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,2,&cs_stop_fmt,(CS_BYTE*)&ti_stop,&intlen,NULL)!=CS_SUCCEED
           || srv_descfmt(spp,CS_SET,SRV_ROWDATA,3,&cs_srv_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,3,&cs_srv_fmt,(CS_BYTE*)srv,&srvlen,&srvind)!=CS_SUCCEED
           || srv_descfmt(spp,CS_SET,SRV_ROWDATA,4,&cs_db_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,4,&cs_db_fmt,(CS_BYTE*)db,&dblen,&dbind)!=CS_SUCCEED
           || srv_descfmt(spp,CS_SET,SRV_ROWDATA,5,&cs_path_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,5,&cs_path_fmt,(CS_BYTE*)path,&pathlen,&pathind)!=CS_SUCCEED
                ){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
	for(i=0;i<cnt;i++){
		ti_start=trloc[i].ti_start;
		ti_stop=trloc[i].ti_stop;
		strcpy(srv,trloc[i].srvname); srvlen=strlen(srv);srvind=(srvlen==0)?-1:0;
		strcpy(db,trloc[i].dbname);   dblen=strlen(db); dbind=(dblen==0)?-1:0;
		strcpy(path,trloc[i].path);   pathlen=strlen(path); pathind=(pathlen==0)?-1:0;
		if(srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED){
			status |= SRV_DONE_ERROR;
			goto DONE;
		}

	}
DONE:
	srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?100:0);
	srv_senddone(spp,status,CS_TRAN_UNDEFINED,i);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
        return CS_SUCCEED;
}
Boolean
TraceLocationsInit(TraceLocationPtr PNTR trlocp,Int4Ptr numsrvp, CharPtr table_name,CS_COMMAND PNTR ctcmd,ValNodePtr PNTR vn_bsrv_list)
{
        CS_CHAR         	logmsg[1024];
	CS_RETCODE 		retcode;	
	CS_INT			restype,outlen,count;
	int			i;
	Char			sql[1024];
	Int4			numsrv_new;
	TraceLocationPtr	trloc_new,trloc_old;


	sprintf(sql,"select count(*) from %s",table_name);
        if(!CTlibSingleValueSelect(ctcmd,sql,&numsrv_new,sizeof(numsrv_new))){
                 return FALSE;
        }
        trloc_new=MemNew((numsrv_new)*sizeof(*trloc_new));

	sprintf(sql,
"select ti_start,ti_stop, \
        case when state & 16777216 = 0 then server_name else NULL end as server_name, \
	case when state & 16777216 = 0 then database_name else NULL end as database_name, \
	case when state & 8192 != 0 then path else NULL end as path \
   from %s  where state & 255 != 0 order by sign(ti_start) desc,ti_start asc",table_name);

	CTRUN(ct_command(ctcmd,CS_LANG_CMD,sql,CS_NULLTERM,CS_UNUSED));
	CTRUN(ct_send(ctcmd));
	while((retcode=ct_results(ctcmd,&restype)) == CS_SUCCEED){
		CTlib_TYPERES(restype);
                switch(restype){
                 case CS_ROW_RESULT:
                        for(i=0;ct_fetch(ctcmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&count)==CS_SUCCEED;i++){
				if(i >= numsrv_new){
					CTRUN(ct_cancel(NULL,ctcmd,CS_CANCEL_CURRENT));
					break;
				}
                                CTRUN1(ct_get_data(ctcmd,1,&trloc_new[i].ti_start,sizeof(trloc_new[i].ti_start),&outlen),1);
                                CTRUN1(ct_get_data(ctcmd,2,&trloc_new[i].ti_stop,sizeof(trloc_new[i].ti_stop),&outlen),1);
                                CTRUN1(ct_get_data(ctcmd,3,trloc_new[i].srvname,sizeof(trloc_new[i].srvname)-1,&outlen),1);
				if(outlen<=1 || trloc_new[i].srvname[0]==' ')outlen=0;
				trloc_new[i].srvname[outlen]='\0';
                                CTRUN1(ct_get_data(ctcmd,4,trloc_new[i].dbname,sizeof(trloc_new[i].dbname)-1,&outlen),1);
				trloc_new[i].dbname[outlen]='\0';
                                CTRUN1(ct_get_data(ctcmd,5,trloc_new[i].path,sizeof(trloc_new[i].path)-1,&outlen),1);
				/*trloc_new[i].path[4]=trloc_new[i].path[5]=trloc_new[i].path[6]='/';*/
				if(outlen==1 && trloc_new[i].path[0]==' ') outlen=0;
				trloc_new[i].path[outlen]='\0';
                        }
                        break;
                 case  CS_STATUS_RESULT: /* skip remote procedure status */
                 default:
                        CTRUN(ct_cancel(NULL,ctcmd,CS_CANCEL_CURRENT)); /* instead of next fetch */
                        break;
                }
	}
	if(retcode!=CS_END_RESULTS)return FALSE;


	for(i=0;i<numsrv_new;i++){
		BalancedServerPtr       bsrv=NULL;
		ValNodePtr		vnp;
		if(trloc_new[i].bsrv==NULL && trloc_new[i].srvname[0]!='\0'){
			for(vnp=*vn_bsrv_list;vnp;vnp=vnp->next){
				bsrv=(BalancedServerPtr)vnp->data.ptrvalue;
#ifdef BALANCED_CONN_MGR
                if( (bsrv->m_Dbs != NULL) && (bsrv->m_Dbs[0] != NULL) && (bsrv->numservers > 0) &&
                    !strcmp( trloc_new[i].srvname, bsrv->m_Dbs[0]->m_Srv)){
#else
				if(!strcmp(trloc_new[i].srvname,bsrv->clu[0].srv)){
#endif
					trloc_new[i].bsrv=bsrv;
					break;
				}
			}
			if(!trloc_new[i].bsrv){
				CharPtr	connstr=MemNew(255);
				
				sprintf(logmsg,"TRACE_LOCATION_CACHE|Creating connection pool to server <%s>",trloc_new[i].srvname);
				os_ErrPost(NULL,NULL,logmsg,0,0,CS_FALSE,0);
				sprintf(connstr,"%s:master=anyone,allowed",trloc_new[i].srvname);
				bsrv=MemNew(sizeof(*bsrv));
				if(!BalancedServerSetupConnections(bsrv,tros.osi.context,connstr,NULL,tros.numconn,tros.packetsize)){
					goto errexit;
				}
				bsrv->is_online=TRUE;
				ServerListAddService(&tros.servers,bsrv);
				if(!BalancedServerSetupMutexes(bsrv)) goto  errexit;
				vnp=ValNodeAdd(vn_bsrv_list);
				vnp->data.ptrvalue=bsrv;
				trloc_new[i].bsrv=bsrv;
			}
		}
	}
	trloc_old=*trlocp;
	*trlocp=trloc_new;
	*numsrvp=numsrv_new;
	if(trloc_old) { sleep(1);MemFree(trloc_old);}
        return TRUE;
errexit:
	MemFree(trloc_new);
	return FALSE;
}

Boolean
TraceOS_new_request_log(CharPtr	newfile)
{
	Char	filename[255];
	Char	logmsg[1024];
	if(tros.fplog){
		FileClose(tros.fplog);
	}	
	if(newfile==NULL){
		struct tm mytime;
		time_t	  t;
		newfile=filename;
	
		t=time(NULL);	
		localtime_r(&t,&mytime);
		sprintf(filename,"usp_get_trace_data.%04d-%02d-%02d_%02d:%02d:%02d",
			mytime.tm_year+1900,mytime.tm_mon+1,mytime.tm_mday,
			mytime.tm_hour,mytime.tm_min,mytime.tm_sec);
	}
	tros.fplog=FileOpen(newfile,"w");
	if(tros.fplog){
		sprintf(logmsg,"Starting logging requests to new file <%s>",newfile);
		os_ErrPost(NULL,NULL,logmsg,0,0,CS_TRUE,0);
		return TRUE;
	} else {
		sprintf(logmsg,"Failed to open file <%s>, request logging is disabled",newfile);
		os_ErrPost(NULL,NULL,logmsg,0,0,CS_TRUE,0);
		return FALSE;
	}
}

Boolean
TraceOS_log_request(unsigned int ti_first,unsigned int ti_last,int outfmt)
{
	CS_INT	intinfo;
	struct tm mytime;
	time_t    t;

	if(tros.fplog){

		t=time(NULL);
		localtime_r(&t,&mytime);

		srv_lockmutex(tros.log_mutex,SRV_M_WAIT,&intinfo);
		fprintf(tros.fplog,"%02d/%02d/%04d %02d:%02d,%d,%d,%d\n",
				mytime.tm_mon+1,mytime.tm_mday, mytime.tm_year+1900,mytime.tm_hour,mytime.tm_min,
				ti_first,ti_last,outfmt);
		srv_unlockmutex(tros.log_mutex);
	}	
	return TRUE;
}



CS_RETCODE
TraceOS_show_pools(SRV_PROC PNTR spp)
{
        CS_INT          status = SRV_DONE_FINAL;
        CS_INT          i,j,rowcount=0,connum;
        CS_INT          busy_spid,is_dead,is_blocked;
        CS_INT          numreq;
        CS_CHAR		logmsg[200];
        CS_CHAR         srv[40],type[10];
        UserThreadDataPtr tdatap=NULL;
        CS_DATAFMT      cs_busy_fmt =
                {"busy_spid",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_numreq_fmt =
                {"numreq",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_dead_fmt =
                {"is_dead",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_blkd_fmt =
                {"is_blocked",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
        CS_DATAFMT      cs_srv_fmt =
                {"server",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,15,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_type_fmt =
		 {"type",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,20,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};

        CS_INT          intlen=sizeof(CS_INT),srvlen;
        CS_INT          typelen;
	BalancedServerPtr	bsp;
	ValNodePtr	vnp;
#ifdef BALANCED_CONN_MGR
    SBalancedDbs* Dbs = NULL;
    SBalancedSrvConnection* Conn = NULL;
#endif

        if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        sprintf(logmsg,"OPEN SERVER NAME: %s",tros.osi.srvname);
        os_ErrPost(NULL,spp,logmsg,0,0,CS_TRUE,500);
        if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_srv_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_srv_fmt,(CS_BYTE*)srv,&srvlen,NULL)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,2,&cs_type_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,2,&cs_type_fmt,(CS_BYTE*)type,&typelen,NULL)!=CS_SUCCEED
           || srv_descfmt(spp,CS_SET,SRV_ROWDATA,3,&cs_numreq_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,3,&cs_numreq_fmt,(CS_BYTE*)&numreq,&intlen,NULL)!=CS_SUCCEED
           || srv_descfmt(spp,CS_SET,SRV_ROWDATA,4,&cs_busy_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,4,&cs_busy_fmt,(CS_BYTE*)&busy_spid,&intlen,NULL)!=CS_SUCCEED
           || srv_descfmt(spp,CS_SET,SRV_ROWDATA,5,&cs_dead_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,5,&cs_dead_fmt,(CS_BYTE*)&is_dead,&intlen,NULL)!=CS_SUCCEED
           || srv_descfmt(spp,CS_SET,SRV_ROWDATA,6,&cs_blkd_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,6,&cs_blkd_fmt,(CS_BYTE*)&is_blocked,&intlen,NULL)!=CS_SUCCEED
                ){
                status |= SRV_DONE_ERROR;
                goto DONE;
        }
        /**** First IQ *****/
	if(tros.iqconn.is_online){

		strcpy(type,"IQ");
		typelen=strlen(type);
		for(i=0,connum=0;i<tros.iqconn.numservers;i++){
#ifdef BALANCED_CONN_MGR
            Dbs = tros.iqconn.m_Dbs[i];
            strcpy( srv, Dbs->m_Srv); srvlen = strlen( srv);
            is_blocked = (Dbs->m_Stat.m_IsBlocked ? 1 : 0);
	
            for( j = 0; j < Dbs->m_ConnNum; j++)
            {
                busy_spid = numreq = is_dead = 0;
                Conn = Dbs->m_Connection[j];
                if( Conn == NULL) is_dead = 1;
                else
                {
                    numreq = Conn->m_RequestCount;
                    if (Conn->m_Owner > 0) busy_spid = Conn->m_Owner;
                    else if( Conn->m_Owner == BAL_CONN_DEAD) is_dead = 1;
                }
            
                if( srv_xferdata( spp, CS_SET, SRV_ROWDATA) != CS_SUCCEED)
                {
                    status |= SRV_DONE_ERROR;  goto DONE;
                }
				rowcount++;
			}
#else
          strcpy(srv,tros.iqconn.clu[i].srv);
			srvlen=strlen(srv);
		
			for(j=0; j < tros.iqconn.numconn;j++,connum++){
				busy_spid=tros.iqconn.busy_spid[connum];
				numreq=tros.iqconn.numreq[connum];
				is_dead=tros.iqconn.is_dead[connum];
				is_blocked=tros.iqconn.is_blocked[connum];
				if(srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED){
					status |= SRV_DONE_ERROR;
					goto DONE;
				}
				rowcount++;
			}
#endif
		}
	}
	strcpy(type,"TrMain");
        typelen=strlen(type);
	for(i=0,connum=0;i<tros.trmainconn.numservers;i++){
#ifdef BALANCED_CONN_MGR
            Dbs = tros.trmainconn.m_Dbs[i];
            strcpy( srv, Dbs->m_Srv); srvlen = strlen( srv);
            is_blocked = (Dbs->m_Stat.m_IsBlocked ? 1 : 0);
	
            for( j = 0; j < Dbs->m_ConnNum; j++)
            {
                busy_spid = numreq = is_dead = 0;
                Conn = Dbs->m_Connection[j];
                if( Conn == NULL) is_dead = 1;
                else
                {
                    numreq = Conn->m_RequestCount;
                    if (Conn->m_Owner > 0) busy_spid = Conn->m_Owner;
                    else if( Conn->m_Owner == BAL_CONN_DEAD) is_dead = 1;
                }
            
                if( srv_xferdata( spp, CS_SET, SRV_ROWDATA) != CS_SUCCEED)
                {
                    status |= SRV_DONE_ERROR;  goto DONE;
                }
				rowcount++;
			}
#else
                strcpy(srv,tros.trmainconn.clu[i].srv);
                srvlen=strlen(srv);

                for(j=0; j < tros.trmainconn.numconn;j++,connum++){
                        busy_spid=tros.trmainconn.busy_spid[connum];
                        numreq=tros.trmainconn.numreq[connum];
                        is_dead=tros.trmainconn.is_dead[connum];
                        is_blocked=tros.trmainconn.is_blocked[connum];
                        if(srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED){
                                status |= SRV_DONE_ERROR;
                                goto DONE;
                        }
                        rowcount++;
                }
#endif
        }
	strcpy(type,"TrParts");
        typelen=strlen(type);
	for(vnp=tros.trlocs.vn_bsrv_list;vnp;vnp=vnp->next){
		bsp=(BalancedServerPtr)vnp->data.ptrvalue;
		for(i=0,connum=0;i<bsp->numservers;i++){
#ifdef BALANCED_CONN_MGR
            Dbs = bsp->m_Dbs[i];
            strcpy( srv, Dbs->m_Srv); srvlen = strlen( srv);
            is_blocked = (Dbs->m_Stat.m_IsBlocked ? 1 : 0);
	
            for( j = 0; j < Dbs->m_ConnNum; j++)
            {
                busy_spid = numreq = is_dead = 0;
                Conn = Dbs->m_Connection[j];
                if( Conn == NULL) is_dead = 1;
                else
                {
                    numreq = Conn->m_RequestCount;
                    if (Conn->m_Owner > 0) busy_spid = Conn->m_Owner;
                    else if( Conn->m_Owner == BAL_CONN_DEAD) is_dead = 1;
                }
            
                if( srv_xferdata( spp, CS_SET, SRV_ROWDATA) != CS_SUCCEED)
                {
                    status |= SRV_DONE_ERROR;  goto DONE;
                }
				rowcount++;
			}
#else
			strcpy(srv,bsp->clu[i].srv);
			srvlen=strlen(srv);

			for(j=0; j < bsp->numconn;j++,connum++){
				busy_spid=bsp->busy_spid[connum];
				numreq=bsp->numreq[connum];
				is_dead=bsp->is_dead[connum];
				is_blocked=bsp->is_blocked[connum];
				if(srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED){
					status |= SRV_DONE_ERROR;
					goto DONE;
				}
				rowcount++;
			}
#endif
		}
        }

DONE:
        srv_sendstatus(spp,((status & SRV_DONE_ERROR)>0)?100:0);
        srv_senddone(spp,status |  SRV_DONE_COUNT,CS_TRAN_UNDEFINED ,rowcount);
        srv_thread_props(spp,CS_SET,SRV_T_USTATE,(CS_VOID PNTR)"AWAITING COMMAND",sizeof("AWAITING COMMAND"),NULL);
        return CS_SUCCEED;
}
