#pragma once

#include "libggg/graphs/validator.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <set>
#include <stdexcept>

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
/**
 * @brief Validator for out-degree range of vertices
 *
 * This validator checks that all vertices have an out-degree within a specified
 * range [MinOutDegree, MaxOutDegree]. Useful for ensuring game graphs have valid
 * moves and bounded branching factor.
 *
 * @tparam MinOutDegree Minimum required out-degree (default: 0, meaning no minimum check)
 * @tparam MaxOutDegree Maximum allowed out-degree (default: unlimited, meaning no maximum check)
 *
 * @example
 * // Require at least one outgoing edge per vertex
 * OutDegreeValidator<1>::validate(graph);
 *
 * // Require between 2 and 5 outgoing edges per vertex
 * OutDegreeValidator<2, 5>::validate(graph);
 *
 * // Require exactly 3 outgoing edges per vertex
 * OutDegreeValidator<3, 3>::validate(graph);
 *
 * // Require at most 10 outgoing edges per vertex (no minimum)
 * OutDegreeValidator<0, 10>::validate(graph);
 */
template <std::size_t MinOutDegree = 0, std::size_t MaxOutDegree = std::numeric_limits<std::size_t>::max()>
struct OutDegreeValidator {
    static_assert(MinOutDegree <= MaxOutDegree, "MinOutDegree must be <= MaxOutDegree");

    template <typename GraphType>
    static void validate(const GraphType &graph) {
        const auto [vertices_begin, vertices_end] = boost::vertices(graph);
        for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
            const auto out_deg = boost::out_degree(vertex, graph);
            if constexpr (MinOutDegree > 0) {
                if (out_deg < MinOutDegree) {
                    throw GraphValidationError(
                        "Vertex '" + graph[vertex].name +
                        "' has out-degree " + std::to_string(out_deg) +
                        " (must be >= " + std::to_string(MinOutDegree) + ")");
                }
            }
            if constexpr (MaxOutDegree < std::numeric_limits<std::size_t>::max()) {
                if (out_deg > MaxOutDegree) {
                    throw GraphValidationError(
                        "Vertex '" + graph[vertex].name +
                        "' has out-degree " + std::to_string(out_deg) +
                        " (must be <= " + std::to_string(MaxOutDegree) + ")");
                }
            }
        }
    }
};

/**
 * @brief Validator that checks for duplicate edges
 *
 * This validator ensures that there are no duplicate edges (multiple edges
 * between the same source and target vertices) in the graph.
 */
struct NoDuplicateEdgesValidator {
    template <typename GraphType>
    static void validate(const GraphType &graph) {
        using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;
        std::set<std::pair<Vertex, Vertex>> seen_edges;
        const auto [edges_begin, edges_end] = boost::edges(graph);
        for (const auto &edge : boost::make_iterator_range(edges_begin, edges_end)) {
            auto source = boost::source(edge, graph);
            auto target = boost::target(edge, graph);
            const auto edge_key = std::make_pair(source, target);
            if (!seen_edges.insert(edge_key).second) {
                throw GraphValidationError(
                    "Duplicate edge found between vertices '" +
                    graph[source].name + "' and '" + graph[target].name + "'");
            }
        }
    }
};

/**
 * Exception thrown on graph parsing errors.
 */
struct ParseError : public std::runtime_error {
    explicit ParseError(const std::string &msg) : std::runtime_error(msg) {}
};

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
 *  - void write(const Graph&, std::ostream&), write(const Graph&, const std::string&)
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
            throw ggg::graphs::ParseError("Failed to parse DOT format");                              \
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
            throw std::ios_base::failure(std::string("Failed to open file: ") + fn);                  \
        }                                                                                             \
        return parse(file);                                                                           \
    }                                                                                                 \
    /** @} */                                                                                         \
    /* Writer utilities (public in call-site namespace) */                                            \
    /**                                                                                               \
     * @brief Serialize a graph to an output stream in DOT format                                     \
     * @param g  The graph to serialize                                                               \
     * @param os Output stream to receive DOT-formatted data                                          \
     * @throws std::ios_base::failure if the stream enters a bad state after writing                  \
     *                                                                                                \
     * The function registers the same dynamic properties used for parsing so                         \
     * that bundled properties are emitted as DOT attributes. */                                      \
    inline void write(const Graph &g, std::ostream &os) {                                             \
        boost::dynamic_properties dp(boost::ignore_other_properties);                                 \
        register_dynamic_properties(dp, const_cast<Graph &>(g));                                      \
        boost::write_graphviz_dp(os, g, dp, "name");                                                  \
        if (!os.good()) {                                                                             \
            throw std::ios_base::failure("Failed to write graph to output stream");                   \
        }                                                                                             \
    }                                                                                                 \
    /**                                                                                               \
     * @brief Serialize a graph to a DOT file                                                         \
     * @param g  The graph to serialize                                                               \
     * @param fn Path to the output file                                                              \
     * @throws std::ios_base::failure on failure to open or write the output file                     \
     *                                                                                                \
     * Convenience overload that opens the file and forwards to write(std::ostream&). */              \
    inline void write(const Graph &g, const std::string &fn) {                                        \
        std::ofstream file(fn);                                                                       \
        if (!file.is_open()) {                                                                        \
            throw std::ios_base::failure(std::string("Failed to open file for writing: ") + fn);      \
        }                                                                                             \
        write(g, file);                                                                               \
    }

} // namespace graphs
/** @} */
} // namespace ggg
