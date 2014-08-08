/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2005 Frediano Ziglio
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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <assert.h>
#include <limits.h>

#include "tds.h"
#include "tdsconvert.h"
#include "tdsiconv.h"
#include "tds_checks.h"
#include "replacements.h"
#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: token.c 362183 2012-05-08 12:21:35Z ivanovp $");

static int tds_process_msg(TDSSOCKET * tds, int marker);
static int tds_process_compute_result(TDSSOCKET * tds);
static int tds_process_compute_names(TDSSOCKET * tds);
static int tds7_process_compute_result(TDSSOCKET * tds);
static int tds_process_result(TDSSOCKET * tds);
static int tds_process_col_name(TDSSOCKET * tds);
static int tds_process_col_fmt(TDSSOCKET * tds);
static int tds_process_colinfo(TDSSOCKET * tds);
static int tds_process_compute(TDSSOCKET * tds, TDS_INT * computeid);
static int tds_process_cursor_tokens(TDSSOCKET * tds);
static int tds_process_row(TDSSOCKET * tds);
static int tds_process_param_result(TDSSOCKET * tds, TDSPARAMINFO ** info);
static int tds7_process_result(TDSSOCKET * tds);
static TDSDYNAMIC *tds_process_dynamic(TDSSOCKET * tds);
static int tds_process_auth(TDSSOCKET * tds);
static int tds_get_data(TDSSOCKET * tds, TDSCOLUMN * curcol, unsigned char *current_row, int i);
static int tds_get_data_info(TDSSOCKET * tds, TDSCOLUMN * curcol, int is_param);
static int tds_process_env_chg(TDSSOCKET * tds);
static /*@observer@*/ const char *_tds_token_name(unsigned char marker);
static int tds_process_param_result_tokens(TDSSOCKET * tds);
static int tds_process_params_result_token(TDSSOCKET * tds);
static int tds_process_dyn_result(TDSSOCKET * tds);
static int tds5_get_varint_size(int datatype);
static int tds5_process_result(TDSSOCKET * tds);
static int tds5_process_dyn_result2(TDSSOCKET * tds);
static void adjust_character_column_size(const TDSSOCKET * tds, TDSCOLUMN * curcol);
static int determine_adjusted_size(const TDSICONV * char_conv, int size);
static int tds_process_default_tokens(TDSSOCKET * tds, int marker);
static int tds_process_end(TDSSOCKET * tds, int marker, /*@out@*/ int *flags_parm);
static int tds5_process_optioncmd(TDSSOCKET * tds);

static int tds5_get_varint_size(int datatype);
static /*@observer@*/ const char *tds_pr_op(int op);
static int tds_alloc_get_string(TDSSOCKET * tds, /*@special@*/ char **string, int len) /*allocates *string*/;
void generate_random_buffer(unsigned char *out, int len);


/**
 * \ingroup libtds
 * \defgroup token Results processing
 * Handle tokens in packets. Many PDU (packets data unit) contain tokens.
 * (like result description, rows, data, errors and many other).
 */


/**
 * \addtogroup token
 * \@{
 */

/**
 * tds_process_default_tokens() is a catch all function that is called to
 * process tokens not known to other tds_process_* routines
 */
static int
tds_process_default_tokens(TDSSOCKET * tds, int marker)
{
    int tok_size;
    int done_flags;
    TDS_INT ret_status;

    CHECK_TDS_EXTRA(tds);

    tdsdump_log(TDS_DBG_FUNC, "tds_process_default_tokens() marker is %x(%s)\n", marker, _tds_token_name(marker));

    if (IS_TDSDEAD(tds)) {
        tdsdump_log(TDS_DBG_FUNC, "leaving tds_process_default_tokens() connection dead\n");
        tds_set_state(tds, TDS_DEAD);
        return TDS_FAIL;
    }

    switch (marker) {
    case TDS_AUTH_TOKEN:
        return tds_process_auth(tds);
        break;
    case TDS_ENVCHANGE_TOKEN:
        return tds_process_env_chg(tds);
        break;
    case TDS_DONE_TOKEN:
    case TDS_DONEPROC_TOKEN:
    case TDS_DONEINPROC_TOKEN:
        return tds_process_end(tds, marker, &done_flags);
        break;
    case TDS_PROCID_TOKEN:
        tds_get_n(tds, NULL, 8);
        break;
    case TDS_RETURNSTATUS_TOKEN:
        ret_status = tds_get_int(tds);
        marker = tds_peek(tds);
        if (marker != TDS_PARAM_TOKEN && marker != TDS_DONEPROC_TOKEN && marker != TDS_DONE_TOKEN)
            break;
        tds->has_status = 1;
        tds->ret_status = ret_status;
        tdsdump_log(TDS_DBG_FUNC, "tds_process_default_tokens: return status is %d\n", tds->ret_status);
        break;
    case TDS_ERROR_TOKEN:
    case TDS_INFO_TOKEN:
    case TDS_EED_TOKEN:
        return tds_process_msg(tds, marker);
        break;
    case TDS_CAPABILITY_TOKEN:
        /* TODO split two part of capability and use it */
        tok_size = tds_get_smallint(tds);
        /* vicm */
        /*
         * Sybase 11.0 servers return the wrong length in the capability packet, causing use to read
         * past the done packet.
         */
        if (!TDS_IS_MSSQL(tds) && tds->product_version < TDS_SYB_VER(12, 0, 0)) {
            unsigned char type, size, *p, *pend;

            p = tds->capabilities;
            pend = tds->capabilities + TDS_MAX_CAPABILITY;

            do {
                type = tds_get_byte(tds);
                size = tds_get_byte(tds);
                if ((p + 2) > pend)
                    break;
                *p++ = type;
                *p++ = size;
                if ((p + size) > pend)
                    break;
                if (tds_get_n(tds, p, size) == NULL)
                    return TDS_FAIL;
            } while (type != 2);
        } else {
            if (tds_get_n(tds, tds->capabilities, tok_size > TDS_MAX_CAPABILITY ? TDS_MAX_CAPABILITY : tok_size) ==
                NULL)
                return TDS_FAIL;
        }
        break;
        /* PARAM_TOKEN can be returned inserting text in db, to return new timestamp */
    case TDS_PARAM_TOKEN:
        tds_unget_byte(tds);
        return tds_process_param_result_tokens(tds);
        break;
    case TDS7_RESULT_TOKEN:
        return tds7_process_result(tds);
        break;
    case TDS_OPTIONCMD_TOKEN:
        return tds5_process_optioncmd(tds);
        break;
    case TDS_RESULT_TOKEN:
        return tds_process_result(tds);
        break;
    case TDS_ROWFMT2_TOKEN:
        return tds5_process_result(tds);
        break;
    case TDS_COLNAME_TOKEN:
        return tds_process_col_name(tds);
        break;
    case TDS_COLFMT_TOKEN:
        return tds_process_col_fmt(tds);
        break;
    case TDS_ROW_TOKEN:
        return tds_process_row(tds);
        break;
    case TDS5_PARAMFMT_TOKEN:
        /* store discarded parameters in param_info, not in old dynamic */
        tds->cur_dyn = NULL;
        return tds_process_dyn_result(tds);
        break;
    case TDS5_PARAMFMT2_TOKEN:
        tds->cur_dyn = NULL;
        return tds5_process_dyn_result2(tds);
        break;
    case TDS5_PARAMS_TOKEN:
        /* save params */
        return tds_process_params_result_token(tds);
        break;
    case TDS_CURINFO_TOKEN:
        return tds_process_cursor_tokens(tds);
        break;
    case TDS5_DYNAMIC_TOKEN:
    case TDS_LOGINACK_TOKEN:
    case TDS_ORDERBY_TOKEN:
    case TDS_CONTROL_TOKEN:
    case TDS_TABNAME_TOKEN: /* used for FOR BROWSE query */
        tdsdump_log(TDS_DBG_WARN, "Eating %s token\n", _tds_token_name(marker));
        tds_get_n(tds, NULL, tds_get_smallint(tds));
        break;
    case TDS_COLINFO_TOKEN:
        return tds_process_colinfo(tds);
        break;
    case TDS_ORDERBY2_TOKEN:
        tdsdump_log(TDS_DBG_WARN, "Eating %s token\n", _tds_token_name(marker));
        tds_get_n(tds, NULL, tds_get_int(tds));
        break;
    default: /* SYBEBTOK */
        tds_client_msg(tds->tds_ctx, tds, 20020, 9, 0, 0, "Bad token from the server: Datastream processing out of sync");
        if (IS_TDSDEAD(tds))
            tds_set_state(tds, TDS_DEAD);
        else
            tds_close_socket(tds);
        tdsdump_log(TDS_DBG_ERROR, "Unknown marker: %d(%x)!!\n", marker, (unsigned char) marker);
        return TDS_FAIL;
    }
    return TDS_SUCCEED;
}

static int
tds_set_spid(TDSSOCKET * tds)
{
    TDS_INT result_type;
    TDS_INT done_flags;
    TDS_INT rc;
    TDSCOLUMN *curcol;

    CHECK_TDS_EXTRA(tds);

    if (tds_submit_query(tds, "select @@spid") != TDS_SUCCEED) {
        return TDS_FAIL;
    }

    while ((rc = tds_process_tokens(tds, &result_type, &done_flags, TDS_RETURN_ROWFMT|TDS_RETURN_ROW|TDS_RETURN_DONE)) == TDS_SUCCEED) {

        switch (result_type) {

            case TDS_ROWFMT_RESULT:
                if (tds->res_info->num_cols != 1)
                    return TDS_FAIL;
                break;

            case TDS_ROW_RESULT:
                curcol = tds->res_info->columns[0];
                if (curcol->column_type == SYBINT2 || (curcol->column_type == SYBINTN && curcol->column_size == 2)) {
                    tds->spid = *((TDS_USMALLINT *) (tds->res_info->current_row + curcol->column_offset));
                } else if (curcol->column_type == SYBINT4 || (curcol->column_type == SYBINTN && curcol->column_size == 4)) {
                    tds->spid = *((TDS_UINT *) (tds->res_info->current_row + curcol->column_offset));
                } else
                    return TDS_FAIL;
                break;

            case TDS_DONE_RESULT:
                if ((done_flags & TDS_DONE_ERROR) != 0)
                    return TDS_FAIL;
                break;

            default:
                break;
        }
    }
    if (rc != TDS_NO_MORE_RESULTS)
        return TDS_FAIL;

    return TDS_SUCCEED;
}

/**
 * tds_process_login_tokens() is called after sending the login packet
 * to the server.  It returns the success or failure of the login
 * dependent on the protocol version. 4.2 sends an ACK token only when
 * successful, TDS 5.0 sends it always with a success byte within
 */
int
tds_process_login_tokens(TDSSOCKET * tds)
{
    int succeed = TDS_FAIL;
    int marker;
    int len;
    int memrc = 0;
    unsigned char major_ver, minor_ver;
    unsigned char ack;
    TDS_UINT product_version;

    CHECK_TDS_EXTRA(tds);

    tdsdump_log(TDS_DBG_FUNC, "tds_process_login_tokens()\n");
    /* get_incoming(tds->s); */
    do {
        marker = tds_get_byte(tds);
        tdsdump_log(TDS_DBG_FUNC, "looking for login token, got  %x(%s)\n", marker, _tds_token_name(marker));

        switch (marker) {
        case TDS_AUTH_TOKEN:
            tds_process_auth(tds);
            break;
        case TDS_LOGINACK_TOKEN:
            /* TODO function */
            len = tds_get_smallint(tds);
            ack = tds_get_byte(tds);
            major_ver = tds_get_byte(tds);
            minor_ver = tds_get_byte(tds);
            tds_get_n(tds, NULL, 2);
            /* ignore product name length, see below */
            tds_get_byte(tds);
            product_version = 0;
            /* get server product name */
            /* compute length from packet, some version seem to fill this information wrongly */
            len -= 10;
            if (tds->product_name)
                free(tds->product_name);
            if (major_ver >= 7u) {
                product_version = 0x80000000u;
                memrc += tds_alloc_get_string(tds, &tds->product_name, len / 2);
            } else if (major_ver >= 5) {
                memrc += tds_alloc_get_string(tds, &tds->product_name, len);
            } else {
                memrc += tds_alloc_get_string(tds, &tds->product_name, len);
                if (tds->product_name != NULL && strstr(tds->product_name, "Microsoft") != NULL)
                    product_version = 0x80000000u;
            }
            product_version |= ((TDS_UINT) tds_get_byte(tds)) << 24;
            product_version |= ((TDS_UINT) tds_get_byte(tds)) << 16;
            product_version |= ((TDS_UINT) tds_get_byte(tds)) << 8;
            product_version |= tds_get_byte(tds);
            /*
             * MSSQL 6.5 and 7.0 seem to return strange values for this
             * using TDS 4.2, something like 5F 06 32 FF for 6.50
             */
            if (major_ver == 4 && minor_ver == 2 && (product_version & 0xff0000ffu) == 0x5f0000ffu)
                product_version = ((product_version & 0xffff00u) | 0x800000u) << 8;
            tds->product_version = product_version;
#ifdef WORDS_BIGENDIAN
            /* TODO do a best check */
/*

                if (major_ver==7) {
                    tds->broken_dates=1;
                }
*/
#endif
            /*
             * TDS 5.0 reports 5 on success 6 on failure
             * TDS 4.2 reports 1 on success and is not
             * present on failure
             */
            if (ack == 5 || ack == 1)
                succeed = TDS_SUCCEED;
            /* authentication is now useless */
            if (tds->authentication) {
                tds->authentication->free(tds, tds->authentication);
                tds->authentication = NULL;
            }
            break;
        default:
            if (tds_process_default_tokens(tds, marker) == TDS_FAIL)
                return TDS_FAIL;
            break;
        }
    } while (marker != TDS_DONE_TOKEN);
    /* TODO why ?? */
    tds->spid = tds->rows_affected;
    if (tds->spid == 0) {
        if (tds_set_spid(tds) != TDS_SUCCEED) {
            tdsdump_log(TDS_DBG_ERROR, "tds_set_spid() failed\n");
            succeed = TDS_FAIL;
        }
    }
    tdsdump_log(TDS_DBG_FUNC, "leaving tds_process_login_tokens() returning %d\n", succeed);
    if (memrc != 0)
        succeed = TDS_FAIL;
    return succeed;
}

/**
put a 8 byte filetime from a time_t
This takes GMT as input
**/
static void unix_to_nt_time(TDS_UINT8 *nt, time_t t)
{
#define TIME_FIXUP_CONSTANT 11644473600LL

    TDS_UINT8 t2;

    if (t == (time_t)-1) {
        *nt = (TDS_UINT8)-1LL;
        return;
    }
    if (t == 0) {
        *nt = 0;
        return;
    }

    t2 = t;
    t2 += TIME_FIXUP_CONSTANT;
    t2 *= 1000*1000*10;

    *nt = t2;
}

#ifdef WORDS_BIGENDIAN
static TDS_UINT
swap_32(TDS_UINT val)
{
    return
      (((((TDS_UINT)val) & 0xff000000) >> 24) | ((((TDS_UINT)val) & 0x00ff0000) >>  8) |
       ((((TDS_UINT)val) & 0x0000ff00) <<  8) | ((((TDS_UINT)val) & 0x000000ff) << 24));
}
#endif

static TDS_UINT8
to_le(TDS_UINT8 val)
{
#ifndef WORDS_BIGENDIAN
  return val;
#else
  union {
        TDS_UINT8 ll;
        TDS_UINT  l[2];
    } w, r;
    w.ll = val;
    r.l[0] = swap_32( w.l[1] );
    r.l[1] = swap_32( w.l[0] );
    return r.ll;
#endif
}


