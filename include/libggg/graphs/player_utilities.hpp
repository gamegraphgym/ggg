#pragma once

#include "libggg/graphs/graph_concepts.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <vector>

namespace ggg {
namespace graphs {
namespace player_utilities {

/**
 * @brief Compute the attractor set for a player to force reaching a target set
 *
 * This function computes the set of vertices from which a given player can
 * force the game to reach at least one vertex in the target set, regardless
 * of the opponent's strategy. The algorithm uses a fixed-point computation:
 *
 * - For player vertices: Add if there exists at least one edge to the current attractor
 * - For opponent vertices: Add if ALL outgoing edges lead to the current attractor
 *
 * @tparam GraphType Any graph type satisfying HasPlayerOnVertices concept
 * @param graph The game graph
 * @param target The target set of vertices to attract to
 * @param player The player computing the attractor (typically 0 or 1)
 * @return Pair containing the attractor set and a strategy mapping for the player
 */
template <HasPlayerOnVertices GraphType>
inline std::pair<std::set<typename boost::graph_traits<GraphType>::vertex_descriptor>,
                 std::map<typename boost::graph_traits<GraphType>::vertex_descriptor,
                          typename boost::graph_traits<GraphType>::vertex_descriptor>>
compute_attractor(const GraphType &graph,
                  const std::set<typename boost::graph_traits<GraphType>::vertex_descriptor> &target,
                  int player) {
    using VertexDescriptor = typename boost::graph_traits<GraphType>::vertex_descriptor;

    std::set<VertexDescriptor> attractor = target;
    std::map<VertexDescriptor, VertexDescriptor> strategy;
    std::queue<VertexDescriptor> worklist;

    // Initialize worklist with target vertices
    for (const auto &vertex : target) {
        worklist.push(vertex);
    }

    while (!worklist.empty()) {
        const auto current = worklist.front();
        worklist.pop();

        // Check all vertices as potential predecessors
        const auto [vertices_begin, vertices_end] = boost::vertices(graph);
        std::for_each(vertices_begin, vertices_end, [&](const auto &predecessor) {
            // Skip if already in attractor
            if (attractor.count(predecessor)) {
                return;
            }

            // Check if predecessor has edge to current
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(predecessor, graph);
            const auto has_edge_to_current = std::any_of(out_edges_begin, out_edges_end, [&](const auto &edge) {
                return boost::target(edge, graph) == current;
            });

            if (!has_edge_to_current) {
                return;
            }

            // Check attractor condition
            bool should_add = false;

            if (graph[predecessor].player == player) {
                // Player vertex: add if has edge to attractor
                should_add = true;
                strategy[predecessor] = current;
            } else {
                // Opponent vertex: add if all edges lead to attractor
                const auto all_to_attractor = std::all_of(out_edges_begin, out_edges_end, [&](const auto &edge) {
                    const auto target_vertex = boost::target(edge, graph);
                    return attractor.count(target_vertex);
                });

                if (all_to_attractor && boost::out_degree(predecessor, graph) > 0) {
                    should_add = true;
                    // For opponent, strategy can be any edge to attractor
                    strategy[predecessor] = boost::target(*out_edges_begin, graph);
                }
            }

            if (should_add) {
                attractor.insert(predecessor);
                worklist.push(predecessor);
            }
        });
    }

    return {attractor, strategy};
}

/**
 * @brief Get all vertices controlled by a specific player
 * @tparam GraphType Any graph type satisfying HasPlayerOnVertices concept
 * @param graph The graph to search
 * @param player Player to search for (typically 0 or 1)
 * @return Vector of vertices controlled by the given player
 */
template <HasPlayerOnVertices GraphType>
inline std::vector<typename boost::graph_traits<GraphType>::vertex_descriptor>
get_vertices_by_player(const GraphType &graph, int player) {
    using VertexDescriptor = typename boost::graph_traits<GraphType>::vertex_descriptor;
    std::vector<VertexDescriptor> result;

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    std::copy_if(vertices_begin, vertices_end, std::back_inserter(result),
                 [&graph, player](const auto &vertex) {
                     return graph[vertex].player == player;
                 });
    return result;
}

/**
 * @brief Get distribution of vertices by player
 * @tparam GraphType Any graph type satisfying HasPlayerOnVertices concept
 * @param graph The graph to analyze
 * @return Map from player ID to count of vertices controlled by that player
 */
template <HasPlayerOnVertices GraphType>
inline std::map<int, int> get_player_distribution(const GraphType &graph) {
    std::map<int, int> distribution;
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    std::for_each(vertices_begin, vertices_end, [&graph, &distribution](const auto &vertex) {
        distribution[graph[vertex].player]++;
    });
    return distribution;
}

/**
 * @brief Get all unique player IDs in the graph
 * @tparam GraphType Any graph type satisfying HasPlayerOnVertices concept
 * @param graph The graph to analyze
 * @return Vector of unique player IDs sorted in ascending order
 */
template <HasPlayerOnVertices GraphType>
inline std::vector<int> get_unique_players(const GraphType &graph) {
    std::vector<int> unique_players;
    auto distribution = get_player_distribution(graph);
    unique_players.reserve(distribution.size());

    for (const auto &[player, _] : distribution) {
        unique_players.push_back(player);
    }
    return unique_players;
}

} // namespace player_utilities
} // namespace graphs
} // namespace ggg
