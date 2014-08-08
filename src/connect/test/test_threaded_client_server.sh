#! /bin/sh
# $Id: test_threaded_client_server.sh 368928 2012-07-13 16:07:48Z lavr $

status=0
port="565`expr $$ % 100`"

$CHECK_EXEC test_threaded_server -port $port </dev/null &
server_pid=$!
trap 'kill $server_pid' 0 1 2 3 15

sleep 2
$CHECK_EXEC test_threaded_client -port $port -requests 34 || status=1

exit $status
