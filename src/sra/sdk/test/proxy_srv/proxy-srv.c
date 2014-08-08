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
#include <kapp/main.h>
#include <kapp/args.h>
#include <klib/out.h>
#include <klib/log.h>
#include <klib/text.h>
#include <klib/namelist.h>
#include <klib/time.h>
#include <klib/container.h>
#include <klib/rc.h>
#include <sysalloc.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <conio.h>

#include <common/test_assert.h>

#define OPTION_PORT     "port"
#define OPTION_SIGPORT  "signal_port"

#define DEFAULT_SERVER_PORT 20000
#define BUFSIZE 2048
#define DEF_TIMEOUT 30000


/***************************************************************************
    key-value-tree ( for environment )
***************************************************************************/

typedef struct key_value_pair
{
    BSTNode node;
    size_t len;
    char * full_line;
    char * key;
    char * value;
} key_value_pair;


static int cmp_pchar( const char * a, const char * b )
{
    int res = 0;
    if ( ( a != NULL )&&( b != NULL ) )
    {
        size_t len_a = string_size( a );
        size_t len_b = string_size( b );
        res = string_cmp ( a, len_a, b, len_b, ( len_a < len_b ) ? len_b : len_a );
    }
    return res;
}


static key_value_pair * make_key_value_pair( const char * s )
{
    key_value_pair *res = calloc( sizeof *res, 1 );
    if ( res != NULL )
    {
        char * eq;
        res->len = string_size( s );
        res->full_line = string_dup( s, res->len );
        res->key = string_dup( s, res->len );
        eq = string_chr( res->key, res->len, '=' );
        if ( eq != NULL )
        {
            res->value = ( eq + 1 );
            *eq = 0;
        }
    }
    return res;
}


static void free_key_value_pair( key_value_pair * kvp )
{
    if ( kvp != NULL )
    {
        if ( kvp->full_line != NULL )
            free( kvp->full_line );
        if ( kvp->key != NULL )
            free( kvp->key );
        free( kvp );
    }
}


static void CC free_kvp_callback( BSTNode *n, void * data )
{
    key_value_pair * kvp = ( key_value_pair * )n;
    free_key_value_pair( kvp );
}


static void free_kvp_tree( BSTree * tree )
{
    BSTreeWhack( tree, free_kvp_callback, NULL );
}

static int CC kvpair_vs_pchar_wrapper( const void *item, const BSTNode *n )
{
    const key_value_pair * kvp = ( const key_value_pair * )n;
    return cmp_pchar( (const char *)item, kvp->key );
}


static key_value_pair * find_key_value_pair( BSTree * kvp_tree, const char * key )
{
    KOutMsg( "looking for >%s<\n", key );
    return ( key_value_pair * ) BSTreeFind( kvp_tree, key, kvpair_vs_pchar_wrapper );
}


static int CC kvp_vs_kvp_wrapper( const BSTNode *item, const BSTNode *n )
{
   const key_value_pair * a = ( const key_value_pair * )item;
   const key_value_pair * b = ( const key_value_pair * )n;
   return cmp_pchar( a->key, b->key );
}


static rc_t add_kvp( BSTree * kvp_tree, const char * s )
{
    rc_t rc = 0;
    key_value_pair * kvp = make_key_value_pair( s );
    if ( kvp != NULL )
    {
        rc = BSTreeInsert( kvp_tree, (BSTNode *)kvp, kvp_vs_kvp_wrapper );
        if ( rc != 0 )
            free_key_value_pair( kvp );
    }
    else
        rc = RC( rcApp, rcNoTarg, rcConstructing, rcMemory, rcExhausted );
    return rc;
}


static void CC merge_callback( BSTNode *n, void *data )
{   
    key_value_pair * replacement = ( key_value_pair * )n;
    BSTree * org_tree = ( BSTree * )data;
    key_value_pair * found = find_key_value_pair( org_tree, replacement->key );
    if ( found != NULL )
    {
        /* the key of the new key-value-pair WAS FOUND in the original tree ---> replace it */
        /* 1 ... remove org entry */ BSTreeUnlink ( org_tree, (BSTNode *)found );
        /* 2 ... free org entry   */ free_key_value_pair( found );
    }
    add_kvp( org_tree, replacement->full_line );
}


