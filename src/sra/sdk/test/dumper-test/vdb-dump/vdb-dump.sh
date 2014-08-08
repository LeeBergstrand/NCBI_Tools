#!/bin/bash
rm %$.log > /dev/null 2>&1
mkdir $1 > /dev/null 2>&1
cd $1
rm * > /dev/null 2>&1

vdb-dump SRR000001 -R1-300 > SRR000001.vdb 2>../$1.log

echo vdb-dump: `ls -1 | wc -l` "file(s)" created
cd ..
chmod -x $1.log > /dev/null 2>&1
