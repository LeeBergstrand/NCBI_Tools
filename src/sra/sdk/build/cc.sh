#!/bin/bash
# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================

# prepare script name
SELF_NAME="$(basename $0)"
SCRIPT_BASE="${0%.sh}"

# os
OS="$1"
shift

# binary compiler
CC="$1"
shift

# configuration
unset TARG
unset ARGS
CHECKSUM=0
OBJX=o

while [ $# -ne 0 ]
do

    case "$1" in
    --cflags)
        ARGS="$ARGS $2"
        shift
        ;;

    --checksum)
        CHECKSUM=1
        ;;

    --objx)
        OBJX="$2"
        shift
        ;;

    -o*)
        ARGS="$ARGS $1"
        ARG="${1#-o}"
        if [ "$ARG" = "" ]
        then
            ARGS="$ARGS $2"
            ARG="$2"
            shift
        fi
        TARG="$ARG"
        ;;

    *)
        ARGS="$ARGS $1"
        ;;
        
    esac

    shift
done

# *** START SCM-code ***************************************
if [ $CHECKSUM -eq 1 ]
then
    if [ "$TARG" = "" ]
    then
        echo "$SELF_NAME: no target specified"
        exit 5
    fi

    if [ "$TARG" = "${TARG%.$OBJX}" ]
    then
        echo "$SELF_NAME: malformed target"
        exit 6
    fi

    TARG="${TARG%.$OBJX}"
fi
# *** END SCM-code ***************************************

CMD="$CC $ARGS"
echo "$CMD"
$CMD || exit $?

# *** START SCM-code ***************************************
if [ $CHECKSUM -eq 1 ] && [ "$OS" = "linux" ]
then
    CMD="strip $TARG.$OBJX -o $TARG.stripped.$OBJX"
    echo "$CMD"
    if $CMD
    then
        MD5RES=`md5sum -b $TARG.stripped.$OBJX`
        STATUS=$?
        rm -f "$TARG.stripped.$OBJX" || true
        if [ $STATUS -eq 0 ]
            then
            MD5VALUE=${MD5RES:0:32}
            echo "$TARG.$OBJX=$MD5VALUE" > "$TARG.$OBJX.md5"
            else
            exit $STATUS
        fi
    else
        STATUS=$?
        rm "$TARG.$OBJX"
        exit $STATUS
    fi
fi
# *** END SCM-code ***************************************
