#pragma once

#include <boost/graph/graph_traits.hpp>
#include <concepts>
#include <functional>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "libggg/solutions/concepts.hpp"
#include "libggg/solutions/isolution.hpp"
#include "libggg/solutions/qsolution.hpp"
#include "libggg/solutions/rsolution.hpp"
#include "libggg/solutions/rsqsolution.hpp"
#include "libggg/solutions/rssolution.hpp"
#include "libggg/solutions/ssolution.hpp"
#include "libggg/strategy/deterministic.hpp"
#include "libggg/strategy/finite_memory.hpp"
#include "libggg/strategy/mixing.hpp"

namespace ggg {
namespace solvers {

// Concepts are defined in libggg/solutions/concepts.hpp; include that directly where needed.

// Solution types live in libggg/solutions/*.hpp; include and use directly in client code.

// =============================================================================
// Solver Interface
// =============================================================================

/**
 * @brief Generic solver interface for game graphs
 * @template GraphType The graph type (ParityGraph, mean_payoff::graph::Graph)
 * @template SolutionType The solution type returned by the solver
 */
template <typename GraphType, typename SolutionType>
class Solver {
  public:
    virtual ~Solver() = default;

    /**
     * @brief Solve the given game graph and return a solution
     * @param graph Game graph to solve
     * @return Solution of the specified type
     */
    virtual SolutionType solve(const GraphType &graph) = 0;

    /**
     * @brief Get solver name/description
     * @return String description of the solver
     */
    virtual std::string get_name() const = 0;
};

} // namespace solvers
} // namespace ggg
