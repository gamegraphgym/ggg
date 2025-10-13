#include "libggg/parity/solvers/progressive_small_progress_measures.hpp"
#include "libggg/graphs/priority_utilities.hpp"
#include <boost/graph/graph_traits.hpp>
#include <cassert>
#include <queue>
#include <vector>

namespace ggg {
namespace parity {

ggg::solutions::RSSolution<graph::Graph> ProgressiveSmallProgressMeasuresSolver::solve(const graph::Graph &graph) {
    // Implementation moved from solvers/parity/progressive_small_progress_measures
    init(graph);

    for (int i = 0; i < k * boost::num_vertices(*pv); i++) {
        pms[i] = 0;
    }

    for (int i = 0; i < boost::num_vertices(*pv); i++) {
        strategy[i] = -1;
    }

    for (int i = 0; i < k; i++) {
        counts[i] = 0;
    }

    auto vertices = boost::vertices(*pv);
    for (auto vertex_it = vertices.first; vertex_it != vertices.second; ++vertex_it) {
        auto vertex = *vertex_it;
        int node = vertex_to_node(*pv, vertex);
        counts[(*pv)[vertex].priority]++;
    }

    for (int n = 0; n < boost::num_vertices(*pv); n++) {
        dirty[n] = 0;
    }

    lift_count = lift_attempt = 0;

    for (int n = boost::num_vertices(*pv) - 1; n >= 0; n--) {
        if (lift(n, -1)) {
            auto vertex = node_to_vertex(*pv, n);
            auto all_vertices = boost::vertices(*pv);
            for (auto other_vertex_it = all_vertices.first; other_vertex_it != all_vertices.second; ++other_vertex_it) {
                auto other_vertex = *other_vertex_it;
                auto outgoing_edges = boost::out_edges(other_vertex, *pv);
                for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
                    auto edge = *edge_it;
                    if (boost::target(edge, *pv) == vertex) {
                        int from_node = vertex_to_node(*pv, other_vertex);
                        if (lift(from_node, n)) {
                            todo_push(from_node);
                        }
                    }
                }
            }
        }
    }

    int64_t last_update = 0;

    while (!todo.empty()) {
        int n = todo_pop();
        auto vertex = node_to_vertex(*pv, n);

        for (auto vertex_it = vertices.first; vertex_it != vertices.second; ++vertex_it) {
            auto other_vertex = *vertex_it;
            auto outgoing_edges = boost::out_edges(other_vertex, *pv);
            for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
                auto edge = *edge_it;
                if (boost::target(edge, *pv) == vertex) {
                    int from_node = vertex_to_node(*pv, other_vertex);
                    if (lift(from_node, n)) {
                        todo_push(from_node);
                    }
                }
            }
        }

        if (last_update + 10 * boost::num_vertices(*pv) < lift_count) {
            last_update = lift_count;
            update(0);
            update(1);
        }
    }

    ggg::solutions::RSSolution<graph::Graph> solution;

    for (auto vertex_it = vertices.first; vertex_it != vertices.second; ++vertex_it) {
        auto vertex = *vertex_it;
        int node = vertex_to_node(*pv, vertex);
        int *pm = pms.data() + k * node;

        if ((pm[0] == -1) == (pm[1] == -1)) {
        }

        int winner = pm[0] == -1 ? 0 : 1;
        solution.set_winning_player(vertex, winner);

        if ((*pv)[vertex].player == winner && strategy[node] != -1) {
            graph::Vertex strategy_vertex = node_to_vertex(*pv, strategy[node]);
            solution.set_strategy(vertex, strategy_vertex);
        }
    }

    return solution;
}

void ProgressiveSmallProgressMeasuresSolver::init(const graph::Graph &game) {
    pv = &game;
    k = graphs::priority_utilities::get_max_priority(game) + 1;
    if (k < 2)
        k = 2;

    int vertex_count = boost::num_vertices(game);
    pms.resize(k * vertex_count);
    strategy.resize(vertex_count);
    counts.resize(k);
    tmp.resize(k);
    best.resize(k);
    dirty.resize(vertex_count);
    unstable.resize(vertex_count);
}

