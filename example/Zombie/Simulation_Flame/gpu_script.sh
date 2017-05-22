#/bin/bash

if [ $# -eq 0 ]
	then
		echo "No arguments supplied: run ./script.sh <params> <gpuid>"
		exit 1
fi

INPUT=$1
ITER=50
GPUID=$2
DATA_DIR="" # folder that contains the XML file of data agent information (intial agent data)

if [ -d "$DATA_DIR" ]; then
   rm -r "$DATA_DIR"
fi

mkdir "$DATA_DIR"

if [ -f "$DATA_DIR/0.xml" ]; then
		rm "$DATA_DIR/0.xml"
fi
cd "" # go to the folder that contains XMLWriter
./XMLWriter "$DATA_DIR/0.xml"  "$INPUT" 
cd "" #go to the path of FlameGPU bin
touch "file$GPUID"
./ZombiePartitioning_console "$DATA_DIR/0.xml" $ITER $GPUID >> "file$GPUID"
rm "file$GPUID"
cd "" #go to the folder that contains XMLReader
./XMLReader "$DATA_DIR/$ITER.xml"
