#include "libggg/graphs/graph_utilities.hpp"
#include <boost/test/unit_test.hpp>
#include <sstream>

// Define a test graph type with player and priority attributes
namespace test_graph {

#define TEST_VERTEX_FIELDS(X) \
    X(std::string, name, "")  \
    X(int, player, -1)        \
    X(int, priority, -1)

#define TEST_EDGE_FIELDS(X) \
    X(std::string, label, "")

#define TEST_GRAPH_FIELDS(X) /* none */

DEFINE_GAME_GRAPH(TEST_VERTEX_FIELDS, TEST_EDGE_FIELDS, TEST_GRAPH_FIELDS)

#undef TEST_VERTEX_FIELDS
#undef TEST_EDGE_FIELDS
#undef TEST_GRAPH_FIELDS

} // namespace test_graph

BOOST_AUTO_TEST_SUITE(ParserTests)

BOOST_AUTO_TEST_CASE(TestParseValidGraph) {
    std::string dot_content = R"(
digraph TestGraph {
    v1 [name="vertex1", player=0, priority=2];
    v2 [name="vertex2", player=1, priority=3];
    v1 -> v2 [label="edge1"];
}
)";

    std::istringstream input(dot_content);
    auto graph = test_graph::parse(input);

    BOOST_REQUIRE(graph != nullptr);
    BOOST_CHECK_EQUAL(boost::num_vertices(*graph), 2);
    BOOST_CHECK_EQUAL(boost::num_edges(*graph), 1);
}

BOOST_AUTO_TEST_CASE(TestParseUndefinedVertexInEdge) {
    // Edge references vertex v3 which is not defined
    // Boost GraphViz parser will auto-create v3 with default property values
    std::string dot_content = R"(
digraph TestGraph {
    v1 [name="vertex1", player=0, priority=2];
    v2 [name="vertex2", player=1, priority=3];
    v1 -> v3 [label="edge_to_undefined"];
}
)";

    std::istringstream input(dot_content);

    // Parser allows this - v3 will be auto-created with default (sentinel) values
    auto graph = test_graph::parse(input);
    BOOST_REQUIRE(graph != nullptr);
    BOOST_CHECK_EQUAL(boost::num_vertices(*graph), 3);
    BOOST_CHECK_EQUAL(boost::num_edges(*graph), 1);

    // Find v3 and verify it has default/sentinel values
    bool found_v3 = false;
    const auto [vertices_begin, vertices_end] = boost::vertices(*graph);
    for (const auto &v : boost::make_iterator_range(vertices_begin, vertices_end)) {
        // Skip v1 and v2 which have explicit properties
        if ((*graph)[v].name == "vertex1" || (*graph)[v].name == "vertex2") {
            continue;
        }

        // This should be v3 with default values
        found_v3 = true;
        BOOST_CHECK_EQUAL((*graph)[v].name, "");
        BOOST_CHECK_EQUAL((*graph)[v].player, -1);
        BOOST_CHECK_EQUAL((*graph)[v].priority, -1);
        BOOST_TEST_MESSAGE("Found auto-created v3 with default values");
        break;
    }
    BOOST_CHECK(found_v3);
}

BOOST_AUTO_TEST_CASE(TestParseMissingPlayerAttribute) {
    // Vertex v2 is missing the required "player" attribute
    std::string dot_content = R"(
digraph TestGraph {
    v1 [name="vertex1", player=0, priority=2];
    v2 [name="vertex2", priority=3];
    v1 -> v2 [label="edge1"];
}
)";

    std::istringstream input(dot_content);

    // Parse the graph (will succeed with sentinel value for missing player)
    auto graph = test_graph::parse(input);
    BOOST_REQUIRE(graph != nullptr);

    // Find v2 and check if player has sentinel value -1 (indicating it wasn't set)
    bool found_v2 = false;
    const auto [vertices_begin, vertices_end] = boost::vertices(*graph);
    for (const auto &v : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if ((*graph)[v].name == "vertex2") {
            found_v2 = true;
            // Player should be -1 (sentinel) since it wasn't set in DOT
            BOOST_CHECK_EQUAL((*graph)[v].player, -1);
            BOOST_TEST_MESSAGE("v2 player value (sentinel for missing): " + std::to_string((*graph)[v].player));
        }
    }
    BOOST_CHECK(found_v2);
}

