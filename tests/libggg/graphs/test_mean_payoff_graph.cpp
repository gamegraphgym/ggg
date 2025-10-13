#include "libggg/mean_payoff/graph.hpp"
#include <boost/test/unit_test.hpp>
#include <sstream>

using namespace ggg::mean_payoff::graph;

BOOST_AUTO_TEST_SUITE(MeanPayoffGameTests)

BOOST_AUTO_TEST_CASE(TestBasicMeanPayoffOperations) {
    Graph graph;

    // Add vertices with weights
    auto v1 = add_vertex(graph, "v1", 0, 2);
    auto v2 = add_vertex(graph, "v2", 1, 3);
    auto v3 = add_vertex(graph, "v3", 0, 0);

    BOOST_CHECK_EQUAL(boost::num_vertices(graph), 3);
    // Use direct bundled property access
    BOOST_CHECK_EQUAL(graph[v1].weight, 2);
    BOOST_CHECK_EQUAL(graph[v2].weight, 3);
    BOOST_CHECK_EQUAL(graph[v3].weight, 0);
}

BOOST_AUTO_TEST_CASE(TestWeightManipulation) {
    Graph graph;

    auto v1 = add_vertex(graph, "v1", 0, 5);
    BOOST_CHECK_EQUAL(graph[v1].weight, 5);

    // Change priority using direct access
    graph[v1].weight = 8;
    BOOST_CHECK_EQUAL(graph[v1].weight, 8);
}

BOOST_AUTO_TEST_CASE(TestMeanPayoffGameValidation) {
    Graph graph;

    // Valid mean payoff game
    auto v1 = add_vertex(graph, "v1", 0, 2);
    auto v2 = add_vertex(graph, "v2", 1, 3);
    add_edge(graph, v1, v2, ""); // Add edge so vertices have outgoing edges
    add_edge(graph, v2, v1, ""); // Add edge so vertices have outgoing edges
    BOOST_CHECK(is_valid(graph));

    // Add vertex without outgoing edges, check validity
    auto v3 = add_vertex(graph, "v3", 0, 0);
    BOOST_CHECK(!is_valid(graph)); // should return false

    // Fix the deadlock
    add_edge(graph, v3, v1, "");
    BOOST_CHECK(is_valid(graph));
}

BOOST_AUTO_TEST_CASE(TestBoostMeanPayoffParsing) {
    std::string dot_content = R"(
digraph MeanPayoffGame {
    v1 [name="vertex1", player=0, weight=2];
    v2 [name="vertex2", player=1, weight=3];
    v3 [name="vertex3", player=0, weight=0];
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
    const auto [vertices_begin, vertices_end] = boost::vertices(*graph);
    bool found_v1 = false, found_v2 = false, found_v3 = false;
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if ((*graph)[vertex].name == "vertex1") {
            BOOST_CHECK_EQUAL((*graph)[vertex].player, 0);
            BOOST_CHECK_EQUAL((*graph)[vertex].weight, 2);
            found_v1 = true;
        } else if ((*graph)[vertex].name == "vertex2") {
            BOOST_CHECK_EQUAL((*graph)[vertex].player, 1);
            BOOST_CHECK_EQUAL((*graph)[vertex].weight, 3);
            found_v2 = true;
        } else if ((*graph)[vertex].name == "vertex3") {
            BOOST_CHECK_EQUAL((*graph)[vertex].player, 0);
            BOOST_CHECK_EQUAL((*graph)[vertex].weight, 0);
            found_v3 = true;
        }
    }

    BOOST_CHECK(found_v1);
    BOOST_CHECK(found_v2);
    BOOST_CHECK(found_v3);
}

BOOST_AUTO_TEST_SUITE_END()
