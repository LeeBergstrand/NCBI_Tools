#!/bin/bash

ACC="SRR000001"
ILLACC="DRR000001"
MIN=1
MAX=300
UNAME=$(uname)

if [ "$UNAME" = "Linux" ]
then
    OS="linux"
    ABI_PATH="/panfs/traces01/sra0/SRR/000000"
elif [ "$UNAME" = "Darwin" ]
then
    OS=mac
    ABI_PATH="/net/traces01/sra0/SRR/000000"
else
    OS=win
    ABI_PATH="//panfs/traces01/sra0/SRR/000000"
fi

dump()
{
    echo -e "----------\n running $1 on ${OS}"
    mkdir -p $1/${OS}
    rm -f $1/${OS}.log
    (
        cd $1/${OS}

        if [ "$1" = "abi-dump" ] ; then
            CMD="$1 -path $ABI_PATH -minSpotId ${MIN} -maxSpotId ${MAX} ${ACC}"
            echo -e ${CMD} "\n-----" &> ../${OS}.log
            ${CMD} >>../${OS}.log 2>&1
        elif [ "$1" = "sra-dump" ] ; then
            CMD="$1 --start ${MIN} --stop ${MAX} ${ACC}"
            echo -e ${CMD} "\n-----" &> ../${OS}.log
            ${CMD} > ${ACC}-sra.txt 2>> ../${OS}.log
        elif [ "$1" = "vdb-dump" ] ; then
            CMD="$1 ${ACC} -R${MIN}-${MAX}"
            echo -e ${CMD} "\n-----" &> ../${OS}.log
            ${CMD} > ${ACC}-vdb.txt 2>>../${OS}.log
        elif [ "$1" = "illumina-dump" ] ; then
            CMD="$1 -N ${MIN} -X ${MAX} ${ILLACC}"
            echo -e ${CMD} "\n-----" &> ../${OS}.log
            ${CMD} >> ../${OS}.log 2>&1
        else
            CMD="$1 -N ${MIN} -X ${MAX} ${ACC}"
            echo -e ${CMD} "\n-----" &> ../${OS}.log
            ${CMD} >> ../${OS}.log 2>&1
        fi
        echo $1: `ls -l | wc -l` "files(s) created"
    )

    # a cygwin/windows thing about permissions
    if [ "$OS" = "win" ]
        then
        chmod -f -x $1/$OS.log
    fi
}

dump abi-dump
dump fastq-dump
dump illumina-dump
dump sff-dump
dump sra-dump
dump vdb-dump
