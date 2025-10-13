#include "libggg/parity/graph.hpp"
#include "libggg/utils/game_graph_generator.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace po = boost::program_options;

/**
 * @brief Tool to generate random parity games for testing solvers
 */
class ParityGameGenerator : public ggg::utils::GameGraphGenerator {
  public:
    ParityGameGenerator()
        : GameGraphGenerator("Parity Game Generator Options") {
        // Parity-game specific options
        desc_.add_options()("max-priority", po::value<int>()->default_value(5), "Maximum vertex priority");
        desc_.add_options()("min-out-degree", po::value<int>()->default_value(1), "Minimum out-degree per vertex");
        desc_.add_options()("max-out-degree", po::value<int>(), "Maximum out-degree per vertex (default: vertices-1)");
    }

  protected:
    // Implement abstract methods from base class
    bool validate_parameters(const po::variables_map &vm) override {
        const auto vertices = vm["vertices"].as<int>();
        const auto min_out_degree = vm["min-out-degree"].as<int>();
        const auto max_out_degree = vm.count("max-out-degree") ? vm["max-out-degree"].as<int>() : vertices - 1;

        if (min_out_degree < 1) {
            std::cerr << "Error: min-out-degree must be at least 1" << std::endl;
            return false;
        }
        if (max_out_degree < min_out_degree) {
            std::cerr << "Error: max-out-degree must be at least min-out-degree" << std::endl;
            return false;
        }
        if (max_out_degree > vertices) {
            std::cerr << "Error: max-out-degree must be at most number of vertices (max: " << vertices << ")" << std::endl;
            return false;
        }
        return true;
    }

    void print_generation_info(const po::variables_map &vm, const std::string &output_dir, int count, unsigned int seed) override {
        const auto vertices = vm["vertices"].as<int>();
        const auto min_out_degree = vm["min-out-degree"].as<int>();
        const auto max_out_degree = vm.count("max-out-degree") ? vm["max-out-degree"].as<int>() : vertices - 1;

        std::cout << "Generating " << count << " parity games with " << vertices << " vertices each" << std::endl;
        std::cout << "Out-degree range: [" << min_out_degree << ", " << max_out_degree << "]" << std::endl;
        std::cout << "Random seed: " << seed << std::endl;
        std::cout << "Output directory: " << output_dir << std::endl
                  << std::endl;
    }

    std::string get_filename_prefix() const override {
        return "parity_game_";
    }

    void generate_single_game(const po::variables_map &vm, std::mt19937 &gen, std::ofstream &file) override {
        const auto vertices = vm["vertices"].as<int>();
        const auto max_priority = vm["max-priority"].as<int>();
        const auto min_out_degree = vm["min-out-degree"].as<int>();
        const auto max_out_degree = vm.count("max-out-degree") ? vm["max-out-degree"].as<int>() : vertices - 1;

        auto graph = generate_parity_game(vertices, max_priority, min_out_degree, max_out_degree, gen);
        ggg::parity::graph::write(graph, file);
    }

  private:
    // Internal single-game generator
    static ggg::parity::graph::Graph generate_parity_game(int vertices,
                                                         int max_priority,
                                                         int min_out_degree,
                                                         int max_out_degree,
                                                         std::mt19937 &gen) {

        std::uniform_int_distribution<int> player_dist(0, 1);
        std::uniform_int_distribution<int> priority_dist(0, max_priority);
        std::uniform_int_distribution<int> out_degree_dist(min_out_degree, max_out_degree);

        ggg::parity::graph::Graph graph;

        // Generate vertices
        std::vector<ggg::parity::graph::Vertex> vertex_descriptors;
        vertex_descriptors.reserve(vertices);

        for (int i = 0; i < vertices; ++i) {
            const auto player = player_dist(gen);
            const auto priority = priority_dist(gen);
            const auto name = "v" + std::to_string(i);

            auto vertex = ggg::parity::graph::add_vertex(graph, name, player, priority);
            vertex_descriptors.push_back(vertex);
        }

        // Generate edges with controlled out-degrees
        for (int i = 0; i < vertices; ++i) {
            // Determine out-degree for this vertex
            const auto out_degree = out_degree_dist(gen);

            // Select unique target vertices (including self-loops)
            std::vector<int> available_targets;
            for (int j = 0; j < vertices; ++j) {
                available_targets.push_back(j);
            }

            // Shuffle and select first out_degree targets
            std::shuffle(available_targets.begin(), available_targets.end(), gen);

            // Take the first out_degree targets (or all if not enough available)
            int actual_degree = std::min(out_degree, static_cast<int>(available_targets.size()));

            for (int k = 0; k < actual_degree; ++k) {
                const auto target = available_targets[k];
                const auto label = "edge_" + std::to_string(i) + "_" + std::to_string(target);

                ggg::parity::graph::add_edge(graph, vertex_descriptors[i], vertex_descriptors[target], label);
            }
        }

        return graph;
    }
};

// Inline main so we don't need a separate _main file.
int main(int argc, char **argv) {
    ParityGameGenerator gen;
    int result = gen.run(argc, argv);
    return result;
}
