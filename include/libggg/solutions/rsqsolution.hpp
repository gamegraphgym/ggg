#pragma once

#include "libggg/solutions/formatting_utils.hpp"
#include "libggg/solutions/qsolution.hpp"
#include "libggg/solutions/rsolution.hpp"
#include "libggg/solutions/ssolution.hpp"
#include "libggg/strategy/deterministic.hpp"

namespace ggg {
namespace solutions {

/**
 * @class RSQSolution
 * @brief Convenience solution combining Regions, Strategies and Quantitative values.
 *
 * Capability: R + S + Q
 *
 * RSQSolution aggregates `RSolution`, `SSolution` and `QSolution` to provide
 * regions, strategies and per-vertex quantitative values in a single type.
 */
template <typename GraphType, typename StrategyType = ggg::strategy::DeterministicStrategy<GraphType>, typename ValueType = double>
class RSQSolution : public RSolution<GraphType>, public SSolution<GraphType, StrategyType>, public QSolution<GraphType, ValueType> {
  public:
    RSQSolution() = default;

    std::string to_json() const {
        auto regions_member = detail::map_member_json<typename RSolution<GraphType>::Vertex>(
            "winning_regions", this->get_winning_regions(), [](int player) { return std::to_string(player); });
        auto strategy_member = detail::map_member_json<typename SSolution<GraphType, StrategyType>::Vertex>(
            "strategy", this->get_strategies(), [this](const StrategyType &s) { return ggg::strategy::to_json<GraphType>(s); });
        auto values_member = detail::map_member_json<typename QSolution<GraphType, ValueType>::Vertex>(
            "values", this->get_values(), [](const ValueType &v) { return std::to_string(v); });
        return detail::merge_json_members({regions_member, strategy_member, values_member});
    }

    friend std::ostream &operator<<(std::ostream &os, const RSQSolution<GraphType, StrategyType, ValueType> &sol) {
        detail::stream_map_label(os, "Winning regions", sol.get_winning_regions(), [](std::ostream &os, int player) -> std::ostream & { os << player; return os; });
        os << ' ';
        detail::stream_map_label(os, "Strategy", sol.get_strategies(), [&](std::ostream &os2, const StrategyType &s) -> std::ostream & { return ggg::strategy::to_stream<GraphType>(os2, s); });
        os << ' ';
        detail::stream_map_label(os, "Values", sol.get_values(), [](std::ostream &os, const ValueType &v) -> std::ostream & { os << v; return os; });
        return os;
    }
};

} // namespace solutions
} // namespace ggg
