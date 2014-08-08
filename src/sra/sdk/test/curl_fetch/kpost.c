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

#include "kpost.vers.h"

#include <kapp/main.h>
#include <kapp/args.h>

#include <klib/out.h>
#include <klib/log.h>
#include <klib/text.h>
#include <klib/rc.h>
#include <klib/data-buffer.h>

#include <kns/kns_mgr.h>
#include <kns/KCurlRequest.h>

#include <sysalloc.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*===========================================================================

    kpost sends a post-message to a server

 =========================================================================== */

const char UsageDefaultName[] = "kpost";

rc_t CC UsageSummary ( const char * progname )
{
    return KOutMsg ("\n"
                    "Usage:\n"
                    "  %s <path> [<path> ...] [options]\n"
                    "\n", progname);
}


#define OPTION_URL "url"
#define ALIAS_URL  "u"
static const char * url_usage[]       = { "specify the url", NULL };

#define OPTION_FIELD "field"
#define ALIAS_FIELD  "f"
static const char * field_usage[]     = { "specify name-value-pair", NULL };

#define OPTION_NAME  "name"
#define ALIAS_NAME   "n"
static const char * name_usage[]     = { "specify name of name-value-pair", NULL };

#define OPTION_VALUE "value"
#define ALIAS_VALUE  "l"
static const char * value_usage[]     = { "specify value of name-value-pair", NULL };


#define OPTION_VERB "curl_verbose"
#define ALIAS_VERB  "o"
static const char * verb_usage[]     = { "show http protocol", NULL };

OptDef MyOptions[] =
{
/*    name            alias         fkt   usage-txt,      cnt, needs value, required */
    { OPTION_URL,     ALIAS_URL,     NULL, url_usage,      1,   true,        true },
    { OPTION_FIELD,   ALIAS_FIELD,   NULL, field_usage,    0,   true,        false },
    { OPTION_NAME,    ALIAS_NAME,    NULL, name_usage,     0,   true,        false },
    { OPTION_VALUE,   ALIAS_VALUE,   NULL, value_usage,    0,   true,        false },
    { OPTION_VERB,    ALIAS_VERB,    NULL, verb_usage,     1,   false,       false }
};

rc_t CC Usage ( const Args * args )
{
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
    uint32_t i;
    rc_t rc;

    if ( args == NULL )
        rc = RC ( rcApp, rcArgv, rcAccessing, rcSelf, rcNull );
    else
        rc = ArgsProgram ( args, &fullpath, &progname );

    if ( rc )
        progname = fullpath = UsageDefaultName;

    UsageSummary ( progname );

    KOutMsg ( "Options:\n" );

    HelpOptionsStandard ();
    for ( i = 0; i < sizeof ( MyOptions ) / sizeof ( MyOptions[ 0 ] ); ++i )
        HelpOptionLine ( MyOptions[i].aliases, MyOptions[i].name, NULL, MyOptions[i].help );

    return rc;
}


/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
ver_t CC KAppVersion ( void )
{
    return KPOST_VERS;
}


rc_t get_str( Args * args, const char *option, const char ** value )
{
    uint32_t count;
    rc_t rc = ArgsOptionCount( args, option, &count );
    *value = NULL;
    if ( rc == 0 && count > 0 )
        rc = ArgsOptionValue( args, option, 0, value );
    return rc;
}


static rc_t add_fields( Args * args, struct KCurlRequest *request )
{
    uint32_t count;
    rc_t rc = ArgsOptionCount( args, OPTION_FIELD, &count );
    if ( rc != 0 )
        KOutMsg( "failed to count fields %R\n", rc );
    else
    {
        uint32_t i;
        for ( i = 0; i < count && rc == 0; ++i )
        {
            const char * fields;
            rc = ArgsOptionValue( args, OPTION_FIELD, i, &fields );
            if ( rc != 0 )
                KOutMsg( "failed to get field[ %i ] %R\n", i, rc );
            else
            {
                String s;
                StringInitCString( &s, fields );
                rc = KCurlRequestAddSFields( request, &s );
                if ( rc != 0 )
                    KOutMsg( "failed to add field >%S< %R\n", &s, rc );
                else
                    KOutMsg( "field >%S< added\n", &s );
            }
        }
    }
    return rc;
}


