#!/bin/sh
#
# $Id: test_netcache_api.sh 356464 2012-03-14 15:03:03Z ivanovp $
#

echo "--------- Without mirroring ---------"

cat >test_netcache_api.ini <<EOF
[netcache_api]
enable_mirroring = false
EOF

$CHECK_EXEC test_netcache_api -repeat 1 NC_UnitTest
status=$?
if [ $status -ne 0 ]; then
    exit $status
fi


echo
echo "--------- With mirroring ---------"

cat >test_netcache_api.ini <<EOF
[netcache_api]
enable_mirroring = true
EOF

$CHECK_EXEC test_netcache_api -repeat 1 NC_UnitTest
exit $?
