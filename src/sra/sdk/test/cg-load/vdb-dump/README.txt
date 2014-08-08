A "quick" test of cg-load:

BIN=/path/to/asm-trace/bin/directory/with/cg-load/and/vdb-dump

WORK=/path/to/test/work/directory
WORK is not cleaned after the test.

RUN=/path/to/output/run
It is not removed after the test.

SRC=/panfs/pan1/sra-test/golikov/loader/cg/YRI_trio.tiny

###########################################
v1 input sets are in /panfs/pan1/sra-test/golikov/loader/cg
v2     :             /panfs/pan1/sra-test/LOADER/cg/*v2
   and               /panfs/pan1/sra-test/LOADER/cg/v.2
                     /panfs/pan1/sra-test/LOADER/cg/v.2/half-lane : 6 reads + 6 mapping files (no evidence)
                     /panfs/pan1/sra-test/LOADER/cg/v.2/half-lane : 6 reads + 6 mapping files (no evidence)
                     /panfs/pan1/sra-test/LOADER/cg/v.2/1-lane    : 1 lane, no evidence
                     /panfs/pan1/sra-test/LOADER/cg/V2-37.NA12878_native_only1.eugene : 1 lane with evidence
                     

################# CG TEST #################

asm-trace/test/cg-load/vdb-dump/test.pl --bin $BIN --src $SRC --work $WORK --sra $RUN --clean [ --evidence ]

1) load and compare SEQUENCE and ALIGNMENTs; load EVIDENCE
2) add "--evidence" option to check loaded evidence files

################# TEST FILES #################

################# V 1.5 #################

SRC=/panfs/pan1/sra-test/golikov/loader/cg/YRI_trio.tiny
# minimal quick test:
# reads  : 0.5M lanes
# mapping: 0.9M lanes
# evidence: 18 records in all files

SRC=/panfs/pan1/sra-test/golikov/loader/cg/YRI_trio.tiny.1M
# a bigger set:
# reads   : 1M lanes
# mapping : 1.4M lanes
# evidence: 13K lanes
Done in         42:08
  39:20 Dumping VDB evidence

SRC=/panfs/pan1/sra-test/golikov/loader/cg/YRI_trio.10M
# reads   : 10M lanes
# mapping : 12M lanes
# evidence: 100K lanes
# evidence dump takes 3h25m
# test without evidence takes ~25'

################# V 2.0 #################

SRC=/panfs/pan1/sra-test/LOADER/cg/V2-37.YRI_trio.tiny
# minimal quick test v.2.0:
# reads  : 3M lanes
# mapping: 7.5M lanes
# evidence:19 records in all files

SRC=/panfs/pan1/sra-test/LOADER/cg/1000.v2
# reads  : 3M lanes
# mapping: 7.5M lanes
# evidence:19 records in all files
Done in         10:56
   5:33 Dumping ALIGNMENT-s

SRC=/panfs/pan1/sra-test/LOADER/cg/2000.v2
# reads  : 3M lanes
# mapping: 7.5M lanes
# evidence:29 records in all files
Done in         10:53
   5:28 Dumping ALIGNMENT-s

2500
# reads  : 2M lanes
# mapping: 5M lanes
# evidence:32 records in all files
Done in          7:30
   3:23 Dumping ALIGNMENT-s

5000
# reads  : 2M lanes
# mapping: 5M lanes
# evidence:69 records in all files
Done in          7:50
   3:34 Dumping ALIGNMENT-s

10000 # contains evidence with 3 reads
# reads  : 2M lanes
# mapping: 5M lanes
# evidence:134 records in all files
Done in          7:26
   3:28 Dumping ALIGNMENT-s

20000
# reads  : 2M lanes
# mapping: 5M lanes
# evidence:279 records in all files
Done in          7:42
   3:36 Dumping ALIGNMENT-s

25000
# reads  : 2M lanes
# mapping: 5M lanes
# evidence:386 records in all files
Done in          7:24
   3:26 Dumping ALIGNMENT-s

50000
# reads  : 2M lanes
# mapping: 5M lanes
# evidence:732 records in all files
Done in          8:03
   3:36 Dumping ALIGNMENT-s

100000
# reads  : 2M lanes
# mapping: 5M lanes
# evidence:1470 records in all files
Done in          9:17
   3:32 Dumping ALIGNMENT-s

200000
# reads  : 2M lanes
# mapping: 5M lanes
# evidence:3080 records in all files

250000
# reads  : 3M lanes
# mapping: 7.5M lanes
# evidence:3922 records in all files
Done in         18:36
   7:49 Dumping VDB evidence
   5:52 Dumping ALIGNMENT-s

SRC=/panfs/pan1/sra-test/LOADER/cg/500000.v2
# reads  : 3M lanes
# mapping: 7.5M lanes
# evidence:7001 records in all files
Done in         18:44
  13:09 Dumping VDB evidence

1000000
# reads  : 3M lanes
# mapping: 7.5M lanes
# evidence:15083 records in all files
Done in         30:30
Load took        2:51
  27:13 Dumping VDB evidence

2500000
# reads  : 2.5M lanes
# mapping: 6M lanes
# evidence:50K records in all files
Done in         1:30:09
Load took        2:13
1:27:56 Dumping VDB evidence

50000000
# reads  : 5M lanes
# mapping: 1.2M lanes
# evidence:100K records in all files
Done in         3:11:24
Load took        3:35
3:07:45 Dumping VDB evidence

100000000
# reads  : 30M lanes
# mapping: 70M lanes
# evidence:600K records in all files
Done in         19:37:28
Load took       14:38
19:22:40        Dumping VDB evidence

250000000=250M
Done in         1:42:45 (no evidencediff run)
  56:00 Dumping ALIGNMENT-s
  20:07 Merging SEQUENCE and ALIGNMENT-s
  17:15 Running cg-load

500000000=500M
Done in         1:54:14 (no evidence diff run)
  52:36 Dumping ALIGNMENT-s
  23:49 Merging SEQUENCE and ALIGNMENT-s
  17:41 Running cg-load

1000000000=1G
Done in         1:47:41 (no evidence diff run)
  53:18 Dumping ALIGNMENT-s
  23:01 Running cg-load

2500000000=2.5G
Done in         1:46:21 (no evidence diff run)
  55:27 Dumping ALIGNMENT-s
  20:51 Merging SEQUENCE and ALIGNMENT-s
  17:50 Running cg-load

5000000000=5G
Done in         1:41:29
  53:17 Dumping ALIGNMENT-s
  20:10 Merging SEQUENCE and ALIGNMENT-s
  16:45 Running cg-load

10000000000=10G
Done in         1:35:15
  51:20 Dumping ALIGNMENT-s
  19:24 Merging SEQUENCE and ALIGNMENT-s
  15:49 Running cg-load

25000000000=25G
Done in         1:39:57
Load took       16:49
  53:35 Dumping ALIGNMENT-s
  20:04 Merging SEQUENCE and ALIGNMENT-s
  16:49 Running cg-load

SRC=/panfs/pan1/sra-test/LOADER/cg/v.2/half-lane/MAP
test.pl -b $BIN -d $SRC -w $WORK -sra $RUN -c && echo +
Done in         12:01:35
Load took       1:41:14
5:27:20 Dumping ALIGNMENT-s
2:23:02 Merging SEQUENCE and ALIGNMENT-s
1:41:14 Running cg-load

/panfs/pan1/sra-test/LOADER/cg/v.2/1-lane/MAP
Done in         25:50:10
10:34:00        Dumping ALIGNMENT-s
4:54:05 Cutting and sorting dump file
4:44:52 Merging SEQUENCE and ALIGNMENT-s
3:07:00 Running cg-load

################# V 2.2 #################

SRC=/panfs/pan1/sra-test/LOADER/cg/V2.2-1
Done in          3:15
VDB dump&merge:  1:39

SRC=/panfs/pan1/sra-test/LOADER/cg/V2.2-10K
Done in         30:41
  29:05 Dumping VDB evidence
