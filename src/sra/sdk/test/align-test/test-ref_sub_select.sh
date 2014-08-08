#!/bin/sh
# $Id: test-ref_sub_select.sh 14717 2013-03-08 15:25:05Z ucko $
trap 'rm -rf RefTableSubSelectTest-*' 0 1 2 15
$CHECK_EXEC test-ref_sub_select -o .
