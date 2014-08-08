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

#ifndef _h_support_
#define _h_support_

#include <kapp/args.h>
#include <klib/rc.h>
#include <insdc/insdc.h>
#include "trans_struct.h"

rc_t get_str_option( const Args *args, const char *name, const char ** res );

rc_t get_uint32_option( const Args *args, const char *name, uint32_t *res, const uint32_t def );

rc_t get_int32_option( const Args *args, const char *name, int32_t *res, const int32_t def );

rc_t get_uint32_array( const Args *args, const char *name, uint32_t *res, uint32_t *count );

/* =========================================================================================== */

INSDC_4na_bin ascii_to_4na( const char c );

char _4na_to_ascii( INSDC_4na_bin c, bool reverse );

INSDC_4na_bin * dup_2_4na( const char * s );

char * dup_2_ascii( const INSDC_4na_bin * b, size_t len, bool reverse );

rc_t get_ro( Args * args, const char * name, const int32_t ** RO, uint32_t * ro_count );

void print_ro( const int32_t * RO, uint32_t ro_count );

bool * make_bool_array( const char * s );

uint32_t count_true( const bool * v, uint32_t len );

int32_t vector_sum( const int32_t * v, uint32_t len, bool ignore_first );

rc_t CC write_to_FILE( void *f, const char *buffer, size_t bytes, size_t *num_writ );

/* =========================================================================================== */

typedef struct stdout_redir stdout_redir;

rc_t make_stdout_redir( stdout_redir ** writer, const char * filename, size_t bufsize );

void release_stdout_redirection( stdout_redir * writer );

/* =========================================================================================== */

rc_t make_trans_opt( trans_opt * opt, Args * args );

rc_t make_trans_ctx( trans_ctx * ctx, trans_opt * opt, bool open_reference );

void free_trans_ctx( trans_ctx * ctx );

#endif /* _h_support_ */
