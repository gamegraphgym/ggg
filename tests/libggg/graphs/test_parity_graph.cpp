#include "libggg/parity/graph.hpp"
#include <boost/test/unit_test.hpp>
#include <sstream>

using namespace ggg::parity::graph;

BOOST_AUTO_TEST_SUITE(ParityGraphTests)

BOOST_AUTO_TEST_CASE(TestBasicParityOperations) {
    Graph graph;

    // Add vertices with priorities
    auto v1 = add_vertex(graph, "v1", 0, 2); // Even priority
    auto v2 = add_vertex(graph, "v2", 1, 3); // Odd priority
    auto v3 = add_vertex(graph, "v3", 0, 0); // Even priority

    BOOST_CHECK_EQUAL(boost::num_vertices(graph), 3);
    // Use direct bundled property access
    BOOST_CHECK_EQUAL(graph[v1].priority, 2);
    BOOST_CHECK_EQUAL(graph[v2].priority, 3);
    BOOST_CHECK_EQUAL(graph[v3].priority, 0);
}

BOOST_AUTO_TEST_CASE(TestPriorityManipulation) {
    Graph graph;

    auto v1 = add_vertex(graph, "v1", 0, 5);
    BOOST_CHECK_EQUAL(graph[v1].priority, 5);

    // Change priority using direct access
    graph[v1].priority = 8;
    BOOST_CHECK_EQUAL(graph[v1].priority, 8);
}

BOOST_AUTO_TEST_CASE(TestParityGameValidation) {
    Graph graph;

    // Valid parity game
    auto v1 = add_vertex(graph, "v1", 0, 2);
    auto v2 = add_vertex(graph, "v2", 1, 3);
    add_edge(graph, v1, v2, ""); // Add edge so vertices have outgoing edges
    add_edge(graph, v2, v1, ""); // Add edge so vertices have outgoing edges
    BOOST_CHECK(is_valid(graph));

    // Test with negative priority (should be invalid)
    auto v3 = add_vertex(graph, "v3", 0, 0);
    add_edge(graph, v3, v1, ""); // Add edge so vertex has outgoing edge
    graph[v3].priority = -1;     // Direct access to set invalid priority
    BOOST_CHECK(!is_valid(graph));

    // Fix the priority
    graph[v3].priority = 0;
    BOOST_CHECK(is_valid(graph));
}

BOOST_AUTO_TEST_CASE(TestBoostParityParsing) {
    std::string dot_content = R"(
digraph ParityGame {
    v1 [name="vertex1", player=0, priority=2];
    v2 [name="vertex2", player=1, priority=3];
    v3 [name="vertex3", player=0, priority=0];
    v1 -> v2 [label="edge1"];
    v2 -> v3 [label="edge2"];
    v3 -> v1 [label="edge3"];
}
)";

    std::istringstream input(dot_content);
    auto graph = parse(input);

    BOOST_REQUIRE(graph != nullptr);
    BOOST_CHECK_EQUAL(boost::num_vertices(*graph), 3);
    BOOST_CHECK_EQUAL(boost::num_edges(*graph), 3);

    // Check vertices exist and have correct properties
    auto vertices = boost::vertices(*graph);
    bool found_v1 = false, found_v2 = false, found_v3 = false;

    for (auto it = vertices.first; it != vertices.second; ++it) {
        if ((*graph)[*it].name == "vertex1") {
            BOOST_CHECK_EQUAL((*graph)[*it].player, 0);
            BOOST_CHECK_EQUAL((*graph)[*it].priority, 2);
            found_v1 = true;
        } else if ((*graph)[*it].name == "vertex2") {
            BOOST_CHECK_EQUAL((*graph)[*it].player, 1);
            BOOST_CHECK_EQUAL((*graph)[*it].priority, 3);
            found_v2 = true;
        } else if ((*graph)[*it].name == "vertex3") {
            BOOST_CHECK_EQUAL((*graph)[*it].player, 0);
            BOOST_CHECK_EQUAL((*graph)[*it].priority, 0);
            found_v3 = true;
        }
    }

    BOOST_CHECK(found_v1);
    BOOST_CHECK(found_v2);
    BOOST_CHECK(found_v3);
}

BOOST_AUTO_TEST_SUITE_END()
