#!/bin/bash
echo -------------
echo calling $1
cd $1
./$1.sh $2
cd ..
