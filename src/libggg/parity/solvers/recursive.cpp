#include "libggg/parity/solvers/recursive.hpp"
#include "libggg/graphs/player_utilities.hpp"
#include "libggg/graphs/priority_utilities.hpp"
#include "libggg/parity/graph.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <algorithm>
#include <stdexcept>

namespace ggg {
namespace parity {

RecursiveParitySolver::RecursiveParitySolver()
    : max_recursion_depth_(0), // 0 means unlimited
      enable_statistics_(true),
      current_depth_(0),
      max_reached_depth_(0),
      subgames_created_(0) {
    working_set_.clear();
    vertex_mapping_.clear();
}

RecursiveParitySolver::RecursiveParitySolver(size_t max_depth)
    : max_recursion_depth_(max_depth),
      enable_statistics_(true),
      current_depth_(0),
      max_reached_depth_(0),
      subgames_created_(0) {
    working_set_.clear();
    vertex_mapping_.clear();
}

void RecursiveParitySolver::reset_solve_state() {
    current_depth_ = 0;
    max_reached_depth_ = 0;
    subgames_created_ = 0;
    working_set_.clear();
    vertex_mapping_.clear();
}

RecursiveParitySolution RecursiveParitySolver::solve(const graph::Graph &graph) {
    reset_solve_state();
    LGG_TRACE("Starting recursive solve with ", boost::num_vertices(graph), " vertices");
    auto solution = solve_internal(graph, 0);

    solution.set_max_depth_reached(max_reached_depth_);
    solution.set_subgames_created(subgames_created_);

    return solution;
}

RecursiveParitySolution RecursiveParitySolver::solve_internal(const graph::Graph &graph, size_t depth) {
    current_depth_ = depth;
    if (enable_statistics_ && depth > max_reached_depth_) {
        max_reached_depth_ = depth;
    }

    if (max_recursion_depth_ > 0 && depth >= max_recursion_depth_) {
        throw std::runtime_error("Maximum recursion depth exceeded: " + std::to_string(max_recursion_depth_));
    }

    LGG_TRACE("Recursive solve at depth ", depth, " with ", boost::num_vertices(graph), " vertices");
    RecursiveParitySolution solution;

    if (boost::num_vertices(graph) == 0) {
        LGG_TRACE("Empty game - returning");
        return solution;
    }

    int max_priority = ggg::graphs::priority_utilities::get_max_priority(graph);
    int priority_player = (max_priority % 2 == 0) ? 0 : 1;

    LGG_TRACE("Max priority: ", max_priority, " (player ", priority_player, ")");

    auto max_priority_vertices = ggg::graphs::priority_utilities::get_vertices_with_priority(graph, max_priority);

    working_set_.clear();
    working_set_.insert(max_priority_vertices.begin(), max_priority_vertices.end());

    LGG_TRACE("Found ", working_set_.size(), " vertices with max priority");

    auto [attractorSet, attractorStrategy] = ggg::graphs::player_utilities::compute_attractor(graph, working_set_, priority_player);

    for (auto vertex : attractorSet) {
        solution.set_winning_player(vertex, priority_player);
    }

    for (auto [from, to] : attractorStrategy) {
        solution.set_strategy(from, to);
    }

    if (enable_statistics_) {
        subgames_created_++;
    }
    auto subgame = create_subgame(graph, attractorSet);

    auto sub_solution = solve_internal(subgame, depth + 1);

    // Proceed with sub-solution regardless of solved flag (no longer tracked)

    working_set_.clear();
    const auto &sub_winning_regions = sub_solution.get_winning_regions();

    vertex_mapping_.clear();
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    const auto [sub_vertices_begin, sub_vertices_end] = boost::vertices(subgame);

    auto sub_it = sub_vertices_begin;
    for (auto it = vertices_begin; it != vertices_end; ++it) {
        if (attractorSet.find(*it) == attractorSet.end()) {
            if (sub_it != sub_vertices_end) {
                vertex_mapping_[*sub_it] = *it;
                ++sub_it;
            }
        }
    }

    for (auto [subgame_vertex, player] : sub_winning_regions) {
        if (player == 1 - priority_player) {
            auto original_vertex_it = vertex_mapping_.find(subgame_vertex);
            if (original_vertex_it != vertex_mapping_.end()) {
                working_set_.insert(original_vertex_it->second);
            }
        }
    }

    if (!working_set_.empty()) {
        auto [opponentAttractor, opponentStrategy] = ggg::graphs::player_utilities::compute_attractor(graph, working_set_, 1 - priority_player);

        for (auto vertex : opponentAttractor) {
            solution.set_winning_player(vertex, 1 - priority_player);
        }

        for (auto [from, to] : opponentStrategy) {
            solution.set_strategy(from, to);
        }

        if (enable_statistics_) {
            subgames_created_++;
        }
        auto final_subgame = create_subgame(graph, opponentAttractor);
        auto final_solution = solve_internal(final_subgame, depth + 1);

        // Proceed with final solution regardless of solved flag (no longer tracked)

        std::map<graph::Vertex, graph::Vertex> final_vertex_mapping;
        const auto [final_vertices_begin, final_vertices_end] = boost::vertices(final_subgame);

        auto final_it = final_vertices_begin;
        for (auto it = vertices_begin; it != vertices_end; ++it) {
            if (opponentAttractor.find(*it) == opponentAttractor.end()) {
                if (final_it != final_vertices_end) {
                    final_vertex_mapping[*final_it] = *it;
                    ++final_it;
                }
            }
        }

        const auto &final_winning_regions = final_solution.get_winning_regions();
        for (auto [subgame_vertex, player] : final_winning_regions) {
            auto original_vertex_it = final_vertex_mapping.find(subgame_vertex);
            if (original_vertex_it != final_vertex_mapping.end()) {
                solution.set_winning_player(original_vertex_it->second, player);
            }
        }
        const auto &final_strategies = final_solution.get_strategies();
        for (auto [subgame_from, subgame_to] : final_strategies) {
            auto original_from_it = final_vertex_mapping.find(subgame_from);
            auto original_to_it = final_vertex_mapping.find(subgame_to);
            if (original_from_it != final_vertex_mapping.end() &&
                original_to_it != final_vertex_mapping.end()) {
                solution.set_strategy(original_from_it->second, original_to_it->second);
            }
        }
    } else {
        for (auto [subgame_vertex, player] : sub_winning_regions) {
            auto original_vertex_it = vertex_mapping_.find(subgame_vertex);
            if (original_vertex_it != vertex_mapping_.end()) {
                solution.set_winning_player(original_vertex_it->second, player);
            }
        }
        const auto &sub_strategies = sub_solution.get_strategies();
        for (auto [subgame_from, subgame_to] : sub_strategies) {
            auto original_from_it = vertex_mapping_.find(subgame_from);
            auto original_to_it = vertex_mapping_.find(subgame_to);
            if (original_from_it != vertex_mapping_.end() &&
                original_to_it != vertex_mapping_.end()) {
                solution.set_strategy(original_from_it->second, original_to_it->second);
            }
        }
    }

    RecursiveParitySolution filtered_solution;

    const auto &winning_regions = solution.get_winning_regions();
    for (auto [vertex, winning_player] : winning_regions) {
        filtered_solution.set_winning_player(vertex, winning_player);
    }

    const auto &strategies = solution.get_strategies();
    for (auto [from_vertex, to_vertex] : strategies) {
        int vertex_owner = graph[from_vertex].player;
        int winning_player = solution.get_winning_player(from_vertex);
        if (vertex_owner == winning_player) {
            filtered_solution.set_strategy(from_vertex, to_vertex);
        }
    }

    const auto [fill_vertices_begin, fill_vertices_end] = boost::vertices(graph);
    for (auto it = fill_vertices_begin; it != fill_vertices_end; ++it) {
        auto vertex = *it;
        int vertex_owner = graph[vertex].player;
        int winning_player = filtered_solution.get_winning_player(vertex);

        if (vertex_owner == winning_player &&
            filtered_solution.get_strategy(vertex) == boost::graph_traits<graph::Graph>::null_vertex()) {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
            for (auto edge_it = out_edges_begin; edge_it != out_edges_end; ++edge_it) {
                auto target = boost::target(*edge_it, graph);
                int target_winning_player = filtered_solution.get_winning_player(target);
                if (target_winning_player == winning_player) {
                    filtered_solution.set_strategy(vertex, target);
                    break;
                }
            }
        }
    }

    return filtered_solution;
}

graph::Graph RecursiveParitySolver::create_subgame(const graph::Graph &graph,
                                                   const std::set<graph::Vertex> &to_remove) {
    graph::Graph subgame;
    std::map<graph::Vertex, graph::Vertex> vertex_mapping;

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    std::for_each(vertices_begin, vertices_end, [&](const auto &vertex) {
        if (!to_remove.count(vertex)) {
            const auto &name = graph[vertex].name;
            const auto player = graph[vertex].player;
            const auto priority = graph[vertex].priority;
            const auto new_vertex = graph::add_vertex(subgame, name, player, priority);
            vertex_mapping[vertex] = new_vertex;
        }
    });

    const auto [edges_begin, edges_end] = boost::edges(graph);
    std::for_each(edges_begin, edges_end, [&](const auto &edge) {
        const auto source = boost::source(edge, graph);
        const auto target = boost::target(edge, graph);

        if (!to_remove.count(source) && !to_remove.count(target)) {
            const auto &label = graph[edge].label;
            const auto new_source = vertex_mapping[source];
            const auto new_target = vertex_mapping[target];
            graph::add_edge(subgame, new_source, new_target, label);
        }
    });

    return subgame;
}

void RecursiveParitySolver::merge_solutions(RecursiveParitySolution &original_solution,
                                            const RecursiveParitySolution &subgame_solution,
                                            const std::map<graph::Vertex, graph::Vertex> &vertex_mapping,
                                            const graph::Graph &graph) {
    const auto &sub_winning_regions = subgame_solution.get_winning_regions();
    for (const auto &[subVertex, player] : sub_winning_regions) {
        const auto it = vertex_mapping.find(subVertex);
        if (it != vertex_mapping.end()) {
            original_solution.set_winning_player(it->second, player);
        }
    }

    const auto &sub_strategies = subgame_solution.get_strategies();
    for (const auto &[subFrom, subTo] : sub_strategies) {
        const auto from_it = vertex_mapping.find(subFrom);
        const auto to_it = vertex_mapping.find(subTo);
        if (from_it != vertex_mapping.end() && to_it != vertex_mapping.end()) {
            original_solution.set_strategy(from_it->second, to_it->second);
        }
    }
}

} // namespace parity
} // namespace ggg
