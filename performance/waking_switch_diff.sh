#!/bin/bash
# $1: trace .txt file, $2: 0 if $3 is task-number or $2:1 if $3 is results file
# output file.

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
	# Finds only the interesting lines for us
	cat $1 | grep $2 | grep -e "sched_switch:" -e "sched_waking:" | grep -e "\<pid=$2" -e "next_pid=$2" > junk1

	# Skips the first sched_switch line if present

	# Drop the first line if it contains sched_switch
	if head -n 1 junk1 | grep sched_switch --quiet;
	then
		tail -n +2 junk1 > junk2
		mv junk2 junk1
	fi

	cat junk1 | cut -d ':' -f1 | cut -b 35- > junk2

	get_diff > junk3

	printf "Thread $2: "
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
