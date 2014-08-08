MY_SRA_PATH=`srapath SRR000001`
echo MY_SRA_PATH = $MY_SRA_PATH
MY_SED_PATH=$(sed 's/a/x/' <<< "$MY_SRA_PATH")
