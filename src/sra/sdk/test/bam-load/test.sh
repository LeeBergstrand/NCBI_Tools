#!/bin/bash

TEMP='/export/home/TMP'
SV='java -jar /panfs/pan1/trace_work/backup/packages/picard-tools-1.50/ValidateSamFile.jar'

# find binaries
BL=$(test -f ./bam-load && echo ./bam-load) || $(which bam-load) || exit 1
SD=$(test -f ./sam-dump && echo ./sam-dump) || $(which sam-dump) || exit 1
AR=$(test -f ./kar && echo ./kar) || $(which kar) || exit 1
VD=$(test -f ./vdb-dump && echo ./vdb-dump) || $(which vdb-dump) || exit 1
ST=$(test -f ./samtools && echo ./samtools) || $(which samtools) || { ST=${NCBI}/samtools/bin/samtools;  test -f $ST || exit 1; }

echo "TOOLS: '$BL $SD $AR $VD $ST'"

ID=1.${$}

# find source file
SRC='/panfs/pan1/trace-flatten/durbrowk/bam.1/in.bam'
[ -r ${SRC} ] || { echo ${SRC} is not readable ; exit 1; }

CFG='/panfs/pan1/trace-flatten/durbrowk/bam.1/analysis.bam.cfg'
[ -r ${CFG} ] || { echo ${CFG} is not readable ; exit 1; }

if [ ${BL} -nt bam.1.csra ]; then
        echo 'bam-load is newer than bam.1.csra; reloading test BAM...'

        # load bam
        ${BL} -Lerr -Q 0 --use-QUAL --accept-dups -t ${TEMP} -o ${TEMP}/bam.${ID} -k ${CFG} ${SRC} || exit 1;

        # turn into csra
        [ -r bam.1.csra ] && EXEC rm -rf bam.1.csra;
        ${AR} -c bam.1.csra -d ${TEMP}/bam.${ID};

        # clean up temporary
        rm -rf ${TEMP}/bam.${ID};
fi

echo 'generating sorted BAM via sam-dump and samtools...'
( ${ST} view -H ${SRC} ; ${SD} --no-header --reverse --unaligned bam.1.csra | sort -t$'\t' -k 3,3 -k 4,4n -k5,5nr -k 10,11 | tee ${TEMP}/bam.${ID}.sorted.sam ) | ${ST} view -bS - -o ${TEMP}/bam.${ID}.bam

echo "input flagstats:"
${ST} flagstat ${SRC} > sam.flagstat

echo "output flagstats:"
${ST} flagstat ${TEMP}/bam.${ID}.bam > vdb.flagstat

echo 'validating sorted BAM with picard...'
${SV} IGNORE=MISMATCH_FLAG_MATE_NEG_STRAND INPUT=${TEMP}/bam.${ID}.bam

# clean up
rm -rf ${TEMP}/bam.${ID}.bam

echo 'fixing up output from samtools...'
${ST} view ${SRC} | gawk 'BEGIN { OFS="\t"; } { if (and($2,4)!=0||$3=="*") { $4=$5=0; $3=$6="*"; } print; }' | sort -t$'\t' -k 3,3 -k 4,4n -k5,5nr -k 10,11 | cut -f 3-6,10,11 > ${TEMP}/bam.${ID}.sam.cut

echo 'fixing up output from sam-dump...'
cut -f 3-6,10,11 ${TEMP}/bam.${ID}.sorted.sam > ${TEMP}/bam.${ID}.vdb.cut

# clean up
rm -rf ${TEMP}/bam.${ID}.sorted.sam

diff --brief ${TEMP}/bam.${ID}.sam.cut ${TEMP}/bam.${ID}.vdb.cut && { echo "Test passed; the files do not differ"; rm -rf ${TEMP}/bam.${ID}*; exit 0; }
echo Test failed; see ${TEMP}/bam.${ID}.sam.cut and ${TEMP}/bam.${ID}.vdb.cut
echo rm ${TEMP}/bam.${ID}.sam.cut ${TEMP}/bam.${ID}.vdb.cut
