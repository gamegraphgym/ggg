#pragma once

#include "libggg/graphs/graph_concepts.hpp"
#include "libggg/graphs/validator.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <algorithm>
#include <map>
#include <vector>

namespace ggg {
namespace graphs {
namespace discount_utilities {

/**
 * @brief Concept for graphs with discount factor attribute on edges
 */
template <typename GraphType>
concept HasDiscountOnEdges = requires(const GraphType &graph) {
    requires requires(const GraphType &g, typename boost::graph_traits<GraphType>::edge_descriptor e) {
        { g[e].discount } -> std::convertible_to<double>;
    };
};

/**
 * @brief Validator for discount factors in discounted games
 *
 * Validates that discount factors are in the valid range (0, 1) for edges
 * originating from vertices matching a filter predicate (typically non-probabilistic vertices).
 *
 * @example
 * // Validate all vertices (default behavior)
 * DiscountValidator::validate(graph);
 *
 * // Validate discounts for non-probabilistic vertices (player != -1)
 * auto filter = [](const Graph& g, Vertex v) { return g[v].player != -1; };
 * DiscountValidator::validate(graph, filter);
 *
 * // Custom range
 * DiscountValidator::validate(graph, filter, 0.5, 0.99);
 */
struct DiscountValidator {
    template <HasDiscountOnEdges GraphType, typename VertexFilter>
    static void validate(const GraphType &graph, VertexFilter vertex_filter, 
                        double min_discount = 0.0, double max_discount = 1.0) {
        using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;
        const auto [edges_begin, edges_end] = boost::edges(graph);

        for (const auto &edge : boost::make_iterator_range(edges_begin, edges_end)) {
            auto source = boost::source(edge, graph);
            
            // Only check discount for vertices matching the filter
            if (vertex_filter(graph, source)) {
                const double discount = graph[edge].discount;
                if (discount <= min_discount || discount >= max_discount) {
                    throw GraphValidationError(
                        "Invalid discount factor " + std::to_string(discount) +
                        " on edge from vertex '" + graph[source].name +
                        "' (must be in range (" + std::to_string(min_discount) +
                        ", " + std::to_string(max_discount) + "))");
                }
            }
        }
    }

    // Overload with default filter that checks all vertices
    template <HasDiscountOnEdges GraphType>
    static void validate(const GraphType &graph, double min_discount = 0.0, double max_discount = 1.0) {
        auto default_filter = [](const GraphType &, auto) { return true; };
        validate(graph, default_filter, min_discount, max_discount);
    }
};

/**
 * @brief Get the minimum discount factor on any edge
 * @tparam GraphType Any graph type satisfying HasDiscountOnEdges concept
 * @param graph The graph to analyze
 * @return Minimum discount value, or 1.0 if graph has no edges
 */
template <HasDiscountOnEdges GraphType>
inline double get_min_discount(const GraphType &graph) {
    if (boost::num_edges(graph) == 0) {
        return 1.0;
    }

    const auto [edges_begin, edges_end] = boost::edges(graph);
    const auto min_edge = std::min_element(edges_begin, edges_end,
                                           [&graph](const auto &a, const auto &b) {
                                               return graph[a].discount < graph[b].discount;
                                           });
    return graph[*min_edge].discount;
}

/**
 * @brief Get the maximum discount factor on any edge
 * @tparam GraphType Any graph type satisfying HasDiscountOnEdges concept
 * @param graph The graph to analyze
 * @return Maximum discount value, or 0.0 if graph has no edges
 */
template <HasDiscountOnEdges GraphType>
inline double get_max_discount(const GraphType &graph) {
    if (boost::num_edges(graph) == 0) {
        return 0.0;
    }

    const auto [edges_begin, edges_end] = boost::edges(graph);
    const auto max_edge = std::max_element(edges_begin, edges_end,
                                           [&graph](const auto &a, const auto &b) {
                                               return graph[a].discount < graph[b].discount;
                                           });
    return graph[*max_edge].discount;
}

/**
 * @brief Get distribution of discount factors in the graph
 * @tparam GraphType Any graph type satisfying HasDiscountOnEdges concept
 * @param graph The graph to analyze
 * @return Map from discount value to count of edges with that discount
 */
template <HasDiscountOnEdges GraphType>
inline std::map<double, int> get_discount_distribution(const GraphType &graph) {
    std::map<double, int> distribution;
    const auto [edges_begin, edges_end] = boost::edges(graph);
    std::for_each(edges_begin, edges_end, [&graph, &distribution](const auto &edge) {
        distribution[graph[edge].discount]++;
    });
    return distribution;
}

/**
 * @brief Get all unique discount factors in the graph, sorted in ascending order
 * @tparam GraphType Any graph type satisfying HasDiscountOnEdges concept
 * @param graph The graph to analyze
 * @return Vector of unique discount values sorted from lowest to highest
 */
template <HasDiscountOnEdges GraphType>
inline std::vector<double> get_unique_discounts(const GraphType &graph) {
    std::vector<double> unique_discounts;
    auto distribution = get_discount_distribution(graph);
    unique_discounts.reserve(distribution.size());

    for (const auto &[discount, _] : distribution) {
        unique_discounts.push_back(discount);
    }
    return unique_discounts;
}

} // namespace discount_utilities
} // namespace graphs
} // namespace ggg