static void
fill_names_blob_prefix(names_blob_prefix_t* prefix)
{
    TDS_UINT8 nttime = 0;

    unix_to_nt_time(&nttime, time(NULL));

    prefix->response_type = 0x01;
    prefix->max_response_type = 0x01;
    prefix->reserved1 = 0x0000;
    prefix->reserved2 = 0x00000000;
    prefix->timestamp.l = to_le(nttime);
    generate_random_buffer(prefix->challenge, sizeof(prefix->challenge));

    prefix->unknown = 0x00000000;
}

static int
tds_process_auth(TDSSOCKET * tds)
{
    int pdu_size;
    unsigned char nonce[8];
    TDS_UINT flags;
    int domain_len;
    int data_block_offset;

    int target_info_len = 0;
    int target_info_offset;

    /* For debuging purposes ...
    unsigned char* target_info;
    int target_info_pos;
    short target_type;
    short target_len;
    unsigned char target_content[128];
    */

    int names_blob_len;
    unsigned char* names_blob;

    unsigned char domain[128];
    int where;
    int rc;

    CHECK_TDS_EXTRA(tds);

    memset(domain, 0, sizeof(domain));

#if ENABLE_EXTRA_CHECKS
    if (!IS_TDS7_PLUS(tds))
        tdsdump_log(TDS_DBG_ERROR, "Called auth on TDS version < 7\n");
#endif

    pdu_size = tds_get_smallint(tds);
    tdsdump_log(TDS_DBG_INFO1, "TDS_AUTH_TOKEN PDU size %d\n", pdu_size);

    if (tds->authentication)
        return tds->authentication->handle_next(tds, tds->authentication, pdu_size);

    /* at least 32 bytes (till context) */
    if (pdu_size < 32)
        return TDS_FAIL;

    /* Getting Message Type 2 */
    /* TODO check first 2 values */
    tds_get_n(tds, NULL, 8);    /* NTLMSSP Signature */
    tds_get_int(tds);   /* sequence -> 2 */
    domain_len = tds_get_smallint(tds); /* domain len */
    domain_len = tds_get_smallint(tds); /* domain len */
    data_block_offset = tds_get_int(tds);   /* domain offset */
    flags = tds_get_int(tds);   /* flags */
    tds_get_n(tds, nonce, 8);
    tdsdump_dump_buf(TDS_DBG_INFO1, "TDS_AUTH_TOKEN nonce", nonce, 8);
    where = 32;

    /*data_block_offset == 32*/
    /* Version 1 -- The Context, Target Information, and OS Version structure are all omitted */

    if (data_block_offset != 32) {
        /* Version 2 -- The Context and Target Information fields are present, but the OS Version structure is not. */
        tds_get_n(tds, NULL, 8); /* Context (two consecutive longs) */

        target_info_len = tds_get_smallint(tds); /* Target Information len */
        target_info_len = tds_get_smallint(tds); /* Target Information len */
        target_info_offset = tds_get_int(tds);  /* Target Information offset */

        where += 16;

        if (data_block_offset == 56) {
            /* Version 3 -- The Context, Target Information, and OS Version structure are all present. */
            tds_get_n(tds, NULL, 8); /* OS Version Structure */
            where += 8;
        }
    }

    /* Read domain name in ucs2 encoding ... */
    tds_get_n(tds, domain, domain_len);
    tdsdump_log(TDS_DBG_INFO1, "TDS_AUTH_TOKEN domain %s\n", domain);
    where += domain_len;

    names_blob_len = sizeof(names_blob_prefix_t) + target_info_len + 4;

    /* Read Terget Info */
    names_blob = (unsigned char*)malloc(names_blob_len);
    memset(names_blob, 0, names_blob_len);

    fill_names_blob_prefix((names_blob_prefix_t*)names_blob);
    tds_get_n(tds, names_blob + sizeof(names_blob_prefix_t), target_info_len);

    /* Decode Target Info for debuging purposes ...*/
    /*
    target_info = names_blob + sizeof(names_blob_prefix_t);
    target_info_pos = 0;
    do {
        target_type = *(target_info + target_info_pos);
        target_info_pos += sizeof(target_type);
        target_len = *(target_info + target_info_pos);
        target_info_pos += sizeof(target_len);
        memcpy(target_content, target_info + target_info_pos, target_len);
        target_info_pos += target_len;
    } while(target_type != 0);
    */

    rc = tds7_send_auth(tds, nonce, flags, names_blob, names_blob_len);

    free(names_blob);

    return rc;
}

/**
 * process all streams.
 * tds_process_tokens() is called after submitting a query with
 * tds_submit_query() and is responsible for calling the routines to
 * populate tds->res_info if appropriate (some query have no result sets)
 * @param tds A pointer to the TDSSOCKET structure managing a client/server operation.
 * @param result_type A pointer to an integer variable which
 *        tds_process_tokens sets to indicate the current type of result.
 *  @par
 *  <b>Values that indicate command status</b>
 *  <table>
 *   <tr><td>TDS_DONE_RESULT</td><td>The results of a command have been completely processed. This command return no rows.</td></tr>
 *   <tr><td>TDS_DONEPROC_RESULT</td><td>The results of a  command have been completely processed. This command return rows.</td></tr>
 *   <tr><td>TDS_DONEINPROC_RESULT</td><td>The results of a  command have been completely processed. This command return rows.</td></tr>
 *  </table>
 *  <b>Values that indicate results information is available</b>
 *  <table><tr>
 *    <td>TDS_ROWFMT_RESULT</td><td>Regular Data format information</td>
 *    <td>tds->res_info now contains the result details ; tds->current_results now points to that data</td>
 *   </tr><tr>
 *    <td>TDS_COMPUTEFMT_ RESULT</td><td>Compute data format information</td>
 *    <td>tds->comp_info now contains the result data; tds->current_results now points to that data</td>
 *   </tr><tr>
 *    <td>TDS_DESCRIBE_RESULT</td><td></td>
 *    <td></td>
 *  </tr></table>
 *  <b>Values that indicate data is available</b>
 *  <table><tr>
 *   <td><b>Value</b></td><td><b>Meaning</b></td><td><b>Information returned</b></td>
 *   </tr><tr>
 *    <td>TDS_ROW_RESULT</td><td>Regular row results</td>
 *    <td>1 or more rows of regular data can now be retrieved</td>
 *   </tr><tr>
 *    <td>TDS_COMPUTE_RESULT</td><td>Compute row results</td>
 *    <td>A single row of compute data can now be retrieved</td>
 *   </tr><tr>
 *    <td>TDS_PARAM_RESULT</td><td>Return parameter results</td>
 *    <td>param_info or cur_dyn->params contain returned parameters</td>
 *   </tr><tr>
 *    <td>TDS_STATUS_RESULT</td><td>Stored procedure status results</td>
 *    <td>tds->ret_status contain the returned code</td>
 *  </tr></table>
 * @param flag Flags to select token type to stop/return
 * @todo Complete TDS_DESCRIBE_RESULT description
 * @retval TDS_SUCCEED if a result set is available for processing.
 * @retval TDS_NO_MORE_RESULTS if all results have been completely processed.
 */
int
tds_process_tokens(TDSSOCKET *tds, TDS_INT *result_type, int *done_flags, unsigned flag)
{
    int marker;
    TDSPARAMINFO *pinfo = NULL;
    TDSCOLUMN   *curcol;
    int rc;
    int saved_rows_affected = tds->rows_affected;
    TDS_INT ret_status;
    int cancel_seen = 0;
    unsigned return_flag = 0;

#define SET_RETURN(ret, f) \
    *result_type = ret; \
    return_flag = TDS_RETURN_##f | TDS_STOPAT_##f; \
    if (flag & TDS_STOPAT_##f) {\
        tds_unget_byte(tds); \
        tdsdump_log(TDS_DBG_FUNC, "tds_process_tokens::SET_RETURN stopping on current token\n"); \
        break; \
    }

    CHECK_TDS_EXTRA(tds);

    tdsdump_log(TDS_DBG_FUNC, "tds_process_tokens(%p, %p, %p, 0x%x)\n", tds, result_type, done_flags, flag);

    if (tds->state == TDS_IDLE) {
        tdsdump_log(TDS_DBG_FUNC, "tds_process_tokens() state is COMPLETED\n");
        *result_type = TDS_DONE_RESULT;
        return TDS_NO_MORE_RESULTS;
    }

    if (tds_set_state(tds, TDS_READING) != TDS_READING)
        return TDS_FAIL;

    rc = TDS_SUCCEED;
    for (;;) {

        marker = tds_get_byte(tds);
        tdsdump_log(TDS_DBG_INFO1, "processing result tokens.  marker is  %x(%s)\n", marker, _tds_token_name(marker));

        switch (marker) {
        case TDS7_RESULT_TOKEN:

            /*
             * If we're processing the results of a cursor fetch
             * from sql server we don't want to pass back the
             * TDS_ROWFMT_RESULT to the calling API
             */

            if (tds->internal_sp_called == TDS_SP_CURSORFETCH) {
                rc = tds7_process_result(tds);
                marker = tds_get_byte(tds);
                if (marker != TDS_TABNAME_TOKEN) {
                    tds_unget_byte(tds);
                } else {
                    if ((rc = tds_process_default_tokens(tds, marker)) == TDS_FAIL)
                        break;
                    marker = tds_get_byte(tds);
                    if (marker != TDS_COLINFO_TOKEN) {
                        tds_unget_byte(tds);
                    } else {
                        tds_process_colinfo(tds);
                    }
                }
            } else {
                SET_RETURN(TDS_ROWFMT_RESULT, ROWFMT);

                rc = tds7_process_result(tds);
                /*
                 * handle browse information (if presents)
                 * TODO copied from below, function or put in results process
                 */
                marker = tds_get_byte(tds);
                if (marker != TDS_TABNAME_TOKEN) {
                    tds_unget_byte(tds);
                    rc = TDS_SUCCEED;
                    break;
                }
                if ((rc = tds_process_default_tokens(tds, marker)) == TDS_FAIL)
                    break;
                marker = tds_get_byte(tds);
                if (marker != TDS_COLINFO_TOKEN) {
                    tds_unget_byte(tds);
                    rc = TDS_SUCCEED;
                    break;
                }
                if (rc == TDS_FAIL)
                    break;
                else {
                    tds_process_colinfo(tds);
                    rc = TDS_SUCCEED;
                    break;
                }
            }
            break;
        case TDS_RESULT_TOKEN:
            SET_RETURN(TDS_ROWFMT_RESULT, ROWFMT);
            rc = tds_process_result(tds);
            break;
        case TDS_ROWFMT2_TOKEN:
            SET_RETURN(TDS_ROWFMT_RESULT, ROWFMT);
            rc = tds5_process_result(tds);
            break;
        case TDS_COLNAME_TOKEN:
            rc = tds_process_col_name(tds);
            break;
        case TDS_COLFMT_TOKEN:
            SET_RETURN(TDS_ROWFMT_RESULT, ROWFMT);
            rc = tds_process_col_fmt(tds);
            /* handle browse information (if presents) */
            marker = tds_get_byte(tds);
            if (marker != TDS_TABNAME_TOKEN) {
                tds_unget_byte(tds);
                break;
            }
            if ((rc = tds_process_default_tokens(tds, marker)) == TDS_FAIL)
                break;
            marker = tds_get_byte(tds);
            if (marker != TDS_COLINFO_TOKEN) {
                tds_unget_byte(tds);
                break;
            }
            if (rc == TDS_FAIL)
                break;
            tds_process_colinfo(tds);
            rc = TDS_SUCCEED;
            break;
        case TDS_PARAM_TOKEN:
            tds_unget_byte(tds);
            if (tds->internal_sp_called) {
                tdsdump_log(TDS_DBG_FUNC, "processing parameters for sp %d\n", tds->internal_sp_called);
                while ((marker = tds_get_byte(tds)) == TDS_PARAM_TOKEN) {
                    tdsdump_log(TDS_DBG_INFO1, "calling tds_process_param_result\n");
                    tds_process_param_result(tds, &pinfo);
                }
                tds_unget_byte(tds);
                tdsdump_log(TDS_DBG_FUNC, "%d hidden return parameters\n", pinfo ? pinfo->num_cols : -1);
                if (pinfo && pinfo->num_cols > 0) {
                    curcol = pinfo->columns[0];
                    if (tds->internal_sp_called == TDS_SP_CURSOROPEN && tds->cur_cursor) {
                        TDSCURSOR  *cursor = tds->cur_cursor;

                        cursor->cursor_id = *(TDS_INT *) &(pinfo->current_row[curcol->column_offset]);
                        tdsdump_log(TDS_DBG_FUNC, "stored internal cursor id %d\n",
                                cursor->cursor_id);
                        cursor->srv_status &= ~TDS_CUR_ISTAT_CLOSED;
                        cursor->srv_status |= TDS_CUR_ISTAT_OPEN;
                        /* 0.95
                        cursor->srv_status &= ~(TDS_CUR_ISTAT_CLOSED|TDS_CUR_ISTAT_OPEN|TDS_CUR_ISTAT_DEALLOC);
                        cursor->srv_status |= cursor->cursor_id ? TDS_CUR_ISTAT_OPEN : TDS_CUR_ISTAT_CLOSED|TDS_CUR_ISTAT_DEALLOC;
                        */
                    }
                    if (tds->internal_sp_called == TDS_SP_PREPARE
                        && tds->cur_dyn && tds->cur_dyn->num_id == 0 && curcol->column_cur_size > 0) {
                        tds->cur_dyn->num_id = *(TDS_INT *) &(pinfo->current_row[curcol->column_offset]);
                    }
                }
                tds_free_param_results(pinfo);
            } else {
                SET_RETURN(TDS_PARAM_RESULT, PROC);
                rc = tds_process_param_result_tokens(tds);
            }
            break;
        case TDS_COMPUTE_NAMES_TOKEN:
            rc = tds_process_compute_names(tds);
            break;
        case TDS_COMPUTE_RESULT_TOKEN:
            SET_RETURN(TDS_COMPUTEFMT_RESULT, COMPUTEFMT);
            rc = tds_process_compute_result(tds);
            break;
        case TDS7_COMPUTE_RESULT_TOKEN:
            SET_RETURN(TDS_COMPUTEFMT_RESULT, COMPUTEFMT);
            rc = tds7_process_compute_result(tds);
            break;
        case TDS_ROW_TOKEN:
            /* overstepped the mark... */
            if (tds->cur_cursor) {
                TDSCURSOR  *cursor = tds->cur_cursor;

                tds->current_results = cursor->res_info;
                tdsdump_log(TDS_DBG_INFO1, "tds_process_tokens(). set current_results to cursor->res_info\n");
            } else {
                /* assure that we point to row, not to compute */
                if (tds->res_info)
                    tds->current_results = tds->res_info;
            }
            /* I don't know when this it's false but it happened, also server can send garbage... */
            if (tds->current_results)
                tds->current_results->rows_exist = 1;
            SET_RETURN(TDS_ROW_RESULT, ROW);

            rc = tds_process_row(tds);
            break;
        case TDS_CMP_ROW_TOKEN:
            /* I don't know when this it's false but it happened, also server can send garbage... */
            if (tds->res_info)
                tds->res_info->rows_exist = 1;
            SET_RETURN(TDS_COMPUTE_RESULT, COMPUTE);
            rc = tds_process_compute(tds, NULL);
            break;
        case TDS_RETURNSTATUS_TOKEN:
            ret_status = tds_get_int(tds);
            marker = tds_peek(tds);
            if (marker != TDS_PARAM_TOKEN
                &&  marker != TDS_DONEPROC_TOKEN
                &&  marker != TDS_DONE_TOKEN
                &&  marker != TDS5_PARAMFMT2_TOKEN)
            {
                break;
            }
            if (tds->internal_sp_called) {
                /* TODO perhaps we should use ret_status ?? */
            } else {
                /* TODO optimize */
                flag &= ~TDS_STOPAT_PROC;
                SET_RETURN(TDS_STATUS_RESULT, PROC);
                tds->has_status = 1;
                tds->ret_status = ret_status;
                tdsdump_log(TDS_DBG_FUNC, "tds_process_tokens: return status is %d\n", tds->ret_status);
                rc = TDS_SUCCEED;
            }
            break;
        case TDS5_DYNAMIC_TOKEN:
            /* process acknowledge dynamic */
            tds->cur_dyn = tds_process_dynamic(tds);
            /* special case, prepared statement cannot be prepared */
            if (!tds->cur_dyn || tds->cur_dyn->emulated)
                break;
            marker = tds_get_byte(tds);
            if (marker != TDS_EED_TOKEN) {
                tds_unget_byte(tds);
                break;
            }
            tds_process_msg(tds, marker);
            if (!tds->cur_dyn || !tds->cur_dyn->emulated)
                break;
            marker = tds_get_byte(tds);
            if (marker != TDS_DONE_TOKEN) {
                tds_unget_byte(tds);
                break;
            }
            rc = tds_process_end(tds, marker, done_flags);
            if (done_flags)
                *done_flags &= ~TDS_DONE_ERROR;
            /* FIXME warning to macro expansion */
            SET_RETURN(TDS_DONE_RESULT, DONE);
            break;
        case TDS5_PARAMFMT_TOKEN:
            SET_RETURN(TDS_DESCRIBE_RESULT, PARAMFMT);
            rc = tds_process_dyn_result(tds);
            break;
        case TDS5_PARAMFMT2_TOKEN:
            SET_RETURN(TDS_DESCRIBE_RESULT, PARAMFMT);
            rc = tds5_process_dyn_result2(tds);
            break;
        case TDS5_PARAMS_TOKEN:
            SET_RETURN(TDS_PARAM_RESULT, PROC);
            rc = tds_process_params_result_token(tds);
            break;
        case TDS_CURINFO_TOKEN:
            rc = tds_process_cursor_tokens(tds);
            break;
        case TDS_DONE_TOKEN:
            SET_RETURN(TDS_DONE_RESULT, DONE);
            rc = tds_process_end(tds, marker, done_flags);
            break;
        case TDS_DONEPROC_TOKEN:
            SET_RETURN(TDS_DONEPROC_RESULT, DONE);
            rc = tds_process_end(tds, marker, done_flags);
            tds->rows_affected = saved_rows_affected; /* ssikorsk */
            switch (tds->internal_sp_called) {
            case 0:
            case TDS_SP_PREPARE:
            case TDS_SP_EXECUTE:
            case TDS_SP_UNPREPARE:
            case TDS_SP_EXECUTESQL:
                break;
            case TDS_SP_CURSOROPEN:
                *result_type       = TDS_DONE_RESULT;
                tds->rows_affected = saved_rows_affected;
                break;
            case TDS_SP_CURSORCLOSE:
                tdsdump_log(TDS_DBG_FUNC, "TDS_SP_CURSORCLOSE\n");
                if (tds->cur_cursor) {

                    TDSCURSOR  *cursor = tds->cur_cursor;

                    cursor->srv_status &= ~TDS_CUR_ISTAT_OPEN;
                    cursor->srv_status |= TDS_CUR_ISTAT_CLOSED;
                    cursor->srv_status |= TDS_CUR_ISTAT_DECLARED;
                    if (cursor->status.dealloc == 2) {
                        tds_free_cursor(tds, cursor);
                    }
                    /* 0.95
                    cursor->srv_status |= TDS_CUR_ISTAT_CLOSED|TDS_CUR_ISTAT_DECLARED;
                    if (cursor->status.dealloc == TDS_CURSOR_STATE_SENT) {
                        tds_cursor_deallocated(tds, cursor);
                    }
                    */
                }
                *result_type = TDS_NO_MORE_RESULTS;
                rc = TDS_NO_MORE_RESULTS;
                break;
            default:
                *result_type = TDS_NO_MORE_RESULTS;
                rc = TDS_NO_MORE_RESULTS;
                break;
            }
            break;
        case TDS_DONEINPROC_TOKEN:
            switch(tds->internal_sp_called) {
            case TDS_SP_CURSOROPEN:
            case TDS_SP_CURSORFETCH:
            case TDS_SP_PREPARE:
            case TDS_SP_CURSORCLOSE:
                rc = tds_process_end(tds, marker, done_flags);
                if (tds->rows_affected != TDS_NO_COUNT) {
                    saved_rows_affected = tds->rows_affected;
                }
                break;
            default:
                SET_RETURN(TDS_DONEINPROC_RESULT, DONE);
                rc = tds_process_end(tds, marker, done_flags);
                break;
            }
            break;
        case TDS_ERROR_TOKEN:
        case TDS_INFO_TOKEN:
        case TDS_EED_TOKEN:
            SET_RETURN(TDS_MSG_RESULT, MSG);
            rc = tds_process_default_tokens(tds, marker);
            break;
        default:
            SET_RETURN(TDS_OTHERS_RESULT, OTHERS);
            rc = tds_process_default_tokens(tds, marker);
            break;
        }

        if (rc == TDS_FAIL) {
            tds_set_state(tds, TDS_PENDING);
            return rc;
        }

        cancel_seen |= tds->in_cancel;
        if (cancel_seen) {
            /* during cancel handle all tokens */
            flag = TDS_HANDLE_ALL;
        }

        if ((return_flag & flag) != 0) {
            tds_set_state(tds, TDS_PENDING);
            return rc;
        }

        if (tds->state == TDS_IDLE)
            return cancel_seen ? TDS_CANCELLED : TDS_NO_MORE_RESULTS;

        if (tds->state == TDS_DEAD) {
            /* TODO free all results ?? */
            return TDS_FAIL;
        }
    }
}

