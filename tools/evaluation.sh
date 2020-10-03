#!/bin/bash

# Script that evaluates the command $1 running $2 profilings. 
# First it runs simulation.sh, then profiling.sh

# NOTE: make sure that the DURATION in SIMULATION is < = than
#	DURATION in PROFILING

SIMULATION="./simulation.sh"	# command used for simulation
PROFILING="./profiling.sh" 	# command used for profiling
OUTPUT="./stats"		# output folder

echo "[$0]> This script requires 'sudo' to run without errors."

if [ "$1" == "" ] || [ "$2" == "" ]
then
	echo -e "[$0]> Error: wrong input parameters.";
        echo -e "Usage:\t $0 <process name> <iterations>.";
        exit 1;
fi

mkdir -p $OUTPUT

for i in $(seq $2)
do
	$SIMULATION &
	sleep 3 
	iter=$(printf "%04d" $i)
	sudo $PROFILING "$1" "$OUTPUT/stats$iter.txt"
	sleep 3
done