static void merge_kvp( BSTree * org_tree, BSTree * new_values )
{
    BSTreeForEach( new_values, false, merge_callback, org_tree );
}


static void env_2_kvp_tree( LPTCH env, BSTree *tree )
{
    LPTSTR name_value = ( LPTSTR )env;
    while ( *name_value )
    {
        size_t l = string_size( ( const char * )name_value ) + 1;
        add_kvp( tree, ( const char * )name_value );
        name_value += l;
    }
}


static void namelist_2_kvp_tree( const VNamelist * nl, BSTree *tree )
{
    uint32_t count;
    rc_t rc = VNameListCount( nl, &count );
    if ( rc == 0 && count > 0 )
    {
        uint32_t idx;
        for ( idx = 0; idx < count && rc == 0; ++idx )
        {
            const char *s;
            rc = VNameListGet( nl, idx, &s );
            if ( rc == 0 && s != NULL )
                add_kvp( tree, s );
        }
    }
}


static void CC count_callback( BSTNode *n, void *data )
{   
    key_value_pair * kvp = ( key_value_pair * )n;
    size_t * len = ( size_t * )data;
    ( *len ) += ( kvp->len + 1 );
}

typedef struct fill_ctx
{
    wchar_t *dst;
    size_t buffersize_in_words;
} fill_ctx;


static void CC fill_callback( BSTNode *n, void *data )
{
    key_value_pair * kvp = ( key_value_pair * )n;
    fill_ctx * ctx = ( fill_ctx * )data;
    size_t chars_copied;

    mbstowcs_s( &chars_copied, ctx->dst, ctx->buffersize_in_words, kvp->full_line, kvp->len );
    ctx->dst += chars_copied;
}


static wchar_t * kvp_tree_to_env( BSTree *tree )
{
    wchar_t *res = NULL;
    size_t len = 0;

    BSTreeForEach( tree, false, count_callback, &len );
    res = malloc( ( len + 2 ) * 2 );
    if ( res != NULL )
    {
        fill_ctx ctx;
        ctx.dst = res;
        ctx.buffersize_in_words = ( len + 2 );
        BSTreeForEach( tree, false, fill_callback, &ctx );
        *( ctx.dst ) = 0; /* final termination */
    }
    return res;
}

/***************************************************************************
    global structures
***************************************************************************/

/* global server-context ( cmd-line params )*/
typedef struct server_context
{
    uint16_t server_port;
    uint16_t signal_port;
    
    SOCKET server_socket;
    SOCKET signal_socket;
} server_context;
typedef server_context* server_context_ptr;

/* client context ( local to client-thread )*/
typedef struct client_context {
    char *path;
    char *cmd;
    VNamelist *tokens;
    VNamelist *env;
    DWORD exit_code;
    SOCKET client_socket;
} client_context;
typedef client_context* client_context_ptr;

/* context for pipe-rd-send-worker-threads */
typedef struct pipe_context {
    SOCKET client_socket;
    HANDLE RdPipe;
} pipe_context;
typedef pipe_context* pipe_context_ptr;

/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
uint32_t CC KAppVersion ( void )
{
    return 0x01010001;
}

rc_t CC UsageSummary (const char * progname)
{
    KOutMsg( "******************************************************************************\n"
             " proxy-srv:\n"
             "******************************************************************************\n" );
    KOutMsg( "usage:\n"
             "    proxy-srv --port 20000\n"
             "    port ... port to listen at\n"
             "example: --port 20000\n"
             "******************************************************************************\n" );
    return 0;
}

const char UsageDefaultName[] = "proxy-srv";

rc_t CC Usage ( const Args * args )
{
    UsageSummary ( UsageDefaultName );
    KOutMsg( "options:\n"
             "--port   -P ... ip-port the server is listening for (default 20000)\n"
             "******************************************************************************\n" );
    return 0;
}

static const char* get_str_option( const Args *my_args, const char *name )
{
    const char* res = NULL;
    uint32_t count;
    rc_t rc = ArgsOptionCount( my_args, name, &count );
    if ( ( rc == 0 )&&( count > 0 ) )
    {
        ArgsOptionValue( my_args, name, 0, &res );
    }
    return res;
}

