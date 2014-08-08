#!/bin/bash

#this script tries to verify that sra-sort does not change the output of sra-pileup

#try-sra-sort-on-one-accession.sh INPUT SCRATCH LOG

#INPUT   ... absolute path to input-csra-file or accession
#SCRATCH ... location of temp. created directory and file
#LOG     ... absolute path to log-file

#example:   try-sra-sort-on-one-accession.sh
#               /panfs/pan1/sra-text/bam/HG00121.chrom20.SOLID.bfast.GBR.low_coverage.20101123.bam.csra
#               /export/home/TMP/raetz/1
#               /panfs/pan1/sra-test/raetz/log.txt

# the INPUT must exist ( will be tested )

# the script will create a directory '/export/home/TMP/raetz/1' and files like '/export/home/TMP/raetz/1.csra'
# the parent directory of these must exist ( in this case '/export/home/TMP/raetz' ), not tested (yet)

# the script will create the logfile and append to it, overwrite it if it exists
# the parent directory of the script file must exist, not tested (yet)

# =====================================================================================================

ACCESSION="$1"
SCRATCH="$2"
LOG_FILE="$3"

SRASORT="/home/klymenka/OUTDIR_NOW/centos/gcc/stat/x86_64/dbg/bin/sra-sort"

log()
{
    echo -e "$1" >> $LOG_FILE
}

execute()
{
    log "STEP $1 = executing '$2'"
    TM=$( { time eval $2; } 2>&1 )
    log "$TM\n\n"
}

echo -e "\n=========================================================\n" > $LOG_FILE

#document the input-parameters
log "`date` running 'try-sra-sort-on-one-accession.sh'\n"
log "ACCESSION ....... $ACCESSION"
log "SCRATCH   ....... $SCRATCH"
log "LOG_FILE ........ $LOG_FILE\n"

#test if the ACCESSION/INPUTFILE can be found...
if [ -f $ACCESSION ];
then
    log "File '$ACCESSION' exists.\n\n"
else
    CMD="srapath $ACCESSION &> /dev/null"
    eval $CMD
    ret_code=$?
    if [ $ret_code != 0 ];
    then
        log "File and ACCESSION '$ACCESSION' does not exist.\nexit script"
        exit $ret_code
    else
        log "ACCESSION '$ACCESSION' found."
    fi
fi


# STEP 1 ... create the pileup-file of the INPUT
execute "1" "sra-pileup $ACCESSION -n -o ${SCRATCH}.pileup.org.txt"

# STEP 2 ... perform sra-sort on the INPUT
rm -rf ${SCRATCH} &> /dev/null
execute "2" "$SRASORT $ACCESSION ${SCRATCH}"

# STEP 3 ... transform into csra
rm -rf ${SCRATCH}.csra &> /dev/null
execute "3" "kar -d ${SCRATCH} -c ${SCRATCH}.csra"

# STEP 4 ... perform pileup of sorted archive
execute "4" "sra-pileup ${SCRATCH}.csra -n -o ${SCRATCH}.pileup.sorted.txt"

# STEP 5 ... compare the pileup outputs
execute "5" "diff ${SCRATCH}.pileup.org.txt ${SCRATCH}.pileup.sorted.txt > ${SCRATCH}.pileup.diff.txt"

log "\n=========================================================\n"