#pragma once

#include "libggg/graphs/graph_concepts.hpp"
#include "libggg/graphs/validator.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <algorithm>
#include <limits>
#include <map>
#include <vector>

namespace ggg {
namespace graphs {
namespace weight_utilities {

/**
 * @brief Concept for graphs with weight attribute on edges
 */
template <typename GraphType>
concept HasWeightOnEdges = requires(const GraphType &graph) {
    requires requires(const GraphType &g, typename boost::graph_traits<GraphType>::edge_descriptor e) {
        { g[e].weight } -> std::convertible_to<double>;
    };
};

/**
 * @brief Concept for graphs with weight attribute on vertices
 */
template <typename GraphType>
concept HasWeightOnVertices = requires(const GraphType &graph) {
    requires requires(const GraphType &g, typename boost::graph_traits<GraphType>::vertex_descriptor v) {
        { g[v].weight } -> std::convertible_to<double>;
    };
};

/**
 * @brief Validator for edge weights with configurable bounds
 *
 * @tparam MinWeight Minimum allowed weight (default: -infinity)
 * @tparam MaxWeight Maximum allowed weight (default: +infinity)
 */
template <double MinWeight = -std::numeric_limits<double>::infinity(),
          double MaxWeight = std::numeric_limits<double>::infinity()>
struct EdgeWeightValidator {
    template <HasWeightOnEdges GraphType>
    static void validate(const GraphType &graph) {
        const auto [edges_begin, edges_end] = boost::edges(graph);

        for (const auto &edge : boost::make_iterator_range(edges_begin, edges_end)) {
            const double weight = graph[edge].weight;
            auto source = boost::source(edge, graph);
            auto target = boost::target(edge, graph);

            if (weight < MinWeight || weight > MaxWeight) {
                throw GraphValidationError(
                    "Invalid weight " + std::to_string(weight) +
                    " on edge from '" + graph[source].name + "' to '" +
                    graph[target].name + "' (must be in range [" +
                    std::to_string(MinWeight) + ", " + std::to_string(MaxWeight) + "])");
            }
        }
    }
};

/**
 * @brief Validator for vertex weights with configurable bounds
 *
 * @tparam MinWeight Minimum allowed weight (default: -infinity)
 * @tparam MaxWeight Maximum allowed weight (default: +infinity)
 */
template <double MinWeight = -std::numeric_limits<double>::infinity(),
          double MaxWeight = std::numeric_limits<double>::infinity()>
struct VertexWeightValidator {
    template <HasWeightOnVertices GraphType>
    static void validate(const GraphType &graph) {
        const auto [vertices_begin, vertices_end] = boost::vertices(graph);

        for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
            const double weight = graph[vertex].weight;

            if (weight < MinWeight || weight > MaxWeight) {
                throw GraphValidationError(
                    "Invalid weight " + std::to_string(weight) +
                    " on vertex '" + graph[vertex].name + "' (must be in range [" +
                    std::to_string(MinWeight) + ", " + std::to_string(MaxWeight) + "])");
            }
        }
    }
};

/**
 * @brief Get the minimum weight on edges
 * @tparam GraphType Any graph type satisfying HasWeightOnEdges concept
 * @param graph The graph to analyze
 * @return Minimum weight value, or 0.0 if graph has no edges
 */
template <HasWeightOnEdges GraphType>
inline double get_min_edge_weight(const GraphType &graph) {
    if (boost::num_edges(graph) == 0) {
        return 0.0;
    }

    const auto [edges_begin, edges_end] = boost::edges(graph);
    const auto min_edge = std::min_element(edges_begin, edges_end,
                                           [&graph](const auto &a, const auto &b) {
                                               return graph[a].weight < graph[b].weight;
                                           });
    return graph[*min_edge].weight;
}

/**
 * @brief Get the maximum weight on edges
 * @tparam GraphType Any graph type satisfying HasWeightOnEdges concept
 * @param graph The graph to analyze
 * @return Maximum weight value, or 0.0 if graph has no edges
 */
template <HasWeightOnEdges GraphType>
inline double get_max_edge_weight(const GraphType &graph) {
    if (boost::num_edges(graph) == 0) {
        return 0.0;
    }

    const auto [edges_begin, edges_end] = boost::edges(graph);
    const auto max_edge = std::max_element(edges_begin, edges_end,
                                           [&graph](const auto &a, const auto &b) {
                                               return graph[a].weight < graph[b].weight;
                                           });
    return graph[*max_edge].weight;
}

/**
 * @brief Get distribution of edge weights in the graph
 * @tparam GraphType Any graph type satisfying HasWeightOnEdges concept
 * @param graph The graph to analyze
 * @return Map from weight value to count of edges with that weight
 */
template <HasWeightOnEdges GraphType>
inline std::map<double, int> get_edge_weight_distribution(const GraphType &graph) {
    std::map<double, int> distribution;
    const auto [edges_begin, edges_end] = boost::edges(graph);
    std::for_each(edges_begin, edges_end, [&graph, &distribution](const auto &edge) {
        distribution[graph[edge].weight]++;
    });
    return distribution;
}

/**
 * @brief Get the minimum weight on vertices
 * @tparam GraphType Any graph type satisfying HasWeightOnVertices concept
 * @param graph The graph to analyze
 * @return Minimum weight value, or 0.0 if graph has no vertices
 */
template <HasWeightOnVertices GraphType>
inline double get_min_vertex_weight(const GraphType &graph) {
    if (boost::num_vertices(graph) == 0) {
        return 0.0;
    }

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    const auto min_vertex = std::min_element(vertices_begin, vertices_end,
                                             [&graph](const auto &a, const auto &b) {
                                                 return graph[a].weight < graph[b].weight;
                                             });
    return graph[*min_vertex].weight;
}

/**
 * @brief Get the maximum weight on vertices
 * @tparam GraphType Any graph type satisfying HasWeightOnVertices concept
 * @param graph The graph to analyze
 * @return Maximum weight value, or 0.0 if graph has no vertices
 */
template <HasWeightOnVertices GraphType>
inline double get_max_vertex_weight(const GraphType &graph) {
    if (boost::num_vertices(graph) == 0) {
        return 0.0;
    }

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    const auto max_vertex = std::max_element(vertices_begin, vertices_end,
                                             [&graph](const auto &a, const auto &b) {
                                                 return graph[a].weight < graph[b].weight;
                                             });
    return graph[*max_vertex].weight;
}

/**
 * @brief Get distribution of vertex weights in the graph
 * @tparam GraphType Any graph type satisfying HasWeightOnVertices concept
 * @param graph The graph to analyze
 * @return Map from weight value to count of vertices with that weight
 */
template <HasWeightOnVertices GraphType>
inline std::map<double, int> get_vertex_weight_distribution(const GraphType &graph) {
    std::map<double, int> distribution;
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    std::for_each(vertices_begin, vertices_end, [&graph, &distribution](const auto &vertex) {
        distribution[graph[vertex].weight]++;
    });
    return distribution;
}

} // namespace weight_utilities
} // namespace graphs
} // namespace ggg
