#pragma once

#include "libggg/solvers/solver.hpp"
#include "libggg/stochastic_discounted/graph.hpp"
#include <map>

namespace ggg {
namespace stochastic_discounted {

using StrategySolutionType = ggg::solutions::RSQSolution<graph::Graph>;

class StochasticDiscountedStrategySolver : public ggg::solvers::Solver<graph::Graph, StrategySolutionType> {
  public:
    auto solve(const graph::Graph &graph) -> StrategySolutionType override;
    [[nodiscard]] auto get_name() const -> std::string override { return "Strategy Improvement Stochastic Discounted Game Solver"; }

  private:
    void switch_str(const graph::Graph &graph);
    int setup_matrix_rows(const graph::Graph &graph,
                          std::vector<std::vector<double>> &matrix_coeff,
                          std::vector<double> &obj_coeff_up,
                          std::vector<double> &obj_coeff_low);

    void calculate_obj_coefficients(const graph::Graph &graph,
                                    std::vector<double> &obj_coeff,
                                    std::vector<double> &var_up,
                                    std::vector<double> &var_low);

    void solve_simplex(const std::vector<std::vector<double>> &matrix_coeff,
                       const std::vector<double> &obj_coeff_low,
                       const std::vector<double> &obj_coeff_up,
                       const std::vector<double> &var_low,
                       const std::vector<double> &var_up,
                       const std::vector<double> &n_obj_coeff,
                       std::vector<double> &sol,
                       double &obj);

    int count_player_edges(const graph::Graph &graph);

    uint switches;
    uint iterations;
    uint lpiter;
    int num_real_vertices;
    std::map<graph::Vertex, size_t> matrixMap;
    std::map<int, graph::Vertex> reverseMap;
    const graph::Graph *graph_;
    std::map<graph::Vertex, int> strategy;
    std::map<graph::Vertex, double> sol;
    double oldcost;
    std::vector<double> obj_coeff;
};

} // namespace stochastic_discounted
} // namespace ggg
