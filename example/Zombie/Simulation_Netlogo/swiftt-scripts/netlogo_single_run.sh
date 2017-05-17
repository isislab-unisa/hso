#!/bin/bash

set -eu

# Args
param_line=$1   # The upf param line    
#echo "$param_line"
instanceDir=$2  # The instance directory
modelname=$3

if [[ ${JAVA:-0} == 0 ]] 
then
  JAVA=java 
fi

IFS=','
read -ra PARAMS <<< "$param_line"
$JAVA -Xmx1536m -XX:-UseLargePages -jar netlogo-1.0-wrapper.jar
	-m $instanceDir/$modelname \
	-outfile counts.csv \
	-runid 1 \
	-s 150 \
	-trial 1 \
	-i human_count,${PARAMS[0]},zombie_count,${PARAMS[1]},human_step_size,${PARAMS[2]},zombie_step_size,${PARAMS[3]} \
	-o human_count\