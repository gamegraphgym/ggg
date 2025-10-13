#include "libggg/buechi/solvers/attractor.hpp"
#include "libggg/utils/solver_wrapper.hpp"

using namespace ggg::parity;

// Use the unified macro to create a main function for the Buchi solver
// note that it uses parity graphs
GGG_GAME_SOLVER_MAIN(graph::Graph, graph::parse, ggg::buechi::AttractorSolver)
