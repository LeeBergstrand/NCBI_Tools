if [ $# -lt 3 ]
then
echo "Usage: $0 <script bin dir> <input dir> <output> [ <vdb-dump bin dir> ]" >&2
   #                1                2           3        4
   exit 1
fi

merge="$1/merge_n_check_cg_files.pl -d $2 -s SKIP"
if [ $# -eq 4 ]
then
    merge="$merge -b $4"
fi

split="$1/split.pl -f 5 -o $3"

echo -n `/bin/date`
echo -e "\t$merge | $split"

set -o pipefail
$merge | $split
