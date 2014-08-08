if [ $# -ne 3 ]
then
    echo "Usage: $0 <fields> <input> <output>" >&2
    exit 1
fi
if [ ! -e $2 ]
then
    echo "$2: No such file or directory" >&2
    exit 1
fi
cut="/bin/cut $1 $2"
sort="/bin/sort"
echo -n `/bin/date`                > /dev/stderr
echo -e "\t$cut | $sort > $3" > /dev/stderr
set -o pipefail
$cut | $sort > $3
