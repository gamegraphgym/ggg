# API Guide {#api_graphs}

## Adding New Game Types

Game Graph Gym uses X-macros to automatically generate graph types and utility functions. To add a new game type, you just need to define the vertex, edge, and graph properties and then call the `DEFINE_GAME_GRAPH` macro as follows.

```cpp
// my_graph.hpp
#include <libggg/graphs/graph_utilities.hpp>

// Define vertex properties with explicit default values
// Each field requires: F(type, name, default_value)
#define VERTEX_FIELDS(F) \
    F(std::string, name, "") \
    F(int, player, -1) \
    F(int, custom_property, 0)

// Define edge properties with explicit default values
#define EDGE_FIELDS(F) \
    F(std::string, label, "")

// Define graph properties with explicit default values (if any)
#define GRAPH_FIELDS(F) \
    F(std::string, title, "")

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
