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

%{

    /* the yacc include is not self-sufficient */
    #include "aesavs-priv.h"
    #include "aesavs-parse.h"
    #include <stdlib.h>

    #define YYSTYPE aesavs_parse_block

    /*  */





    /* code to populate token with internal values */
    #define YY_USER_ACTION                      \
        yylval -> t . lineno = yylineno;        \
        yylval -> t . val . c = yytext;

%}

%option reentrant

%option noyywrap

 /* we use multiple states */
%option stack

 /* generate a header file */
%option header-file="aesavs-lex.h"



%x AESAVS_FILENAME
%x AESAVS_REQ_FILE

 /* ==========
  * defintions
  */

DIGIT   [0-9]
LOWER   [a-z]
UPPER   [A-Z]
ALPHA   [A-Za-z]
ALNUM   [0-9A-Za-z]
XDIGIT  [0-9A-Fa-f]


%%

 /* ==========
  * rules
  */

<AESAVS_FILENAME> {
     .*"/"                      { return PATH;           }

     "ECB"                      { return MODE_ECB;       }
     "CBC"                      { return MODE_CBC;       }
     "OFB"                      { return MODE_OFB;       }
     "CFB1"                     { return MODE_CFB1;      }
     "CFB8"                     { return MODE_CFB8;      }
     "CFB128"                   { return MODE_CFB128;    }


     "GFSbox" | "KeyBox"        { return KAT;            }
     "VarKey" | "VarTxt"        { return KAT;            }
     "MMT"                      { return MMT; /* KAT? */ }
     "MCT"                      { return MCT;            }
     "128" | "192" | "256"      { return KEYSIZE;        }
     ".".*                      { return REQEXT;         }
     .*                         { return UNRECOGNIZED;   }
}


<AESAVS_REQ_FILE> {

    /* contextual tokens */
    "COUNT"                                 { return COUNT;                     }
    "KEY"                                   { return KEY;                       }
    "IV"                                    { return IV;                        }
    "PLAINTEXT"                             { return PLAINTEXT;                 }
    "CIPHERTEXT"                            { return CIPHERTEXT;                }
    "[ENCRYPT]"                             { return ENCRYPT;                   }
    "[DECRYPT]"                             { return DECRYPT;                   }

    [:xdigit:]{32}                          { return BLOCK128; }
    [:xdigit:]{48}                          { return KEY192; }
    [:xdigit:]{64}                          { return KEY256; }


    /* line comments */
    "#".+                                   { return COMMENT;                   }

    /* white space */
    [ \t\f\v]+                              { return WS;                        }
    \r\n                                    { ++ yylineno; return EOLN;         }
    \r                                      { ++ yylineno; return EOLN;         }
    \n                                      { ++ yylineno; return EOLN;         }

    /* numeric tokens */
    [0-9]+                                  { return DECIMAL;                   }


    /* punctuation */
    "="                                     { return '=';                       }

    /* unrecognized input */
    .+                                      { return UNRECOGNIZED;              }

}
%%

 /* ==========
  * subroutines
  */


rc_t aesavs_parse_init (aesavs_scan_block * sb)
{
    int  ret;
    rc_t rc;

    ret = yylex_init (sb->scanner);
    if (ret)
    {
        /* check errno? */
        if (1)
            rc = RC (rcExe, rcFile, rcParsing, rcMemory, rcExhausted);
        else
            rc = RC (rcExe, rcFile, rcParsing, rcParam, rcInvalid);
    }
    else
    {
        rc = 0;
    }
    return rc;
}    


rc_t parse_file_name (aesavs_scan_block * sb, char * file_name)
{
    rc_t rc;

    yy_push_state (AESAVS_FILENAME, sb->scanner);

    yy_scan_string (file_name);
 
    yy_parse (aesavs_parse_block, aesavs_scan_block);

    yylex_destroy ();

    yy_pop_state (sb->scanner);
}

rc_t parse_file (aesavs_scan_block * sb, char


rc_t CC aesavs_path_yylex_init (aesavs_scan_block * sb, const char * path)
{
    rc_t rc;

    if (yylex_init (&sb->scanner) != 0)
        rc = RC (rcExe, 0, rcParsing, rcMemory, rcExhausted);

    else
    {
        struct yyguts_t * yyg;

        yyg = sb->scanner;
        sb->buffer - yy_scan_string (path, yyg);
        yylineno = 1;
        yycolumn = 1;
        yy_push_state (AESAVS_FILENAMEMODE);
        rc = 0;
    }
    return rc;
}


rc_t line_out (


