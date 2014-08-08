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
#
# Linux Dynamic Debug 64bit build and unit-tested in a cron context
#
# $1 - top makefile directory
# $2 - linkage (dynamic/static, default = static)
# return:
# 0 - OK
# 1 - wrong OS
# 2 - wrong architecture
# 3 - build failed
# 4 - build ok, tests failed

OS=$(uname -s)
if [ "${OS}" != "Linux" ] ; then
    echo "not a Linux OS: ${OS}" >&2
    exit 1
fi

export MARCH=$(/usr/bin/uname -i)
if [ "$MARCH" = "i386" ]
then
    export ARCH_BITS=32
else
    echo "not a 32 bit machine" >&2
    exit 2
fi

if [ "$2" = "" ]
then
    LINKAGE="static"
else
    LINKAGE=$2
fi

export VDB_BUILD_TOOLS=/net/snowman/vol/projects/trace_software/tools/$MARCH
export PATH=/usr/local/gcc/4.6.0/bin/:$VDB_BUILD_TOOLS/bin:$PATH
export PATH=/usr/local/bison/2.5/bin/:/usr/local/flex/2.5.35/bin/:$PATH

OUTPUT=$1/centos/dbg/gcc/$MARCH
export LD_LIBRARY_PATH=$OUTPUT/lib:$OUTPUT/ilib:$LD_LIBRARY_PATH
export PATH=$OUTPUT/bin:/bin:$PATH
export NCBI=/netopt/ncbi_tools
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}" >&2
echo "PATH=${PATH}" >&2
which gcc
cd $1

make NOREBUILD_LINKS=1 local $LINKAGE debug clean config std
if [ "$?" != "0" ] ; then
    exit 3
fi

# make sure all tools use the correct schema
vdb-config --set vdb/schema/paths="$1/interfaces"

make NOREBUILD_LINKS=1 runtests
if [ "$?" != "0" ] ; then
    exit 4
fi
exit 0
