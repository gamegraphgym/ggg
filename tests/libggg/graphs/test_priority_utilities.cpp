#include "libggg/graphs/graph_concepts.hpp"
#include "libggg/graphs/graph_utilities.hpp"
#include "libggg/mean_payoff/graph.hpp"
#include "libggg/parity/graph.hpp"
#include "libggg/graphs/priority_utilities.hpp"
#include <boost/test/unit_test.hpp>

using namespace ggg::graphs::priority_utilities;

using ParityGraph = ggg::parity::graph::Graph;
using ParityVertex = ggg::parity::graph::Vertex;
using MeanPayoffGraph = ggg::mean_payoff::graph::Graph;

// Define a test graph type with priority properties for testing the generic utilities
#define TEST_VERTEX_FIELDS(X) \
    X(std::string, name)      \
    X(int, priority)          \
    X(double, value)

#define TEST_EDGE_FIELDS(X) \
    X(std::string, label)

#define TEST_GRAPH_FIELDS(X) /* none */

namespace TestNS {
DEFINE_GAME_GRAPH(TEST_VERTEX_FIELDS, TEST_EDGE_FIELDS, TEST_GRAPH_FIELDS)
} // namespace TestNS
using TestGraph = TestNS::Graph;
using TestVertex = TestNS::Vertex;
using namespace TestNS;

#undef TEST_VERTEX_FIELDS
#undef TEST_EDGE_FIELDS
#undef TEST_GRAPH_FIELDS

BOOST_AUTO_TEST_SUITE(PriorityUtilitiesTests)

BOOST_AUTO_TEST_CASE(TestGetVerticesWithPriority) {
    TestGraph graph;

    // Add vertices with various priorities
    TestVertex v1 = add_vertex(graph, "v1", 5, 1.0);
    TestVertex v2 = add_vertex(graph, "v2", 3, 2.0);
    TestVertex v3 = add_vertex(graph, "v3", 5, 3.0); // Same priority as v1
    TestVertex v4 = add_vertex(graph, "v4", 7, 4.0);

    // Test finding vertices with priority 5
    auto vertices_prio_5 = get_vertices_with_priority(graph, 5);
    BOOST_CHECK_EQUAL(vertices_prio_5.size(), 2);

    // Test finding vertices with priority 3
    auto vertices_prio_3 = get_vertices_with_priority(graph, 3);
    BOOST_CHECK_EQUAL(vertices_prio_3.size(), 1);

    // Test finding vertices with non-existent priority
    auto vertices_prio_10 = get_vertices_with_priority(graph, 10);
    BOOST_CHECK_EQUAL(vertices_prio_10.size(), 0);
}

BOOST_AUTO_TEST_CASE(TestMinMaxPriority) {
    TestGraph graph;

    TestVertex v1 = add_vertex(graph, "v1", 2, 1.0);
    TestVertex v2 = add_vertex(graph, "v2", 7, 2.0);
    TestVertex v3 = add_vertex(graph, "v3", 2, 3.0);
    TestVertex v4 = add_vertex(graph, "v4", 1, 4.0);

    // Test min/max priority with generic utilities
    BOOST_CHECK_EQUAL(get_min_priority(graph), 1);
    BOOST_CHECK_EQUAL(get_max_priority(graph), 7);
}

BOOST_AUTO_TEST_CASE(TestMinMaxPriorityEdgeCases) {
    // Test min/max priority on empty graph
    TestGraph empty_graph;
    BOOST_CHECK_EQUAL(get_min_priority(empty_graph), 0);
    BOOST_CHECK_EQUAL(get_max_priority(empty_graph), 0);

    // Test with single vertex
    TestGraph single_graph;
    TestVertex v1 = add_vertex(single_graph, "v1", 42, 1.0);
    BOOST_CHECK_EQUAL(get_min_priority(single_graph), 42);
    BOOST_CHECK_EQUAL(get_max_priority(single_graph), 42);

    // Test with negative priorities
    TestGraph negative_graph;
    TestVertex v2 = add_vertex(negative_graph, "v2", -5, 2.0);
    TestVertex v3 = add_vertex(negative_graph, "v3", 10, 3.0);
    BOOST_CHECK_EQUAL(get_min_priority(negative_graph), -5);
    BOOST_CHECK_EQUAL(get_max_priority(negative_graph), 10);
}

BOOST_AUTO_TEST_CASE(TestPriorityDistribution) {
    TestGraph graph;

    // Test empty graph
    auto empty_distribution = get_priority_distribution(graph);
    BOOST_CHECK(empty_distribution.empty());

    // Add vertices with various priorities
    TestVertex v1 = add_vertex(graph, "v1", 2, 1.0);
    TestVertex v2 = add_vertex(graph, "v2", 3, 2.0);
    TestVertex v3 = add_vertex(graph, "v3", 2, 3.0); // Same priority as v1
    TestVertex v4 = add_vertex(graph, "v4", 5, 4.0);
    TestVertex v5 = add_vertex(graph, "v5", 3, 5.0); // Same priority as v2

    auto distribution = get_priority_distribution(graph);

    // Check distribution
    BOOST_CHECK_EQUAL(distribution.size(), 3);
    BOOST_CHECK_EQUAL(distribution[2], 2); // v1 and v3
    BOOST_CHECK_EQUAL(distribution[3], 2); // v2 and v5
    BOOST_CHECK_EQUAL(distribution[5], 1); // v4
}

