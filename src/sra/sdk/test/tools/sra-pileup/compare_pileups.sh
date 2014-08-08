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

# run samtools pileup and sra-pileup from the same source and compare outputs
# $1 - source csra file 
# $* - extra arguments to pileup programs (e.g. -r 20:59000-60000 to specify a reference window)
SOURCE_CSRA=$1
shift 1

TEMP_SAM_FILE="temp.sam"
TEMP_BAM_FILE="temp.bam"
REFERENCE_FASTA="/panfs/pan1/sra-test/bam/chrom20.fasta"
SAMTOOLS_PILEDUP="samtools.piledup"
SRA_PILEDUP="sra.piledup"
DIFF="diff.txt"

export PATH=/panfs/pan1/sra-test/bin/:$PATH

CMD="sam-dump ${SOURCE_CSRA} >${TEMP_SAM_FILE}"
echo ${CMD}
eval ${CMD}

CMD="samtools view -bt $2.vdb.txt -o ${TEMP_BAM_FILE} ${TEMP_SAM_FILE}"
echo "${CMD}"
eval ${CMD}

CMD="samtools index ${TEMP_BAM_FILE}"
echo "${CMD}"
eval ${CMD}

#rm -f ${TEMP_SAM_FILE}

CMD="samtools mpileup  ${TEMP_BAM_FILE} -f ${REFERENCE_FASTA} $* >${SAMTOOLS_PILEDUP}"
echo ${CMD}
eval ${CMD}

#rm -f ${TEMP_BAM_FILE}

CMD="sra-pileup ${SOURCE_CSRA} -o ${SRA_PILEDUP} $*"
echo ${CMD}
eval ${CMD}

CMD="diff ${SAMTOOLS_PILEDUP} ${SRA_PILEDUP} >${DIFF}"
echo ${CMD}
eval ${CMD}

exit $?
