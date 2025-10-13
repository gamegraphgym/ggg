#include "libggg/buechi/solvers/attractor.hpp"
#include "libggg/graphs/player_utilities.hpp"
#include "libggg/graphs/priority_utilities.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <algorithm>
#include <map>
#include <optional>
#include <set>

namespace ggg {
namespace buechi {

ggg::solutions::RSSolution<ggg::parity::graph::Graph> AttractorSolver::solve(const ggg::parity::graph::Graph &graph) {

    LGG_DEBUG("Buechi solver starting with ", boost::num_vertices(graph), " vertices");
    ggg::solutions::RSSolution<ggg::parity::graph::Graph> solution;

    if (!validate_buchi_game(graph)) {
        LGG_ERROR("Invalid Buechi game: priorities must be 0 or 1");

        return solution;
    }

    if (boost::num_vertices(graph) == 0) {
        LGG_TRACE("Empty game - returning solved");

        return solution;
    }

    iterations = 0;
    attractions = 0;

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    std::set<ggg::parity::graph::Vertex> current_active(vertices_begin, vertices_end);
    const auto target_vertices = get_buchi_accepting_vertices(graph);

    LGG_TRACE("Found ", target_vertices.size(), " Buechi accepting vertices (priority 1)");

    while (!current_active.empty()) {
        iterations++;
        std::set<ggg::parity::graph::Vertex> p1_attractor = compute_attractor(graph, current_active, 1, target_vertices);

        LGG_TRACE("Player 1 attractor to targets has ", p1_attractor.size(), " vertices");

        std::set<ggg::parity::graph::Vertex> p0_target = compute_complement(current_active, p1_attractor);

        if (p0_target.empty()) {
            LGG_TRACE("No complement - Player 1 wins remaining ", current_active.size(), " vertices");
            for (const auto &curr_active_out : current_active) {
                solution.set_winning_player(curr_active_out, 1);
            }
            break;
        }

        std::set<ggg::parity::graph::Vertex> p0_attractor = compute_attractor(graph, current_active, 0, p0_target);

        LGG_TRACE("Player 0 attractor to complement has ", p0_attractor.size(), " vertices");

        for (const auto &curr_attr_out : p0_attractor) {
            solution.set_winning_player(curr_attr_out, 0);
        }

        std::set<ggg::parity::graph::Vertex> new_active;
        for (const auto &curr_active_out : current_active) {
            if (p0_attractor.find(curr_active_out) == p0_attractor.end()) {
                new_active.insert(curr_active_out);
            }
        }

        current_active = new_active;
    }

    std::map<ggg::parity::graph::Vertex, ggg::parity::graph::Vertex> strategy;

    const auto [all_vertices_begin, all_vertices_end] = boost::vertices(graph);
    for (auto it = all_vertices_begin; it != all_vertices_end; ++it) {
        auto curr_vertex = *it;

        if (solution.get_winning_player(curr_vertex) == 0 && graph[curr_vertex].player == 0) {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(curr_vertex, graph);

            for (auto edge_it = out_edges_begin; edge_it != out_edges_end; ++edge_it) {
                auto target = boost::target(*edge_it, graph);
                if (solution.get_winning_player(target) == 0) {
                    strategy[curr_vertex] = target;
                    break;
                }
            }

            if (strategy.find(curr_vertex) == strategy.end() && out_edges_begin != out_edges_end) {
                strategy[curr_vertex] = boost::target(*out_edges_begin, graph);
            }
        } else if (solution.get_winning_player(curr_vertex) == 1 && graph[curr_vertex].player == 1) {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(curr_vertex, graph);

            for (auto edge_it = out_edges_begin; edge_it != out_edges_end; ++edge_it) {
                auto target = boost::target(*edge_it, graph);
                if (solution.get_winning_player(target) == 1) {
                    strategy[curr_vertex] = target;
                    break;
                }
            }

            if (strategy.find(curr_vertex) == strategy.end() && out_edges_begin != out_edges_end) {
                strategy[curr_vertex] = boost::target(*out_edges_begin, graph);
            }
        }
    }

    for (const auto &[from, to] : strategy) {
        solution.set_strategy(from, to);
    }

    const auto player_0_wins = std::count_if(all_vertices_begin, all_vertices_end,
                                             [&solution](const auto &curr_vertex) {
                                                 return solution.get_winning_player(curr_vertex) == 0;
                                             });
    const auto player_1_wins = std::count_if(all_vertices_begin, all_vertices_end,
                                             [&solution](const auto &curr_vertex) {
                                                 return solution.get_winning_player(curr_vertex) == 1;
                                             });

    LGG_DEBUG("Buechi game solved: Player 0 wins ", player_0_wins, " vertices, Player 1 wins ", player_1_wins, " vertices");

    LGG_TRACE("Solved with ", iterations, " iterations");
    LGG_TRACE("Solved with ", attractions, " attractions");

    return solution;
}

std::set<ggg::parity::graph::Vertex> AttractorSolver::compute_attractor(const ggg::parity::graph::Graph &graph,
                                                                        const std::set<ggg::parity::graph::Vertex> &active_vertices,
                                                                        int curr_player,
                                                                        const std::set<ggg::parity::graph::Vertex> &curr_target) {

    std::set<ggg::parity::graph::Vertex> attractor = curr_target;

    std::set<ggg::parity::graph::Vertex> filtered_target;
    std::set_intersection(curr_target.begin(), curr_target.end(),
                          active_vertices.begin(), active_vertices.end(),
                          std::inserter(filtered_target, filtered_target.begin()));
    attractor = filtered_target;

    if (attractor.size() >= active_vertices.size() || attractor.empty()) {
        return attractor;
    }

    bool changed = true;
    while (changed && attractor.size() < active_vertices.size()) {
        changed = false;

        for (const auto &curr_vertex : active_vertices) {
            if (attractor.find(curr_vertex) != attractor.end()) {
                continue;
            }

            int count = 0;
            int total_successors = 0;
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(curr_vertex, graph);

            for (auto edge_it = out_edges_begin; edge_it != out_edges_end; ++edge_it) {
                auto target = boost::target(*edge_it, graph);
                if (active_vertices.find(target) != active_vertices.end()) {
                    total_successors++;
                    if (attractor.find(target) != attractor.end()) {
                        count++;
                    }
                }
            }

            if (graph[curr_vertex].player == curr_player && count > 0) {
                attractor.insert(curr_vertex);
                attractions++;
                changed = true;
            } else if (graph[curr_vertex].player != curr_player && count == total_successors && total_successors > 0) {
                attractor.insert(curr_vertex);
                attractions++;
                changed = true;
            }
        }
    }

    return attractor;
}

bool AttractorSolver::validate_buchi_game(const ggg::parity::graph::Graph &graph) const {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    return std::all_of(vertices_begin, vertices_end, [&graph](const auto &curr_vertex) {
        int priority = graph[curr_vertex].priority;
        return priority == 0 || priority == 1;
    });
}

std::set<ggg::parity::graph::Vertex> AttractorSolver::get_buchi_accepting_vertices(const ggg::parity::graph::Graph &graph) const {
    auto vertices_vector = graphs::priority_utilities::get_vertices_with_priority(graph, 1);
    return std::set<ggg::parity::graph::Vertex>(vertices_vector.begin(), vertices_vector.end());
}

std::set<ggg::parity::graph::Vertex> AttractorSolver::compute_complement(const std::set<ggg::parity::graph::Vertex> &active_vertices,
                                                                         const std::set<ggg::parity::graph::Vertex> &inactive_vertices) const {
    std::set<ggg::parity::graph::Vertex> complement;

    for (const auto &curr_vertex : active_vertices) {
        if (inactive_vertices.find(curr_vertex) == inactive_vertices.end()) {
            complement.insert(curr_vertex);
        }
    }

    return complement;
}

} // namespace buechi
} // namespace ggg
