# API Guide {#api-guide}

This guide covers the core APIs for extending Game Graph Gym with new game types and solvers.

## Adding New Game Types

Game Graph Gym uses X-macros to automatically generate graph types and utility functions. To add a new game type, you just need to define the vertex, edge, and graph properties and then call the `DEFINE_GAME_GRAPH` macro as follows.

```cpp
// my_graph.hpp
#include <libggg/graphs/graph_utilities.hpp>

// Define vertex properties
#define VERTEX_FIELDS(F) \
    F(std::string, name) \
    F(int, player) \
    F(int, custom_property)

// Define edge properties
#define EDGE_FIELDS(F) \
    F(std::string, label)

// Define graph properties (if any)
#define GRAPH_FIELDS(F) \
    F(std::string, title)

// Generate the complete graph type with utilities
DEFINE_GAME_GRAPH(VERTEX_FIELDS, EDGE_FIELDS, GRAPH_FIELDS)

// Optionally add utilities specific to your graph type
inline bool is_valid(const MyGraph &graph) {
    return true;
}
```

This will generate (in the call-site namespace)

- `Graph` - a Boost adjacency_list alias with the declared bundled properties
- `Vertex`, `Edge` - descriptor type aliases
- `add_vertex()` and `add_edge()` - convenience helpers whose parameter lists
    match the declared vertex/edge fields
- `parse(std::istream&)` and `parse(const std::string&)` - DOT parsers that
    return `std::shared_ptr<Graph>` on success (or `nullptr` on failure)
- `write(std::ostream&)` and `write(const std::string&)` - DOT writers

To use the generated types and functions:

```cpp
#include "my_graph.hpp"

// Create graph and add vertices with named parameters
Graph graph;
const auto v1 = add_vertex(graph, "vertex1", 0, 42);
const auto v2 = add_vertex(graph, "vertex2", 1, 24);

// Add edges with properties
add_edge(graph, v1, v2, "edge_label");

// Parse from file or stream
auto graph_ptr = parse("input.dot");
auto graph_ptr2 = parse(std::cin);

// Write to file or stream
write(*graph_ptr, "output.dot");
write(*graph_ptr, std::cout);
```

## Adding New Solvers

### Implementing a Solver

To implement a new solver, inherit from the unified `Solver` interface with the appropriate [solution type](#solution-types) and implement the `solve()` method. For example, implement a new solver for parity games, that operates on `ggg::parity::graph::Graph` and produces solutions which encode winning regions only (`RSolution<ggg::parity::graph::Graph>`) as follows.

```cpp
#include "libggg/parity/graph.hpp"
#include "libggg/solvers/solver.hpp"

// Example solver providing winning regions and strategies
class YourSolver : public ggg::solvers::Solver<ggg::parity::graph::Graph, ggg::parity::RSolution> {
public:
    ggg::parity::RSolution solve(const ggg::parity::graph::Graph &game) override {
        ggg::parity::RSolution solution; // Default construct

        const auto [vertices_begin, vertices_end] = boost::vertices(game);

        for (auto it = vertices_begin; it != vertices_end; ++it) {
            const auto vertex = *it;
            const auto winner = computeWinner(game, vertex);
            solution.setWinningPlayer(vertex, winner);
        }

        return solution;
    }

    std::string get_name() const override {
        return "Your Custom Solver Name";
    }
};
```

You can use the project macro to expose this solver as a CLI, following the pattern used by the shipped tools:

```C++
// inside the game-specific tools/ or main.cpp
GGG_GAME_SOLVER_MAIN(ggg::parity::graph::Graph, ggg::parity::graph::parse, YourSolver)
```

See the implementations of existing solvers under `solvers/` as examples.

## Solution Types

We define different solution capabilities via C++ concepts and inheritance.
Solution types are named `XSolution`, where the prefix `X` indicates what is encoded:
initial state only (`X=I`), winning regions (`R`), a winning strategy (`S`), and quantitative (vertex) values (`Q`).
For instance, `RSSolution<graphs::ParityGraph>`C++ is a type of solution operating on `ParityGraph`s and which can compute a winning `R`egion as well as synthesize a `S`trategy. In more details,

### `ISolution<GraphType>`

The most basic solution type on `<GraphType>` graphs. It records only the initial winner label for convenience and provides serialization for solution payloads. There is no solved/valid flag in solutions; solvers either return a result or signal errors via logs/exceptions.

See also: [`include/libggg/solutions/isolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/isolution.hpp)

### `RSolution<GraphType>`

Contains info about winning **R**egions.

In addition to the inherited methods of `ISolution`, solutions of this kind implement:

- `is_won_by_player0(vertex)` - Check if vertex is won by player 0
- `is_won_by_player1(vertex)` - Check if vertex is won by player 1
- `get_winning_player(vertex)` - Get winning player (0, 1, or -1)
- `set_winning_player(vertex, player)` - Set winning player for vertex

See also: [`include/libggg/solutions/rsolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/rsolution.hpp)

### `SSolution<GraphType>`

Contains info about a winning **S**trategy

In addition to the inherited methods of `ISolution`, solutions of this kind implement:

- `get_strategy(vertex)` - Get strategic choice for vertex
- `has_strategy(vertex)` - Check if vertex has strategy defined
- `set_strategy(vertex, strategy)` - Set strategy for vertex

There is actually a second generic parameter specifying the type of strategy contained (mixing, deterministic, positional, finite-memory etc). The default is `DeterministicStrategy`, representing deterministic and positional strategies (i.e. mappings from vertices to vertices).

See also: [`include/libggg/solutions/ssolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/ssolution.hpp)

### `QSolution<GraphType, ValueType>`

Contains info about **Q**ualitative (vertex) values of type `ValueType`.

In addition to the inherited methods of `ISolution`, solutions of this kind implement:

- `get_value(vertex)` - Get numerical value for vertex
- `has_value(vertex)` - Check if vertex has value defined
- `set_value(vertex, value)` - Set value for vertex

See also: [`include/libggg/solutions/qsolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/qsolution.hpp)

### Combined Solution Types

- **`RSSolution<GraphType>`**: Regions + Strategies (inherits R and S methods)
- **`RQSolution<GraphType, ValueType>`**: Regions + Quantitative values (inherits R and Q methods)
- **`RSQSolution<GraphType, ValueType>`**: All capabilities (inherits R, S, and Q methods)

See also: [`include/libggg/solutions/rssolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/rssolution.hpp), [`include/libggg/solutions/rsqsolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/rsqsolution.hpp)

Choose the appropriate solution type based on what your solver provides.
