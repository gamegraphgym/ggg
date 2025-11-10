#pragma once
#include "libggg/graphs/graph_utilities.hpp"
#include "libggg/graphs/player_utilities.hpp"
#include "libggg/graphs/probability_utilities.hpp"
#include "libggg/graphs/discount_utilities.hpp"
#include "libggg/graphs/validator.hpp"
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

/**
 * @brief Validator for cycles in filtered vertices
 *
 * Checks that vertices matching the filter predicate form an acyclic subgraph.
 * This is used to ensure that player 1 vertices don't form cycles in stochastic games.
 */
struct CycleValidator {
    template <typename GraphType>
    static void validate(const GraphType &graph) {
        auto fg = boost::make_filtered_graph(graph, boost::keep_all{}, VertexFilter(graph));
        CycleDetector vis{};
        boost::depth_first_search(fg, boost::visitor(vis));
        if (vis.has_cycle) {
            throw graphs::GraphValidationError("Cycle detected in player 1 vertices (not allowed in stochastic discounted games)");
        }
    }
};

// Standard validators for stochastic discounted graphs
using graphs::player_utilities::PlayerValidator;
using graphs::probability_utilities::ProbabilityValidator;
using graphs::discount_utilities::DiscountValidator;
using graphs::OutDegreeValidator;
using graphs::NoDuplicateEdgesValidator;

// Specialized validator wrapper that applies probability and discount validators with filters
struct FilteredValidator {
    template <typename GraphType>
    static void validate(const GraphType &graph) {
        // Validate probabilities only for probabilistic vertices (player == -1)
        auto prob_filter = [](const GraphType &g, auto v) { return g[v].player == -1; };
        ProbabilityValidator::validate(graph, prob_filter);
        
        // Validate discounts only for non-probabilistic vertices (player != -1)
        auto discount_filter = [](const GraphType &g, auto v) { return g[v].player != -1; };
        DiscountValidator::validate(graph, discount_filter);
    }
};

/**
 * @brief Standard composite validator for stochastic discounted games
 *
 * This validator checks:
 * - Players are -1 (probabilistic), 0, or 1
 * - All vertices have at least one outgoing edge
 * - Probabilities on edges from probabilistic vertices (player == -1) are in (0,1] and sum to 1.0
 * - Discount factors on edges from non-probabilistic vertices (player != -1) are in (0,1)
 * - No duplicate edges exist
 * - No cycles exist within player 1 vertices
 */
using StandardValidator = graphs::CompositeValidator<
    Graph,
    PlayerValidator<-1, 0, 1>,
    OutDegreeValidator<1>,
    FilteredValidator,
    NoDuplicateEdgesValidator,
    CycleValidator>;

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
