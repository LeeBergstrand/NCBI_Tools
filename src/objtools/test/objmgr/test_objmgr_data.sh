#! /bin/sh
#$Id: test_objmgr_data.sh 70404 2005-10-05 18:14:26Z vasilche $

NCBI_ABORT_ON_NULL=1
export NCBI_ABORT_ON_NULL

for args in "" "-fromgi 30240900 -togi 30241000"; do
    echo "Testing: test_objmgr_data $args"
    if time $CHECK_EXEC test_objmgr_data $args; then
        echo "Done."
    else
        exit 1
    fi
done
exit 0
