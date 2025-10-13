#include "libggg/stochastic_discounted/graph.hpp"
#include "libggg/utils/game_graph_generator.hpp"
#include <random>
#include <string>

namespace po = boost::program_options;

class StochasticDiscountedGameGenerator : public ggg::utils::GameGraphGenerator {
  public:
    StochasticDiscountedGameGenerator() : GameGraphGenerator("Stochastic Discounted Generator Options") {
        desc_.add_options()("min-weight", po::value<int>()->default_value(-10), "Minimum edge weight");
        desc_.add_options()("max-weight", po::value<int>()->default_value(10), "Maximum edge weight");
        desc_.add_options()("discount", po::value<double>()->default_value(0.95), "Discount factor (0 < discount < 1)");
        desc_.add_options()("probability", po::value<double>()->default_value(0.5), "Probability for probabilistic edges (0 < p <= 1)");
    }

  protected:
    bool validate_parameters(const po::variables_map &vm) override {
        const auto vertices = vm["vertices"].as<int>();
        const auto discount = vm["discount"].as<double>();
        const auto probability = vm["probability"].as<double>();
        if (vertices <= 0) {
            std::cerr << "Error: vertices must be positive" << std::endl;
            return false;
        }
        if (!(discount > 0.0 && discount < 1.0)) {
            std::cerr << "Error: discount must be in (0,1)" << std::endl;
            return false;
        }
        if (!(probability > 0.0 && probability <= 1.0)) {
            std::cerr << "Error: probability must be in (0,1]" << std::endl;
            return false;
        }
        return true;
    }

    void print_generation_info(const po::variables_map &vm, const std::string &output_dir, int count, unsigned int seed) override {
        std::cout << "Generating " << count << " stochastic discounted games" << std::endl;
        std::cout << "Output directory: " << output_dir << std::endl;
    }

    std::string get_filename_prefix() const override { return "stochastic_discounted_game_"; }

    void generate_single_game(const po::variables_map &vm, std::mt19937 &gen, std::ofstream &file) override {
        const auto vertices = vm["vertices"].as<int>();
        const auto min_weight = vm["min-weight"].as<int>();
        const auto max_weight = vm["max-weight"].as<int>();
        const auto discount = vm["discount"].as<double>();
        const auto probability = vm["probability"].as<double>();

        auto graph = generate_stochastic_discounted_game(vertices, min_weight, max_weight, discount, probability, gen);
        ggg::stochastic_discounted::graph::write(graph, file);
    }

  private:
    static ggg::stochastic_discounted::graph::Graph generate_stochastic_discounted_game(int vertices,
                                                                                       int min_weight,
                                                                                       int max_weight,
                                                                                       double discount,
                                                                                       double probability,
                                                                                       std::mt19937 &gen) {
        std::uniform_int_distribution<int> player_dist(0, 1);
        std::uniform_int_distribution<int> weight_dist(min_weight, max_weight);
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
        std::uniform_int_distribution<int> out_degree_dist(1, std::max(1, vertices - 1));

        ggg::stochastic_discounted::graph::Graph graph;
        std::vector<ggg::stochastic_discounted::graph::Vertex> vdesc;
        vdesc.reserve(vertices);

        for (int i = 0; i < vertices; ++i) {
            const auto player = player_dist(gen);
            const auto name = "v" + std::to_string(i);
            vdesc.push_back(ggg::stochastic_discounted::graph::add_vertex(graph, name, player));
        }

        // For each vertex add a probabilistic distribution over targets with total probability <= 1
        for (int i = 0; i < vertices; ++i) {
            const auto out_degree = out_degree_dist(gen);
            std::vector<int> targets(vertices);
            std::iota(targets.begin(), targets.end(), 0);
            std::shuffle(targets.begin(), targets.end(), gen);

            double remaining = 1.0;
            for (int k = 0; k < out_degree && remaining > 0.0; ++k) {
                const auto target = targets[k];
                const auto weight = weight_dist(gen);

                // assign a probability for this transition; use the provided base probability as a hint
                const double p = std::min(remaining, probability * prob_dist(gen));
                remaining -= p;

                ggg::stochastic_discounted::graph::add_edge(graph, vdesc[i], vdesc[target], std::string(""), weight, discount, p);
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
