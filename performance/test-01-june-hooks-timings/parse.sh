#!/bin/bash

get_diff()
{
	prev=0

	while read line;
	do
		bc <<< "$line-$prev";
		prev=$line;
	done < junk2
}

cat $1 | grep -e "cpu_frequency_v: .* cpu_id=1" | grep -e "\[00$2\]" > junk1
#cat $1 | grep -e "cpu_frequency_v: .* cpu_id=3" -e "cpu_frequency_v: .* cpu_id=4" | grep -e "\[00$2\]" > junk1
cat junk1 | sed -e "s/^.*\].....//g" | cut -d ':' -f1 | sed -e "s/\.//g"> junk2
get_diff > junk3
cat junk3 | cut -d '.' -f2 | datamash min 1 max 1 mean 1
