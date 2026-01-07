#pragma once

#include "libggg/solvers/solver.hpp"
#include "libggg/stochastic_discounted/graph.hpp"
#include "libggg/utils/uintqueue.hpp"
#include <boost/dynamic_bitset.hpp>
#include <map>

namespace ggg {
namespace stochastic_discounted {

using ValueSolutionType = ggg::solutions::RSQSolution<graph::Graph>;

/**
 * @brief Value iteration algorithm for stochastic discounted games
 *
 * Implementation of the classical value iteration method for solving stochastic
 * discounted games based on @cite DBLP:journals/pnas/Shapley53 and @cite DBLP:books/wi/Puterman94.
 * The algorithm iteratively updates value estimates until convergence using
 * Bellman equations with discounting factors.
 */
class StochasticDiscountedValueSolver : public ggg::solvers::Solver<graph::Graph, ValueSolutionType> {
  public:
    auto solve(const graph::Graph &graph) -> ValueSolutionType override;
    [[nodiscard]] auto get_name() const -> std::string override { return "Value Iteration Stochastic Discounted Game Solver"; }

  private:
    uint lifts;
    uint iterations;
    const graph::Graph *graph_;
    Uintqueue TAtr;
    boost::dynamic_bitset<> BAtr;
    double oldcost;
    std::map<graph::Vertex, int> strategy;
    std::map<graph::Vertex, double> sol;
};

} // namespace stochastic_discounted
} // namespace ggg
