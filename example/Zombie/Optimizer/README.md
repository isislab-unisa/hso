# Optimizer

The goal of the Optimizer is to find the better value of the following parameters:

* human_count, the number of people involved in the simulation process;
* zombie_count, the number (at the beginning of the simulation) of infected people;
* human_step_size, the human velocity;
* zombie_step_size, the zombie velocity.

The core of the Optimizer is the DEAP Framework, that make defining and running evolutionary algorithm like GAs easy.  

The GA creates an initial population p0 of parameter combinations of size _num_pop_ and proceeds to iteratively generate populations pi, for each generation i, based on the evaluated performance of individuals in the previous population pi-1. Each new population pi is created by first selecting with replacement num_pop individuals from pi-1 using a stochastic selection method (tournament selection with size 3 here). This biases pi to higher performing individuals. Then the population members are mated and mutated to create a new generation of num_pop offspring, where both mating and mutation probabilities are set through user defined parameters. The number of generation in _num_iter_.

To implement the optimizer, we started from the deap_ga.py code presented in the paper [_From desktop to large-scale model exploration with Swift/T_](http://dl.acm.org/citation.cfm?id=3042094.3042132).

We've made changes to allow deap_ga.py (our optimizer) to communicate with Master HSO.  
In the paper deap_ga.py communicate with a Swift/T script through two queue (IN_Queue and OUT_Queue), we've remove this queue and we've add the two zeroMQ sockets. To do this, we changed the queue_map function and added other lines of code to the deap_ga.py source.  
Below we can see the points on which we acted in the code:

```python
...

def queue_map(obj_func, pops):
    # Note that the obj_func is not used
    if not pops:
        return []
    buff=create_list_of_lists_string(pops);
    # -----begin code for zeroMQ--------
    evalQ.send(str(len(buff)));
    evalQ.send(buff)
    len_res = outQ.recv()
    result = outQ.recv()
    #-----end code for zeroMQ-----------
    split_result = result.split('::')
    results = split_result[1].split(";")
    return [(float(x),) for x in results];

...

# -----begin code for zeroMQ--------
context = zmq.Context()
evalQ = context.socket(zmq.PAIR)
evalQ.connect("evalQ address...")
outQ = context.socket(zmq.PAIR)
outQ.bind("outQ address...")
cpu_model="INIT_CPU::address model cpu::"
gpu_model="INIT_GPU::address model gpu::"
evalQ.send(str(len("INIT_OUT:: evalQ address...::")))
evalQ.send("INIT_OUT:: outQ address...::")
numcpugpu="INIT_NUM::"+str(cpu)+"::"+str(gpu)+"::"
evalQ.send(str(len(numcpugpu)))
evalQ.send(numcpugpu)
evalQ.send(str(len(cpu_model)))
evalQ.send(cpu_model)
evalQ.send(str(len(gpu_model)))
evalQ.send(gpu_model)
```
## Optimizer's prerequisites

Before running the Optimizer make sure you have the following requirements:

- Python 
- Numpy, the fundamental package for scientific computing with Python
- pandas (Python Data Analysis Library), is an open source, BSD-licensed library providing high-performance, easy-to-use data structures and data analysis tools for the Python programming language.
- DEAP is a novel evolutionary computation framework for rapid prototyping and testing of ideas
- PyZMQ, is the Python bindings for ØMQ.

## How to run the Optimizer

Before executing the optimizer, the addresses of the zeroMQ sockets must be correctly set up with their addresses.

To run the Optimizer:
> python deap_ga.py

#### Example output



