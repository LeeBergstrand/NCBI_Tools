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

#include <TraceOSutils.h>
#include <mcheck.h>

/* Global data declarations.                                            */
CS_CHAR *Name = '\0';                   /* Application name.            */
FILE    *Sched_fp = (FILE *)NULL;       /* Scheduling info file pointer.*/
SRV_SERVER PNTR Server_Ptr;
CS_INT		DebugLevel=255;

Args myargs[] = {
        {"Server Name",NULL,NULL,NULL,FALSE,'S',ARG_STRING,0.0,0,NULL},
	{"Primary IQ connection as SERVER:database=login,password\n   SERVER and database are REQURIED!",NULL,
                NULL,NULL,TRUE,'h',ARG_STRING,0.0,0,NULL},
	{"Secondary IQ connection as SERVER:database=login,password\n   SERVER and database are REQURIED!",NULL,
                NULL,NULL,TRUE,'s',ARG_STRING,0.0,0,NULL},
	{"Number of pre-initialized connections per IQ","0",NULL,NULL,TRUE,'c',ARG_INT,0.0,0,NULL},
	{"Trace ASE server connection as SERVER:database=login,password\n   SERVER and database are REQURIED!",NULL,
                NULL,NULL,TRUE,'t',ARG_STRING,0.0,0,NULL},
	{"Number of pre-initialized connections per Trace ASE server","0",NULL,NULL,TRUE,'n',ARG_INT,0.0,0,NULL},
	{"Debug level (the higher, the more messages in the log)","0","0","1024",TRUE,'d',ARG_INT,0.0,0,NULL},
	{"Network Packet Size.","3584",NULL,NULL,TRUE,'p',ARG_INT,0.0,0,NULL},
	{"Maximum number of users","100",NULL,NULL,TRUE,'u',ARG_INT,0.0,0,NULL},
	{"Start logging requests to usp_get_trace_data","F",NULL,NULL,TRUE,'l',ARG_BOOLEAN,0.0,0,NULL},
	{"TraceImageLocation table","TraceImageLocation",NULL,NULL,TRUE,'I',ARG_STRING,0.0,0,NULL},
	{"TraceAltLocation table","TraceAltLocation",NULL,NULL,TRUE,'A',ARG_STRING,0.0,0,NULL},
	{"TraceDataLocation table","TraceDataLocation",NULL,NULL,TRUE,'D',ARG_STRING,0.0,0,NULL},
	{"SRA data location configuration file","TraceOS.cfg",NULL,NULL,TRUE,'C',ARG_STRING,0.0,0,NULL}
};

enum {
	servernamearg=0,
	iqdbarg=1,
	iqdb2arg,
	iqnumconnarg,
	trsrvarg,
	trnumconnarg,
	debugarg,
	packetsizearg,
        numusersarg,
	logreqarg,
	imgtabarg,
	alttabarg,
	datatabarg,
	sracfgarg,
	Numarg
};

TraceOS	tros;