static uint16_t get_uint16_option( const Args *my_args,
                                   const char *name,
                                   const uint16_t def )
{
    uint16_t res = def;
    uint32_t count;
    rc_t rc = ArgsOptionCount( my_args, name, &count );
    if ( ( rc == 0 )&&( count > 0 ) )
    {
        const char *s;
        rc = ArgsOptionValue( my_args, name, 0,  &s );
        if ( rc == 0 ) res = atoi( s );
    }
    return res;
}

static rc_t setup_server_context( const Args * args, server_context_ptr ctx )
{
    if ( ctx == NULL ) return 1;
    
    ctx->server_port = get_uint16_option( args, OPTION_PORT, DEFAULT_SERVER_PORT );
    ctx->signal_port = get_uint16_option( args, OPTION_SIGPORT, ctx->server_port + 1 );
    
    return 0;
}

OptDef ServerOptions[] =
{
    { OPTION_PORT,   "P", NULL, (const char**)&"listen-port of the server", 1, true, false },
    { OPTION_SIGPORT,"S", NULL, (const char**)&"listen-port for exit signal", 1, true, false }
};

static BOOL open_server_socket( SOCKET *server_socket, const int port, bool bind_local )
{
    struct sockaddr_in local;

    local.sin_family = AF_INET;
    if ( bind_local )
        local.sin_addr.s_addr = htonl( 0x7F000001 ); /* 127.0.0.1 */
    else
        local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons( (u_short)port );

    *server_socket = socket( AF_INET,SOCK_STREAM, 0 );
    if ( *server_socket == INVALID_SOCKET )
    {
        return FALSE;
    }

    if ( bind( *server_socket, (struct sockaddr*)&local, sizeof(local) ) != 0 )
    {
        return FALSE;
    }

    if ( listen ( *server_socket, 10 ) != 0 )
    {
        return FALSE;
    }

    return TRUE;
}

static void connect_to_signal_socket( const int port, bool local )
{
    struct sockaddr_in srv;
    struct hostent * localHost;
    char * localIP;

    SOCKET s = socket( AF_INET,SOCK_STREAM, 0 );
    if ( s == INVALID_SOCKET )
    {
        return;
    }

    memset ( &srv, 0, sizeof ( srv ) );
    srv.sin_family = AF_INET;
    srv.sin_port = htons( port );
    if ( local )
        srv.sin_addr.s_addr = htonl( 0x7F000001 ); /* 127.0.0.1 */
    else
    {
        localHost = gethostbyname( "" );
        localIP = inet_ntoa( *(struct in_addr*)*localHost->h_addr_list);
        srv.sin_addr.s_addr = inet_addr( localIP );
    }
    
    connect( s, (struct sockaddr *) &srv, sizeof( srv ) );
    closesocket( s );
}

static void send_number( SOCKET client_socket, const char c, const int number )
{
    char buff[ 25 ];
    size_t l = sprintf_s( buff, sizeof( buff ) - 1, "%c%d\n", c, number );
    send ( client_socket, buff, (int)l, 0 );
}

/***************************************************************************
    start windows-specific stuff
***************************************************************************/
static HANDLE create_simple_thread( LPTHREAD_START_ROUTINE func, LPVOID parameter )
{
    return CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            func,                   // thread function
            parameter,              // argument to thread function 
            0,                      // use default creation flags 
            NULL );                 // returns the thread identifier 
}

static BOOL create_pipe_pair( HANDLE * Pipe_Rd, HANDLE * Pipe_Wr )
{
	SECURITY_ATTRIBUTES saAttr;
 
	// Set the bInheritHandle flag so pipe handles are inherited. 
	saAttr.nLength = sizeof( SECURITY_ATTRIBUTES ); 
	saAttr.bInheritHandle = true; 
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
   if ( !CreatePipe( Pipe_Rd, Pipe_Wr, &saAttr, 0 ) ) 
      return FALSE;

	// Ensure the read handle to the pipe for STDOUT is not inherited.
   if ( !SetHandleInformation( *Pipe_Rd, HANDLE_FLAG_INHERIT, 0 ) )
      return FALSE;

   return TRUE;
}
/***************************************************************************
    end windows-specific stuff
***************************************************************************/

static void init_client_context( client_context_ptr ctx, SOCKET client_socket )
{
    if ( ctx == NULL ) return;
    ctx->path = NULL;
    ctx->cmd = NULL;
    VNamelistMake ( &(ctx->tokens), 5 );
    VNamelistMake ( &(ctx->env), 5 );

    ctx->exit_code = 0;
    ctx->client_socket = client_socket;
}

