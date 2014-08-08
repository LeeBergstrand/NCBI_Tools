#! /bin/sh
# $Id: datatool.sh 385192 2013-01-07 15:56:02Z gouriano $
#

if test "$OSTYPE" = "cygwin"; then
  bases="./testdata //snowman/toolkit_test_data/objects/datatool"
else
  bases="./testdata /net/snowman/vol/projects/toolkit_test_data/objects/datatool"
fi
if test -z "$CHECK_EXEC"; then
  tool="./datatool"
else
  tool="datatool"
fi


do_test() {
    eval args=\""$1"\"
    shift
    file="$1"
    shift
    echo "$tool" -m "$asn" $args out "$@"
    cmd=`echo "$tool" -m "$asn" $args out "$@"`
    $CHECK_EXEC $cmd
    if test "$?" != 0; then
        echo "datatool failed!"
        exit 1
    fi
    diff -w out "$r/$file"
    if test "$?" != 0; then
        echo "wrong result!"
        exit 1
    fi
    rm out
}

for base in $bases; do
    if test ! -d $base; then
        echo "Test data dir not found: $base"
        continue
    fi
    d="$base/data"
    r="$base/res"
    asn="$base/all.asn"

    for i in "-t Seq-entry -d $d/set.bin" "-v $d/set.ent" "-vx $d/set.xml"; do
        do_test "$i -e" set.bin "$@"
        do_test "$i -p" set.ent "$@"
        do_test "$i -px" set.xml "$@"
    done
    echo "Done!"
done

