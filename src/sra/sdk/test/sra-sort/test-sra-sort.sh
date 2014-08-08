#!/bin/bash

execute()
{
    echo $1
    eval $1
}


perform_test()
{
    echo "perform pileup of the 'original' csra-file"
    execute "rm $DST/pileup.1.txt $DST/pileup.2.txt"
    execute "time sra-pileup $1 -o $DST/pileup.1.txt"

    echo "perform sra-sort"
    execute "rm -rf $DST/1"
    execute "time $SRASORT $1 $DST/1"

    echo "transorm into archive"
    execute "rm $DST/1.csra"
    execute "time kar -d $DST/1 -c $DST/1.csra"

    echo "perform pileup of the sorted archive"
    execute "time sra-pileup $DST/1.csra -o $DST/pileup.2.txt"

    echo "compare both pileup's "
    execute "diff $DST/pileup.1.txt $DST/pileup.2.txt > diff.txt"
    execute "head diff.txt"
}

SRASORT="/home/klymenka/OUTDIR_NOW/centos/gcc/stat/x86_64/dbg/bin/sra-sort"
DST="/panfs/pan1/sra-test/raetz"

SRC1="/panfs/pan1/trace-flatten/HG00121.chrom20.SOLID.bfast.GBR.low_coverage.20101123.bam"
SRC2="/panfs/pan1/sra-test/bam/HG00121.chrom20.SOLID.bfast.GBR.low_coverage.20101123.bam.csra"

SRC3="/panfs/pan1/sra-test/bam/HG00155.chrom20.ILLUMINA.bwa.GBR.low_coverage.20101123.bam.csra"

SRC4="/panfs/pan1/sra-test/bam/NA18525.mapped.SOLID.bfast.CHB.low_coverage.20101123.bam.csra"
SRC5="/panfs/pan1/sra-test/bam/NA18595.mapped.illumina.mosaik.CHB.exome.20110521.bam.csra"
SRC6="/panfs/pan1/sra-test/bam/NA06986.chromMT.ILLUMINA.bwa.CEU.exon_targetted.20100311.bam.csra"
SRC7="/panfs/pan1/sra-test/bam/NA18612.chrom20.ILLUMINA.bwa.CHB.exon_targetted.20100311.bam.csra"
SRC8="/panfs/pan1/sra-test/bam/NA06986.nonchrom.ILLUMINA.bwa.CEU.exon_targetted.20100311.bam.csra"
SRC9="/panfs/pan1/sra-test/bam/NA18628.chromY.LS454.ssaha2.CHB.exon_targetted.20100311.bam.csra"
SRC10="/panfs/pan1/sra-test/bam/NA07346.mapped.LS454.ssaha2.CEU.low_coverage.20101123.bam.csra"
SRC11="/panfs/pan1/sra-test/bam/NA18628.nonchrom.LS454.ssaha2.CHB.exon_targetted.20100311.bam.csra"
SRC12="/panfs/pan1/sra-test/bam/NA12043.chrom20.LS454.ssaha2.CEU.low_coverage.20101123.bam.csra"
SRC13="/panfs/pan1/sra-test/bam/NA18633.mapped.ILLUMINA.bwa.CHB.low_coverage.20101123.bam.csra"
SRC14="/panfs/pan1/sra-test/bam/NA12043.unmapped.LS454.ssaha2.CEU.low_coverage.20101123.bam.csra"
SRC15="/panfs/pan1/sra-test/bam/NA19172.unmapped.ILLUMINA.bwa.YRI.low_coverage.20101123.bam.csra"
SRC16="/panfs/pan1/sra-test/bam/NA12878.chrom11.ILLUMINA.bwa.CEU.high_coverage.20100311.bam.csra"
SRC17="/panfs/pan1/sra-test/bam/NA19238.nonchrom.ILLUMINA.bwa.YRI.high_coverage.20100311.bam.csra"
SRC18="/panfs/pan1/sra-test/bam/NA12878.chrom20.ILLUMINA.bwa.CEU.high_coverage.20100311.bam.csra"
SRC19="/panfs/pan1/sra-test/bam/NA19239.unmapped.ILLUMINA.bwa.YRI.high_coverage.20100311.bam.csra"
SRC20="/panfs/pan1/sra-test/bam/NA12878.chrom4.SOLID.bfast.CEU.high_coverage.20100125.bam.csra"
SRC21="/panfs/pan1/sra-test/bam/NA19240.chrom20.LS454.ssaha2.YRI.high_coverage.20100311.bam.csra"
SRC22="/panfs/pan1/sra-test/bam/NA12878.nonchrom.SOLID.bfast.CEU.high_coverage.20100125.bam.csra"
SRC23="/panfs/pan1/sra-test/bam/NA19240.chromY.LS454.ssaha2.YRI.high_coverage.20100311.bam.csra"
SRC24="/panfs/pan1/sra-test/bam/NA12878.unmapped.ILLUMINA.bwa.CEU.exon_targetted.20100311.bam.csra"
SRC25="/panfs/pan1/sra-test/bam/NA19240.nonchrom.LS454.ssaha2.YRI.high_coverage.20100311.bam.csra"
SRC26="/panfs/pan1/sra-test/bam/NA18147.chrom20.LS454.ssaha2.CHD.exon_targetted.20100311.bam.csra"
SRC27="/panfs/pan1/sra-test/bam/NA19240.unmapped.LS454.ssaha2.YRI.high_coverage.20100311.bam.csra"
SRC28="/panfs/pan1/sra-test/bam/NA18147.unmapped.LS454.ssaha2.CHD.exon_targetted.20100311.bam.csra"

SCRIPT="try-sra-sort-on-one-accession.sh"
SRC=`srapath SRR341548`
SCRATCH="/panfs/pan1/sra-test/raetz/3"
LOG="/panfs/pan1/sra-test/raetz/3.log"

execute "$SCRIPT $SRC $SCRATCH $LOG"
