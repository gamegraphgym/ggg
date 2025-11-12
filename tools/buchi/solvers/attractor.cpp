#include "libggg/buechi/solvers/attractor.hpp"
#include "libggg/utils/solver_wrapper.hpp"

#include "libggg/graphs/graph_utilities.hpp"
#include "libggg/graphs/player_utilities.hpp"
#include "libggg/graphs/priority_utilities.hpp"
#include "libggg/graphs/validator.hpp"

// This is the only solver for Buechi games so far, so we do not bother introducing a special namespace for buechi graphs.
// instead, we recycle parity graphs and introduce a specialised validator here.

using BuechiGraphValidator = ggg::graphs::CompositeValidator<
    ggg::parity::graph::Graph,
    ggg::graphs::OutDegreeValidator<1>,
    ggg::graphs::NoDuplicateEdgesValidator,
    ggg::graphs::player_utilities::PlayerValidator<0, 1>,
    ggg::graphs::priority_utilities::PriorityValidator<0, 1>> // priorities must be 0 or 1 only (Buechi condition)

    ;

// Use the unified macro to create a main function for the Buchi solver
// note that it uses parity graphs
GGG_GAME_SOLVER_MAIN(ggg::parity::graph::Graph,
                     ggg::parity::graph::parse,
                     BuechiGraphValidator,
                     ggg::buechi::AttractorSolver)