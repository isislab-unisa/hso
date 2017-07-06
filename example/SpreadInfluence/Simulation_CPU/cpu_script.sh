#!/bin/bash

set -eu

if [ $# -eq 0 ]
	then
		echo "No arguments supplied: run ./script.sh <params>"
		exit 1
fi

param_line=$1

IFS='_'
read -ra PARAMS <<< "$param_line"
cd ../example/Spread_Influence/Simulation_Cpu
./cpu_spread ${PARAMS[0]} ${PARAMS[1]} ${PARAMS[2]}
