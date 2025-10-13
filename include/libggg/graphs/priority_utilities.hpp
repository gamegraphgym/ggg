#pragma once

#include "libggg/graphs/graph_concepts.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <algorithm>
#include <map>
#include <vector>

namespace ggg {
namespace graphs {
namespace priority_utilities {

/**
 * @brief Get all vertices with a specific priority from any graph with priority properties
 * @tparam GraphType Any graph type satisfying HasPriorityOnVertices concept
 * @param graph The graph to search
 * @param priority Priority value to search for
 * @return Vector of vertices with the given priority
 */
template <HasPriorityOnVertices GraphType>
inline std::vector<typename boost::graph_traits<GraphType>::vertex_descriptor>
get_vertices_with_priority(const GraphType &graph, int priority) {
    using VertexDescriptor = typename boost::graph_traits<GraphType>::vertex_descriptor;
    std::vector<VertexDescriptor> result;

    const auto [VERTICES_BEGIN, VERTICES_END] = boost::vertices(graph);
    std::copy_if(VERTICES_BEGIN, VERTICES_END, std::back_inserter(result),
                 [&graph, priority](const auto &vertex) {
                     return graph[vertex].priority == priority;
                 });
    return result;
}

/**
 * @brief Get the maximum priority of any vertex
 * @tparam GraphType Any graph type satisfying HasPriorityOnVertices concept
 * @param graph The graph to analyze
 * @return Maximum priority value, or 0 if graph is empty
 */
template <HasPriorityOnVertices GraphType>
inline int get_max_priority(const GraphType &graph) {
    if (boost::num_vertices(graph) == 0) {
        return 0;
    }

    const auto [VERTICES_BEGIN, VERTICES_END] = boost::vertices(graph);
    const auto max_vertex = std::max_element(VERTICES_BEGIN, VERTICES_END,
                                             [&graph](const auto &a, const auto &b) {
                                                 return graph[a].priority < graph[b].priority;
                                             });
    return graph[*max_vertex].priority;
}

/**
 * @brief Get the minimum priority of any vertex
 * @tparam GraphType Any graph type satisfying HasPriorityOnVertices concept
 * @param graph The graph to analyze
 * @return Minimum priority value, or 0 if graph is empty
 */
template <HasPriorityOnVertices GraphType>
inline int get_min_priority(const GraphType &graph) {
    if (boost::num_vertices(graph) == 0) {
        return 0;
    }

    const auto [VERTICES_BEGIN, VERTICES_END] = boost::vertices(graph);
    const auto min_vertex = std::min_element(VERTICES_BEGIN, VERTICES_END,
                                             [&graph](const auto &a, const auto &b) {
                                                 return graph[a].priority < graph[b].priority;
                                             });
    return graph[*min_vertex].priority;
}

/**
 * @brief Get distribution of priority values in the graph
 * @tparam GraphType Any graph type satisfying HasPriorityOnVertices concept
 * @param graph The graph to analyze
 * @return Map from priority value to count of vertices with that priority
 */
template <HasPriorityOnVertices GraphType>
inline std::map<int, int> get_priority_distribution(const GraphType &graph) {
    std::map<int, int> distribution;
    const auto [VERTICES_BEGIN, VERTICES_END] = boost::vertices(graph);
    std::for_each(VERTICES_BEGIN, VERTICES_END, [&graph, &distribution](const auto &vertex) {
        distribution[graph[vertex].priority]++;
    });
    return distribution;
}

/**
 * @brief Get all unique priority values in the graph, sorted in ascending order
 * @tparam GraphType Any graph type satisfying HasPriorityOnVertices concept
 * @param graph The graph to analyze
 * @return Vector of unique priority values sorted from lowest to highest
 */
template <HasPriorityOnVertices GraphType>
inline std::vector<int> get_unique_priorities(const GraphType &graph) {
    std::vector<int> unique_priorities;
    auto distribution = get_priority_distribution(graph);
    unique_priorities.reserve(distribution.size());

    for (const auto &[priority, _] : distribution) {
        unique_priorities.push_back(priority);
    }
    return unique_priorities;
}

/**
 * @brief Compress priority values in the given graph in-place
 *
 * This function modifies the input graph by compressing priority values while
 * preserving both the relative order and the parity of priorities. The algorithm
 * ensures that:
 * 1. Odd priorities always map to odd compressed priorities
 * 2. Even priorities always map to even compressed priorities
 * 3. Consecutive priorities with the same parity are collapsed when there's
 *    no priority of the opposite parity between them
 * 4. The relative order of all priorities is preserved
 * 5. The parity of the least number is preserved
 *
 * For example, priorities {7, 9, 10, 15, 22} compress to preserve parity:
 * - 7 (odd, minimum) -> 1 (first odd compressed priority, preserving minimum parity)
 * - 9 (odd, consecutive with 7, no even in between) -> 1 (collapsed with 7)
 * - 10 (even) -> 2 (first even compressed priority)
 * - 15 (odd) -> 3 (next odd, since even 10 intervened after 7/9)
 * - 22 (even) -> 4 (next even, since odd 15 intervened after 10)
 *
 * @tparam GraphType Any graph type satisfying HasPriorityOnVertices concept
 * @param graph The graph to modify in-place. Priority values will be updated.
 */
template <HasPriorityOnVertices GraphType>
inline void compress_priorities(GraphType &graph) {
    // Get all unique priorities and sort them
    auto unique_priorities = get_unique_priorities(graph);

    if (unique_priorities.empty()) {
        return; // Nothing to compress
    }

    // Determine starting values based on the parity of the minimum priority
    int min_priority = unique_priorities[0];
    int min_parity = min_priority % 2;

    // Start compressed priorities to preserve the parity of the minimum
    int next_even, next_odd;
    if (min_parity == 0) { // Minimum is even
        next_even = 0;     // Start even sequence at 0
        next_odd = 1;      // Start odd sequence at 1
    } else {               // Minimum is odd
        next_even = 2;     // Start even sequence at 2
        next_odd = 1;      // Start odd sequence at 1
    }

    // Create the compression mapping with parity preservation
    std::map<int, int> mapping;
    int current_even = -1; // Current even priority being used
    int current_odd = -1;  // Current odd priority being used
    int last_parity = -1;  // Parity of the last processed priority

    for (int priority : unique_priorities) {
        int parity = priority % 2; // 0 for even, 1 for odd

        if (parity == 0) { // Even priority
            // Use new even priority if this is the first even or if we had an odd priority since last even
            if (current_even == -1 || last_parity == 1) {
                current_even = next_even;
                next_even += 2; // Next even number
            }
            mapping[priority] = current_even;
        } else { // Odd priority
            // Use new odd priority if this is the first odd or if we had an even priority since last odd
            if (current_odd == -1 || last_parity == 0) {
                current_odd = next_odd;
                next_odd += 2; // Next odd number
            }
            mapping[priority] = current_odd;
        }

        last_parity = parity;
    }

    // Update all vertex priorities in-place
    const auto [VERTICES_BEGIN, VERTICES_END] = boost::vertices(graph);
    for (auto vertex_it = VERTICES_BEGIN; vertex_it != VERTICES_END; ++vertex_it) {
        const auto &vertex = *vertex_it;
        int old_priority = graph[vertex].priority;
        graph[vertex].priority = mapping[old_priority];
    }
}

/**
 * @brief Get vertices sorted by priority in ascending order
 * @tparam GraphType Any graph type satisfying HasPriorityOnVertices concept
 * @param graph The graph to analyze
 * @return Vector of vertices sorted by priority (lowest to highest)
 * @complexity O(V log V) where V is the number of vertices
 */
template <HasPriorityOnVertices GraphType>
inline std::vector<typename boost::graph_traits<GraphType>::vertex_descriptor>
get_vertices_by_priority_ascending(const GraphType &graph) {
    using VertexDescriptor = typename boost::graph_traits<GraphType>::vertex_descriptor;
    std::vector<VertexDescriptor> vertices;

    const auto [VERTICES_BEGIN, VERTICES_END] = boost::vertices(graph);

    // Collect all vertices
    for (const auto &vertex : boost::make_iterator_range(VERTICES_BEGIN, VERTICES_END)) {
        vertices.push_back(vertex);
    }

    // Sort by priority (ascending)
    std::sort(vertices.begin(), vertices.end(),
              [&graph](const VertexDescriptor &a, const VertexDescriptor &b) {
                  return graph[a].priority < graph[b].priority;
              });

    return vertices;
}

/**
 * @brief Get vertices sorted by priority in descending order
 * @tparam GraphType Any graph type satisfying HasPriorityOnVertices concept
 * @param graph The graph to analyze
 * @return Vector of vertices sorted by priority (highest to lowest)
 * @complexity O(V log V) where V is the number of vertices
 */
template <HasPriorityOnVertices GraphType>
inline std::vector<typename boost::graph_traits<GraphType>::vertex_descriptor>
get_vertices_by_priority_descending(const GraphType &graph) {
    using VertexDescriptor = typename boost::graph_traits<GraphType>::vertex_descriptor;
    std::vector<VertexDescriptor> vertices;

    const auto [VERTICES_BEGIN, VERTICES_END] = boost::vertices(graph);

    // Collect all vertices
    for (const auto &vertex : boost::make_iterator_range(VERTICES_BEGIN, VERTICES_END)) {
        vertices.push_back(vertex);
    }

    // Sort by priority (descending)
    std::sort(vertices.begin(), vertices.end(),
              [&graph](const VertexDescriptor &a, const VertexDescriptor &b) {
                  return graph[a].priority > graph[b].priority;
              });

    return vertices;
}

/**
 * @brief Get vertices grouped by priority level
 * @tparam GraphType Any graph type satisfying HasPriorityOnVertices concept
 * @param graph The graph to analyze
 * @return Map from priority to vector of vertices with that priority
 * @complexity O(V) where V is the number of vertices
 */
template <HasPriorityOnVertices GraphType>
inline std::map<int, std::vector<typename boost::graph_traits<GraphType>::vertex_descriptor>>
get_vertices_grouped_by_priority(const GraphType &graph) {
    using VertexDescriptor = typename boost::graph_traits<GraphType>::vertex_descriptor;
    std::map<int, std::vector<VertexDescriptor>> priority_groups;

    const auto [VERTICES_BEGIN, VERTICES_END] = boost::vertices(graph);

    // Group vertices by priority
    for (const auto &vertex : boost::make_iterator_range(VERTICES_BEGIN, VERTICES_END)) {
        const int prio = graph[vertex].priority;
        priority_groups[prio].push_back(vertex);
    }

    return priority_groups;
}

} // namespace priority_utilities
} // namespace graphs
} // namespace ggg
