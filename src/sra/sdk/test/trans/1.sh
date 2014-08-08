#!/bin/bash

execute()
{
    echo "$1 $2"
    eval "$1 $2"
}

texecute()
{
    echo "totalview $1 -a $2"
    eval "totalview $1 -a $2"
}

 HMM="000000000000000000000000"
 HRO="000010010000000000000000"
READ="ACGTACGTACGTACGTACGTACGT"
  RO="-f 2 -f -2"

 HMM1="00000000100010000100001000010100010000100"
 HRO1="00000000000010100100100000000000000000100"
READ1="GGGGAAACTCCGCTTCTAACAAGAATATATAAAATTAGTCC"
  RO1="-f -1 -f -1 -f 1 -f 1 -f -1"

 HMM2="0000000000000000000000000000000000000000100110000001100001100000000"
 HRO2="0000000000000000000000000000000000000000000000001001000001000000100"
READ2="TAGCCCCAGCTACTTGGGAGGCTGAAGCAGGAGAATGGCGGGAGGCTGAGGCAGGAGAATGGCATGA"
  RO2="-f 2 -f -1 -f -1 -f 1"

 HMM3="00000000000000000000000000000000000000000000"
 HRO3="10000000000000000000000000000000000000010000"
READ3="TAGCCCCAGCTACTTGGGAGGCTGAAGCAGGAGAATGGCGGGAG"
  RO2="-f -9 -f 1"

 HMM4="100010000000000000000000000000000000"
 HRO4="100000000000000000000000000000000000"
READ4="GTTTGCAAGAATGGGAATATGTCTTTTTCTTCCTGT"
  RO4="-f -1"

ACC1="/panfs/traces01/compress/DATA/exome/CEU/NA06984.mapped.illumina.mosaik.CEU.exome.20110521/non-quantized.csra"
ACC2="/panfs/traces01/compress/1KG/ASW/NA19625/genome.ILLUMINA.BWA.csra"
ACC3="/panfs/traces01/compress/1KG/ASW/NA19625/exome.ILLUMINA.MOSAIK.csra"
ACC4="/panfs/traces01/compress/qa/yaschenk/RICHA/NA12878.broad.hiseq.wgs.csra"
REF1="11"
REF2="CM000673.1"
REF3="MT"
REFOFFSET="80000"
REFLEN="30000"

########################################################################
#
#       test of alignment-iterator (inner most)
# -a1 ... takes HMM,HRO,READ,RO from commandline and shows this alignment
#
#       test of reference-iterator (outer most)
# -a2 ... takes file and refrence/range from commandline, shows depth of pileup
#
#       test of placement-iterator
# -a3 ... takes file and refrence/range from commandline, shows alignments

########################################################################

#execute "trans" "-a1 -p -10 -m $HMM -o $HRO -r $READ $RO -w hallo.txt"
#execute "trans" "-a1 -m $HMM4 -o $HRO4 -r $READ4 $RO4"

#execute "trans" "-a1 -m $HMM1 -o $HRO1 -r $READ1 $RO1"
#execute "trans" "-a2 -n $ACC4 -e 1 -f 14000 -l 1"
#execute "trans" "-a1 -m $HMM2 -o $HRO2 -r $READ2 $RO2"

#execute "trans" "-a2 -n $ACC1 -e $REF1 -f $REFOFFSET -l $REFLEN"
#execute "trans" "-a2 -n $ACC1 -e $REF1 -f $REFOFFSET -l $REFLEN -d 0 -i 0 > 1.txt"
#execute "trans" "-a2 -n $ACC1 -e $REF1 -f $REFOFFSET -l $REFLEN -d 0 > 1.txt"

#execute "trans" "-a2 -n $ACC1 -e $REF1 -f 103500 -l 50"

#execute "trans" "-a3 -n $ACC1 -e $REF1 -f 87150 -l 343"
#execute "trans" "-a3 -n $ACC1 -e $REF1 -f 87150 -l 100"
#execute "trans" "-a3 -n $ACC1 -e $REF1 -f 87150 -l 344"
#execute "trans" "-a3 -n $ACC3 -e MT -f 1 -l 10"
#execute "trans" "-a3 -n $ACC4 -e 1 -f 14000 -l 1"

#execute "trans" "-a4 -n $ACC2 -e 1 -f 15274 -l 1"
#execute "trans" "-a4 -n $ACC2 -e 1 -f 15274 -l 1 -f 15283 -l 1"

#-n ... csra-object
#-e ... prim = 1 / sec = 2
#-f ... alignment-id
#execute "trans" "-a5 -n $ACC1 -e 1 -f 118"

execute "trans" "-a6 -n tokens.txt"