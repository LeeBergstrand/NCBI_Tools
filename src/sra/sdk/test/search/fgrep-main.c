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
#include "search-priv.h"
#include <sysalloc.h>

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

LIB_EXPORT int32_t CC slow_fgrep( char *strings[], 
        int32_t numstrings, char *buf, int32_t buflen, FgrepFlags alg )
{
    int32_t count = 0;
    int32_t pos = 0;
    char *p, *endp, *np;
    int32_t len;
    int32_t ret;
    FgrepParams *search;
    FgrepFlags mode = FGREP_MODE_ASCII | alg;
    FgrepMatch match;

    FgrepMake(&search, mode, strings, numstrings);

    p = buf;
    endp = buf + buflen;
    while (p < endp) {
        np = strchr(p, '\n');
        if (np == NULL) {
            np = buf+buflen;
        }
        len = np - p;
        if (len == 0) {
            p++;
            continue;
        }
        if (FgrepFindFirst(search, p, len, &match)) {
            /* fprintf(stderr, "Found at line pos %d\n", match.position); */
            count++;
        }
        p = ++np;
    }
    return count;
}

main(int32_t ac, char **av)
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

    FgrepFlags alg = FGREP_ALG_AHOCORASICK;

    while (ac > 2) {
        if (!strcmp(av[1], "--dumb")) {
            alg = FGREP_ALG_DUMB;
            ac--; av++;
            continue;
        }
        if (!strcmp(av[1], "--aho")) {
            alg = FGREP_ALG_AHOCORASICK;
            ac--; av++;
            continue;
        }
        if (!strcmp(av[1], "--boyer")) {
            alg = FGREP_ALG_BOYERMOORE;
            ac--; av++;
            continue;
        }
        if (!strcmp(av[1], "-f")) {
            ac--; av++;
            if (ac > 2) {
                patternfile = av[1];
                fp = fopen(patternfile, "r");
                stat(patternfile, &sbuf);
                buflen = sbuf.st_size;
                buf = malloc(buflen + 1);
                fread(buf, 1, buflen, fp);
                fclose(fp);
                s = strtok(buf, " \n");
                strings[numstrings++] = s;
                while (NULL != (s = strtok(NULL, " \n"))) {
                    strings[numstrings++] = s;
                }
	
                ac--; av++;
                continue;
            } else {
                fprintf(stderr, "Missing arg: -f\n");
            }
        }
        s = strtok(av[1], " \n");
        strings[numstrings++] = s;
        while (NULL != (s = strtok(NULL, " \n"))) {
            strings[numstrings++] = s;
        }
        av++; ac--;
    }

    file = av[1];
  
    p = string;


    stat(file, &sbuf);
    buflen = sbuf.st_size;
    fp = fopen(file, "r");
    buf = malloc(buflen + 1);
    fread(buf, 1, buflen, fp);
    fclose(fp);

    count = slow_fgrep(strings, numstrings, buf, buflen, alg);

    printf("Found %d matching lines.\n", count);
}
