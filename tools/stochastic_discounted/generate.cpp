#include "libggg/stochastic_discounted/graph.hpp"
#include "libggg/utils/game_graph_generator.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace po = boost::program_options;

class StochasticDiscountedGameGenerator : public ggg::utils::GameGraphGenerator {
  public:
    StochasticDiscountedGameGenerator() : GameGraphGenerator("Stochastic Discounted Generator Options") {
        desc_.add_options()("player-vertices", po::value<int>()->default_value(10), "Number of player-owned vertices (players 0/1)");
        desc_.add_options()("stochastic-vertices", po::value<int>()->default_value(10), "Number of stochastic vertices (player -1), must satisfy: player-vertices <= stochastic-vertices <= player-vertices^2");
        desc_.add_options()("min-weight", po::value<int>()->default_value(-10), "Minimum edge weight");
        desc_.add_options()("max-weight", po::value<int>()->default_value(10), "Maximum edge weight");
        desc_.add_options()("discount", po::value<double>()->default_value(0.95), "Discount factor (0 < discount < 1)");
    }

    int run(int argc, char *argv[]) {
        po::variables_map vm;
        try {
            po::store(po::parse_command_line(argc, argv, desc_), vm);
            po::notify(vm);
        } catch (const std::exception &e) {
            std::cerr << "Error parsing options: " << e.what() << std::endl;
            return 1;
        }

        if (vm.count("help")) {
            std::cout << desc_ << std::endl;
            std::cout
                << "Model-specific notes for this generator:\n"
                << "  - Use --player-vertices and --stochastic-vertices to control game size.\n"
                << "  - --vertices is a shared base option and is ignored by this generator.\n"
                << "  - Edge structure is strictly bipartite: (player 0/1 -> player -1) and (player -1 -> player 0/1).\n"
                << "  - Reverse edge pairs are forbidden: if u->v exists, then v->u does not.\n"
                << "  - Outgoing probabilities from each stochastic vertex are normalized to sum to 1.\n";
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
        const auto stochastic_vertices = vm["stochastic-vertices"].as<int>();
        const auto discount = vm["discount"].as<double>();
        if (player_vertices <= 0) {
            std::cerr << "Error: player-vertices must be positive" << std::endl;
            return false;
        }
        if (player_vertices == 1) {
            std::cerr << "Error: player-vertices must be >= 2 (player-vertices == 1 is unsatisfiable with the no-reverse-edge rule)" << std::endl;
            return false;
        }
        if (stochastic_vertices < player_vertices) {
            std::cerr << "Error: stochastic-vertices must be >= player-vertices" << std::endl;
            return false;
        }
        const long long max_stochastic_vertices =
            static_cast<long long>(player_vertices) * static_cast<long long>(player_vertices);
        if (static_cast<long long>(stochastic_vertices) > max_stochastic_vertices) {
            std::cerr << "Error: stochastic-vertices must be <= player-vertices^2" << std::endl;
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
        const auto stochastic_vertices = vm["stochastic-vertices"].as<int>();
        std::cout << "Generating " << count << " stochastic discounted games" << std::endl;
        std::cout << "Player-owned vertices (players 0/1): " << player_vertices << std::endl;
        std::cout << "Stochastic vertices (player -1): " << stochastic_vertices << std::endl;
        std::cout << "Output directory: " << output_dir << std::endl;
    }

    std::string get_filename_prefix() const override { return "stochastic_discounted_game_"; }

    void generate_single_game(const po::variables_map &vm, std::mt19937 &gen, std::ofstream &file) override {
        const auto player_vertices = vm["player-vertices"].as<int>();
        const auto stochastic_vertices = vm["stochastic-vertices"].as<int>();
        const auto min_weight = vm["min-weight"].as<int>();
        const auto max_weight = vm["max-weight"].as<int>();
        const auto discount = vm["discount"].as<double>();

        auto graph = generate_stochastic_discounted_game(player_vertices, stochastic_vertices, min_weight, max_weight, discount, gen);
        ggg::stochastic_discounted::graph::write(graph, file);
    }

  private:
    static ggg::stochastic_discounted::graph::Graph generate_stochastic_discounted_game(int player_vertices,
                                                                                        int stochastic_vertices,
                                                                                        int min_weight,
                                                                                        int max_weight,
                                                                                        double discount,
                                                                                        std::mt19937 &gen) {
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
        // Invariant for every pair (p, s): not both player_to_stoch[p][s] and stoch_to_player[s][p].
        std::vector<std::vector<bool>> player_to_stoch(player_vertices,
                                                       std::vector<bool>(stochastic_vertices, false));
        std::vector<std::vector<bool>> stoch_to_player(stochastic_vertices,
                                                       std::vector<bool>(player_vertices, false));

        // Base coverage for players: each player gets at least one outgoing edge to a distinct
        // stochastic vertex (possible since stochastic_vertices >= player_vertices).
        std::vector<int> stoch_perm(stochastic_vertices);
        std::iota(stoch_perm.begin(), stoch_perm.end(), 0);
        std::shuffle(stoch_perm.begin(), stoch_perm.end(), gen);
        for (int p = 0; p < player_vertices; ++p) {
            player_to_stoch[p][stoch_perm[p]] = true;
        }

        // Base coverage for stochastic vertices: each stochastic vertex gets at least one
        // outgoing edge to a player that does NOT already point to it.
        for (int s = 0; s < stochastic_vertices; ++s) {
            std::vector<int> allowed;
            allowed.reserve(player_vertices);
            for (int p = 0; p < player_vertices; ++p) {
                if (!player_to_stoch[p][s]) {
                    allowed.push_back(p);
                }
            }

            // By construction of the base player pass, this should always hold.
            if (allowed.empty()) {
                throw std::runtime_error("Internal generator error: no admissible stochastic target");
            }

            std::shuffle(allowed.begin(), allowed.end(), gen);
            stoch_to_player[s][allowed.front()] = true;
        }

        // Add extra player->stochastic edges while preserving the no-reverse-edge invariant.
        for (int p = 0; p < player_vertices; ++p) {
            std::vector<int> candidates;
            candidates.reserve(stochastic_vertices);
            for (int s = 0; s < stochastic_vertices; ++s) {
                if (!player_to_stoch[p][s] && !stoch_to_player[s][p]) {
                    candidates.push_back(s);
                }
            }

            if (!candidates.empty()) {
                std::shuffle(candidates.begin(), candidates.end(), gen);
                std::uniform_int_distribution<int> extra_count_dist(0, static_cast<int>(candidates.size()));
                const int extra = extra_count_dist(gen);
                for (int i = 0; i < extra; ++i) {
                    player_to_stoch[p][candidates[i]] = true;
                }
            }
        }

        // Add extra stochastic->player edges while preserving the no-reverse-edge invariant.
        for (int s = 0; s < stochastic_vertices; ++s) {
            std::vector<int> candidates;
            candidates.reserve(player_vertices);
            for (int p = 0; p < player_vertices; ++p) {
                if (!stoch_to_player[s][p] && !player_to_stoch[p][s]) {
                    candidates.push_back(p);
                }
            }

            if (!candidates.empty()) {
                std::shuffle(candidates.begin(), candidates.end(), gen);
                std::uniform_int_distribution<int> extra_count_dist(0, static_cast<int>(candidates.size()));
                const int extra = extra_count_dist(gen);
                for (int i = 0; i < extra; ++i) {
                    stoch_to_player[s][candidates[i]] = true;
                }
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

        // Materialize stochastic->player edges: probabilities sum to exactly 1 per stochastic vertex.
        for (int s = 0; s < stochastic_vertices; ++s) {
            std::vector<int> targets;
            targets.reserve(player_vertices);
            for (int p = 0; p < player_vertices; ++p) {
                if (stoch_to_player[s][p]) {
                    targets.push_back(p);
                }
            }

            std::vector<double> probs(targets.size());
            double total = 0.0;
            for (auto &pr : probs) {
                pr = prob_dist(gen);
                total += pr;
            }
            double assigned = 0.0;
            for (std::size_t i = 0; i + 1 < probs.size(); ++i) {
                probs[i] /= total;
                assigned += probs[i];
            }
            probs.back() = 1.0 - assigned;

            for (std::size_t i = 0; i < targets.size(); ++i) {
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
