#!/system/bin/sh

path=/data/pelt_walt.txt

reset_cpufreq_stats()
{
	for policy in /sys/devices/system/cpu/cpufreq/policy*;
	do
		echo 1 > $policy/stats/reset
	done
}

get_cpufreq_stats()
{
	echo "cpufreq-stats\n" >> $path
	grep . /sys/devices/system/cpu/cpufreq/schedutil/* >> $path
	echo "" >> $path

	for policy in /sys/devices/system/cpu/cpufreq/policy*;
	do
		grep . $policy/* >> $path
		echo "" >> $path
		grep . $policy/stats/* >> $path
		echo "" >> $path
	done
}

reset_thermal_stats()
{
	for device in /sys/kernel/debug/thermal/cooling*;
	do
		echo 1 > $device/reset
	done
}

get_thermal_stats()
{
	echo "thermal-stats\n" >> $path

	for device in /sys/class/thermal/cooling*;
	do
		grep . $device/* >> $path
		echo "" >> $path
	done

	for device in /sys/kernel/debug/thermal/cooling*;
	do
		grep . $device/* >> $path
		echo "" >> $path
		grep . $device/*/* >> $path
		echo "" >> $path
	done
}

if [ $1 -a $1 -eq 1 ]; then
	reset_cpufreq_stats
	reset_thermal_stats
else
	echo "" > $path
	get_cpufreq_stats
	get_thermal_stats
fi
