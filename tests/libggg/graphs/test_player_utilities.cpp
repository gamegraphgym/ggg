#include "libggg/graphs/graph_concepts.hpp"
#include "libggg/graphs/graph_utilities.hpp"
#include "libggg/mean_payoff/graph.hpp"
#include "libggg/parity/graph.hpp"
#include "libggg/graphs/player_utilities.hpp"
#include <boost/test/unit_test.hpp>

using namespace ggg::graphs::player_utilities;

using ParityGraph = ggg::parity::graph::Graph;
using ParityVertex = ggg::parity::graph::Vertex;
using MeanPayoffGraph = ggg::mean_payoff::graph::Graph;

// Define a test graph type with player properties for testing the generic utilities
#define TEST_VERTEX_FIELDS(X) \
    X(std::string, name)      \
    X(int, player)            \
    X(double, value)

#define TEST_EDGE_FIELDS(X) \
    X(std::string, label)

#define TEST_GRAPH_FIELDS(X) /* none */

namespace TestPlayerNS {
DEFINE_GAME_GRAPH(TEST_VERTEX_FIELDS, TEST_EDGE_FIELDS, TEST_GRAPH_FIELDS)
} // namespace TestPlayerNS
using TestPlayerGraph = TestPlayerNS::Graph;
using TestPlayerVertex = TestPlayerNS::Vertex;
using namespace TestPlayerNS; // bring add_vertex/add_edge into scope

#undef TEST_VERTEX_FIELDS
#undef TEST_EDGE_FIELDS
#undef TEST_GRAPH_FIELDS

BOOST_AUTO_TEST_SUITE(PlayerUtilitiesTests)

BOOST_AUTO_TEST_CASE(TestGetVerticesByPlayer) {
    TestPlayerGraph graph;

    // Add vertices controlled by different players
    TestPlayerVertex v1 = add_vertex(graph, "v1", 0, 1.0);
    TestPlayerVertex v2 = add_vertex(graph, "v2", 1, 2.0);
    TestPlayerVertex v3 = add_vertex(graph, "v3", 0, 3.0); // Same player as v1
    TestPlayerVertex v4 = add_vertex(graph, "v4", 1, 4.0); // Same player as v2
    TestPlayerVertex v5 = add_vertex(graph, "v5", 2, 5.0); // Different player

    // Test finding vertices controlled by player 0
    auto vertices_player_0 = get_vertices_by_player(graph, 0);
    BOOST_CHECK_EQUAL(vertices_player_0.size(), 2);

    // Test finding vertices controlled by player 1
    auto vertices_player_1 = get_vertices_by_player(graph, 1);
    BOOST_CHECK_EQUAL(vertices_player_1.size(), 2);

    // Test finding vertices controlled by player 2
    auto vertices_player_2 = get_vertices_by_player(graph, 2);
    BOOST_CHECK_EQUAL(vertices_player_2.size(), 1);

    // Test finding vertices controlled by non-existent player
    auto vertices_player_99 = get_vertices_by_player(graph, 99);
    BOOST_CHECK_EQUAL(vertices_player_99.size(), 0);
}

BOOST_AUTO_TEST_CASE(TestPlayerDistribution) {
    TestPlayerGraph graph;

    // Test empty graph
    auto empty_distribution = get_player_distribution(graph);
    BOOST_CHECK(empty_distribution.empty());

    // Add vertices controlled by different players
    TestPlayerVertex v1 = add_vertex(graph, "v1", 0, 1.0);
    TestPlayerVertex v2 = add_vertex(graph, "v2", 1, 2.0);
    TestPlayerVertex v3 = add_vertex(graph, "v3", 0, 3.0); // Same player as v1
    TestPlayerVertex v4 = add_vertex(graph, "v4", 1, 4.0); // Same player as v2
    TestPlayerVertex v5 = add_vertex(graph, "v5", 0, 5.0); // Same player as v1

    auto distribution = get_player_distribution(graph);

    // Check distribution
    BOOST_CHECK_EQUAL(distribution.size(), 2);
    BOOST_CHECK_EQUAL(distribution[0], 3); // v1, v3, v5
    BOOST_CHECK_EQUAL(distribution[1], 2); // v2, v4
}

