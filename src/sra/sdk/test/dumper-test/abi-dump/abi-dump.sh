#!/bin/bash
rm $1.log > /dev/null 2>&1
mkdir $1 > /dev/null 2>&1
cd $1
rm * > /dev/null 2>&1

if [ "$1" = "win" ]; then
APATH="//panfs/traces01/sra0/SRR/000000"
else
APATH="/panfs/traces01/sra0/SRR/000000"
fi

abi-dump -path "$APATH" -minSpotId 1 -maxSpotId 300 SRR000001 > ../$1.log 2>&1

echo abi-dump: `ls -1 | wc -l` "file(s)" created
cd ..
chmod -x $1.log > /dev/null 2>&1
