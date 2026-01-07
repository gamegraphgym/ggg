#pragma once

#include "libggg/graphs/graph_concepts.hpp"
#include "libggg/graphs/validator.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

namespace ggg {
namespace graphs {
namespace probability_utilities {

/**
 * @brief Concept for graphs with probability attribute on edges
 */
template <typename GraphType>
concept HasProbabilityOnEdges = requires(const GraphType &graph) {
    requires requires(const GraphType &g, typename boost::graph_traits<GraphType>::edge_descriptor e) {
        { g[e].probability } -> std::convertible_to<double>;
    };
};

/**
 * @brief Validator for edge probabilities in stochastic games
 *
 * Validates that:
 * - All probabilities are in the range (0, 1]
 * - For vertices identified by a filter predicate, outgoing edge probabilities sum to 1.0
 *
 * @example
 * // Validate all vertices (default behavior)
 * ProbabilityValidator::validate(graph);
 *
 * // Validate only vertices where player == -1
 * auto filter = [](const Graph& g, Vertex v) { return g[v].player == -1; };
 * ProbabilityValidator::validate(graph, filter);
 */
struct ProbabilityValidator {
    template <HasProbabilityOnEdges GraphType, typename VertexFilter>
    static void validate(const GraphType &graph, VertexFilter vertex_filter) {
        using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;
        const auto [vertices_begin, vertices_end] = boost::vertices(graph);

        for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
            // Only check probabilities for vertices matching the filter
            if (vertex_filter(graph, vertex)) {
                double sum = 0.0;
                const auto [out_begin, out_end] = boost::out_edges(vertex, graph);

                for (const auto &edge : boost::make_iterator_range(out_begin, out_end)) {
                    const double prob = graph[edge].probability;
                    if (prob <= 0.0 || prob > 1.0) {
                        throw GraphValidationError(
                            "Invalid probability " + std::to_string(prob) +
                            " on edge from vertex '" + graph[vertex].name +
                            "' (must be in range (0, 1])");
                    }
                    sum += prob;
                }

                // Check that probabilities sum to 1.0 (with small epsilon for floating point)
                if (std::abs(sum - 1.0) > 1e-8) {
                    throw GraphValidationError(
                        "Probabilities on outgoing edges from vertex '" +
                        graph[vertex].name + "' sum to " + std::to_string(sum) +
                        " (must sum to 1.0)");
                }
            }
        }
    }

    // Overload with default filter that checks all vertices
    template <HasProbabilityOnEdges GraphType>
    static void validate(const GraphType &graph) {
        auto default_filter = [](const GraphType &, auto) { return true; };
        validate(graph, default_filter);
    }
};

/**
 * @brief Get the minimum probability value on any edge
 * @tparam GraphType Any graph type satisfying HasProbabilityOnEdges concept
 * @param graph The graph to analyze
 * @return Minimum probability value, or 1.0 if graph has no edges
 */
template <HasProbabilityOnEdges GraphType>
inline double get_min_probability(const GraphType &graph) {
    if (boost::num_edges(graph) == 0) {
        return 1.0;
    }

    const auto [edges_begin, edges_end] = boost::edges(graph);
    const auto min_edge = std::min_element(edges_begin, edges_end,
                                           [&graph](const auto &a, const auto &b) {
                                               return graph[a].probability < graph[b].probability;
                                           });
    return graph[*min_edge].probability;
}

/**
 * @brief Get the maximum probability value on any edge
 * @tparam GraphType Any graph type satisfying HasProbabilityOnEdges concept
 * @param graph The graph to analyze
 * @return Maximum probability value, or 0.0 if graph has no edges
 */
template <HasProbabilityOnEdges GraphType>
inline double get_max_probability(const GraphType &graph) {
    if (boost::num_edges(graph) == 0) {
        return 0.0;
    }

    const auto [edges_begin, edges_end] = boost::edges(graph);
    const auto max_edge = std::max_element(edges_begin, edges_end,
                                           [&graph](const auto &a, const auto &b) {
                                               return graph[a].probability < graph[b].probability;
                                           });
    return graph[*max_edge].probability;
}

/**
 * @brief Get distribution of probability values in the graph
 * @tparam GraphType Any graph type satisfying HasProbabilityOnEdges concept
 * @param graph The graph to analyze
 * @return Map from probability value to count of edges with that probability
 */
template <HasProbabilityOnEdges GraphType>
inline std::map<double, int> get_probability_distribution(const GraphType &graph) {
    std::map<double, int> distribution;
    const auto [edges_begin, edges_end] = boost::edges(graph);
    std::for_each(edges_begin, edges_end, [&graph, &distribution](const auto &edge) {
        distribution[graph[edge].probability]++;
    });
    return distribution;
}

} // namespace probability_utilities
} // namespace graphs
} // namespace ggg
