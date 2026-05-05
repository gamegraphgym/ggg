#include "libggg/stochastic_discounted/solvers/objective.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/graph_utility.hpp>
#include <cmath>
#include <limits>
#include <random>

namespace ggg {
namespace stochastic_discounted {

namespace g = ggg::stochastic_discounted::graph;
using graphs_t = g::Graph;

bool StochasticDiscountedObjectiveSolver::switch_str(const graphs_t &graph) {
    bool no_switch = true;
    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        const auto old_edge = boost::edge(vertex, strategy[vertex], graph);
        double oldval = graph[old_edge.first].weight;
        const auto reach = g::get_reachable_through_probabilistic(
            graph, vertex, strategy[vertex]);
        for (const auto &[target, prob] : reach) {
            oldval += prob * graph[old_edge.first].discount * sol[target];
        }

        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex,
                                                                       graph);
        for (const auto &gedge :
             boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            const auto successor = boost::target(gedge, graph);
            if (successor == strategy[vertex]) {
                continue;
            }

            const auto new_edge = boost::edge(vertex, successor, graph);
            double newval = graph[new_edge.first].weight;
            const auto reach2 = g::get_reachable_through_probabilistic(
                graph, vertex, successor);
            for (const auto &[target, prob] : reach2) {
                newval += prob * graph[new_edge.first].discount * sol[target];
            }

            if (graph[vertex].player == 0) {
                if (oldval + 1e-6 < newval) {
                    strategy[vertex] = successor;
                    switches++;
                    no_switch = false;
                }
            } else {
                if (oldval > newval + 1e-6) {
                    strategy[vertex] = successor;
                    switches++;
                    no_switch = false;
                }
            }
        }
    }
    return no_switch;
}

int StochasticDiscountedObjectiveSolver::setup_matrix_rows(
    const graphs_t &graph,
    std::vector<std::vector<double>> &matrix_coeff,
    std::vector<double> &obj_coeff_up,
    std::vector<double> &obj_coeff_low,
    std::vector<double> &var_up,
    std::vector<double> &var_low) {
    int row = 0;
    var_up.resize(num_real_vertices);
    var_low.resize(num_real_vertices);
    for (size_t i = 0; i < num_real_vertices; ++i) {
        var_low[i] = -1e100;
        var_up[i] = 1e100;
    }

    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex,
                                                                       graph);
        for (const auto &gedge :
             boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            std::fill(matrix_coeff[row].begin(), matrix_coeff[row].end(), 0.0);
            const auto successor = boost::target(gedge, graph);
            const auto curre = boost::edge(vertex, successor, graph);
            if (graph[vertex].player == 0) {
                obj_coeff_up[row] = std::numeric_limits<double>::infinity();
                obj_coeff_low[row] = graph[curre.first].weight;
            } else {
                obj_coeff_up[row] = graph[curre.first].weight;
                obj_coeff_low[row] = -std::numeric_limits<double>::infinity();
            }
            const auto reach =
                g::get_reachable_through_probabilistic(graph, vertex, successor);
            matrix_coeff[row][matrixMap[vertex]] = 1.0;
            for (const auto &[target, prob] : reach) {
                if (target == vertex) {
                    matrix_coeff[row][matrixMap[target]] =
                        1.0 - prob * graph[curre.first].discount;
                } else {
                    matrix_coeff[row][matrixMap[target]] =
                        -prob * graph[curre.first].discount;
                }
            }
            row++;
        }
    }
    return row;
}

void StochasticDiscountedObjectiveSolver::calculate_obj_coefficients(
    const graphs_t &graph,
    std::vector<double> &obj_coeff) {
    cff = 0.0;
    std::fill(obj_coeff.begin(), obj_coeff.end(), 0.0);

    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        const auto curre = boost::edge(vertex, strategy[vertex], graph);
        const auto reach = g::get_reachable_through_probabilistic(
            graph, vertex, strategy[vertex]);

        if (graph[vertex].player == 0) {
            obj_coeff[matrixMap[vertex]] += 1.0;
            for (const auto &[target, prob] : reach) {
                obj_coeff[matrixMap[target]] +=
                    -prob * graph[curre.first].discount;
            }
            cff += -graph[curre.first].weight;
        } else {
            obj_coeff[matrixMap[vertex]] += -1.0;
            for (const auto &[target, prob] : reach) {
                obj_coeff[matrixMap[target]] +=
                    prob * graph[curre.first].discount;
            }
            cff += graph[curre.first].weight;
        }
    }
}

void StochasticDiscountedObjectiveSolver::solve_simplex(
    Simplex &solver,
    std::vector<double> &sol_vec,
    double &obj) {
    while (solver.remove_artificial_variables()) {
    }
    while (solver.calculate_simplex()) {
        lpiter++;
    }
    solver.get_full_results(sol_vec, obj, true);
}