static void free_client_context( client_context_ptr ctx )
{
    if ( ctx )
    {
        if ( ctx->path ) free( ctx->path );
        if ( ctx->cmd ) free( ctx->cmd );
        VNamelistRelease ( ctx->tokens );
        VNamelistRelease ( ctx->env );
     }
}

static void ReadFromPipeUntilClosed( HANDLE pipe_handle, SOCKET client_socket, const char pipechar )
{
    BOOL res = TRUE;

    while ( res )
    {
        DWORD dwRead;
        char buffer[ 2048 ];

        res = ReadFile( pipe_handle, &(buffer[6]), (DWORD)( sizeof( buffer ) - 6 ), &dwRead, NULL );
        if ( dwRead > 0 )
        {
            char num_buff[ 16 ];
            size_t num_len = sprintf_s( num_buff, sizeof( num_buff ) - 1, "%c%.04d\n", pipechar, dwRead );
            memcpy( buffer, num_buff, 6 );
            send ( client_socket, buffer, dwRead+6, 0 );
        }
    }
    CloseHandle( pipe_handle );
}

DWORD WINAPI StdOutPipeThread ( LPVOID pParam )
{
    pipe_context_ptr ctx = (pipe_context_ptr)pParam;
    ReadFromPipeUntilClosed( ctx->RdPipe, ctx->client_socket, 'S' );
    return 0;
}

DWORD WINAPI StdErrPipeThread ( LPVOID pParam )
{
    pipe_context_ptr ctx = (pipe_context_ptr)pParam;
    ReadFromPipeUntilClosed( ctx->RdPipe, ctx->client_socket, 'E' );
    return 0;
}

static wchar_t* convert_into_full_path(const char* appname, size_t len)
{
    char name[4096];
    char buf[4096];
    DWORD full_len;
    wchar_t* fullname;

    strncpy(name, appname, len);
    name[len]=0;
    full_len = SearchPath(NULL, name, ".exe", sizeof(buf), buf, NULL);
    if ( full_len == 0 )
    {
        KOutMsg( "error %d searching path for '%s'\n", GetLastError(), name );
        return NULL;
    }
    fullname = malloc( ( full_len + 1 ) * 2 );
    if (fullname == NULL)
    {
        KOutMsg( "cannot allocate full app name\n" );
    }
    else
    {
        size_t chars_copied;
        mbstowcs_s ( &chars_copied, fullname, full_len + 1, buf, full_len );
    }
    return fullname;
}

static BOOL prepare_command(client_context_ptr ctx, wchar_t** wapp, wchar_t** wparams, wchar_t** wpath)
{
    size_t chars_copied;
    size_t appLen;
    size_t w_len;

    const char* space=strchr(ctx->cmd, ' ');

    *wapp=NULL;
    *wparams=NULL;
    *wpath=NULL;

    if ( space != NULL ) 
    {
        appLen = space - ctx->cmd;
    }
    else
    {
        appLen = strlen(ctx->cmd);
    }
    *wapp = convert_into_full_path( ctx->cmd, appLen );
    if (*wapp == NULL)
    {
        return FALSE;
    }

    w_len = strlen( ctx->cmd );
    *wparams = malloc( ( w_len + 1 ) * 2 );
    if ( *wparams == NULL )
    {
        KOutMsg( "cannot allocate cmdline\n" );
        free( *wapp );
        *wapp=NULL;
        return FALSE;
    }
    mbstowcs_s ( &chars_copied, *wparams, w_len + 1, ctx->cmd, w_len );

    if ( ctx->path && ctx->path[ 0 ] != 0 )
    {
        BOOL ok=FALSE;
        size_t w_len = strlen( ctx->path );
        int i;
        DWORD file_attrs;
        for ( i=0; i < 20; ++i )
        {
            file_attrs = GetFileAttributes(ctx->path);
            if ( file_attrs == INVALID_FILE_ATTRIBUTES )
            {
                KOutMsg( "waiting for the path to become available: (%d)'%s'...\n", w_len, ctx->path );
                Sleep( 500 );
            }
            else
            {
                ok=TRUE;
                break;
            }
        }
        if ( file_attrs == INVALID_FILE_ATTRIBUTES )
        {
            KOutMsg( "path does not exist: (%d)'%s'\n", w_len, ctx->path );
            ok=FALSE;
        }
        else if ( !( file_attrs & FILE_ATTRIBUTE_DIRECTORY ) )
        {
            KOutMsg( "not a directory: (%d)'%s'\n", w_len, ctx->path );
            ok=FALSE;
        }
        else
        {
            KOutMsg( "path=(%d)'%s'\n", w_len, ctx->path );
            *wpath = malloc( ( w_len + 1 ) * 2 );
            if ( *wpath == NULL )
            {
                KOutMsg( "cannot allocate path\n" );
                ok=FALSE;
            }
        }
        if (!ok)
        {
            free( *wapp );
            free( *wparams );
            *wapp=NULL;
            *wparams=NULL;
            return FALSE;
        }
        mbstowcs_s ( &chars_copied, *wpath, w_len + 1, ctx->path, w_len );
    }
    return TRUE;
}


