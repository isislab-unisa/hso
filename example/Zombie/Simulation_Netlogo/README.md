# Zombie: simulation Netlogo

This is a Java wrapper to execute NetLogo models from command line available on [GitHub](https://github.com/spagnuolocarmine/swiftlangabm/tree/master/netlogo) developed by:
- Carmine Spagnuolo (Univeristà degli Studi di Salerno)
- Jonathan Ozik (Argonne National Laboratory)
- Nicholson Collier (Argonne National Laboratory)

### Build the project

First, download in this folder the files from [GitHub repository](https://github.com/spagnuolocarmine/swiftlangabm/tree/master/netlogo), and follow the instructions in the README file to build the project.

In the folder, a script was written to launch the project. This is the script that the optimizer will tell HSO to use for the simulation to run on the CPU.

cpu_script.sh:
```
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
```