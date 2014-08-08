#! /bin/bash

tmpdir=tf.$$
echo working in $tmpdir
rm -fr $tmpdir

echo mkdir -p $tmpdir
mkdir -p $tmpdir
echo mkdir -p $tmpdir/s
mkdir -p $tmpdir/s
echo mkdir -p $tmpdir/d
mkdir -p $tmpdir/d

echo create empty file $tmpdir/s/emptyfile
touch $tmpdir/s/emptyfile

echo create file1
cp $0 $tmpdir/s/file1
echo create file2
cat $tmpdir/s/file1 $tmpdir/s/file1 > $tmpdir/s/file2

echo ==========
echo copycat empty file to directory
echo running copycat $tmpdir/s/emptyfile $tmpdir/d/
copycat $tmpdir/s/emptyfile $tmpdir/d/

echo ==========
echo copycat empty file to /dev/null
echo running copycat $tmpdir/s/emptyfile /dev/null
copycat $tmpdir/s/emptyfile /dev/null

echo ==========
echo copycat empty file to file
echo running copycat $tmpdir/s/emptyfile $tmpdir/d/emptyfile1
copycat $tmpdir/s/emptyfile $tmpdir/d/emptyfile1

echo ==========
echo copycat file to directory without ending '/'
echo running copycat $tmpdir/s/file1 $tmpdir/d
copycat $tmpdir/s/file1 $tmpdir/d

echo ==========
echo copycat file to directory with ending '/'
echo running copycat -f $tmpdir/s/file1 $tmpdir/d/
copycat -f $tmpdir/s/file1 $tmpdir/d/

echo ==========
echo copycat file to directory with ending '.'
echo running copycat -f $tmpdir/s/file1 $tmpdir/d/.
copycat -f $tmpdir/s/file1 $tmpdir/d/.

echo ==========
echo copycat file to /dev/null
echo running copycat -f $tmpdir/s/file1 /dev/null
copycat -f $tmpdir/s/file1 /dev/null

echo ==========
echo copycat files to dir
echo copycat -f $tmpdir/s/file1 $tmpdir/s/file2 $tmpdir/d
copycat -f $tmpdir/s/file1 $tmpdir/s/file2 $tmpdir/d

echo ==========
echo copycat files to /dev/null
echo copycat -f $tmpdir/s/file1 $tmpdir/s/file2 /dev/null
copycat -f $tmpdir/s/file1 $tmpdir/s/file2 /dev/null





