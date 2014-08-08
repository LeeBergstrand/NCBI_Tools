#! /bin/sh
# $Id: cgitest.sh 32579 2004-01-07 15:03:32Z ivanov $

echo "test" | $CHECK_EXEC_STDIN cgitest
exit $?
