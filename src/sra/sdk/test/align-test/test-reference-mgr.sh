#!/bin/sh
# $Id: test-reference-mgr.sh 14717 2013-03-08 15:25:05Z ucko $
trap 'rm -rf ReferenceMgrTest-*' 0 1 2 15
$CHECK_EXEC test-reference-mgr -o .
