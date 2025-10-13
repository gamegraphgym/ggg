#include "libggg/stochastic_discounted/solvers/objective.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/graph_utility.hpp>

namespace ggg {
namespace stochastic_discounted {

namespace g = ggg::stochastic_discounted::graph;
using graphs_t = g::Graph;

bool StochasticDiscountedObjectiveSolver::switch_str(const graphs_t &graph) {
    bool no_switch = true;
    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        const auto olde = edge(vertex, strategy[vertex], graph);
        double oldval = graph[olde.first].weight;
        const auto reach = g::get_reachable_through_probabilistic(graph, vertex, strategy[vertex]);
        for (const auto &[TARGET, PROB] : reach) {
            oldval += PROB * graph[olde.first].discount * sol[TARGET];
        }
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
        for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            const auto successor = boost::target(gedge, graph);
            if (successor != strategy[vertex]) {
                const auto newe = edge(vertex, successor, graph);
                double newval = graph[newe.first].weight;
                const auto reach = g::get_reachable_through_probabilistic(graph, vertex, successor);
                for (const auto &[TARGET, PROB] : reach) {
                    newval += PROB * graph[newe.first].discount * sol[TARGET];
                }
                if (((graph[vertex].player == 0) && (oldval + 1e-6 < newval)) || ((graph[vertex].player == 1) && (oldval > 1e-6 + newval))) { // Precision stop
                    strategy[vertex] = successor;
                    switches++;
                    no_switch = false;
                }
            }
        }
    }
    return no_switch;
}

int StochasticDiscountedObjectiveSolver::setup_matrix_rows(const graphs_t &graph,
                                                           std::vector<std::vector<double>> &matrix_coeff,
                                                           std::vector<double> &obj_coeff_up,
                                                           std::vector<double> &obj_coeff_low,
                                                           std::vector<double> &var_up,
                                                           std::vector<double> &var_low) {
    int row = 0;
    var_up.resize(num_real_vertices);
    var_low.resize(num_real_vertices);

    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        std::fill(matrix_coeff[row].begin(), matrix_coeff[row].end(), 0.0);
        var_up[matrixMap[vertex]] = std::numeric_limits<double>::infinity();
        var_low[matrixMap[vertex]] = -std::numeric_limits<double>::infinity();
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
        for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            const auto successor = boost::target(gedge, graph);
            const auto curre = edge(vertex, successor, graph);
            if (graph[vertex].player == 0) {
                obj_coeff_up[row] = std::numeric_limits<double>::infinity();
                obj_coeff_low[row] = graph[curre.first].weight;
            } else {
                obj_coeff_up[row] = graph[curre.first].weight;
                obj_coeff_low[row] = -std::numeric_limits<double>::infinity();
            }
            const auto reach = g::get_reachable_through_probabilistic(graph, vertex, successor);
            matrix_coeff[row][matrixMap[vertex]] = 1.0;
            for (const auto &[TARGET, PROB] : reach) {
                if (TARGET == vertex) {
                    matrix_coeff[row][matrixMap[TARGET]] = 1.0 - PROB * graph[curre.first].discount;
                } else {
                    matrix_coeff[row][matrixMap[TARGET]] = -PROB * graph[curre.first].discount;
                }
            }
            row++;
        }
    }
    return row;
}

void StochasticDiscountedObjectiveSolver::calculate_obj_coefficients(const graphs_t &graph, std::vector<double> &obj_coeff) {
    cff = 0;
    obj_coeff.resize(num_real_vertices);
    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        const auto curre = edge(vertex, strategy[vertex], graph);
        const auto reach = g::get_reachable_through_probabilistic(graph, vertex, strategy[vertex]);
        if (graph[vertex].player == 0) {
            obj_coeff[matrixMap[vertex]] += 1;
            for (const auto &[TARGET, PROB] : reach) {
                obj_coeff[matrixMap[TARGET]] += -PROB * graph[curre.first].discount;
            }
            cff += -(graph[curre.first].weight);
        } else {
            obj_coeff[matrixMap[vertex]] += -1;
            for (const auto &[TARGET, PROB] : reach) {
                obj_coeff[matrixMap[TARGET]] += PROB * graph[curre.first].discount;
            }
            cff += graph[curre.first].weight;
        }
    }
}

void StochasticDiscountedObjectiveSolver::solve_simplex(Simplex &solver,
                                                        const std::vector<std::vector<double>> &matrix_coeff,
                                                        const std::vector<double> &obj_coeff_low,
                                                        const std::vector<double> &obj_coeff_up,
                                                        const std::vector<double> &var_low,
                                                        const std::vector<double> &var_up,
                                                        const std::vector<double> &n_obj_coeff,
                                                        std::vector<double> &sol_vec,
                                                        double &obj) {
    while (solver.remove_artificial_variables()) {
        // solver.printTableau();
    }
    while (solver.calculate_simplex()) {
        lpiter++;
    }
    solver.get_full_results(sol_vec, obj, true);
}

