#pragma once

#include <ostream>
#include <sstream>
#include <string>

namespace ggg {
namespace solutions {
namespace detail {

template <typename Vertex, typename Map, typename ValToJson>
inline std::pair<std::string, std::string> map_member_json(const std::string &key, const Map &m, ValToJson val_to_json) {
    std::ostringstream oss;
    oss << '{';
    bool first = true;
    for (const auto &p : m) {
        if (!first)
            oss << ',';
        first = false;
        oss << '"' << static_cast<std::size_t>(p.first) << "\": " << val_to_json(p.second);
    }
    oss << '}';
    return {key, oss.str()};
}

inline std::string merge_json_members(std::initializer_list<std::pair<std::string, std::string>> items) {
    std::ostringstream oss;
    oss << '{';
    bool first = true;
    for (const auto &kv : items) {
        if (!first)
            oss << ',';
        first = false;
        oss << '"' << kv.first << '"' << ':' << kv.second;
    }
    oss << '}';
    return oss.str();
}

template <typename Map, typename ValToStream>
inline std::ostream &stream_map_label(std::ostream &os, const std::string &label, const Map &m, ValToStream val_to_stream) {
    os << label << ": {";
    bool first = true;
    for (const auto &p : m) {
        if (!first)
            os << ',';
        first = false;
        os << static_cast<std::size_t>(p.first) << ':';
        val_to_stream(os, p.second);
    }
    os << '}';
    return os;
}

} // namespace detail
} // namespace solutions
} // namespace ggg
