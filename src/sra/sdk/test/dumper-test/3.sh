#!/bin/bash

execute()
{
    echo $1
    eval $1
}

# comparing the output of the tip of development
# driven by a schedule file on LINUX

SCHEDULE="schedule.txt"

execute "./dumpers.pl $SCHEDULE"
