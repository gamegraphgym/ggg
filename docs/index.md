# Game Graph Gym

Welcome to the Game Graph Gym documentation! This framework provides a comprehensive C++20 library for implementing and benchmarking solvers for games on graphs.

## Overview

**Game Graph Gym (GGG)** is a C++20 framework designed for implementing and benchmarking solvers for games on graphs. The architecture consists of:

- **Core Library**: provides templates for defining custom graph types, parsers, and generic algorithms.
- **Game Solvers**: for some well-known types of games (buechi, parity, mean-payoff, and stochastic discounted)
- **CLI Tools**: for solvers with uniform CLI interface
- **Convenience Scripts**: for benchmarking and visualisations


## Supported Game Types

Game Graph Gym supports several types of two-player, zero-sum, non-stochastic games out of the box.

- **Büchi Games**: Games with Büchi acceptance conditions ("buechi");
- **Parity Games**: Games with parity winning conditions ("parity");
- **Mean-Payoff Games**: Games with mean-payoff objectives ("mean_payoff");
- **Stochastic Discounted**: Probabilistic Games with discounted payoffs.

## File Formats

All game types can be parsed from / written as files in a format similar to [Graphviz DOT](https://graphviz.org/doc/info/lang.html) with custom attributes. See the [File Formats](file_formats.md) guide for details.

## Command-Line Tools

The framework includes several command-line utilities:

- `ggg_X_generate`: Generate random game instances for type `X` (parity,mean_payoff, or stochastic_discounted)
- `ggg_X_solver_Y` Solve a game of type `X` using algorithm `Y`

## Next Steps

- [User Quick Start](quick_start.md) - How to get up and running quickly and use GGG in your projects
- [Building the Library](building.md) - Detailed build instructions
- [File Formats](file_formats.md) - Supported input/output formats
- [Developer Guide](developing.md) - Setup development environment and contribution guidelines
