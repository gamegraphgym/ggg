#pragma once
#include "libggg/graphs/graph_utilities.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace ggg {
namespace parity {
namespace graph {

// Parity graph property field lists
#define PARITY_VERTEX_FIELDS(X) \
    X(std::string, name)        \
    X(int, player)              \
    X(int, priority)

#define PARITY_EDGE_FIELDS(X) \
    X(std::string, label)

#define PARITY_GRAPH_FIELDS(X) /* none */

// Instantiate Graph/parse/write in ggg::parity::graph
DEFINE_GAME_GRAPH(PARITY_VERTEX_FIELDS, PARITY_EDGE_FIELDS, PARITY_GRAPH_FIELDS)

#undef PARITY_VERTEX_FIELDS
#undef PARITY_EDGE_FIELDS
#undef PARITY_GRAPH_FIELDS

// Utilities migrated from legacy graphs/parity_graph.hpp
inline bool is_valid(const Graph &graph) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    return std::all_of(vertices_begin, vertices_end, [&graph](const auto &vertex) {
        if (graph[vertex].player != 0 && graph[vertex].player != 1) {
            return false;
        }
        if (graph[vertex].priority < 0) {
            return false;
        }
        return boost::out_degree(vertex, graph) > 0;
    });
}

inline void check_no_duplicate_edges(const Graph &graph) {
    std::set<std::pair<Vertex, Vertex>> seen_edges;
    const auto [edges_begin, edges_end] = boost::edges(graph);
    for (const auto &edge : boost::make_iterator_range(edges_begin, edges_end)) {
        auto source = boost::source(edge, graph);
        auto target = boost::target(edge, graph);
        const auto edge_key = std::make_pair(source, target);
        if (!seen_edges.insert(edge_key).second) {
            std::string source_name = graph[source].name;
            std::string target_name = graph[target].name;
            throw std::runtime_error("Duplicate edge found between vertices '" +
                                     source_name + "' and '" + target_name + "'");
        }
    }
}

} // namespace graph
} // namespace parity
} // namespace ggg
