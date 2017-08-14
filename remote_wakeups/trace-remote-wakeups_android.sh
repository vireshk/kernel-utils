#!/system/bin/sh

#gcc -pthread -o remote_latency_wake_steve.o remote_latency_wake_steve.c

# Locals
BUFFER=40000
FILE="dft"

# What to trace ??
EVENTS=" -e sched -e power -e workqueue -e irq"

echo "Starting trace"

./trace-cmd start -b $BUFFER $EVENTS

#./remote_latency_wake_steve.o

./rt-app remote-wakeup.json

./trace-cmd stop

cat /sys/kernel/debug/tracing/trace > $FILE.txt
./trace-cmd extract -o $FILE.dat

echo "Saved trace in: "$FILE
sync
