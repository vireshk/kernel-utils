#!/bin/bash

get_single_thread_stats()
{
	hrtimer=$(cat $1 | grep -e "-$2.*hrtimer_start: hrtimer=" | tail -1 | sed -e "s/.*hrtimer=//g" | sed -e "s/\ .*$//g")
	cat $1 | grep -e "-$2.*hrtimer_start: hrtimer=$hrtimer" | sed -e "s/\t/:/g" | sed -e "s/\ \ */:/g" | sed -e "s/::*/:/g" > junk1

	rm junk2
	while read line
	do
		now=$(echo $line | cut -d ':' -f 5 | sed -e 's/\.//g')
		next=$(echo $line | sed -e 's/^.*softexpires=//g' | sed -e 's/...$//g')
		bc <<< "$next-$now" >> junk2
	done < junk1

	cat junk2 | datamash min 1 max 1 mean 1
}

parse_all_threads()
{
	cat $2 | grep T: | cut -d ')' -f 1 | cut -d '('  -f2 > tasks

	while read task
	do
		get_single_thread_stats $1 $task
	done < tasks
}

if [ $2 == 0 ]
then
	get_single_thread_stats $1 $3
else
	parse_all_threads $1 $3 > junk3
	cat junk3
fi
