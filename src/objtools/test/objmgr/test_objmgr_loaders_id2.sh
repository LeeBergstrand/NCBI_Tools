#! /bin/sh
#$Id: test_objmgr_loaders_id2.sh 57631 2005-03-14 17:30:44Z vasilche $

method="ID2"

echo "Checking GenBank loader $method:"
GENBANK_LOADER_METHOD="$method"
export GENBANK_LOADER_METHOD
$CHECK_EXEC "$@"
exitcode=$?
if test $exitcode -ne 0; then
    echo "Test of GenBank loader $method failed: $exitcode"
    case $exitcode in
        # signal 1 (HUP), 2 (INTR), 9 (KILL), or 15 (TERM).
        129|130|137|143) echo "Apparently killed"; break ;;
    esac
fi

exit $exitcode
