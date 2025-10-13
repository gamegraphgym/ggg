#pragma once

#include "libggg/solutions/formatting_utils.hpp"
#include "libggg/solutions/isolution.hpp"
#include <boost/graph/graph_traits.hpp>
#include <map>

namespace ggg {
namespace solutions {

/**
 * @class RSolution
 * @brief Solution that stores per-vertex winning-region information.
 *
 * Capability: R (Regions)
 *
 * RSolution provides methods to query and set the winning player for individual
 * vertices and to obtain the full map of winning regions. It also implements
 * JSON/text conversion helpers.
 */
template <typename GraphType>
class RSolution : public virtual ISolution {
  public:
    using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;

  protected:
    std::map<Vertex, int> winning_regions_;

  public:
    RSolution() = default;

    bool is_won_by_player0(Vertex vertex) const {
        const auto it = winning_regions_.find(vertex);
        return it != winning_regions_.end() && it->second == 0;
    }
    bool is_won_by_player1(Vertex vertex) const {
        const auto it = winning_regions_.find(vertex);
        return it != winning_regions_.end() && it->second == 1;
    }
    int get_winning_player(Vertex vertex) const {
        const auto it = winning_regions_.find(vertex);
        return it != winning_regions_.end() ? it->second : -1;
    }
    void set_winning_player(Vertex vertex, int player) { winning_regions_[vertex] = player; }
    const std::map<Vertex, int> &get_winning_regions() const { return winning_regions_; }

    std::string to_json() const {
        auto member = detail::map_member_json<Vertex>("winning_regions", winning_regions_, [](int player) { return std::to_string(player); });
        return detail::merge_json_members({member});
    }

    friend std::ostream &operator<<(std::ostream &os, const RSolution<GraphType> &sol) {
        return detail::stream_map_label(os, "Winning regions", sol.winning_regions_, [](std::ostream &os, int player) -> std::ostream & { os << player; return os; });
    }
};

} // namespace solutions
} // namespace ggg
