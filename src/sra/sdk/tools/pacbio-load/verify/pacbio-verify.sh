#!/bin/bash

reload()
{
    pacbio-load $H5FILE -o $VDBDIR -fp
}

header()
{
    echo ""
    echo $1
    rm -f x1.bin x1.spot_len_f5 x1.dump_vdb
}

# $1...name of the HDF5-dump    ( "x1.dump_5" )
# $2...what was compared        ( "READ" )
# $3...table                    ( "CONSENSUS" )
diff_and_show()
{
    rm -f x1.bin
    echo -n " / vdb-dump"
    vdb-dump $VDBDIR -T $3 -C $2 -N -l0 > x1.dump_vdb
    echo -n " / diff: "
    diff $1 x1.dump_vdb > /dev/null
    LASTSTATUS=$?
    if [ $LASTSTATUS -eq 0 ];then
       echo "$2 load : OK"
    else
       echo "$2 load : differences found!"
    fi
}

compare_SEQ_SPOT_LEN()
{
    header "comparing SEQ.SPOT_LEN"

    #dump the hdf5-table-rows from NumEvent in binary form into the file x1.bin
    h5dump -d "/PulseData/BaseCalls/ZMW/NumEvent" -b MEMORY -o x1.bin $H5FILE > /dev/null

    #use hexdump to write it out, one 32-bit integer a line into the file x1.dump_h5
    hexdump x1.bin -e '1/4 "%d"' -e '"\n"' -v > x1.spot_len_h5

    # dump via vdb-dump, then compare the hdf5-dump vs. the vdb-dump
    diff_and_show x1.spot_len_h5 "SPOT_LEN" "SEQUENCE"
}

compare_SEQ_HOLE_STATUS()
{
    header "comparing SEQ.HOLE_STATUS"
    h5dump -d "/PulseData/BaseCalls/ZMW/HoleStatus" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/1 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "(U8)HOLE_STATUS" "SEQUENCE"
}

compare_SEQ_XY()
{
    header "comparing SEQ.X/Y"
    h5dump -d "/PulseData/BaseCalls/ZMW/HoleXY" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/2 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "X,Y" "SEQUENCE"
}

compare_SEQ_HOLENUMBER()
{
    header "comparing SEQ.HOLENUMBER"
    h5dump -d "/PulseData/BaseCalls/ZMW/HoleNumber" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/4 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "HOLE_NUMBER" "SEQUENCE"
}

compare_SEQ_READ()
{
    header "comparing vdb:'SEQ.READ' vs hdf5:'/PulseData/BaseCalls/Basecall'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/Basecall" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "READ" "SEQUENCE"
}

compare_SEQ_QUALITY()
{
    header "comparing vdb:'SEQ.QUALITY' vs hdf5:'/PulseData/BaseCalls/QualityValue'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/QualityValue" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_1.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "QUALITY" "SEQUENCE"
}

compare_SEQ_PRE_BASE_FRAMES()
{
    header "comparing vdb:'SEQ.PRE_BASE_FRAMES' vs hdf5:'/PulseData/BaseCalls/PreBaseFrames'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/PreBaseFrames" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_2.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "PRE_BASE_FRAMES" "SEQUENCE"
}

compare_SEQ_WIDTH_IN_FRAMES()
{
    header "comparing vdb:'SEQ.WIDTH_IN_FRAMES' vs hdf5:'/PulseData/BaseCalls/WidthInFrames'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/WidthInFrames" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_2.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "WIDTH_IN_FRAMES" "SEQUENCE"
}

compare_SEQ_PULSE_INDEX_16()
{
    header "comparing vdb:'SEQ.PULSE_INDEX(16 bit)' vs hdf5:'/PulseData/BaseCalls/PulseIndex'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/PulseIndex" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_2.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "PULSE_INDEX" "SEQUENCE"
}

compare_SEQ_PULSE_INDEX_32()
{
    header "comparing vdb:'SEQ.PULSE_INDEX(32 bit)' vs hdf5:'/PulseData/BaseCalls/PulseIndex'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/PulseIndex" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_4.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "PULSE_INDEX" "SEQUENCE"
}

compare_SEQ_INSERTION_QV()
{
    header "comparing vdb:'SEQ.INSERTION_QV' vs hdf5:'/PulseData/BaseCalls/InsertionQV'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/InsertionQV" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_1.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "INSERTION_QV" "SEQUENCE"
}

