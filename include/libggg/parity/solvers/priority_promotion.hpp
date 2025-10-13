#pragma once

#include "libggg/graphs/priority_utilities.hpp"
#include "libggg/parity/graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <queue>
#include <string>
#include <vector>

namespace ggg {
namespace parity {

/**
 * @brief Priority Promotion (PP) parity game solver
 *
 * Implementation of the Priority Promotion algorithm for solving parity games.
 * Based on the algorithm by Benerecetti, Dell'Erba, and Mogavero, following
 * the implementation approach from Oink solver.
 *
 * The algorithm iterates vertices from highest to lowest priority, maintaining
 * regions of vertices and promoting them to higher priorities when regions become
 * closed. When a region becomes a dominion, all vertices in it are solved.
 *
 * Time complexity: Exponential in worst case, but often efficient in practice.
 * Space complexity: O(n) where n is the number of vertices.
 *
 * References:
 * - Benerecetti et al. (2016): "Solving Parity Games via Priority Promotion"
 * - Oink parity game solver implementation (Tom van Dijk)
 * @complexity Time: O(2^n), Space: O(n)
 */

class PriorityPromotionSolver : public ggg::solvers::Solver<graph::Graph, ggg::solutions::RSSolution<graph::Graph>> {
  public:
    using Vertex = graph::Vertex;
    using Graph = graph::Graph;
    using Solution = ggg::solutions::RSSolution<graph::Graph>;

    /**
     * @brief Solve the parity game using priority promotion algorithm
     * @param graph Parity graph to solve
     * @return Solution with winning regions and strategies
     */
    Solution solve(const Graph &graph) override;

    /**
     * @brief Get solver name
     * @return Solver description
     */
    std::string get_name() const override {
        return "Priority Promotion (PP) Parity Game Solver";
    }

    /**
     * @brief Get number of promotions performed during last solve
     * @return Number of promotions
     */
    int get_promotions() const { return promotions_; }

  private:
    // Algorithm state
    int max_priority_;
    int promotions_;

    // Vertex mappings - using maps for safe vertex ID handling
    std::map<Vertex, int> region_;        // vertex -> current priority/region
    std::map<Vertex, Vertex> strategy_;   // vertex -> strategy choice (target vertex)
    std::map<Vertex, bool> has_strategy_; // vertex -> whether strategy is valid
    std::map<Vertex, bool> disabled_;     // vertex -> is disabled (solved)

    // Priority/region data structures
    std::vector<std::vector<Vertex>> regions_; // priority -> list of vertices in region
    std::vector<int> inverse_;                 // priority -> representative vertex index in sorted_vertices_

    // Utility data structures
    std::queue<Vertex> queue_;

    // Graph information - stored as maps for safe vertex ID handling
    std::map<Vertex, std::vector<Vertex>> predecessors_;
    std::map<Vertex, std::vector<Vertex>> successors_;
    std::map<Vertex, int> vertex_priority_;
    std::map<Vertex, int> vertex_player_;

    // Priority-sorted vertex list (highest to lowest)
    std::vector<Vertex> sorted_vertices_;

    // Vertex to index mapping for array operations
    std::map<Vertex, size_t> vertex_to_index_;

    /**
     * @brief Initialize algorithm state from graph
     * @param graph Input parity graph
     */
    void initialize(const Graph &graph);

    /**
     * @brief Build predecessor and successor lists for efficient access
     * @param graph Input parity graph
     */
    void build_adjacency_cache(const Graph &graph);

    /**
     * @brief Attract vertices to a given priority region
     * @param priority Target priority to attract to
     */
    void attract(int priority);

    /**
     * @brief Promote all vertices from one priority to another
     * @param from_priority Source priority
     * @param to_priority Target priority
     */
    void promote(int from_priority, int to_priority);

    /**
     * @brief Reset a region by moving vertices back to their original priorities
     * @param priority Priority region to reset
     */
    void reset_region(int priority);

    /**
     * @brief Setup a region for processing
     * @param vertex_index Current vertex index in sorted_vertices_
     * @param priority Priority to setup
     * @param must_reset Whether the region must be reset
     * @return True if region is non-empty and ready
     */
    bool setup_region(int vertex_index, int priority, bool must_reset);

    /**
     * @brief Mark a region as a dominion and solve all vertices in it
     * @param priority Priority of the dominion
     * @param solution Solution object to update
     */
    void set_dominion(int priority, Solution &solution);

    /**
     * @brief Check the status of a region (closed, open, or can be promoted)
     * @param vertex_index Current vertex index in sorted_vertices_
     * @param priority Priority to check
     * @return -1 for dominion, -2 for open, or target priority for promotion
     */
    int get_region_status(int vertex_index, int priority);

    /**
     * @brief Get original priority of a vertex
     * @param vertex Vertex to query
     * @return Original priority
     */
    int get_original_priority(Vertex curr_vertex) const {
        return vertex_priority_.at(curr_vertex);
    }

    /**
     * @brief Get player of a vertex
     * @param vertex Vertex to query
     * @return Player (0 or 1)
     */
    int get_player(Vertex curr_vertex) const {
        return vertex_player_.at(curr_vertex);
    }

    /**
     * @brief Check if vertex is disabled (already solved)
     * @param vertex Vertex to check
     * @return True if disabled
     */
    bool is_disabled(Vertex curr_vertex) const {
        auto it = disabled_.find(curr_vertex);
        return it != disabled_.end() && it->second;
    }

    /**
     * @brief Get current region of a vertex
     * @param vertex Vertex to query
     * @return Current region/priority (-2 if disabled)
     */
    int get_region(Vertex curr_vertex) const {
        return region_.at(curr_vertex);
    }

    /**
     * @brief Check if a vertex has a valid strategy
     * @param vertex Vertex to check
     * @return True if vertex has a strategy set
     */
    bool has_strategy(Vertex curr_vertex) const {
        return has_strategy_.count(curr_vertex) && has_strategy_.at(curr_vertex);
    }

    /**
     * @brief Get strategy for a vertex (only call if has_strategy returns true)
     * @param vertex Vertex to query
     * @return Strategy vertex
     */
    Vertex get_strategy(Vertex curr_vertex) const {
        return strategy_.at(curr_vertex);
    }

    // Statistic fields
    uint iterations; // Total number of queries needed for the game solution
    uint totpromos;  // Total number of promotions needed for the game solution
    uint maxqueries; // Maximal number of queries needed for finding a dominion
    uint maxpromos;  // Maximal number of promotions needed for finding a dominion
    uint queries;    // Current number of queries
    uint promos;     // Current number of promotions
    uint doms;       // Number of dominions found
};

} // namespace parity
} // namespace ggg