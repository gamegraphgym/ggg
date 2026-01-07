#pragma once
#include "libggg/graphs/graph_utilities.hpp"
#include "libggg/graphs/player_utilities.hpp"
#include "libggg/graphs/priority_utilities.hpp"

namespace ggg {
namespace parity {
namespace graph {

// Parity graph property field lists
#define PARITY_VERTEX_FIELDS(X) \
    X(std::string, name, "")    \
    X(int, player, -1)          \
    X(int, priority, -1)

#define PARITY_EDGE_FIELDS(X) \
    X(std::string, label, "")

#define PARITY_GRAPH_FIELDS(X) /* none */

// Instantiate Graph/parse/write in ggg::parity::graph
DEFINE_GAME_GRAPH(PARITY_VERTEX_FIELDS, PARITY_EDGE_FIELDS, PARITY_GRAPH_FIELDS)

#undef PARITY_VERTEX_FIELDS
#undef PARITY_EDGE_FIELDS
#undef PARITY_GRAPH_FIELDS

// Standard validators for parity graphs
using graphs::NoDuplicateEdgesValidator;
using graphs::OutDegreeValidator;
using graphs::player_utilities::PlayerValidator;
using graphs::priority_utilities::PriorityValidator;

/**
 * @brief Standard composite validator for 2-player turn-based parity games
 *
 * This validator checks:
 * - Players are either 0 or 1
 * - Priorities are non-negative
 * - All vertices have at least one outgoing edge
 * - No duplicate edges exist
 */
using StandardValidator = graphs::CompositeValidator<
    Graph,
    PlayerValidator<0, 1>,
    PriorityValidator<0>,
    OutDegreeValidator<1>,
    NoDuplicateEdgesValidator>;

} // namespace graph
} // namespace parity
} // namespace ggg
