#include "libggg/stochastic_discounted/solvers/objective.hpp"
#include "libggg/utils/solver_wrapper.hpp"

using namespace ggg::stochastic_discounted;

// Use the unified macro to create a main function for the stochastic discounted objective improvement solver
GGG_GAME_SOLVER_MAIN(graph::Graph, graph::parse, StochasticDiscountedObjectiveSolver)