compare_SEQ_DELETION_QV()
{
    header "comparing vdb:'SEQ.DELETION_QV' vs hdf5:'/PulseData/BaseCalls/DeletionQV'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/DeletionQV" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_1.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "DELETION_QV" "SEQUENCE"
}

compare_SEQ_DELETION_TAG()
{
    header "comparing vdb:'SEQ.DELETION_TAG' vs hdf5:'/PulseData/BaseCalls/DeletionTag'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/DeletionTag" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "DELETION_TAG" "SEQUENCE"
}

compare_SEQ_SUBSTITUTION_QV()
{
    header "comparing vdb:'SEQ.SUBSTITUTION_QV' vs hdf5:'/PulseData/BaseCalls/SubstitutionQV'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/SubstitutionQV" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_1.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "SUBSTITUTION_QV" "SEQUENCE"
}

compare_SEQ_SUBSTITUTION_TAG()
{
    header "comparing vdb:'SEQ.SUBSTITUTION_TAG' vs hdf5:'/PulseData/BaseCalls/SubstitutionTag'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/SubstitutionTag" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "SUBSTITUTION_TAG" "SEQUENCE"
}

compare_SEQ()
{
    compare_SEQ_SPOT_LEN
    compare_SEQ_HOLE_STATUS
    compare_SEQ_XY
    compare_SEQ_HOLENUMBER
    compare_SEQ_READ
    compare_SEQ_QUALITY
    compare_SEQ_PRE_BASE_FRAMES
    compare_SEQ_WIDTH_IN_FRAMES
    compare_SEQ_PULSE_INDEX_16
    compare_SEQ_PULSE_INDEX_32
    compare_SEQ_INSERTION_QV
    compare_SEQ_DELETION_QV
    compare_SEQ_DELETION_TAG
    compare_SEQ_SUBSTITUTION_QV
    compare_SEQ_SUBSTITUTION_TAG
}

#==============================================================================

compare_CONS_SPOT_LEN()
{
    header "comparing CONS.SPOT_LEN"
    h5dump -d "/PulseData/ConsensusBaseCalls/ZMW/NumEvent" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/4 "%d"' -e '"\n"' -v > x1.spot_len_h5
    diff_and_show x1.spot_len_h5 "SPOT_LEN" "CONSENSUS"
}

compare_CONS_HOLE_STATUS()
{
    header "comparing CONS.HOLE_STATUS"
    h5dump -d "/PulseData/ConsensusBaseCalls/ZMW/HoleStatus" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/1 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "(U8)HOLE_STATUS" "CONSENSUS"
}

compare_CONS_XY()
{
    header "comparing CONS.X/Y"
    h5dump -d "/PulseData/ConsensusBaseCalls/ZMW/HoleXY" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/2 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "X,Y" "CONSENSUS"
}

compare_CONS_HOLENUMBER()
{
    header "comparing CONS.HOLENUMBER"
    h5dump -d "/PulseData/ConsensusBaseCalls/ZMW/HoleNumber" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/4 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "HOLE_NUMBER" "CONSENSUS"
}

compare_CONS_NUM_PASSES()
{
    header "comparing CONS.NUM_PASSES"
    h5dump -d "/PulseData/ConsensusBaseCalls/Passes/NumPasses" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/4 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "NUM_PASSES" "CONSENSUS"
}

compare_CONS_READ()
{
    header "comparing vdb:'CONS.READ' vs hdf5:'/PulseData/ConsensusBaseCalls/Basecall'"
    echo -n "h5dump"
    h5dump -d "/PulseData/ConsensusBaseCalls/Basecall" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "READ" "CONSENSUS"
}

compare_CONS_QUALITY()
{
    header "comparing vdb:'CONS.QUALITY' vs hdf5:'/PulseData/ConsensusBaseCalls/QualityValue'"
    echo -n "h5dump"
    h5dump -d "/PulseData/ConsensusBaseCalls/QualityValue" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_1.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "QUALITY" "CONSENSUS"
}

compare_CONS_INSERTION_QV()
{
    header "comparing vdb:'CONS.INSERTION_QV' vs hdf5:'/PulseData/ConsensusBaseCalls/InsertionQV'"
    echo -n "h5dump"
    h5dump -d "/PulseData/ConsensusBaseCalls/InsertionQV" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_1.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "INSERTION_QV" "CONSENSUS"
}

compare_CONS_DELETION_QV()
{
    header "comparing vdb:'CONS.DELETION_QV' vs hdf5:'/PulseData/ConsensusBaseCalls/DeletionQV'"
    echo -n "h5dump"
    h5dump -d "/PulseData/ConsensusBaseCalls/DeletionQV" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_1.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "DELETION_QV" "CONSENSUS"
}

