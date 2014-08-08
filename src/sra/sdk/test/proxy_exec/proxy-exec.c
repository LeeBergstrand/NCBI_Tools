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
#include <klib/printf.h>
#include <klib/data-buffer.h>
#include <sysalloc.h>
#include <kfs/file.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>          /* hostent struct, gethostbyname() */
#include <arpa/inet.h>      /* inet_ntoa() to format IP address */
#include <netinet/in.h>     /* in_addr structure */
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>         /* for close */

#include <common/test_assert.h>

#define DEFAULT_SERVER_PORT 20000
#define BUFSIZE 2048

#define OPTION_SERVER "server"
#define OPTION_PORT   "port"
#define OPTION_DIR    "dir"
#define OPTION_TOKEN  "token"
#define OPTION_ENV    "env"

typedef struct proxy_context
{
    /* where to connect to */
    const char *server_name;
    uint16_t server_port;

    /* what to send */
    const char *server_dir;
    const char *command_line;
    VNamelist *tokens;
    VNamelist *env;

    /* reply from proxy */
    uint32_t exit_code;
    uint32_t stdout_bytes;
    uint32_t stderr_bytes;
} proxy_context;
typedef proxy_context* p_proxy_context;

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
const char UsageDefaultName[] = "proxy-exec";
const char * progname = UsageDefaultName;

rc_t CC UsageSummary ( const char * progname )
{
    KOutMsg( "******************************************************************************\n"
             " proxy-exec:\n"
             "******************************************************************************\n" );
    KOutMsg( "usage:\n"
             "    proxy-exec --server NCBIPC0000 command\n"
             "    server .... name of the exec-server (a windows pc)\n"
             "    command ... program with parameters to be executed on proxy\n\n"
             "example: --server NBCBIPC0000 cmd.exe /C dir\n"
             "******************************************************************************\n" );
    return 0;
}

rc_t CC Usage ( const Args * args )
{
    ArgsArgvValue ( args, 0, &progname );

    UsageSummary ( progname );
    KOutMsg( "options:\n"
             "--server -S ... name of ip-addr of the execute server (mandatory)\n"
             "--port   -P ... ip-port the server is listening for (default 20000)\n"
             "--dir    -D ... directory where to execute the command (default server-dir)\n"
             "--token  -T ... token to be replaced in the commandline on server-site\n"
             "--evn    -E ... environment name-value pair (use quotes!)\n"
             "******************************************************************************\n" );
    return 0;
}

rc_t CC Version ( const Args * args )
{
    rc_t rc;
    ver_t version;

    rc = ArgsArgvValue ( args, 0, &progname );
    version = KAppVersion();

    KOutMsg( "%s: %V\n", progname, version );

    return 0;
}

static void init_client_context( p_proxy_context ctx )
{
    if ( ctx )
    {
        ctx->server_name = NULL;
        ctx->server_dir = NULL;
        ctx->command_line = NULL;
        ctx->server_port = 0;
        VNamelistMake ( &(ctx->tokens), 5 );
        VNamelistMake ( &(ctx->env), 5 );

        ctx->exit_code = 0;
        ctx->stdout_bytes = 0;
        ctx->stderr_bytes = 0;
    }
}

static void free_context_ptr( char ** ptr )
{
    if ( *ptr )
    {
        free( *ptr );
        *ptr = NULL;
    }
}

