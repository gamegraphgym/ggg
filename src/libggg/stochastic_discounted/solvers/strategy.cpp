#include "libggg/stochastic_discounted/solvers/strategy.hpp"
#include "libggg/utils/logging.hpp"
#include "libggg/utils/simplex.hpp"
#include <boost/graph/graph_utility.hpp>
#include <map>

namespace ggg {
namespace stochastic_discounted {

namespace g = ggg::stochastic_discounted::graph;
using graphs_t = g::Graph;

void StochasticDiscountedStrategySolver::switch_str(const graphs_t &graph) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].player == 0) {
            const auto olde = edge(vertex, strategy[vertex], graph);
            double oldval = graph[olde.first].weight;
            const auto reach = g::get_reachable_through_probabilistic(graph, vertex, strategy[vertex]);
            for (const auto &[TARGET, PROB] : reach) {
                oldval += PROB * graph[olde.first].discount * sol[TARGET];
            }
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
            for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto successor = boost::target(gedge, graph);
                const auto newe = edge(vertex, successor, graph);
                double newval = graph[newe.first].weight;
                const auto reach = g::get_reachable_through_probabilistic(graph, vertex, successor);
                for (const auto &[TARGET, PROB] : reach) {
                    newval += PROB * graph[newe.first].discount * sol[TARGET];
                }
                if (oldval + 1e-6 < newval) {
                    strategy[vertex] = successor;
                    switches++;
                }
            }
        }
    }
}

int StochasticDiscountedStrategySolver::count_player_edges(const graphs_t &graph) {
    int edges = 0;
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].player == 0) {
            edges++;
        } else {
            if (graph[vertex].player == 1) {
                const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
                for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                    edges++;
                }
            }
        }
    }
    return edges;
}

void StochasticDiscountedStrategySolver::calculate_obj_coefficients(const graphs_t &graph,
                                                                    std::vector<double> &obj_coeff,
                                                                    std::vector<double> &var_up,
                                                                    std::vector<double> &var_low) {
    obj_coeff.resize(num_real_vertices);
    var_up.resize(num_real_vertices);
    var_low.resize(num_real_vertices);

    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        obj_coeff[matrixMap[vertex]] = -1;
        var_up[matrixMap[vertex]] = std::numeric_limits<double>::infinity();
        var_low[matrixMap[vertex]] = -std::numeric_limits<double>::infinity();
    }
}

int StochasticDiscountedStrategySolver::setup_matrix_rows(const graphs_t &graph,
                                                          std::vector<std::vector<double>> &matrix_coeff,
                                                          std::vector<double> &obj_coeff_up,
                                                          std::vector<double> &obj_coeff_low) {
    int row = 0;
    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        std::fill(matrix_coeff[row].begin(), matrix_coeff[row].end(), 0.0);
        if (graph[vertex].player == 0) {
            const auto curre = edge(vertex, strategy[vertex], graph);
            obj_coeff_up[row] = graph[curre.first].weight;
            obj_coeff_low[row] = graph[curre.first].weight;
            const auto reach = g::get_reachable_through_probabilistic(graph, vertex, strategy[vertex]);
            matrix_coeff[row][matrixMap[vertex]] = 1.0;
            for (const auto &[TARGET, PROB] : reach) {
                if (TARGET == vertex) {
                    matrix_coeff[row][matrixMap[TARGET]] = 1.0 - PROB * graph[curre.first].discount;
                } else {
                    matrix_coeff[row][matrixMap[TARGET]] = -1.0 * PROB * graph[curre.first].discount;
                }
            }
            row++;
        } else {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
            for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto successor = boost::target(gedge, graph);
                const auto curre = edge(vertex, successor, graph);
                obj_coeff_up[row] = graph[curre.first].weight;
                obj_coeff_low[row] = -std::numeric_limits<double>::infinity();
                const auto reach = g::get_reachable_through_probabilistic(graph, vertex, successor);
                matrix_coeff[row][matrixMap[vertex]] = 1.0;
                for (const auto &[TARGET, PROB] : reach) {
                    if (TARGET == vertex) {
                        matrix_coeff[row][matrixMap[TARGET]] = 1.0 - PROB * graph[curre.first].discount;
                    } else {
                        matrix_coeff[row][matrixMap[TARGET]] = -1.0 * PROB * graph[curre.first].discount;
                    }
                }
                row++;
            }
        }
    }
    return row;
}