BOOST_AUTO_TEST_CASE(TestParseInvalidDOTSyntax) {
    // Completely malformed DOT content - not even close to valid
    std::string dot_content = "This is not DOT format at all!";

    std::istringstream input(dot_content);

    // Should throw ParseError due to completely invalid format
    BOOST_CHECK_THROW(test_graph::parse(input), ggg::graphs::ParseError);
}

BOOST_AUTO_TEST_CASE(TestParseEmptyGraph) {
    std::string dot_content = R"(
digraph TestGraph {
}
)";

    std::istringstream input(dot_content);
    auto graph = test_graph::parse(input);

    BOOST_REQUIRE(graph != nullptr);
    BOOST_CHECK_EQUAL(boost::num_vertices(*graph), 0);
    BOOST_CHECK_EQUAL(boost::num_edges(*graph), 0);
}

BOOST_AUTO_TEST_CASE(TestParseGraphWithSelfLoop) {
    std::string dot_content = R"(
digraph TestGraph {
    v1 [name="vertex1", player=0, priority=2];
    v1 -> v1 [label="self_loop"];
}
)";

    std::istringstream input(dot_content);
    auto graph = test_graph::parse(input);

    BOOST_REQUIRE(graph != nullptr);
    BOOST_CHECK_EQUAL(boost::num_vertices(*graph), 1);
    BOOST_CHECK_EQUAL(boost::num_edges(*graph), 1);
}

BOOST_AUTO_TEST_CASE(TestParseMalformedLineWithEdgeReference) {
    // This tests Boost GraphViz parser behavior with malformed syntax
    // The line "asdfasdasdasd //v4 [player=0, priority=0];" is invalid
    // Boost's parser is lenient and does NOT throw exceptions for syntax errors
    // It ignores malformed lines and may auto-create vertices with default values for attributes
    //
    // IMPORTANT: This is a known limitation of Boost's GraphViz parser.
    // To catch malformed DOT files, use validators that check for:
    // - Empty vertex names
    // - Sentinel values (-1) in required fields  
    // - Out-degree constraints
    std::string dot_content = R"(
digraph TestGraph {
    v0 [name="v0", player=1, priority=0];
    asdfasdasdasd //v4 [player=0, priority=0];
    v0->v4  [label="edge_to_undefined"];
}
)";

    std::istringstream input(dot_content);
    
    // Boost's parser won't throw - it creates vertices with sentinel defaults
    auto graph = test_graph::parse(input);
    BOOST_REQUIRE(graph != nullptr);
    
    // Boost creates: 
    // - An unnamed vertex (from malformed line)
    // - v0 (properly defined)
    // - Another unnamed vertex (auto-created v4 referenced by edge)
    BOOST_TEST_MESSAGE("Number of vertices: " + std::to_string(boost::num_vertices(*graph)));
    {
        const auto [v_begin, v_end] = boost::vertices(*graph);
        for (auto v : boost::make_iterator_range(v_begin, v_end)) {
            BOOST_TEST_MESSAGE("Vertex: name='" + (*graph)[v].name + 
                              "', player=" + std::to_string((*graph)[v].player) +
                              ", priority=" + std::to_string((*graph)[v].priority));
        }
    }
    
    BOOST_CHECK_EQUAL(boost::num_edges(*graph), 1);
    
    // Multiple vertices will have sentinel values due to malformed input
    int vertices_with_sentinels = 0;
    {
        const auto [vb, ve] = boost::vertices(*graph);
        for (auto v : boost::make_iterator_range(vb, ve)) {
            if ((*graph)[v].player == -1 && (*graph)[v].priority == -1) {
                vertices_with_sentinels++;
            }
        }
    }
    // Expect 2 vertices with sentinel values (from malformed line and auto-created v4)
    BOOST_CHECK_EQUAL(vertices_with_sentinels, 2);
}

BOOST_AUTO_TEST_SUITE_END()