compare_CONS_DELETION_TAG()
{
    header "comparing vdb:'CONS.DELETION_TAG' vs hdf5:'/PulseData/ConsensusBaseCalls/DeletionTag'"
    echo -n "h5dump"
    h5dump -d "/PulseData/ConsensusBaseCalls/DeletionTag" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "DELETION_TAG" "CONSENSUS"
}

compare_CONS_SUBSTITUTION_QV()
{
    header "comparing vdb:'CONS.SUBSTITUTION_QV' vs hdf5:'/PulseData/ConsensusBaseCalls/SubstitutionQV'"
    echo -n "h5dump"
    h5dump -d "/PulseData/ConsensusBaseCalls/SubstitutionQV" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl_1.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "SUBSTITUTION_QV" "CONSENSUS"
}

compare_CONS_SUBSTITUTION_TAG()
{
    header "comparing vdb:'CONS.SUBSTITUTION_TAG' vs hdf5:'/PulseData/ConsensusBaseCalls/SubstitutionTag'"
    echo -n "h5dump"
    h5dump -d "/PulseData/ConsensusBaseCalls/SubstitutionTag" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    ins_nl.pl x1.bin x1.spot_len_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "SUBSTITUTION_TAG" "CONSENSUS"
}


compare_CONS()
{
    compare_CONS_SPOT_LEN
    compare_CONS_HOLE_STATUS
    compare_CONS_XY
    compare_CONS_HOLENUMBER
    compare_CONS_NUM_PASSES
    compare_CONS_READ
    compare_CONS_QUALITY
    compare_CONS_INSERTION_QV
    compare_CONS_DELETION_QV
    compare_CONS_DELETION_TAG
    compare_CONS_SUBSTITUTION_QV
    compare_CONS_SUBSTITUTION_TAG
}

#==============================================================================

compare_PASS_HIT_BEFORE()
{
    header "comparing vdb:'PASS.ADAPTER_HIT_BEFORE' vs hdf5:'.../Passes/AdapterHitBefore'"
    h5dump -d "/PulseData/ConsensusBaseCalls/Passes/AdapterHitBefore" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/1 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "ADAPTER_HIT_BEFORE" "PASSES"
}

compare_PASS_HIT_AFTER()
{
    header "comparing vdb:'PASS.ADAPTER_HIT_AFTER' vs hdf5:'.../Passes/AdapterHitAfter'"
    h5dump -d "/PulseData/ConsensusBaseCalls/Passes/AdapterHitAfter" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/1 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "ADAPTER_HIT_AFTER" "PASSES"
}

compare_PASS_DIRECTION()
{
    header "comparing vdb:'PASS.PASS_DIRECTION' vs hdf5:'../Passes/PassDirection'"
    h5dump -d "/PulseData/ConsensusBaseCalls/Passes/PassDirection" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/1 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "PASS_DIRECTION" "PASSES"
}

compare_NUM_BASES()
{
    header "comparing vdb:'PASS.NUM_BASES' vs hdf5:'../Passes/PassNumBases'"
    h5dump -d "/PulseData/ConsensusBaseCalls/Passes/PassNumBases" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/4 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "PASS_NUM_BASES" "PASSES"
}

compare_START_BASE()
{
    header "comparing vdb:'PASS.START_BASE' vs hdf5:'../Passes/PassStartBase'"
    h5dump -d "/PulseData/ConsensusBaseCalls/Passes/PassStartBase" -b MEMORY -o x1.bin $H5FILE > /dev/null
    hexdump x1.bin -e '1/4 "%d"' -e '"\n"' -v > x1.dump_h5
    diff_and_show x1.dump_h5 "PASS_START_BASE" "PASSES"
}

compare_PASSES()
{
    compare_PASS_HIT_BEFORE
    compare_PASS_HIT_AFTER
    compare_PASS_DIRECTION
    compare_NUM_BASES
    compare_START_BASE
}

#==============================================================================

compare_METRICS_BASE_FRACTION()
{
    header "comparing vdb:'METRICS.BASE_FRACTION' vs hdf5:'.../ZMWMetrics/BaseFraction'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/BaseFraction" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '"[" 4/4 "%e, " "]\n"' > x2.dump_h5
    sed 's/,]/]/g' x2.dump_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "BASE_FRACTION" "ZMW_METRICS"
}

