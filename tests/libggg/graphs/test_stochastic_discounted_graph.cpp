#include "libggg/stochastic_discounted/graph.hpp"
#include <string>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(StochasticDiscountedGraphTests)

BOOST_AUTO_TEST_CASE(CreateSimpleStochasticDiscountedGame) {
    using namespace ggg::stochastic_discounted::graph;

    Graph graph;

    // Add vertices: player 0, player 1, and probabilistic (player -1)
    auto v0 = add_vertex(graph, "player0", 0);
    auto v1 = add_vertex(graph, "player1", 1);
    auto v2 = add_vertex(graph, "probabilistic", -1);

    // Add edges: normal edges with discount/weight, probabilistic edges with probability
    auto [edge01, success01] = add_edge(graph, v0, v1, "edge_0_1", 10.0, 0.9, 0.0);
    auto [edge12, success12] = add_edge(graph, v1, v2, "edge_1_2", -3.0, 0.8, 0.0);
    auto [edge20, success20] = add_edge(graph, v2, v0, "edge_2_0", 0.0, 0.0, 0.6);
    auto [edge21, success21] = add_edge(graph, v2, v1, "edge_2_1", 0.0, 0.0, 0.4);

    BOOST_TEST(success01);
    BOOST_TEST(success12);
    BOOST_TEST(success20);
    BOOST_TEST(success21);

    // Test vertex properties
    BOOST_TEST(graph[v0].name == "player0");
    BOOST_TEST(graph[v0].player == 0);
    BOOST_TEST(graph[v1].name == "player1");
    BOOST_TEST(graph[v1].player == 1);
    BOOST_TEST(graph[v2].name == "probabilistic");
    BOOST_TEST(graph[v2].player == -1);

    // Test edge properties
    BOOST_TEST(graph[edge01].weight == 10.0);
    BOOST_TEST(graph[edge01].discount == 0.9);
    BOOST_TEST(graph[edge12].weight == -3.0);
    BOOST_TEST(graph[edge12].discount == 0.8);
    BOOST_TEST(graph[edge20].probability == 0.6);
    BOOST_TEST(graph[edge21].probability == 0.4);
}

BOOST_AUTO_TEST_CASE(ValidGraphTest) {
    using namespace ggg::stochastic_discounted::graph;

    Graph graph;

    // Create a valid graph
    auto v0 = add_vertex(graph, "v0", 0);
    auto v1 = add_vertex(graph, "v1", 1);
    auto v2 = add_vertex(graph, "v2", -1);

    // Add valid edges
    add_edge(graph, v0, v1, "edge_0_1", 5.0, 0.9, 0.0);
    add_edge(graph, v1, v2, "edge_1_2", -2.0, 0.8, 0.0);
    add_edge(graph, v2, v0, "edge_2_0", 0.0, 0.0, 0.7);
    add_edge(graph, v2, v1, "edge_2_1", 0.0, 0.0, 0.3);

    BOOST_TEST(is_valid(graph));
}

BOOST_AUTO_TEST_CASE(InvalidGraphTests) {
    using namespace ggg::stochastic_discounted::graph;

    // Test invalid player
    {
    Graph graph;
        auto v0 = add_vertex(graph, "v0", 2); // Invalid player
        add_edge(graph, v0, v0, "self", 1.0, 0.5, 0.0);
    BOOST_TEST(!is_valid(graph));
    }

    // Test invalid probabilities that don't sum to 1
    {
    Graph graph;
        auto v0 = add_vertex(graph, "v0", 0);
        auto v1 = add_vertex(graph, "v1", 1);
        auto v2 = add_vertex(graph, "v2", -1);

        add_edge(graph, v0, v2, "edge_0_2", 1.0, 0.5, 0.0);
        add_edge(graph, v2, v0, "edge_2_0", 0.0, 0.0, 0.6);
        add_edge(graph, v2, v1, "edge_2_1", 0.0, 0.0, 0.3); // Sum = 0.9, should be 1.0

    BOOST_TEST(!is_valid(graph));
    }
}

BOOST_AUTO_TEST_CASE(NonProbabilisticVerticesTest) {
    using namespace ggg::stochastic_discounted::graph;

    Graph graph;

    auto v0 = add_vertex(graph, "player0", 0);
    auto v1 = add_vertex(graph, "player1", 1);
    auto v2 = add_vertex(graph, "probabilistic1", -1);
    auto v3 = add_vertex(graph, "probabilistic2", -1);

    // Count non-probabilistic vertices
    int count = 0;
    for (const auto &vertex : get_non_probabilistic_vertices(graph)) {
        BOOST_TEST(graph[vertex].player != -1);
        count++;
    }
    BOOST_TEST(count == 2); // Should have exactly 2 non-probabilistic vertices
}

BOOST_AUTO_TEST_CASE(ReachableThroughProbabilisticTest) {
    using namespace ggg::stochastic_discounted::graph;

    Graph graph;

    // Create a graph: v0 -> v1 (prob) -> {v2: 0.7, v3: 0.3}
    auto v0 = add_vertex(graph, "start", 0);
    auto v1 = add_vertex(graph, "prob", -1);
    auto v2 = add_vertex(graph, "end1", 1);
    auto v3 = add_vertex(graph, "end2", 0);

    add_edge(graph, v0, v1, "edge_0_1", 1.0, 0.5, 0.0);
    add_edge(graph, v1, v2, "edge_1_2", 0.0, 0.0, 0.7);
    add_edge(graph, v1, v3, "edge_1_3", 0.0, 0.0, 0.3);

    auto reachable = get_reachable_through_probabilistic(graph, v0, v1);

    // v0 should reach v2 with probability 0.7 and v3 with probability 0.3
    BOOST_TEST(reachable.size() == 2);
    BOOST_TEST(reachable[v2] == 0.7);
    BOOST_TEST(reachable[v3] == 0.3);
}

BOOST_AUTO_TEST_CASE(ReachableThroughProbabilisticComplexTest) {
    using namespace ggg::stochastic_discounted::graph;

    Graph graph;

    // Create a more complex graph: v0 -> v1 (prob) -> {v2 (prob): 0.6, v4: 0.4}
    //                                      v2 (prob) -> {v3: 0.5, v4: 0.5}
    auto v0 = add_vertex(graph, "start", 0);
    auto v1 = add_vertex(graph, "prob1", -1);
    auto v2 = add_vertex(graph, "prob2", -1);
    auto v3 = add_vertex(graph, "end1", 1);
    auto v4 = add_vertex(graph, "end2", 0);

    add_edge(graph, v0, v1, "edge_0_1", 1.0, 0.5, 0.0);
    add_edge(graph, v1, v2, "edge_1_2", 0.0, 0.0, 0.6);
    add_edge(graph, v1, v4, "edge_1_4", 0.0, 0.0, 0.4);
    add_edge(graph, v2, v3, "edge_2_3", 0.0, 0.0, 0.5);
    add_edge(graph, v2, v4, "edge_2_4", 0.0, 0.0, 0.5);

    auto reachable = get_reachable_through_probabilistic(graph, v0, v1);

    // v0 should reach:
    // v3 with probability 0.6 * 0.5 = 0.3
    // v4 with probability 0.4 + 0.6 * 0.5 = 0.7
    BOOST_TEST(reachable.size() == 2);
    BOOST_TEST(reachable[v3] == 0.3);
    BOOST_TEST(reachable[v4] == 0.7);
}

BOOST_AUTO_TEST_SUITE_END()