/**
 * Process results for simple query as "SET TEXTSIZE" or "USE dbname"
 * If the statement returns results, beware they are discarded.
 *
 * This function was written to avoid direct calls to tds_process_default_tokens
 * (which caused problems such as ignoring query errors).
 * Results are read until idle state or severe failure (do not stop for
 * statement failure).
 * @return see tds_process_tokens for results (TDS_NO_MORE_RESULTS is never returned)
 */
int
tds_process_simple_query(TDSSOCKET * tds)
{
    TDS_INT res_type;
    TDS_INT done_flags;
    int     rc;
    int     ret = TDS_SUCCEED;

    CHECK_TDS_EXTRA(tds);

    while ((rc = tds_process_tokens(tds, &res_type, &done_flags, TDS_RETURN_DONE)) == TDS_SUCCEED) {
        switch (res_type) {

            case TDS_DONE_RESULT:
            case TDS_DONEPROC_RESULT:
            case TDS_DONEINPROC_RESULT:
                if ((done_flags & TDS_DONE_ERROR) != 0)
                    ret = TDS_FAIL;
                break;

            default:
                break;
        }
    }
    if (rc != TDS_NO_MORE_RESULTS) {
        ret = TDS_FAIL;
    }

    return ret;
}

/**
 * tds_process_col_name() is one half of the result set under TDS 4.2
 * it contains all the column names, a TDS_COLFMT_TOKEN should
 * immediately follow this token with the datatype/size information
 * This is a 4.2 only function
 */
static int
tds_process_col_name(TDSSOCKET * tds)
{
    int hdrsize, len = 0;
    int memrc = 0;
    int col, num_cols = 0;
    struct tmp_col_struct
    {
        char *column_name;
        int column_namelen;
        struct tmp_col_struct *next;
    };
    struct tmp_col_struct *head = NULL, *cur = NULL, *prev;
    TDSCOLUMN *curcol;
    TDSRESULTINFO *info;

    CHECK_TDS_EXTRA(tds);

    hdrsize = tds_get_smallint(tds);

    /*
     * this is a little messy...TDS 5.0 gives the number of columns
     * upfront, while in TDS 4.2, you're expected to figure it out
     * by the size of the message. So, I use a link list to get the
     * colum names and then allocate the result structure, copy
     * and delete the linked list
     */
    /*
     * TODO reallocate columns
     * TODO code similar below, function to reuse
     */
    while (len < hdrsize) {
        prev = cur;
        cur = (struct tmp_col_struct *)
            malloc(sizeof(struct tmp_col_struct));

        if (!cur) {
            memrc = -1;
            break;
        }

        if (prev)
            prev->next = cur;
        if (!head)
            head = cur;

        cur->column_namelen = tds_get_byte(tds);
        memrc += tds_alloc_get_string(tds, &cur->column_name, cur->column_namelen);
        cur->next = NULL;

        len += cur->column_namelen + 1;
        num_cols++;
    }

    /* free results/computes/params etc... */
    tds_free_all_results(tds);
    tds->rows_affected = TDS_NO_COUNT;

    if ((info = tds_alloc_results(num_cols)) == NULL)
        memrc = -1;
    tds->current_results = tds->res_info = info;

    cur = head;

    if (memrc != 0) {
        while (cur != NULL) {
            prev = cur;
            cur = cur->next;
            free(prev->column_name);
            free(prev);
        }
        return TDS_FAIL;
    } else {
        for (col = 0; col < info->num_cols; col++) {
            curcol = info->columns[col];
            tds_strlcpy(curcol->column_name, cur->column_name, sizeof(curcol->column_name));
            curcol->column_namelen = strlen(curcol->column_name);
            prev = cur;
            cur = cur->next;
            free(prev->column_name);
            free(prev);
        }
        return TDS_SUCCEED;
    }
}

/**
 * Add a column size to result info row size and calc offset into row
 * @param info   result where to add column
 * @param curcol column to add
 */
void
tds_add_row_column_size(TDSRESULTINFO * info, TDSCOLUMN * curcol)
{
    CHECK_RESULTINFO_EXTRA(info);
    CHECK_COLUMN_EXTRA(curcol);

    /*
     * the column_offset is the offset into the row buffer
     * where this column begins, text types are no longer
     * stored in the row buffer because the max size can
     * be too large (2gig) to allocate
     */
    curcol->column_offset = info->row_size;
    if (is_numeric_type(curcol->column_type)) {
        info->row_size += sizeof(TDS_NUMERIC);
    } else if (is_blob_type(curcol->column_type)) {
        info->row_size += sizeof(TDSBLOB);
    } else {
        info->row_size += curcol->column_size;
    }

    /* Added by ssikorsk */
    info->row_size += curcol->column_varint_size;

    info->row_size += (TDS_ALIGN_SIZE - 1);
    info->row_size -= info->row_size % TDS_ALIGN_SIZE;
}

/**
 * tds_process_col_fmt() is the other half of result set processing
 * under TDS 4.2. It follows tds_process_col_name(). It contains all the
 * column type and size information.
 * This is a 4.2 only function
 */
static int
tds_process_col_fmt(TDSSOCKET * tds)
{
    int col, hdrsize;
    TDSCOLUMN *curcol;
    TDSRESULTINFO *info;
    /* TDS_SMALLINT tabnamesize; */
    int bytes_read = 0;
    int rest;
    TDS_SMALLINT flags;

    CHECK_TDS_EXTRA(tds);

    hdrsize = tds_get_smallint(tds);

    /* TODO use current_results instead of res_info ?? */
    info = tds->res_info;
    for (col = 0; col < info->num_cols; col++) {
        curcol = info->columns[col];
        /* In Sybase all 4 byte are used for usertype, while mssql place 2 byte as usertype and 2 byte as flags */
        if (TDS_IS_MSSQL(tds)) {
            curcol->column_usertype = tds_get_smallint(tds);
            flags = tds_get_smallint(tds);
            curcol->column_nullable = flags & 0x01;
            curcol->column_writeable = (flags & 0x08) > 0;
            curcol->column_identity = (flags & 0x10) > 0;
        } else {
            curcol->column_usertype = tds_get_int(tds);
        }
        /* on with our regularly scheduled code (mlilback, 11/7/01) */
        tds_set_column_type(tds, curcol, tds_get_byte(tds));

        tdsdump_log(TDS_DBG_INFO1, "processing result. type = %d(%s), varint_size %d\n",
                curcol->column_type, tds_prtype(curcol->column_type), curcol->column_varint_size);

        switch (curcol->column_varint_size) {
        case 4:
            curcol->column_size = tds_get_int(tds);
            /* junk the table name -- for now
            tabnamesize = tds_get_smallint(tds);
            tds_get_n(tds, NULL, tabnamesize);
            bytes_read += 5 + 4 + 2 + tabnamesize;
            */
            /* ssikorsk */
            curcol->table_namelen = tds_get_smallint(tds);
            tds_get_n(tds, curcol->table_name, curcol->table_namelen);
            bytes_read += 5 + 4 + 2 + curcol->table_namelen;
            break;
        case 1:
            curcol->column_size = tds_get_byte(tds);
            bytes_read += 5 + 1;
            break;
        case 0:
            bytes_read += 5 + 0;
            break;
        }

        /* Adjust column size according to client's encoding */
        curcol->on_server.column_size = curcol->column_size;
        adjust_character_column_size(tds, curcol);

        tds_add_row_column_size(info, curcol);
    }

    /* get the rest of the bytes */
    rest = hdrsize - bytes_read;
    if (rest > 0) {
        tdsdump_log(TDS_DBG_INFO1, "NOTE:tds_process_col_fmt: draining %d bytes\n", rest);
        tds_get_n(tds, NULL, rest);
    }

    if ((info->current_row = tds_alloc_row(info)) != NULL)
        return TDS_SUCCEED;
    else
        return TDS_FAIL;
}

static int
tds_process_colinfo(TDSSOCKET * tds)
{
    int hdrsize;
    TDSCOLUMN *curcol;
    TDSRESULTINFO *info;
    int bytes_read = 0;
    unsigned char col_info[3], l;

    CHECK_TDS_EXTRA(tds);

    hdrsize = tds_get_smallint(tds);

    info = tds->current_results;

    while (bytes_read < hdrsize) {

        tds_get_n(tds, col_info, 3);
        bytes_read += 3;
        if (info && col_info[0] > 0 && col_info[0] <= info->num_cols) {
            curcol = info->columns[col_info[0] - 1];
            curcol->column_writeable = (col_info[2] & 0x4) == 0;
            curcol->column_key = (col_info[2] & 0x8) > 0;
            curcol->column_hidden = (col_info[2] & 0x10) > 0;
        }
        /* skip real name */
        /* TODO keep it */
        if (col_info[2] & 0x20) {
            l = tds_get_byte(tds);
            if (IS_TDS7_PLUS(tds))
                l *= 2;
            tds_get_n(tds, NULL, l);
            bytes_read += l + 1;
        }
    }

    return TDS_SUCCEED;
}

/**
 * tds_process_param_result() processes output parameters of a stored
 * procedure. This differs from regular row/compute results in that there
 * is no total number of parameters given, they just show up singly.
 */
