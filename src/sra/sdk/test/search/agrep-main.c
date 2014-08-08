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

#include <search/extern.h>
#include <sysalloc.h>
#include "search-priv.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct agrepinfo {
    char *buf;
    int32_t linenum;
} agrepinfo;

rc_t agrep_callback( void *cbinfo, AgrepMatch *match, AgrepContinueFlag *cont )
{
    *cont = AGREP_CONTINUE;
    printf("%d,%d,%d: %.*s\n", ((agrepinfo *)cbinfo)->linenum, match->position, match->score, match->length, ((agrepinfo *)cbinfo)->buf + match->position);
}


rc_t agrep_2na_callback( void *cbinfo, AgrepMatch *match, AgrepContinueFlag *cont )
{
    char *p;
    static char *buf = NULL;
    static int32_t buflen = 0;
    int32_t i;

    *cont = AGREP_CONTINUE;
    if (buf == NULL || buflen < match->length) {
        free(buf);
        buf = malloc(match->length+1);
        buflen = match->length;
    }
    p = ((agrepinfo *)cbinfo)->buf + match->position;
    for (i=0; i<match->length; i++) {
        buf[i] = "ACGTN"[p[i]];
    }
    buf[match->length] = '\0';

    printf("%d,%d,%d: %s\n", ((agrepinfo *)cbinfo)->linenum, match->position, match->score, buf);
}

LIB_EXPORT int32_t CC agrep ( AgrepFlags alg, char *pattern, char *buf, int32_t print, 
        int32_t printcontext, int32_t k, int32_t findall, int32_t simulate )
{
    char *line;
    AgrepParams *self = NULL;
    int32_t count;
    int32_t len;
    AgrepFlags mode = alg;
    AgrepMatch match;
    agrepinfo info;

    AgrepCallArgs args;

    int32_t linenum = 0;
    char *na = "ACGTN";

    int32_t i;

    AgrepMake(&self, mode, pattern);

    count = 0;
    line = strtok(buf, "\n");
    while (line != NULL) {
        linenum++;
        len = strlen(line);

        if (simulate) {
            for (i=0; i<len; i++) {
                if (NULL != index(na, line[i])) {
                    line[i] = index(na, line[i]) - na;
                }
            }
        }
        
        if (findall) {
            info.buf = line;
            info.linenum = linenum;

            args.self = self;
            args.threshold = k;
            args.buf = line;
            args.buflen = len;
            args.cb = simulate? agrep_2na_callback: agrep_callback;
            args.cbinfo = (void *)&info;
            AgrepFindAll(&args);
        } else if (AgrepFindFirst(self, k, line, len, &match)) {
            count++;
            if (print) {
                printf("%s\n", line);
            }
            if (printcontext) {
                printf("%.*s\n", match.length, line+match.position);
            }
        }
        line = strtok(NULL, "\n");
    }    
    return count;
}

main( int ac, char **av )
{
    char *strings[10000];
    int32_t numstrings = 0;
    char *string;
    char *file;
    char *p, *np;
    int32_t length;
    FILE *fp;
    char *buf;
    int32_t buflen;
    char *endp;
    int32_t len;
    char *s;
    struct stat sbuf;
    int32_t count;
    char *progname = av[0];
    int32_t slow = 0;
    char *patternfile;
    char *pattern;
    int32_t print = 1;
    int32_t printcontext = 0;
    char *digits = "0123456789";
    int32_t k=0;
    AgrepFlags alg = AGREP_ALG_WUMANBER | AGREP_EXTEND_BETTER | AGREP_LEFT_MAINTAIN_SCORE;
    AgrepFlags mode = 0;
    int32_t findall = 0;
    int32_t simulate = 0;

    while (ac > 2 && av[1][0] == '-') {
        if (!strcmp("--4na", av[1])) {
            mode |= AGREP_PATTERN_4NA;
            ac--; av++;
            continue;
        }
        if (!strcmp("--extend", av[1])) {
            mode |= AGREP_EXTEND_SAME;
            ac--; av++;
            continue;
        }
        if (!strcmp("--better", av[1])) {
            mode |= AGREP_EXTEND_BETTER;
            ac--; av++;
            continue;
        }
        if (!strcmp("--maintain", av[1])) {
            mode |= AGREP_LEFT_MAINTAIN_SCORE;
            ac--; av++;
            continue;
        }

        if (!strcmp("--anchor", av[1])) {
            mode |= AGREP_ANCHOR_LEFT;
            ac--; av++;
            continue;
        }
        
        if (!strcmp("--2na", av[1])) {
            simulate = 1;
            mode |= AGREP_TEXT_EXPANDED_2NA | AGREP_PATTERN_4NA;
            ac--; av++;
            continue;
        }
        if (!strcmp("-a", av[1])) {
            findall = 1;
            printcontext = 1;
            ac--; av++;
            continue;
        }
        if (NULL != index(digits, av[1][1])) {
            k = atoi(++av[1]);
            ac--; av++;
            continue;
        }
        if (!strcmp(av[1], "--myers")) {
            alg = AGREP_ALG_MYERS;
            ac--; av++;
            continue;
        }
        if (!strcmp(av[1], "--dp")) {
            alg = AGREP_ALG_DP;
            ac--; av++;
            continue;
        }
        if (!strcmp(av[1], "--myersunlimited")) {
            alg = AGREP_ALG_MYERS_UNLTD;
            ac--; av++;
            continue;
        }
        if (!strcmp(av[1], "--myersunltd")) {
            alg = AGREP_ALG_MYERS_UNLTD;
            ac--; av++;
            continue;
        }
        if (!strcmp(av[1], "--wu")) {
            alg = AGREP_ALG_WUMANBER;
            ac--; av++;
            continue;
        }
        if (!strcmp(av[1], "--wumanber")) {
            alg = AGREP_ALG_WUMANBER;
            ac--; av++;
            continue;
        }

        if (!strcmp(av[1], "--slow")) {
            slow = 1;
            ac--; av++;
            continue;
        }
        if (!strcmp(av[1], "-c")) {
            print = 0;
            ac--; av++;
            continue;
        }
        if (!strcmp(av[1], "-x")) {
            printcontext = 1;
            print = 0;
            ac--; av++;
            continue;
        }
        fprintf(stderr, "Unknown switch: %s\n", av[1]);
        ac--; av++;
    }

    pattern = av[1];
    file = av[2];
  
    p = string;


    stat(file, &sbuf);
    buflen = sbuf.st_size;
    fp = fopen(file, "r");
    buf = malloc(buflen + 1);
    fread(buf, 1, buflen, fp);
    fclose(fp);

    count = agrep(alg|mode, pattern, buf, print, printcontext, k, findall, simulate);
    
    printf("Found %d matching lines.\n", count);
    }
    
