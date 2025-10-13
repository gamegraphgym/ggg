#/**
# * @file formatting_utils.hpp
# * @brief Small helpers (internal) for creating compact JSON-like strings and streaming maps.
# *
# * These utilities live in `ggg::solutions::detail` and expect callers to provide
# * appropriate value conversion callables. Keys are written as numeric indices
# * via static_cast<std::size_t>. No escaping is performed on produced fragments.
# */
#pragma once

#include <ostream>
#include <sstream>
#include <string>

namespace ggg {
namespace solutions {
namespace detail {

/**
 * @brief Build a JSON object string from a map and return it paired with a field name.
 * @tparam Vertex   Reserved for overload resolution / caller context (unused)
 * @tparam Map      Map-like container type; element type must be pair-like with key convertible to std::size_t
 * @tparam ValToJson Callable with signature std::string(T const&) that produces a JSON fragment for a mapped value
 * @param key       JSON member name to associate with the produced object (returned as first)
 * @param m         The map to serialize; each element is emitted as "<index>": <value-json>
 * @param val_to_json Callable used to convert each mapped value into a JSON fragment string
 * @returns Pair {key, json_object_string} where json_object_string is e.g. {"0": <v0>, "1": <v1>}
 * @note Keys are cast to std::size_t. The produced value strings are inserted verbatim and should be valid JSON.
 */
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

/**
 * @brief Merge preformatted name/json-fragment pairs into a single JSON object string.
 * @param items initializer_list of {name, json_fragment} pairs. `name` will be quoted;
 *              `json_fragment` is inserted verbatim as the member value.
 * @returns A std::string like {"name1":<json1>,"name2":<json2>}
 * @note No escaping or validation is performed on json_fragment content.
 */
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

/**
 * @brief Stream a labeled map to an ostream using a compact "label: {k:v,...}" format.
 * @tparam Map         Map-like container type; element type must be pair-like with key convertible to std::size_t
 * @tparam ValToStream Callable with signature void(std::ostream&, const T&) that writes a representation of a mapped value
 * @param os    Output stream to write into
 * @param label Text label written before the serialized map (as "label: {")
 * @param m     Map to stream; each entry emitted as index:value with commas between entries
 * @param val_to_stream Callable that writes each mapped value representation to os
 * @returns The same std::ostream& passed in (allows chaining)
 * @note Keys are written as numeric indices via static_cast<std::size_t>. No extra quoting/escaping is done here.
 */
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