static int
tds_process_param_result(TDSSOCKET * tds, TDSPARAMINFO ** pinfo)
{
    int hdrsize;
    TDSCOLUMN *curparam;
    TDSPARAMINFO *info;
    int i;

    CHECK_TDS_EXTRA(tds);
    if (*pinfo)
        CHECK_PARAMINFO_EXTRA(*pinfo);

    /* TODO check if current_results is a param result */

    /* limited to 64K but possible types are always smaller (not TEXT/IMAGE) */
    hdrsize = tds_get_smallint(tds);
    if ((info = tds_alloc_param_result(*pinfo)) == NULL)
        return TDS_FAIL;

    *pinfo = info;
    curparam = info->columns[info->num_cols - 1];

    /*
     * FIXME check support for tds7+ (seem to use same format of tds5 for data...)
     * perhaps varint_size can be 2 or collation can be specified ??
     */
    tds_get_data_info(tds, curparam, 1);

    curparam->column_cur_size = curparam->column_size;  /* needed ?? */

    if (tds_alloc_param_row(info, curparam) == NULL)
        return TDS_FAIL;

    i = tds_get_data(tds, curparam, info->current_row, info->num_cols - 1);

    /*
     * Real output parameters will either be unnamed or will have a valid
     * parameter name beginning with '@'. Ignore any other Spurious parameters
     * such as those returned from calls to writetext in the proc.
     */
    if (curparam->column_namelen > 0 && curparam->column_name[0] != '@')
        tds_free_param_result(*pinfo);

    return i;
}

static int
tds_process_param_result_tokens(TDSSOCKET * tds)
{
    int marker;
    TDSPARAMINFO **pinfo;

    CHECK_TDS_EXTRA(tds);

    if (tds->cur_dyn)
        pinfo = &(tds->cur_dyn->res_info);
    else
        pinfo = &(tds->param_info);

    while ((marker = tds_get_byte(tds)) == TDS_PARAM_TOKEN) {
        tds_process_param_result(tds, pinfo);
    }
    if (!marker) {
        tdsdump_log(TDS_DBG_FUNC, "error: tds_process_param_result() returned TDS_FAIL\n");
        return TDS_FAIL;
    }

    tds->current_results = *pinfo;
    tds_unget_byte(tds);
    return TDS_SUCCEED;
}

/**
 * tds_process_params_result_token() processes params on TDS5.
 */
static int
tds_process_params_result_token(TDSSOCKET * tds)
{
    int i;
    TDSCOLUMN *curcol;
    TDSPARAMINFO *info;

    CHECK_TDS_EXTRA(tds);

    /* TODO check if current_results is a param result */
    info = tds->current_results;
    if (!info)
        return TDS_FAIL;

    for (i = 0; i < info->num_cols; i++) {
        curcol = info->columns[i];
        if (tds_get_data(tds, curcol, info->current_row, i) != TDS_SUCCEED)
            return TDS_FAIL;
    }
    return TDS_SUCCEED;
}

/**
 * tds_process_compute_result() processes compute result sets.  These functions
 * need work but since they get little use, nobody has complained!
 * It is very similar to normal result sets.
 */
static int
tds_process_compute_result(TDSSOCKET * tds)
{
    int hdrsize;
    int col, num_cols;
    TDS_TINYINT by_cols = 0;
    TDS_SMALLINT *cur_by_col;
    TDS_SMALLINT compute_id = 0;
    TDSCOLUMN *curcol;
    TDSCOMPUTEINFO *info;
    int i;

    CHECK_TDS_EXTRA(tds);

    hdrsize = tds_get_smallint(tds);

    /*
     * compute statement id which this relates
     * to. You can have more than one compute
     * statement in a SQL statement
     */

    compute_id = tds_get_smallint(tds);

    tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. compute_id = %d\n", compute_id);

    /*
     * number of compute columns returned - so
     * COMPUTE SUM(x), AVG(x)... would return
     * num_cols = 2
     */

    num_cols = tds_get_byte(tds);

    for (i = 0;; ++i) {
        if (i >= tds->num_comp_info)
            return TDS_FAIL;
        info = tds->comp_info[i];
        tdsdump_log(TDS_DBG_FUNC, "in dbaltcolid() found computeid = %d\n", info->computeid);
        if (info->computeid == compute_id)
            break;
    }

    tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. num_cols = %d\n", num_cols);

    for (col = 0; col < num_cols; col++) {
        tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. point 2\n");
        curcol = info->columns[col];

        curcol->column_operator = tds_get_byte(tds);
        curcol->column_operand = tds_get_byte(tds);

        /*
         * if no name has been defined for the compute column,
         * put in "max", "avg" etc.
         */

        if (curcol->column_namelen == 0) {
            strcpy(curcol->column_name, tds_pr_op(curcol->column_operator));
            curcol->column_namelen = strlen(curcol->column_name);
        }

        /*  User defined data type of the column */
        curcol->column_usertype = tds_get_int(tds);

        tds_set_column_type(tds, curcol, tds_get_byte(tds));

        switch (curcol->column_varint_size) {
        case 4:
            curcol->column_size = tds_get_int(tds);
            break;
        case 2:
            curcol->column_size = tds_get_smallint(tds);
            break;
        case 1:
            curcol->column_size = tds_get_byte(tds);
            break;
        case 0:
            break;
        }
        tdsdump_log(TDS_DBG_INFO1, "processing result. column_size %d\n", curcol->column_size);

        /* Adjust column size according to client's encoding */
        curcol->on_server.column_size = curcol->column_size;
        /* TODO check if this column can have collation information associated */
        adjust_character_column_size(tds, curcol);

        /* skip locale */
        if (!IS_TDS42(tds))
            tds_get_n(tds, NULL, tds_get_byte(tds));

        tds_add_row_column_size(info, curcol);
    }

    by_cols = tds_get_byte(tds);

    tdsdump_log(TDS_DBG_INFO1, "processing tds compute result. by_cols = %d\n", by_cols);

    if (by_cols) {
        if ((info->bycolumns = (TDS_SMALLINT *) malloc(by_cols * sizeof(TDS_SMALLINT))) == NULL)
            return TDS_FAIL;

        memset(info->bycolumns, '\0', by_cols * sizeof(TDS_SMALLINT));
    }
    info->by_cols = by_cols;

    cur_by_col = info->bycolumns;
    for (col = 0; col < by_cols; col++) {
        *cur_by_col = tds_get_byte(tds);
        cur_by_col++;
    }

    if ((info->current_row = tds_alloc_compute_row(info)) != NULL)
        return TDS_SUCCEED;
    else
        return TDS_FAIL;
}

/**
 * Read data information from wire
 * \param tds state information for the socket and the TDS protocol
 * \param curcol column where to store information
 */
static int
tds7_get_data_info(TDSSOCKET * tds, TDSCOLUMN * curcol)
{
    int colnamelen;

    CHECK_TDS_EXTRA(tds);
    CHECK_COLUMN_EXTRA(curcol);

    /*  User defined data type of the column */
    curcol->column_usertype = tds_get_smallint(tds);

    curcol->column_flags = tds_get_smallint(tds);   /*  Flags */

    curcol->column_nullable = curcol->column_flags & 0x01;
    curcol->column_writeable = (curcol->column_flags & 0x08) > 0;
    curcol->column_identity = (curcol->column_flags & 0x10) > 0;

    tds_set_column_type(tds, curcol, tds_get_byte(tds)); /* sets "cardinal" type */

    curcol->column_timestamp = (curcol->column_type == SYBBINARY && curcol->column_usertype == TDS_UT_TIMESTAMP);

    switch (curcol->column_varint_size) {
    case 4:
        curcol->column_size = tds_get_int(tds);
        break;
    case 2:
        curcol->column_size = tds_get_smallint(tds);
        break;
    case 1:
        curcol->column_size = tds_get_byte(tds);
        break;
    case 0:
        break;
    }

    /* Adjust column size according to client's encoding */
    curcol->on_server.column_size = curcol->column_size;

    /* numeric and decimal have extra info */
    if (is_numeric_type(curcol->column_type)) {
        curcol->column_prec = tds_get_byte(tds);    /* precision */
        curcol->column_scale = tds_get_byte(tds);   /* scale */
        /* FIXME check prec/scale, don't let server crash us */
    }

    if (IS_TDS80(tds) && is_collate_type(curcol->on_server.column_type)) {
        /* based on true type as sent by server */
        /*
         * first 2 bytes are windows code (such as 0x409 for english)
         * other 2 bytes ???
         * last bytes is id in syscharsets
         */
        tds_get_n(tds, curcol->column_collation, 5);
        curcol->char_conv =
            tds_iconv_from_collate(tds, curcol->column_collation[4],
                           curcol->column_collation[1] * 256 + curcol->column_collation[0]);
    }

    /* NOTE adjustements must be done after curcol->char_conv initialization */
    adjust_character_column_size(tds, curcol);

    if (is_blob_type(curcol->column_type)) {
        curcol->table_namelen =
            tds_get_string(tds, tds_get_smallint(tds), curcol->table_name, sizeof(curcol->table_name) - 1);
    }

    /*
     * under 7.0 lengths are number of characters not
     * number of bytes...tds_get_string handles this
     */
    colnamelen = tds_get_string(tds, tds_get_byte(tds), curcol->column_name, sizeof(curcol->column_name) - 1);
    curcol->column_name[colnamelen] = 0;
    curcol->column_namelen = colnamelen;

    tdsdump_log(TDS_DBG_INFO1, "tds7_get_data_info: \n"
            "\tcolname = %s (%d bytes)\n"
            "\ttype = %d (%s)\n"
            "\tserver's type = %d (%s)\n"
            "\tcolumn_varint_size = %d\n"
            "\tcolumn_size = %d (%d on server)\n",
            curcol->column_name, curcol->column_namelen,
            curcol->column_type, tds_prtype(curcol->column_type),
            curcol->on_server.column_type, tds_prtype(curcol->on_server.column_type),
            curcol->column_varint_size,
            curcol->column_size, curcol->on_server.column_size);

    CHECK_COLUMN_EXTRA(curcol);

    return TDS_SUCCEED;
}

/**
 * tds7_process_result() is the TDS 7.0 result set processing routine.  It
 * is responsible for populating the tds->res_info structure.
 * This is a TDS 7.0 only function
 */
static int
tds7_process_result(TDSSOCKET * tds)
{
    int col, num_cols;
    TDSCOLUMN *curcol;
    TDSRESULTINFO *info;

    CHECK_TDS_EXTRA(tds);
    tdsdump_log(TDS_DBG_INFO1, "processing TDS7 result metadata.\n");

    /* read number of columns and allocate the columns structure */

    num_cols = tds_get_smallint(tds);

    /* This can be a DUMMY results token from a cursor fetch */

    if (num_cols == -1) {
        tdsdump_log(TDS_DBG_INFO1, "no meta data\n");
        return TDS_SUCCEED;
    }

    tds_free_all_results(tds);
    tds->rows_affected = TDS_NO_COUNT;

    if ((info = tds_alloc_results(num_cols)) == NULL)
        return TDS_FAIL;
    tds->current_results = info;
    if (tds->cur_cursor) {
        tds->cur_cursor->res_info = info;
        tdsdump_log(TDS_DBG_INFO1, "set current_results to cursor->res_info\n");
    } else {
        tds->res_info = info;
        tdsdump_log(TDS_DBG_INFO1, "set current_results (%d column%s) to tds->res_info\n", num_cols, (num_cols==1? "":"s"));
    }


    /*
     * loop through the columns populating COLINFO struct from
     * server response
     */
    for (col = 0; col < num_cols; col++) {

        curcol = info->columns[col];

        tdsdump_log(TDS_DBG_INFO1, "setting up column %d\n", col);
        tds7_get_data_info(tds, curcol);

        tds_add_row_column_size(info, curcol);
    }

    /* all done now allocate a row for tds_process_row to use */
    if ((info->current_row = tds_alloc_row(info)) != NULL) {
        CHECK_TDS_EXTRA(tds);
        return TDS_SUCCEED;
    } else {
        CHECK_TDS_EXTRA(tds);
        return TDS_FAIL;
    }
}

/**
 * Read data information from wire
 * \param tds state information for the socket and the TDS protocol
 * \param curcol column where to store information
 */
static int
tds_get_data_info(TDSSOCKET * tds, TDSCOLUMN * curcol, int is_param)
{
    CHECK_TDS_EXTRA(tds);
    CHECK_COLUMN_EXTRA(curcol);

    curcol->column_namelen = tds_get_string(tds, tds_get_byte(tds), curcol->column_name, sizeof(curcol->column_name) - 1);
    curcol->column_name[curcol->column_namelen] = '\0';

    curcol->column_flags = tds_get_byte(tds);   /*  Flags */
    if (!is_param) {
        /* TODO check if all flags are the same for all TDS versions */
        if (IS_TDS50(tds))
            curcol->column_hidden = curcol->column_flags & 0x1;
        curcol->column_key = (curcol->column_flags & 0x2) > 1;
        curcol->column_writeable = (curcol->column_flags & 0x10) > 1;
        curcol->column_nullable = (curcol->column_flags & 0x20) > 1;
        curcol->column_identity = (curcol->column_flags & 0x40) > 1;
    }

    curcol->column_usertype = tds_get_int(tds);
    tds_set_column_type(tds, curcol, tds_get_byte(tds));

    tdsdump_log(TDS_DBG_INFO1, "processing result. type = %d(%s), varint_size %d\n",
            curcol->column_type, tds_prtype(curcol->column_type), curcol->column_varint_size);
    switch (curcol->column_varint_size) {
    case 4:
        curcol->column_size = tds_get_int(tds);
        /* Only read table_name for blob columns (eg. not for SYBLONGBINARY) */
        if (is_blob_type (curcol->column_type)) {
            curcol->table_namelen =
                tds_get_string(tds, tds_get_smallint(tds), curcol->table_name, sizeof(curcol->table_name) - 1);
        }
        break;
    case 2:
        /* assure > 0 */
        curcol->column_size = tds_get_smallint(tds);
        break;
    case 1:
        curcol->column_size = tds_get_byte(tds);
        break;
    case 0:
        break;
    }
    tdsdump_log(TDS_DBG_INFO1, "processing result. column_size %d\n", curcol->column_size);

    /* numeric and decimal have extra info */
    if (is_numeric_type(curcol->column_type)) {
        curcol->column_prec = tds_get_byte(tds);    /* precision */
        curcol->column_scale = tds_get_byte(tds);   /* scale */
        /* FIXME check prec/scale, don't let server crash us */
    }

    /* read sql collation info */
    /* TODO: we should use it ! */
    if (IS_TDS80(tds) && is_collate_type(curcol->on_server.column_type)) {
        tds_get_n(tds, curcol->column_collation, 5);
        curcol->char_conv =
            tds_iconv_from_collate(tds, curcol->column_collation[4],
                           curcol->column_collation[1] * 256 + curcol->column_collation[0]);
    }

    /* Adjust column size according to client's encoding */
    curcol->on_server.column_size = curcol->column_size;
    adjust_character_column_size(tds, curcol);

    return TDS_SUCCEED;
}

/**
 * tds_process_result() is the TDS 5.0 result set processing routine.  It
 * is responsible for populating the tds->res_info structure.
 * This is a TDS 5.0 only function
 */
static int
tds_process_result(TDSSOCKET * tds)
{
    int hdrsize;
    int col, num_cols;
    TDSCOLUMN *curcol;
    TDSRESULTINFO *info;

    CHECK_TDS_EXTRA(tds);

    tds_free_all_results(tds);
    tds->rows_affected = TDS_NO_COUNT;

    hdrsize = tds_get_smallint(tds);

    /* read number of columns and allocate the columns structure */
    num_cols = tds_get_smallint(tds);

    if ((info = tds_alloc_results(num_cols)) == NULL)
        return TDS_FAIL;
    tds->current_results = info;
    if (tds->cur_cursor) {
        tds->cur_cursor->res_info = info;
    } else {
        tds->res_info = info;
    }

    /*
     * loop through the columns populating COLINFO struct from
     * server response
     */
    for (col = 0; col < info->num_cols; col++) {
        curcol = info->columns[col];

        tds_get_data_info(tds, curcol, 0);

        /* skip locale information */
        /* NOTE do not put into tds_get_data_info, param do not have locale information */
        tds_get_n(tds, NULL, tds_get_byte(tds));

        tds_add_row_column_size(info, curcol);
    }
    if ((info->current_row = tds_alloc_row(info)) != NULL)
        return TDS_SUCCEED;
    else
        return TDS_FAIL;
}

