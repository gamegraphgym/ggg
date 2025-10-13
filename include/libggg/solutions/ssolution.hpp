#pragma once

#include "libggg/solutions/formatting_utils.hpp"
#include "libggg/solutions/isolution.hpp"
#include "libggg/strategy/deterministic.hpp"
#include "libggg/strategy/finite_memory.hpp"
#include "libggg/strategy/mixing.hpp"
#include <boost/graph/graph_traits.hpp>
#include <map>

namespace ggg {
namespace solutions {

/**
 * @class SSolution
 * @brief Solution that stores a winning strategy.
 *
 * Capability: S (Strategies)
 *
 * SSolution provides accessors to set, query and retrieve strategies for vertices.
 * It emits strategy information in JSON/text form using the project's strategy helpers.
 */
template <typename GraphType, typename StrategyType = ggg::strategy::DeterministicStrategy<GraphType>>
class SSolution : public virtual ISolution {
  public:
    using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;
    using Strategy = StrategyType;

  protected:
    std::map<Vertex, StrategyType> strategy_;

  public:
    SSolution() = default;

    StrategyType get_strategy(Vertex vertex) const {
        const auto it = strategy_.find(vertex);
        if (it != strategy_.end()) {
            return it->second;
        }
        if constexpr (std::is_same_v<StrategyType, ggg::strategy::DeterministicStrategy<GraphType>>) {
            return boost::graph_traits<GraphType>::null_vertex();
        } else {
            return StrategyType{};
        }
    }
    bool has_strategy(Vertex vertex) const { return strategy_.find(vertex) != strategy_.end(); }
    void set_strategy(Vertex vertex, const StrategyType &strategy) { strategy_[vertex] = strategy; }
    const std::map<Vertex, StrategyType> &get_strategies() const { return strategy_; }

    std::string to_json() const {
        auto member = detail::map_member_json<Vertex>("strategy", strategy_, [](const StrategyType &s) { return ggg::strategy::to_json<GraphType>(s); });
        return detail::merge_json_members({member});
    }

    friend std::ostream &operator<<(std::ostream &os, const SSolution<GraphType, StrategyType> &sol) {
        return detail::stream_map_label(os, "Strategy", sol.strategy_, [](std::ostream &os, const StrategyType &s) -> std::ostream & { return ggg::strategy::to_stream<GraphType>(os, s); });
    }
};

} // namespace solutions
} // namespace ggg
