#! /bin/sh
#$Id: test_objmgr_loaders.sh 128840 2008-05-27 17:46:28Z ucko $

status_dir="../../../../../status"

if test -f "$status_dir/PubSeqOS.enabled"; then
    methods="PUBSEQOS ID1 ID2"
    NCBI_LOAD_PLUGINS_FROM_DLLS=1
    export NCBI_LOAD_PLUGINS_FROM_DLLS
else
    echo Sybase is disabled or unaware of PubSeqOS: skipping PUBSEQOS loader test
    methods="ID1 ID2"
fi

exitcode=0
for method in $methods; do
    echo "Checking GenBank loader $method:"
    GENBANK_LOADER_METHOD="$method"
    export GENBANK_LOADER_METHOD
    $CHECK_EXEC "$@"
    error=$?
    if test $error -ne 0; then
        echo "Test of GenBank loader $method failed: $error"
        exitcode=$error
        case $error in
            # signal 1 (HUP), 2 (INTR), 9 (KILL), or 15 (TERM).
            129|130|137|143) echo "Apparently killed"; break ;;
        esac
    fi
    unset NCBI_LOAD_PLUGINS_FROM_DLLS
done

exit $exitcode
