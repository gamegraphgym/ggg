#pragma once

#include "libggg/parity/graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <queue>
#include <string>
#include <vector>

namespace ggg {
namespace parity {

class ProgressiveSmallProgressMeasuresSolver : public ggg::solvers::Solver<graph::Graph, ggg::solutions::RSSolution<graph::Graph>> {
  public:
    ProgressiveSmallProgressMeasuresSolver() = default;
    ggg::solutions::RSSolution<graph::Graph> solve(const graph::Graph &graph) override;
    std::string get_name() const override { return "Progressive Small Progress Measures"; }

  private:
    const graph::Graph *pv;
    int k;
    std::vector<int> pms;
    std::vector<int> strategy;
    std::vector<int> counts;
    std::vector<int> tmp;
    std::vector<int> best;
    std::vector<int> dirty;
    std::vector<int> unstable;
    std::queue<int> todo;
    int64_t lift_count;
    int64_t lift_attempt;

    void init(const graph::Graph &game);
    bool pm_less(int *a, int *b, int d, int pl);
    void pm_copy(int *dst, int *src, int pl);
    void prog(int *dst, int *src, int d, int pl);
    bool canlift(int node, int pl);
    bool lift(int node, int target);
    void update(int pl);
    void todo_push(int node);
    int todo_pop();
    int vertex_to_node(const graph::Graph &game, graph::Vertex vertex);
    graph::Vertex node_to_vertex(const graph::Graph &game, int node);
};

} // namespace parity
} // namespace ggg
