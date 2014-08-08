#!/bin/bash

execute()
{
    echo $1
    eval $1
}

THE_URL_1="http://ftp-private.ncbi.nlm.nih.gov/sra/sra-instant/reads/ByRun/sra/SRR/SRR000/SRR000123/SRR000123.sra"
THE_URL_2="http://ftp-private.ncbi.nlm.nih.gov/sra/sra-instant/reads/ByRun/sra/SRR/SRR000/SRR000129/SRR000129.sra"
THE_URL_3="ftp://ftp-private.ncbi.nlm.nih.gov/sra/sra-instant/reads/ByRun/sra/SRR/SRR000/SRR000123/SRR000123.sra"
THE_URL_4="ftp://ftp-private.ncbi.nlm.nih.gov/sra/sra-instant/reads/ByRun/sra/SRR/SRR000/SRR000129/SRR000129.sra"
THE_URL_5="http://ftp-trace/sra/sra-instant/reads/ByRun/sra/SRR/SRR000/SRR000009/SRR000009.sra"
THE_URL_6="http://ftp-private.ncbi.nlm.nih.gov/sra/sra-instant/reads/ByRun/sra/SRR/SRR000/SRR000009/SRR000009.sra"

DELIM="-------------------------------------------------------------------------------------------------------"

read_part()
{
    FROM="$1"
    COUNT="$2"
    execute "kget $THE_URL_1 dest1.txt --start $FROM --count $COUNT -c cache.bin -pb"
    execute "diff -s <( dd if=dest1.txt ibs=1 obs=1 skip=$FROM count=$COUNT ) <(dd if=dest.ftp ibs=1 obs=1 skip=$FROM count=$COUNT)"
}

echo "$DELIM"

#execute "totalview kget -a $THE_URL_1 -w -b"
#execute "kget $WRONG_URL -w -s 50000 -b"
#execute "kget $THE_URL_4 dest.txt -s 500000 -b"

#with -c that means caching...

#execute "kget $THE_URL_1 dest.txt -s 600000 --log log.txt -c cache.bin -p"
#execute "diff dest.txt dest.ftp"

#execute "totalview kget -a $THE_URL_1 dest.txt -s 500000"

#ORG_PATH="/export/home/sybase/clients-mssql/current/lib:/net/snowman/vol/projects/trace_software/tools/x86_64/lib:/opt/ghostscript/lib64:/opt/gd-2.0.35/lib64:/home/raetzw/output/lib64:/home/raetzw/output/ilib64"
#NEW_PATH="/opt/curl-7.27.0/lib:$ORG_PATH"

#export "LD_LIBRARY_PATH=$NEW_PATH"
#execute "kget $THE_URL_4 dest.txt -s 500000 -b"
#execute "kget $THE_URL_1 dest.txt -s 500000 -b"

#read_part 920500 500
execute "time kget $THE_URL_5"
echo "$DELIM"
execute "time kget $THE_URL_6"