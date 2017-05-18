# Example: Zombie

The goal of this tutorial is show the usage of the Heterogeneous Simulation Optimization (**HSO**).

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
* zombie_count, the intial number (at time 0 of the simulation) of infected people;
* human_step_size, the human velocity;
* zombie_step_size, the zombie velocity.

