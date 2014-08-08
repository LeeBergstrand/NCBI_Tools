#!/bin/bash

USAGE0="Usage: `basename $0` [-h] [-d directory]"
USAGE1="\tClear dump outputs from windows, macintons and linux runs"
USAGE2="\tof the sister compare.sh script"
USAGE3="\t  -d: base directory for comparisons [default: pwd]"
USAGE4="\t  -h: displays this help message"

#default to current directory
DIR="."
# all dump utilities we will test
DUMPERS="abi-dump fastq-dump illumina-dump sff-dump sra-dump vdb-dump"


cleanup()
{
    echo -e "-------------\n$1"
    (
        cd $DIR/$1
        rm -rf linux
        rm -rf win
        rm -rf mac
        rm -f *.log
    )
}


while getopts  d:hv OPT; do

    case "$OPT" in
        h)
            echo -e $USAGE0
            echo -e $USAGE1
            echo -e $USAGE2
            echo -e $USAGE3
            echo -e $USAGE4
            exit 0
            ;;

        d)
            DIR=$OPTARG
            ;;

    esac
done

# loop through defined dumpers to test
for D in $DUMPERS ; do
    cleanup $D
done

