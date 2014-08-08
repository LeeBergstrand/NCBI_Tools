#!/bin/sh
# $Id: copybin.sh 190015 2010-04-27 17:14:13Z gouriano $
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
# Author:  Andrei Gourianov, NCBI (gouriano@ncbi.nlm.nih.gov)
#
# Copy binaries into bin directory
#
# DO NOT ATTEMPT to run this script manually
# ===========================================================================

for v in "$BUILD_TREE_BIN" "$BUILD_TREE_ROOT"; do
  if test "$v" = ""; then
    echo ERROR: required environment variable is missing
    echo DO NOT ATTEMPT to run this script manually
    exit 1
  fi
done

test -d ${BUILD_TREE_BIN} || mkdir -p ${BUILD_TREE_BIN}
echo ${CONFIGURATION} | grep elease > /dev/null
if test $? -eq 0 -o "$PTB_NOCOPYBIN" != ""; then
  mv -f ${TARGET_BUILD_DIR}/${TARGET_NAME}* ${BUILD_TREE_BIN}
else
  cp -f -p -R ${TARGET_BUILD_DIR}/${TARGET_NAME}* ${BUILD_TREE_BIN}
fi
exit 0