static size_t count_env( LPWCH wenv )
{
    size_t res = 0;
    LPWSTR name_value = ( LPWSTR )wenv;
    while ( *name_value )
    {
        size_t l = wcslen( name_value ) + 1;
        res += l;
        name_value += l;
    }
    return res + 1;
}


static size_t count_to_add( const VNamelist *to_add )
{
    size_t res = 0;
    uint32_t count;
    rc_t rc = VNameListCount( to_add, &count );
    if ( rc == 0 && count > 0 )
    {
        uint32_t idx;
        for ( idx = 0; idx < count && rc == 0; ++idx )
        {
            const char *s;
            rc = VNameListGet( to_add, idx, &s );
            if ( rc == 0 && s != NULL )
                res += ( strlen( s ) + 1 );
        }
    }
    return res;
}


static wchar_t * make_new_env( LPWCH wenv, const VNamelist *to_add )
{
    size_t w_size = count_env( wenv ) + count_to_add( to_add ) + 2;
    size_t bytes_needed = w_size * 2;
    wchar_t * res = malloc( bytes_needed );
    if ( res != NULL )
    {
        rc_t rc;
        uint32_t count;
        wchar_t * dst = res;

        LPWSTR name_value = ( LPWSTR )wenv;
        while ( *name_value )
        {
            size_t l = wcslen( name_value ) + 1;
            memcpy( dst, name_value, l * 2 );
            dst += l;
            name_value += l;
            w_size -= l;
        }

        rc = VNameListCount( to_add, &count );
        if ( rc == 0 && count > 0 )
        {
            uint32_t idx;
            for ( idx = 0; idx < count && rc == 0; ++idx )
            {
                const char *s;
                rc = VNameListGet( to_add, idx, &s );
                if ( rc == 0 && s != NULL )
                {
                    size_t chars_copied;
                    size_t w_len = strlen( s );
                    mbstowcs_s( &chars_copied, dst, w_size, s, w_len );
                    dst += chars_copied;
                    w_size -= chars_copied;
                }
            }
            *dst = 0; /* final double termination for env-block */
        }
    }
    return res;
}


static wchar_t * merge_env_1( client_context_ptr ctx )
{
    wchar_t * res = NULL;
    uint32_t count;
    rc_t rc = VNameListCount( ctx->env, &count );
    if ( rc == 0 && count > 0 )
    {
        LPWCH wenv = GetEnvironmentStringsW();  /* get copy of current env. */
        res = make_new_env( wenv, ctx->env ); /* mix */
        FreeEnvironmentStringsW( wenv ); /* free copy of current env. */
    }
    return res;
}


static wchar_t * merge_env_2( client_context_ptr ctx )
{
    wchar_t * res = NULL;
    uint32_t count;
    rc_t rc = VNameListCount( ctx->env, &count );
    if ( rc == 0 && count > 0 )
    {
        BSTree org_tree, new_tree;

        LPTCH env = GetEnvironmentStrings();  /* get copy of current env. */
        BSTreeInit( &org_tree );
        BSTreeInit( &new_tree );

        env_2_kvp_tree( env, &org_tree );
        namelist_2_kvp_tree( ctx->env, &new_tree );
        merge_kvp( &org_tree, &new_tree );

        res = kvp_tree_to_env( &org_tree );

        free_kvp_tree( &org_tree );
        free_kvp_tree( &new_tree );
        FreeEnvironmentStrings( env ); /* free copy of current env. */
    }
    return res;
}


