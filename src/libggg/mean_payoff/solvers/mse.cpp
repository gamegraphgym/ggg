#include "libggg/mean_payoff/solvers/mse.hpp"
#include "libggg/utils/logging.hpp"
#include <map>
#include <queue>

namespace ggg {
namespace mean_payoff {

SolutionType MSESolver::solve(const graph::Graph &graph) {
    LGG_DEBUG("Mean payoff MSE solver starting with ", boost::num_vertices(graph), " vertices");

    // Initialize solution
    SolutionType solution;

    // Base case: empty game
    if (boost::num_vertices(graph) == 0) {
        LGG_TRACE("Empty game - returning solved");

        return solution;
    }

    // Algorithm state variables
    int iterations = 0;
    int lifts = 0;
    int limit = 0;

    std::vector<graph::Graph::vertex_descriptor> current_strategy(
        boost::num_vertices(graph),
        boost::graph_traits<graph::Graph>::null_vertex());
    std::vector<int> current_cost(boost::num_vertices(graph), 0);
    std::vector<int> current_count(boost::num_vertices(graph), 0);
    std::queue<graph::Graph::vertex_descriptor> t_atr;
    std::vector<bool> b_atr(boost::num_vertices(graph), false);

    // Map vertex to appropriate index for above vectors and queue
    std::map<graph::Graph::vertex_descriptor, int> vertex_map;
    std::vector<graph::Graph::vertex_descriptor> index_to_vertex(boost::num_vertices(graph));

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    int vertex_count = 0;

    // Initialize vertex mappings and initial state
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        vertex_map[vertex] = vertex_count;
        index_to_vertex[vertex_count] = vertex;
        vertex_count++;

        const auto weight = graph[vertex].weight;

        if (weight > 0) {
            t_atr.push(vertex);
            b_atr[vertex_map[vertex]] = true;
            limit += weight;
        } else {
            if (graph[vertex].player) {
                current_count[vertex_map[vertex]] = boost::out_degree(vertex, graph);
            }
        }
    }
    limit += 1;

    // Main solution cycle
    while (!t_atr.empty()) {
        iterations++;
        const auto pos = t_atr.front();
        t_atr.pop();
        b_atr[vertex_map[pos]] = false;
        int old_cost = current_cost[vertex_map[pos]];
        graph::Graph::vertex_descriptor best_successor =
            boost::graph_traits<graph::Graph>::null_vertex();

        if (graph[pos].player) {
            // Player 1 minimizer
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(pos, graph);
            for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto successor = boost::target(edge, graph);
                if (best_successor == boost::graph_traits<graph::Graph>::null_vertex()) {
                    best_successor = successor;
                    current_count[vertex_map[pos]] = 1;
                } else {
                    // Compare with current best successor
                    if (current_cost[vertex_map[best_successor]] > current_cost[vertex_map[successor]]) {
                        best_successor = successor;
                        current_count[vertex_map[pos]] = 1;
                    } else if (current_cost[vertex_map[best_successor]] == current_cost[vertex_map[successor]]) {
                        current_count[vertex_map[pos]]++;
                    }
                }
            }

            if (current_cost[vertex_map[best_successor]] >= limit) {
                current_count[vertex_map[pos]] = 0;
                lifts++;
                current_cost[vertex_map[pos]] = limit;
            } else {
                int sum = current_cost[vertex_map[best_successor]] + graph[pos].weight;
                if (sum >= limit) {
                    sum = limit;
                }
                if (current_cost[vertex_map[pos]] < sum) {
                    lifts++;
                    current_cost[vertex_map[pos]] = sum;
                }
            }
        } else {
            // Player 0 maximizer
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(pos, graph);
            for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto successor = boost::target(edge, graph);
                if (best_successor == boost::graph_traits<graph::Graph>::null_vertex()) {
                    best_successor = successor;
                } else {
                    if (current_cost[vertex_map[best_successor]] < current_cost[vertex_map[successor]]) {
                        best_successor = successor;
                    }
                }
            }

            if (current_cost[vertex_map[best_successor]] >= limit) {
                lifts++;
                current_cost[vertex_map[pos]] = limit;
                current_strategy[vertex_map[pos]] = best_successor;
            } else {
                int sum = current_cost[vertex_map[best_successor]] + graph[pos].weight;
                if (sum >= limit) {
                    sum = limit;
                }
                if (current_cost[vertex_map[pos]] < sum) {
                    lifts++;
                    current_cost[vertex_map[pos]] = sum;
                    current_strategy[vertex_map[pos]] = best_successor;
                }
            }
        }

        // Process predecessors by manually finding incoming edges
        const auto [all_vertices_begin, all_vertices_end] = boost::vertices(graph);
        for (const auto &other_vertex : boost::make_iterator_range(all_vertices_begin, all_vertices_end)) {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(other_vertex, graph);
            for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                if (boost::target(edge, graph) == pos) {
                    // Found a predecessor
                    const auto predecessor = other_vertex;
                    if (!b_atr[vertex_map[predecessor]] &&
                        (current_cost[vertex_map[predecessor]] < limit) &&
                        ((current_cost[vertex_map[pos]] == limit) ||
                         (current_cost[vertex_map[predecessor]] <
                          current_cost[vertex_map[pos]] + graph[predecessor].weight))) {

                        if (graph[predecessor].player) {
                            if (current_cost[vertex_map[predecessor]] >= old_cost + graph[predecessor].weight) {
                                current_count[vertex_map[predecessor]]--;
                            }
                            if (current_count[vertex_map[predecessor]] <= 0) {
                                t_atr.push(predecessor);
                                b_atr[vertex_map[predecessor]] = true;
                            }
                        } else {
                            t_atr.push(predecessor);
                            b_atr[vertex_map[predecessor]] = true;
                        }
                    }
                }
            }
        }
    }

    // Set the final solution
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        // Set the quantitative value (currentCost as int)
        solution.set_value(vertex, current_cost[vertex_map[vertex]]);

        if (current_cost[vertex_map[vertex]] >= limit) {
            solution.set_winning_player(vertex, 0);
        } else {
            solution.set_winning_player(vertex, 1);

            if (graph[vertex].player) {
                const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
                for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                    const auto successor = boost::target(edge, graph);
                    if (current_cost[vertex_map[successor]] == 0 ||
                        current_cost[vertex_map[vertex]] >=
                            current_cost[vertex_map[successor]] + graph[vertex].weight) {
                        current_strategy[vertex_map[vertex]] = successor;
                        break;
                    }
                }
            }
        }

        // Only set strategy if it's a valid vertex
        if (current_strategy[vertex_map[vertex]] != boost::graph_traits<graph::Graph>::null_vertex()) {
            solution.set_strategy(vertex, current_strategy[vertex_map[vertex]]);
        }
    }

    // Game finished, log trace and return solution
    LGG_TRACE("Solved with ", iterations, " iterations");
    LGG_TRACE("Solved with ", lifts, " lifts");

    return solution;
}

} // namespace mean_payoff
} // namespace ggg