/**
 * tds5_process_result() is the new TDS 5.0 result set processing routine.
 * It is responsible for populating the tds->res_info structure.
 * This is a TDS 5.0 only function
 */
static int
tds5_process_result(TDSSOCKET * tds)
{
    int hdrsize;

    int colnamelen;
    int col, num_cols;
    TDSCOLUMN *curcol;
    TDSRESULTINFO *info;

    CHECK_TDS_EXTRA(tds);

    tdsdump_log(TDS_DBG_INFO1, "tds5_process_result\n");

    /*
     * free previous resultset
     */
    tds_free_all_results(tds);
    tds->rows_affected = TDS_NO_COUNT;

    /*
     * read length of packet (4 bytes)
     */
    hdrsize = tds_get_int(tds);

    /* read number of columns and allocate the columns structure */
    num_cols = tds_get_smallint(tds);

    if ((info = tds_alloc_results(num_cols)) == NULL)
        return TDS_FAIL;
    tds->current_results = info;
    if (tds->cur_cursor) {
        tds->cur_cursor->res_info = info;
    }
    else
        tds->res_info = info;

    tdsdump_log(TDS_DBG_INFO1, "num_cols=%d\n", num_cols);

    /* TODO reuse some code... */
    /*
     * loop through the columns populating COLINFO struct from
     * server response
     */
    for (col = 0; col < info->num_cols; col++) {
        curcol = info->columns[col];

        /* label */
        curcol->column_namelen =
            tds_get_string(tds, tds_get_byte(tds), curcol->column_name, sizeof(curcol->column_name) - 1);
        curcol->column_name[curcol->column_namelen] = '\0';

        /* TODO save informations somewhere */
        /* database */
        colnamelen = tds_get_byte(tds);
        tds_get_n(tds, NULL, colnamelen);
        /*
         * tds_get_n(tds, curcol->catalog_name, colnamelen);
         * curcol->catalog_name[colnamelen] = '\0';
         */

        /* owner */
        colnamelen = tds_get_byte(tds);
        tds_get_n(tds, NULL, colnamelen);
        /*
         * tds_get_n(tds, curcol->schema_name, colnamelen);
         * curcol->schema_name[colnamelen] = '\0';
         */

        /* table */
        curcol->table_namelen =
            tds_get_string(tds, tds_get_byte(tds), curcol->table_name, sizeof(curcol->table_name) - 1);
        curcol->table_name[curcol->table_namelen] = '\0';

        /* column name */
        colnamelen = tds_get_byte(tds);
        if (colnamelen != 0) {
            if (curcol->column_namelen == 0) {
                tds_get_string(tds, colnamelen, curcol->column_name, sizeof(curcol->column_name) - 1);
                curcol->column_name[colnamelen] = '\0';
                curcol->column_namelen = colnamelen;
            }
            else {
                tds_get_n(tds, NULL, colnamelen);
            }
        }
        /*
        if (curcol->table_column_name)
            TDS_ZERO_FREE(curcol->table_column_name);
        tds_alloc_get_string(tds, &curcol->table_column_name, tds_get_byte(tds));
        */

        /* if label is empty, use the table column name */
        /*
        if (!curcol->column_namelen && curcol->table_column_name) {
            tds_strlcpy(curcol->column_name, curcol->table_column_name, sizeof(curcol->column_name));
            curcol->column_namelen = strlen(curcol->column_name);
        }
        */

        /* flags (4 bytes) */
        curcol->column_flags = tds_get_int(tds);
        curcol->column_hidden = curcol->column_flags & 0x1;
        curcol->column_key = (curcol->column_flags & 0x2) > 1;
        curcol->column_writeable = (curcol->column_flags & 0x10) > 1;
        curcol->column_nullable = (curcol->column_flags & 0x20) > 1;
        curcol->column_identity = (curcol->column_flags & 0x40) > 1;

        curcol->column_usertype = tds_get_int(tds);

        tds_set_column_type(tds, curcol, tds_get_byte(tds));

        switch (curcol->column_varint_size) {
        case 4:
            if (curcol->column_type == SYBTEXT || curcol->column_type == SYBIMAGE) {
                int namelen;

                curcol->column_size = tds_get_int(tds);

                /* skip name */
                namelen = tds_get_smallint(tds);
                if (namelen)
                    tds_get_n(tds, NULL, namelen);

            } else {
                curcol->column_size = tds_get_int(tds);
            }
            break;
        case 2:
            curcol->column_size = tds_get_smallint(tds);
            break;
        case 1:
            curcol->column_size = tds_get_byte(tds);
            break;
        case 0:
            curcol->column_size = tds_get_size_by_type(curcol->column_type);
            break;
        }

        /* numeric and decimal have extra info */
        if (is_numeric_type(curcol->column_type)) {
            curcol->column_prec = tds_get_byte(tds);    /* precision */
            curcol->column_scale = tds_get_byte(tds);   /* scale */
            /* FIXME check prec/scale, don't let server crash us */
        }

        /* Adjust column size according to client's encoding */
        curcol->on_server.column_size = curcol->column_size;
        adjust_character_column_size(tds, curcol);

        /* discard Locale */
        tds_get_n(tds, NULL, tds_get_byte(tds));

        tds_add_row_column_size(info, curcol);

        /*
         *  Dump all information on this column
         */
        tdsdump_log(TDS_DBG_INFO1, "col %d:\n", col);
        tdsdump_log(TDS_DBG_INFO1, "tcolumn_label=[%s]\n", curcol->column_name);
/*
        tdsdump_log(TDS_DBG_INFO1, "\tcolumn_name=[%s]\n", curcol->column_colname);
        tdsdump_log(TDS_DBG_INFO1, "\tcatalog=[%s] schema=[%s] table=[%s]\n",
                curcol->catalog_name, curcol->schema_name, curcol->table_name, curcol->column_colname);
*/
        tdsdump_log(TDS_DBG_INFO1, "\tflags=%x utype=%d type=%d varint=%d\n",
                curcol->column_flags, curcol->column_usertype, curcol->column_type, curcol->column_varint_size);

        tdsdump_log(TDS_DBG_INFO1, "\tcolsize=%d prec=%d scale=%d\n",
                curcol->column_size, curcol->column_prec, curcol->column_scale);
    }
    if ((info->current_row = tds_alloc_row(info)) != NULL)
        return TDS_SUCCEED;
    else
        return TDS_FAIL;
}

/**
 * tds_process_compute() processes compute rows and places them in the row
 * buffer.
 */
static int
tds_process_compute(TDSSOCKET * tds, TDS_INT * computeid)
{
    int i;
    TDSCOLUMN *curcol;
    TDSCOMPUTEINFO *info;
    TDS_INT compute_id;

    CHECK_TDS_EXTRA(tds);

    compute_id = tds_get_smallint(tds);

    for (i = 0;; ++i) {
        if (i >= tds->num_comp_info)
            return TDS_FAIL;
        info = tds->comp_info[i];
        if (info->computeid == compute_id)
            break;
    }
    tds->current_results = info;

    for (i = 0; i < info->num_cols; i++) {
        curcol = info->columns[i];
        if (tds_get_data(tds, curcol, info->current_row, i) != TDS_SUCCEED)
            return TDS_FAIL;
    }
    if (computeid)
        *computeid = compute_id;
    return TDS_SUCCEED;
}

/*
static int
tds_process_compute_095(TDSSOCKET * tds, TDS_INT * pcomputeid)
{
    int i;
    TDSCOLUMN *curcol;
    TDSCOMPUTEINFO *info;
    TDS_INT id;

    CHECK_TDS_EXTRA(tds);

    id = tds_get_smallint(tds);

    tdsdump_log(TDS_DBG_INFO1, "tds_process_compute() found compute id %d\n", id);

    for (i = 0;; ++i) {
        if (i >= tds->num_comp_info) {
            tdsdump_log(TDS_DBG_INFO1, "tds_process_compute() FAIL: id exceeds bound (%d)\n", tds->num_comp_info);
            return TDS_FAIL;
        }
        info = tds->comp_info[i];
        if (info->computeid == id)
            break;
    }
    tds->current_results = info;

    for (i = 0; i < info->num_cols; i++) {
        curcol = info->columns[i];
        if (tds_get_data(tds, curcol, info->current_row, i) != TDS_SUCCEED) {
            tdsdump_log(TDS_DBG_INFO1, "tds_process_compute() FAIL: tds_get_data() failed\n");
            return TDS_FAIL;
        }
    }
    if (pcomputeid)
        *pcomputeid = id;
    return TDS_SUCCEED;
}
*/

/**
 * Read a data from wire
 * \param tds state information for the socket and the TDS protocol
 * \param curcol column where store column information
 * \param current_row pointer to row data to store information
 * \param i column position in current_row
 * \return TDS_FAIL on error or TDS_SUCCEED
 */
static int
tds_get_data(TDSSOCKET * tds, TDSCOLUMN * curcol, unsigned char *current_row, int i)
{
    unsigned char *dest;
    int len, colsize;
    int fillchar;
    TDSBLOB *blob = NULL;

    CHECK_TDS_EXTRA(tds);
    CHECK_COLUMN_EXTRA(curcol);

    tdsdump_log(TDS_DBG_INFO1, "tds_get_data: type %d, varint size %d\n", curcol->column_type, curcol->column_varint_size);
    switch (curcol->column_varint_size) {
    case 4:
        /*
         * TODO finish
         * This strange type has following structure
         * 0 len (int32) -- NULL
         * len (int32), type (int8), data -- ints, date, etc
         * len (int32), type (int8), 7 (int8), collation, column size (int16) -- [n]char, [n]varchar, binary, varbinary
         * BLOBS (text/image) not supported
         */
        if (curcol->column_type == SYBVARIANT) {
            colsize = tds_get_int(tds);
            tds_get_n(tds, NULL, colsize);
            curcol->column_cur_size = -1;
            return TDS_SUCCEED;
        }

        /* It's a BLOB... */
        if (is_blob_type(curcol->column_type)) {
            len = tds_get_byte(tds);
            blob = (TDSBLOB *) & (current_row[curcol->column_offset]);
            if (len == 16) {    /*  Jeff's hack */
                tds_get_n(tds, blob->textptr, 16);
                tds_get_n(tds, blob->timestamp, 8);
                colsize = tds_get_int(tds);
            } else {
                colsize = -1;
            }
            break;
        }

        /* Any other type (XSYBCHAR and alike) */
        colsize = tds_get_int(tds);
        if (colsize == 0)
            colsize = -1;
        break;
    case 2:
        colsize = tds_get_smallint(tds);
        break;
    case 1:
        colsize = tds_get_byte(tds);
        if (colsize == 0)
            colsize = -1;
        break;
    case 0:
        /* TODO this should be column_size */
        colsize = tds_get_size_by_type(curcol->column_type);
        break;
    default:
        colsize = -1;
        break;
    }
    if (IS_TDSDEAD(tds))
        return TDS_FAIL;

    tdsdump_log(TDS_DBG_INFO1, "processing row.  column size is %d \n", colsize);
    /* set NULL flag in the row buffer */
    if (colsize < 0) {
        curcol->column_cur_size = -1;
        return TDS_SUCCEED;
    }

    /*
     * We're now set to read the data from the wire.  For varying types (e.g. char/varchar)
     * make sure that curcol->column_cur_size reflects the size of the read data,
     * after any charset conversion.  tds_get_char_data() does that for you,
     * but of course tds_get_n() doesn't.
     *
     * colsize == wire_size, bytes to read
     * curcol->column_cur_size == sizeof destination buffer, room to write
     */
    dest = &(current_row[curcol->column_offset]);
    if (is_numeric_type(curcol->column_type)) {
        /*
         * Handling NUMERIC datatypes:
         * Since these can be passed around independent
         * of the original column they came from, we embed the TDS_NUMERIC datatype in the row buffer
         * instead of using the wire representation, even though it uses a few more bytes.
         */
        TDS_NUMERIC *num = (TDS_NUMERIC *) dest;
        memset(num, '\0', sizeof(TDS_NUMERIC));
        /* TODO perhaps it would be fine to change format ?? */
        num->precision = curcol->column_prec;
        num->scale = curcol->column_scale;

        /* server is going to crash freetds ?? */
        /* TODO close connection it server try to do so ?? */
        if (colsize > sizeof(num->array))
            return TDS_FAIL;
        tds_get_n(tds, num->array, colsize);

        /* corrected colsize for column_cur_size */
        colsize = sizeof(TDS_NUMERIC);
        if (IS_TDS7_PLUS(tds)) {
            tdsdump_log(TDS_DBG_INFO1, "swapping numeric data...\n");
            tds_swap_numeric(num);
        }
        curcol->column_cur_size = colsize;
    } else if (is_blob_type(curcol->column_type)) {
        TDS_CHAR *p;
        int new_blob_size;
        assert(blob == (TDSBLOB *) dest);   /* cf. column_varint_size case 4, above */

        /*
         * Blobs don't use a column's fixed buffer because the official maximum size is 2 GB.
         * Instead, they're reallocated as necessary, based on the data's size.
         * Here we allocate memory, if need be.
         */
        /* TODO this can lead to a big waste of memory */
        new_blob_size = determine_adjusted_size(curcol->char_conv, colsize);
        if (new_blob_size == 0) {
            curcol->column_cur_size = 0;
            if (blob->textvalue)
                TDS_ZERO_FREE(blob->textvalue);
            return TDS_SUCCEED;
        }

        /* NOTE we use an extra pointer (p) to avoid lose of memory in the case realloc fails */
        p = blob->textvalue;
        if (!p) {
            p = (TDS_CHAR *) malloc(new_blob_size);
        } else {
            /* TODO perhaps we should store allocated bytes too ? */
            if (new_blob_size > curcol->column_cur_size ||  (curcol->column_cur_size - new_blob_size) > 10240) {
                p = (TDS_CHAR *) realloc(p, new_blob_size);
            }
        }

        if (!p)
            return TDS_FAIL;
        blob->textvalue = p;
        curcol->column_cur_size = new_blob_size;

        /* read the data */
        if (is_char_type(curcol->column_type)) {
            if (tds_get_char_data(tds, (char *) blob, colsize, curcol) == TDS_FAIL)
                return TDS_FAIL;
        } else {
            assert(colsize == new_blob_size);
            tds_get_n(tds, blob->textvalue, colsize);
        }
    } else {        /* non-numeric and non-blob */
        curcol->column_cur_size = colsize;

        if (curcol->char_conv) {
            if (tds_get_char_data(tds, (char *) dest, colsize, curcol) == TDS_FAIL)
                return TDS_FAIL;
        } else {
            /*
             * special case, some servers seem to return more data in some conditions
             * (ASA 7 returning 4 byte nullable integer)
             */
            int discard_len = 0;
            if (colsize > curcol->column_size) {
                discard_len = colsize - curcol->column_size;
                colsize = curcol->column_size;
            }
            if (tds_get_n(tds, dest, colsize) == NULL)
                return TDS_FAIL;
            if (discard_len > 0)
                tds_get_n(tds, NULL, discard_len);
            curcol->column_cur_size = colsize;
        }

        /* pad (UNI)CHAR and BINARY types */
        fillchar = 0;
        switch (curcol->column_type) {
        /* extra handling for SYBLONGBINARY */
        case SYBLONGBINARY:
            if (curcol->column_usertype != USER_UNICHAR_TYPE)
                break;
        case SYBCHAR:
        case XSYBCHAR:
            if (curcol->column_size != curcol->on_server.column_size)
                break;
            /* FIXME use client charset */
            fillchar = ' ';
        case SYBBINARY:
        case XSYBBINARY:
            if (colsize < curcol->column_size)
                memset(dest + colsize, fillchar, curcol->column_size - colsize);
            colsize = curcol->column_size;
            break;
        }

        if (curcol->column_type == SYBDATETIME4) {
            tdsdump_log(TDS_DBG_INFO1, "datetime4 %d %d %d %d\n", dest[0], dest[1], dest[2], dest[3]);
        }
    }

#ifdef WORDS_BIGENDIAN
    /*
     * MS SQL Server 7.0 has broken date types from big endian
     * machines, this swaps the low and high halves of the
     * affected datatypes
     *
     * Thought - this might be because we don't have the
     * right flags set on login.  -mjs
     *
     * Nope its an actual MS SQL bug -bsb
     */
    /* TODO test on login, remove configuration -- freddy77 */
    if (tds->broken_dates &&
        (curcol->column_type == SYBDATETIME ||
         curcol->column_type == SYBDATETIME4 ||
         curcol->column_type == SYBDATETIMN ||
         curcol->column_type == SYBMONEY ||
         curcol->column_type == SYBMONEY4 || (curcol->column_type == SYBMONEYN && curcol->column_size > 4)))
        /*
         * above line changed -- don't want this for 4 byte SYBMONEYN
         * values (mlilback, 11/7/01)
         */
    {
        unsigned char temp_buf[8];

        memcpy(temp_buf, dest, colsize / 2);
        memcpy(dest, &dest[colsize / 2], colsize / 2);
        memcpy(&dest[colsize / 2], temp_buf, colsize / 2);
    }
    if (tds->emul_little_endian) {
        tdsdump_log(TDS_DBG_INFO1, "swapping coltype %d\n", tds_get_conversion_type(curcol->column_type, colsize));
        tds_swap_datatype(tds_get_conversion_type(curcol->column_type, colsize), dest);
    }
#endif
    return TDS_SUCCEED;
}

