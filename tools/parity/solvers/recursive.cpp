#include "libggg/parity/solvers/recursive.hpp"
#include "libggg/utils/solver_wrapper.hpp"


using namespace ggg::parity;

// Unified macro to create a main function for the recursive parity solver
GGG_GAME_SOLVER_MAIN(graph::Graph, graph::parse, RecursiveParitySolver)