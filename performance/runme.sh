#!/bin/bash

SCHEDUTIL="/sys/devices/system/cpu/cpufreq/schedutil"
COUNT=1

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
	while read line
	do
		for run in `seq 1 $COUNT`
		do
			perform_test $line
			echo
		done
		echo
	done < list.txt
}

echo 0 > /sys/devices/system/cpu/cpufreq/schedutil/tick
runme > perf-results-vanila3.txt

echo 1 > /sys/devices/system/cpu/cpufreq/schedutil/tick
runme > perf-results-tick3.txt
