# Command-line tools {#tools}

GGG provides macros to define command-line tools with a uniform interface, input parsing and validation.

## Game solvers

The executables for game solvers are called `ggg_X_solver_Y`, where `X` identifies the game graph type and `Y` is the name of the algorithm. For instance, the "priority promotion" algorithm for solving Parity games is compiled into a binary  named `ggg_parity_solver_priority_promotion`.


### Input File Formats {#input_formats}

GGG provides parsing and writing game graphs in [Graphviz DOT](https://graphviz.org/doc/info/lang.html) format with custom attributes for the dynamic properties defined on the graph type.

For example, Parity games graphs have the following vertex attributes:

- `player` (int): owner of the vertex (can be 0 or 1)
- `priority` (int): Priority value for parity condition
  
It also interprets a `String` valued attribute called "label" on edges.  
The following is a valid input format for such a game graph

```dot
digraph ParityGame {
    v0 [player=0, priority=2];
    v1 [player=1, priority=1]; 
    v2 [player=0, priority=0];
    
    v0 -> v1 [label="move"];
    v1 -> v2 [label="choose"];
    v2 -> v0 [label="restart"];
}
```

## Random Game Graph Generators

The executables for randome game graph generators are called `ggg_X_generate`, where `X` identifies the game graph type. For instance, a generator for Parity game graphs is compiled into a binary named `ggg_parity_generate`.

## Benchmarking Scripts

GGG ships some convenience scripts in `/extra/scripts` to run and plot benchmarks and convert file formats.
