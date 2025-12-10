#pragma once

#include "libggg/mean_payoff/graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace ggg {
namespace mean_payoff {

/**
 * @brief MSE (Mean payoff Solver using Energy games) solver for mean payoff vertex games
 *
 * Implementation of the MSE algorithm for solving mean-payoff games.
 * The algorithm transforms the mean payoff game into an energy game and solves it
 * using an iterative approach with progress measures.
 * Such algorithms are described in @cite DBLP:journals/iandc/BenerecettiDM24.
 */
using SolutionType = ggg::solutions::RSQSolution<graph::Graph, ggg::strategy::DeterministicStrategy<graph::Graph>, int>;

class MSESolver : public ggg::solvers::Solver<graph::Graph, SolutionType> {
  public:
    /**
     * @brief Solve the mean payoff game using MSE algorithm
     * @param graph Mean payoff graph to solve
     * @return Complete solution with winning regions, strategies, and quantitative values
     */
    SolutionType solve(const graph::Graph &graph) override;

    /**
     * @brief Get solver name
     * @return Solver description
     */
    std::string get_name() const override {
        return "MSE (Mean payoff Solver using Energy games) Solver";
    }
};

} // namespace mean_payoff
} // namespace ggg
