#!/bin/bash

get_diff()
{
	x=0;
	prev=0

	while read line;
	do
		if [ $x == 0 ]
		then 
			x=1;
			prev=$line;
		else
			x=0;
			bc <<< "$line-$prev";
		fi;
	done < junk2
}

get_single_thread_stats()
{
	hrtimer=$(cat $1 | grep -e "-$2.*hrtimer_start: hrtimer=" | tail -1 | sed -e "s/.*hrtimer=//g" | sed -e "s/\ .*$//g")
	cat $1 | grep "hrtimer_start: hrtimer=$hrtimer" | sed -e "s/\t/:/g" | sed -e "s/\ \ */:/g" | sed -e "s/::*/:/g" | cut -d ':' -f 5 > junk2
	get_diff > junk3
	cat junk3 | cut -d '.' -f2 | datamash min 1 max 1 mean 1
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
	parse_all_threads $1 $3 > junk4
	cat junk4
fi
