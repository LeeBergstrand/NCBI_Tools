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
#echo "$0 $*"

# ===========================================================================
# input library types, and their handling
#
#  normal or static linkage
#   -l : require static
#   -s : require static
#   -d : ignore
# ===========================================================================

# script name
SELF_NAME="$(basename $0)"

# parameters and common functions
source "${0%rwin.slib.sh}win.cmn.sh"
RHOST=$1
RPORT=$2
RHOME=$3
LHOME=$4
PROXY_TOOL=$5

# initialize command
if [ $STATIC -eq 0 ]
then
    CMD="lib /NOLOGO"
else
    CMD="ar -rc"
fi

# function to convert an archive into individual object files
convert-static ()
{
    if [ $STATIC -eq 0 ]
    then
        WINLIB="$RHOME${1#$LHOME}"
        WINLIB="$(echo $WINLIB | tr '/' '\\')"

        CMD="$CMD $WINLIB"
    else
        # list members
        local path="$1"
        local mbrs="$(ar -t $path)"

        # unpack archive into temporary directory
        mkdir -p ld-tmp
        if ! cd ld-tmp
        then
            echo "$SELF_NAME: failed to cd to ld-tmp"
            exit 5
        fi
        ar -x "$path"

        # rename and add to source files list
        local m=
        for m in $mbrs
        do
            mv $m $NAME-$m
            CMD="$CMD ld-tmp/$NAME-$m"
        done

        # return to prior location
        cd - > /dev/null
    fi
}

if [ $STATIC -eq 0 ]
then 
    WINTARG="$RHOME${TARG#$LHOME}"
    WINTARG="$(echo $WINTARG | tr '/' '\\')"

    # no versioned output here, sorry
    CMD="$CMD /OUT:$WINTARG"

    # tack on object files
    CMD="$CMD $OBJS"
else
    CMD="$CMD $TARG $OBJS"
fi

# initial dependency upon Makefile and vers file
DEPS="$SRCDIR/Makefile"
if [ "$LIBS" != "" ]
then
    # tack on libraries, finding as we go
    for xLIB in $LIBS
    do
        # strip off switch
        xLIBNAME="${xLIB#-[lsd]}"

        # look at linkage
        case "$xLIB" in
        -s*)

            # force static load
            find-lib $xLIBNAME.a $LDIRS # .a for static, .lib for dynamic?
            if [ "$xLIBPATH" != "" ]
            then

                # add it to dependencies
                DEPS="$DEPS $xLIBPATH"

                # convert to individual object files
                convert-static "$xLIBPATH" || exit $?

            fi
            ;;

        esac
    done
fi

# produce static library
rm -f $TARG
echo $CMD

if [ $STATIC -eq 0 ]
then
    # determine current directory
    CURDIR="$(pwd)"    
    ${TOP}/build/run_remotely.sh $PROXY_TOOL $RHOST $RPORT $RHOME $LHOME $CURDIR $CMD 
    STATUS=$?
    if [ "$STATUS" = "0" ]
    then
        # wait for the result file to appear (there may be a network delay)
        ${TOP}/build/wait_for_file.sh ${TARG}
        STATUS=$?
        if [ "$STATUS" = "1" ]
        then
            echo "timed out, TARG='$TARG'"
        fi
    fi        
else
    $CMD || exit $?
    STATUS=0
fi

# remove temporaries
rm -rf ld-tmp

if [ "$STATUS" = "0" ]
then
    # produce dependencies
    if [ "$DEPFILE" != "" ] && [ "$DEPS" != "" ]
    then
        echo "$TARG: $DEPS" > "$DEPFILE"
    fi
fi    

exit $STATUS
