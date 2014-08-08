#!/bin/bash

execute()
{
    echo $1
    eval $1
}

if [ -d /panfs ]; then
    panfs=/panfs
    domain=.be-md.ncbi.nlm.nih.gov
else
    panfs=/net
    domain=
fi
 
ACCPATH="$panfs/traces01$domain/compress/DATA/CEU"

ACC1="$ACCPATH/NA06984.mapped.illumina.mosaik.CEU.exome.20110521/non-quantized.csra"
ACC2="$ACCPATH/NA06986.mapped.illumina.mosaik.CEU.exome.20110411/non-quantized.csra"
ACC3="$ACCPATH/NA06989.mapped.illumina.mosaik.CEU.exome.20110411/non-quantized.csra"
ACC4="$ACCPATH/NA06994.mapped.illumina.mosaik.CEU.exome.20110411/non-quantized.csra"
ACC5="$ACCPATH/NA07037.mapped.illumina.mosaik.CEU.exome.20110521/non-quantized.csra"
ACC5="$ACCPATH/NA07048.mapped.illumina.mosaik.CEU.exome.20110411/non-quantized.csra"
ACC6="$ACCPATH/NA07051.mapped.illumina.mosaik.CEU.exome.20110411/non-quantized.csra"
ACC7="$ACCPATH/NA07347.mapped.illumina.mosaik.CEU.exome.20110411/non-quantized.csra"
ACC8="$ACCPATH/NA10847.mapped.illumina.mosaik.CEU.exome.20110411/non-quantized.csra"
ACC9="$ACCPATH/NA11840.mapped.illumina.mosaik.CEU.exome.20110411/non-quantized.csra"
ACC10="$ACCPATH/NA11843.mapped.illumina.mosaik.CEU.exome.20110521/non-quantized.csra"
ACC11="/panfs/traces01.be-md.ncbi.nlm.nih.gov/compress/qa/yaschenk/CGNative/SRR554096/SRR554096.sra"

ACCA="$ACC1 $ACC2 $ACC3 $ACC4 $ACC5 $ACC6 $ACC7 $ACC8 $ACC9 $ACC10"

BAM1="$panfs/traces01$domain/compress/DATA/exome/CEU/NA06984.mapped.illumina.mosaik.CEU.exome.20110521/non-quantized.recalibrated.chr20only.bam"
REF1="11"
REF2="CM000673.1"
START="80000"
START1="87050"
END="90000"
DST="$panfs/pan1$domain/sra-test/raetz/pileup/NA06984.txt"
SAMTOOLS="/netopt/ncbi_tools64/samtools/bin/samtools"
MPILEUP="$SAMTOOLS mpileup"

#execute "sra-pileup $ACC2 -r 11:120120-120500 -r 12:80000-85000 -o 5.txt"
#execute "sra-pileup $ACC2 -r 11:120120-120500 -r 12:80000-85000 -o 5.txt"
#execute "time sra-pileup $ACCA -r 11:120120-120500 -r 12:80000-85000 -o 5.txt"
#execute "time sra-pileup $ACC1 -r 11 -o 5.txt"

#execute "valgrind --log-file=memloss.txt sra-pileup $ACC1 -r 11:120120-120500 -o 5.txt"

# start is before the one and only alignment, end is after it...
#execute "sra-pileup $ACC1 -r $REF1:120120-120500 -o 1.txt"

# start is after the start of the one and only alignment, end is after it...
#execute "sra-pileup $ACC1 -r $REF1:120246-120500 -o 2.txt"

# start is before the one and only alignment, end is before it ends it...
#execute "sra-pileup $ACC1 -r $REF1:120120-120300 -o 3.txt"

# start is after the start the one and only alignment, end is before it ends it...
#execute "sra-pileup $ACC1 -r $REF1:120246-120300 -o 4.txt"

execute "sra-pileup $ACC1 -r $REF1:120120-123050 -o $DST"

#execute "totalview sra-pileup -a $ACC1 -r $REF1:$REFOFFSET-$REFLEN"
#execute "sra-pileup $ACC1"

#execute "time sra-pileup $ACC1 -r $REF1 -o $DST"
#execute "time $MPILEUP $BAM1"

#sra-pileup -f /dev/fd/0 <<LIMITxxxLIMIT
#-r MT:1-20
#/panfs/traces01/compress/DATA/exome/CEU/NA06984.mapped.illumina.mosaik.CEU.exome.20110521/non-quantized.csra
#LIMITxxxLIMIT

#sra-pileup -f /dev/fd/0 < params.txt

#execute "sra-pileup $ACC1 -r 11:87150-87494"
#execute "sra-pileup $ACC1 -r 11:87150-87493 -e"
#execute "sra-pileup \"ncbi-file:${ACC1}?readgroup=something\" -r 11:87150-87150 -e"