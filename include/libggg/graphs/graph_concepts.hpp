#pragma once

#include <boost/graph/graph_traits.hpp>
#include <concepts>

namespace ggg {
namespace graphs {

/**
 * @brief C++20 concept for graphs that have priority properties on vertices
 *
 * This concept requires that vertices in the graph have a bundled property called
 * "priority" of type int. This is commonly used in parity games where each vertex
 * has an associated priority value.
 */
template <typename GraphType>
concept HasPriorityOnVertices = requires(const GraphType &graph,
                                         typename boost::graph_traits<GraphType>::vertex_descriptor vertex) {
    { graph[vertex].priority } -> std::convertible_to<int>;
};

/**
 * @brief C++20 concept for graphs that have player properties on vertices
 *
 * This concept requires that vertices in the graph have a bundled property called
 * "player" of type int. This is used in game graphs to identify which player
 * controls each vertex (typically 0 or 1).
 */
template <typename GraphType>
concept HasPlayerOnVertices = requires(const GraphType &graph,
                                       typename boost::graph_traits<GraphType>::vertex_descriptor vertex) {
    { graph[vertex].player } -> std::convertible_to<int>;
};

/**
 * @brief C++20 concept for graphs that have label properties on vertices
 *
 * This concept requires that vertices in the graph have a bundled property called
 * "label" of type string. This is used for vertex identification and naming.
 */
template <typename GraphType>
concept HasLabelOnVertices = requires(const GraphType &graph,
                                      typename boost::graph_traits<GraphType>::vertex_descriptor vertex) {
    { graph[vertex].label } -> std::convertible_to<std::string>;
};

/**
 * @brief C++20 concept for graphs that have weight properties on vertices
 *
 * This concept requires that vertices in the graph have a bundled property called
 * "weight" of type int, as used for Mean-Payoff or Energy games.
 */
template <typename GraphType>
concept HasWeightOnVertices = requires(const GraphType &graph,
                                       typename boost::graph_traits<GraphType>::vertex_descriptor vertex) {
    { graph[vertex].weight } -> std::convertible_to<int>;
};

/**
 * @brief C++20 concept for graphs that have label properties on edges
 *
 * This concept requires that edges in the graph have a bundled property called
 * "label" of type string. This is used for edge identification and naming.
 */
template <typename GraphType>
concept HasLabelOnEdges = requires(const GraphType &graph,
                                   typename boost::graph_traits<GraphType>::edge_descriptor edge) {
    { graph[edge].label } -> std::convertible_to<std::string>;
};

/**
 * @brief C++20 concept for graphs that have discount properties on edges
 *
 * This concept requires that edges in the graph have a bundled property called
 * "discount" of type double. This is used in discounted games where each edge
 * has an associated discount factor.
 */
template <typename GraphType>
concept HasDiscountOnEdges = requires(const GraphType &graph,
                                      typename boost::graph_traits<GraphType>::edge_descriptor edge) {
    { graph[edge].discount } -> std::convertible_to<double>;
};

} // namespace graphs
} // namespace ggg