/**
 * tds_process_row() processes rows and places them in the row buffer.
 */
static int
tds_process_row(TDSSOCKET * tds)
{
    int i;
    TDSCOLUMN *curcol;
    TDSRESULTINFO *info;

    CHECK_TDS_EXTRA(tds);

    info = tds->current_results;
    if (!info)
        return TDS_FAIL;

    assert(info->num_cols > 0);

    info->row_count++;
    for (i = 0; i < info->num_cols; i++) {
        tdsdump_log(TDS_DBG_INFO1, "tds_process_row(): reading column %d \n", i);
        curcol = info->columns[i];
        if (tds_get_data(tds, curcol, info->current_row, i) != TDS_SUCCEED)
            return TDS_FAIL;
    }
    return TDS_SUCCEED;
}

/**
 * tds_process_end() processes any of the DONE, DONEPROC, or DONEINPROC
 * tokens.
 * \param tds        state information for the socket and the TDS protocol
 * \param marker     TDS token number
 * \param flags_parm filled with bit flags (see TDS_DONE_ constants).
 *        Is NULL nothing is returned
 */
static int
tds_process_end(TDSSOCKET * tds, int marker, int *flags_parm)
{
    int more_results, was_cancelled, error, done_count_valid;
    int tmp, state;

    CHECK_TDS_EXTRA(tds);

    tmp = tds_get_smallint(tds);

    state = tds_get_smallint(tds);

    more_results = (tmp & TDS_DONE_MORE_RESULTS) != 0;
    was_cancelled = (tmp & TDS_DONE_CANCELLED) != 0;
    error = (tmp & TDS_DONE_ERROR) != 0;
    done_count_valid = (tmp & TDS_DONE_COUNT) != 0;


    tdsdump_log(TDS_DBG_FUNC, "tds_process_end: more_results = %d\n"
            "\t\twas_cancelled = %d\n"
            "\t\terror = %d\n"
            "\t\tdone_count_valid = %d\n", more_results, was_cancelled, error, done_count_valid);

    if (tds->res_info) {
        tds->res_info->more_results = more_results;
        if (tds->current_results == NULL)
            tds->current_results = tds->res_info;

    }

    if (flags_parm)
        *flags_parm = tmp;

    if (was_cancelled || (!more_results && !tds->in_cancel)) {
        tdsdump_log(TDS_DBG_FUNC, "tds_process_end() state set to TDS_IDLE\n");
        /* reset of in_cancel should must done before setting IDLE */
        tds->in_cancel = 0;
        tds_set_state(tds, TDS_IDLE);
        if (!tds->query_timeout  &&  tds->save_query_timeout)
            tds->query_timeout = tds->save_query_timeout;
    }

    if (IS_TDSDEAD(tds))
        return TDS_FAIL;

    /*
     * rows affected is in the tds struct because a query may affect rows but
     * have no result set.
     */

    if (done_count_valid) {
        tds->rows_affected = tds_get_int(tds);
        tdsdump_log(TDS_DBG_FUNC, "                rows_affected = %d\n", tds->rows_affected);
    } else {
        tmp = tds_get_int(tds); /* throw it away */
        tds->rows_affected = TDS_NO_COUNT;
    }

    if (IS_TDSDEAD(tds))
        return TDS_FAIL;

    return TDS_SUCCEED;
}



/**
 * tds_client_msg() sends a message to the client application from the CLI or
 * TDS layer. A client message is one that is generated from with the library
 * and not from the server.  The message is sent to the CLI (the
 * err_handler) so that it may forward it to the client application or
 * discard it if no msg handler has been by the application. tds->parent
 * contains a void pointer to the parent of the tds socket. This can be cast
 * back into DBPROCESS or CS_CONNECTION by the CLI and used to determine the
 * proper recipient function for this message.
 * \todo This procedure is deprecated, because the client libraries use differing messages and message numbers.
 *  The general approach is to emit ct-lib error information and let db-lib and ODBC map that to their number and text.
 */
int
tds_client_msg(const TDSCONTEXT * tds_ctx, TDSSOCKET * tds, int msgno, int severity, int state, int line, const char *msg_text)
{
    int ret;
    TDSMESSAGE msg;

    CHECK_CONTEXT_EXTRA(tds_ctx);
    if (tds)
        CHECK_TDS_EXTRA(tds);

    if (tds_ctx->err_handler) {
        memset(&msg, 0, sizeof(TDSMESSAGE));
        msg.msgno = msgno;
        msg.severity = severity;
        msg.state = state;
        /* TODO is possible to avoid copy of strings ? */
        msg.server = strdup("OpenClient");
        msg.line_number = line;
        msg.message = strdup(msg_text);
        if (msg.sql_state == NULL)
            msg.sql_state = tds_alloc_client_sqlstate(msg.msgno);
        ret = tds_ctx->err_handler(tds_ctx, tds, &msg);
        tds_free_msg(&msg);
#if 1
        /*
         * error handler may return:
         * INT_EXIT -- Print an error message, and exit application, . returning an error to the OS.
         * INT_CANCEL -- Return FAIL to the db-lib function that caused the error.
         * For SQLETIME errors only, call dbcancel() to try to cancel the current command batch
         *  and flush any pending results. Break the connection if dbcancel() times out,
         * INT_CONTINUE -- For SQLETIME, wait for one additional time-out period, then call the error handler again.
         *      Else treat as INT_CANCEL.
         */
#else
        /*
         * This was bogus afaict.
         * Definitely, it's a mistake to set the state to TDS_DEAD for information messages when the handler
         * returns INT_CANCEL, at least according to Microsoft's documentation.
         * --jkl
         */
        /*
         * message handler returned FAIL/CS_FAIL
         * mark socket as dead
         */
        if (ret && tds) {
            /* TODO close socket too ?? */
            tds_set_state(tds, TDS_DEAD);
        }
#endif
    }

    tdsdump_log(TDS_DBG_FUNC, "tds_client_msg: #%d: \"%s\".  Connection state is now %d.  \n", msgno, msg_text, tds ? (int)tds->state : -1);

    return 0;
}

/**
 * tds_process_env_chg()
 * when ever certain things change on the server, such as database, character
 * set, language, or block size.  A environment change message is generated
 * There is no action taken currently, but certain functions at the CLI level
 * that return the name of the current database will need to use this.
 */
static int
tds_process_env_chg(TDSSOCKET * tds)
{
    int size, type;
    char *oldval = NULL;
    char *newval = NULL;
    char **dest;
    int new_block_size;
    int lcid;
    int memrc = 0;

    CHECK_TDS_EXTRA(tds);

    size = tds_get_smallint(tds);
    /*
     * this came in a patch, apparently someone saw an env message
     * that was different from what we are handling? -- brian
     * changed back because it won't handle multibyte chars -- 7.0
     */
    /* tds_get_n(tds,NULL,size); */

    type = tds_get_byte(tds);

    /*
     * handle collate default change (if you change db or during login)
     * this environment is not a string so need different handles
     */
    if (type == TDS_ENV_SQLCOLLATION) {
        /* save new collation */
        size = tds_get_byte(tds);
        memset(tds->collation, 0, 5);
        if (size < 5) {
            tds_get_n(tds, tds->collation, size);
        } else {
            tds_get_n(tds, tds->collation, 5);
            tds_get_n(tds, NULL, size - 5);
            lcid = (tds->collation[0] + ((int) tds->collation[1] << 8) + ((int) tds->collation[2] << 16)) & 0xffffflu;
            tds7_srv_charset_changed(tds, tds->collation[4], lcid);
        }
        /* discard old one */
        tds_get_n(tds, NULL, tds_get_byte(tds));
        return TDS_SUCCEED;
    }

    /* fetch the new value */
    memrc += tds_alloc_get_string(tds, &newval, tds_get_byte(tds));

    /* fetch the old value */
    memrc += tds_alloc_get_string(tds, &oldval, tds_get_byte(tds));

    if (memrc != 0) {
        if (newval != NULL)
            free(newval);
        if (oldval != NULL)
            free(oldval);
        return TDS_FAIL;
    }

    dest = NULL;
    switch (type) {
    case TDS_ENV_PACKSIZE:
        new_block_size = atoi(newval);
        if (new_block_size > tds->env.block_size) {
            tdsdump_log(TDS_DBG_INFO1, "increasing block size from %s to %d\n", oldval, new_block_size);
            /*
             * I'm not aware of any way to shrink the
             * block size but if it is possible, we don't
             * handle it.
             */
            /* Reallocate buffer if impossible (strange values from server or out of memory) use older buffer */
            tds_realloc_socket(tds, new_block_size);
        }
        break;
    case TDS_ENV_DATABASE:
        dest = &tds->env.database;
        break;
    case TDS_ENV_LANG:
        dest = &tds->env.language;
        break;
    case TDS_ENV_CHARSET:
        dest = &tds->env.charset;
        tds_srv_charset_changed(tds, newval);
        break;
    }
    if (tds->env_chg_func) {
        (*(tds->env_chg_func)) (tds, type, oldval, newval);
    }

    if (oldval)
        free(oldval);
    if (newval) {
        if (dest) {
            if (*dest)
                free(*dest);
            *dest = newval;
        } else
            free(newval);
    }

    return TDS_SUCCEED;
}

/**
 * tds_process_msg() is called for MSG, ERR, or EED tokens and is responsible
 * for calling the CLI's message handling routine
 * returns TDS_SUCCEED if informational, TDS_ERROR if error.
 */
static int
tds_process_msg(TDSSOCKET * tds, int marker)
{
    int rc;
    int len;
    int len_sqlstate;
    int has_eed = 0;
    TDSMESSAGE msg;

    CHECK_TDS_EXTRA(tds);

    /* make sure message has been freed */
    memset(&msg, 0, sizeof(TDSMESSAGE));

    /* packet length */
    len = tds_get_smallint(tds);

    /* message number */
    rc = tds_get_int(tds);
    msg.msgno = rc;

    /* msg state */
    msg.state = tds_get_byte(tds);

    /* msg level */
    msg.severity = tds_get_byte(tds);

    /* determine if msg or error */
    switch (marker) {
    case TDS_EED_TOKEN:
        if (msg.severity <= 10)
            msg.priv_msg_type = 0;
        else
            msg.priv_msg_type = 1;

        /* read SQL state */
        len_sqlstate = tds_get_byte(tds);
        msg.sql_state = (char *) malloc(len_sqlstate + 1);
        if (!msg.sql_state) {
            tds_free_msg(&msg);
            return TDS_FAIL;
        }

        tds_get_n(tds, msg.sql_state, len_sqlstate);
        msg.sql_state[len_sqlstate] = '\0';

        /* do a better mapping using native errors */
        if (strcmp(msg.sql_state, "ZZZZZ") == 0)
            TDS_ZERO_FREE(msg.sql_state);

        /* if has_eed = 1, extended error data follows */
        has_eed = tds_get_byte(tds);

        /* junk status and transaction state */
        tds_get_smallint(tds);
        break;
    case TDS_INFO_TOKEN:
        msg.priv_msg_type = 0;
        break;
    case TDS_ERROR_TOKEN:
        msg.priv_msg_type = 1;
        break;
    default:
        tdsdump_log(TDS_DBG_ERROR, "tds_process_msg() called with unknown marker '%d'!\n", (int) marker);
        tds_free_msg(&msg);
        return TDS_FAIL;
    }

    tdsdump_log(TDS_DBG_ERROR, "tds_process_msg() reading message from server\n");

    rc = 0;
    /* the message */
    rc += tds_alloc_get_string(tds, &msg.message, tds_get_smallint(tds));

    /* server name */
    rc += tds_alloc_get_string(tds, &msg.server, tds_get_byte(tds));

    /* stored proc name if available */
    rc += tds_alloc_get_string(tds, &msg.proc_name, tds_get_byte(tds));

    /* line number in the sql statement where the problem occured */
    msg.line_number = tds_get_smallint(tds);

    /*
     * If the server doesen't provide an sqlstate, map one via server native errors
     * I'm assuming there is not a protocol I'm missing to fetch these from the server?
     * I know sybase has an sqlstate column in it's sysmessages table, mssql doesn't and
     * TDS_EED_TOKEN is not being called for me.
     */
    if (msg.sql_state == NULL)
        msg.sql_state = tds_alloc_lookup_sqlstate(tds, msg.msgno);


    /* In case extended error data is sent, we just try to discard it */
    if (has_eed == 1) {
        int next_marker;
        for (;;) {
            switch (next_marker = tds_get_byte(tds)) {
            case TDS5_PARAMFMT_TOKEN:
            case TDS5_PARAMFMT2_TOKEN:
            case TDS5_PARAMS_TOKEN:
                if (tds_process_default_tokens(tds, next_marker) != TDS_SUCCEED)
                    ++rc;
                continue;
            }
            break;
        }
        tds_unget_byte(tds);
    }

    /*
     * call the msg_handler that was set by an upper layer
     * (dblib, ctlib or some other one).  Call it with the pointer to
     * the "parent" structure.
     */

    if (rc != 0) {
        tds_free_msg(&msg);
        return TDS_ERROR;
    }

    /* special case, */
    if (marker == TDS_EED_TOKEN && tds->cur_dyn && !TDS_IS_MSSQL(tds) && msg.msgno == 2782) {
        /* we must emulate prepare */
        tds->cur_dyn->emulated = 1;
    /* 0.95
    } else if (marker == TDS_INFO_TOKEN && msg.msgno == 16954 && TDS_IS_MSSQL(tds)
           && tds->internal_sp_called == TDS_SP_CURSOROPEN && tds->cur_cursor) {
    */
        /* here mssql say "Executing SQL directly; no cursor." opening cursor */
    } else {
        /* EED can be followed to PARAMFMT/PARAMS, do not store it in dynamic */
        tds->cur_dyn = NULL;

        if (tds->tds_ctx->msg_handler) {
            tdsdump_log(TDS_DBG_ERROR, "tds_process_msg() calling client msg handler\n");
            tds->tds_ctx->msg_handler(tds->tds_ctx, tds, &msg);
        } else if (msg.msgno) {
            tdsdump_log(TDS_DBG_WARN,
                    "Msg %d, Severity %d, State %d, Server %s, Line %d\n%s\n",
                    msg.msgno,
                    msg.severity ,
                    msg.state, msg.server, msg.line_number, msg.message);
        }
    }

    tds_free_msg(&msg);

    tdsdump_log(TDS_DBG_ERROR, "tds_process_msg() returning TDS_SUCCEED\n");

    return TDS_SUCCEED;
}

