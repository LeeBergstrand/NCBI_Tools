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

 /* ==========
  * defintions
  */

%pure-parser
%parse-param { aesavs_parse_block * apb }
%lex-param   { aesavs_scan_block  * asb }
???? %parse-param { aesavs_scan_block  * asb }

%name-prefix="aesavs_"

%token 

%defines
%debug


%{

    /* pick up types and function prototypes */
    #include "aesavs-priv.h"

%}

%union
{
    /* the token from scanner */
    /* TBD - this has to be defined in aesavs-priv.h */
    KToken t;

    /* TBD - fix up intermediate result node type */
    int n;
}

 /* params to yylex */
%lex-param { AesAvsParser *self }

 /* params to yyparse */
%parse-param { AesAvsParser *self }

 /* special tokens for comment or unrecognized input */
%token COMMENT UNRECOGNIZED

 /* white space defined but never seen */
%token WS

 /* tokens with values */
%token <t> DECIMAL HEX

 /* keywords */
%token<t> COUNT KEY IV PLAINTEXT CIPHERTEXT ENCRYPT DECRYPT



%token FN_PATH FN_MODE FN_TEST FN_SIZE FN_EXT FN_UNRECOGNIZED



 /* production types */
%type<n> source 

%start source

%%
 /* ==========
  * rules
  */

filename
 : validname
 | invalidname;

validname
 : FN_PATH validname
 | validname REQEXT
 | FN_MODE FN_TEST FN_SIZE;

invalidname
 : FN_PATH invalidname
 | modw test keysize FN_UNRECOGNIZED
 | mode test FN_UNRECOGNIZED
 | mode FN_UNRECOGNIZED
 | FN_UNRECOGNIZED ;

mode : MODE_ECB | MODE_CBC | MODE_OFB | MODE_CFB1 | MODE_CFB8 | MODE_CFB128;

test : TEST_KAT | TEST_MMT | TEST_MCT;

keysize : KEY_128 | KEY_192 | KEY_256;

request_file
    : /* empty */
    | aes_tests ;


aes_tests
    : aes_test
    | aes_tests aes_test
    ;

aes_test
    : aes_counted_test
    | aes_uncounted_test
    | COMMENT
    | UNRECOGNIZED
    ;

aes_counted_test
    : count_line aes_uncounted_test
    ;

aes_uncounted_test
    : aes_ecb_encrypt_test
    | aes_ecb_decrypt_test
    | aes_cbc_encrypt_test
    | aes_cbc_decrypt_test
    ;

aes_ecb_encrypt_test
    : key plaintext
    ;

aes_ecb_decrypt_test
    : key ciphertext
    ;

aes_cbc_encrypt_test
    : key iv plaintext                          { /* normal C code, EXCEPT $1, $2, etc. refer to terms */ }
    ;

aes_cbc_decrypt_test
    : key iv ciphertext
    ;

key
    : KEY '=' BLOCK128                               { /* return value will be a binary value for key */ }
    : KEY '=' BLOCK192                               { /* return value will be a binary value for key */ }
    : KEY '=' BLOCK256                               { /* return value will be a binary value for key */ }
    ;

iv
    : IV '=' BLOCK128                                { /* return value will be a binary value for iv */ }
    ;

plaintext
    : PLAINTEXT '=' block_list
    ;

ciphertext
    : CIPHERTEXT '=' block_list
    ;

block_list
    : BLOCK128
    | hex_list BLOCK128
    ;


%%
