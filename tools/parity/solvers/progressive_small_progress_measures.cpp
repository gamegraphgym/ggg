#include "libggg/parity/solvers/progressive_small_progress_measures.hpp"
#include "libggg/utils/solver_wrapper.hpp"

using namespace ggg::parity;

// Unified macro to create a main function for the Progressive Small Progress Measures parity solver
GGG_GAME_SOLVER_MAIN(graph::Graph, graph::parse, ProgressiveSmallProgressMeasuresSolver)