bool ProgressiveSmallProgressMeasuresSolver::pm_less(int *a, int *b, int d, int pl) {
    if (b[pl] == -1)
        return a[pl] != -1;
    else if (a[pl] == -1)
        return false;

    const int start = ((k & 1) == pl) ? k - 2 : k - 1;
    for (int i = start; i >= d; i -= 2) {
        if (a[i] == b[i])
            continue;
        if (a[i] > counts[i] && b[i] > counts[i])
            return false;
        return a[i] < b[i];
    }
    return false;
}

void ProgressiveSmallProgressMeasuresSolver::pm_copy(int *dst, int *src, int pl) {
    for (int i = pl; i < k; i += 2) {
        dst[i] = src[i];
    }
}

void ProgressiveSmallProgressMeasuresSolver::prog(int *dst, int *src, int d, int pl) {
    if (src[pl] == -1) {
        dst[pl] = -1;
        return;
    }

    int i = pl;
    for (; i < d; i += 2) {
        dst[i] = 0;
    }

    int carry = (d % 2 == pl) ? 1 : 0;
    for (; i < k; i += 2) {
        int v = src[i] + carry;
        if (v > counts[i]) {
            dst[i] = 0;
            carry = 1;
        } else {
            dst[i] = v;
            carry = 0;
        }
    }

    if (carry)
        dst[pl] = -1;
}

bool ProgressiveSmallProgressMeasuresSolver::canlift(int node, int pl) {
    int *pm = pms.data() + k * node;
    if (pm[pl] == -1)
        return false;

    auto vertex = node_to_vertex(*pv, node);
    const int d = (*pv)[vertex].priority;

    if ((*pv)[vertex].player == pl) {
        auto outgoing_edges = boost::out_edges(vertex, *pv);
        for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
            auto edge = *edge_it;
            auto to_vertex = boost::target(edge, *pv);
            int to_node = vertex_to_node(*pv, to_vertex);
            prog(tmp.data(), pms.data() + k * to_node, d, pl);
            if (pm_less(pm, tmp.data(), d, pl))
                return true;
        }
        return false;
    } else {
        int best_to = -1;
        auto outgoing_edges = boost::out_edges(vertex, *pv);
        for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
            auto edge = *edge_it;
            auto to_vertex = boost::target(edge, *pv);
            int to_node = vertex_to_node(*pv, to_vertex);
            prog(tmp.data(), pms.data() + k * to_node, d, pl);
            if (best_to == -1 || pm_less(tmp.data(), best.data(), d, pl)) {
                for (int i = 0; i < k; i++) {
                    best[i] = tmp[i];
                }
                best_to = to_node;
            }
        }
        if (best_to == -1)
            return false;
        return pm_less(pm, best.data(), d, pl);
    }
}

bool ProgressiveSmallProgressMeasuresSolver::lift(int node, int target) {
    int *pm = pms.data() + k * node;
    if (pm[0] == -1 && pm[1] == -1)
        return false;

    auto vertex = node_to_vertex(*pv, node);
    const int pl_max = (*pv)[vertex].player;
    const int pl_min = 1 - pl_max;
    const int d = (*pv)[vertex].priority;

    int best_ch0 = -1, best_ch1 = -1;

    if (pm[pl_max] != -1) {
        if (target != -1) {
            prog(tmp.data(), pms.data() + k * target, d, pl_max);
            if (pm_less(pm, tmp.data(), d, pl_max)) {
                pm_copy(pm, tmp.data(), pl_max);
                if (pl_max)
                    best_ch1 = target;
                else
                    best_ch0 = target;
            }
        } else {
            auto outgoing_edges = boost::out_edges(vertex, *pv);
            for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
                auto edge = *edge_it;
                auto to_vertex = boost::target(edge, *pv);
                int to_node = vertex_to_node(*pv, to_vertex);
                prog(tmp.data(), pms.data() + k * to_node, d, pl_max);
                if (pm_less(pm, tmp.data(), d, pl_max)) {
                    pm_copy(pm, tmp.data(), pl_max);
                    if (pl_max)
                        best_ch1 = to_node;
                    else
                        best_ch0 = to_node;
                }
            }
        }
    }

    if (pm[pl_min] != -1 && (target == -1 || target == strategy[node])) {
        int best_to = -1;
        auto outgoing_edges = boost::out_edges(vertex, *pv);
        for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
            auto edge = *edge_it;
            auto to_vertex = boost::target(edge, *pv);
            int to_node = vertex_to_node(*pv, to_vertex);
            prog(tmp.data(), pms.data() + k * to_node, d, pl_min);
            if (best_to == -1 || pm_less(tmp.data(), best.data(), d, pl_min)) {
                for (int i = 0; i < k; i++) {
                    best[i] = tmp[i];
                }
                best_to = to_node;
            }
        }

        strategy[node] = best_to;

        if (pm_less(pm, best.data(), d, pl_min)) {
            pm_copy(pm, best.data(), pl_min);
            if (pl_min)
                best_ch1 = best_to;
            else
                best_ch0 = best_to;
        }
    }

    bool ch0 = best_ch0 != -1;
    bool ch1 = best_ch1 != -1;

    if (ch0 && pm[0] == -1) {
        if ((d & 1) == 0)
            counts[d]--;
    }
    if (ch1 && pm[1] == -1) {
        if ((d & 1) == 1)
            counts[d]--;
    }

    if (ch0 || ch1) {
        lift_count++;
        return true;
    }

    lift_attempt++;
    return false;
}

