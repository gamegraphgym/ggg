#pragma once

#include "libggg/solutions/formatting_utils.hpp"
#include "libggg/solutions/isolution.hpp"
#include <boost/graph/graph_traits.hpp>
#include <map>

namespace ggg {
namespace solutions {

/**
 * @class QSolution
 * @brief Solution that stores per-vertex quantitative values.
 *
 * Capability: Q (Quantitative)
 *
 * QSolution offers accessors to set and query numeric values associated with vertices
 * (for example, values produced by value-iteration or mean-payoff analyses). It
 * also provides JSON/text conversion for the stored values.
 */
template <typename GraphType, typename ValueType = double>
class QSolution : public virtual ISolution {
  public:
    using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;
    using Value = ValueType;

  protected:
    std::map<Vertex, ValueType> values_;

  public:
    QSolution() = default;

    ValueType get_value(Vertex vertex) const {
        const auto it = values_.find(vertex);
        return it != values_.end() ? it->second : ValueType{};
    }
    bool has_value(Vertex vertex) const { return values_.find(vertex) != values_.end(); }
    void set_value(Vertex vertex, const ValueType &value) { values_[vertex] = value; }
    const std::map<Vertex, ValueType> &get_values() const { return values_; }

    std::string to_json() const {
        auto member = detail::map_member_json<Vertex>("values", values_, [](const ValueType &v) { return std::to_string(v); });
        return detail::merge_json_members({member});
    }

    friend std::ostream &operator<<(std::ostream &os, const QSolution<GraphType, ValueType> &sol) {
        return detail::stream_map_label(os, "Values", sol.values_, [](std::ostream &os, const ValueType &v) -> std::ostream & { os << v; return os; });
    }
};

} // namespace solutions
} // namespace ggg
