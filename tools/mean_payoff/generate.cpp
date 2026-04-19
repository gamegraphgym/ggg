#include "libggg/mean_payoff/graph.hpp"
#include "libggg/utils/game_graph_generator.hpp"
#include <filesystem>
#include <random>
#include <string>

namespace po = boost::program_options;

class MPVGameGenerator : public ggg::utils::GameGraphGenerator {
  public:
    MPVGameGenerator() : GameGraphGenerator("Mean-Payoff Generator Options") {
        desc_.add_options()("min-weight,mw", po::value<int>()->default_value(-10), "Minimum edge weight");
        desc_.add_options()("max-weight,mxw", po::value<int>()->default_value(10), "Maximum edge weight");
        desc_.add_options()("min-out-degree,mo", po::value<int>()->default_value(1), "Minimum out-degree per vertex");
        desc_.add_options()("max-out-degree,mxo", po::value<int>()->default_value(-1), "Maximum out-degree per vertex (-1 means vertices-1)");
    }

    static std::vector<std::string> normalize_mpv_aliases(int argc, char *argv[]) {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            std::string arg(argv[i]);
            if (arg == "-mo" || arg == "--mo") {
                args.emplace_back("--min-out-degree");
            } else if (arg == "-mxo" || arg == "--mxo") {
                args.emplace_back("--max-out-degree");
            } else if (arg == "-mw" || arg == "--mw") {
                args.emplace_back("--min-weight");
            } else if (arg == "-mxw" || arg == "--mxw") {
                args.emplace_back("--max-weight");
            } else if (arg == "-vx" || arg == "--vx") {
                args.emplace_back("--vertices");
            } else {
                args.emplace_back(std::move(arg));
            }
        }
        return args;
    }

    void print_help() const {
        std::cout << "Mean-Payoff Generator Options:\n"
                  << "  -h [ --help ]                       Show help message\n"
                  << "  -o [ --output-dir ] arg (=./generated)\n"
                  << "                                      Output directory\n"
                  << "  --seed arg                          Random seed (default: random)\n"
                  << "  --verbose                           Verbose output\n"
                  << "  -vx [ --vertices ] arg (=10)        Number of vertices per game\n"
                  << "  -c [ --count ] arg (=1)             Number of games to generate\n"
                  << "  --min-weight, -mw arg (=-10)        Minimum edge weight\n"
                  << "  --max-weight, -mxw arg (=10)        Maximum edge weight\n"
                  << "  --min-out-degree, -mo arg (=1)     Minimum out-degree per vertex\n"
                  << "  --max-out-degree, -mxo arg (=-1)    Maximum out-degree per vertex (-1 means vertices-1)\n";
    }

    int run(int argc, char *argv[]) {
        po::variables_map vm;
        try {
            const auto args = normalize_mpv_aliases(argc, argv);
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

        if (!validate_parameters(vm))
            return 1;

        std::filesystem::create_directories(output_dir);

        std::mt19937 gen(seed);

        print_generation_info(vm, output_dir, count, seed);

        for (int i = 0; i < count; ++i) {
            const auto filename = std::filesystem::path(output_dir) / (get_filename_prefix() + std::to_string(i) + ".dot");
            std::ofstream ofs(filename);
            if (!ofs) {
                std::cerr << "Failed to open output file: " << filename << std::endl;
                return 1;
            }

            generate_single_game(vm, gen, ofs);

            if (verbose)
                std::cout << "Wrote: " << filename << std::endl;
        }

        return 0;
    }

  protected:
    bool validate_parameters(const po::variables_map &vm) override {
        const auto vertices = vm["vertices"].as<int>();
        if (vertices <= 0) {
            std::cerr << "Error: vertices must be positive" << std::endl;
            return false;
        }
        const auto min_out_degree = vm["min-out-degree"].as<int>();
        const auto max_out_degree = vm["max-out-degree"].as<int>();
        if (min_out_degree < 1) {
            std::cerr << "Error: min-out-degree must be >= 1" << std::endl;
            return false;
        }
        if (max_out_degree != -1 && max_out_degree < min_out_degree) {
            std::cerr << "Error: max-out-degree must be >= min-out-degree" << std::endl;
            return false;
        }
        if (max_out_degree != -1 && max_out_degree > vertices - 1) {
            std::cerr << "Error: max-out-degree must be <= vertices-1" << std::endl;
            return false;
        }
        return true;
    }

    void print_generation_info(const po::variables_map &vm, const std::string &output_dir, int count, unsigned int seed) override {
        std::cout << "Generating " << count << " mean-payoff games" << std::endl;
        std::cout << "Output directory: " << output_dir << std::endl;
        const auto min_out_degree = vm["min-out-degree"].as<int>();
        const auto max_out_degree = vm["max-out-degree"].as<int>();
        if (max_out_degree == -1) {
            std::cout << "Out-degree range per vertex: [" << min_out_degree << ", vertices-1]" << std::endl;
        } else {
            std::cout << "Out-degree range per vertex: [" << min_out_degree << ", " << max_out_degree << "]" << std::endl;
        }
    }

    std::string get_filename_prefix() const override { return "mpv_game_"; }

    void generate_single_game(const po::variables_map &vm, std::mt19937 &gen, std::ofstream &file) override {
        const auto vertices = vm["vertices"].as<int>();
        const auto min_weight = vm["min-weight"].as<int>();
        const auto max_weight = vm["max-weight"].as<int>();
        const auto min_out_degree = vm["min-out-degree"].as<int>();
        auto max_out_degree = vm["max-out-degree"].as<int>();
        if (max_out_degree < 0) {
            max_out_degree = vertices - 1;
        }

        auto graph = generate_mpv_game(vertices, min_weight, max_weight, min_out_degree, max_out_degree, gen);
        write_dot(graph, file);
    }

    void write_dot(const ggg::mean_payoff::graph::Graph &g, std::ostream &os) {
        os << "digraph G {\n";
        // Vertices
        for (auto v : boost::make_iterator_range(boost::vertices(g))) {
            os << "v" << v << " [name=\"" << g[v].name << "\", player=" << g[v].player << ", weight=" << g[v].weight << "];\n";
        }
        // Edges
        for (auto e : boost::make_iterator_range(boost::edges(g))) {
            auto source = boost::source(e, g);
            auto target = boost::target(e, g);
            os << "v" << source << "->v" << target << ";\n";
        }
        os << "}\n";
    }

  private:
    static ggg::mean_payoff::graph::Graph generate_mpv_game(int vertices,
                                                            int min_weight,
                                                            int max_weight,
                                                            int min_out_degree,
                                                            int max_out_degree,
                                                            std::mt19937 &gen) {
        std::uniform_int_distribution<int> player_dist(0, 1);
        std::uniform_int_distribution<int> weight_dist(min_weight, max_weight);
        std::uniform_int_distribution<int> out_degree_dist(min_out_degree, max_out_degree);
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
