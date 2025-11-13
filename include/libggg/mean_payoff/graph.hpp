#pragma once
#include "libggg/graphs/graph_utilities.hpp"
#include "libggg/graphs/player_utilities.hpp"

namespace ggg {
namespace mean_payoff {
namespace graph {

// Mean-payoff graph property field lists
#define MEAN_PAYOFF_VERTEX_FIELDS(X) \
    X(std::string, name, "")         \
    X(int, player, -1)               \
    X(int, weight, 0)

#define MEAN_PAYOFF_EDGE_FIELDS(X) \
    X(std::string, label, "")

#define MEAN_PAYOFF_GRAPH_FIELDS(X) /* none */

// Instantiate Graph/parse/write in ggg::mean_payoff::graph
DEFINE_GAME_GRAPH(MEAN_PAYOFF_VERTEX_FIELDS, MEAN_PAYOFF_EDGE_FIELDS, MEAN_PAYOFF_GRAPH_FIELDS)

#undef MEAN_PAYOFF_VERTEX_FIELDS
#undef MEAN_PAYOFF_EDGE_FIELDS
#undef MEAN_PAYOFF_GRAPH_FIELDS

// Standard validators for mean-payoff graphs
using graphs::NoDuplicateEdgesValidator;
using graphs::OutDegreeValidator;
using graphs::player_utilities::PlayerValidator;

/**
 * @brief Standard composite validator for 2-player mean-payoff games
 *
 * This validator checks:
 * - Players are either 0 or 1
 * - All vertices have at least one outgoing edge
 * - No duplicate edges exist
 */
using StandardValidator = graphs::CompositeValidator<
    Graph,
    PlayerValidator<0, 1>,
    OutDegreeValidator<1>,
    NoDuplicateEdgesValidator>;

} // namespace graph
} // namespace mean_payoff
} // namespace ggg
