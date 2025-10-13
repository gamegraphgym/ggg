#include "libggg/mean_payoff/graph.hpp"
#include "libggg/utils/game_graph_generator.hpp"
#include <random>
#include <string>

namespace po = boost::program_options;

class MPVGameGenerator : public ggg::utils::GameGraphGenerator {
  public:
    MPVGameGenerator() : GameGraphGenerator("Mean-Payoff Generator Options") {
        desc_.add_options()("min-weight", po::value<int>()->default_value(-10), "Minimum edge weight");
        desc_.add_options()("max-weight", po::value<int>()->default_value(10), "Maximum edge weight");
        desc_.add_options()("min-out-degree", po::value<int>()->default_value(1), "Minimum out-degree per vertex");
    }

  protected:
    bool validate_parameters(const po::variables_map &vm) override {
        const auto vertices = vm["vertices"].as<int>();
        if (vertices <= 0) {
            std::cerr << "Error: vertices must be positive" << std::endl;
            return false;
        }
        return true;
    }

    void print_generation_info(const po::variables_map &vm, const std::string &output_dir, int count, unsigned int seed) override {
        std::cout << "Generating " << count << " mean-payoff games" << std::endl;
        std::cout << "Output directory: " << output_dir << std::endl;
    }

    std::string get_filename_prefix() const override { return "mpv_game_"; }

    void generate_single_game(const po::variables_map &vm, std::mt19937 &gen, std::ofstream &file) override {
        const auto vertices = vm["vertices"].as<int>();
        const auto min_weight = vm["min-weight"].as<int>();
        const auto max_weight = vm["max-weight"].as<int>();
        const auto min_out_degree = vm["min-out-degree"].as<int>();

        auto graph = generate_mpv_game(vertices, min_weight, max_weight, min_out_degree, gen);
        ggg::mean_payoff::graph::write(graph, file);
    }

  private:
    static ggg::mean_payoff::graph::Graph generate_mpv_game(int vertices,
                                                            int min_weight,
                                                            int max_weight,
                                                            int min_out_degree,
                                                            std::mt19937 &gen) {
        std::uniform_int_distribution<int> player_dist(0, 1);
        std::uniform_int_distribution<int> weight_dist(min_weight, max_weight);
        std::uniform_int_distribution<int> out_degree_dist(min_out_degree, std::max(1, vertices - 1));

        ggg::mean_payoff::graph::Graph graph;
        std::vector<ggg::mean_payoff::graph::Vertex> vertices_desc;
        vertices_desc.reserve(vertices);

        for (int i = 0; i < vertices; ++i) {
            const auto player = player_dist(gen);
            const auto name = "v" + std::to_string(i);
            const auto weight = weight_dist(gen);
            const auto v = ggg::mean_payoff::graph::add_vertex(graph, name, player, weight);
            vertices_desc.push_back(v);
        }

        for (int i = 0; i < vertices; ++i) {
            const auto out_degree = out_degree_dist(gen);
            std::vector<int> targets(vertices);
            std::iota(targets.begin(), targets.end(), 0);
            std::shuffle(targets.begin(), targets.end(), gen);
            for (int k = 0; k < out_degree; ++k) {
                const auto target = targets[k];
                ggg::mean_payoff::graph::add_edge(graph, vertices_desc[i], vertices_desc[target], std::string(""));
            }
        }

        return graph;
    }
};

// Inline main to avoid a separate _main file.
int main(int argc, char **argv) {
    MPVGameGenerator gen;
    int result = gen.run(argc, argv);
    return result;
}
