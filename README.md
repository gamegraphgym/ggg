# Game Graph Gym <img src="https://github.com/gamegraphgym/ggg/raw/main/.github/logo.png" align="right" height="100" alt="logo"/>

[![Build and Test](https://github.com/gamegraphgym/ggg/actions/workflows/ci.yml/badge.svg)](https://github.com/gamegraphgym/ggg/actions/workflows/ci.yml)
[![Documentation](https://github.com/gamegraphgym/ggg/actions/workflows/docs.yml/badge.svg)](https://github.com/gamegraphgym/ggg/actions/workflows/docs.yml)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

A C++ framework for easy implementing and benchmarking solvers for [games on graphs][GOG-book].
It comes in the form of

- a lightweight C++ library, built on top of the [Boost Graph Library][BGL] that defines data structures for various game graphs (plus utilities like parser/formatter/optimizations) and solvers, and generic algorithms.
- a set of command-line tools for generating random benchmark corpora, filtering/optimising game graphs graphs, running benchmarks.

The library is independent from benchmarking tools, which are designed with inter-operability in mind. For instance, one can compare the performance of any game solver implementation as long as executable has a compatible I/O interface.  
This project also ships with implementations of several standard solvers, available through the API and stand-alone executables for comparisons and direct use.

See the documentation at [docs/](docs/) for detailed build instructions and development setup.

## Quick Start

You will need cmake and some boost libraries:
*Graph* as well as *Filesystem*, *System*, *ProgramOptions* (only for tools and solvers), and *Test* (only for unit testing).  
Get all you need and more via
`apt-get install -y build-essential cmake libboost-all-dev` (Debian/Ubuntu) or `brew install cmake boost` (MacOS). Then,

```bash
# Clone the repository
git clone https://github.com/gamegraphgym/ggg.git && cd ggg

# Configure and build with all bells and whistles included
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTOOLS_ALL=ON
cmake --build build -j$(nproc)
```

Done! You can now use the API in your own code or run the included solvers and benchmark tools.

## CLI Tools

There are command-line tools for benchmarking as well as executables for various game solvers.
For instance, benchmark parity game solvers like so:

```bash
# Generate 20 parity games with 100 vertices each and store them in directory `./games/`
./build/bin/ggg_parity_generate -o games -c 20 -v 100

# List available solvers for `parity` games
ls -1 build/bin/ggg_parity_solver_*

# Solve some generated game
./build/bin/ggg_parity_solver_priority_promotion games/parity_game_0.dot
```

### Universal CLI Interface for Solvers

All solver binaries provide a standardized command-line interface:

- `-h, --help`: Show help message and usage synopsis
- `-f, --format`: Output format; one of `plain` (default) or `json`
- `--time-only`: Only output timing information
- `--solver-name`: Display solver name and exit
- `-v`: Increase verbosity (can be used multiple times: `-v`, `-vv`, `-vvv`) when logging is enabled

The first non-option positional argument is interpreted as the input path (use `-` for stdin). If omitted, the input defaults to `-` (stdin).

All binaries are placed into `build/bin`.
Solvers follow the naming scheme `ggg_X_solver_Y` where `X` refers to the type of game (parity, mean_payoff,...) and `Y` is the name of the algorithm it implements.

## Included Game Types and their Representations

So far, supported are (two-player, zero-sum, non-stochastic) games with parity, mean-payoff, buechi, and  reachability conditions.  
Game graphs can be imported from and exported to [Graphviz DOT](https://graphviz.org/doc/info/lang.html) format with custom attributes for vertices and (directed) edges:

- **All Game Graph Types** admit properties `name` (string), `player` (int) on vertices and `label` (string) on edges.
- **Parity game graphs** add a property `priority` (int) on edges;
- **Mean-Payoff game graphs** add a property `weight` (int) on vertices.

By design, it is rather easy to define your own game graph types with custom edge/vertex/graph properties and automatically derive data structures, parsers and writers for the DOT like format.
The above serve as proof-of concept as much as starting point for a collection of solvers for common games on graphs.

## Documentation

The complete documentation is built with [MkDocs Material](https://squidfunk.github.io/mkdocs-material/). From within the project root:

```bash
pip install mkdocs-material
mkdocs serve  # Development server at localhost:8000
mkdocs build  # Static site in site/ directory
```

[GOG-book]: https://arxiv.org/abs/2305.10546
[BGL]: https://www.boost.org/doc/libs/release/libs/graph/