compare_METRICS_BASE_IPD()
{
    header "comparing vdb:'METRICS.BASE_IPD' vs hdf5:'.../ZMWMetrics/BaseIpd'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/BaseIpd" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
    diff_and_show x1.dump_h5 "BASE_IPD" "ZMW_METRICS"
}

compare_METRICS_BASE_RATE()
{
    header "comparing vdb:'METRICS.BASE_RATE' vs hdf5:'.../ZMWMetrics/BaseRate'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/BaseRate" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
    diff_and_show x1.dump_h5 "BASE_RATE" "ZMW_METRICS"
}

compare_METRICS_BASE_WIDTH()
{
    header "comparing vdb:'METRICS.BASE_WIDTH' vs hdf5:'.../ZMWMetrics/BaseWidth'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/BaseWidth" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
    diff_and_show x1.dump_h5 "BASE_WIDTH" "ZMW_METRICS"
}

compare_METRICS_CHAN_BASE_QV()
{
    header "comparing vdb:'METRICS.CHAN_BASE_QV' vs hdf5:'.../ZMWMetrics/CmBasQv'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/CmBasQv" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '"[" 4/4 "%e, " "]\n"' > x2.dump_h5
    sed 's/,]/]/g' x2.dump_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "CHAN_BASE_QV" "ZMW_METRICS"
}

compare_METRICS_CHAN_DEL_QV()
{
    header "comparing vdb:'METRICS.CHAN_DEL_QV' vs hdf5:'.../ZMWMetrics/CmDelQv'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/CmDelQv" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '"[" 4/4 "%e, " "]\n"' > x2.dump_h5
    sed 's/,]/]/g' x2.dump_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "CHAN_DEL_QV" "ZMW_METRICS"
}

compare_METRICS_CHAN_INS_QV()
{
    header "comparing vdb:'METRICS.CHAN_INS_QV' vs hdf5:'.../ZMWMetrics/CmInsQv'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/CmInsQv" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '"[" 4/4 "%e, " "]\n"' > x2.dump_h5
    sed 's/,]/]/g' x2.dump_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "CHAN_INS_QV" "ZMW_METRICS"
}

compare_METRICS_CHAN_SUB_QV()
{
    header "comparing vdb:'METRICS.CHAN_SUB_QV' vs hdf5:'.../ZMWMetrics/CmSubQv'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/CmSubQv" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '"[" 4/4 "%e, " "]\n"' > x2.dump_h5
    sed 's/,]/]/g' x2.dump_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "CHAN_SUB_QV" "ZMW_METRICS"
}

compare_METRICS_LOCAL_BASE_RATE()
{
    header "comparing vdb:'METRICS.LOCAL_BASE_RATE' vs hdf5:'.../ZMWMetrics/LocalBaseRate'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/LocalBaseRate" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
    diff_and_show x1.dump_h5 "LOCAL_BASE_RATE" "ZMW_METRICS"
}

compare_METRICS_DARK_BASE_RATE()
{
    header "comparing vdb:'METRICS.DARK_BASE_RATE' vs hdf5:'.../ZMWMetrics/DarkBaseRate'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/DarkBaseRate" -b MEMORY -o x1.bin $H5FILE > /dev/null 2>/dev/null
    LASTSTATUS=$?
    if [ $LASTSTATUS -eq 0 ];then
        echo -n " / bin 2 ascii"
        hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
        diff_and_show x1.dump_h5 "DARK_BASE_RATE" "ZMW_METRICS"
    else
       echo " / the hdf5-source has no DARK_BASE_RATE-table!"
    fi
}

compare_METRICS_HQ_RGN_START_TIME()
{
    header "comparing vdb:'METRICS.HQ_RGN_START_TIME' vs hdf5:'.../ZMWMetrics/HQRegionStartTime'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/HQRegionStartTime" -b MEMORY -o x1.bin $H5FILE > /dev/null 2>/dev/null
    LASTSTATUS=$?
    if [ $LASTSTATUS -eq 0 ];then
        echo -n " / bin 2 ascii"
        hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
        diff_and_show x1.dump_h5 "HQ_RGN_START_TIME" "ZMW_METRICS"
    else
       echo " / the hdf5-source has no HQ_RGN_START_TIME-table!"
    fi
}

