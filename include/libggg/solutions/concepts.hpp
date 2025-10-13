#pragma once

#include "libggg/strategy/deterministic.hpp"
#include "libggg/strategy/finite_memory.hpp"
#include "libggg/strategy/mixing.hpp"
#include <boost/graph/graph_traits.hpp>
#include <concepts>

namespace ggg {
namespace solutions {

/**
 * @brief C++20 concept for solution types that provide winning regions
 *
 * Solutions satisfying this concept can determine which player wins each vertex
 * in the game graph.
 */
template <typename SolutionType, typename GraphType>
concept HasRegions = requires(const SolutionType &solution,
                              typename boost::graph_traits<GraphType>::vertex_descriptor vertex) {
    { solution.is_won_by_player0(vertex) } -> std::convertible_to<bool>;
    { solution.is_won_by_player1(vertex) } -> std::convertible_to<bool>;
    { solution.get_winning_player(vertex) } -> std::convertible_to<int>;
};

/**
 * @brief C++20 concept for valid strategy types
 */
template <typename StrategyType, typename GraphType>
concept DeterministicStrategyType = std::same_as<StrategyType, ggg::strategy::DeterministicStrategy<GraphType>>;

template <typename StrategyType, typename GraphType>
concept MixingStrategyType = std::same_as<StrategyType, ggg::strategy::MixingStrategy<GraphType>>;

template <typename StrategyType, typename GraphType>
concept FiniteMemoryStrategyType = std::same_as<StrategyType, ggg::strategy::FiniteMemoryStrategy<GraphType>>;

template <typename StrategyType, typename GraphType>
concept ValidStrategyType = DeterministicStrategyType<StrategyType, GraphType> ||
                            MixingStrategyType<StrategyType, GraphType> ||
                            FiniteMemoryStrategyType<StrategyType, GraphType>;

/**
 * @brief C++20 concept for solution types that provide strategic choices
 */
template <typename SolutionType, typename GraphType, typename StrategyType = ggg::strategy::DeterministicStrategy<GraphType>>
concept HasStrategy = requires(const SolutionType &solution,
                               typename boost::graph_traits<GraphType>::vertex_descriptor vertex) {
    { solution.get_strategy(vertex) } -> std::convertible_to<StrategyType>;
    { solution.has_strategy(vertex) } -> std::convertible_to<bool>;
} && ValidStrategyType<StrategyType, GraphType>;

/**
 * @brief C++20 concept for solution types that provide value mapping
 */
template <typename SolutionType, typename GraphType, typename ValueType = double>
concept HasValueMapping = requires(const SolutionType &solution,
                                   typename boost::graph_traits<GraphType>::vertex_descriptor vertex) {
    { solution.get_value(vertex) } -> std::convertible_to<ValueType>;
    { solution.has_value(vertex) } -> std::convertible_to<bool>;
};

/**
 * @brief C++20 concept for solution types that provide initial state status
 */
template <typename SolutionType>
concept HasInitialStateStatus = true;

} // namespace solutions
} // namespace ggg
