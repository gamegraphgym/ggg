#include "libggg/stochastic_discounted/solvers/strategy.hpp"
#include "libggg/utils/solver_wrapper.hpp"

using namespace ggg::stochastic_discounted;

// Use the unified macro to create a main function for the discounted strategy improvement solver
GGG_GAME_SOLVER_MAIN(graph::Graph, graph::parse, StochasticDiscountedStrategySolver)
