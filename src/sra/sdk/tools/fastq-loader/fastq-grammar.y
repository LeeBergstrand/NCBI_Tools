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
    #include <ctype.h>
    #include <stdlib.h>
    #include <string.h>

    #include "fastq-parse.h"

    #define YYSTYPE FASTQToken
    #define YYLEX_PARAM pb->scanner
    #define YYDEBUG 1

    #include "fastq-tokens.h"

    static void AddQuality(FASTQParseBlock* pb, const FASTQToken* token);
    static void SetReadNumber(FASTQParseBlock* pb, const FASTQToken* token);
    static void GrowTagLine(FASTQParseBlock* pb, const FASTQToken* token);
    static void StopSpotName(FASTQParseBlock* pb);
    static void SetSpotGroup(FASTQParseBlock* pb, const FASTQToken* token);
    static void SetRead(FASTQParseBlock* pb, const FASTQToken* token);

    #define UNLEX do { if (yychar != YYEMPTY && yychar != YYEOF) FASTQ_unlex(pb, & yylval); } while (0)
%}

%pure-parser
%parse-param {FASTQParseBlock* pb }
%lex-param {FASTQParseBlock* pb }

%name-prefix="FASTQ_"

%token fqNUMBER
%token fqALPHANUM
%token fqWS
%token fqENDLINE
%token fqBASESEQ
%token fqCOLORSEQ
%token fqTOKEN
%token fqASCQUAL
%token fqUNRECOGNIZED
%token fqENDOFTEXT 0

%%

sequence /* have to return the lookahead symbol before returning since it belongs to the next record and cannot be dropped */
    : readLines qualityLines    { UNLEX; return 1; }
    
    | readLines                 { UNLEX; return 1; }
    
    | qualityLines              { UNLEX; return 1; } 
    
    | fqALPHANUM                { GrowTagLine(pb, &$1); }
        ':'                     { GrowTagLine(pb, &$3); }
        fqNUMBER                { GrowTagLine(pb, &$5); }
        ':'                     { GrowTagLine(pb, &$7);}
        fqNUMBER                { GrowTagLine(pb, &$9); }
        ':'                     { GrowTagLine(pb, &$11); }
        fqNUMBER                { GrowTagLine(pb, &$13); }
        ':'                     { GrowTagLine(pb, &$15); }
        fqNUMBER                { GrowTagLine(pb, &$17); }
        ':'                     { FASTQScan_inline_sequence(pb); } 
        inlineRead              
        ':'                     { FASTQScan_inline_quality(pb); } 
        quality                 { UNLEX; return 1; }
           
    | fqALPHANUM error endline  { UNLEX; return 1; }
           
    | endfile                   { return 0; }
    ;

endfile
    : fqENDOFTEXT
    | endline endfile
    ;

endline
    : fqENDLINE
    ;

readLines
    : header  endline  read   
    | header  endline error endline
    | error   endline  read 
    ;

header 
    : '@' tagLine
    | '>' tagLine
    ;

read
    : baseRead  { pb->record->seq.is_colorspace = false; }
    | csRead    { pb->record->seq.is_colorspace = true; }
    ;

baseRead
    : fqBASESEQ { SetRead(pb, & $1); } 
        endline            
    | baseRead fqBASESEQ { SetRead(pb, & $2); } 
        endline  
    ;

csRead
    : fqCOLORSEQ { SetRead(pb, & $1); } 
        endline           
    | csRead fqCOLORSEQ { SetRead(pb, & $2); } 
        endline
    ;

inlineRead
    : fqBASESEQ                   { SetRead(pb, & $1); pb->record->seq.is_colorspace = false; }
    | fqCOLORSEQ                  { SetRead(pb, & $1); pb->record->seq.is_colorspace = true; }
    ;
    
 /*************** tag line rules *****************/
tagLine    
    : nameSpotGroup 
    | nameSpotGroup readNumberOrTail
    ;
    
nameSpotGroup
    : name { StopSpotName(pb); } 
        spotGroup 
    ;
    
name
    : fqALPHANUM        { GrowTagLine(pb, &$1); }
    | fqNUMBER          { GrowTagLine(pb, &$1); }
    | name '_'          { GrowTagLine(pb, &$2); }
    | name '-'          { GrowTagLine(pb, &$2); }
    | name '.'          { GrowTagLine(pb, &$2); }
    | name ':'          { GrowTagLine(pb, &$2); }
    | name fqALPHANUM   { GrowTagLine(pb, &$2); }
    | name fqNUMBER     { GrowTagLine(pb, &$2); }
    ;

spotGroup
    : '#'           { GrowTagLine(pb, &$1); }
        fqNUMBER    { SetSpotGroup(pb, &$3);  GrowTagLine(pb, &$3); }
    | '#'           { GrowTagLine(pb, &$1); }    
        fqALPHANUM  { SetSpotGroup(pb, &$3);  GrowTagLine(pb, &$3); }    
    |
    ;
    
readNumberOrTail
    : '/'       { GrowTagLine(pb, &$1); } 
      fqNUMBER  { SetReadNumber(pb, &$3); GrowTagLine(pb, &$3); } 
    | fqWS  { GrowTagLine(pb, &$1); }    
        casava1_8
    | fqWS  { GrowTagLine(pb, &$1); }  
        tail
    | readNumberOrTail fqWS { GrowTagLine(pb, &$2); } tail
    ;

