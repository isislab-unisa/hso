# Zombie: simulation FLameGPU

In this section we will see how to build the FlameGPU simulation for the Zombie model. 

## Install FlameGPU

First of all we need install FlameGPU on the machine, to do this:

1. Download FlameGPU from the [download link](http://www.flamegpu.com/downloads/links).
2. Unzip the file just downloaded  

> Note: for the example, I have unpacked the zip file in the Simulation_Flame folder, and the script written for simulation uses this assumption, if you want to unpack FlameGPU in a different folder, you must take this into account and edit the script with the Appropriate path

3. Go to the _examples_ directory and build all example or just the one you want, look at the READ ME (makefiles) file in the FlameGPU directory


Before running an example in the _bin_ directory of FlameGPU, make sure you have installed a compatible CUDA version.  

For more information about FlameGPU and its use, look at the documentation in the _doc_ directory.

## Create the Zombie model

In the _Simulation\_Flame_ directory there a directory _Flame\_model_ that contains the necessary file to create the model with FlameGPU.  
To create and build  the Zombie example follow this step:

- Go to the _examples_ directory of FlameGPU
- Copy the directory _Template_ and rename the directory just copied as Zombie_Partitioning  
- Go to the _Zombie\_Partitioning/src/model_ 
- Replace the _function.c_ and _XMLModelFile.xml_ files with those in the _Flame\_model_ folder
- Go to the _Zombie\_Partitioning directory and build the example using "make"


After doing this you can run the example by launching the Zombie_Partitioning_console.sh script located in the _bin_ folder of FlameGPU.

##Run the model

To simplify the launch of the example were written:

- XMLWriter.c: this c program create the initial XML file for the FlameGPU simulation. To build this file:
	```
	gcc -o XMLWriter XMLWriter.c $(xml2-config --cflags --libs)

	```
- XMLReader.c: this c program read the output XML file generated from the FlameGPU simulation.
 	```
	gcc -o XMLReader XMLReader.c $(xml2-config --cflags --libs)

	```
- gpu_script.sh: the script that create the XML initial file for the simulation, starts the simulation and get the result. Follow the code of the script:  
	```
	#/bin/bash

	if [ $# -eq 0 ]
		then
			echo "No arguments supplied: run ./script.sh <params> <gpuid>"
			exit 1
	fi

	INPUT=$1
	ITER=50
	GPUID=$2
	DATA_DIR="" # folder that contains the XML file of data agent information

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
	```

