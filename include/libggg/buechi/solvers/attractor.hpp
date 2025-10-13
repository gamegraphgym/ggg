
#pragma once

#include "libggg/parity/graph.hpp"
#include "libggg/solvers/solver.hpp"

#include <set>

namespace ggg {
namespace buechi {

class AttractorSolver : public ggg::solvers::Solver<ggg::parity::graph::Graph, ggg::solutions::RSSolution<ggg::parity::graph::Graph>> {
  public:
    ggg::solutions::RSSolution<ggg::parity::graph::Graph> solve(const ggg::parity::graph::Graph &graph) override;
    std::string get_name() const override { return "Buechi Game Solver (Iterative Attractor Algorithm)"; }

  private:
    bool validate_buchi_game(const ggg::parity::graph::Graph &graph) const;
    std::set<ggg::parity::graph::Vertex> compute_attractor(const ggg::parity::graph::Graph &graph,
                                                           const std::set<ggg::parity::graph::Vertex> &active_vertices,
                                                           int curr_player,
                                                           const std::set<ggg::parity::graph::Vertex> &curr_target);
    std::set<ggg::parity::graph::Vertex> get_buchi_accepting_vertices(const ggg::parity::graph::Graph &graph) const;
    std::set<ggg::parity::graph::Vertex> compute_complement(const std::set<ggg::parity::graph::Vertex> &active_vertices,
                                                            const std::set<ggg::parity::graph::Vertex> &inactive_vertices) const;

    uint iterations;
    uint attractions;
};

} // namespace buechi
} // namespace ggg
