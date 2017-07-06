# Optimizer

The goal of the Optimizer is find an allocation of partial incentives, on a members of a social network, in order to  start aan influence diffusion process that influence the whole network (or a maximum number of member).

The Optimizer is written in Java, and the idea behind its operation is as follows:

Given a certain ammount of incentives S, the optimizer generate a num_conf configurations "*s*" of allocation of incentives on the members of network. And for each of this configurations starts different process of influence diffusion (each process use a randomly genereted vector of threshold t). The goal is find the configuration "_s_" that maximize the number of members influenced.


The core of the Optimizer is the **Optimal Computing Budget Allocation (OCBA)** algorithm.

The OCBA approach can determine the most efficient simulation replication numbers or simulation lengths for all simulated alternatives. The goal is to obtain the highest simulation decision quality using a fixed computing budget, or to attain a desired simulation decision quality using a minimum computing budget.[[1]](http://mason.gmu.edu/~cchen9/ocba.html)

## Optimizer's prerequisites
Before running the Optimizer make sure you have the follow requirements:

* Java
* ZeroMQ Java binding
* JGraphT Library
