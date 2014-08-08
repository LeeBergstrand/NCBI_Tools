#!/usr/bin/bash
#
# This is an example script for running test-math
#
# You just need to use the vschema from this directory with the -s, 
# a directory to build the VDB table in such as tbl, and a path
# or several paths to float data column[s]
# this script runs using the data columns that happened to be there
# and runs each of the four transforms (round, trunc, ceil and floor
# on each of the three
#
echo round test 1
test-math -f -s test-math.vschema -o tbl -R -k ../../../kdb/SRR06899/col/SIGNAL
echo round test 2
test-math -f -s test-math.vschema -o tbl -R -k ../../../kdb/SRR06900/col/SIGNAL
echo round test 3
test-math -f -s test-math.vschema -o tbl -R -k ../../../kdb/SRR006901/col/SIGNAL

echo trunc test 1
test-math -f -s test-math.vschema -o tbl -T -k ../../../kdb/SRR06899/col/SIGNAL
echo trunc test 2
test-math -f -s test-math.vschema -o tbl -T -k ../../../kdb/SRR06900/col/SIGNAL
echo trunc test 3
test-math -f -s test-math.vschema -o tbl -T -k ../../../kdb/SRR006901/col/SIGNAL

echo ceil test 1
test-math -f -s test-math.vschema -o tbl -C -k ../../../kdb/SRR06899/col/SIGNAL
echo ceil test 2
test-math -f -s test-math.vschema -o tbl -C -k ../../../kdb/SRR06900/col/SIGNAL
echo ceil test 3
test-math -f -s test-math.vschema -o tbl -C -k ../../../kdb/SRR006901/col/SIGNAL

echo floor test 1
test-math -f -s test-math.vschema -o tbl -F -k ../../../kdb/SRR06899/col/SIGNAL
echo floor test 2
test-math -f -s test-math.vschema -o tbl -F -k ../../../kdb/SRR06900/col/SIGNAL
echo floor test 3
test-math -f -s test-math.vschema -o tbl -F -k ../../../kdb/SRR006901/col/SIGNAL

rm -fr tbl