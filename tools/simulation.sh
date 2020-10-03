#!/bin/bash

# This helper script spawns 2 processes at random time inverval between 1 and $INTERVAL. 
# It is used as simulation script for evaluation purposes.

# % ---------- Variables to edit ---------- %
BACKDOOR="./bin/tests/backdoor" # cmd to execute the backdoor
MALWARE="./bin/tests/malware"   # cmd to execute the malware
INTERVAL=10      		# max seconds of sleep at each iteration 
DURATION=112     		# seconds after which the simulation stops
# % ----------  ---------------  ---------- %

#set -x # for debug

# starts a background process that kills this script after DURATION secs
{
    sleep $DURATION
    for x in $(pgrep -f $BACKDOOR); do kill -9 $x; done
    for x in $(pgrep -f $MALWARE); do kill -9 $x; done
    kill $$
} &

# Run the binary passed as argument
function run_bin() {
        # $1 binary to run

#        # Check if already in execution
#        temp=$(pidof "$1")
#        # if return value = 0, process already in execution
#        if [ $? -eq 1 ]; then
#        # if return value = 1, process not found: run it
                echo "[$0]> $1"
                $1 &
#        fi
}

while true
do
	run_bin "$BACKDOOR"
	run_bin "$MALWARE"
	rand=$(($RANDOM % $INTERVAL + 1))
	echo "[$0]> Sleeping for $rand s."
	sleep $rand
done
#while true
#do
#        sleep $INTERVAL
#        # get a random value
#        num=$RANDOM
#        #echo $num
#        if [ $((num%2)) -eq 0 ]; then
#                # if it's even: execute backdoor
#                run_bin "$BACKDOOR"
#        else
#                # if it's odd: execute malware
#                run_bin "$MALWARE"
#        fi
#done
