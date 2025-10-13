#pragma once

#include <boost/graph/graph_traits.hpp>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ggg {
namespace strategy {

template <typename GraphType>
using MixingStrategy = std::vector<std::pair<typename boost::graph_traits<GraphType>::vertex_descriptor, double>>;

template <typename GraphType>
inline std::string to_json(const MixingStrategy<GraphType> &m) {
    std::ostringstream oss;
    oss << '[';
    bool first = true;
    for (const auto &pr : m) {
        if (!first)
            oss << ',';
        first = false;
        oss << "{" << "\"succ\":" << static_cast<std::size_t>(pr.first) << ",\"prob\":" << pr.second << '}';
    }
    oss << ']';
    return oss.str();
}

template <typename GraphType>
inline std::ostream &to_stream(std::ostream &os, const MixingStrategy<GraphType> &m) {
    os << '[';
    bool first = true;
    for (const auto &pr : m) {
        if (!first)
            os << ',';
        first = false;
        os << '(' << static_cast<std::size_t>(pr.first) << '@' << pr.second << ')';
    }
    os << ']';
    return os;
}

} // namespace strategy
} // namespace ggg
