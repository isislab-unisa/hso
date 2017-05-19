# Optimizer

The goal of the Optimizer is to find the value of the following parameters:

* human_count, the number of people involved in the simulation process;
* zombie_count, the number (at the beginning of the simulation) of infected people;
* human_step_size, the human velocity;
* zombie_step_size, the zombie velocity.

The core of the Optimizer is the DEAP Framework, that make defining and running evolutionary algorithm like GAs easy.  

Our GA creates an initial population p0 of parameter combinations of size num_pop and proceeds to iteratively generate populations pi, for each generation i, based on the evaluated performance of individuals in the previous population pi-1. Each new population pi is created by first selecting with replacement num_pop individuals from pi-1 using a stochastic selection method (tournament selection with size 3 here). This biases pi to higher performing individuals. Then the population members are mated and mutated to create a new generation of num_pop offspring, where both mating and mutation probabilities are set through user defined parameters. The number of generation in num_iter.


## Optimizer's prerequisites

## How to run the Optimizer