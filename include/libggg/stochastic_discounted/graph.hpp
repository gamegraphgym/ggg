#pragma once
#include "libggg/graphs/graph_utilities.hpp"
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>

namespace ggg {
namespace stochastic_discounted {
namespace graph {

// Property field lists
#define STOCHASTIC_DISCOUNTED_VERTEX_FIELDS(X) \
    X(std::string, name)                       \
    X(int, player)

#define STOCHASTIC_DISCOUNTED_EDGE_FIELDS(X) \
    X(std::string, label)                    \
    X(double, weight)                        \
    X(double, discount)                      \
    X(double, probability)

#define STOCHASTIC_DISCOUNTED_GRAPH_FIELDS(X) /* none */

// Instantiate Graph/parse/write in ggg::stochastic_discounted::graph
DEFINE_GAME_GRAPH(STOCHASTIC_DISCOUNTED_VERTEX_FIELDS, STOCHASTIC_DISCOUNTED_EDGE_FIELDS, STOCHASTIC_DISCOUNTED_GRAPH_FIELDS)

#undef STOCHASTIC_DISCOUNTED_VERTEX_FIELDS
#undef STOCHASTIC_DISCOUNTED_EDGE_FIELDS
#undef STOCHASTIC_DISCOUNTED_GRAPH_FIELDS

// Utilities analogous to parity/mean_payoff headers, adapted for discounted games

/**
 * Find vertex by name, or return null_vertex if not found
 */
inline Vertex find_vertex(const Graph &g, const std::string &name) {
    const auto [vb, ve] = boost::vertices(g);
    const auto it = std::find_if(vb, ve, [&g, &name](const auto &v) { return g[v].name == name; });
    return (it != ve) ? *it : boost::graph_traits<Graph>::null_vertex();
}

struct VertexFilter {
    const Graph *g{};
    VertexFilter() = default;
    explicit VertexFilter(const Graph &graph) : g(&graph) {}
    bool operator()(Vertex v) const { return (*g)[v].player == 1; }
};

struct CycleDetector : public boost::dfs_visitor<> {
    bool has_cycle = false;
    template <typename EdgeT, typename GraphT>
    void back_edge(EdgeT, const GraphT &) {
        has_cycle = true;
    }
};

inline bool is_valid(const Graph &g) {
    const auto [vb, ve] = boost::vertices(g);

    // Vertex checks
    for (const auto &v : boost::make_iterator_range(vb, ve)) {
        if (g[v].player != -1 && g[v].player != 0 && g[v].player != 1) {
            return false;
        }
        if (boost::out_degree(v, g) == 0) {
            return false;
        }
    }

    // Edge checks
    const auto [eb, ee] = boost::edges(g);
    for (const auto &e : boost::make_iterator_range(eb, ee)) {
        auto s = boost::source(e, g);
        if (g[s].player != -1) {
            const auto d = g[e].discount;
            if ((d <= 0.0) || (d >= 1.0)) {
                return false;
            }
        }
    }

    // Probabilities from probabilistic vertices must be in (0,1] and sum to 1
    for (const auto &v : boost::make_iterator_range(vb, ve)) {
        if (g[v].player == -1) {
            double sum = 0.0;
            for (const auto &e : boost::make_iterator_range(boost::out_edges(v, g))) {
                const auto p = g[e].probability;
                if ((p <= 0.0) || (p > 1.0)) {
                    return false;
                }
                sum += p;
            }
            if (std::abs(sum - 1.0) > 1e-8) {
                return false;
            }
        }
    }

    // Check for cycles within filtered vertices (currently: player==1)
    auto fg = boost::make_filtered_graph(g, boost::keep_all{}, VertexFilter(g));
    CycleDetector vis{};
    boost::depth_first_search(fg, boost::visitor(vis));
    if (vis.has_cycle) {
        return false;
    }

    return true;
}

inline void check_no_duplicate_edges(const Graph &g) {
    std::set<std::pair<Vertex, Vertex>> seen;
    const auto [eb, ee] = boost::edges(g);
    for (const auto &e : boost::make_iterator_range(eb, ee)) {
        auto s = boost::source(e, g);
        auto t = boost::target(e, g);
        const auto key = std::make_pair(s, t);
        if (!seen.insert(key).second) {
            throw std::runtime_error("Duplicate edge found between vertices '" + g[s].name + "' and '" + g[t].name + "'");
        }
    }
}

inline double get_min_discount(const Graph &g) {
    if (boost::num_edges(g) == 0) {
        return 1.0;
    }
    const auto [eb, ee] = boost::edges(g);
    const auto it = std::min_element(eb, ee, [&g](const auto &a, const auto &b) { return g[a].discount < g[b].discount; });
    return g[*it].discount;
}

inline double get_max_discount(const Graph &g) {
    if (boost::num_edges(g) == 0) {
        return 0.0;
    }
    const auto [eb, ee] = boost::edges(g);
    const auto it = std::max_element(eb, ee, [&g](const auto &a, const auto &b) { return g[a].discount < g[b].discount; });
    return g[*it].discount;
}

inline std::map<double, int> get_weight_distribution(const Graph &g) {
    std::map<double, int> dist;
    const auto [eb, ee] = boost::edges(g);
    std::for_each(eb, ee, [&g, &dist](const auto &e) { dist[g[e].weight]++; });
    return dist;
}

inline auto get_non_probabilistic_vertices(const Graph &g) {
    const auto [vb, ve] = boost::vertices(g);
    return boost::make_iterator_range(vb, ve) | boost::adaptors::filtered([&g](const auto &v) { return g[v].player != -1; });
}

inline std::map<Vertex, double> get_reachable_through_probabilistic(const Graph &g, Vertex source, Vertex successor) {
    std::map<Vertex, double> reachable;
    if (g[source].player == -1) {
        return reachable;
    }

    std::queue<std::pair<Vertex, double>> q;
    std::set<Vertex> visited;

    if (g[successor].player == -1) {
        q.push({successor, 1.0});
    } else {
        reachable[successor] = 1.0;
    }

    while (!q.empty()) {
        auto [cur, prob] = q.front();
        q.pop();
        if (visited.count(cur)) {
            continue;
        }
        visited.insert(cur);

        const auto [ob, oe] = boost::out_edges(cur, g);
        for (auto it = ob; it != oe; ++it) {
            auto succ = boost::target(*it, g);
            double edge_prob = g[*it].probability;
            double total = prob * edge_prob;
            if (g[succ].player == -1) {
                if (!visited.count(succ)) {
                    q.push({succ, total});
                }
            } else {
                reachable[succ] += total;
            }
        }
    }
    return reachable;
}

} // namespace graph
} // namespace stochastic_discounted
} // namespace ggg