static BOOL run_command( client_context_ptr ctx )
{
    BOOL res;
    wchar_t * wapp = NULL;
    wchar_t * wparams = NULL;
    wchar_t * wpath = NULL;
    wchar_t * wnew_env = NULL;
    HANDLE StdOutWr = NULL;
    HANDLE StdErrWr = NULL;
    pipe_context StdOut_ctx;
    pipe_context StdErr_ctx;
    PROCESS_INFORMATION process_info; 
    STARTUPINFO startup_info;
    HANDLE StdOutThreadHandle, StdErrThreadHandle;
    DWORD create_flags = 0;

    res = prepare_command( ctx, &wapp, &wparams, &wpath );
    if ( !res )
    {
        return res;
    }

    res = create_pipe_pair( &(StdOut_ctx.RdPipe), &StdOutWr );
    if ( !res )
    {
        KOutMsg( "cannot create stdout-pipes\n" );
        return res;
    }

    res = create_pipe_pair( &(StdErr_ctx.RdPipe), &StdErrWr );
    if ( !res )
    {
        KOutMsg( "cannot create stderr-pipes\n" );
        return res;
    }

    wnew_env = merge_env_2( ctx );
    if ( wnew_env != NULL )
        create_flags |= CREATE_UNICODE_ENVIRONMENT;

    StdOut_ctx.client_socket = ctx->client_socket;
    StdErr_ctx.client_socket = ctx->client_socket;

    ZeroMemory( &process_info, sizeof( PROCESS_INFORMATION ) );
    ZeroMemory( &startup_info, sizeof( STARTUPINFO ) );
    startup_info.cb = sizeof(STARTUPINFO);
    startup_info.hStdError  = StdErrWr;
    startup_info.hStdOutput = StdOutWr;
    startup_info.hStdInput  = NULL;
    startup_info.dwFlags   |= STARTF_USESTDHANDLES;

    StdOutThreadHandle = create_simple_thread( StdOutPipeThread, &StdOut_ctx );
    StdErrThreadHandle = create_simple_thread( StdErrPipeThread, &StdErr_ctx );

    res = CreateProcessW( wapp,
                        wparams,            // command-line
                        NULL,               // process security attributes
                        NULL,               // primary thread security attributes
                        true,               // handles are inherited
                        create_flags,       // creation flags
                        wnew_env,           // use this environment
                        (LPCWSTR)wpath,     // use parent's current directory or the supplied one
                        (LPSTARTUPINFOW)&startup_info,  // STARTUPINFO pointer
                        &process_info       // receives PROCESS_INFORMATION
                        );
    if ( res )
    {
        CloseHandle( StdOutWr );
        CloseHandle( StdErrWr );

        WaitForSingleObject( process_info.hProcess, INFINITE );
        WaitForSingleObject( StdOutThreadHandle, INFINITE);
        WaitForSingleObject( StdErrThreadHandle, INFINITE);

        GetExitCodeProcess( process_info.hProcess, &(ctx->exit_code) );
        CloseHandle( process_info.hProcess );
        CloseHandle( process_info.hThread );
    }
    else
    {
        KOutMsg( "error executing command: %d\n", GetLastError() );
        CloseHandle( StdOutWr );
        CloseHandle( StdErrWr );
        CloseHandle( StdOut_ctx.RdPipe );
        CloseHandle( StdErr_ctx.RdPipe );
    }

    free( wnew_env );
    free( wapp );
    free( wparams );
    free( wpath );

    return res;
}

#define RX_HEADER 1
#define RX_BODY 2
#define RX_READY 3

typedef struct rx_context
{
    client_context_ptr ctx;
    char func;
    char *temp;
    size_t temp_alloc;
    size_t body_n;
    size_t body_rx;
    size_t idx;
    size_t mode;
} rx_context;
typedef rx_context* rx_context_ptr;

static void init_rx_context( rx_context_ptr rx_ctx, client_context_ptr cl_ctx )
{
    if ( rx_ctx ) 
    {
        rx_ctx->ctx = cl_ctx;
        rx_ctx->mode = RX_HEADER;
        rx_ctx->idx = 0;
        rx_ctx->body_rx = 0;
        rx_ctx->body_n = 0;
        rx_ctx->temp_alloc = BUFSIZE;
        rx_ctx->temp = malloc( rx_ctx->temp_alloc );
    }
}