static rc_t add_name_values( Args * args, struct KCurlRequest *request )
{
    uint32_t name_count, value_count;
    rc_t rc = ArgsOptionCount( args, OPTION_NAME, &name_count );
    if ( rc != 0 )
        KOutMsg( "failed to count names %R\n", rc );
    else
    {
        rc = ArgsOptionCount( args, OPTION_VALUE, &value_count );
        if ( rc != 0 )
            KOutMsg( "failed to count values %R\n", rc );
        else if ( name_count != value_count )
        {
            KOutMsg( "number of name- ( %u ) and value ( &u ) parameters do not match\n", name_count, value_count );
            rc = RC( rcExe, rcNoTarg, rcConstructing, rcParam, rcInvalid );
        }
        else
        {
            uint32_t i;
            for ( i = 0; i < name_count && rc == 0; ++i )
            {
                const char * name;
                rc = ArgsOptionValue( args, OPTION_NAME, i, &name );
                if ( rc != 0 )
                    KOutMsg( "failed to get name[ %i ] %R\n", i, rc );
                else
                {
                    const char * value;
                    rc = ArgsOptionValue( args, OPTION_VALUE, i, &value );
                    if ( rc != 0 )
                        KOutMsg( "failed to get value[ %i ] %R\n", i, rc );
                    else
                    {
                        String s_name, s_value;
                        StringInitCString( &s_name, name );
                        StringInitCString( &s_value, value );
                        rc = KCurlRequestAddSField( request, &s_name, &s_value );
                        if ( rc != 0 )
                            KOutMsg( "failed to add name >%S< = value >%S< %R\n", &s_name, &s_value, rc );
                        else
                            KOutMsg( "name >%S< = value >%S< added\n", &s_name, &s_value );
                    }
                }
            }
        }
    }
    return rc;
}


static rc_t perform_request( struct KCurlRequest *request )
{
    rc_t rc;
    KDataBuffer buffer;
    memset( &buffer, 0, sizeof buffer );
    rc = KCurlRequestPerform( request, &buffer );
    if ( rc != 0 )
        KOutMsg( "failed to perform POST-request %R\n", rc );
    else
    {
        KOutMsg( "%u bytes received\n", buffer.elem_count );
        KOutMsg( "%s\n", (char *)buffer.base );
    }
    KDataBufferWhack ( &buffer );
    return rc;
}


rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;

    rc_t rc = ArgsMakeAndHandle ( &args, argc, argv, 1,
                MyOptions, sizeof ( MyOptions ) / sizeof ( OptDef ) );
    if ( rc != 0 )
        KOutMsg( "ArgsMakeAndHandle() failed %R\n", rc );
    else
    {
        struct KNSManager * mgr;
        rc = KNSManagerMake( &mgr );
        if ( rc != 0 )
            KOutMsg( "cannot create KNSManager %R\n", rc );
        else
        {
            const char * url;
            rc = get_str( args, OPTION_URL, &url );
            if ( rc != 0 )
                KOutMsg( "failed to detect url in commandline %R\n", rc );
            else
            {
                uint32_t count;
                rc = ArgsOptionCount( args, OPTION_VERB, &count );
                if ( rc != 0 )
                    KOutMsg( "failed to count verbose %R\n", rc );
                else
                {
                    struct KCurlRequest *request;
                    rc = KNSManagerMakeRequest( mgr, &request, url, ( count > 0 ) );
                    if ( rc != 0 )
                        KOutMsg( "failed to create KCurlRequest %R\n", rc );
                    else
                    {
                        KOutMsg( "sending POST to >%s<\n", url );

                        rc = add_fields( args, request );
                        if ( rc == 0 )
                            rc = add_name_values( args, request );
                        if ( rc == 0 )
                            rc = perform_request( request );

                        KCurlRequestRelease( request );
                    }
                }
            }
            KNSManagerRelease( mgr );
        }
        ArgsWhack ( args );
    }
    return rc;
}
