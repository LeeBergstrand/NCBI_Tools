if [ $# -ne 4 ]
then
    echo "Usage: $0 <bin dir> <sequence> <sorted alignment> <output>" >&2
    exit 1
fi
merge="$1/merge_vdb_dumps.pl -s $2 -a $3"
split="$1/split.pl -f 12 -o $4"
echo -n `date`
echo -e "\t$merge | $split"
set -o pipefail
$merge | $split
