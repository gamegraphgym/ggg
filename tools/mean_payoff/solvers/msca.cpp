#include "libggg/mean_payoff/solvers/msca.hpp"
#include "libggg/utils/solver_wrapper.hpp"

using namespace ggg::mean_payoff;

// Use the unified macro to create a main function for the mean-payoff MSCA solver
GGG_GAME_SOLVER_MAIN(graph::Graph, graph::parse, MSCASolver)
