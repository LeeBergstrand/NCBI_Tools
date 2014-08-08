#!/bin/bash

execute()
{
    echo $1
    eval $1
}

# comparing the output of the tip of development
# with the output of version 2.1.6
# on LINUX

TOOLPATH="/home/raetzw/output/centos/dbg/gcc/x86_64/bin/"
OUTDIR="/panfs/pan1/sra-test/raetz/dumps"
CMP="2.1.6"

execute "./dumpto.pl $OUTDIR $TOOLPATH"
execute "./comparewith.pl $OUTDIR $CMP $TOOLPATH"
