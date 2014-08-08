#! /bin/sh
#
# $Id: test_fstream_pushback.sh 364070 2012-05-22 17:56:54Z lavr $
#
exit_code=0
log=test_fstream_pushback.log
rm -f $log

trap 'rm -f $log; echo "`date`."' 0 1 2 15

i=0
j=`expr $$ % 2 + 1`
while [ $i -lt $j ]; do
  echo "${i} of ${j}: `date`"
  $CHECK_EXEC test_fstream_pushback >$log 2>&1
  exit_code=$?
  if [ "$exit_code" != "0" ]; then
    if [ "`head -301 $log | wc -l`" -gt "300" ]; then
      head -100 "$log"
      echo '...'
      tail -200 "$log"
    else
      cat "$log"
    fi
    break
  fi
  i="`expr $i + 1`"
done
exit $exit_code
