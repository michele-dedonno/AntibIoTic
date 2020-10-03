#! /bin/bash

# Script that profiles the CPU, Memory and Network usage of a process.
# Input parameter: command to profile

INTERVAL=0.9 		# seconds between samples of CPU usage
DURATION=125 		# seconds of monitoring
OUTPUT="./stats.txt"	# output file

#set -x # debug

echo "[$0]> Run this script as 'sudo' to avoid errors."

if [ "$1" == "" ]
then
	echo -e "[$0]> Error:\n command missing.";
	echo -e "Usage:\t $0 <process name> [<output file>].";
	exit 1;
fi

if [ "$2" != "" ]
then
	OUTPUT=$2
fi

# get first word after /
# e.g., "./nc 127.0.0.1 999" will result in "nc"
pname=$(echo ${1##*/}| awk '{print $1}')
# split based on _
pname=$(echo ${pname//_/ } | awk '{print $1}')

# start a background process that kills this script and all related processes after DURATION secs.
{
	sleep $DURATION
        echo "[$0]> Killing main processes..."
        # check if it's not the current process or "evaluation.sh"
        evalpid=$(pgrep -nf evaluation)
	if [ $? == 1 ]; then evalpid=-1; fi
        for x in $(pgrep -f $pname); do if [ $x != $BASHPID ] && [ $x != $evalpid ]; then kill -9 $x; fi; done
        sleep 1
	echo "[$0]> Killing nethogs..."
        for x in $(pgrep nethog); do kill -9 $x; done
#	#cat $OUTPUT
#	echo "[$0]> Killing nethogs.."
#	for x in $(pgrep nethog); do kill $x; done
#	echo "[$0]> Killing '$pname'"
#	for x in $(pgrep -f '$pname'); do kill $x; done
#	echo "[$0]> Killing '$0'"
#	kill $(pgrep -nf '$0')
#	kill $$
} &

# Clean output files
echo " ">$OUTPUT
echo " ">out.txt
echo " ">err.txt

echo "[$0]> Starting profiling '$1'. Results saved in '$OUTPUT'."
echo "[$0]> Interval: $INTERVAL s; Duration: $DURATION s."

# start network profiling in background every $INTERVAL secs
nethogs -at -d $INTERVAL -v 1 >> $OUTPUT & # shows total KB Sent / Received, using . to separate decimals
#nethogs -at -v 1 | grep $initpid >> $OUTPUT &

# Execute command in background. 
# Output redirected to out.txt, Error to err.txt
$1>out.txt 2>err.txt &
#echo ">[PID]: $(pgrep -nf $pname)" >> $OUTPUT

sleep $INTERVAL # needed to let $1 run before getting its pid
ipid=$(pidof -s $1)
echo ">[PID]: $ipid" >> $OUTPUT	
while true
do
	#mpid=$(pgrep -nf $pname)
	mpid=$(pidof -s $1)
	if [ "$?" -eq 0 ]; then
		if [ $ipid != $mpid ]; then echo "!! WARNING: PID changed !! " >> $OUTPUT; fi
		# CPU and MEM profiling
		echo -e "."
		echo "-------------------------------------------------">> $OUTPUT
		ps --format pid,pcpu,pmem,cputime,etime,cmd -p $mpid >> $OUTPUT 
		echo ">Memory $(pmap -x $mpid | grep total)" >> $OUTPUT
		#cat /proc/$mpid/status | grep Vm >> $OUTPUT
		echo "=================================================">> $OUTPUT
	else
		echo "[$0]> Process not found: '$pname'"
	fi
	sleep $INTERVAL
done

#mpid=$initpid
#while true
#do
	#sleep $INTERVAL
	## check if the pid has changed (maybe because $1 was killed and restarted)
	#mpid=$(pgrep -n $pname)
	#if [ "$?" -eq 0 ]; then
	#	if [ $mpid != $initpid ]; then
	#		#if changed pid, restart network profiling
	#		initpid=$mpid
	#		nethogs -at -v 1 | grep $initpid >> $OUTPUT & # shows total KB Sent / Received
	#	fi
	#	ps --format pid,pcpu,pmem,cputime,etime,cmd -p $mpid >> $OUTPUT
	#	echo "Memory $(pmap -x $mpid | grep total)" >> $OUTPUT
	#else
	#	echo "[$0]> Process not found: '$pname'."
	#fi
#done
