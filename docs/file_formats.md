# File Formats {#file_formats}

Game Graph Gym supports reading and writing game graphs in [Graphviz DOT](https://graphviz.org/doc/info/lang.html) format with custom attributes for different game types.

## Common Format Elements

All game graph types share these common elements:

### Vertex Attributes

- `name` (string): Human-readable vertex identifier
- `player` (int): Owning player (0 or 1)

### Edge Attributes  

- `label` (string): Human-readable edge label

## Game-Specific Formats

### Parity Games

Parity games add the following vertex attribute:

- `priority` (int): Priority value for parity condition

**Example:**

```dot
digraph ParityGame {
    v0 [name="start", player=0, priority=2];
    v1 [name="choice", player=1, priority=1]; 
    v2 [name="target", player=0, priority=0];
    
    v0 -> v1 [label="move"];
    v1 -> v2 [label="choose"];
    v2 -> v0 [label="restart"];
}
```

### Mean-Payoff Games

Mean-payoff games add the following vertex attribute:

- `weight` (int): Weight value for mean-payoff calculation

**Example:**

```dot
digraph MeanPayoffGame {
    v0 [name="start", player=0, weight=5];
    v1 [name="middle", player=1, weight=-2];
    v2 [name="end", player=0, weight=3];
    
    v0 -> v1 [label="forward"];
    v1 -> v2 [label="advance"];
    v2 -> v0 [label="cycle"];
}
```

### Büchi Games

Büchi games use the common format with additional semantic interpretation:

- Accepting vertices are typically marked with specific priorities or names
- The exact acceptance condition depends on the specific variant


## Parser Usage

All parsers follow the same interface pattern, using free functions:

```cpp
#include <libggg/parity/graph.hpp>

// Parse from file
auto game = ggg::parity::graph::parse("game.dot");

// Parse from stream
#include <fstream>
std::ifstream in("game.dot");
auto game2 = ggg::parity::graph::parse(in);
```

## Writing Games

Games can be written back to DOT format using the appropriate writer functions:

```cpp
#include <libggg/parity/graph.hpp>

ggg::parity::graph::write(*game, "output.dot");
```

## Validation

The parsers perform basic validation:

- Required attributes must be present
- Player values must be 0 or 1  
- Numeric attributes must be valid integers
- Graph structure must be well-formed

Invalid files will throw parsing exceptions with descriptive error messages.
