#!/bin/bash
echo -------------
echo comparing $1
cd $1
diff --brief -N -s -w linux win
cd ..