static void free_client_context( p_proxy_context ctx )
{
    if ( ctx )
    {
        free_context_ptr( (char**)&(ctx->server_name) );
        free_context_ptr( (char**)&(ctx->server_dir) );
        free_context_ptr( (char**)&(ctx->command_line) );
        VNamelistRelease ( ctx->tokens );
        VNamelistRelease ( ctx->env );
    }
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

static rc_t read_from_stdin(KDataBuffer* buf)
{
    rc_t rc = 0;
    const KFile* std_in = NULL;    
    
    rc = KFileMakeStdIn(&std_in);
    if (rc == 0) 
    {
        rc = KDataBufferMakeBytes(buf, 0);
        if (rc == 0)
        {
            const uint64_t ChunkSize=4096;
            uint64_t pos = 0;
            size_t num_read = 0;
            do 
            {
                rc = KDataBufferResize(buf, buf->elem_count + ChunkSize);
                if (rc != 0)
                {
                    KDataBufferWhack(buf);
                    return rc;
                }
                rc = KFileRead(std_in, pos, (char*)buf->base + pos, ChunkSize, &num_read);
                if (rc != 0)
                {
                    KDataBufferWhack(buf);
                    return rc;
                }
                pos += num_read;
            }
            while (num_read == ChunkSize);
            ((char*)buf->base)[pos] = 0;
        }
        KFileRelease(std_in);
    }
    return rc;
}

static rc_t setup_commandline( const Args * args, p_proxy_context ctx )
{
    uint32_t count;
    rc_t rc = ArgsParamCount( args, &count );
    if ( rc == 0 )
    {
        if (count > 0)
        {
            uint32_t idx;
            for ( idx = 0; idx < count; ++idx )
            {
                const char *value = NULL;
                rc = ArgsParamValue( args, idx, &value );
                if ( rc == 0 )
                {
                    if ( ctx->command_line == NULL )
                    {
                        ctx->command_line = string_dup_measure ( value, NULL );
                    }
                    else
                    {
                        uint32_t new_len = string_size ( ctx->command_line ) + string_size( value ) + 2;
                        char *temp = malloc( new_len );
                        if ( temp )
                        {
                            rc = string_printf ( temp, new_len, NULL, "%s %s", ctx->command_line, value );
                            if ( rc == 0 )
                            {
                                free( (void*)ctx->command_line );
                                ctx->command_line = temp;
                            }
                            else
                            {
                                free( temp );
                            }
                        }
                    }
                }
            }
         }
         else /* no cmdline parameters; read the command from stdin (one line only) */
         {
            KDataBuffer buf;
            rc = read_from_stdin(&buf);
            if ( rc == 0)
            {
                char* eol=string_chr((const char*)buf.base, buf.elem_count, '\n');
                if (eol)
                {
                    *eol=0;
                }
                eol=string_chr((const char*)buf.base, buf.elem_count, '\r');
                if (eol)
                {
                    *eol=0;
                }
                ctx->command_line=string_dup_measure( (const char*)buf.base, NULL );
                KDataBufferWhack(&buf);
            }
         }
    }
    return rc;
}


static void collect_list_items( const Args * args, const char * option, VNamelist *nl )
{
    uint32_t count;
    rc_t rc = ArgsOptionCount( args, option, &count );
    if ( ( rc == 0 )&&( count > 0 )&&( nl != NULL ) )
    {
        uint32_t i;
        for ( i = 0; i < count; ++i )
        {
            const char* s = NULL;
            rc = ArgsOptionValue( args, option, i, &s );
            if ( rc == 0 && s != NULL )
                VNamelistAppend ( nl, s );
        }
    }
}


static rc_t setup_client_context( const Args * args, p_proxy_context ctx )
{
    rc_t rc = 0;

    ctx->server_name = string_dup_measure ( get_str_option( args, OPTION_SERVER ), NULL );
    ctx->server_dir  = string_dup_measure ( get_str_option( args, OPTION_DIR ), NULL );
    ctx->server_port = get_uint16_option( args, OPTION_PORT, DEFAULT_SERVER_PORT );
    collect_list_items( args, OPTION_TOKEN, ctx->tokens );
    collect_list_items( args, OPTION_ENV, ctx->env );
    rc = setup_commandline( args, ctx );

    return rc;
}

static char const * name_of_the_server[]            = { "name of the server", NULL };
static char const * listen_port_of_the_server[]     = { "listen-port of the server", NULL };
static char const * curr_directory_on_the_server[]  = { "curr. directory on the server", NULL };
static char const * cmd_line_token[]                = { "cmd-line-token", NULL };
static char const * cmd_line_env[]                  = { "environment name-value-pair", NULL };

OptDef ProxyOptions[] =
{
    { OPTION_SERVER, "S", NULL, name_of_the_server, 1, true, false },
    { OPTION_PORT,   "P", NULL, listen_port_of_the_server, 1, true, false },
    { OPTION_DIR,    "D", NULL, curr_directory_on_the_server, 1, true, false },
    { OPTION_TOKEN,  "T", NULL, cmd_line_token, 0, true, false },
    { OPTION_ENV,    "E", NULL, cmd_line_env, 0, true, false }
};

static rc_t connect_to_server( const p_proxy_context ctx, int * sock )
{
    struct hostent *host;
    struct sockaddr_in srv;
    char *first_addr;
    
    if ( ( host = gethostbyname( ctx->server_name ) ) == NULL )
    {
        KOutMsg( "failed to lookup host '%s'\n", ctx->server_name );
        return -1;
    }
    first_addr = host->h_addr_list[0];
    
    if ( ( *sock = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 )
    {
        KOutMsg( "failed to create socket\n" );
        return -1;
    }

    memset ( &srv, 0, sizeof ( srv ) );
    srv.sin_family = AF_INET;    
    memcpy( &(srv.sin_addr.s_addr), first_addr, host->h_length );
    srv.sin_port = htons( ctx->server_port );
    
    /* Establish connection */
    if ( connect( *sock, (struct sockaddr *) &srv, sizeof( srv ) ) < 0 )
    {
        KOutMsg( "failed to connect\n" );
        close( *sock );
        return -1;
    }

/*    KOutMsg( "connected to %s:%d\n", ctx->server_name, ctx->server_port ); */
    return 0;
}

#define RX_HEADER 1
#define RX_BODY 2
#define RX_READY 3

typedef struct rx_context
{
    p_proxy_context ctx;
    char func;
    char *temp;
    size_t temp_alloc;
    size_t body_n;
    size_t body_rx;
    size_t idx;
    size_t mode;
} rx_context;
typedef rx_context* p_rx_context;

static void init_rx_context( p_rx_context rx_ctx, p_proxy_context pr_ctx )
{
    if ( rx_ctx ) 
    {
        rx_ctx->ctx = pr_ctx;
        rx_ctx->mode = RX_HEADER;
        rx_ctx->idx = 0;
        rx_ctx->body_rx = 0;
        rx_ctx->body_n = 0;
        rx_ctx->temp_alloc = BUFSIZE;
        rx_ctx->temp = malloc( rx_ctx->temp_alloc );
    }
}

static void free_rx_context( p_rx_context rx_ctx )
{
    if ( rx_ctx ) 
    {
        if ( rx_ctx->temp ) free( rx_ctx->temp );
    }
}

static void put_into_temp( p_rx_context rx_ctx, const char c )
{
    if ( rx_ctx->idx >= rx_ctx->temp_alloc )
    {
        rx_ctx->temp_alloc += BUFSIZE;
        rx_ctx->temp = realloc( rx_ctx->temp, rx_ctx->temp_alloc );
    }
    rx_ctx->temp[ rx_ctx->idx++ ] = c;
    rx_ctx->body_rx++;
}

static void receive_header( p_rx_context rx_ctx, const char c )
{
    if ( c == '\n' )
    {
        /* the header got terminated... */
        rx_ctx->func  = rx_ctx->temp[ 0 ]; /* X=exit_code, S=stdout, E=stderr */
        rx_ctx->temp[ rx_ctx->idx ] = 0; /* terminate as c-string */
        rx_ctx->body_n = atoi( &(rx_ctx->temp[1]) ); /* convert to int */
        switch ( rx_ctx->func )
        {
        case 'X' :  rx_ctx->ctx->exit_code = rx_ctx->body_n;
                    rx_ctx->mode = RX_READY;
                    break;

        case 'S' :  rx_ctx->ctx->stdout_bytes+=rx_ctx->body_n;
                    break;

        case 'E' :  rx_ctx->ctx->stderr_bytes+=rx_ctx->body_n;
                    break;
        }

        /* if it was the header of a body > 0 */
        if ( rx_ctx->body_n > 0 && rx_ctx->mode == RX_HEADER )
        {
            rx_ctx->mode = RX_BODY;
        }

        rx_ctx->idx = 0;
        rx_ctx->body_rx = 0;
    }
    else
    {
        put_into_temp( rx_ctx, c );
    }
}

static void write_to_output_without_cr( const char *s )
{
    char * temp;
    size_t i, j, l;

    if ( s == NULL ) return;
    l = strlen( s );
    if ( l < 1 ) return;

    temp = malloc(  l+1 );
    if ( temp == NULL ) return;
    
    j=0;
    for ( i=0; i<l; i++ )
    {
        if ( s[i] != '\r' )
        {
            temp[j++] = s[i];
        }
    }
    temp[ j ] = 0;
    KOutMsg( "%s", temp );
    free( temp );
}

static void receive_body( p_rx_context rx_ctx, const char c )
{
    put_into_temp( rx_ctx, c );
    if ( rx_ctx->body_rx >= rx_ctx->body_n )
    {
        /* the last char has been received */
        rx_ctx->temp[ rx_ctx->idx ] = 0; /* terminate the string part... */
        switch( rx_ctx->func )
        {
            case 'S' : KOutHandlerSetStdOut(); break;
            case 'E' : KOutHandlerSetStdErr(); break;
        }
        /* write the received chars to stdout/stderr */
        write_to_output_without_cr( rx_ctx->temp );
        rx_ctx->idx = 0;
        rx_ctx->mode = RX_HEADER;
    }
}

static rc_t receive_reply_from_proxy( const int sock, p_proxy_context ctx )
{
    rx_context rx_ctx;

    init_rx_context( &rx_ctx, ctx );
    while ( rx_ctx.mode != RX_READY )
    {
        char buffer[ 2048 ];
        ssize_t received = recv( sock, buffer, sizeof( buffer )-1, 0 );
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
            rx_ctx.ctx->exit_code = 1;
        }
    }
    KOutHandlerSetStdOut();
    free_rx_context( &rx_ctx );
    return 0;
}

static void send_number( const int sock, const char c, const int number )
{
    char buff[ 25 ];
    size_t l = snprintf( buff, sizeof( buff ) - 1, "%c%d\n", c, number );
    send ( sock, buff, (int)l, 0 );
}

static void send_hdr_and_body( const int sock, const char c, const char *body )
{
    if ( body )
    {
        int l = strlen( body );
        if ( l > 0 )
        {
            send_number( sock, c, l );
            send( sock, body, l, 0 );
        }
    }
}

static void send_namelist( const int sock, const char key, const VNamelist *list )
{
    const KNamelist *names;
    rc_t rc = VNamelistToConstNamelist( list, &names );
    if ( rc == 0 )
    {
        uint32_t count;
        rc = KNamelistCount( names, &count );
        if ( rc == 0 && count > 0 )
        {
            uint32_t i;
            for ( i = 0; i < count; ++i )
            {
                const char *s;
                rc = KNamelistGet( names, i, &s );
                if ( rc == 0 && s != NULL )
                    send_hdr_and_body( sock, key, s );
            }
        }
        KNamelistRelease ( names );
    }
}

static void send_request( const int sock, const p_proxy_context ctx )
{
    send_hdr_and_body( sock, 'P', ctx->server_dir );
    send_hdr_and_body( sock, 'C', ctx->command_line );
    send_namelist( sock, 'T', ctx->tokens );
    send_namelist( sock, 'E', ctx->env );
    send_number( sock, 'X', 0 );
}

static rc_t perform_proxy_fkt( const p_proxy_context ctx )
{
    int sock;

    rc_t rc = connect_to_server( ctx, &sock );
    if ( rc == 0 )
    {
        send_request( sock, ctx );
        rc = receive_reply_from_proxy( sock, ctx );
        close( sock );
    }
    else
    {
        ctx->exit_code = rc;
    }
    return rc;
}

rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;
    rc_t rc = ArgsMakeAndHandle ( &args, argc, argv, 1,
        ProxyOptions, sizeof ( ProxyOptions ) / sizeof ( OptDef ) );
    if ( rc == 0 )
    {
        proxy_context ctx;
        init_client_context( &ctx );
        rc = setup_client_context( args, &ctx );
        if ( rc == 0 )
        {
            if ( ctx.command_line == NULL || ctx.server_name == NULL )
            {
                Usage ( args );
            }
            else
            {
                rc = perform_proxy_fkt( &ctx );
                /*
                KOutMsg( "%d bytes STDOUT, %d bytes STDERR, EXITCODE = %d\n",
                         ctx.stdout_bytes, 
                         ctx.stderr_bytes,
                         ctx.exit_code );
                */
            }
        }
        rc = ctx.exit_code;
        free_client_context( &ctx );
        ArgsWhack ( args );
    }
    else
    {
        KOutMsg( "ArgsMakeAndHandle() failed", rc );
    }

    return rc;
}
