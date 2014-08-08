#!/bin/bash
rm %$.log > /dev/null 2>&1
mkdir $1 > /dev/null 2>&1
cd $1
rm * > /dev/null 2>&1

sra-dump --start 1 --stop 300 SRR000001 >SRR000001.sra 2>../$1.log

echo sra-dump: `ls -1 | wc -l` "file(s)" created
cd ..
chmod -x $1.log > /dev/null 2>&1
