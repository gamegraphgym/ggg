#pragma once

#include "libggg/utils/logging.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/dynamic_property_map.hpp>

namespace ggg {
/**
 * @file graph_utilities.hpp
 * @brief Graph utilities for creating type-safe Boost.Graph wrappers with DOT format I/O support
 *
 * This header provides a macro-based system for generating complete graph classes with
 * custom vertex, edge, and graph properties, along with parsing and serialization capabilities.
 */
/**
 * @defgroup graphs Graph utilities
 * @brief Utilities and macros for defining game graphs (vertex/edge/graph properties and DOT I/O)
 *
 * This module exposes the macro `DEFINE_GAME_GRAPH` which generates a strongly-typed
 * Boost.Graph-based graph type in the call-site namespace along with parser/writer helpers.
 *
 * Note: many small helper-macro implementation details are internal and therefore are not
 * documented individually here; document the generated API surface via the `DEFINE_GAME_GRAPH`
 * macro documentation below.
 * @{
 */
namespace graphs {

// --- Helper macros for property struct field generation ---
#define PROPERTY_STRUCT_FIELD(type, name) type name;

// These macros work within the context of register_*_dynamic_properties functions
// where dp, g, and Props are in scope
#define REGISTER_VERTEX_FIELD_IMPL(type, name) dp.property(#name, boost::get(&Props::name, g));
#define REGISTER_EDGE_FIELD_IMPL(type, name) dp.property(#name, boost::get(&Props::name, g));
#define REGISTER_GRAPH_FIELD_IMPL(type, name) dp.property(#name, boost::get(&Props::name, g));

// Helper macros for generating add_vertex and add_edge parameters
#define ADD_VERTEX_PARAM(type, name) , const type &name
#define ADD_VERTEX_ASSIGN(type, name) v.name = name;
#define ADD_EDGE_PARAM(type, name) , const type &name
#define ADD_EDGE_ASSIGN(type, name) e.name = name;

/**
 * @def DEFINE_GAME_GRAPH(VERTEX_FIELDS, EDGE_FIELDS, GRAPH_FIELDS)
 * @brief Generates a complete Boost.Graph-based game graph type and helper functions.
 *
 * This macro expands in the call-site namespace and creates:
 *  - internal structs: detail_graphxx::VertexProps, EdgeProps, GraphProps
 *  - using Graph = boost::adjacency_list<...>
 *  - using Vertex, Edge, VertexIterator, EdgeIterator, OutEdgeIterator
 *  - register_dynamic_properties(boost::dynamic_properties&, Graph&)
 *  - Vertex add_vertex(Graph&, (params from VERTEX_FIELDS))
 *  - std::pair<Edge,bool> add_edge(Graph&, Vertex, Vertex, (params from EDGE_FIELDS))
 *  - std::shared_ptr<Graph> parse(std::istream&), parse(const std::string&)
 *  - bool write(const Graph&, std::ostream&), write(const Graph&, const std::string&)
 *
 * @param VERTEX_FIELDS Macro that expands a field macro to declare vertex fields (F(type,name) ...)
 * @param EDGE_FIELDS   Macro that expands a field macro to declare edge fields
 * @param GRAPH_FIELDS  Macro that expands a field macro to declare graph-level fields
 *
 * @note Doxygen cannot see the expanded declarations produced by this macro. This macro-level
 *       documentation describes the API surface the macro generates in the call-site namespace.
 *
 * @example
 * #define VERTEX_FIELDS(F) \
 *     F(std::string, name) \
 *     F(int, value)
 * #define EDGE_FIELDS(F)   F(double, weight)
 * #define GRAPH_FIELDS(F)  F(std::string, title)
 * DEFINE_GAME_GRAPH(VERTEX_FIELDS, EDGE_FIELDS, GRAPH_FIELDS)
 */
#define DEFINE_GAME_GRAPH(VERTEX_FIELDS, EDGE_FIELDS, GRAPH_FIELDS)                                   \
    /* Property structs (internal linkage in call-site namespace) */                                  \
    namespace detail_graphxx {                                                                        \
    struct VertexProps {                                                                              \
        VERTEX_FIELDS(PROPERTY_STRUCT_FIELD)                                                          \
    };                                                                                                \
    struct EdgeProps {                                                                                \
        EDGE_FIELDS(PROPERTY_STRUCT_FIELD)                                                            \
    };                                                                                                \
    struct GraphProps {                                                                               \
        GRAPH_FIELDS(PROPERTY_STRUCT_FIELD)                                                           \
    };                                                                                                \
    }                                                                                                 \
    /* Graph type alias (public in call-site namespace) */                                            \
    using Graph = boost::adjacency_list<                                                              \
        boost::setS, boost::vecS, boost::directedS,                                                   \
        detail_graphxx::VertexProps,                                                                  \
        detail_graphxx::EdgeProps,                                                                    \
        detail_graphxx::GraphProps>;                                                                  \
    /* Type aliases for convenience */                                                                \
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;                                     \
    using Edge = boost::graph_traits<Graph>::edge_descriptor;                                         \
    using VertexIterator = boost::graph_traits<Graph>::vertex_iterator;                               \
    using EdgeIterator = boost::graph_traits<Graph>::edge_iterator;                                   \
    using OutEdgeIterator = boost::graph_traits<Graph>::out_edge_iterator;                            \
    /**                                                                                               \
     * @brief Register dynamic property mappings used by Boost.Graph DOT I/O                          \
     *                                                                                                \
     * This function maps bundled vertex, edge and graph properties to names used                     \
     * by Graphviz/DOT via a boost::dynamic_properties instance. It is intended                       \
     * for internal use by the parse/write helpers generated by this macro. */                        \
    inline void register_dynamic_properties(boost::dynamic_properties &dp, Graph &g) {                \
        /* Register vertex properties */                                                              \
        {                                                                                             \
            using Props [[maybe_unused]] = detail_graphxx::VertexProps;                               \
            VERTEX_FIELDS(REGISTER_VERTEX_FIELD_IMPL)                                                 \
        }                                                                                             \
        /* Register edge properties */                                                                \
        {                                                                                             \
            using Props [[maybe_unused]] = detail_graphxx::EdgeProps;                                 \
            EDGE_FIELDS(REGISTER_EDGE_FIELD_IMPL)                                                     \
        }                                                                                             \
        /* Register graph properties */                                                               \
        {                                                                                             \
            using Props [[maybe_unused]] = detail_graphxx::GraphProps;                                \
            GRAPH_FIELDS(REGISTER_GRAPH_FIELD_IMPL)                                                   \
        }                                                                                             \
    }                                                                                                 \
    /**                                                                                               \
     * @brief Add a vertex with the provided property values                                          \
     * @param graph The graph to which the vertex is added                                            \
     * @param ...   Values for each vertex property as declared by VERTEX_FIELDS                      \
     * @returns The descriptor of the newly added vertex                                              \
     *                                                                                                \
     * The parameter list is generated from the VERTEX_FIELDS macro; see the                          \
     * macro-level documentation for details. */                                                      \
    inline Vertex add_vertex(Graph &graph VERTEX_FIELDS(ADD_VERTEX_PARAM)) {                          \
        Vertex vertex = boost::add_vertex(graph);                                                     \
        auto &v = graph[vertex];                                                                      \
        VERTEX_FIELDS(ADD_VERTEX_ASSIGN)                                                              \
        return vertex;                                                                                \
    }                                                                                                 \
    /**                                                                                               \
     * @brief Add an edge between two vertices with the provided property values                      \
     * @param graph  The graph to which the edge is added                                             \
     * @param source Source vertex descriptor                                                         \
     * @param target Target vertex descriptor                                                         \
     * @param ...    Values for each edge property as declared by EDGE_FIELDS                         \
     * @returns A pair of (edge_descriptor, success_flag)                                             \
     *                                                                                                \
     * The parameter list after `target` is generated from the EDGE_FIELDS macro. */                  \
    inline std::pair<Edge, bool> add_edge(Graph &graph,                                               \
                                          Vertex source, Vertex target EDGE_FIELDS(ADD_EDGE_PARAM)) { \
        auto result = boost::add_edge(source, target, graph);                                         \
        if (result.second) {                                                                          \
            auto &e = graph[result.first];                                                            \
            EDGE_FIELDS(ADD_EDGE_ASSIGN)                                                              \
        }                                                                                             \
        return result;                                                                                \
    }                                                                                                 \
    /** @name Parser utilities                                                                        \
     *  Functions to parse graphs from DOT format.                                                    \
     *  @{ */                                                                                         \
    /**                                                                                               \
     * @brief Parse a graph from an input stream in DOT format                                        \
     * @param in Input stream containing DOT-formatted graph data                                     \
     * @returns Shared pointer to the parsed Graph, or nullptr on failure                             \
     *                                                                                                \
     * This function populates bundled vertex/edge/graph properties from                              \
     * attributes present in the DOT representation using dynamic properties. */                      \
    inline std::shared_ptr<Graph> parse(std::istream &in) {                                           \
        LGG_DEBUG("Starting DOT graph parsing from stream");                                          \
        auto g = std::make_shared<Graph>();                                                           \
        boost::dynamic_properties dp(boost::ignore_other_properties);                                 \
        register_dynamic_properties(dp, *g);                                                          \
        if (!boost::read_graphviz(in, *g, dp, "name")) {                                              \
            LGG_ERROR("Failed to parse DOT format");                                                  \
            return nullptr;                                                                           \
        }                                                                                             \
        return g;                                                                                     \
    }                                                                                                 \
    /**                                                                                               \
     * @brief Parse a graph from a DOT file                                                           \
     * @param fn Path to the DOT file to read                                                         \
     * @returns Shared pointer to the parsed Graph, or nullptr on failure                             \
     *                                                                                                \
     * Convenience overload that opens the file and forwards to parse(std::istream&). */              \
    inline std::shared_ptr<Graph> parse(const std::string &fn) {                                      \
        LGG_DEBUG("Parsing DOT file: ", fn);                                                          \
        std::ifstream file(fn);                                                                       \
        if (!file.is_open()) {                                                                        \
            LGG_ERROR("Failed to open file: ", fn);                                                   \
            return nullptr;                                                                           \
        }                                                                                             \
        return parse(file);                                                                           \
    }                                                                                                 \
    /** @} */                                                                                         \
    /* Writer utilities (public in call-site namespace) */                                            \
    /**                                                                                               \
     * @brief Serialize a graph to an output stream in DOT format                                     \
     * @param g  The graph to serialize                                                               \
     * @param os Output stream to receive DOT-formatted data                                          \
     * @returns true on success, false on failure                                                     \
     *                                                                                                \
     * The function registers the same dynamic properties used for parsing so                         \
     * that bundled properties are emitted as DOT attributes. */                                      \
    inline bool write(const Graph &g, std::ostream &os) {                                             \
        boost::dynamic_properties dp(boost::ignore_other_properties);                                 \
        register_dynamic_properties(dp, const_cast<Graph &>(g));                                      \
        boost::write_graphviz_dp(os, g, dp, "name");                                                  \
        return true;                                                                                  \
    }                                                                                                 \
    /**                                                                                               \
     * @brief Serialize a graph to a DOT file                                                         \
     * @param g  The graph to serialize                                                               \
     * @param fn Path to the output file                                                              \
     * @returns true on success, false on failure                                                     \
     *                                                                                                \
     * Convenience overload that opens the file and forwards to write(std::ostream&). */              \
    inline bool write(const Graph &g, const std::string &fn) {                                        \
        std::ofstream file(fn);                                                                       \
        if (!file.is_open())                                                                          \
            return false;                                                                             \
        return write(g, file);                                                                        \
    }

} // namespace graphs
/** @} */
} // namespace ggg
