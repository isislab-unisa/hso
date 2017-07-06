# Example: Spread of Influence



Social influence is the process by witch individual adjusts their opinions, revise their beliefs, or change their behaiviors as a result of interaction with other people.

This implies that social influence is a natural process, but can be used by people or businesses to change a person’s attitude or behavior.
For example the social influence can be used for promoting new products.

The goal of this example study the process of of influence diffusion in a social network.  

In this example we have:
* an Optimizer written in Java;
* a simulation written in C;
* a simulation written in CUDA C.

## Model detail
Given a graph G = (V,E), representing the network, initially only the members of a given S ⊆ V are influenced; subsequently, at each round, the set of influenced nodes is augmented by all the nodes in the network that have a sufficiently large number of already influenced neighbors.

The general problem is to find a small initial set of nodes that influences the
whole network. In this example we extend the previously described model in the following way: we assume that there are non negative values t(v) associated to each node v ∈ V ( t(v) rappresent how difficult is to influence the member v), and instead of using a set S, of initial influenced member, we study the consequences of giving incentives to member of the networks, and we quantify how this affects on starting an influence diffusion process that influence the whole network.

For the implementation of the Spread of Influence example see Optimizer, C simulation, CUDA C simulation
