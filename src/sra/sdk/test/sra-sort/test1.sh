#!/bin/bash

execute()
{
    echo $1
    eval $1
}


perform_test()
{
#    execute "rm $DST/pileup.1.txt $DST/pileup.2.txt"
#    execute "time sra-pileup $1 -o $DST/pileup.1.txt"
#    execute "rm -rf $DST/1"
#    execute "time $SRASORT $1 $DST/1"
#    execute "time sra-pileup $DST/1 -o $DST/pileup.2.txt"
    execute "diff $DST/pileup.1.txt $DST/pileup.2.txt > diff.txt"
}

SRASORT="/home/klymenka/OUTDIR_NOW/centos/gcc/stat/x86_64/dbg/bin/sra-sort"
DST="/panfs/pan1/sra-test/raetz"

SRC1="/panfs/pan1/trace-flatten/HG00121.chrom20.SOLID.bfast.GBR.low_coverage.20101123.bam"
SRC2="/panfs/pan1/sra-test/bam/HG00121.chrom20.SOLID.bfast.GBR.low_coverage.20101123.bam.csra"

execute "sra-pileup $SRC2 -r 20:61400-69500 > org.txt"
execute "sra-pileup $DST/1 -r 20:61400-69500 > new.txt"
execute "diff org.txt new.txt"
#execute "vdb-dump $SRC2 -T PRIMARY_ALIGNMENT -R431-433 -CALIGN_ID,READ,REF_POS,REF_LEN"
#execute "vdb-dump $DST/1 -T PRIMARY_ALIGNMENT -R431-433 -CALIGN_ID,READ,REF_POS,REF_LEN"