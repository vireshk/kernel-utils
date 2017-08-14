#!/bin/bash

SCHEDUTIL="/sys/devices/system/cpu/cpufreq/schedutil"

trace()
{
	BUFFER=100000
	FILE=dft-$1
	EVENTS=" -e sched -e power"
	#EVENTS=" -e all"

	trace-cmd start -b $BUFFER $EVENTS

	cyclictest -q -t 32 -i 10000 --latency=1000000 -D 20

	trace-cmd stop

	cat /sys/kernel/debug/tracing/trace > $FILE.txt
	trace-cmd extract -o $FILE.dat
	echo "Saved trace in: "$FILE
	sync
}

trace 0
