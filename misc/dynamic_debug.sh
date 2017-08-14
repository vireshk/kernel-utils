#!/system/bin/sh

echo "module greybus =p" > /sys/kernel/debug/dynamic_debug/control
dmesg -n8