BOOST_AUTO_TEST_CASE(TestGetUniquePriorities) {
    TestGraph graph;

    // Test empty graph
    auto empty_priorities = get_unique_priorities(graph);
    BOOST_CHECK(empty_priorities.empty());

    // Add vertices with various priorities (unsorted)
    TestVertex v1 = add_vertex(graph, "v1", 10, 1.0);
    TestVertex v2 = add_vertex(graph, "v2", 3, 2.0);
    TestVertex v3 = add_vertex(graph, "v3", 10, 3.0); // Duplicate priority
    TestVertex v4 = add_vertex(graph, "v4", 7, 4.0);
    TestVertex v5 = add_vertex(graph, "v5", 3, 5.0); // Duplicate priority

    auto unique_priorities = get_unique_priorities(graph);

    // Check that we get unique priorities only, sorted in ascending order
    std::vector<int> expected = {3, 7, 10};
    BOOST_CHECK_EQUAL_COLLECTIONS(unique_priorities.begin(), unique_priorities.end(),
                                  expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(TestGetUniquePrioritiesSingleVertex) {
    TestGraph graph;

    // Test with single vertex
    TestVertex v1 = add_vertex(graph, "v1", 42, 1.0);
    auto unique_priorities = get_unique_priorities(graph);

    std::vector<int> expected = {42};
    BOOST_CHECK_EQUAL_COLLECTIONS(unique_priorities.begin(), unique_priorities.end(),
                                  expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(TestPriorityCompression) {
    // Test empty graph compression
    TestGraph empty_graph;
    compress_priorities(empty_graph); // Should not crash on empty graph
    BOOST_CHECK_EQUAL(boost::num_vertices(empty_graph), 0);

    // Create a graph with sparse priorities
    TestGraph graph;
    TestVertex v1 = add_vertex(graph, "v1", 10, 1.0); // Even, high priority
    TestVertex v2 = add_vertex(graph, "v2", 15, 2.0); // Odd, high priority
    TestVertex v3 = add_vertex(graph, "v3", 22, 3.0); // Even, very high priority
    TestVertex v4 = add_vertex(graph, "v4", 7, 4.0);  // Odd, medium priority

    // Add edges to make it a valid graph
    add_edge(graph, v1, v2, "e1");
    add_edge(graph, v2, v3, "e2");
    add_edge(graph, v3, v4, "e3");
    add_edge(graph, v4, v1, "e4");

    // Compress priorities using generic utility
    compress_priorities(graph);

    // Check graph structure is preserved
    BOOST_CHECK_EQUAL(boost::num_vertices(graph), 4);
    BOOST_CHECK_EQUAL(boost::num_edges(graph), 4);

    // Check that priorities are compressed correctly
    // Original priorities: {7, 10, 15, 22} (sorted)
    // Expected mapping with parity preservation:
    // 7 (odd, minimum) -> 1 (first odd priority, preserving minimum parity)
    // 10 (even) -> 2 (first even priority, starting from 2 since minimum was odd)
    // 15 (odd) -> 3 (next odd, since even 10 intervened after 7)
    // 22 (even) -> 4 (next even, since odd 15 intervened after 10)

    std::map<std::string, int> expected_compressed_priorities = {
        {"v1", 2}, // original 10 (even) -> 2
        {"v2", 3}, // original 15 (odd) -> 3
        {"v3", 4}, // original 22 (even) -> 4
        {"v4", 1}  // original 7 (odd) -> 1
    };

    // Verify compressed priorities
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    for (auto vertex_it = vertices_begin; vertex_it != vertices_end; ++vertex_it) {
        const auto &vertex = *vertex_it;
        std::string name = graph[vertex].name;
        int compressed_priority = graph[vertex].priority;

        BOOST_CHECK_EQUAL(compressed_priority, expected_compressed_priorities[name]);
    }
}

BOOST_AUTO_TEST_CASE(TestConceptCompatibility) {
    // Test that our concept works correctly
    static_assert(ggg::graphs::HasPriorityOnVertices<TestGraph>, "TestGraph should satisfy HasPriorityOnVertices");
    static_assert(ggg::graphs::HasPriorityOnVertices<ParityGraph>, "ParityGraph should satisfy HasPriorityOnVertices");

    // MeanPayoffGraph doesn't have priority property, so this should fail:
    static_assert(!ggg::graphs::HasPriorityOnVertices<MeanPayoffGraph>, "MeanPayoffGraph should not satisfy HasPriorityOnVertices (has weight instead)");
}

BOOST_AUTO_TEST_SUITE_END()
