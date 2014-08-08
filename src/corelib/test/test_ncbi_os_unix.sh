#! /bin/sh
# $Id: test_ncbi_os_unix.sh 257300 2011-03-11 15:00:22Z lavr $

rm -f  ./test_ncbi_os_unix.log
$CHECK_EXEC test_ncbi_os_unix
test $? = 0  ||  exit 1
sleep 5
test -f ./test_ncbi_os_unix.log  ||  exit 1
line="`cat ./test_ncbi_os_unix.log 2>/dev/null`"
echo "$line"
test `echo "$line" | grep -s -c "COMPLETED SUCCESSFULLY"` != 1  &&  exit 1
exit 0
