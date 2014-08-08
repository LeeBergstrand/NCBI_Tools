#! /bin/sh
# $Id: test_ncbi_namedpipe.sh 368928 2012-07-13 16:07:48Z lavr $

outlog()
{
  logfile="$1"
  if [ -s "$logfile" ]; then
    echo "=== $logfile ==="
    if [ "`head -201 $logfile 2>/dev/null | wc -l`" -gt "200" ]; then
      head -100 "$logfile"
      echo '...'
      tail -100 "$logfile"
    else
      cat "$logfile"
    fi
  fi
}

timeout=10
exit_code=0
server_log=test_ncbi_namedpipe_server.log
client_log=test_ncbi_namedpipe_client.log

rm -f $server_log $client_log

$CHECK_EXE test_ncbi_namedpipe -suffix $$ server </dev/null >$server_log 2>&1 &
spid=$!
trap 'kill -9 $spid 2>/dev/null; rm -f ./.ncbi_test_pipename_$$; echo "`date`."' 0 1 2 3 15

t=0
while true; do
  if [ -s $server_log ]; then
    sleep 2
    $CHECK_EXEC test_ncbi_namedpipe -suffix $$ client >$client_log 2>&1  ||  exit_code=1
    break
  fi
  t="`expr $t + 1`"
  if [ $t -gt $timeout ]; then
    echo "`date` FATAL:  Timed out waiting on server to start." >$client_log
    exit_code=1
    break
  fi
  sleep 1
done

( kill    $spid ) >/dev/null 2>&1  ||  exit_code=2
( kill -9 $spid ) >/dev/null 2>&1
wait $spid 2>/dev/null

if [ $exit_code != 0 ]; then
  outlog "$server_log"
  outlog "$client_log"
fi

exit $exit_code
