#include "libggg/stochastic_discounted/graph.hpp"
#include "libggg/utils/game_graph_generator.hpp"
#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace po = boost::program_options;

class StochasticDiscountedGameGenerator : public ggg::utils::GameGraphGenerator {
  public:
    static std::vector<std::string> normalize_stochastic_aliases(int argc, char *argv[]) {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            std::string arg(argv[i]);
            if (arg == "-vx" || arg == "--vx") {
                args.emplace_back("--player-vertices");
            } else if (arg == "-mo" || arg == "--mo") {
                args.emplace_back("--min-player-outgoing");
            } else if (arg == "-mxo" || arg == "--mxo") {
                args.emplace_back("--max-player-outgoing");
            } else if (arg == "-mw" || arg == "--mw") {
                args.emplace_back("--min-weight");
            } else if (arg == "-mxw" || arg == "--mxw") {
                args.emplace_back("--max-weight");
            } else if (arg == "-d" || arg == "--d") {
                args.emplace_back("--discount");
            } else {
                args.emplace_back(std::move(arg));
            }
        }
        return args;
    }
    StochasticDiscountedGameGenerator() : GameGraphGenerator("Stochastic Discounted Generator Options") {
        desc_.add_options()("player-vertices,vx", po::value<int>()->default_value(10), "Number of player-owned vertices (players 0/1)");
        desc_.add_options()("min-player-outgoing,mo", po::value<int>()->default_value(1), "Minimum outgoing edges per player-owned vertex (must satisfy 1 <= min <= N-1)");
        desc_.add_options()("max-player-outgoing,mxo", po::value<int>()->default_value(3), "Maximum outgoing edges per player-owned vertex (must satisfy min <= max <= N-1)");
        desc_.add_options()("min-weight,mw", po::value<int>()->default_value(-10), "Minimum edge weight");
        desc_.add_options()("max-weight,mxw", po::value<int>()->default_value(10), "Maximum edge weight");
        desc_.add_options()("discount,d", po::value<double>()->default_value(0.95), "Discount factor (0 < discount < 1)");
    }

    void write_dot(const ggg::stochastic_discounted::graph::Graph &g, std::ostream &os) {
        os << "digraph G {\n";
        // Vertices
        for (auto v : boost::make_iterator_range(boost::vertices(g))) {
            os << "v" << v << " [name=\"" << g[v].name << "\", player=" << g[v].player << "];\n";
        }
        // Edges
        for (auto e : boost::make_iterator_range(boost::edges(g))) {
            auto source = boost::source(e, g);
            auto target = boost::target(e, g);
            os << "v" << source << "->v" << target << "  [";
            if (g[source].player != -1) {
                os << "discount=" << g[e].discount << ", label=\"" << g[source].name << "->" << g[target].name << "\", weight=" << g[e].weight;
            } else {
                os << "label=\"" << g[source].name << "->" << g[target].name << "\", probability=" << g[e].probability;
            }
            os << "];\n";
        }
        os << "}\n";
    }

    void print_help() const {
        std::cout << "Stochastic Discounted Generator Options:\n"
                  << "  -h [ --help ]                         Show help message\n"
                  << "  -o [ --output-dir ] arg (=./generated)\n"
                  << "                                        Output directory\n"
                  << "  --seed arg                            Random seed (default: random)\n"
                  << "  --verbose                             Verbose output\n"
                  << "  -c [ --count ] arg (=1)               Number of games to generate\n"
                  << "  --player-vertices, -vx arg (=10)      Number of player-owned vertices (players 0/1)\n"
                  << "  --min-player-outgoing, -mo arg (=1)  Minimum outgoing edges per player-owned vertex (must satisfy 1 <= min <= N-1)\n"
                  << "  --max-player-outgoing, -mxo arg (=3)  Maximum outgoing edges per player-owned vertex (must satisfy min <= max <= N-1)\n"
                  << "  --min-weight, -mw arg (=-10)          Minimum edge weight\n"
                  << "  --max-weight, -mxw arg (=10)          Maximum edge weight\n"
                  << "  --discount, -d arg (=0.95)            Discount factor (0 < discount < 1)\n\n"
                  << "Additional notes for this generator:\n"
                  << "  - Constraints: 1 <= min-player-outgoing <= max-player-outgoing <= -1+player-vertices.\n"
                  << "  - Edge structure is alternating between player-owned and stochastic vertices.\n"
                  << "  - Outgoing probabilities from each stochastic vertex are normalized to sum to 1.\n";
    }

    int run(int argc, char *argv[]) {
        po::variables_map vm;
        try {
            const auto args = normalize_stochastic_aliases(argc, argv);
            po::store(po::command_line_parser(args)
                          .options(desc_)
                          .style(po::command_line_style::default_style |
                                 po::command_line_style::allow_long_disguise)
                          .run(),
                      vm);
            po::notify(vm);
        } catch (const std::exception &e) {
            std::cerr << "Error parsing options: " << e.what() << std::endl;
            return 1;
        }

        if (vm.count("help")) {
            print_help();
            return 0;
        }

        const auto output_dir = vm["output-dir"].as<std::string>();
        const auto count = vm["count"].as<int>();
        unsigned int seed = vm.count("seed") ? vm["seed"].as<unsigned int>() : std::random_device{}();
        const bool verbose = vm["verbose"].as<bool>();

        if (!validate_parameters(vm)) {
            return 1;
        }

        std::filesystem::create_directories(output_dir);

        std::mt19937 gen(seed);

        print_generation_info(vm, output_dir, count, seed);

        for (int i = 0; i < count; ++i) {
            const auto filename = std::filesystem::path(output_dir) /
                                  (get_filename_prefix() + std::to_string(i) + ".dot");
            std::ofstream ofs(filename);
            if (!ofs) {
                std::cerr << "Failed to open output file: " << filename << std::endl;
                return 1;
            }

            generate_single_game(vm, gen, ofs);

            if (verbose) {
                std::cout << "Wrote: " << filename << std::endl;
            }
        }

        return 0;
    }

  protected:
    bool validate_parameters(const po::variables_map &vm) override {
        const auto player_vertices = vm["player-vertices"].as<int>();
        const auto min_player_outgoing = vm["min-player-outgoing"].as<int>();
        const auto max_player_outgoing = vm["max-player-outgoing"].as<int>();
        const auto discount = vm["discount"].as<double>();
        if (player_vertices <= 0) {
            std::cerr << "Error: player-vertices must be positive" << std::endl;
            return false;
        }
        if (player_vertices == 1) {
            std::cerr << "Error: player-vertices must be >= 2" << std::endl;
            return false;
        }
        const auto max_allowed = player_vertices - 1;
        if (min_player_outgoing < 1 || min_player_outgoing > max_allowed) {
            std::cerr << "Error: min-player-outgoing must satisfy 1 <= min-player-outgoing <= player-vertices-1" << std::endl;
            return false;
        }
        if (max_player_outgoing < min_player_outgoing || max_player_outgoing > max_allowed) {
            std::cerr << "Error: max-player-outgoing must satisfy min-player-outgoing <= max-player-outgoing <= player-vertices-1" << std::endl;
            return false;
        }
        const long long max_stochastic_vertices =
            static_cast<long long>(player_vertices) * static_cast<long long>(max_player_outgoing);
        if (max_stochastic_vertices > static_cast<long long>(std::numeric_limits<int>::max())) {
            std::cerr << "Error: configuration produces too many stochastic vertices" << std::endl;
            return false;
        }
        if (!(discount > 0.0 && discount < 1.0)) {
            std::cerr << "Error: discount must be in (0,1)" << std::endl;
            return false;
        }
        return true;
    }

    void print_generation_info(const po::variables_map &vm, const std::string &output_dir, int count, unsigned int seed) override {
        const auto player_vertices = vm["player-vertices"].as<int>();
        const auto min_player_outgoing = vm["min-player-outgoing"].as<int>();
        const auto max_player_outgoing = vm["max-player-outgoing"].as<int>();
        std::cout << "Generating " << count << " stochastic discounted games" << std::endl;
        std::cout << "Player-owned vertices (players 0/1): " << player_vertices << std::endl;
        std::cout << "Player outgoing edges range per vertex: [" << min_player_outgoing << ", "
                  << max_player_outgoing << "]" << std::endl;
        std::cout << "Derived stochastic vertices per game range: ["
                  << player_vertices * min_player_outgoing << ", "
                  << player_vertices * max_player_outgoing << "]" << std::endl;
        std::cout << "Seed: " << seed << std::endl;
        std::cout << "Output directory: " << output_dir << std::endl;
    }

    std::string get_filename_prefix() const override { return "stochastic_discounted_game_"; }

    void generate_single_game(const po::variables_map &vm, std::mt19937 &gen, std::ofstream &file) override {
        const auto player_vertices = vm["player-vertices"].as<int>();
        const auto min_player_outgoing = vm["min-player-outgoing"].as<int>();
        const auto max_player_outgoing = vm["max-player-outgoing"].as<int>();
        const auto min_weight = vm["min-weight"].as<int>();
        const auto max_weight = vm["max-weight"].as<int>();
        const auto discount = vm["discount"].as<double>();

        auto graph = generate_stochastic_discounted_game(player_vertices, min_player_outgoing, max_player_outgoing,
                                                         min_weight, max_weight, discount, gen);
        write_dot(graph, file);
    }

  private:
    static ggg::stochastic_discounted::graph::Graph generate_stochastic_discounted_game(int player_vertices,
                                                                                        int min_player_outgoing,
                                                                                        int max_player_outgoing,
                                                                                        int min_weight,
                                                                                        int max_weight,
                                                                                        double discount,
                                                                                        std::mt19937 &gen) {
        std::uniform_int_distribution<int> player_outgoing_dist(min_player_outgoing, max_player_outgoing);
        std::vector<int> player_outdegrees(player_vertices);
        int stochastic_vertices = 0;
        for (auto &outdeg : player_outdegrees) {
            outdeg = player_outgoing_dist(gen);
            stochastic_vertices += outdeg;
        }

        const auto total_vertices = player_vertices + stochastic_vertices;
        std::uniform_int_distribution<int> player_dist(0, 1);
        std::uniform_int_distribution<int> weight_dist(min_weight, max_weight);
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

        ggg::stochastic_discounted::graph::Graph graph;
        std::vector<ggg::stochastic_discounted::graph::Vertex> vdesc;
        vdesc.reserve(total_vertices);

        // Create player-owned vertices (player 0 or 1, chosen at random)
        for (int i = 0; i < player_vertices; ++i) {
            vdesc.push_back(ggg::stochastic_discounted::graph::add_vertex(
                graph, "v" + std::to_string(i), player_dist(gen)));
        }

        // Create stochastic vertices (player -1)
        for (int i = 0; i < stochastic_vertices; ++i) {
            vdesc.push_back(ggg::stochastic_discounted::graph::add_vertex(
                graph, "v" + std::to_string(player_vertices + i), -1));
        }

        // Directed bipartite incidence matrices.
        // Constraint: no deterministic cycles (cycles where all s->p edges have probability 1).
        std::vector<std::vector<bool>> player_to_stoch(player_vertices,
                                                       std::vector<bool>(stochastic_vertices, false));
        std::vector<std::vector<bool>> stoch_to_player(stochastic_vertices,
                                                       std::vector<bool>(player_vertices, false));

        // Assign each stochastic vertex to exactly one owning player so that player outdegree
        // requirements are met exactly and each stochastic vertex has at least N-1 admissible
        // targets for outgoing stochastic->player edges.
        std::vector<int> stochastic_owner;
        stochastic_owner.reserve(stochastic_vertices);
        for (int p = 0; p < player_vertices; ++p) {
            for (int k = 0; k < player_outdegrees[p]; ++k) {
                stochastic_owner.push_back(p);
            }
        }
        std::shuffle(stochastic_owner.begin(), stochastic_owner.end(), gen);
        for (int s = 0; s < stochastic_vertices; ++s) {
            player_to_stoch[stochastic_owner[s]][s] = true;
        }

        // For each stochastic vertex, pick random outdegree in [1, N-1] and connect to player vertices.
        for (int s = 0; s < stochastic_vertices; ++s) {
            std::vector<int> all_players;
            all_players.reserve(player_vertices);
            for (int p = 0; p < player_vertices; ++p) {
                all_players.push_back(p);
            }

            std::uniform_int_distribution<int> stoch_outdegree_dist(1, player_vertices);
            const int outdegree = stoch_outdegree_dist(gen);
            std::shuffle(all_players.begin(), all_players.end(), gen);
            for (int i = 0; i < outdegree && i < static_cast<int>(all_players.size()); ++i) {
                stoch_to_player[s][all_players[i]] = true;
            }
        }

        // Materialize player->stochastic edges: weighted and discounted, non-probabilistic.
        for (int p = 0; p < player_vertices; ++p) {
            for (int s = 0; s < stochastic_vertices; ++s) {
                if (player_to_stoch[p][s]) {
                    ggg::stochastic_discounted::graph::add_edge(
                        graph, vdesc[p], vdesc[player_vertices + s],
                        std::string(""), weight_dist(gen), discount, 0.0);
                }
            }
        }

        // Track stochastic->player edge probabilities to enforce no deterministic cycles.
        // prob_matrix[s][p] = probability of s->p, or -1 if edge doesn't exist.
        std::vector<std::vector<double>> prob_matrix(stochastic_vertices,
                                                     std::vector<double>(player_vertices, -1.0));

        // Helper lambda to detect if a deterministic path exists from a player to a stochastic vertex.
        // A path is deterministic if all s->p edges have probability >= prob_threshold.
        auto has_deterministic_path_to_stoch = [&](int start_player, int target_stoch, double prob_threshold = 0.99) -> bool {
            std::vector<bool> visited_stoch(stochastic_vertices, false);
            std::function<bool(int)> dfs = [&](int curr_stoch) -> bool {
                if (visited_stoch[curr_stoch])
                    return false;
                if (curr_stoch == target_stoch)
                    return true;
                visited_stoch[curr_stoch] = true;

                // Check all player targets from current stochastic vertex
                for (int p = 0; p < player_vertices; ++p) {
                    if (prob_matrix[curr_stoch][p] >= prob_threshold) {
                        if (p == start_player) {
                            visited_stoch[curr_stoch] = false;
                            return true; // Found cycle back to start player
                        }

                        // Try to continue from player p to other stochastic vertices
                        for (int next_s = 0; next_s < stochastic_vertices; ++next_s) {
                            if (!visited_stoch[next_s] && player_to_stoch[p][next_s]) {
                                if (dfs(next_s)) {
                                    visited_stoch[curr_stoch] = false;
                                    return true;
                                }
                            }
                        }
                    }
                }
                visited_stoch[curr_stoch] = false;
                return false;
            };

            // Start DFS from target stochastic vertex
            return dfs(target_stoch);
        };

        // Materialize stochastic->player edges: probabilities sum to exactly 1 per stochastic vertex.
        for (int s = 0; s < stochastic_vertices; ++s) {
            std::vector<int> targets;
            targets.reserve(player_vertices);
            for (int p = 0; p < player_vertices; ++p) {
                if (stoch_to_player[s][p]) {
                    targets.push_back(p);
                }
            }

            // Assign probabilities, ensuring no deterministic cycles.
            std::vector<double> probs(targets.size());
            double total = 0.0;
            for (auto &pr : probs) {
                pr = prob_dist(gen);
                total += pr;
            }
            if (total <= 0.0) {
                double equal = 1.0 / static_cast<double>(probs.size());
                for (auto &pr : probs) {
                    pr = equal;
                }
            } else {
                for (auto &pr : probs) {
                    pr /= total;
                }
            }

            auto normalize_probs = [&](std::vector<double> &values) {
                double sum = 0.0;
                for (auto v : values) {
                    sum += v;
                }
                if (sum <= 0.0) {
                    double equal = 1.0 / static_cast<double>(values.size());
                    for (auto &v : values) {
                        v = equal;
                    }
                } else {
                    for (auto &v : values) {
                        v /= sum;
                    }
                }
            };

            auto add_alternate_target = [&](std::vector<int> &targets, std::vector<double> &probs) {
                int current = targets[0];
                int alternate = (current + 1) % player_vertices;
                while (alternate == current) {
                    alternate = (alternate + 1) % player_vertices;
                }
                targets.push_back(alternate);
                probs.push_back(0.0);
            };

            auto break_prob1_cycle = [&](std::size_t idx) {
                if (targets.size() == 1) {
                    const double preserved = 0.98;
                    probs[0] = preserved;
                    add_alternate_target(targets, probs);
                    probs[1] = 1.0 - preserved;
                    return;
                }
                const double preserved = 0.98;
                probs[idx] = preserved;
                double remaining = 1.0 - preserved;
                double other_sum = 0.0;
                for (std::size_t j = 0; j < probs.size(); ++j) {
                    if (j == idx) {
                        continue;
                    }
                    other_sum += probs[j];
                }
                if (other_sum <= 0.0) {
                    double equal = remaining / static_cast<double>(probs.size() - 1);
                    for (std::size_t j = 0; j < probs.size(); ++j) {
                        if (j != idx) {
                            probs[j] = equal;
                        }
                    }
                } else {
                    for (std::size_t j = 0; j < probs.size(); ++j) {
                        if (j != idx) {
                            probs[j] = probs[j] / other_sum * remaining;
                        }
                    }
                }
            };

            bool adjusted = true;
            while (adjusted) {
                adjusted = false;
                for (std::size_t i = 0; i < targets.size(); ++i) {
                    if (probs[i] >= 0.99 && has_deterministic_path_to_stoch(targets[i], s, 0.99)) {
                        break_prob1_cycle(i);
                        normalize_probs(probs);
                        adjusted = true;
                        break;
                    }
                }
            }

            // Assign to prob_matrix and materialize edges
            for (std::size_t i = 0; i < targets.size(); ++i) {
                prob_matrix[s][targets[i]] = probs[i];
                ggg::stochastic_discounted::graph::add_edge(
                    graph, vdesc[player_vertices + s], vdesc[targets[i]],
                    std::string(""), 0, 0.0, probs[i]);
            }
        }

        return graph;
    }
};

// Inline main so no separate _main file is required
int main(int argc, char **argv) {
    StochasticDiscountedGameGenerator gen;
    int result = gen.run(argc, argv);
    return result;
}
