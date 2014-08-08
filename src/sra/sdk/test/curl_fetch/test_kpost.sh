#!/bin/bash

execute()
{
    echo $1
    eval $1
}

DELIM="--------------------------------------------------------------------------------------------------"

THE_URL="http://sponomar.ncbi.nlm.nih.gov/Traces/names/names.cgi"

VER_KEY="version"
VER_VAL="1.0"
VER_REQ="$VER_KEY=$VER_VAL"

ACC_KEY="acc"
ACC_VAL="SRR000007"
ACC_REQ="$ACC_KEY=$ACC_VAL"

echo "$DELIM"
execute "kpost -u $THE_URL -f \"$VER_REQ\" -f \"$ACC_REQ\" -o"
echo "$DELIM"
execute "kpost -u $THE_URL -n $VER_KEY -l $VER_VAL -n $ACC_KEY -l $ACC_VAL -o"
echo "$DELIM"
