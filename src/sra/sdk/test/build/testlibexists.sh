#!/bin/bash
# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================

#
# Unit Test for libexists.sh
# Use:
#    testlibexists.sh [-v] [-k] [ testId ]
# Parameters:
#    testId : ID of the test case to run (all by default)
# Options:
#    -v : verbose mode
#    -k : keep temporary files
# Returns:
#    0 all test pass
#    >0 number of failed tests. The error messages are sent to stdout
#    -1 bad arguments
#    -2 unsupported OS

#TODO: port to Mac

#globals
BUILD_DIR="../../build/"

verbose="false"

failed=0
passed=0
keepFiles="false"

# initialize
UNAME=($(uname -s))
if [ "Linux" == "$UNAME" ]
then
    OS="linux"
    ARCH="x86_64"
    CC="gcc -c"
    LD="gcc"
    CC_SH="cc.sh"    
elif [ "CYGWIN" == "${UNAME:0:6}" ] 
then
    OS="win"
    ARCH="i386"
    CC="cl -nologo"
    LD="link"
    CC_SH="win-cc.sh"
    # need to make sure test dlls are found
    PATH="./dyn;$PATH"
elif [ "Darwin" == "$UNAME" ]
then
    OS="mac"
    ARCH=$(${BUILD_DIR}mac.arch.sh)
    CC="gcc -c"
    LD="gcc"
    CC_SH="cc.sh"    
else
    echo "OS $UNAME is not supported"
    exit -2        
fi
LD_SH="ld.sh"

# parse parameters
while getopts vk o
do	case "$o" in
	v)	    verbose="true";; 
	k)	    keepFiles="true";; 
	[?])	printf >&2 "Usage: $0 [-v]"
		exit -1;;
	esac
done
shift $((OPTIND - 1))
testId=$*

################################################## test fixture #################################################
# Build libraries for use in tests:
# - libA defines fnA1, fnA2
# - libB defines fnB1, fnB2, depends on fnA1, fnA2
# sources: src/libA.c,libB.c
# object files:
#   Linux: dyn/libA.so,libB.so; Windows: dyn/libA.dll,libA.lib,libA.exp,LibB.dll,libB.lib,cd stalibB/exp
#   sta/libA.a,libB.a

rm -r src sta dyn 2>/dev/null
mkdir src sta dyn 

compile="${BUILD_DIR}/$CC_SH $OS $CC"

### libA:
# static
echo "void fnA1(){} void fnA2(){}"    >src/libA.c
${BUILD_DIR}$CC_SH $OS $CC -c src/libA.c -o sta/libA.o $COPTS >/dev/null
ar rc sta/libA.a sta/libA.o

# dynamic
if [ "win" != "$OS" ]
then
    ${BUILD_DIR}$CC_SH $OS $CC -c src/libA.c -o dyn/libA.o -fPIC $COPTS >/dev/null
    ${BUILD_DIR}/$LD_SH $OS $ARCH $LD --dlib --objx o dyn/libA.o -Wl,-soname,libA.so -o dyn/libA.so >/dev/null
else
    echo "__declspec(dllexport) void fnA1(); void fnA1(){} __declspec(dllexport) void fnA2(); void fnA2(){}"    >dyn/libA.c
     cl /nologo /LD dyn/libA.c /Fedyn/libA.dll /Fodyn/libA.o >/dev/null
fi
###

### libB:
# static
echo "extern void fnA1(); extern void fnA2(); void fnB1(){fnA1();} void fnB2(){fnA2();}"    >src/libB.c
${BUILD_DIR}/$CC_SH $OS $CC -c src/libB.c -o sta/libB.o $COPTS >/dev/null 
ar rc sta/libB.a sta/libB.o

# dynamic
# to test a dependency of one .so on another:
if [ "win" != "$OS" ]
then
    ${BUILD_DIR}/$CC_SH $OS $CC -c src/libB.c -o dyn/libB.o -fPIC $COPTS >/dev/null 
    ${BUILD_DIR}/$LD_SH $OS $ARCH $LD --dlib --objx o dyn/libB.o -Wl,-soname,libB.so -o dyn/libB.so -Ldyn -lA >/dev/null
else
    echo "extern __declspec(dllimport) void fnA1(); extern __declspec(dllimport) void fnA2(); __declspec(dllexport) void fnB1(); void fnB1(){fnA1();} __declspec(dllexport) void fnB2(); void fnB2(){fnA2();}"    >src/libB.c
     cl /nologo /LD src/libB.c /Fedyn/libB.dll /Fodyn/libB.o /link dyn/libA.lib >/dev/null
