#pragma once

#include <boost/graph/graph_traits.hpp>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

namespace ggg {
namespace strategy {

template <typename GraphType>
using FiniteMemoryStrategy = std::pair<typename boost::graph_traits<GraphType>::vertex_descriptor, int>;

template <typename GraphType>
inline std::string to_json(const FiniteMemoryStrategy<GraphType> &fm) {
    std::ostringstream oss;
    oss << "{" << "\"move\":" << static_cast<std::size_t>(fm.first) << ",\"memory\":" << fm.second << '}';
    return oss.str();
}

template <typename GraphType>
inline std::ostream &to_stream(std::ostream &os, const FiniteMemoryStrategy<GraphType> &fm) {
    os << '(' << static_cast<std::size_t>(fm.first) << ',' << fm.second << ')';
    return os;
}

} // namespace strategy
} // namespace ggg
