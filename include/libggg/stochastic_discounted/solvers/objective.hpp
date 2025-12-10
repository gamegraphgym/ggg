#pragma once

#include "libggg/solvers/solver.hpp"
#include "libggg/stochastic_discounted/graph.hpp"
#include "libggg/utils/simplex.hpp"
#include <map>

namespace ggg {
namespace stochastic_discounted {

using ObjectiveSolutionType = ggg::solutions::RSQSolution<graph::Graph>;

/**
 * @brief Objective Improvement solver for stochastic discounted games
 *
 * Implementation of the novel objective improvement approach described in
 * @cite DBLP:journals/corr/DellErbaDS24. This algorithm builds constraint systems using
 * every edge to define inequations and updates the objective function by
 * considering strategy edges for both players.
 */
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