/**
 * Read a string from wire in a new allocated buffer
 * \param tds state information for the socket and the TDS protocol
 * \param len length of string to read
 */
static int
tds_alloc_get_string(TDSSOCKET * tds, char **string, int len)
{
    char *s;
    int out_len;

    CHECK_TDS_EXTRA(tds);

    if (len < 0) {
        *string = NULL;
        return 0;
    }

    /* assure sufficient space for every conversion */
    s = (char *) malloc(len * 4 + 1);
    out_len = tds_get_string(tds, len, s, len * 4);
    if (!s) {
        *string = NULL;
        return -1;
    }
    s = realloc(s, out_len + 1);
    s[out_len] = '\0';
    *string = s;
    return 0;
}

/**
 * tds_process_cancel() processes the incoming token stream until it finds
 * an end token (DONE, DONEPROC, DONEINPROC) with the cancel flag set.
 * a that point the connetion should be ready to handle a new query.
 */
int
tds_process_cancel(TDSSOCKET * tds)
{
    CHECK_TDS_EXTRA(tds);

    /* silly cases, nothing to do */
    if (!tds->in_cancel)
        return TDS_SUCCEED;
    /* TODO handle cancellation sending data */
    if (tds->state != TDS_PENDING)
        return TDS_SUCCEED;

    tds->query_start_time = 0;
    tds->query_timeout = 0;

    /* TODO support TDS5 cancel, wait for cancel packet first, then wait for done */
    for (;;) {
        TDS_INT result_type;

        switch (tds_process_tokens(tds, &result_type, NULL, 0)) {
        case TDS_FAIL:
            return TDS_FAIL;
        case TDS_CANCELLED:
        case TDS_SUCCEED:
        case TDS_NO_MORE_RESULTS:
            return TDS_SUCCEED;
        }
    }
}

/**
 * Find a dynamic given string id
 * \return dynamic or NULL is not found
 * \param tds  state information for the socket and the TDS protocol
 * \param id   dynamic id to search
 */
TDSDYNAMIC *
tds_lookup_dynamic(TDSSOCKET * tds, char *id)
{
    TDSDYNAMIC *curr;

    CHECK_TDS_EXTRA(tds);

    for (curr = tds->dyns; curr != NULL; curr = curr->next) {
        if (!strcmp(curr->id, id))
            return curr;
    }
    return NULL;
}

/**
 * tds_process_dynamic()
 * finds the element of the dyns array for the id
 */
static TDSDYNAMIC *
tds_process_dynamic(TDSSOCKET * tds)
{
    int token_sz;
    unsigned char type, status;
    int id_len;
    char id[TDS_MAX_DYNID_LEN + 1];
    int drain = 0;

    CHECK_TDS_EXTRA(tds);

    token_sz = tds_get_smallint(tds);
    type = tds_get_byte(tds);
    status = tds_get_byte(tds);
    /* handle only acknowledge */
    if (type != 0x20) {
        tdsdump_log(TDS_DBG_ERROR, "Unrecognized TDS5_DYN type %x\n", type);
        tds_get_n(tds, NULL, token_sz - 2);
        return NULL;
    }
    id_len = tds_get_byte(tds);
    if (id_len > TDS_MAX_DYNID_LEN) {
        drain = id_len - TDS_MAX_DYNID_LEN;
        id_len = TDS_MAX_DYNID_LEN;
    }
    id_len = tds_get_string(tds, id_len, id, TDS_MAX_DYNID_LEN);
    id[id_len] = '\0';
    if (drain) {
        tds_get_string(tds, drain, NULL, drain);
    }
    return tds_lookup_dynamic(tds, id);
}

static int
tds_process_dyn_result(TDSSOCKET * tds)
{
    int hdrsize;
    int col, num_cols;
    TDSCOLUMN *curcol;
    TDSPARAMINFO *info;
    TDSDYNAMIC *dyn;

    CHECK_TDS_EXTRA(tds);

    hdrsize = tds_get_smallint(tds);
    num_cols = tds_get_smallint(tds);

    if (tds->cur_dyn) {
        dyn = tds->cur_dyn;
        tds_free_param_results(dyn->res_info);
        /* read number of columns and allocate the columns structure */
        if ((dyn->res_info = tds_alloc_results(num_cols)) == NULL)
            return TDS_FAIL;
        info = dyn->res_info;
    } else {
        tds_free_param_results(tds->param_info);
        if ((tds->param_info = tds_alloc_results(num_cols)) == NULL)
            return TDS_FAIL;
        info = tds->param_info;
    }
    tds->current_results = info;

    for (col = 0; col < info->num_cols; col++) {
        curcol = info->columns[col];

        tds_get_data_info(tds, curcol, 1);

        /* skip locale information */
        tds_get_n(tds, NULL, tds_get_byte(tds));

        tds_add_row_column_size(info, curcol);
    }

    if ((info->current_row = tds_alloc_row(info)) != NULL)
        return TDS_SUCCEED;
    else
        return TDS_FAIL;
}

/**
 *  New TDS 5.0 token for describing output parameters
 */
static int
tds5_process_dyn_result2(TDSSOCKET * tds)
{
    int hdrsize;
    int col, num_cols;
    TDSCOLUMN *curcol;
    TDSPARAMINFO *info;
    TDSDYNAMIC *dyn;

    CHECK_TDS_EXTRA(tds);

    hdrsize = tds_get_int(tds);
    num_cols = tds_get_smallint(tds);

    if (tds->cur_dyn) {
        dyn = tds->cur_dyn;
        tds_free_param_results(dyn->res_info);
        /* read number of columns and allocate the columns structure */
        if ((dyn->res_info = tds_alloc_results(num_cols)) == NULL)
            return TDS_FAIL;
        info = dyn->res_info;
    } else {
        tds_free_param_results(tds->param_info);
        if ((tds->param_info = tds_alloc_results(num_cols)) == NULL)
            return TDS_FAIL;
        info = tds->param_info;
    }
    tds->current_results = info;

    for (col = 0; col < info->num_cols; col++) {
        curcol = info->columns[col];

        /* TODO reuse tds_get_data_info code, sligthly different */

        /* column name */
        curcol->column_namelen =
            tds_get_string(tds, tds_get_byte(tds), curcol->column_name, sizeof(curcol->column_name) - 1);
        curcol->column_name[curcol->column_namelen] = '\0';

        /* column status */
        curcol->column_flags = tds_get_int(tds);
        curcol->column_nullable = (curcol->column_flags & 0x20) > 0;

        /* user type */
        curcol->column_usertype = tds_get_int(tds);

        /* column type */
        tds_set_column_type(tds, curcol, tds_get_byte(tds));

        /* FIXME this should be done by tds_set_column_type */
        curcol->column_varint_size = tds5_get_varint_size(curcol->column_type);
        /* column size */
        switch (curcol->column_varint_size) {
        case 4:
            if (curcol->column_type == SYBTEXT || curcol->column_type == SYBIMAGE) {
                curcol->column_size = tds_get_int(tds);
                /* read table name */
                curcol->table_namelen =
                    tds_get_string(tds, tds_get_smallint(tds), curcol->table_name,
                               sizeof(curcol->table_name) - 1);
            } else {
                curcol->column_size = tds_get_int(tds);
                /*tdsdump_log(TDS_DBG_INFO1, "UNHANDLED TYPE %x\n", curcol->column_type);*/
            }
            break;
        case 2:
            curcol->column_size = tds_get_smallint(tds);
            break;
        case 1:
            curcol->column_size = tds_get_byte(tds);
            break;
        case 0:
            break;
        }

        /* numeric and decimal have extra info */
        if (is_numeric_type(curcol->column_type)) {
            curcol->column_prec = tds_get_byte(tds);    /* precision */
            curcol->column_scale = tds_get_byte(tds);   /* scale */
            /* FIXME check prec/scale, don't let server crash us */
        }

        /* Adjust column size according to client's encoding */
        curcol->on_server.column_size = curcol->column_size;
        adjust_character_column_size(tds, curcol);

        /* discard Locale */
        tds_get_n(tds, NULL, tds_get_byte(tds));

        tds_add_row_column_size(info, curcol);

        tdsdump_log(TDS_DBG_INFO1, "elem %d:\n", col);
        tdsdump_log(TDS_DBG_INFO1, "\tcolumn_name=[%s]\n", curcol->column_name);
        tdsdump_log(TDS_DBG_INFO1, "\tflags=%x utype=%d type=%d varint=%d\n",
                curcol->column_flags, curcol->column_usertype, curcol->column_type, curcol->column_varint_size);
        tdsdump_log(TDS_DBG_INFO1, "\tcolsize=%d prec=%d scale=%d\n",
                curcol->column_size, curcol->column_prec, curcol->column_scale);
    }

    if ((info->current_row = tds_alloc_row(info)) != NULL)
        return TDS_SUCCEED;
    else
        return TDS_FAIL;
}

/**
 * tds_get_token_size() returns the size of a fixed length token
 * used by tds_process_cancel() to determine how to read past a token
 */
int
tds_get_token_size(int marker)
{
    /* TODO finish */
    switch (marker) {
    case TDS_DONE_TOKEN:
    case TDS_DONEPROC_TOKEN:
    case TDS_DONEINPROC_TOKEN:
        return 8;
    case TDS_RETURNSTATUS_TOKEN:
        return 4;
    case TDS_PROCID_TOKEN:
        return 8;
    default:
        return 0;
    }
}

void
tds_swap_datatype(int coltype, unsigned char *buf)
{
    switch (coltype) {
    case SYBINT2:
        tds_swap_bytes(buf, 2);
        break;
    case SYBINT4:
    case SYBMONEY4:
    case SYBREAL:
        tds_swap_bytes(buf, 4);
        break;
    case SYBINT8:
    case SYBFLT8:
        tds_swap_bytes(buf, 8);
        break;
    case SYBMONEY:
    case SYBDATETIME:
        tds_swap_bytes(buf, 4);
        tds_swap_bytes(&buf[4], 4);
        break;
    case SYBDATETIME4:
        tds_swap_bytes(buf, 2);
        tds_swap_bytes(&buf[2], 2);
        break;
    case SYBUNIQUE:
        tds_swap_bytes(buf, 4);
        tds_swap_bytes(&buf[4], 2);
        tds_swap_bytes(&buf[6], 2);
        break;
    }
}

void
tds_swap_numeric(TDS_NUMERIC *num)
{
    /* swap the sign */
    num->array[0] = (num->array[0] == 0) ? 1 : 0;
    /* swap the data */
    tds_swap_bytes(&(num->array[1]), tds_numeric_bytes_per_prec[num->precision] - 1);
}



/**
 * tds5_get_varint_size5() returns the size of a variable length integer
 * returned in a TDS 5.1 result string
 */
/* TODO can we use tds_get_varint_size ?? */
static int
tds5_get_varint_size(int datatype)
{
    switch (datatype) {
    case SYBLONGBINARY:
    case XSYBCHAR:
    case SYBTEXT:
    case SYBNTEXT:
    case SYBIMAGE:
    case SYBVARIANT:
        return 4;

    case SYBVOID:
    case SYBINT1:
    case SYBBIT:
    case SYBINT2:
    case SYBINT4:
    case SYBINT8:
    case SYBDATETIME4:
    case SYBREAL:
    case SYBMONEY:
    case SYBDATETIME:
    case SYBFLT8:
    case SYBMONEY4:
    case SYBSINT1:
    case SYBUINT2:
    case SYBUINT4:
    case SYBUINT8:
        return 0;

    case XSYBNVARCHAR:
    case XSYBVARCHAR:
    case XSYBBINARY:
    case XSYBVARBINARY:
        return 2;

    default:
        return 1;
    }
}

/**
 * tds_process_compute_names() processes compute result sets.
 */
static int
tds_process_compute_names(TDSSOCKET * tds)
{
    int hdrsize;
    int remainder;
    int num_cols = 0;
    int col;
    int memrc = 0;
    TDS_SMALLINT compute_id = 0;
    TDS_TINYINT namelen;
    TDSCOMPUTEINFO *info;
    TDSCOLUMN *curcol;

    struct namelist
    {
        char name[256];
        int namelen;
        struct namelist *nextptr;
    };

    struct namelist *topptr = NULL;
    struct namelist *curptr = NULL;
    struct namelist *freeptr = NULL;

    CHECK_TDS_EXTRA(tds);

    hdrsize = tds_get_smallint(tds);
    remainder = hdrsize;
    tdsdump_log(TDS_DBG_INFO1, "processing tds5 compute names. remainder = %d\n", remainder);

    /*
     * compute statement id which this relates
     * to. You can have more than one compute
     * statement in a SQL statement
     */

    compute_id = tds_get_smallint(tds);
    remainder -= 2;

    while (remainder) {
        namelen = tds_get_byte(tds);
        remainder--;
        if (topptr == NULL) {
            if ((topptr = (struct namelist *) malloc(sizeof(struct namelist))) == NULL) {
                memrc = -1;
                break;
            }
            curptr = topptr;
            curptr->nextptr = NULL;
        } else {
            if ((curptr->nextptr = (struct namelist *) malloc(sizeof(struct namelist))) == NULL) {
                memrc = -1;
                break;
            }
            curptr = curptr->nextptr;
            curptr->nextptr = NULL;
        }
        if (namelen == 0)
            strcpy(curptr->name, "");
        else {
            namelen = tds_get_string(tds, namelen, curptr->name, sizeof(curptr->name) - 1);
            curptr->name[namelen] = 0;
            remainder -= namelen;
        }
        curptr->namelen = namelen;
        num_cols++;
        tdsdump_log(TDS_DBG_INFO1, "processing tds5 compute names. remainder = %d\n", remainder);
    }

    tdsdump_log(TDS_DBG_INFO1, "processing tds5 compute names. num_cols = %d\n", num_cols);

    if ((tds->comp_info = tds_alloc_compute_results(tds, num_cols, 0)) == NULL)
        memrc = -1;

    tdsdump_log(TDS_DBG_INFO1, "processing tds5 compute names. num_comp_info = %d\n", tds->num_comp_info);

    info = tds->comp_info[tds->num_comp_info - 1];
    tds->current_results = info;

    info->computeid = compute_id;

    curptr = topptr;

    if (memrc == 0) {
        for (col = 0; col < num_cols; col++) {
            curcol = info->columns[col];

            assert(strlen(curcol->column_name) == curcol->column_namelen);
            memcpy(curcol->column_name, curptr->name, curptr->namelen + 1);
            curcol->column_namelen = curptr->namelen;

            freeptr = curptr;
            curptr = curptr->nextptr;
            free(freeptr);
        }
        return TDS_SUCCEED;
    } else {
        while (curptr != NULL) {
            freeptr = curptr;
            curptr = curptr->nextptr;
            free(freeptr);
        }
        return TDS_FAIL;
    }
}

