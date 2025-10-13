#pragma once

#include "libggg/solvers/solver.hpp"
#include "libggg/stochastic_discounted/graph.hpp"
#include "libggg/utils/uintqueue.hpp"
#include <boost/dynamic_bitset.hpp>
#include <map>

namespace ggg {
namespace stochastic_discounted {

using ValueSolutionType = ggg::solutions::RSQSolution<graph::Graph>;

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
