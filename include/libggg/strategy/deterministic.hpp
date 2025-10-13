#pragma once

#include <boost/graph/graph_traits.hpp>
#include <ostream>
#include <string>

namespace ggg {
namespace strategy {

template <typename GraphType>
using DeterministicStrategy = typename boost::graph_traits<GraphType>::vertex_descriptor;

template <typename GraphType>
inline std::string to_json(const DeterministicStrategy<GraphType> &s) {
    if (s == boost::graph_traits<GraphType>::null_vertex())
        return std::string("null");
    return std::to_string(static_cast<std::size_t>(s));
}

template <typename GraphType>
inline std::ostream &to_stream(std::ostream &os, const DeterministicStrategy<GraphType> &s) {
    if (s == boost::graph_traits<GraphType>::null_vertex())
        os << "null";
    else
        os << static_cast<std::size_t>(s);
    return os;
}

} // namespace strategy
} // namespace ggg
