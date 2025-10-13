#include "libggg/mean_payoff/solvers/msca.hpp"
#include "libggg/mean_payoff/graph.hpp"
#include "libggg/utils/logging.hpp"
#include <algorithm>
#include <cmath>

namespace ggg {
namespace mean_payoff {

ggg::solutions::RSQSolution<graph::Graph, ggg::strategy::DeterministicStrategy<graph::Graph>, long long>
MSCASolver::solve(const graph::Graph &graph) {
    LGG_DEBUG("MSCA solver starting with ", boost::num_vertices(graph), " vertices");

    ggg::solutions::RSQSolution<graph::Graph, ggg::strategy::DeterministicStrategy<graph::Graph>, long long> solution;

    if (boost::num_vertices(graph) == 0) {
        LGG_TRACE("Empty game - returning solved");

        return solution;
    }

    init(graph);

    bool empty_fun = is_empty();

    if (!empty_fun) {
        compute_energy();

        const auto [vertices_begin, vertices_end] = boost::vertices(graph);
        for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
            int idx = vertex_to_index_[vertex];

            if (msrfun_[idx] >= (nw_ / 2)) {
                solution.set_winning_player(vertex, 1);
            } else {
                solution.set_winning_player(vertex, 0);
                if (strategy_[idx] != boost::graph_traits<graph::Graph>::null_vertex()) {
                    solution.set_strategy(vertex, strategy_[idx]);
                }
            }

            solution.set_value(vertex, msrfun_[idx]);
        }
    } else {
        const auto [vertices_begin, vertices_end] = boost::vertices(graph);
        for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
            solution.set_winning_player(vertex, 0);
            solution.set_value(vertex, 0);

            if (graph[vertex].player == 0) {
                const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
                if (out_edges_begin != out_edges_end) {
                    solution.set_strategy(vertex, boost::target(*out_edges_begin, graph));
                }
            }
        }
    }

    LGG_TRACE("MSCA solved with ", count_update_, " updates");
    LGG_TRACE("               ", count_delta_, " delta lifts");
    LGG_TRACE("               ", count_scaling_, " scaling operations");
    LGG_TRACE("               ", count_update_ + count_delta_ + count_scaling_, " total operations");
    LGG_TRACE("               ", count_iter_delta_, " delta iterations");
    LGG_TRACE("               ", count_super_delta_, " effective deltas");
    LGG_TRACE("               ", count_null_delta_, " null deltas");
    LGG_TRACE("               ", max_delta_, " maximum delta");

    return solution;
}

void MSCASolver::init(const graph::Graph &graph) {
    graph_ = &graph;

    scaling_val_ = 1;
    delta_value_ = 0;
    working_vertex_index_ = 0;

    count_update_ = 0;
    count_delta_ = 0;
    count_iter_delta_ = 0;
    count_super_delta_ = 0;
    count_null_delta_ = 0;
    count_scaling_ = 0;
    max_delta_ = 0;

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    int vertex_count = boost::num_vertices(graph);

    vertex_to_index_.clear();
    index_to_vertex_.resize(vertex_count);

    int idx = 0;
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        vertex_to_index_[vertex] = idx;
        index_to_vertex_[idx] = vertex;
        idx++;
    }

    weight_.resize(vertex_count);
    msrfun_.resize(vertex_count);
    count_.resize(vertex_count);
    strategy_.resize(vertex_count);

    rescaled_.resize(vertex_count);
    setL_.resize(vertex_count);
    setB_.resize(vertex_count);

    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        int idx = vertex_to_index_[vertex];
        strategy_[idx] = boost::graph_traits<graph::Graph>::null_vertex();
        msrfun_[idx] = 0;
        count_[idx] = 0;
        weight_[idx] = graph[vertex].weight;
    }

    nw_ = calc_n_w();
}

long long MSCASolver::calc_n_w() {
    long long max_weight = 0;
    const auto [vertices_begin, vertices_end] = boost::vertices(*graph_);

    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        long long abs_weight = std::abs(static_cast<long long>((*graph_)[vertex].weight));
        max_weight = std::max(max_weight, abs_weight);
    }

    return max_weight;
}

long long MSCASolver::wf(int predecessor_idx, int successor_idx) {
    if (rescaled_[predecessor_idx]) {
        return std::ceil(static_cast<double>(weight_[successor_idx]) / static_cast<double>(scaling_val_)) +
               msrfun_[predecessor_idx] - msrfun_[successor_idx];
    } else {
        double scaled_weight = static_cast<double>(weight_[successor_idx]) / static_cast<double>(scaling_val_);
        double double_scaled = static_cast<double>(weight_[successor_idx]) / static_cast<double>(scaling_val_ * 2);

        if ((2 * std::ceil(double_scaled)) > std::ceil(scaled_weight)) {
            return std::ceil(scaled_weight) + 1 + msrfun_[predecessor_idx] - msrfun_[successor_idx];
        } else {
            return -2 * nw_;
        }
    }
}

