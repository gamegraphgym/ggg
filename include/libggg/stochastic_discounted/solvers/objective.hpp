#pragma once

#include "libggg/solvers/solver.hpp"
#include "libggg/stochastic_discounted/graph.hpp"
#include "libggg/utils/simplex.hpp"
#include <map>

namespace ggg {
namespace stochastic_discounted {

using ObjectiveSolutionType = ggg::solutions::RSQSolution<graph::Graph>;

class StochasticDiscountedObjectiveSolver : public ggg::solvers::Solver<graph::Graph, ObjectiveSolutionType> {
  public:
    auto solve(const graph::Graph &graph) -> ObjectiveSolutionType override;
    std::string get_name() const override { return "Objective improvement Stochastic Discounted Game Solver"; }

  private:
    bool switch_str(const graph::Graph &graph);
    int setup_matrix_rows(const graph::Graph &graph,
                          std::vector<std::vector<double>> &matrix_coeff,
                          std::vector<double> &obj_coeff_up,
                          std::vector<double> &obj_coeff_low,
                          std::vector<double> &var_up,
                          std::vector<double> &var_low);

    void calculate_obj_coefficients(const graph::Graph &graph,
                                    std::vector<double> &obj_coeff);

    void solve_simplex(Simplex &solver,
                       const std::vector<std::vector<double>> &matrix_coeff,
                       const std::vector<double> &obj_coeff_low,
                       const std::vector<double> &obj_coeff_up,
                       const std::vector<double> &var_low,
                       const std::vector<double> &var_up,
                       const std::vector<double> &n_obj_coeff,
                       std::vector<double> &sol,
                       double &obj);

    uint switches;
    uint iterations;
    uint lpiter;
    uint stales;
    int num_real_vertices;
    std::map<graph::Vertex, size_t> matrixMap;
    std::map<int, graph::Vertex> reverseMap;
    const graph::Graph *graph_;
    std::map<graph::Vertex, int> strategy;
    std::map<graph::Vertex, double> sol;
    std::vector<double> obj_coeff;
    double cff;
};

} // namespace stochastic_discounted
} // namespace ggg
