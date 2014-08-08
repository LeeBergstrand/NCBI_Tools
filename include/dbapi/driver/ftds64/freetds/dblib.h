/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _dblib_h_
#define _dblib_h_

#ifdef __cplusplus
extern "C"
{
#if 0
}
#endif
#endif

/* $Id: dblib.h 103491 2007-05-04 17:18:18Z kazimird $ */

enum {
	  _DB_RES_INIT            = 0
	, _DB_RES_RESULTSET_EMPTY = 1
	, _DB_RES_RESULTSET_ROWS  = 2
	, _DB_RES_NEXT_RESULT     = 3
	, _DB_RES_NO_MORE_RESULTS = 4
	, _DB_RES_SUCCEED         = 5
};

struct tds_dblib_loginrec
{
	TDSLOGIN *tds_login;
};

struct dblib_buffer_row;

typedef struct tag_DBPROC_ROWBUF
{
	int received;	  	/* how many rows have been received for this result set */
	int head;	  	/* queue insertion point */
	int tail;	  	/* oldest item in queue	*/
	int current;		/* dbnextrow() reads this row */
	int capacity;		/* how many elements the queue can hold  */
	struct dblib_buffer_row *rows;		/* pointer to the row storage */
} DBPROC_ROWBUF;

typedef struct
{
	int host_column;
	int datatype;
	int prefix_len;
	DBINT column_len;
	BYTE *terminator;
	int term_len;
	int tab_colnum;
	int column_error;
	BCPCOLDATA *bcp_column_data;
} BCP_HOSTCOLINFO;

typedef struct 
{
	TDS_CHAR *hostfile;
	TDS_CHAR *errorfile;
	FILE *bcp_errfileptr;
	TDS_INT host_colcount;
	BCP_HOSTCOLINFO **host_columns;
	TDS_INT firstrow;
	TDS_INT lastrow;
	TDS_INT maxerrs;
	TDS_INT batch;
} BCP_HOSTFILEINFO;

typedef struct
{
	char *hint;
	TDS_CHAR *tablename;
	TDS_CHAR *insert_stmt;
	TDS_INT direction;
	TDS_INT queryout;
	TDS_INT identity_insert_on;
	TDS_INT xfer_init;
	TDS_INT var_cols;
	TDS_INT bind_count;
	TDSRESULTINFO *bindinfo;
} DB_BCPINFO;
/* linked list of rpc parameters */

typedef struct _DBREMOTE_PROC_PARAM
{
	struct _DBREMOTE_PROC_PARAM *next;

	char *name;
	BYTE status;
	int type;
	DBINT maxlen;
	DBINT datalen;
	BYTE *value;
} DBREMOTE_PROC_PARAM;

typedef struct _DBREMOTE_PROC
{
	struct _DBREMOTE_PROC *next;

	char *name;
	DBSMALLINT options;
	DBREMOTE_PROC_PARAM *param_list;
} DBREMOTE_PROC;

struct tds_dblib_dbprocess
{
	TDSSOCKET *tds_socket;

	TDS_INT row_type;
	DBPROC_ROWBUF row_buf;

	int noautofree;
	int more_results;	/* boolean.  Are we expecting results? */
	int dbresults_state;
	int dbresults_retcode;
	BYTE *user_data;	/* see dbsetuserdata() and dbgetuserdata() */
	unsigned char *dbbuf;	/* is dynamic!                   */
	int dbbufsz;
	int command_state;
	TDS_INT text_size;
	TDS_INT text_sent;
	DBTYPEINFO typeinfo;
	unsigned char avail_flag;
	DBOPTION *dbopts;
	DBSTRING *dboptcmd;
	BCP_HOSTFILEINFO *hostfileinfo;
	DB_BCPINFO *bcpinfo;
	DBREMOTE_PROC *rpc;
	DBUSMALLINT envchange_rcv;
	char dbcurdb[DBMAXNAME + 1];
	char servcharset[DBMAXNAME + 1];
	FILE *ftos;
	DB_DBCHKINTR_FUNC dbchkintr;
	DB_DBHNDLINTR_FUNC dbhndlintr;
	
	/** boolean use ms behaviour */
	int msdblib;

	int ntimeouts;
};

#define DBLIB_INFO_MSG_TYPE 0
#define DBLIB_ERROR_MSG_TYPE 1

/*
 * internal prototypes
 */
int dbperror (DBPROCESS *dbproc, DBINT msgno, int errnum);
int _dblib_handle_info_message(const TDSCONTEXT * ctxptr, TDSSOCKET * tdsptr, TDSMESSAGE* msgptr);
int _dblib_handle_err_message(const TDSCONTEXT * ctxptr, TDSSOCKET * tdsptr, TDSMESSAGE* msgptr);
int _dblib_client_msg(DBPROCESS * dbproc, int dberr, int severity, const char *dberrstr);
void _dblib_setTDS_version(TDSLOGIN * tds_login, DBINT version);

DBINT _convert_char(int srctype, BYTE * src, int destype, BYTE * dest, DBINT destlen);
DBINT _convert_intn(int srctype, BYTE * src, int destype, BYTE * dest, DBINT destlen);

RETCODE _bcp_clear_storage(DBPROCESS * dbproc);
RETCODE _bcp_get_prog_data(DBPROCESS * dbproc);

extern MHANDLEFUNC _dblib_msg_handler;
extern EHANDLEFUNC _dblib_err_handler;

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