long long MSCASolver::delta_p1() {
    long long max_d2 = -2 * nw_;

    for (std::size_t pos = setB_.find_first(); pos != Bitset::npos && pos < setB_.size(); pos = setB_.find_next(pos)) {
        const auto &vertex = index_to_vertex_[pos];

        if ((*graph_)[vertex].player == 0) {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, *graph_);
            for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto &successor = boost::target(edge, *graph_);
                int successor_idx = vertex_to_index_[successor];

                if (!setB_[successor_idx]) {
                    long long edge_weight = wf(pos, successor_idx);
                    max_d2 = std::max(max_d2, edge_weight);
                }
            }
        }
    }

    return -max_d2;
}

long long MSCASolver::delta_p2() {
    long long min_d2 = 2 * nw_;
    long long min_d3 = 2 * nw_;

    setB_.flip();

    for (std::size_t pos = setB_.find_first(); pos != Bitset::npos && pos < setB_.size(); pos = setB_.find_next(pos)) {
        const auto &vertex = index_to_vertex_[pos];
        long long max_d2 = -2 * nw_;

        if ((*graph_)[vertex].player == 0) {
            bool enable2 = true;

            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, *graph_);
            for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto &successor = boost::target(edge, *graph_);
                int successor_idx = vertex_to_index_[successor];

                if (enable2 && setB_[successor_idx] && wf(pos, successor_idx) >= 0) {
                    enable2 = false;
                }
            }

            if (enable2) {
                for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                    const auto &successor = boost::target(edge, *graph_);
                    int successor_idx = vertex_to_index_[successor];
                    max_d2 = std::max(max_d2, wf(pos, successor_idx));
                }
                min_d2 = std::min(min_d2, max_d2);
            }
        } else {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, *graph_);
            for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto &successor = boost::target(edge, *graph_);
                int successor_idx = vertex_to_index_[successor];

                if (!setB_[successor_idx]) {
                    min_d3 = std::min(min_d3, wf(pos, successor_idx));
                }
            }
        }
    }

    setB_.flip();
    return std::min(min_d2, min_d3);
}

void MSCASolver::delta() {
    long long d1 = delta_p1();
    long long d2 = delta_p2();
    delta_value_ = std::min(d1, d2);
}

void MSCASolver::update_func(int pos) {
    setL_[pos] = false;
    ++msrfun_[pos];
    count_update_++;

    const auto &vertex = index_to_vertex_[pos];

    if ((*graph_)[vertex].player == 0) {
        count_[pos] = 0;
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, *graph_);
        for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            const auto &successor = boost::target(edge, *graph_);
            int successor_idx = vertex_to_index_[successor];
            if (wf(pos, successor_idx) >= 0) {
                ++count_[pos];
            }
        }
    }

    const auto [all_vertices_begin, all_vertices_end] = boost::vertices(*graph_);
    for (const auto &pred_vertex : boost::make_iterator_range(all_vertices_begin, all_vertices_end)) {
        int pred_idx = vertex_to_index_[pred_vertex];

        const auto [out_edges_begin, out_edges_end] = boost::out_edges(pred_vertex, *graph_);
        for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            if (boost::target(edge, *graph_) == vertex) {
                if (wf(pred_idx, pos) < 0) {
                    if ((*graph_)[pred_vertex].player == 0) {
                        if (wf(pred_idx, pos) == -1) {
                            --count_[pred_idx];
                            if (count_[pred_idx] == 0) {
                                setL_[pred_idx] = true;
                                setB_[pred_idx] = true;
                            }
                        }
                    } else {
                        setL_[pred_idx] = true;
                        setB_[pred_idx] = true;
                        strategy_[pred_idx] = vertex;
                    }
                }
                break;
            }
        }
    }
}

