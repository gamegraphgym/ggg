#pragma once

#include "libggg/mean_payoff/graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <boost/dynamic_bitset.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <map>
#include <vector>

namespace ggg {
namespace mean_payoff {

using MSCASolutionType = ggg::solutions::RSQSolution<graph::Graph, ggg::strategy::DeterministicStrategy<graph::Graph>, long long>;

class MSCASolver : public ggg::solvers::Solver<graph::Graph, MSCASolutionType> {
  public:
    ggg::solutions::RSQSolution<graph::Graph, ggg::strategy::DeterministicStrategy<graph::Graph>, long long> solve(const graph::Graph &graph) override;
    std::string get_name() const override { return "MSCA (Mean-payoff Solver with Constraint Analysis) Solver"; }

  private:
    using Vertex = graph::Graph::vertex_descriptor;
    using Bitset = boost::dynamic_bitset<unsigned long long>;

    long long scaling_val_;
    long long delta_value_;
    long long nw_;
    int working_vertex_index_;

    std::map<Vertex, int> vertex_to_index_;
    std::vector<Vertex> index_to_vertex_;

    std::vector<long long> weight_;
    std::vector<long long> msrfun_;
    std::vector<int> count_;
    std::vector<Vertex> strategy_;
    Bitset rescaled_;
    Bitset setL_;
    Bitset setB_;

    unsigned long count_update_;
    unsigned long count_delta_;
    unsigned long count_iter_delta_;
    unsigned long count_super_delta_;
    unsigned long count_null_delta_;
    unsigned long count_scaling_;
    unsigned long max_delta_;

    const graph::Graph *graph_;

    void init(const graph::Graph &graph);
    long long calc_n_w();
    long long wf(int predecessor_idx, int successor_idx);
    long long delta_p1();
    long long delta_p2();
    void delta();
    void update_func(int pos);
    void update_energy();
    void compute_energy();

    bool is_empty() const;
    void reset();
};

} // namespace mean_payoff
} // namespace ggg
