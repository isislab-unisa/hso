#/bin/bash

if [ $# -eq 0 ]
	then
		echo "No arguments supplied: run ./script.sh <params> <gpuid>"
		exit 1
fi

INPUT=$1
ITER=50
GPUID=$2
DATA_DIR="$HOME/provainput/$GPUID"

if [ -d "$DATA_DIR" ]; then
   rm -r "$DATA_DIR"
fi

mkdir "$DATA_DIR"

if [ -f "$DATA_DIR/0.xml" ]; then
		rm "$DATA_DIR/0.xml"
fi
cd ../example/Zombie/Simulation_Flame
./XMLWriter "$DATA_DIR/0.xml"  "$INPUT"
cd FLAMEGPU-master/bin/x64/Release_Console
touch "file$GPUID"
./ZombiePartitioning_console "$DATA_DIR/0.xml" $ITER $GPUID >> "file$GPUID"
rm "file$GPUID"
cd ../../../../
./XMLReader "$DATA_DIR/$ITER.xml"