static void free_rx_context( rx_context_ptr rx_ctx )
{
    if ( rx_ctx ) 
    {
        if ( rx_ctx->temp ) free( rx_ctx->temp );
    }
}

static void put_into_temp( rx_context_ptr ctx, const char c )
{
    if ( ctx->idx >= ctx->temp_alloc-1 )
    {
        ctx->temp_alloc += BUFSIZE;
        ctx->temp = realloc( ctx->temp, ctx->temp_alloc );
    }
    ctx->temp[ ctx->idx++ ] = c;
    ctx->body_rx++;
}

static void receive_header( rx_context_ptr ctx, const char c )
{
    if ( c == '\n' )
    {
        /* the header got terminated... */
        ctx->func  = ctx->temp[ 0 ]; /* X=ready, P=path, C=command */
        ctx->temp[ ctx->idx ] = 0; /* terminate as c-string */
        if ( ctx->idx > 1 )
        {
            ctx->body_n = atoi( &(ctx->temp[1]) ); /* convert to int */
        }
        else
        {
            ctx->body_n = 0;
        }
        
        if ( ctx->func == 'X' )
            ctx->mode = RX_READY;

        /* if it was the header of a body > 0 */
        if ( ctx->body_n > 0 && ctx->mode == RX_HEADER )
        {
            ctx->mode = RX_BODY;
        }

        ctx->idx = 0;
        ctx->body_rx = 0;
    }
    else
    {
        put_into_temp( ctx, c );
    }
}

static void receive_body( rx_context_ptr ctx, const char c )
{
    put_into_temp( ctx, c );
    if ( ctx->body_rx >= ctx->body_n )
    {
        /* the last char has been received */
        ctx->temp[ ctx->idx ] = 0; /* terminate the string part... */
        /* depending on the function put the body into client-ctx-buffers */
        switch( ctx->func )
        {
            case 'P' : ctx->ctx->path = string_dup_measure ( ctx->temp, NULL ); break;
            case 'C' : ctx->ctx->cmd = string_dup_measure( ctx->temp, NULL ); break;
            case 'T' : VNamelistAppend ( ctx->ctx->tokens, ctx->temp ); break;
            case 'E' : VNamelistAppend ( ctx->ctx->env, ctx->temp ); break;
        }
        ctx->idx = 0;
        ctx->mode = RX_HEADER;
    }
}

static rc_t receive_request_from_client( const int sock, client_context_ptr ctx )
{
    rx_context rx_ctx;
    rc_t rc = 0;
    
    init_rx_context( &rx_ctx, ctx );
    while ( rx_ctx.mode != RX_READY )
    {
        char buffer[ BUFSIZE ];
        int received = recv( sock, buffer, sizeof( buffer )-1, 0 );
        if ( received > 0 )
        {
            uint16_t src_idx = 0;
            while ( src_idx < received && rx_ctx.mode != RX_READY )
            {
                char c = buffer[ src_idx++ ];
                switch( rx_ctx.mode )
                {
                case RX_HEADER : receive_header( &rx_ctx, c ); break;
                case RX_BODY   : receive_body( &rx_ctx, c ); break;
                }
            }
        }
        else if ( received < 0 )
        {
        rx_ctx.mode = RX_READY;
        rc = 1;
        }
    }
    free_rx_context( &rx_ctx );
    return rc;
}

DWORD WINAPI client_thread( LPVOID pParam )
{
    SOCKET client_socket;
    client_context ctx;

    if ( pParam == NULL )
    {
        KOutMsg( "client-thread: param is NULL, exit\n" );
        return 0;
    }

    client_socket = *(SOCKET*)pParam;
    init_client_context( &ctx, client_socket );
    if ( receive_request_from_client( client_socket, &ctx ) == 0 )
    {
        if ( strlen( ctx.cmd ) > 0 )
        {
            KTime kt;
            KOutMsg( "%T: running (%d)'%s'\n", KTimeLocal (&kt, KTimeStamp()), strlen(ctx.cmd), ctx.cmd );
            if ( run_command( &ctx ) )
            {
                KOutMsg( "success! ( exit-code = %d )\n", ctx.exit_code );
            }
            else
            {
                KOutMsg( "failed \n" );
                ctx.exit_code = 1;
            }
            send_number( client_socket, 'X', ctx.exit_code );
        }
    }
    else
    {
        KOutMsg( "error receiving client-request!\n" );
    }

    free_client_context( &ctx );
    shutdown( client_socket, SD_BOTH );
    return 0;
}

