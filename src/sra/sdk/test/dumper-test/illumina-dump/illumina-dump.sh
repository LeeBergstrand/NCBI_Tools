#!/bin/bash
rm %$.log > /dev/null 2>&1
mkdir $1 > /dev/null 2>&1
cd $1
rm * > /dev/null 2>&1

illumina-dump -N 1 -X 300 DRR000001 > ../$1.log 2>&1

echo illumina-dump: `ls -1 | wc -l` "file(s)" created
cd ..
chmod -x $1.log > /dev/null 2>&1
