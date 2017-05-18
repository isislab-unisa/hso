# Heterogeneous Simulation Optimization (HSO)

The simulation are used to model and analyze complex behaviors. These behaviors are translated into a mathematical model governed by a set of parameters.  
Usually, we want find some parameters that make the solution of the model admissible and close to the best.
Simulation optimization is the process of finding the best input variable values from among all possibilities without explicitly evaluating each possibility. This process optimization aims to minimize the resources spent while maximizing the information obtained in a simulation experiment.


**Heterogeneous Simulation Optimization (HSO)** is:

* Developed for Simulation Optimization.
* Written in C, and use the MPI Library.
* Supported platform: Linux.

HSO supports the Heterogeneous Computing, that is exploit the available computational accelerators for run the simulations in the better way.

Another interesting feature of HSO is that you can use optimization function written in different programming language (C, C++, C#, Python, Java, etc.) using the [ZeroMQ](http://www.zeromq.org/) Library.

## HSO structure

![HSO structure](./assets/architettura.jpg)

The figure illustrates the main components of **HSO**. We can see that the optimizer is connected to the HSO master through two queues, which are essentially two ZeroMQ socket. Strings are sent on these sockets, these strings can be of three types:

* Control strings, used to set the simulation execution environment.
* Strings containing the list of parameters to run the simulations.
* Strings containing the result of the simulations.

> The first two types of string are sent on the EVAL socket from the optimization function to the HSO Master, while the last type is sent on the OUT socket form the HSO Master to the optimization function

### HSO workflow

In the initialization phase, the coordinated execution between the HSO Master (and Slaves) and  the Optimization function follows this pattern:

1. When the HSO Master starts, it waits for the Slaves to send the number of CPUs and Computational Accelerators available (for the moment we consider only the GPUs).
2. The Slave sent this information and wait for simulation model informations from the HSO Master.
3. The HSO Master waits for the Optimization function to send strings.
4. When the Optimization function starts, send to the Master the address of OUT socket, the CPU model and the GPU model.
5. The HSO master sets the simulation execution environment, sends the information received to the slaves, and waits the list of parameters from the Optimization function.
6. The Slave sets the CPU model and the GPU model and waits for the parameters of simulations from HSO master.

![Scambio messaggi fase iniziale](./assets/HSO_init.png)

After the initialization phase, the coordinated execution between the HSO Master (and Slaves) and  the Optimization function follows this pattern:

1. The Optimization function send the list of parameters to the Master HSO.
2. The HSO Master, received the list, divides it and starts running the simulations by distributing them to themselves and to the slaves. After finishing his computation part waits for the slave result.
3. The Slave, received the list of parameters, starts running the simulations. After finishing his simulations send the result to the HSO Master. 
4. The HSO Master collects the slave's output, concatenates his, and sends everything to the Optimization function.
5. The optimization function, received the output, makes its own consideration and decides whether to terminate or send a new parameter list.
6. After finishing the computation for an Optimization function,the HSO Master waits for a new Optimization function to be launched, and the process just described will start again

![scambio messaggi computazione](./assets/HSO_com.png)

## HSO installation and usage

Before installing make sure you have the following requirements:

* autoreconf 2.69 or higher 
* MPI
* ZeroMQ, for installation guide see [ZeroMQ - Get the software](http://zeromq.org/intro:get-the-software)

For installing HSO:

1. Download the HSO github repository 
>git clone https://github.com/isislab-unisa/hso.git

2. Install HSO:
> cd "path to HSO folder"  
> chmod +x autogen.sh  
> ./autogen.sh  
> ./configure   
> make  
> cd src/  
> chmod +x hso

For usage of HSO see the [Example: Zombie](./example/Zombie/README.md)

