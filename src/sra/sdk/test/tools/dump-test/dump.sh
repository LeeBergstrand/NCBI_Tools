#!/bin/bash

# -----
# Define some 'global' variables

USAGE0="Usage: `basename $0` [-hv] [-d directory]"
USAGE1="    Run the various SRA project dumpers from an example accession run"
USAGE2="    the sister comp.sh can compare the otuput from runs of the three base platforms"
USAGE3="      -d: base directory for output [default: pwd]"
USAGE4="      -h: displays this help message"
USAGE5="      -v: display more output"
VERBOSE="false"
DIR="."
PROGDIR=""

# most dumpers can use this accession table
ACC="SRR000001"
# illumina-dump cant dump the same table as the other dumpers
ILLACC="DRR000001"
# the range of ids we'll dump
MIN=460000
MAX=470000
# all dump utilities we will test
DUMPERS="abi-dump fastq-dump illumina-dump sff-dump sra-dump vdb-dump"


# -----
# define functions to execute specific dump utilities

# abi-dump does not use a common user interface like other 
# specific dumpers
do-abi-dump()
{
    if [ "$UNAME" = "Linux" ]
        then
        ABI_PATH="/panfs/traces01/sra0/SRR/000000"
    elif [ "$UNAME" = "Darwin" ]
        then
        ABI_PATH="/net/traces01/sra0/SRR/000000"
    else
        ABI_PATH="//panfs/traces01/sra0/SRR/000000"
    fi

    PROG=$PROGDIR$1
    if [ "$OS" = "win" ] ; then
        PROG=$PROG.exe
    fi

    CMD="$PROG -path $ABI_PATH -minSpotId $2 -maxSpotId $3 $4"
    echo -e $CMD "\n-----" &> ../$OS.log
    $CMD >>../$OS.log 2>&1
}

do-sra-dump()
{
    PROG=$PROGDIR$1
    if [ "$OS" = "win" ] ; then
        PROG=$PROG.exe
    fi

    CMD="$PROG --start $2 --stop $3 $4"
    echo -e $CMD "\n-----" &> ../$OS.log
    $CMD > $ACC-sra.txt 2>> ../$OS.log
}

do-vdb-dump()
{
    PROG=$PROGDIR$1
    if [ "$OS" = "win" ] ; then
        PROG=$PROG.exe
    fi

    CMD="$PROG $4 --rows $2-$3"
    echo -e $CMD "\n-----" &> ../$OS.log
    $CMD > $ACC-vdb.txt 2>>../$OS.log
}

# the common core dumpers all use the same interface
do-common-dump()
{
    PROG=$PROGDIR$1
    if [ "$OS" = "win" ] ; then
        PROG=$PROG.exe
    fi

    CMD="$PROG -N $2 -X $3 $4"
    echo -e $CMD "\n-----" &> ../$OS.log
    $CMD >> ../$OS.log 2>&1
}

# a generalized function to call the more specific functions for each dumper
do-dump()
{
    if [ "$VERBOSE" = "true" ] ; then
        echo -e "----------\n running $1 on $OS to $DIR/$1/$OS"
    fi
    # remove any old files
    rm -fr $DIR/$1/$OS.log $DIR/$1/$OS/*
    mkdir -p $DIR/$1/$OS
    (
        cd $DIR/$1/$OS
        case "$1" in
            abi-dump)
                do-abi-dump $1 $MIN $MAX $ACC
                ;;
            illumina-dump)
                do-common-dump $1 $MIN $MAX $ILLACC
                ;;
            sra-dump)
                do-sra-dump $1 $MIN $MAX $ACC
                ;;
            vdb-dump)
                do-vdb-dump $1 $MIN $MAX $ACC
                ;;
            *)
                do-common-dump $1 $MIN $MAX $ACC
                ;;
        esac
        if [ "$VERBOSE" = "true" ] ; then
            echo `ls -l | wc -l` "files(s) created"
        fi

        # a cygwin/windows thing about permissions
        if [ "$OS" = "win" ] ; then
            chmod -f -x ../$OS.log
        fi
    )

}

# -----
# main script body

# get the platform was are running on
UNAME=$(uname)

# now translate platform to an OS name we'll use throughout this script run
if [ "$UNAME" = "Linux" ] ; then
    OS="linux"
elif [ "$UNAME" = "Darwin" ] ; then
    OS="mac"
else
    OS="win"
fi

while getopts  dp:hv OPT; do

    case "$OPT" in
        h)
            echo $USAGE0
            echo $USAGE1
            echo $USAGE2
            echo $USAGE3
            echo $USAGE4
            echo $USAGE5
            exit 0
            ;;

        d)
            DIR=$OPTARG
            ;;

        p)
            PROGDIR=$OPTARG
            ;;

        v)
            VERBOSE="true"
            ;;

    esac
done

if [ "$DIR" != "." ] ; then
    echo "putting dumper output under directory <$DIR>"
fi

if [ "$PROGDIR" != "" ] ; then
    echo "using dumper binaries from <$PROGDIR>"
fi

# loop through defined dumpers to test
for D in $DUMPERS ; do
  do-dump $D
done

