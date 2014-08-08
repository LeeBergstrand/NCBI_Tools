#! /bin/sh
# $Id: test_ncbiexec.sh 32490 2004-01-05 19:17:40Z ivanov $

$CHECK_EXEC test_ncbiexec
exit_code=$?
echo "exit code = $exit_code"

os=`uname -s`

if test -n "`echo $os | grep CYGWIN`" ; then
   test $exit_code -eq 0  -o  $exit_code -eq 99  ||  exit 1
else
   test $exit_code -eq 99  ||  exit 1
fi

exit 0
