#!/bin/bash

USAGE0="Usage: `basename $0` [-hv] [-d directory]"
USAGE1="\tCompare dump outputs from windows, macintons and linux runs"
USAGE2="\tof the sister compare.sh script"
USAGE3="\t  -d: base directory for comparisons [default: pwd]"
USAGE4="\t  -h: displays this help message"
USAGE5="\t  -v: display full differences between files from the runs"

# default to a brief output on differences
VERBOSE="false"
#default to current directory
DIR="."
# all dump utilities we will test
DUMPERS="abi-dump fastq-dump illumina-dump sff-dump sra-dump vdb-dump"


comp-two()
{
    (
        if [ -e $1.log -a -e $2.log ] ; then
        
            echo "Comparing $1 and $2"
            if ! diff --brief -Nsw $1 $2 && [ "$VERBOSE" = "true" ] ; then
                diff -w $1 $2
            fi
        fi
    )
}


comp()
{
    echo -e "-------------\n$1"
    (
        cd $DIR/$1

        comp-two linux win
        comp-two linux mac
        comp-two mac win
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
            echo -e $USAGE5
            exit 0
            ;;

        d)
            DIR=$OPTARG
            ;;

        v)
            VERBOSE="true"
            ;;

    esac
done

# loop through defined dumpers to test
for D in $DUMPERS ; do
    comp $D
done

