#! /bin/sh
#$Id: test_objmgr_data_ids.sh 70403 2005-10-05 18:14:15Z vasilche $

OBJMGR_SCOPE_AUTORELEASE_SIZE=0
export OBJMGR_SCOPE_AUTORELEASE_SIZE
OBJMGR_BLOB_CACHE=0
export OBJMGR_BLOB_CACHE
GENBANK_ID2_DEBUG=5
export GENBANK_ID2_DEBUG

for mode in "" "-no_reset" "-keep_handles" "-no_reset -keep_handles"; do
    for file in test_objmgr_data.id*; do
        echo "Testing: $@ $mode -idlist $file"
        if time $CHECK_EXEC "$@" $mode -idlist "$file"; then
            echo "Done."
        else
            exit 1
        fi
    done
done

exit 0
