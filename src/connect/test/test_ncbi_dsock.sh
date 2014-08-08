#! /bin/sh
# $Id: test_ncbi_dsock.sh 368928 2012-07-13 16:07:48Z lavr $

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
port=test_ncbi_dsock.$$
server_log=test_ncbi_dsock_server.log
client_log=test_ncbi_dsock_client.log

rm -f $port $server_log $client_log

CONN_DEBUG_PRINTOUT=SOME;  export CONN_DEBUG_PRINTOUT

if [ -x /sbin/ifconfig ]; then
  if [ "`arch`" = "x86_64" ]; then
    mtu="`/sbin/ifconfig lo 2>&1 | grep 'MTU:' | sed 's/^.*MTU:\([0-9][0-9]*\).*$/\1/'`"
  fi
fi

$CHECK_EXEC test_ncbi_dsock server $port $mtu </dev/null >$server_log 2>&1 &
spid=$!
trap 'kill -9 $spid 2>/dev/null; rm -f $port; echo "`date`."' 0 1 2 3 15

t=0
while true; do
  if [ -s "$port" ]; then
    sleep 1
    $CHECK_EXEC test_ncbi_dsock client "`cat $port`" $mtu >$client_log 2>&1  ||  exit_code=1
    break
  fi
  t="`expr $t + 1`"
  if [ $t -gt $timeout ]; then
    echo "`date` FATAL: Timed out waiting on server to start." >$client_log
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
