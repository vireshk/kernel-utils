#!/bin/bash

# Locals
BUFFER=4000
ISOL_CPU=${1-1}
FILE=${2-dft}
TIME=${3-20}
DUMMY=0

# What to trace ??
# EVENTS=" -e all"
#EVENTS=" -e all -v -e syscalls"
#EVENTS=" -e block -e net -e printk -e random -e rcu -e scsi -e signal -e task -e writeback -e vmscan -e sched -e power -e timer -e workqueue -e irq"
#EVENTS=" -e sched -e power -e timer -e workqueue -e irq -e syscalls"
#EVENTS=" -e sys_enter -e sys_exit"
#EVENTS=" -e sched -e power -e timer -e workqueue -e irq -l syscall_trace_enter -l syscall_trace_exit"

EVENTS=" -e sched -e power -e timer -e workqueue -e irq"

# Usage: ./is-cpu-isolated.sh <CPU to isolate (default 1)> <number of samples to take (default 1)> <Min Isolation Time Expected in seconds (default 10)> <function to call: 1: isolate, 2: get isolation time, 3: clean>
# isolate CPU
./is-cpu-isolated.sh $ISOL_CPU $DUMMY $DUMMY 1

echo "Sleeping for $TIME seconds"

/usr/bin/trace-cmd start -b $BUFFER $EVENTS

# Get isolation time
./is-cpu-isolated.sh $ISOL_CPU 2 3 2

/usr/bin/trace-cmd stop

cat /sys/kernel/debug/tracing/trace > $FILE.txt
/usr/bin/trace-cmd extract -o $FILE.dat

# clear cpusets
./is-cpu-isolated.sh $ISOL_CPU $DUMMY $DUMMY 3

echo "Saved trace in: "$FILE
sync
