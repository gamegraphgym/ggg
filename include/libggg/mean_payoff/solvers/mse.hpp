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
 * This solver implements the MSE algorithm for solving mean payoff vertex games.
 * The algorithm transforms the mean payoff game into an energy game and solves it
 * using an iterative approach with progress measures. Provides complete solutions
 * with winning regions, strategies, and quantitative values.
 */
using SolutionType = ggg::solutions::RSQSolution<graph::Graph, ggg::strategy::DeterministicStrategy<graph::Graph>, int>;

class MSESolver : public ggg::solvers::Solver<graph::Graph, SolutionType> {
  public:
    /**
     * @brief Solve the mean payoff game using MSE algorithm
     * @param graph Mean payoff graph to solve
     * @return Complete solution with winning regions, strategies, and quantitative values
     * @complexity Time: O(V^2 * W), Space: O(V) where V = vertices and W = sum of positive weights + 1
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
