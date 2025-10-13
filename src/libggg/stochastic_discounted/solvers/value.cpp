#include "libggg/stochastic_discounted/solvers/value.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/graph_utility.hpp>
#include <map>

namespace ggg {
namespace stochastic_discounted {

namespace g = ggg::stochastic_discounted::graph;
using graphs_t = g::Graph;

auto StochasticDiscountedValueSolver::solve(const graphs_t &graph) -> ggg::solutions::RSQSolution<graphs_t> {
    LGG_INFO("Starting Value Iteration solver for stochastic discounted game");

    ggg::solutions::RSQSolution<graphs_t> solution;
    if (!g::is_valid(graph)) {
        LGG_ERROR("Invalid stochastic discounted graph provided");
        return solution;
    }
    if (boost::num_vertices(graph) == 0) {
        LGG_WARN("Empty graph provided");
        return solution;
    }

    lifts = 0;
    iterations = 0;

    int num_vertices = boost::num_vertices(graph);
    strategy.clear();
    sol.clear();
    TAtr.resize(num_vertices);
    BAtr.resize(num_vertices);

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);

    for (const auto &vertex : g::get_non_probabilistic_vertices(graph)) {
        strategy[vertex] = -1;
        sol[vertex] = 0.0;
        TAtr.push(vertex);
        BAtr[vertex] = true;
    }

    int pos;
    int best_succ;
    double sum;
    double best;
    while (TAtr.nonempty()) {
        iterations++;
        pos = TAtr.pop();
        BAtr[pos] = false;
        oldcost = sol[pos];
        best_succ = -1;
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(pos, graph);
        for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            const auto successor = boost::target(gedge, graph);
            const auto curre = edge(pos, successor, graph);
            const auto reach = g::get_reachable_through_probabilistic(graph, pos, successor);
            if (best_succ == -1) {
                best_succ = successor;
                best = 0;
                for (const auto &[TARGET, PROB] : reach) {
                    best += (PROB * graph[curre.first].discount * sol[TARGET]);
                }
                best += graph[curre.first].weight;
            } else {
                sum = 0;
                for (const auto &[TARGET, PROB] : reach) {
                    sum += (PROB * graph[curre.first].discount * sol[TARGET]);
                }
                sum += graph[curre.first].weight;
                if ((graph[pos].player == 0 && sum > best) || (graph[pos].player == 1 && sum < best)) {
                    best_succ = successor;
                    best = sum;
                }
            }
        }
        if (sol[pos] != best || strategy[pos] == -1) {
            lifts++;
            sol[pos] = best;
            strategy[pos] = best_succ;

            for (auto pred : boost::make_iterator_range(boost::vertices(graph))) {
                for (auto e : boost::make_iterator_range(boost::out_edges(pred, graph))) {
                    if (boost::target(e, graph) == pos) {
                        if (!BAtr[pred]) {
                            TAtr.push(pred);
                            BAtr[pred] = true;
                        }
                    }
                }
            }
        }
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
    LGG_TRACE("Solved with ", lifts, " lifts");
    return solution;
}

} // namespace stochastic_discounted
} // namespace ggg
