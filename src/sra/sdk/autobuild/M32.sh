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
# Mac Dynamic Debug 64bit build in a cron context
#
# $1 - top makefile directory
# $2 - linkage (dynamic/static, default = static)
# return:
# 0 - OK
# 1 - wrong OS
# 2 - wrong architecture (not used)
# 3 - build failed
# 4 - build ok, tests failed

OS=$(uname -s)
if [ "${OS}" != "Darwin" ] ; then
    echo "not a Darwin OS: ${OS}" >&2
    exit 1
fi

if [ "$2" = "" ]
then
    LINKAGE="static"
else
    LINKAGE=$2
fi

OUTPUT=$1/mac/dbg/gcc/${MARCH}
export LD_LIBRARY_PATH=${OUTPUT}/lib:${OUTPUT}/ilib:$LD_LIBRARY_PATH
export PATH=${OUTPUT}/bin:/bin:$PATH
export NCBI=/netopt/ncbi_tools
export SRA_TEST=/net/pan1/sra-test
export SAMTOOLS=${SRA_TEST}/bin/mac/samtools
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}" >&2
cd $1

make NOREBUILD_LINKS=1 local i386 $LINKAGE debug clean config std 
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