void MSCASolver::update_energy() {
    const auto &vertex = index_to_vertex_[working_vertex_index_];
    bool valid;

    if ((*graph_)[vertex].player == 0) {
        valid = true;
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, *graph_);
        for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            const auto &successor = boost::target(edge, *graph_);
            int successor_idx = vertex_to_index_[successor];
            if (valid && wf(working_vertex_index_, successor_idx) >= 0) {
                valid = false;
            }
        }
    } else {
        valid = false;
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, *graph_);
        for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            const auto &successor = boost::target(edge, *graph_);
            int successor_idx = vertex_to_index_[successor];
            if (!valid && wf(working_vertex_index_, successor_idx) < 0) {
                valid = true;
            }
        }
    }

    setL_.reset();
    setB_.reset();

    if (valid) {
        setL_[working_vertex_index_] = true;

        while ((setL_.count() == 1) && setL_[working_vertex_index_]) {

            const auto [vertices_begin, vertices_end] = boost::vertices(*graph_);
            for (const auto &v : boost::make_iterator_range(vertices_begin, vertices_end)) {
                int v_idx = vertex_to_index_[v];
                if ((*graph_)[v].player == 0) {
                    count_[v_idx] = 0;
                    const auto [out_edges_begin, out_edges_end] = boost::out_edges(v, *graph_);
                    for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                        const auto &successor = boost::target(edge, *graph_);
                        int successor_idx = vertex_to_index_[successor];
                        if (wf(v_idx, successor_idx) >= 0) {
                            ++count_[v_idx];
                        }
                    }
                }
            }

            update_func(working_vertex_index_);

            while (((setL_.count() > 1) || ((setL_.count() == 1) && !setL_[working_vertex_index_]))) {
                bool pick = false;
                std::size_t ind = setL_.find_first();
                while (!pick && ind != Bitset::npos) {
                    if (ind < setL_.size() && ind != static_cast<std::size_t>(working_vertex_index_)) {
                        pick = true;
                    } else {
                        ind = setL_.find_next(ind);
                    }
                }
                if (pick && ind != Bitset::npos) {
                    update_func(ind);
                } else {
                    LGG_DEBUG("No more vertices to update, exiting loop");
                    break;
                }
            }

            bool delta_root = false;
            if ((*graph_)[vertex].player == 0) {
                if (count_[working_vertex_index_] < 1) {
                    delta_root = true;
                }
            } else {
                const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, *graph_);
                for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                    const auto &successor = boost::target(edge, *graph_);
                    int successor_idx = vertex_to_index_[successor];
                    if (!delta_root && wf(working_vertex_index_, successor_idx) < 0) {
                        delta_root = true;
                    }
                }
            }

            if (delta_root) {
                delta();
                if (delta_value_ > 0) {
                    bool enable_d = false;
                    for (std::size_t pos = setB_.find_first(); pos != Bitset::npos && pos < setB_.size(); pos = setB_.find_next(pos)) {
                        msrfun_[pos] += delta_value_;
                        count_delta_++;
                        enable_d = true;
                    }

                    if (enable_d) {
                        if (delta_value_ > 1) {
                            count_super_delta_++;
                        }
                        if (delta_value_ > static_cast<long long>(max_delta_)) {
                            max_delta_ = static_cast<unsigned long>(delta_value_);
                        }
                    }
                    count_iter_delta_++;
                } else {
                    count_null_delta_++;
                }
            }
        }
    }
}

void MSCASolver::compute_energy() {
    bool neg = false;
    working_vertex_index_ = 0;

    while (!neg && working_vertex_index_ < static_cast<int>(weight_.size())) {
        double scaled_weight = static_cast<double>(weight_[working_vertex_index_]) / static_cast<double>(scaling_val_);
        double double_scaled = static_cast<double>(weight_[working_vertex_index_]) / static_cast<double>(scaling_val_ * 2);

        if ((2 * std::ceil(double_scaled)) > std::ceil(scaled_weight)) {
            if (std::ceil(scaled_weight) - 1 < 0) {
                neg = true;
            } else {
                working_vertex_index_++;
            }
        } else if (std::ceil(scaled_weight) < 0) {
            neg = true;
        } else {
            working_vertex_index_++;
        }
    }

    if (neg) {
        count_scaling_ += boost::num_edges(*graph_);
        scaling_val_ *= 2;
        compute_energy();
        scaling_val_ /= 2;
        count_scaling_ += boost::num_edges(*graph_);
        count_scaling_ += boost::num_vertices(*graph_);

        for (std::size_t pos = 0; pos < msrfun_.size(); ++pos) {
            msrfun_[pos] *= 2;
        }

        rescaled_.reset();
        for (working_vertex_index_ = 0; working_vertex_index_ < static_cast<int>(weight_.size()); ++working_vertex_index_) {
            rescaled_[working_vertex_index_] = true;
            update_energy();
        }
    }
}

bool MSCASolver::is_empty() const {
    const auto [vertices_begin, vertices_end] = boost::vertices(*graph_);
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if ((*graph_)[vertex].weight != 0) {
            return false;
        }
    }
    return true;
}

void MSCASolver::reset() {
    vertex_to_index_.clear();
    index_to_vertex_.clear();
    weight_.clear();
    msrfun_.clear();
    count_.clear();
    strategy_.clear();
    rescaled_.clear();
    setL_.clear();
    setB_.clear();
}

} // namespace mean_payoff
} // namespace ggg
