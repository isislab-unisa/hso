# Example: Zombie

The goal of this tutorial is to show the usage of the Heterogeneous Simulation Optimization (**HSO**).

In this example we have:

1. An Optimizer written in Python;
2. A Netlogo simulation for Zombie model.
3. A FlameGPU simulation for Zombie model.

>Note: The simulations are not totally equivalent but both follow the Zombie model specification described below.

## Model detail
The model has two kinds of agent:

* Zombies
* Humans.

Zombies chase the Humans, seeking to infect them, while Humans attempt to evade Zombies. When a Zombie is close enough to a Human, that Human is infected and becomes a Zombie.
During a typical run all the Humans will eventually become Zombies.

These agents are located in a two dimensional continuous space where each agent has a x and y coordinate expressed as a floating point number. Movement is performed in the continuous space.


The model is characterized by:

* human_count, the number of people involved in the simulation process;
* zombie_count, the number (at the beginning of the simulation) of infected people;
* human_step_size, the human velocity;
* zombie_step_size, the zombie velocity.


In the Simulation Optimization process we want to find the value of these parameters that maximize the number of alive Humans after the simulation is completed.

For the implementation of the Zombie example see [Optimizer](./Optimizer/README.md), [Netlogo simulation](./Simulation_Netlogo/README.md), [FlameGPU simulation](./Simulation_Flame/README.md)

## Run the example

Before run the example, you must make these preliminary steps:

1. Check the Optimizer's prerequisites and set some of its variables correctly for it to run, see [Optimizer](./Optimizer/README.md)
2. Build the Netlogo simulation, see [Netlogo](./Simulation_Netlogo/README.md)
3. Build the FlameGPU simulation, see [FlameGPU](./Simulation_Flame/README.md)


After completing these preliminary steps:

1. Run HSO
```
    mpirun -np numTask -host listHost hso
```
2. Run the Optimizer (the Optimizer must not necessarily be launched on the same machine where HSO is running), more details in [Optimizer](./Optimizer/README.md) :
```
    cd "path of Optimizer"  
    python deap_ga.py
```