compare_METRICS_HQ_RGN_END_TIME()
{
    header "comparing vdb:'METRICS.HQ_RGN_END_TIME' vs hdf5:'.../ZMWMetrics/HQRegionEndTime'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/HQRegionEndTime" -b MEMORY -o x1.bin $H5FILE > /dev/null 2>/dev/null
    LASTSTATUS=$?
    if [ $LASTSTATUS -eq 0 ];then
        echo -n " / bin 2 ascii"
        hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
        diff_and_show x1.dump_h5 "HQ_RGN_END_TIME" "ZMW_METRICS"
    else
       echo " / the hdf5-source has no HQ_RGN_END_TIME-table!"
    fi
}

compare_METRICS_HQ_RGN_SNR()
{
    header "comparing vdb:'METRICS.HQ_RGN_SNR' vs hdf5:'.../ZMWMetrics/HQRegionSNR'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/HQRegionSNR" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '"[" 4/4 "%e, " "]\n"' > x2.dump_h5
    sed 's/,]/]/g' x2.dump_h5 > x1.dump_h5
    diff_and_show x1.dump_h5 "HQ_RGN_SNR" "ZMW_METRICS"
}

compare_METRICS_PRODUCTIVITY()
{
    header "comparing vdb:'METRICS.PRODUCTIVITY' vs hdf5:'.../ZMWMetrics/Productivity'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/Productivity" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '1/1 "%d" "\n"' > x1.dump_h5
    diff_and_show x1.dump_h5 "PRODUCTIVITY" "ZMW_METRICS"
}

compare_METRICS_READ_SCORE()
{
    header "comparing vdb:'METRICS.READ_SCORE' vs hdf5:'.../ZMWMetrics/ReadScore'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/ReadScore" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
    diff_and_show x1.dump_h5 "READ_SCORE" "ZMW_METRICS"
}

compare_METRICS_READ_BASE_QV()
{
    header "comparing vdb:'METRICS.READ_BASE_QV' vs hdf5:'.../ZMWMetrics/RmBasQv'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/RmBasQv" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
    diff_and_show x1.dump_h5 "READ_BASE_QV" "ZMW_METRICS"
}

compare_METRICS_READ_DEL_QV()
{
    header "comparing vdb:'METRICS.READ_DEL_QV' vs hdf5:'.../ZMWMetrics/RmDelQv'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/RmDelQv" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
    diff_and_show x1.dump_h5 "READ_DEL_QV" "ZMW_METRICS"
}

compare_METRICS_READ_INS_QV()
{
    header "comparing vdb:'METRICS.READ_INS_QV' vs hdf5:'.../ZMWMetrics/RmInsQv'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/RmInsQv" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
    diff_and_show x1.dump_h5 "READ_INS_QV" "ZMW_METRICS"
}

compare_METRICS_READ_SUB_QV()
{
    header "comparing vdb:'METRICS.READ_SUB_QV' vs hdf5:'.../ZMWMetrics/RmSubQv'"
    echo -n "h5dump"
    h5dump -d "/PulseData/BaseCalls/ZMWMetrics/RmSubQv" -b MEMORY -o x1.bin $H5FILE > /dev/null
    echo -n " / bin 2 ascii"
    hexdump x1.bin -v -e '1/4 "%e" "\n"' > x1.dump_h5
    diff_and_show x1.dump_h5 "READ_SUB_QV" "ZMW_METRICS"
}

compare_METRICS()
{
    compare_METRICS_BASE_FRACTION
    compare_METRICS_BASE_IPD
    compare_METRICS_BASE_RATE
    compare_METRICS_BASE_WIDTH
    compare_METRICS_CHAN_BASE_QV
    compare_METRICS_CHAN_DEL_QV
    compare_METRICS_CHAN_INS_QV
    compare_METRICS_CHAN_SUB_QV
    compare_METRICS_LOCAL_BASE_RATE
    compare_METRICS_DARK_BASE_RATE
    compare_METRICS_HQ_RGN_START_TIME
    compare_METRICS_HQ_RGN_END_TIME
    compare_METRICS_HQ_RGN_SNR
    compare_METRICS_PRODUCTIVITY
    compare_METRICS_READ_SCORE
    compare_METRICS_READ_BASE_QV
    compare_METRICS_READ_DEL_QV
    compare_METRICS_READ_INS_QV
    compare_METRICS_READ_SUB_QV
}

#==============================================================================

H5FILE=`eval "ls $1"`
VDBDIR=`eval "readlink -f $2"`
echo "load the hdf5-file: '$H5FILE'"
echo "into a vdb-dir. at: '$VDBDIR'"
echo "and verify this load"

reload
compare_SEQ
compare_CONS
compare_PASSES
compare_METRICS
