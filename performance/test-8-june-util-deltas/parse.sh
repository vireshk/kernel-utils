#!/bin/bash

get_diff()
{
	prev=0

	while read line;
	do
		[ $prev != 0 ] && bc <<< "$line-$prev";
		prev=$line;
	done < junk-$1-2
}

cat $1 | grep -e "\[00$2\]" > cpu$2
#cat $1 | grep -e "sugov_update_single" | grep -e "\[00$2\]" > junk-$2-1
##cat $1 | grep -e "cpu_frequency_v: .* cpu_id=3" -e "cpu_frequency_v: .* cpu_id=4" | grep -e "\[00$2\]" > junk1
#cat junk-$2-1 | sed -e "s/^.*\].....//g" | cut -d ':' -f1 | sed -e "s/\.//g"> junk-$2-2
#get_diff $2 > junk-$2-3
#cat junk-$2-3 | cut -d '.' -f2 | datamash min 1 max 1 mean 1 > junk-$2-4
#cat junk-$2-4