DWORD WINAPI server_thread ( LPVOID pParam )
{
    server_context_ptr ctx;
    BOOL running = TRUE;

    if ( pParam == NULL )
    {
        KOutMsg( "server-thread: param is NULL, exit\n" );
        return 0;
    }
    
    ctx = (server_context_ptr)pParam;
    KOutMsg( "Starting up proxy-execute-server at port %d\n", ctx->server_port );

    while ( running )
    {
        struct sockaddr_in from;
        int fromlen = sizeof( from );
        int read_sockets;
        fd_set my_set;

        FD_ZERO( &my_set );
        FD_SET( ctx->server_socket, &my_set );
        FD_SET( ctx->signal_socket, &my_set );

        read_sockets = select( 1, &my_set, NULL, NULL, NULL );
        if ( read_sockets > 0 )
        {
            if ( FD_ISSET( ctx->signal_socket, &my_set ) )
            {
                running = FALSE;
                KOutMsg( "\nserver thread terminated\n" );
            }
            else if ( FD_ISSET( ctx->server_socket, &my_set ) )
            {
                SOCKET client_socket = accept ( ctx->server_socket, 
                                                ( struct sockaddr* )&from, &fromlen );
                KOutMsg( "\nConnection from %s\n", inet_ntoa( from.sin_addr ) );

                /* we do not store the handle, we are not waiting for this thread... */
                create_simple_thread( client_thread, &client_socket );
            }
        }
    }

    closesocket( ctx->server_socket );
    closesocket( ctx->signal_socket );
    return 0;
}

DWORD WINAPI cancel_thread ( LPVOID pParam )
{
    int sig_port = 0;

    if ( pParam != NULL )
    {
        sig_port = *(int*)pParam;
    }

    KOutMsg( "Press ESCAPE to terminate program\n" );

    /* waiting for a ESC-key-press */
    while ( _getch() != 27 ) Sleep(100);

    /* connecting to the server-signal-socket ends the server-thread */
    if ( sig_port != 0 )
    {
        connect_to_signal_socket( sig_port, true );
    }

    return 0;
}

static rc_t perform_server_fkt ( server_context_ptr ctx )
{
    WSADATA wsaData;
    int wsaret;
    HANDLE h_cancel, h_server;

    wsaret = WSAStartup ( 0x101, &wsaData );
    if ( wsaret != 0 )
    {
        KOutMsg( "WSAStartup() failed\n" );
        return 1;
    }

    if ( !open_server_socket( &(ctx->server_socket), ctx->server_port, false ) )
    {
        KOutMsg( "open server socket failed\n" );
        WSACleanup();
        return 1;
    }

    if ( !open_server_socket( &(ctx->signal_socket), ctx->signal_port, true ) )
    {
        KOutMsg( "open server-signal socket failed\n" );
        WSACleanup();
        return 1;
    }

    h_cancel = create_simple_thread( cancel_thread, &(ctx->signal_port) );
    if ( h_cancel != 0 )
    {
        h_server = create_simple_thread( server_thread, ctx );
        if ( h_server != 0 )
        {
            WaitForSingleObject( h_server, INFINITE );
        }
        else
        {
            KOutMsg( "ERROR server thread not created\n" );
            WaitForSingleObject( h_cancel, INFINITE );
        }
    }
    else
    {
        KOutMsg( "cancel thread not created\n" );
    }

    WSACleanup();
    return 0;
}

rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;
    rc_t rc = ArgsMakeAndHandle ( &args, argc, argv, 1,
        ServerOptions, sizeof ( ServerOptions ) / sizeof ( OptDef ) );
    if ( rc == 0 )
    {
        server_context ctx;
        rc = setup_server_context( args, &ctx );
        if ( rc == 0 )
        {
            rc = perform_server_fkt( &ctx );
        }
        ArgsWhack ( args );
    }
    else
    {
        KOutMsg( "ArgsMakeAndHandle() failed\n", rc );
    }

    return rc;
}
