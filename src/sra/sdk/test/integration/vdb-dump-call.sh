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

execute()
{
    echo $1
    eval $1
}


####################################################################
#   test the parameter
####################################################################
EXPECTED_ARGS=1
E_BADARGS=65

if [ $# -ne $EXPECTED_ARGS ]
then
  echo "Usage: `basename $0` vdb-database-path"
  exit $E_BADARGS
fi

if [ ! -d $1 ]
then
    echo "vdb-database-path not found!"
    exit $E_BADARGS
fi

outdir="vdbtools"
aligned_dir="$outdir/aligned"
unaligned_dir="$outdir/unaligned"

#execute "bam-load-call.pl small.bam ./ ref.cfg ttt"

####################################################################
#   remove the output-directory ( recursivly )
#   create the output-directories
####################################################################
execute "rm -rf $outdir"
execute "mkdir -p $aligned_dir"
execute "mkdir -p $unaligned_dir"

####################################################################
#   PREPARE vdb-dump command for PRIMARY_ALIGNMENT ( aligned rows )
####################################################################
cols="READ,CIGAR_LONG,CIGAR_SHORT,MAPQ,SPOT_GROUP"
src=$1
table="PRIMARY_ALIGNMENT"
v1="vdb-dump $src -T $table -f tab -C $cols"

####################################################################
#   PREPARE vdb-dump command for SECONDARY_ALIGNMENT( aligned rows )
####################################################################
table="SECONDARY_ALIGNMENT"
v2="vdb-dump $src -T $table -f tab -C $cols"

####################################################################
#   PREPARE sorting on READ (1st column)
####################################################################
# passing tab as field-seperator is not trivial
# sorting on the 1st column (READ)
s="sort -t $'\t' -k1,1"

####################################################################
#   PREPARE splitting by fields into column-files
####################################################################
# BEGIN { FS = \"\t\" }; --- sets the field-separator to tab
# split into multiple files
t="awk 'BEGIN { FS = \"\t\"};
{ print \$1 > \"$aligned_dir/SEQ\"}; 
{ print \$2 > \"$aligned_dir/CIGAR_L\" };
{ print \$3 > \"$aligned_dir/CIGAR_S\" };
{ print \$4 > \"$aligned_dir/MAPQ\" };
{ print \$5 > \"$aligned_dir/SPOT_GROUP\" };'"


####################################################################
#   *** produce the aligned output ***
#   join the output of the 2 vdb-dump-commands
#   pipe the output into sort
#   pipe the output into awk for splitting on column
####################################################################
execute "( $v1; $v2 ) | $s | $t"


####################################################################
#   PREPARE vdb-dump command for SEQUENCE ( for unaligned rows )
####################################################################
cols="PRIMARY_ALIGNMENT_ID,READ,(INSDC:quality:text:phred_33)QUALITY,SPOT_GROUP"
src=$1
table="SEQUENCE"
v1="vdb-dump $src -T $table -f tab -C $cols"

####################################################################
#   PREPARE filter to pass only rows where the 1st field is empty
#   (PRIMARY_ALIGNMENT_ID == "" ) ---> unaligned
####################################################################
# BEGIN { FS = \"\t\" }; --- sets the field-separator to tab
# { if ( \$1 == \"\" ) print } --- prints the line if the first field is empty
f="awk 'BEGIN { FS = \"\t\" }; { if ( \$1 == \"\" ) print }'"

####################################################################
#   PREPARE sorting on READ (2nd column)
####################################################################
s="sort -t $'\t' -k2,2"

####################################################################
#   PREPARE splitting by fields into column-files
####################################################################
# BEGIN { FS = \"\t\" }; --- sets the field-separator to tab
# split into multiple files ( skip 1st field )
t="awk 'BEGIN { FS = \"\t\"};
{ print \$2 > \"$unaligned_dir/SEQ\"}; 
{ print \$3 > \"$unaligned_dir/QUAL\" };
{ print \$4 > \"$unaligned_dir/SPOT_GROUP\" };'"


####################################################################
#   *** produce the unaligned output ***
#   call vdb-dump on SEQUENCE-table
#   pipe the output into filtering out only unaligned rows
#   pipe the output into sort
#   pipe the output into awk for splitting on column
####################################################################
execute "$v1 | $f | $s | $t"

echo "finished!"