/*
static int
tds_process_compute_names_095(TDSSOCKET * tds)
{
    int hdrsize;
    int remainder;
    int num_cols = 0;
    int col;
    int memrc = 0;
    TDS_SMALLINT compute_id = 0;
    TDS_TINYINT namelen;
    TDSCOMPUTEINFO *info;
    TDSCOLUMN *curcol;

    struct namelist
    {
        char name[256];
        int namelen;
        struct namelist *nextptr;
    };

    struct namelist *topptr = NULL;
    struct namelist *curptr = NULL;
    struct namelist *nextptr = NULL;

    CHECK_TDS_EXTRA(tds);

    hdrsize = tds_get_smallint(tds);
    remainder = hdrsize;
    tdsdump_log(TDS_DBG_INFO1, "processing tds5 compute names. remainder = %d\n", remainder);

    / *
     * compute statement id which this relates
     * to. You can have more than one compute
     * statement in a SQL statement
     * /

    compute_id = tds_get_smallint(tds);
    remainder -= 2;

    while (remainder > 0) {
        namelen = tds_get_byte(tds);
        remainder--;
        if ((nextptr = (struct namelist *) malloc(sizeof(struct namelist))) == NULL) {
            memrc = -1;
            break;
        }
        if (topptr == NULL)
            topptr = nextptr;
        else
            curptr->nextptr = nextptr;
        curptr = nextptr;
        curptr->nextptr = NULL;
        remainder -= namelen;
        namelen = tds_get_string(tds, namelen, curptr->name, sizeof(curptr->name) - 1);
        curptr->name[namelen] = 0;
        curptr->namelen = namelen;
        num_cols++;
        tdsdump_log(TDS_DBG_INFO1, "processing tds5 compute names. remainder = %d\n", remainder);
    }

    tdsdump_log(TDS_DBG_INFO1, "processing tds5 compute names. num_cols = %d\n", num_cols);

    if ((tds->comp_info = tds_alloc_compute_results(tds, num_cols, 0)) == NULL)
        memrc = -1;

    tdsdump_log(TDS_DBG_INFO1, "processing tds5 compute names. num_comp_info = %d\n", tds->num_comp_info);

    info = tds->comp_info[tds->num_comp_info - 1];
    tds->current_results = info;

    info->computeid = compute_id;

    curptr = topptr;

    if (memrc == 0) {
        for (col = 0; col < num_cols; col++) {
            curcol = info->columns[col];

            assert(strlen(curcol->column_name) == curcol->column_namelen);
            memcpy(curcol->column_name, curptr->name, curptr->namelen + 1);
            curcol->column_namelen = curptr->namelen;

            nextptr = curptr->nextptr;
            free(curptr);
            curptr = nextptr;
        }
        return TDS_SUCCEED;
    } else {
        while (curptr != NULL) {
            nextptr = curptr->nextptr;
            free(curptr);
            curptr = nextptr;
        }
        return TDS_FAIL;
    }
}
*/
/**
 * tds7_process_compute_result() processes compute result sets for TDS 7/8.
 * They is are very  similar to normal result sets.
 */
static int
tds7_process_compute_result(TDSSOCKET * tds)
{
    int col, num_cols;
    TDS_TINYINT by_cols;
    TDS_SMALLINT *cur_by_col;
    TDS_SMALLINT compute_id;
    TDSCOLUMN *curcol;
    TDSCOMPUTEINFO *info;

    CHECK_TDS_EXTRA(tds);

    /*
     * number of compute columns returned - so
     * COMPUTE SUM(x), AVG(x)... would return
     * num_cols = 2
     */

    num_cols = tds_get_smallint(tds);

    tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. num_cols = %d\n", num_cols);

    /*
     * compute statement id which this relates
     * to. You can have more than one compute
     * statement in a SQL statement
     */

    compute_id = tds_get_smallint(tds);

    tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. compute_id = %d\n", compute_id);
    /*
     * number of "by" columns in compute - so
     * COMPUTE SUM(x) BY a, b, c would return
     * by_cols = 3
     */

    by_cols = tds_get_byte(tds);
    tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. by_cols = %d\n", by_cols);

    if ((tds->comp_info = tds_alloc_compute_results(tds, num_cols, by_cols)) == NULL)
        return TDS_FAIL;

    tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. num_comp_info = %d\n", tds->num_comp_info);

    info = tds->comp_info[tds->num_comp_info - 1];
    tds->current_results = info;

    tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. point 0\n");

    info->computeid = compute_id;

    /*
     * the by columns are a list of the column
     * numbers in the select statement
     */

    cur_by_col = info->bycolumns;
    for (col = 0; col < by_cols; col++) {
        *cur_by_col = tds_get_smallint(tds);
        cur_by_col++;
    }
    tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. point 1\n");

    for (col = 0; col < num_cols; col++) {
        tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. point 2\n");
        curcol = info->columns[col];

        curcol->column_operator = tds_get_byte(tds);
        curcol->column_operand = tds_get_smallint(tds);

        tds7_get_data_info(tds, curcol);

        if (!curcol->column_namelen) {
            strcpy(curcol->column_name, tds_pr_op(curcol->column_operator));
            curcol->column_namelen = strlen(curcol->column_name);
        }

        tds_add_row_column_size(info, curcol);
    }

    /* all done now allocate a row for tds_process_row to use */
    tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. point 5 \n");
    if ((info->current_row = tds_alloc_compute_row(info)) != NULL)
        return TDS_SUCCEED;
    else
        return TDS_FAIL;
}

static int
tds_process_cursor_tokens(TDSSOCKET * tds)
{
    TDS_SMALLINT hdrsize;
    TDS_INT rowcount;
    TDS_INT cursor_id;
    TDS_TINYINT namelen;
    unsigned char cursor_cmd;
    TDS_SMALLINT cursor_status;
    TDSCURSOR *cursor;

    CHECK_TDS_EXTRA(tds);

    hdrsize  = tds_get_smallint(tds);
    cursor_id = tds_get_int(tds);
    hdrsize  -= sizeof(TDS_INT);
    if (cursor_id == 0){
        namelen = tds_get_byte(tds);
        hdrsize -= 1;
        /* discard name */
        tds_get_n(tds, NULL, namelen);
        hdrsize -= namelen;
    }
    cursor_cmd    = tds_get_byte(tds);
    cursor_status = tds_get_smallint(tds);
    hdrsize -= 3;

    if (hdrsize == sizeof(TDS_INT))
        rowcount = tds_get_int(tds);

    if (tds->cur_cursor) {
        cursor = tds->cur_cursor;
        cursor->cursor_id = cursor_id;
        cursor->srv_status = cursor_status;
        if ((cursor_status & TDS_CUR_ISTAT_DEALLOC) != 0)
            tds_free_cursor(tds, cursor);
            /* 0.95 tds_cursor_deallocated(tds, cursor); */
    }
    return TDS_SUCCEED;
}

static int
tds5_process_optioncmd(TDSSOCKET * tds)
{
    TDS_SMALLINT length;
    TDS_INT command;
    TDS_INT option;
    TDS_INT argsize;
    TDS_INT arg;

    CHECK_TDS_EXTRA(tds);

    tdsdump_log(TDS_DBG_INFO1, "tds5_process_optioncmd()\n");

    assert(IS_TDS50(tds));

    length = tds_get_smallint(tds);
    command = tds_get_byte(tds);
    option = tds_get_byte(tds);
    argsize = tds_get_byte(tds);

    switch (argsize) {
    case 0:
        arg = 0;
        break;
    case 1:
        arg = tds_get_byte(tds);
        break;
    case 4:
        arg = tds_get_int(tds);
        break;
    default:
        tdsdump_log(TDS_DBG_INFO1, "oops: cannot process option of size %d\n", argsize);
        assert(argsize <= 4);
        exit(1);     /* FIXME: stream would become desynchronized */
        break;
    }
    tdsdump_log(TDS_DBG_INFO1, "received option %d value %d\n", option, arg);

    if (command != TDS_OPT_INFO)
        return TDS_FAIL;

    tds->option_value = arg;

    return TDS_SUCCEED;
}

static const char *
tds_pr_op(int op)
{
#define TYPE(con, s) case con: return s; break
    switch (op) {
        TYPE(SYBAOPAVG, "avg");
        TYPE(SYBAOPAVGU, "avg");
        TYPE(SYBAOPCNT, "count");
        TYPE(SYBAOPCNTU, "count");
        TYPE(SYBAOPMAX, "max");
        TYPE(SYBAOPMIN, "min");
        TYPE(SYBAOPSUM, "sum");
        TYPE(SYBAOPSUMU, "sum");
        TYPE(SYBAOPCHECKSUM_AGG, "checksum_agg");
        TYPE(SYBAOPCNT_BIG, "count");
        TYPE(SYBAOPSTDEV, "stdevp");
        TYPE(SYBAOPSTDEVP, "stdevp");
        TYPE(SYBAOPVAR, "var");
        TYPE(SYBAOPVARP, "varp");
    default:
        break;
    }
    return "";
#undef TYPE
}

const char *
tds_prtype(int token)
{
#define TYPE(con, s) case con: return s; break
    switch (token) {
        TYPE(SYBAOPAVG, "avg");
        TYPE(SYBAOPCNT, "count");
        TYPE(SYBAOPMAX, "max");
        TYPE(SYBAOPMIN, "min");
        TYPE(SYBAOPSUM, "sum");

        TYPE(SYBBINARY, "binary");
        TYPE(SYBLONGBINARY, "longbinary");
        TYPE(SYBBIT, "bit");
        TYPE(SYBBITN, "bit-null");
        TYPE(SYBCHAR, "char");
        TYPE(SYBDATETIME4, "smalldatetime");
        TYPE(SYBDATETIME, "datetime");
        TYPE(SYBDATETIMN, "datetime-null");
        TYPE(SYBDECIMAL, "decimal");
        TYPE(SYBFLT8, "float");
        TYPE(SYBFLTN, "float-null");
        TYPE(SYBIMAGE, "image");
        TYPE(SYBINT1, "tinyint");
        TYPE(SYBINT2, "smallint");
        TYPE(SYBINT4, "int");
        TYPE(SYBINT8, "bigint");
        TYPE(SYBINTN, "integer-null");
        TYPE(SYBMONEY4, "smallmoney");
        TYPE(SYBMONEY, "money");
        TYPE(SYBMONEYN, "money-null");
        TYPE(SYBNTEXT, "UCS-2 text");
        TYPE(SYBNVARCHAR, "UCS-2 varchar");
        TYPE(SYBNUMERIC, "numeric");
        TYPE(SYBREAL, "real");
        TYPE(SYBTEXT, "text");
        TYPE(SYBUNIQUE, "uniqueidentifier");
        TYPE(SYBVARBINARY, "varbinary");
        TYPE(SYBVARCHAR, "varchar");
        TYPE(SYBVARIANT, "variant");
        TYPE(SYBVOID, "void");
        TYPE(XSYBBINARY, "xbinary");
        TYPE(XSYBCHAR, "xchar");
        TYPE(XSYBNCHAR, "x UCS-2 char");
        TYPE(XSYBNVARCHAR, "x UCS-2 varchar");
        TYPE(XSYBVARBINARY, "xvarbinary");
        TYPE(XSYBVARCHAR, "xvarchar");
    default:
        break;
    }
    return "";
#undef TYPE
}

/** \@} */

static const char *
_tds_token_name(unsigned char marker)
{
    switch (marker) {

    case 0x20:
        return "TDS5_PARAMFMT2";
    case 0x22:
        return "ORDERBY2";
    case 0x61:
        return "ROWFMT2";
    case 0x71:
        return "LOGOUT";
    case 0x79:
        return "RETURNSTATUS";
    case 0x7C:
        return "PROCID";
    case 0x81:
        return "TDS7_RESULT";
    case 0x88:
        return "TDS7_COMPUTE_RESULT";
    case 0xA0:
        return "COLNAME";
    case 0xA1:
        return "COLFMT";
    case 0xA3:
        return "DYNAMIC2";
    case 0xA4:
        return "TABNAME";
    case 0xA5:
        return "COLINFO";
    case 0xA7:
        return "COMPUTE_NAMES";
    case 0xA8:
        return "COMPUTE_RESULT";
    case 0xA9:
        return "ORDERBY";
    case 0xAA:
        return "ERROR";
    case 0xAB:
        return "INFO";
    case 0xAC:
        return "PARAM";
    case 0xAD:
        return "LOGINACK";
    case 0xAE:
        return "CONTROL";
    case 0xD1:
        return "ROW";
    case 0xD3:
        return "CMP_ROW";
    case 0xD7:
        return "TDS5_PARAMS";
    case 0xE2:
        return "CAPABILITY";
    case 0xE3:
        return "ENVCHANGE";
    case 0xE5:
        return "EED";
    case 0xE6:
        return "DBRPC";
    case 0xE7:
        return "TDS5_DYNAMIC";
    case 0xEC:
        return "TDS5_PARAMFMT";
    case 0xED:
        return "AUTH";
    case 0xEE:
        return "RESULT";
    case 0xFD:
        return "DONE";
    case 0xFE:
        return "DONEPROC";
    case 0xFF:
        return "DONEINPROC";

    default:
        break;
    }

    return "";
}

/**
 * Adjust column size according to client's encoding
 */
static void
adjust_character_column_size(const TDSSOCKET * tds, TDSCOLUMN * curcol)
{
    CHECK_TDS_EXTRA(tds);
    CHECK_COLUMN_EXTRA(curcol);

    if (is_unicode_type(curcol->on_server.column_type))
        curcol->char_conv = tds->char_convs[client2ucs2];

    /* Sybase UNI(VAR)CHAR fields are transmitted via SYBLONGBINARY and in UTF-16*/
    if (curcol->on_server.column_type == SYBLONGBINARY && (
        curcol->column_usertype == USER_UNICHAR_TYPE ||
        curcol->column_usertype == USER_UNIVARCHAR_TYPE)) {
        /* FIXME ucs2 is not UTF-16... */
        /* FIXME what happen if client is big endian ?? */
        curcol->char_conv = tds->char_convs[client2ucs2];
    }

    /* FIXME: and sybase ?? */
    if (!curcol->char_conv && IS_TDS7_PLUS(tds) && is_ascii_type(curcol->on_server.column_type))
        curcol->char_conv = tds->char_convs[client2server_chardata];

    if (!curcol->char_conv)
        return;

    curcol->on_server.column_size = curcol->column_size;
    curcol->column_size = determine_adjusted_size(curcol->char_conv, curcol->column_size);

    tdsdump_log(TDS_DBG_INFO1, "adjust_character_column_size:\n"
                   "\tServer charset: %s\n"
                   "\tServer column_size: %d\n"
                   "\tClient charset: %s\n"
                   "\tClient column_size: %d\n",
                   curcol->char_conv->server_charset.name,
                   curcol->on_server.column_size,
                   curcol->char_conv->client_charset.name,
                   curcol->column_size);
}

/**
 * Allow for maximum possible size of converted data,
 * while being careful about integer division truncation.
 * All character data pass through iconv.  It doesn't matter if the server side
 * is Unicode or not; even Latin1 text need conversion if,
 * for example, the client is UTF-8.
 */
static int
determine_adjusted_size(const TDSICONV * char_conv, int size)
{
    if (!char_conv)
        return size;

    /* ssikorsk */
    if (char_conv->client_charset.max_bytes_per_char > 1 &&
        INT_MAX / char_conv->client_charset.max_bytes_per_char < size
        ) {
        size = INT_MAX;
    } else {
        size *= char_conv->client_charset.max_bytes_per_char;
    }

    if (size % char_conv->server_charset.min_bytes_per_char
        &&  INT_MAX - char_conv->server_charset.min_bytes_per_char >= size)
    {
        size += char_conv->server_charset.min_bytes_per_char;
    }
    size /= char_conv->server_charset.min_bytes_per_char;

    return size;
}