auto StochasticDiscountedObjectiveSolver::solve(const graphs_t &graph) -> ggg::solutions::RSQSolution<graphs_t> {
    LGG_INFO("Starting objective improvement solver for stochastic discounted game");

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
    stales = 0;

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
        const auto [out_begin, out_end] = boost::out_edges(vertex, graph);
        if (out_begin != out_end) {
            strategy[vertex] = boost::target(*out_begin, graph);
        }
        sol[vertex] = 0.0;
    }

    int edges = 0;
    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
        for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            edges++;
        }
    }
    num_real_vertices = boost::distance(g::get_non_probabilistic_vertices(graph));

    std::vector<std::vector<double>> matrix_coeff(edges, std::vector<double>(num_real_vertices));
    std::vector<double> obj_coeff_up(edges);
    std::vector<double> obj_coeff_low(edges);
    std::vector<double> obj_coeff;
    std::vector<double> var_up;
    std::vector<double> var_low;

    calculate_obj_coefficients(graph, obj_coeff);

    setup_matrix_rows(graph, matrix_coeff, obj_coeff_up, obj_coeff_low, var_up, var_low);

    std::vector<double> n_obj_coeff(num_real_vertices);
    for (std::size_t i = 0; i < n_obj_coeff.size(); ++i) {
        n_obj_coeff[i] = obj_coeff[i] * (-1);
    }

    double obj = 0;
    std::vector<double> sol_vec(num_real_vertices);
    Simplex solver(matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff);
    solve_simplex(solver, matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff, sol_vec, obj);
    solver.purge_artificial_columns();

    for (size_t i = 0; i < sol_vec.size(); ++i) {
        sol[reverseMap[i]] = sol_vec[i];
    }
    bool stale = false;
    std::map<graphs_t::vertex_descriptor, std::list<int>> stale_str;
    bool improving = true;
    int nr_str = 0;
    double stalevalue = 0;
    while (!stale && (cff - obj > 1e-8)) {
        stale = switch_str(graph);
        if (stale) {
            stales++;
            if (nr_str == 0) {
                if (improving) {
                    improving = false;
                    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
                        const auto olde = edge(vertex, strategy[vertex], graph);
                        double oldval = graph[olde.first].weight;
                        const auto reach = g::get_reachable_through_probabilistic(graph, vertex, strategy[vertex]);
                        for (const auto &[TARGET, PROB] : reach) {
                            oldval += PROB * graph[olde.first].discount * sol[TARGET];
                        }
                        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
                        for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                            const auto successor = boost::target(gedge, graph);
                            if (successor != strategy[vertex]) {
                                const auto newe = edge(vertex, successor, graph);
                                double newval = graph[newe.first].weight;
                                const auto reach = g::get_reachable_through_probabilistic(graph, vertex, successor);
                                for (const auto &[TARGET, PROB] : reach) {
                                    newval += PROB * graph[newe.first].discount * sol[TARGET];
                                }
                                stalevalue = oldval - newval;
                                if (std::abs(stalevalue) < 1e-8) {
                                    stale_str[vertex].push_back(successor);
                                    nr_str++;
                                }
                            }
                        }
                    }
                } else {
                    break;
                }
            }
            for (const auto &[key, lst] : stale_str) {
                if (!lst.empty()) {
                    strategy[key] = lst.front();
                    stale_str[key].pop_front();
                    stale = false;
                    nr_str--;
                    break;
                }
            }
        } else {
            iterations++;
            improving = true;
            stale_str.clear();
        }

        calculate_obj_coefficients(graph, obj_coeff);
        for (std::size_t i = 0; i < n_obj_coeff.size(); ++i) {
            n_obj_coeff[i] = obj_coeff[i] * (-1);
        }

        solver.update_objective_row(n_obj_coeff, 0);
        solver.normalize_objective_row();
        solve_simplex(solver, matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff, sol_vec, obj);
    }

    if (cff - obj > 1e-8) {
        LGG_INFO("Warning, stopping with no local improvements, solution not optimal");
    }

    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (sol[vertex] >= 0) {
            solution.set_winning_player(vertex, 0);
        } else {
            solution.set_winning_player(vertex, 1);
        }
        solution.set_strategy(vertex, strategy[vertex]);
        solution.set_value(vertex, sol[vertex]);
    }

    LGG_TRACE("Solved with ", iterations, " iterations");
    LGG_TRACE("Solved with ", lpiter, " LP pivotes");
    LGG_TRACE("Solved with ", switches, " switches");
    LGG_TRACE("Solved with ", stales, " stales");
    return solution;
}

} // namespace stochastic_discounted
} // namespace ggg
