#pragma once

#include <boost/program_options.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

namespace ggg {
namespace utils {

namespace po = boost::program_options;

class GameGraphGenerator {
  public:
    explicit GameGraphGenerator(const std::string &desc_name) : desc_(desc_name) {
        // common options used by all generators
        desc_.add_options()("help,h", "Show help message");
        desc_.add_options()("output-dir,o", po::value<std::string>()->default_value("./generated"), "Output directory");
        desc_.add_options()("seed", po::value<unsigned int>(), "Random seed (default: random)");
        desc_.add_options()("verbose", po::bool_switch()->default_value(false), "Verbose output");
        desc_.add_options()("vertices,v", po::value<int>()->default_value(10), "Number of vertices per game");
        desc_.add_options()("count,c", po::value<int>()->default_value(1), "Number of games to generate");
    }

    int run(int argc, char *argv[]) {
        po::variables_map vm;
        try {
            const auto args = normalize_aliases(argc, argv);
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
            std::cout << desc_ << std::endl;
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
    static std::vector<std::string> normalize_aliases(int argc, char *argv[]) {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            std::string arg(argv[i]);
            if (arg == "-pv" || arg == "--pv") {
                args.emplace_back("--player-vertices");
            } else if (arg == "-mo" || arg == "--mo") {
                args.emplace_back("--min-out-degree");
            } else if (arg == "-mxo" || arg == "--mxo") {
                args.emplace_back("--max-out-degree");
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
    po::options_description desc_;

    virtual bool validate_parameters(const po::variables_map &vm) = 0;
    virtual void print_generation_info(const po::variables_map &vm, const std::string &output_dir, int count, unsigned int seed) = 0;
    virtual std::string get_filename_prefix() const = 0;
    virtual void generate_single_game(const po::variables_map &vm, std::mt19937 &gen, std::ofstream &file) = 0;
};

} // namespace utils
} // namespace ggg