auto StochasticDiscountedObjectiveSolver::solve(const graphs_t &graph)
    -> ggg::solutions::RSQSolution<graphs_t> {
    LGG_INFO("Starting objective improvement solver for stochastic discounted game");

    ggg::solutions::RSQSolution<graphs_t> solution;
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
    reverseMap.clear();
    int reindex = 0;

    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        matrixMap[vertex] = reindex;
        reverseMap[reindex] = vertex;
        ++reindex;
    }

    strategy.clear();
    sol.clear();
    obj_coeff.clear();

    for (const auto &vertex : boost::make_iterator_range(vertices_begin,
                                                         vertices_end)) {
        const auto [out_begin, out_end] = boost::out_edges(vertex, graph);
        if (out_begin != out_end) {
            strategy[vertex] = boost::target(*out_begin, graph);
        } else {
            strategy[vertex] = static_cast<int>(boost::graph_traits<graphs_t>::null_vertex());
        }
        sol[vertex] = 0.0;
    }

    int edges = 0;
    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        const auto [out_edges_begin, out_edges_end] =
            boost::out_edges(vertex, graph);
        edges += std::distance(out_edges_begin, out_edges_end);
    }

    num_real_vertices = boost::distance(
        g::get_non_probabilistic_vertices(graph));

    std::vector<std::vector<double>> matrix_coeff(
        edges, std::vector<double>(num_real_vertices));
    std::vector<double> obj_coeff_up(edges);
    std::vector<double> obj_coeff_low(edges);
    obj_coeff.resize(num_real_vertices);
    std::vector<double> var_up;
    std::vector<double> var_low;

    setup_matrix_rows(graph, matrix_coeff, obj_coeff_up, obj_coeff_low, var_up,
                      var_low);

    calculate_obj_coefficients(graph, obj_coeff);
    std::vector<double> n_obj_coeff(num_real_vertices);
    for (std::size_t i = 0; i < n_obj_coeff.size(); ++i) {
        n_obj_coeff[i] = obj_coeff[i] * (-1);
    }

    double obj = 0.0;
    std::vector<double> sol_vec(num_real_vertices);
    Simplex solver(matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up,
                   n_obj_coeff);
    solve_simplex(solver, sol_vec, obj);

    // Optionally purge artificial columns after first phase
    solver.purge_artificial_columns();

    for (size_t i = 0; i < sol_vec.size(); ++i) {
        sol[reverseMap[i]] = sol_vec[i];
    }
    obj = 0.0;
    for (size_t i = 0; i < sol_vec.size(); ++i) {
        obj += obj_coeff[i] * sol_vec[i];
    }

    bool stale = false;
    std::map<graphs_t::vertex_descriptor, std::vector<int>> stale_str;

    while (!stale && (obj + cff > 1e-8)) {
        stale = switch_str(graph);

        if (stale) {
            stales++;
            LGG_TRACE("STALE: obj=", obj, " cff=", cff);

            int nr_stale_vertices = 0;
            for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
                stale_str[vertex].clear();
                const auto old_edge = boost::edge(vertex, strategy[vertex], graph);
                double oldval = graph[old_edge.first].weight;
                const auto reach = g::get_reachable_through_probabilistic(
                    graph, vertex, strategy[vertex]);
                for (const auto &[target, prob] : reach) {
                    oldval += prob * graph[old_edge.first].discount * sol[target];
                }
                const auto [out_edges_begin, out_edges_end] =
                    boost::out_edges(vertex, graph);
                for (const auto &gedge :
                     boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                    const auto successor = boost::target(gedge, graph);
                    if (successor == strategy[vertex]) {
                        continue;
                    }
                    const auto new_edge = boost::edge(vertex, successor, graph);
                    double newval = graph[new_edge.first].weight;
                    const auto reach2 =
                        g::get_reachable_through_probabilistic(graph, vertex,
                                                               successor);
                    for (const auto &[target, prob] : reach2) {
                        newval += prob * graph[new_edge.first].discount * sol[target];
                    }
                    double stalevalue = oldval - newval;
                    if (std::abs(stalevalue) < 1e-8) {
                        stale_str[vertex].push_back(successor);
                    }
                }
                if (!stale_str[vertex].empty()) {
                    ++nr_stale_vertices;
                }
            }

            if (nr_stale_vertices > 0) {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::bernoulli_distribution dist(0.5);

                std::vector<bool> draws(nr_stale_vertices);
                bool has_true = false;
                for (int i = 0; i < nr_stale_vertices - 1; ++i) {
                    draws[i] = dist(gen);
                    if (draws[i]) {
                        has_true = true;
                    }
                }
                draws[nr_stale_vertices - 1] = has_true ? dist(gen) : true;
                std::shuffle(draws.begin(), draws.end(), gen);

                int swi = 0;
                for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
                    const auto &lst = stale_str[vertex];
                    if (!lst.empty()) {
                        if (draws[swi]) {
                            stale = false;
                            std::uniform_int_distribution<> dist_choice(
                                0, static_cast<int>(lst.size() - 1));
                            int choice = dist_choice(gen);
                            strategy[vertex] = lst[choice];
                        }
                        ++swi;
                    }
                }
            }
        } else {
            iterations++;
            for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
                stale_str[vertex].clear();
            }
        }

        calculate_obj_coefficients(graph, obj_coeff);
        for (std::size_t i = 0; i < n_obj_coeff.size(); ++i) {
            n_obj_coeff[i] = obj_coeff[i] * (-1);
        }

        // Use solver reuse pattern instead of full reconstruction
        solver.update_objective_row(n_obj_coeff);
        solver.normalize_objective_row();
        while (solver.calculate_simplex()) {
            lpiter++;
        }
        solver.get_full_results(sol_vec, obj, true);

        for (size_t i = 0; i < sol_vec.size(); ++i) {
            sol[reverseMap[i]] = sol_vec[i];
        }
        obj = 0.0;
        for (size_t i = 0; i < sol_vec.size(); ++i) {
            obj += obj_coeff[i] * sol_vec[i];
        }
    }

    if (obj + cff > 1e-8) {
        LGG_WARN("Stopped with no local improvements, solution may not be optimal");
    }

    for (const auto &vertex : boost::make_iterator_range(vertices_begin,
                                                         vertices_end)) {
        if (sol[vertex] >= 0.0) {
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