fi
###

################################################## test helpers #################################################

# assertFail
#   parameters:
#       $1  - ID of the test case
#       $2  - description of the test case
#       $3  - command line invoking libexists.sh
#    globals:
#       $failed - counter of failed tests
#       $passed - counter of passed tests
function assertFail
{
    if [ "$testId" == "" ] || [ "$testId" == "$1" ]
    then
        lib=$(${BUILD_DIR}libexists.sh "$OS" "$ARCH" "$CC" "$LD" $CC_SH ld.sh $3)
        if [ $? == 0 ] && [ "$lib" = "" ]
        then
            echo "Failed test case $1: '$2', lib='$lib'"
            failed=$(($failed+1))
        else
            if [ $verbose == "true" ] 
            then echo "Passed test case $1: '$2'" 
            fi
            passed=$(($passed+1))
        fi
    fi     
}
# assertPass
#    parameters:
#       $1  - ID of the test case
#       $2  - description of the test case
#       $3  - command line invoking libexists.sh
#       $4  - expected lib name
#    globals:
#       $failed - counter of failed tests
#       $passed - counter of passed tests
function assertPass
{
    if [ "$testId" == "" ] || [ "$testId" == "$1" ]
    then
        lib=$(${BUILD_DIR}libexists.sh "$OS" "$ARCH" "$CC" "$LD" $CC_SH ld.sh $3)
        if [ $? != 0 ] || [ "$lib" != "$4" ]
        then
            echo "Failed test case $1: '$2', lib='$lib', expected='$4'";failed=$(($failed+1))
        else
            if [ $verbose == "true" ] 
            then echo "Passed test case $1: '$2'" 
            fi
            passed=$(($passed+1))
        fi
    fi        
}

################################################## test cases #################################################

## test cases that should pass
# static linking
#assertPass "S0" "one -s library, one function, match"                                                       "-s z        compress2"       z
assertPass "S1" "one -s library, one function, match"                                                       "-Lsta      -s A        fnA1"       A
assertPass "S2" "many libraries (-L and -X directories, -l, -s libs), one function, match in the firstlib"  "-Xsta -L.. -lA -sB     fnA1"       A
assertPass "S3" "many libraries (-L and -X), one function, match in the last lib"                           "-Lsta -X.. -lB -sA     fnA1"       A
assertPass "S4" "one library, many functions, full match"                                                   "-Lsta      -l A        fnA1 fnA2"  A
assertPass "S5" "many libraries, many functions, full match in the last lib"                                "-Lsta      -l B -l A   fnA1 fnA2"  A

# dynamic linking
assertPass "D1" "one -d library, one function, match"                                                       "-Ldyn      -dA         fnA1"       A
assertPass "D2" "one -d library with a dependency on another .so, one function, match"                      "-Ldyn      -dB         fnB1"       B
assertPass "D3" "many libraries (-L and -X directories, -d libs), one function, match in the firstlib"      "-Xdyn -L.. -dA -dB     fnA2"       A
assertPass "D4" "many libraries (-L and -X), one function, match in the last lib"                           "-Ldyn -X.. -dB -dA     fnB1"       B
assertPass "D5" "one library, many functions, full match"                                                   "-Ldyn      -lA         fnA1 fnA2"  A
assertPass "D6" "many libraries, many functions, full match in the last lib"                                "-Ldyn      -lA -lB     fnB1 fnB2"  B

## error test cases that should fail
assertFail "E1" "no arguments"                                                       ""
assertFail "E2" "no function names"                                                  "-Lqq"
assertFail "E3" "function name, no libraries"                                        "              fnA1"
assertFail "E4" "one library, one function, no match"                               "-Lsta -lA     fnB1"
assertFail "E5" "many libraries, one function, no match"                            "-Lsta -lA -lB x1"
assertFail "E6" "no libraries, many functions"                                      "              fnA1 fnB1"
assertFail "E7" "one library, many functions, partial match"                        "-Lsta -lA     fnA1 fnB1"
assertFail "E8" "many libraries, many functions, partial matches but no full match" "-Lsta -lA -lB fnA1 fnB1"

echo $passed tests passed
echo $failed tests failed

# clean up
if [ $keepFiles == "false" ]
then rm -r src sta dyn
fi    

exit $failed