Int2 Main()
{
    CS_CHAR *servername,errstr[200];
    Int4 intinfo;
    
    /*mtrace();*/
    if(!GetArgs("TraceOS",Numarg, myargs)) {
        return 3;
    }

    servername = myargs[servernamearg].strvalue;
    if(AlreadyRunning(servername)) {
        printf("The open server %s is already running\n",servername);
        exit(1);
    }

    memset(&tros, 0, sizeof(tros));
    tros.packetsize = myargs[packetsizearg].intvalue;
    tros.numconn = myargs[trnumconnarg].intvalue;
    tros.numusers = 0;
    tros.restart_is_running = FALSE;
    tros.service_is_sleeping = FALSE;
    tros.debug_level = (KLogLevel)myargs[debugarg].intvalue;
    tros.trlocs.trloc_img_tab = myargs[imgtabarg].strvalue;
    tros.trlocs.trloc_alt_tab = myargs[alttabarg].strvalue;
    tros.trlocs.trloc_data_tab = myargs[datatabarg].strvalue;
    tros.sracfgfile = myargs[sracfgarg].strvalue;
    Name = servername; /* Use the servername */
    DebugLevel = tros.debug_level;

    /* Configure Arguments of Open Server.*/
    if(!os_arg_init(&tros.osi, servername)) {
        return 10;
    }
    {{
        CS_BOOL boolval = CS_TRUE;
        srv_props(tros.osi.context,CS_SET,SRV_S_PREEMPT,(CS_VOID *)&boolval,sizeof(boolval),NULL);
    }}

    if(myargs[numusersarg].intvalue > 0) {
        CS_INT intval = myargs[numusersarg].intvalue;
        srv_props(tros.osi.context,CS_SET,SRV_S_NUMCONNECTIONS,(CS_VOID *)&intval,sizeof(CS_INT),NULL);
    }
    {{
        CS_INT intinfo;
        srv_props(tros.osi.context,CS_GET,SRV_S_TRACEFLAG,&intinfo,sizeof(CS_INT),NULL);
        intinfo |= SRV_TR_ATTN /*|SRV_TR_DEFQUEUE|SRV_TR_EVENT|SRV_TR_TDSDATA*/ ;
        srv_props(tros.osi.context,CS_SET,SRV_S_TRACEFLAG,&intinfo,sizeof(CS_INT),NULL);
    }}
    {{
        sigset_t SignalSetToBlock;
        sigemptyset(&SignalSetToBlock);
        sigaddset(&SignalSetToBlock, SIGALRM);
        sigprocmask(SIG_BLOCK, &SignalSetToBlock, NULL);
    }}

    /* Initialize ct_library */
    if(!(ct_init(tros.osi.context,OS_CS_VERSION))) {
        fprintf(stderr, "%s: Could not perform initialization... Exiting... ", tros.osi.srvname);
        fflush(stderr);
        goto FATAL;
    }

    /* Install the CT-Library error handlers */
    ct_callback(tros.osi.context,NULL,CS_SET,CS_SERVERMSG_CB,(CS_VOID PNTR)os_ct_servermsg_callback);
    ct_callback(tros.osi.context,NULL,CS_SET,CS_CLIENTMSG_CB,(CS_VOID PNTR)os_ct_clientmsg_callback);
    /* Settting number of connecitons */
    intinfo=500;
    ct_config(tros.osi.context,CS_SET,CS_MAX_CONNECT,(CS_VOID PNTR)&intinfo,CS_UNUSED,NULL);
    intinfo=3;
    ct_config(tros.osi.context,CS_SET,CS_LOGIN_TIMEOUT,&intinfo,CS_UNUSED,NULL);

    /********************************/
    /* Initialize Open Server. */
    if (!(tros.osi.serverp = srv_init(NULL,tros.osi.srvname,CS_NULLTERM))) {
        fprintf(stderr, "%s: Could not perform initialization... Exiting... ", tros.osi.srvname);
        fflush(stderr);
        goto FATAL;
    }
    /*** catch CoreLib errors into srv_log *****/
    ErrSetHandler(os_ncbierrorhandler);
    Nlm_CallErrHandlerOnly(TRUE);

    Server_Ptr = tros.osi.serverp;

    if(myargs[iqdbarg].strvalue && myargs[iqnumconnarg].intvalue > 0){
	if(!BalancedServerSetupConnections(&tros.iqconn,
                                       tros.osi.context,
                                       myargs[iqdbarg].strvalue,
                                       myargs[iqdb2arg].strvalue,
                                       myargs[iqnumconnarg].intvalue,
				       tros.packetsize)) {
		return 10;
	}
	ServerListAddService(&tros.iqservers,&tros.iqconn);
	tros.iqconn.is_online = TRUE;
	/* Set up mutexes */
	if(!BalancedServerSetupMutexes(&tros.iqconn)) {
		return 10;
	}
    } else {
	tros.iqconn.is_online = FALSE;
    }

    if(myargs[trsrvarg].strvalue && myargs[trnumconnarg].intvalue > 0){
	/**** Initialize TraceLocationSet *****/
	if(!BalancedServerSetupConnections(&tros.trmainconn,
				       tros.osi.context,
				       myargs[trsrvarg].strvalue,
				       NULL,
				       tros.numconn,
				       tros.packetsize)) {
	return 10;
	}
	tros.trmainconn.is_online = TRUE;
	ServerListAddService(&tros.servers, &tros.trmainconn);
	/* Set up mutexes */
	if(!BalancedServerSetupMutexes(&tros.trmainconn)) {
		return 10;
	}
	/** init Trace Locations **/
#ifdef BALANCED_CONN_MGR
    SBalancedDbs* Dbs = NULL;
    SBalancedSrvConnection* Conn =
        BalancedServerGetConnection( &tros.trmainconn, BAL_CONN_MAIN, &Dbs);
    if( !Conn) return 20;
    Conn->m_CTCmd;
    if( TraceLocationSetInit (NULL, Conn->m_CTCmd, &tros.trlocs) != CS_SUCCEED) return 20;
    /** init Fields Info **/
    if( TraceFieldsInit( NULL, Conn->m_CTCmd, NULL) != CS_SUCCEED) return 20;
#else
	if(TraceLocationSetInit(NULL,tros.trmainconn.ctcmd[0],&tros.trlocs) != CS_SUCCEED) {
		return 20;
	}
	/** init Fields Info **/
	if(TraceFieldsInit(NULL,tros.trmainconn.ctcmd[0],NULL) != CS_SUCCEED) {
		return 20;
	}
#endif
    } else {
	tros.trmainconn.is_online = FALSE;
    }

    if(srv_createmutex("LOGFILE_MUTEX", CS_NULLTERM, &tros.log_mutex) != CS_SUCCEED) {
        return TRUE;
    }
    if(myargs[logreqarg].intvalue) {
        TraceOS_new_request_log(NULL);
    }


    /* Install start handler. It installs other event handlers. */
    if (!srv_handle(NULL, SRV_START,TraceOS_start_handler)) {
        sprintf(errstr,"%s: Failed to install start_handler\n", tros.osi.srvname);
        srv_log(tros.osi.serverp,CS_TRUE,errstr,CS_NULLTERM);
        goto FATAL;
    }

    /* Start up Open Server. */
    if(srv_run((SRV_SERVER *)NULL) == CS_FAIL) {
        sprintf(errstr,"%s: Failed to run the server", tros.osi.srvname);
        srv_log(tros.osi.serverp,CS_TRUE,errstr,CS_NULLTERM);
        goto FATAL;
    }
FATAL:
    return 0;
}
