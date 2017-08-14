#!/system/bin/sh

path=/lib/modules

# $1: name of the module
modprobe_with_depen()
{
	OIFS=$IFS
	IFS=','

	local mod=$1
	local _mod=$(echo $mod | sed 's/-/_/g')

	# $mod not present in $path
	[[ -f $path/$mod.ko ]] || return

	# Return if $mod is already inserted
	[[ $(lsmod | cut -d ' ' -f 1 | grep "^$_mod$") ]] && return

	# Find and insert all modules on which $mod depend
	dependencies=$(modinfo $path/$mod.ko | grep depends\: | sed 's/^depends\:\ *//')

	for dep in $dependencies
	do
		modprobe_with_depen $dep
	done

	insmod $path/$mod.ko

	IFS=$OIFS;
}

# Inserts all modules present in /lib/modules/ and named as g*.ko
insert_modules()
{
	# Get name of the module
	for mod in $(ls $path/g*.ko)
	do
		modprobe_with_depen $(echo $(basename $mod) | sed 's/\.ko//')
	done

}

# For testing only
remove_modules()
{
	for mod in $(lsmod | sed 1d | sed 's/\ .*//')
	do
		rmmod -f $mod
	done

}

# Uncomment below for testing
remove_modules
