# Game Graph Gym

The **Game Graph Gym (GGG)** is a framework for implementing and benchmarking solvers for games on graphs. The architecture consists of:

- **Core Library**: a C++20 library that provides templates for defining custom graph types, parsers, and generic algorithms.
- **Graphs and Solvers**: for some well-known types of games (buechi, parity, mean-payoff, and stochastic discounted)
- **Commandline Tools**: for using solvers with uniform CLI interface, random game generators, and convenience scripts for benchmarking.