casava1_8
    : fqNUMBER          { SetReadNumber(pb, &$1); GrowTagLine(pb, &$1); }
     ':'                { GrowTagLine(pb, &$3); }
     fqALPHANUM         { GrowTagLine(pb, &$5); if ($5.tokenLength == 1 && $5.tokenText[0] == 'Y') pb->record->seq.lowQuality = true; }
     ':'                { GrowTagLine(pb, &$7); }
     fqNUMBER           { GrowTagLine(pb, &$9); }
     ':'                { GrowTagLine(pb, &$11); } 
     indexSequence
     
indexSequence
    : fqALPHANUM        { SetSpotGroup(pb, &$1); GrowTagLine(pb, &$1); } 
    | fqNUMBER          { SetSpotGroup(pb, &$1); GrowTagLine(pb, &$1); } 
    ;
    
tail
    : fqALPHANUM        { GrowTagLine(pb, &$1); }
    | tail fqNUMBER          { GrowTagLine(pb, &$2); }
    | tail fqALPHANUM        { GrowTagLine(pb, &$2); }
    | tail '_'               { GrowTagLine(pb, &$2); }
    | tail '/'               { GrowTagLine(pb, &$2); }
    | tail '='               { GrowTagLine(pb, &$2); }
    ;
    
 /*************** quality rules *****************/

qualityLines
    : qualityHeader endline quality 
    | qualityHeader endline error endline
    ;

qualityHeader
    : '+'                 
    | qualityHeader fqTOKEN
    ;

quality
    : qualityLine endline           
    | quality qualityLine endline
    
qualityLine
    : fqASCQUAL                {  AddQuality(pb, & $1); }
    ;

%%

static
void GrowByteBuffer(KDataBuffer* self, const char* buf, uint64_t length)
{
    uint64_t oldSize = self->elem_count;
    KDataBufferResize( self, oldSize + length );
    memcpy( (char*)self->base + oldSize, buf, length );
}

void AddQuality(FASTQParseBlock* pb, const FASTQToken* token)
{
    if (pb->phredOffset != 0)
    {
        uint8_t floor   = pb->phredOffset == 33 ? MIN_PHRED_33 : MIN_PHRED_64;
        uint8_t ceiling = pb->phredOffset == 33 ? MAX_PHRED_33 : MAX_PHRED_64;
        unsigned int i;
        for (i=0; i < token->tokenLength; ++i)
        {
            char buf[200];
            if (token->tokenText[i] < floor || token->tokenText[i] > ceiling)
            {
                sprintf(buf, "Invalid quality value (%d): for %s, valid range is from %d to %d.", 
                                                         token->tokenText[i],
                                                         pb->phredOffset == 33 ? "Phred33": "Phred64", 
                                                         floor, 
                                                         ceiling);
                pb->fatalError = true;
                yyerror(pb, buf);
                return;
            }
        }
    }
    GrowByteBuffer( & pb->quality, token->tokenText, token->tokenLength);
}

void SetReadNumber(FASTQParseBlock* pb, const FASTQToken* token)
{
    if (token->tokenLength == 1)
    {
        switch (token->tokenText[0])
        {
        case '1': pb->record->seq.readnumber = 1; return;
        case '2': pb->record->seq.readnumber = 2; return;
        }
    }
    pb->record->seq.readnumber = pb->defaultReadNumber;
}

void GrowTagLine(FASTQParseBlock* pb, const FASTQToken* token)
{
    /* TODO: move tagline to FileReader, move this function to fastq-reader.c */

    /* grow the buffer as necessary*/
    uint64_t oldSize = pb->tagLine.elem_count;
    KDataBufferResize( & pb->tagLine, oldSize + token->tokenLength);
    string_copy((char*)pb->tagLine.base + oldSize, pb->tagLine.elem_count - oldSize, token->tokenText, token->tokenLength);
    
    if (!pb->spotNameDone)
        pb->spotNameLength = pb->tagLine.elem_count;
}

void StopSpotName(FASTQParseBlock* pb)
{   /* spot name is the current content of the tag line (there may be more tokens coming, they will not be a part of the spot name) */
    pb->spotNameDone = true;
}

void SetSpotGroup(FASTQParseBlock* pb, const FASTQToken* token)
{
    if (token->tokenLength != 1 || token->tokenText[0] != '0') /* ignore spot group 0 */
    {
        pb->spotGroupOffset = pb->tagLine.elem_count;    
        pb->spotGroupLength = token->tokenLength;
    }
}

void SetRead(FASTQParseBlock* pb, const FASTQToken* token)
{ 
    if (pb->record->seq.read)
    {
        pb->record->seq.read = (char*)realloc(pb->record->seq.read, strlen(pb->record->seq.read) + 1 + token->tokenLength + 1);
        strcat(pb->record->seq.read, token->tokenText); 
        ++pb->expectedQualityLines;
    }
    else
    {
        pb->record->seq.read = (char*)malloc(token->tokenLength+1);
        strcpy(pb->record->seq.read, token->tokenText); 
        pb->expectedQualityLines = 1;
    }
}