void StochasticDiscountedStrategySolver::solve_simplex(const std::vector<std::vector<double>> &matrix_coeff,
                                                       const std::vector<double> &obj_coeff_low,
                                                       const std::vector<double> &obj_coeff_up,
                                                       const std::vector<double> &var_low,
                                                       const std::vector<double> &var_up,
                                                       const std::vector<double> &n_obj_coeff,
                                                       std::vector<double> &sol_vec,
                                                       double &obj) {
    Simplex solver(matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff);
    while (solver.remove_artificial_variables()) {
        // solver.printTableau();
    }
    while (solver.calculate_simplex()) {
        lpiter++;
    }
    solver.get_full_results(sol_vec, obj, true);
}

auto StochasticDiscountedStrategySolver::solve(const graphs_t &graph) -> ggg::solutions::RSQSolution<graphs_t> {
    LGG_INFO("Starting Strategy Improvement solver for stochastic discounted game");

    ggg::solutions::RSQSolution<graphs_t> solution;
    if (!g::is_valid(graph)) {
        LGG_ERROR("Invalid stochastic discounted graph provided");
        return solution;
    }
    if (boost::num_vertices(graph) == 0) {
        LGG_WARN("Empty graph provided");
        return solution;
    }

    switches = 0;
    iterations = 0;
    lpiter = 0;

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    matrixMap.clear();
    int reindex = 0;
    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        matrixMap[vertex] = reindex;
        reverseMap[reindex] = vertex;
        ++reindex;
    }
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (matrixMap.count(vertex) == 0) {
            matrixMap[vertex] = reindex;
            reverseMap[reindex] = vertex;
            ++reindex;
        }
    }
    strategy.clear();
    sol.clear();

    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].player == 0) {
            const auto [out_begin, out_end] = boost::out_edges(vertex, graph);
            if (out_begin != out_end) {
                strategy[vertex] = boost::target(*out_begin, graph);
            }
        } else {
            strategy[vertex] = 0;
        }
        sol[vertex] = 0.0;
    }

    int edges = count_player_edges(graph);
    int num_vertices = boost::num_vertices(graph);
    num_real_vertices = boost::distance(g::get_non_probabilistic_vertices(graph));

    std::vector<std::vector<double>> matrix_coeff(edges, std::vector<double>(num_real_vertices));
    std::vector<double> obj_coeff_up(edges);
    std::vector<double> obj_coeff_low(edges);
    std::vector<double> obj_coeff;
    std::vector<double> var_up;
    std::vector<double> var_low;

    calculate_obj_coefficients(graph, obj_coeff, var_up, var_low);

    setup_matrix_rows(graph, matrix_coeff, obj_coeff_up, obj_coeff_low);

    std::vector<double> n_obj_coeff(num_real_vertices);
    for (std::size_t i = 0; i < n_obj_coeff.size(); ++i) {
        n_obj_coeff[i] = obj_coeff[i] * (-1);
    }

    double obj = 0;
    std::vector<double> sol_vec(num_real_vertices);
    solve_simplex(matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff, sol_vec, obj);

    for (size_t i = 0; i < sol_vec.size(); ++i) {
        sol[reverseMap[i]] = sol_vec[i];
    }

    double old_obj = obj - 1;

    while (old_obj < obj) {
        iterations++;
        old_obj = obj;
        switch_str(graph);

        setup_matrix_rows(graph, matrix_coeff, obj_coeff_up, obj_coeff_low);

        solve_simplex(matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff, sol_vec, obj);

        for (size_t i = 0; i < sol_vec.size(); ++i) {
            sol[reverseMap[i]] = sol_vec[i];
        }
    }

    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (sol[vertex] >= 0) {
            solution.set_winning_player(vertex, 0);
        } else {
            solution.set_winning_player(vertex, 1);
        }
        if (graph[vertex].player == 0) {
            solution.set_strategy(vertex, strategy[vertex]);
        } else {
            solution.set_strategy(vertex, -1);
        }
        solution.set_value(vertex, sol[vertex]);
    }

    LGG_TRACE("Solved with ", iterations, " iterations");
    LGG_TRACE("Solved with ", lpiter, " LP pivotes");
    LGG_TRACE("Solved with ", switches, " switches");
    return solution;
}

} // namespace stochastic_discounted
} // namespace ggg
