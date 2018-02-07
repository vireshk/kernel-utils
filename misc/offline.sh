#!/system/bin/sh

if [ $1 -a $1 -eq 4 ]; then
	for i in `seq 4 7`;
	do
		echo 0 > /sys/devices/system/cpu/cpu$i/online
	done
else
	for i in `seq 0 3`;
	do
		echo 0 > /sys/devices/system/cpu/cpu$i/online
	done
fi
