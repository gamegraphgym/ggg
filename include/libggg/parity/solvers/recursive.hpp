#pragma once

#include "libggg/parity/graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <boost/graph/graph_traits.hpp>
#include <map>
#include <set>
#include <string>

namespace ggg {
namespace parity {

/**
 * @brief Solution type for recursive solver that includes statistics
 */
class RecursiveParitySolution : public ggg::solutions::RSSolution<graph::Graph> {
  private:
    size_t max_depth_reached_ = 0;
    size_t subgames_created_ = 0;

  public:
    RecursiveParitySolution() = default;

    void set_max_depth_reached(size_t depth) { max_depth_reached_ = depth; }
    void set_subgames_created(size_t count) { subgames_created_ = count; }

    std::map<std::string, std::string> get_statistics() const {
        std::map<std::string, std::string> stats;
        stats["max_depth_reached"] = std::to_string(max_depth_reached_);
        stats["subgames_created"] = std::to_string(subgames_created_);
        return stats;
    }

    size_t get_max_depth_reached() const { return max_depth_reached_; }
    size_t get_subgames_created() const { return subgames_created_; }
};

/**
 * @brief Simple recursive parity game solver
 */
class RecursiveParitySolver : public ggg::solvers::Solver<graph::Graph, RecursiveParitySolution> {
  public:
    RecursiveParitySolver();
    explicit RecursiveParitySolver(size_t max_depth);
    RecursiveParitySolution solve(const graph::Graph &graph) override;
    std::string get_name() const override { return "Recursive Parity Game Solver"; }
    size_t get_current_depth() const { return current_depth_; }

  private:
    size_t max_recursion_depth_;
    bool enable_statistics_;

    size_t current_depth_;
    size_t max_reached_depth_;
    size_t subgames_created_;

    std::set<graph::Vertex> working_set_;
    std::map<graph::Vertex, graph::Vertex> vertex_mapping_;

    RecursiveParitySolution solve_internal(const graph::Graph &graph, size_t depth);
    void reset_solve_state();
    graph::Graph create_subgame(const graph::Graph &graph,
                                const std::set<graph::Vertex> &to_remove);
    void merge_solutions(RecursiveParitySolution &original_solution,
                         const RecursiveParitySolution &subgame_solution,
                         const std::map<graph::Vertex, graph::Vertex> &vertex_mapping,
                         const graph::Graph &graph);
};

} // namespace parity
} // namespace ggg
