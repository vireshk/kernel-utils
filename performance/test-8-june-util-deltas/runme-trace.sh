#!/bin/bash

SCHEDUTIL="/sys/devices/system/cpu/cpufreq/schedutil"

pretest="echo 1 > $SCHEDUTIL/reset"
posttest="grep . $SCHEDUTIL/*"

perform_test()
{
	eval $pretest
	printf "Test: $*\n\n"
	eval $*
	printf "\nSched-stats:\n"
	eval $posttest
}

runme()
{
#	perform_test cyclictest -q -t 32 -i 10000 --latency=1000000 -D 5
	perform_test perf bench sched pipe
}

trace()
{
	BUFFER=100000
	FILE=dft-$1
	EVENTS=" -e sched -e power"
	#EVENTS=" -e all"

	trace-cmd start -b $BUFFER $EVENTS

	sleep 5
#	runme > perf-trace-$1.txt

	trace-cmd stop

	cat /sys/kernel/debug/tracing/trace > $FILE.txt
	trace-cmd extract -o $FILE.dat
	echo "Saved trace in: "$FILE
	sync
}

cat /proc/version > version
trace 0
