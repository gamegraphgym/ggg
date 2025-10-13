#include "libggg/parity/solvers/priority_promotion.hpp"
#include "libggg/graphs/graph_utilities.hpp"
#include "libggg/graphs/player_utilities.hpp"
#include <algorithm>
#include <cassert>

namespace ggg {
namespace parity {

PriorityPromotionSolver::Solution PriorityPromotionSolver::solve(const Graph &graph) {
    // Initialize solution
    Solution solution;

    // Check for empty graph
    const auto [VERTICES_BEGIN, VERTICES_END] = boost::vertices(graph);
    if (VERTICES_BEGIN == VERTICES_END) {

        return solution;
    }

    // Initialize algorithm state
    initialize(graph);
    build_adjacency_cache(graph);

    // Get vertices sorted by priority (highest to lowest) using new priority utilities
    sorted_vertices_ = graphs::priority_utilities::get_vertices_by_priority_descending(graph);

    // Create vertex to index mapping for safe array access
    vertex_to_index_.clear();
    for (size_t i = 0; i < sorted_vertices_.size(); i++) {
        vertex_to_index_[sorted_vertices_[i]] = i;
    }

    // Initialize tracking data structures using maps for safety
    region_.clear();
    strategy_.clear();
    disabled_.clear();
    regions_.resize(max_priority_ + 1);
    inverse_.resize(max_priority_ + 1);

    // Initialize vertex states following Oink's approach
    for (Vertex v : sorted_vertices_) {
        region_[v] = disabled_[v] ? -2 : get_original_priority(v);
        has_strategy_[v] = false;
        disabled_[v] = false; // Initially no vertex is disabled
    }

    // Start main algorithm loop from first vertex (highest priority)
    int i = 0;

    /**
     * Two loops: the outer (normal) loop, and the inner (promotion-chain) loop.
     * The outer loop does region setup and attractor on the full region.
     * The inner loop only attracts from the promoted region.
     */

    while (i < static_cast<int>(sorted_vertices_.size())) {
        // Get current priority and skip all disabled/attracted vertices
        int p = get_original_priority(sorted_vertices_[i]);

        // Skip disabled or already attracted vertices with this priority
        while (i < static_cast<int>(sorted_vertices_.size()) &&
               get_original_priority(sorted_vertices_[i]) == p &&
               (is_disabled(sorted_vertices_[i]) || get_region(sorted_vertices_[i]) > p)) {
            i++;
        }
        if (i >= static_cast<int>(sorted_vertices_.size()))
            break;

        // If empty, possibly reset and continue with next
        if (get_original_priority(sorted_vertices_[i]) != p) {
            if (!regions_[p].empty()) {
                reset_region(p);
            }
            continue;
        }

        // Set representative vertex for this priority
        inverse_[p] = i; // Store the index, not the vertex ID
        queries++;

        // PP: always reset region (following Oink's approach)
        if (setup_region(i, p, true)) {
            // Region not empty, maybe promote
            while (true) {
                int status = get_region_status(i, p);
                if (status == -2) {
                    // Not closed, skip to next priority and break inner loop
                    while (i < static_cast<int>(sorted_vertices_.size()) &&
                           get_original_priority(sorted_vertices_[i]) == p) {
                        i++;
                    }
                    break;
                } else if (status == -1) {
                    // Found dominion
                    set_dominion(p, solution);
                    // Restart algorithm and break inner loop
                    iterations += queries;
                    totpromos += promos;
                    if (queries > maxqueries) {
                        maxqueries = queries;
                    }
                    if (promos > maxpromos) {
                        maxpromos = promos;
                    }
                    i = 0;
                    break;
                } else {
                    // Found promotion, promote
                    promote(p, status);
                    // Continue inner loop with the higher priority
                    i = inverse_[status];
                    p = status;
                }
            }
        } else {
            // Skip to next priority
            while (i < static_cast<int>(sorted_vertices_.size()) &&
                   get_original_priority(sorted_vertices_[i]) == p) {
                i++;
            }
        }
    }

    // Transfer strategies from internal representation to solution
    // Only set strategies for vertices owned by the same player who wins them
    const auto [VERTICES_BEGIN2, VERTICES_END2] = boost::vertices(graph);
    for (auto it = VERTICES_BEGIN2; it != VERTICES_END2; ++it) {
        Vertex v = *it;
        // Only set strategy if:
        // 1. Vertex was solved (disabled)
        // 2. Vertex has a strategy from the algorithm
        // 3. Vertex owner matches the winning player
        if (is_disabled(v) && has_strategy(v)) {
            int vertex_owner = get_player(v);
            int winning_player = solution.get_winning_player(v);

            // Only set strategy if the vertex owner is the same as the winning player
            if (vertex_owner == winning_player) {
                solution.set_strategy(v, get_strategy(v));
            }
        }
    }

    LGG_TRACE("Solved with ", iterations, " iterations");
    LGG_TRACE("Solved with ", totpromos, " promotions");
    LGG_TRACE("Solved with ", doms, " dominions");
    LGG_TRACE("Solved with ", maxqueries, " max local iterations");
    LGG_TRACE("Solved with ", maxpromos, " max local promotions");

    return solution;
}

void PriorityPromotionSolver::initialize(const Graph &graph) {
    // Reset counters
    promos = 0;
    doms = 0;
    queries = 0;
    maxpromos = 0;
    maxqueries = 0;
    totpromos = 0;
    iterations = 0;
    // Initialize vertex data using maps
    vertex_priority_.clear();
    vertex_player_.clear();
    max_priority_ = 0;
    // Initialize vertex data
    const auto [VERTICES_BEGIN, VERTICES_END] = boost::vertices(graph);
    for (auto it = VERTICES_BEGIN; it != VERTICES_END; ++it) {
        Vertex v = *it;
        int priority = graph[v].priority;
        int player = graph[v].player;
        vertex_priority_[v] = priority;
        vertex_player_[v] = player;
        max_priority_ = std::max(max_priority_, priority);
    }
}

void PriorityPromotionSolver::build_adjacency_cache(const Graph &graph) {
    // Initialize adjacency maps
    predecessors_.clear();
    successors_.clear();

    // Build adjacency lists
    const auto [EDGES_BEGIN, EDGES_END] = boost::edges(graph);
    for (auto edge_it = EDGES_BEGIN; edge_it != EDGES_END; ++edge_it) {
        Vertex source = boost::source(*edge_it, graph);
        Vertex target = boost::target(*edge_it, graph);

        successors_[source].push_back(target);
        predecessors_[target].push_back(source);
    }
}

void PriorityPromotionSolver::attract(int priority) {
    const int player = priority & 1;
    auto &region_vertices = regions_[priority];

    // If queue is empty, add all nodes in region to queue
    if (queue_.empty()) {
        for (Vertex v : region_vertices) {
            queue_.push(v);
        }
    }

    // Attract predecessors
    while (!queue_.empty()) {
        Vertex current = queue_.front();
        queue_.pop();

        // Check all predecessors of current vertex
        for (Vertex pred : predecessors_[current]) {
            if (is_disabled(pred) || get_region(pred) > priority) {
                continue; // Skip disabled or higher priority vertices
            } else if (get_region(pred) == priority) {
                // Already in region - update strategy if needed
                if (get_player(pred) == player && !has_strategy(pred)) {
                    strategy_[pred] = current;
                    has_strategy_[pred] = true;
                }
            } else if (get_player(pred) == player) {
                // Same player - attract to region
                region_vertices.push_back(pred);
                region_[pred] = priority;
                strategy_[pred] = current;
                has_strategy_[pred] = true;
                queue_.push(pred);
            } else {
                // Opponent player - check if forced (all successors in region or higher)
                bool can_escape = false;
                for (Vertex succ : successors_[pred]) {
                    if (!is_disabled(succ) && get_region(succ) < priority) {
                        can_escape = true;
                        break;
                    }
                }

                if (!can_escape) {
                    // All successors are in region or higher - attract
                    region_vertices.push_back(pred);
                    region_[pred] = priority;
                    has_strategy_[pred] = false; // No strategy for opponent
                    queue_.push(pred);
                }
            }
        }
    }
}

void PriorityPromotionSolver::promote(int from_priority, int to_priority) {
    assert(from_priority < to_priority);
    promos++;

    // Move all vertices from source region to target region and add to queue
    auto &from_region = regions_[from_priority];
    auto &to_region = regions_[to_priority];

    for (Vertex v : from_region) {
        region_[v] = to_priority;
        queue_.push(v);
    }

    // Merge regions
    to_region.insert(to_region.end(), from_region.begin(), from_region.end());
    from_region.clear();

    // Attract from newly promoted vertices
    attract(to_priority);
}

void PriorityPromotionSolver::reset_region(int priority) {
    auto &region_vertices = regions_[priority];

    for (Vertex v : region_vertices) {
        if (is_disabled(v)) {
            region_[v] = -2; // Mark as disabled region
        } else if (get_region(v) == priority) {
            region_[v] = get_original_priority(v); // Reset to original priority
            has_strategy_[v] = false;              // Reset strategy
        }
    }

    region_vertices.clear();
}

bool PriorityPromotionSolver::setup_region(int vertex_index, int priority, bool must_reset) {
    // PP always resets (must_reset is always true in PP)
    if (!regions_[priority].empty()) {
        reset_region(priority);
    }

    // Add all escapes (vertices with original priority p that should be in the region)
    for (int j = vertex_index; j < static_cast<int>(sorted_vertices_.size()) &&
                               get_original_priority(sorted_vertices_[j]) == priority;
         j++) {
        Vertex v = sorted_vertices_[j];

        if (is_disabled(v)) {
            region_[v] = -2; // set to disabled
        } else if (get_region(v) == get_original_priority(v)) {
            // Vertex is at its original priority level, should be in region
            regions_[priority].push_back(v);
            region_[v] = priority;
            has_strategy_[v] = false; // Reset strategy
        }
    }

    // If region is empty, return false
    if (regions_[priority].empty()) {
        return false;
    }

    // Attract to maximize the region
    attract(priority);
    return true;
}

void PriorityPromotionSolver::set_dominion(int priority, Solution &solution) {
    const int player = priority & 1;
    auto &region_vertices = regions_[priority];

    // Create a filtered graph view that only contains non-disabled vertices
    // For simplicity, we'll use a different approach: manual attractor computation
    // on the implicitly filtered graph

    std::set<Vertex> dominion_core;
    std::set<Vertex> full_attractor;
    std::map<Vertex, Vertex> strategy_map;
    doms++; // Increment dominions counter

    // Step 1: Collect initial vertices in the dominion (current region)
    for (Vertex v : region_vertices) {
        if (get_region(v) == priority && !is_disabled(v)) {
            dominion_core.insert(v);
            full_attractor.insert(v);
        }
    }

    // Step 2: Manually compute attractor, respecting disabled vertices
    std::queue<Vertex> worklist;
    for (Vertex v : dominion_core) {
        worklist.push(v);
    }

    while (!worklist.empty()) {
        Vertex current = worklist.front();
        worklist.pop();

        // Check all non-disabled predecessors
        for (Vertex pred : predecessors_[current]) {
            if (is_disabled(pred) || full_attractor.count(pred)) {
                continue; // Skip disabled or already in attractor
            }

            bool should_add = false;

            if (get_player(pred) == player) {
                // Player vertex: add if has edge to attractor
                should_add = true;
                strategy_map[pred] = current;
            } else {
                // Opponent vertex: add if all non-disabled edges lead to attractor
                bool all_to_attractor = true;
                bool has_edges = false;

                for (Vertex succ : successors_[pred]) {
                    if (is_disabled(succ))
                        continue;
                    has_edges = true;
                    if (!full_attractor.count(succ)) {
                        all_to_attractor = false;
                        break;
                    }
                }

                if (has_edges && all_to_attractor) {
                    should_add = true;
                    // For opponent, choose any edge to attractor
                    for (Vertex succ : successors_[pred]) {
                        if (!is_disabled(succ) && full_attractor.count(succ)) {
                            strategy_map[pred] = succ;
                            break;
                        }
                    }
                }
            }

            if (should_add) {
                full_attractor.insert(pred);
                worklist.push(pred);
            }
        }
    }

    // Step 3: Mark all vertices in the full attractor as won by the player
    for (Vertex v : full_attractor) {
        solution.set_winning_player(v, player);
        disabled_[v] = true;
    }

    // Step 4: Set strategies from attractor computation
    // Only set strategies for vertices owned by the winning player
    for (const auto &[from, to] : strategy_map) {
        int vertex_owner = get_player(from);
        // PLAYER is the winning player for this dominion
        if (vertex_owner == player) {
            solution.set_strategy(from, to);
        }
        // If vertex_owner != PLAYER, then this vertex is owned by the opponent
        // and should not have a strategy set
    }
}

int PriorityPromotionSolver::get_region_status(int vertex_index, int priority) {
    const int player = priority & 1;

    // Check if the region is closed in the subgame
    for (int j = vertex_index; j < static_cast<int>(sorted_vertices_.size()) &&
                               get_original_priority(sorted_vertices_[j]) == priority;
         j++) {
        Vertex v = sorted_vertices_[j];

        if (is_disabled(v) || get_region(v) > priority) {
            continue; // Skip disabled or higher regions
        }

        if (get_player(v) == player) {
            // Region player owns this vertex - must have a strategy
            if (!has_strategy(v)) {
                return -2; // Region is open
            }
        } else {
            // Opponent owns this vertex - check for escapes to lower regions
            for (Vertex succ : successors_[v]) {
                if (is_disabled(succ))
                    continue;
                if (get_region(succ) < priority) {
                    return -2; // Open - escape to lower region
                }
            }
        }
    }

    // Region is closed, find promotion target (lowest higher region)
    int lowest_promotion_target = -1;

    // Check for promotion opportunities (exits to higher regions)
    for (Vertex v : regions_[priority]) {
        if (get_player(v) != player) {
            // Check successors for higher regions
            for (Vertex succ : successors_[v]) {
                if (is_disabled(succ))
                    continue;
                int succ_region = get_region(succ);
                if (succ_region > priority &&
                    (succ_region < lowest_promotion_target || lowest_promotion_target == -1)) {
                    lowest_promotion_target = succ_region;
                }
            }
        }
    }

    return lowest_promotion_target; // -1 if dominion, target region otherwise
}
} // namespace parity
} // namespace ggg