void ProgressiveSmallProgressMeasuresSolver::update(int pl) {
    std::queue<int> q;

    for (int i = 0; i < boost::num_vertices(*pv); i++) {
        unstable[i] = 0;
        if (pms[k * i + pl] == -1 || canlift(i, pl)) {
            unstable[i] = 1;
            q.push(i);
        }
    }

    auto vertices = boost::vertices(*pv);

    while (!q.empty()) {
        int n = q.front();
        q.pop();
        auto vertex = node_to_vertex(*pv, n);

        for (auto vertex_it = vertices.first; vertex_it != vertices.second; ++vertex_it) {
            auto other_vertex = *vertex_it;
            auto outgoing_edges = boost::out_edges(other_vertex, *pv);
            for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
                auto edge = *edge_it;
                if (boost::target(edge, *pv) == vertex) {
                    int m = vertex_to_node(*pv, other_vertex);
                    if (unstable[m])
                        continue;

                    if ((*pv)[other_vertex].player != pl) {
                        int best_to = -1;
                        const int d = (*pv)[other_vertex].priority;
                        auto out_edges = boost::out_edges(other_vertex, *pv);

                        for (auto edge_it = out_edges.first; edge_it != out_edges.second; ++edge_it) {
                            auto out_edge = *edge_it;
                            auto to_vertex = boost::target(out_edge, *pv);
                            int to_node = vertex_to_node(*pv, to_vertex);
                            if (unstable[to_node])
                                continue;

                            prog(tmp.data(), pms.data() + k * to_node, d, pl);
                            if (best_to == -1 || pm_less(tmp.data(), best.data(), d, pl)) {
                                for (int i = 0; i < k; i++) {
                                    best[i] = tmp[i];
                                }
                                best_to = to_node;
                            }
                        }

                        if (best_to == -1)
                            continue;

                        if (pm_less(pms.data() + k * m, best.data(), d, pl))
                            continue;

                        unstable[m] = 1;
                        q.push(m);
                    }
                }
            }
        }
    }

    for (int i = 0; i < boost::num_vertices(*pv); i++) {
        if (unstable[i] == 0 && pms[k * i + 1 - pl] != -1) {
            auto vertex = node_to_vertex(*pv, i);
            if (((*pv)[vertex].priority & 1) != pl) {
                counts[(*pv)[vertex].priority]--;
                pms[k * i + 1 - pl] = -1;
                todo_push(i);
            }
        }
    }
}

void ProgressiveSmallProgressMeasuresSolver::todo_push(int node) {
    if (dirty[node] == 0) {
        dirty[node] = 1;
        todo.push(node);
    }
}

int ProgressiveSmallProgressMeasuresSolver::todo_pop() {
    int node = todo.front();
    todo.pop();
    dirty[node] = 0;
    return node;
}

int ProgressiveSmallProgressMeasuresSolver::vertex_to_node(const graph::Graph &game, graph::Vertex vertex) {
    auto vertices = boost::vertices(game);
    int index = 0;
    for (auto it = vertices.first; it != vertices.second; ++it) {
        if (*it == vertex)
            return index;
        index++;
    }
    return -1;
}

graph::Vertex ProgressiveSmallProgressMeasuresSolver::node_to_vertex(const graph::Graph &game, int node) {
    auto vertices = boost::vertices(game);
    int index = 0;
    for (auto it = vertices.first; it != vertices.second; ++it) {
        if (index == node)
            return *it;
        index++;
    }
    return graph::Vertex();
}

} // namespace parity
} // namespace ggg
