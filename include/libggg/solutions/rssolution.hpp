#pragma once

#include "libggg/solutions/formatting_utils.hpp"
#include "libggg/solutions/rsolution.hpp"
#include "libggg/solutions/ssolution.hpp"
#include "libggg/strategy/deterministic.hpp"

namespace ggg {
namespace solutions {

/**
 * @class RSSolution
 * @brief Convenience solution combining Regions (R) and Strategies (S).
 *
 * Capability: R + S
 *
 * RSSolution aggregates `RSolution` and `SSolution` to provide both winning-region
 * information and positional strategies in a single type suitable as a solver return.
 */
template <typename GraphType, typename StrategyType = ggg::strategy::DeterministicStrategy<GraphType>>
class RSSolution : public RSolution<GraphType>, public SSolution<GraphType, StrategyType> {
  public:
    RSSolution() = default;

    std::string to_json() const {
        auto regions_member = detail::map_member_json<typename RSolution<GraphType>::Vertex>(
            "winning_regions", this->get_winning_regions(), [](int player) { return std::to_string(player); });
        auto strategy_member = detail::map_member_json<typename SSolution<GraphType, StrategyType>::Vertex>(
            "strategy", this->get_strategies(), [this](const StrategyType &s) { return ggg::strategy::to_json<GraphType>(s); });
        return detail::merge_json_members({regions_member, strategy_member});
    }

    friend std::ostream &operator<<(std::ostream &os, const RSSolution<GraphType, StrategyType> &sol) {
        detail::stream_map_label(os, "Winning regions", sol.get_winning_regions(), [](std::ostream &os, int player) -> std::ostream & { os << player; return os; });
        os << '\n';
        detail::stream_map_label(os, "Strategy", sol.get_strategies(), [&](std::ostream &os2, const StrategyType &s) -> std::ostream & { return ggg::strategy::to_stream<GraphType>(os2, s); });
        return os;
    }
};

} // namespace solutions
} // namespace ggg
