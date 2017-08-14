#!/bin/bash

SCHEDUTIL="/sys/devices/system/cpu/cpufreq/schedutil"

trace()
{
	BUFFER=100000
	FILE=dft-$1
	EVENTS=" -e sched -e power"
	#EVENTS=" -e all"

	trace-cmd start -b $BUFFER $EVENTS

	sleep 2

	trace-cmd stop

	cat /sys/kernel/debug/tracing/trace > $FILE.txt
	trace-cmd extract -o $FILE.dat
	echo "Saved trace in: "$FILE
	sync
}

echo 0 > /sys/devices/system/cpu/cpufreq/schedutil/tick
trace 0
echo 1 > /sys/devices/system/cpu/cpufreq/schedutil/tick
trace 1
