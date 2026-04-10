#include "libggg/stochastic_discounted/graph.hpp"
#include "libggg/utils/game_graph_generator.hpp"
#include <algorithm>
#include <limits>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace po = boost::program_options;

class StochasticDiscountedGameGenerator : public ggg::utils::GameGraphGenerator {
  public:
    StochasticDiscountedGameGenerator() : GameGraphGenerator("Stochastic Discounted Generator Options") {
        desc_.add_options()("player-vertices", po::value<int>()->default_value(10), "Number of player-owned vertices (players 0/1)");
        desc_.add_options()("min-player-outgoing", po::value<int>()->default_value(1), "Minimum outgoing edges per player-owned vertex (must satisfy 1 <= min <= N-1)");
        desc_.add_options()("max-player-outgoing", po::value<int>()->default_value(3), "Maximum outgoing edges per player-owned vertex (must satisfy min <= max <= N-1)");
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
                << "  - Let N = --player-vertices.\n"
                << "  - For each player-owned vertex, outgoing edges to stochastic vertices are sampled in [min-player-outgoing, max-player-outgoing].\n"
                << "  - Constraints: 1 <= min-player-outgoing <= max-player-outgoing <= N-1.\n"
                << "  - --vertices is a shared base option and is ignored by this generator.\n"
                << "  - Number of stochastic vertices is derived per game as the sum of sampled player out-degrees.\n"
                << "  - For each stochastic vertex, outgoing edges to player-owned vertices are sampled in [1, N-1].\n"
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
        const auto min_player_outgoing = vm["min-player-outgoing"].as<int>();
        const auto max_player_outgoing = vm["max-player-outgoing"].as<int>();
        const auto discount = vm["discount"].as<double>();
        if (player_vertices <= 0) {
            std::cerr << "Error: player-vertices must be positive" << std::endl;
            return false;
        }
        if (player_vertices == 1) {
            std::cerr << "Error: player-vertices must be >= 2 (player-vertices == 1 is unsatisfiable with the no-reverse-edge rule)" << std::endl;
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
        ggg::stochastic_discounted::graph::write(graph, file);
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
        // Invariant for every pair (p, s): not both player_to_stoch[p][s] and stoch_to_player[s][p].
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

        // For each stochastic vertex, pick random outdegree in [1, N-1] and connect to player
        // vertices that do not violate the no-reverse-edge invariant.
        for (int s = 0; s < stochastic_vertices; ++s) {
            std::vector<int> allowed;
            allowed.reserve(player_vertices);
            for (int p = 0; p < player_vertices; ++p) {
                if (!player_to_stoch[p][s]) {
                    allowed.push_back(p);
                }
            }

            if (allowed.size() != static_cast<std::size_t>(player_vertices - 1)) {
                throw std::runtime_error("Internal generator error: unexpected admissible stochastic target count");
            }

            std::uniform_int_distribution<int> stoch_outdegree_dist(1, player_vertices - 1);
            const int outdegree = stoch_outdegree_dist(gen);
            std::shuffle(allowed.begin(), allowed.end(), gen);
            for (int i = 0; i < outdegree; ++i) {
                stoch_to_player[s][allowed[i]] = true;
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
