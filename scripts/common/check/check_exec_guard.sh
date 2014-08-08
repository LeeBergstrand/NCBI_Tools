#! /bin/sh

# $Id: check_exec_guard.sh 279760 2011-05-02 18:55:38Z ucko $
# Author:  Vladimir Ivanov, NCBI 
#
###########################################################################
#
# Auxiliary time-guard script for run check command script
#
# Usage:
#    check_exec_guard.sh <timeout> <pid>
#
#    timeout - maximal time of execution process with "pid" in seconds
# 
# Note:
#    If process with "pid" still execute after "timeout" seconds, that 
#    it will be killed and exit code from this script will be 1. 
#    Otherwise exit code 0 will be returned to parent shell.
#
###########################################################################


# Parameters
timeout=$1
pid=$2
sleep_time=5

# Wait 
while [ $timeout -gt 0 ]; do
  kill -0 $pid > /dev/null 2>&1  ||  exit 0
  if [ $timeout -lt  $sleep_time ]; then
    sleep_time=$timeout
  fi
  timeout="`expr $timeout - $sleep_time`"
  sleep $sleep_time >/dev/null 2>&1
done

# Time out, kill the process
echo
echo "Maximum execution time of $1 seconds is exceeded"
echo

kill $pid > /dev/null 2>&1
sleep $sleep_time >/dev/null 2>&1
kill -9 $pid > /dev/null 2>&1

exit 1
