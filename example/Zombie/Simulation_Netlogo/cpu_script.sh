#!/bin/bash

set -eu

if [ $# -eq 0 ]
	then
		echo "No arguments supplied: run ./script.sh <params>"
		exit 1
fi

param_line=$1

IFS=','
read -ra PARAMS <<< "$param_line"
cd ../example/Zombie/Simulation_Netlogo
java -Xmx1536m -XX:-UseLargePages -jar target/netlogo-1.0-wrapper.jar \
	-m resources/models/JZombiesLogo.nlogo \
	-outfile counts.csv \
	-runid 1 \
	-s 50 \
	-trial 1 \
	-i human_count,${PARAMS[0]},zombie_count,${PARAMS[1]},human_step_size,${PARAMS[2]},zombie_step_size,${PARAMS[3]} \
	-o human_count\
