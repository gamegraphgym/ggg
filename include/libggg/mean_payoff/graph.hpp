#pragma once
#include "libggg/graphs/graph_utilities.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace ggg {
namespace mean_payoff {
namespace graph {

// Mean-payoff graph property field lists
#define MEAN_PAYOFF_VERTEX_FIELDS(X) \
    X(std::string, name)             \
    X(int, player)                   \
    X(int, weight)

#define MEAN_PAYOFF_EDGE_FIELDS(X) \
    X(std::string, label)

#define MEAN_PAYOFF_GRAPH_FIELDS(X) /* none */

// Instantiate Graph/parse/write in ggg::mean_payoff::graph
DEFINE_GAME_GRAPH(MEAN_PAYOFF_VERTEX_FIELDS, MEAN_PAYOFF_EDGE_FIELDS, MEAN_PAYOFF_GRAPH_FIELDS)

#undef MEAN_PAYOFF_VERTEX_FIELDS
#undef MEAN_PAYOFF_EDGE_FIELDS
#undef MEAN_PAYOFF_GRAPH_FIELDS

// Utilities migrated from legacy graphs/mean_payoff_graph.hpp
inline bool is_valid(const Graph &graph) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    return std::all_of(vertices_begin, vertices_end, [&graph](const auto &vertex) {
        if (graph[vertex].player != 0 && graph[vertex].player != 1) {
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
} // namespace mean_payoff
} // namespace ggg
