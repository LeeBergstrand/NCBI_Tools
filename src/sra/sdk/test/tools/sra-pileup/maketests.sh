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
SOURCE_CSRA=$1
TEMP_SAM_FILE="temp.sam"
TEMP_BAM_FILE="temp.bam"

export PATH=/panfs/pan1/sra-test/bin/:$PATH
sam-dump ${SOURCE_CSRA} >${TEMP_SAM_FILE}
samtools view -bt $2.vdb.txt -o ${TEMP_BAM_FILE} ${TEMP_SAM_FILE}


#samtools view -bt test11.vdb.txt -o test11.bam test11.sam
#samtools mpileup test11.bam #-r MT:16568-1000000
#exit

# convert a csra file to bam and index in preparation for samtools mpileup
# $1 - source csra file 
# $2 - test name
# $3 - reference file name
# $4 - reference name
function makeexp
{
#    sam-dump $1 >$2.sam
    samtools view -bt $2.vdb.txt -o $2.bam $2.sam
    samtools index $2.bam
#    vdb-dump $3 -C READ -f tab >$2.tmp
#    echo ">$2\n" | cat - $2.tmp >$2.fasta
#    samtools faidx $2.fasta
}

# prepare input .bam files:
#test1:  copied from /panfs/pan1/sra-test/bam/NA06986.chromMT.ILLUMINA.bwa.CEU.exon_targetted.20100311.bam.csra
#test2:  copied from /panfs/pan1/sra-test/bam/NA06986.nonchrom.ILLUMINA.bwa.CEU.exon_targetted.20100311.bam.csra
#test3:  copied from /panfs/pan1/sra-test/bam/NA19240.chromY.LS454.ssaha2.YRI.high_coverage.20100311.bam.csra
#test4: is a manually modified test1 (replicated one alignment to create overlapping placements)
#makeexp /panfs/pan1/sra-test/bam/NA06986.chromMT.ILLUMINA.bwa.CEU.exon_targetted.20100311.bam.csra  test1 /panfs/traces01/refseq/CM000686.1 MT
#makeexp /panfs/pan1/sra-test/bam/NA06986.nonchrom.ILLUMINA.bwa.CEU.exon_targetted.20100311.bam.csra test2 /panfs/traces01/refseq/GL000226.1 GL000226.1
#makeexp /panfs/pan1/sra-test/bam/NA06986.nonchrom.ILLUMINA.bwa.CEU.exon_targetted.20100311.bam.csra test2 /panfs/traces01/refseq/GL000212.1 GL000212.1
#makeexp /panfs/pan1/sra-test/bam/NA19240.chromY.LS454.ssaha2.YRI.high_coverage.20100311.bam.csra    test3 /panfs/traces01/refseq/CM000686.1 Y

#makeexp qq test 
#mkdir expected
# run mpileup to create expected results
samtools mpileup test.bam -f MT.fasta -r MT:100-200     >expected/1-1 # no placements in the reference window (RW)
samtools mpileup test.bam -f MT.fasta -r MT:600-1000    >expected/1-2 # 1 placement fully inside the RW
samtools mpileup test.bam -f MT.fasta -r MT:600-5040    >expected/1-3 # 2 placements fully inside RW, no overlap
samtools mpileup test.bam -f MT.fasta -r MT:900-1000    >expected/1-4 # 1 placement, starts outside RW
samtools mpileup test.bam -f MT.fasta -r MT:800-900     >expected/1-5 # 1 placement, ends outside RW
samtools mpileup test.bam -f MT.fasta -r MT:900-920     >expected/1-6 # 1 placement, starts and ends outside RW
samtools mpileup test.bam -f MT.fasta -r MT:9300-9400   >expected/2-2 # 2 placements, overlap
samtools mpileup test.bam -f MT.fasta -r MT:10000-11000 >expected/2-3 # 3 placements overlap
samtools mpileup test.bam -f MT.fasta -r MT:13000-14000 >expected/2-4 # 3 placements overlap, some start at the same pos within RW
samtools mpileup test.bam -f MT.fasta -r MT:0-1000      >expected/3-1 # RW starts at 0
samtools mpileup test.bam -f MT.fasta -r MT:1-1000      >expected/3-2 # RW starts at the beginning of the reference
samtools mpileup test.bam -f MT.fasta -r MT:1-16573     >expected/3-3 # RW ends at the end of the reference
samtools mpileup test.bam -f MT.fasta -r MT:1-1657400   >expected/3-4 # RW ends beyond the end of the reference
samtools mpileup test.bam -f MT.fasta -r MT:896-800     >expected/3-5 # RW size is negative
samtools mpileup test.bam -f MT.fasta -r MT:896-896     >expected/3-6 # RW size is 1 

