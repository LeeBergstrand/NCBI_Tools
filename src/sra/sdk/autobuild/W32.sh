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
# Windows (Remote) 32 bit Debug build in a cron context
#
# Acrhitecture is specified in the host definition file, must match the 
# environment running on the build server  
#
# $1 - top makefile directory
# $2 - remote host definition file (has to exist under $1/rhosts)
# $3 - linkage (dynamic/static, default = static)
# return:
# 0 - OK
# 1 - wrong OS
# 3 - build failed
# 4 - build ok, tests failed
echo "$0 $*"
if [ ! -f $1/rhosts/$2 ] ; then
    echo "host file does not exist: $1/rhosts/$2" >&2
    exit 1
fi

if [ "$3" = "" ]
then
    LINKAGE="static"
else
    LINKAGE=$3
fi

umask 2 # give the server permission to update my files
cd $1

# need path to gcc and a few linux libraries to build proxy-exec
export VDB_BUILD_TOOLS=/net/snowman/vol/projects/trace_software/tools/$MARCH
export PATH=/usr/local/gcc/4.6.0/bin/:$VDB_BUILD_TOOLS/bin:$PATH
export NCBI=/netopt/ncbi_tools32

make NOREBUILD_LINKS=1 static $2
if [ "$?" != "0" ] ; then
    exit 3
fi

make NOREBUILD_LINKS=1 $LINKAGE debug clean std 
if [ "$?" != "0" ] ; then
    exit 3
fi

# make sure all tools use the correct schema
${OUTPUT}/vdb-config --set vdb/schema/paths="$1/interfaces"

make NOREBUILD_LINKS=1 runtests
if [ "$?" != "0" ] ; then
    exit 4
fi
exit 0
