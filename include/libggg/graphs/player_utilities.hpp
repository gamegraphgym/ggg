#pragma once

#include "libggg/graphs/graph_concepts.hpp"
#include "libggg/graphs/validator.hpp"
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
 * @brief Validator for player values with configurable allowed players
 *
 * This validator checks that all vertices have player values from a specified set.
 * Template parameters specify which player values are valid (e.g., 0 and 1 for 2-player games).
 *
 * @tparam AllowedPlayers Variadic pack of integers representing valid player values
 *
 * Example usage:
 * @code
 * // 2-player game validator (players 0 and 1)
 * PlayerValidator<0, 1>::validate(graph);
 *
 * // 3-player game validator (players 0, 1, and 2)
 * PlayerValidator<0, 1, 2>::validate(graph);
 * @endcode
 */
template <int... AllowedPlayers>
struct PlayerValidator {
    /**
     * @brief Validate that all vertices have allowed player values
     * @tparam GraphType Graph type with player properties on vertices
     * @param graph The graph to validate
     * @throws GraphValidationError if any vertex has an invalid player value
     */
    template <HasPlayerOnVertices GraphType>
    static void validate(const GraphType &graph) {
        constexpr int allowed[] = {AllowedPlayers...};
        const auto [vertices_begin, vertices_end] = boost::vertices(graph);
        for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
            const int player = graph[vertex].player;
            const bool is_valid = std::find(std::begin(allowed), std::end(allowed), player) != std::end(allowed);
            if (!is_valid) {
                throw GraphValidationError(
                    "Invalid player value " + std::to_string(player) +
                    " for vertex '" + graph[vertex].name + "' (must be one of the allowed values)");
            }
        }
    }
};

/**
 * @brief Compute the attractor set for a player to force reaching a target set
 *
 * Computes the largest set of vertices from which @p player can force the play
 * to reach at least one vertex in @p target, regardless of the opponent's
 * strategy.  The result also contains a positional witness strategy for
 * @p player on those vertices.
 *
 * ### Algorithm
 * A standard backward BFS over the game graph:
 *
 * 1. Seed the attractor with every vertex in @p target and push them onto a
 *    worklist.
 * 2. For each vertex @c current dequeued from the worklist, inspect every
 *    predecessor via `boost::in_edges(current, graph)`:
 *    - **@p player vertex**: one edge into the attractor is enough to attract
 *      it.  Record `predecessor → current` as the strategy edge.
 *    - **Opponent vertex**: all outgoing edges must lead into the attractor
 *      (the opponent has no escape).  If so, record any successor already in
 *      the attractor as the (irrelevant) strategy edge placeholder.
 * 3. Newly attracted vertices are added to the attractor set and enqueued for
 *    further backward exploration.
 *
 * Requires a bidirectional graph (`boost::bidirectionalS`) so that
 * `boost::in_edges` is available in O(in-degree) time.
 *
 * ### Complexity
 * O(V + E) — each vertex and each edge is visited at most once.
 *
 * @tparam GraphType Any graph type satisfying @c HasPlayerOnVertices and
 *         providing bidirectional edge traversal.
 * @param graph  The game graph.
 * @param target Seed set of vertices to attract to; included in the result.
 * @param player The player whose attractor is computed (0 or 1).
 * @return A pair `{attractor, strategy}` where @c attractor is the full
 *         attractor set and @c strategy maps each attracted @p player-owned
 *         vertex to a successor inside the attractor.
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

    for (const auto &vertex : target) {
        worklist.push(vertex);
    }

    while (!worklist.empty()) {
        const auto current = worklist.front();
        worklist.pop();

        // Walk all predecessors directly — O(in_degree(current)).
        const auto [in_edges_begin, in_edges_end] = boost::in_edges(current, graph);
        for (auto edge_it = in_edges_begin; edge_it != in_edges_end; ++edge_it) {
            const auto predecessor = boost::source(*edge_it, graph);

            if (attractor.count(predecessor)) {
                continue;
            }

            bool should_add = false;

            if (graph[predecessor].player == player) {
                // Player vertex: this in-edge is sufficient to attract.
                should_add = true;
                strategy[predecessor] = current;
            } else {
                // Opponent vertex: attracted only when every successor is
                // already in the attractor (no escape route).
                const auto [out_edges_begin, out_edges_end] = boost::out_edges(predecessor, graph);
                const auto all_to_attractor = std::all_of(out_edges_begin, out_edges_end, [&](const auto &edge) {
                    return attractor.count(boost::target(edge, graph));
                });

                if (all_to_attractor && boost::out_degree(predecessor, graph) > 0) {
                    should_add = true;
                    strategy[predecessor] = boost::target(*out_edges_begin, graph);
                }
            }

            if (should_add) {
                attractor.insert(predecessor);
                worklist.push(predecessor);
            }
        }
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