BOOST_AUTO_TEST_CASE(TestGetUniquePlayers) {
    TestPlayerGraph graph;

    // Test empty graph
    auto empty_players = get_unique_players(graph);
    BOOST_CHECK(empty_players.empty());

    // Add vertices controlled by different players (unsorted)
    TestPlayerVertex v1 = add_vertex(graph, "v1", 2, 1.0);
    TestPlayerVertex v2 = add_vertex(graph, "v2", 0, 2.0);
    TestPlayerVertex v3 = add_vertex(graph, "v3", 2, 3.0); // Duplicate player
    TestPlayerVertex v4 = add_vertex(graph, "v4", 1, 4.0);
    TestPlayerVertex v5 = add_vertex(graph, "v5", 0, 5.0); // Duplicate player

    auto unique_players = get_unique_players(graph);

    // Check that we get unique players only, sorted in ascending order
    std::vector<int> expected = {0, 1, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(unique_players.begin(), unique_players.end(),
                                  expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(TestComputeAttractorSimple) {
    // Create a simple graph where player 0 can attract to a target
    TestPlayerGraph graph;

    TestPlayerVertex v1 = add_vertex(graph, "v1", 0, 1.0); // Player 0 vertex
    TestPlayerVertex v2 = add_vertex(graph, "v2", 1, 2.0); // Player 1 vertex
    TestPlayerVertex v3 = add_vertex(graph, "v3", 0, 3.0); // Target vertex (Player 0)

    // v1 -> v3, v2 -> v3
    add_edge(graph, v1, v3, "e1");
    add_edge(graph, v2, v3, "e2");

    std::set<TestPlayerVertex> target = {v3};

    // Player 0 should attract v1 (player 0 can choose to go to target)
    // and v2 (player 1 is forced to go to target since all edges lead there)
    auto [attractor, strategy] = compute_attractor(graph, target, 0);

    BOOST_CHECK_EQUAL(attractor.size(), 3); // v1, v2, and v3
    BOOST_CHECK(attractor.count(v1));
    BOOST_CHECK(attractor.count(v2)); // v2 is attracted because it's forced to go to target
    BOOST_CHECK(attractor.count(v3));

    // Check strategy
    BOOST_CHECK_EQUAL(strategy[v1], v3);
    BOOST_CHECK_EQUAL(strategy[v2], v3); // Player 1 forced to go to v3
}

BOOST_AUTO_TEST_CASE(TestComputeAttractorOpponentForced) {
    // Create a graph where opponent vertex is forced into attractor
    TestPlayerGraph graph;

    TestPlayerVertex v1 = add_vertex(graph, "v1", 1, 1.0); // Player 1 vertex (opponent to player 0)
    TestPlayerVertex v2 = add_vertex(graph, "v2", 0, 2.0); // Target vertex (Player 0)
    TestPlayerVertex v3 = add_vertex(graph, "v3", 0, 3.0); // Another target vertex

    // v1 has edges only to target vertices
    add_edge(graph, v1, v2, "e1");
    add_edge(graph, v1, v3, "e2");

    std::set<TestPlayerVertex> target = {v2, v3};

    // Player 0 should attract v1 since all of v1's edges go to target
    auto [attractor, strategy] = compute_attractor(graph, target, 0);

    BOOST_CHECK_EQUAL(attractor.size(), 3); // v1, v2, v3
    BOOST_CHECK(attractor.count(v1));
    BOOST_CHECK(attractor.count(v2));
    BOOST_CHECK(attractor.count(v3));

    // Check that strategy exists for v1 (opponent vertex)
    BOOST_CHECK(strategy.count(v1));
}

BOOST_AUTO_TEST_CASE(TestComputeAttractorOpponentNotForced) {
    // Create a graph where opponent vertex has escape option
    TestPlayerGraph graph;

    TestPlayerVertex v1 = add_vertex(graph, "v1", 1, 1.0); // Player 1 vertex (opponent to player 0)
    TestPlayerVertex v2 = add_vertex(graph, "v2", 0, 2.0); // Target vertex
    TestPlayerVertex v3 = add_vertex(graph, "v3", 0, 3.0); // Non-target vertex

    // v1 has edge to target and to non-target
    add_edge(graph, v1, v2, "e1");
    add_edge(graph, v1, v3, "e2");

    std::set<TestPlayerVertex> target = {v2};

    // Player 0 should NOT attract v1 since v1 has escape to v3
    auto [attractor, strategy] = compute_attractor(graph, target, 0);

    BOOST_CHECK_EQUAL(attractor.size(), 1); // Only v2
    BOOST_CHECK(attractor.count(v2));
    BOOST_CHECK_EQUAL(attractor.count(v1), 0); // v1 not attracted
    BOOST_CHECK_EQUAL(attractor.count(v3), 0); // v3 not attracted
}

BOOST_AUTO_TEST_CASE(TestComputeAttractorChain) {
    // Create a chain where attractor propagates
    TestPlayerGraph graph;

    TestPlayerVertex v1 = add_vertex(graph, "v1", 0, 1.0); // Player 0
    TestPlayerVertex v2 = add_vertex(graph, "v2", 0, 2.0); // Player 0
    TestPlayerVertex v3 = add_vertex(graph, "v3", 0, 3.0); // Target

    // Chain: v1 -> v2 -> v3
    add_edge(graph, v1, v2, "e1");
    add_edge(graph, v2, v3, "e2");

    std::set<TestPlayerVertex> target = {v3};

    // Player 0 should attract entire chain
    auto [attractor, strategy] = compute_attractor(graph, target, 0);

    BOOST_CHECK_EQUAL(attractor.size(), 3); // v1, v2, v3
    BOOST_CHECK(attractor.count(v1));
    BOOST_CHECK(attractor.count(v2));
    BOOST_CHECK(attractor.count(v3));

    // Check strategy forms proper chain
    BOOST_CHECK_EQUAL(strategy[v1], v2);
    BOOST_CHECK_EQUAL(strategy[v2], v3);
}

BOOST_AUTO_TEST_CASE(TestComputeAttractorWithParityGraph) {
    // Test that attractor works with ParityGraph (which has player property)
    ParityGraph graph;

    ParityVertex v1 = ggg::parity::graph::add_vertex(graph, "v1", 0, 2); // Player 0, priority 2
    ParityVertex v2 = ggg::parity::graph::add_vertex(graph, "v2", 1, 3); // Player 1, priority 3
    ParityVertex v3 = ggg::parity::graph::add_vertex(graph, "v3", 0, 1); // Target vertex

    ggg::parity::graph::add_edge(graph, v1, v3, "e1");
    ggg::parity::graph::add_edge(graph, v2, v3, "e2");

    std::set<ParityVertex> target = {v3};

    // Player 0 should attract v1 and v2 (v2 is forced since all edges go to target)
    auto [attractor, strategy] = compute_attractor(graph, target, 0);

    BOOST_CHECK_EQUAL(attractor.size(), 3); // v1, v2, and v3
    BOOST_CHECK(attractor.count(v1));
    BOOST_CHECK(attractor.count(v2)); // v2 is attracted because forced
    BOOST_CHECK(attractor.count(v3));
    BOOST_CHECK_EQUAL(strategy[v1], v3);
    BOOST_CHECK_EQUAL(strategy[v2], v3);
}

BOOST_AUTO_TEST_CASE(TestConceptCompatibility) {
    // Test that our concept works correctly
    static_assert(ggg::graphs::HasPlayerOnVertices<TestPlayerGraph>, "TestPlayerGraph should satisfy HasPlayerOnVertices");
    static_assert(ggg::graphs::HasPlayerOnVertices<ParityGraph>, "ParityGraph should satisfy HasPlayerOnVertices");
    static_assert(ggg::graphs::HasPlayerOnVertices<MeanPayoffGraph>, "MeanPayoffGraph should satisfy HasPlayerOnVertices");
}

BOOST_AUTO_TEST_SUITE_END()
