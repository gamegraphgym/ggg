#include "libggg/mean_payoff/solvers/mse.hpp"
#include "libggg/utils/solver_wrapper.hpp"

using namespace ggg::mean_payoff;

// Use the unified macro to create a main function for the mean-payoff MSE solver
GGG_GAME_SOLVER_MAIN(graph::Graph, graph::parse, MSESolver)
