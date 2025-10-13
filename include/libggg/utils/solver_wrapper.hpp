#pragma once

#include "libggg/solutions/concepts.hpp"
#include "libggg/solvers/solver.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <concepts>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ggg {
namespace utils {

// C++20 concept to ensure solver has solve method
template <typename SolverType, typename GraphType>
concept HasSolveMethod = requires(SolverType solver, const GraphType &graph) {
    solver.solve(graph);
};

// C++20 concept to detect solutions with generic statistics
template <typename SolutionType>
concept HasStatistics = requires(const SolutionType &solution) {
    { solution.get_statistics() } -> std::convertible_to<std::map<std::string, std::string>>;
};

/**
 * @brief Generic wrapper for game solvers
 * @template GraphType The graph type (ParityGraph, MeanPayoffGraph)
 * @template SolverType The solver class for the graph type
 */
template <typename GraphType, typename SolverType>
class GameSolverWrapper {
  private:
    struct ParseResult {
        boost::program_options::variables_map vm;
        std::string input; // first positional token; "-" means stdin
    };
    /**
     * @brief Parse command line options
     */
    static ParseResult parse_command_line(int argc, char *argv[]) {
        boost::program_options::options_description desc("Solver Options");
        desc.add_options()("help,h", "Show help message");
        // Output format flag: plain (default) or json
        desc.add_options()("format,f", boost::program_options::value<std::string>()->default_value("plain"), "Output format: plain | json (default: plain)");
        desc.add_options()("time-only,t", "Only output time to solve (in milliseconds)");
        desc.add_options()("solver-name", "Output solver name");
        // Do not declare a named --input; we'll treat the first non-option token as input.

#ifdef ENABLE_LOGGING
#if LOG_LEVEL == 3
        desc.add_options()("verbose,v", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens(),
                           "Increase verbosity");
#elif LOG_LEVEL == 5
        desc.add_options()("verbose,v", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens(),
                           "Increase verbosity (can be used multiple times: -v, -vv, or -vvv)");
#endif
#endif

        boost::program_options::variables_map vm;

        // Parse manually to handle multiple -v flags and -vv, -vvv style flags.
        // Only interpret -v when logging is enabled; otherwise treat all args as positional.
#ifdef ENABLE_LOGGING
        std::vector<std::string> args;
        int verbosity = 0;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-v" || arg == "--verbose") {
                verbosity++;
            } else if (arg.size() > 2 && arg.rfind("-v", 0) == 0) {
                // Count multiple v's in -vvv style
                verbosity = static_cast<int>(arg.size()) - 1;
            } else {
                args.push_back(arg);
            }
        }
#else
        // Logging disabled: preserve all args
        std::vector<std::string> args;
        int verbosity = 0;
        for (int i = 1; i < argc; ++i)
            args.push_back(argv[i]);
#endif

        // Create modified argv (args contains either filtered args or all args)
        std::vector<char *> new_argv;
        new_argv.push_back(argv[0]);
        for (const auto &arg : args) {
            new_argv.push_back(const_cast<char *>(arg.c_str()));
        }

        // Parse known options; allow unrecognized to collect the first positional token as input
        auto parsed = boost::program_options::command_line_parser(static_cast<int>(new_argv.size()), new_argv.data())
                          .options(desc)
                          .allow_unregistered()
                          .run();

        boost::program_options::store(parsed, vm);
        boost::program_options::notify(vm);

        // Collect positional tokens (unrecognized by Boost) and choose the first as input
        auto unrecognized = boost::program_options::collect_unrecognized(parsed.options, boost::program_options::include_positional);

        // If help requested, print a synopsis that includes the positional input parameter
        if (vm.count("help")) {
            // Usage synopsis: program [options] <input>
            std::cout << "Usage: " << argv[0] << " [options] <input>\n\n";
            std::cout << desc << std::endl;
            exit(0);
        }

        // Determine input file: first positional token if provided; default to stdin ("-")
        std::string input_file = "-";
        if (!unrecognized.empty()) {
            input_file = unrecognized.front();
        }

#ifdef ENABLE_LOGGING
        if (verbosity > 0) {
            auto log_level = verbosity_to_log_level(verbosity);
            ggg::utils::set_log_level(log_level);
            LGG_INFO("Logging level set to ", static_cast<int>(log_level));
        }
#endif

        return ParseResult{std::move(vm), std::move(input_file)};
    }

    /**
     * @brief Output solution in human-readable format
     * TODO:instead of printing to stdout, make this return a string representation of the solution
     * (and put this function somewhere more apropriate)
     */
    template <typename SolutionType>
    static void output_human(const GraphType &graph,
                             const SolutionType &solution,
                             double time_to_solve)
        requires solutions::HasRegions<SolutionType, GraphType> && solutions::HasStrategy<SolutionType, GraphType>
    {
        using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;

        std::cout << "Time to solve: " << time_to_solve << "ms" << std::endl;

        auto vertices = boost::vertices(graph);
        for (auto it = vertices.first; it != vertices.second; ++it) {
            Vertex vertex = *it;
            std::string vertex_name = graph[vertex].name;

            std::cout << "  " << vertex_name << ": ";
            if (solution.is_won_by_player0(vertex)) {
                std::cout << "Player 0";
            } else if (solution.is_won_by_player1(vertex)) {
                std::cout << "Player 1";
            } else {
                std::cout << "Unknown";
            }

            auto strategy_vertex = solution.get_strategy(vertex);
            if (strategy_vertex != boost::graph_traits<GraphType>::null_vertex()) {
                std::cout << " -> " << graph[strategy_vertex].name;
            }

            // Print quantitative values if available
            if constexpr (solutions::HasValueMapping<SolutionType, GraphType>) {
                if (solution.has_value(vertex)) {
                    std::cout << " (value: " << solution.get_value(vertex) << ")";
                }
            }

            std::cout << std::endl;
        }

        // Print statistics if available
        if constexpr (HasStatistics<SolutionType>) {
            auto stats = solution.get_statistics();
            if (!stats.empty()) {
                std::cout << "Statistics:" << std::endl;
                for (const auto &[key, value] : stats) {
                    std::cout << "  " << key << ": " << value << std::endl;
                }
            }
        }
    }

  public:
    template <typename ParserFunc>
    static int run(int argc, char *argv[], ParserFunc parser_func) {
        try {
            auto parsed = parse_command_line(argc, argv);
            auto &vm = parsed.vm;

            LGG_DEBUG("Starting GameSolverWrapper");

            if (vm.count("solver-name")) {
                SolverType solver;
                std::cout << solver.get_name() << std::endl;
                return 0;
            }

            // Parse input game
            std::string input_file = parsed.input;
            std::string output_format = vm["format"].template as<std::string>();
            std::shared_ptr<GraphType> graph;

            LGG_INFO("Parsing input from: ", (input_file == "-" ? "stdin" : input_file));

            if (input_file == "-") {
                graph = parser_func(std::cin);
            } else {
                graph = parser_func(input_file);
            }

            if (!graph) {
                LGG_ERROR("Failed to parse input game");
                std::cerr << "Error: Failed to parse input game" << std::endl;
                return 1;
            }

            LGG_INFO("Successfully parsed game with ", boost::num_vertices(*graph), " vertices");

            // Create solver and measure time
            SolverType solver;
            LGG_DEBUG("Starting solver: ", solver.get_name());

            static_assert(HasSolveMethod<SolverType, GraphType>,
                          "Solver must have solve() method");

            auto start = std::chrono::high_resolution_clock::now();
            auto solution = solver.solve(*graph);
            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            double time_to_solve = duration.count() / 1000.0;

            LGG_DEBUG("Solver completed in ", time_to_solve, " milliseconds");

            LGG_INFO("Solver completed; emitting results");

            // Output results
            if (vm.count("time-only")) {
                std::cout << "Time to solve: " << time_to_solve << " ms" << std::endl;
            } else {
                if (output_format == "json") {
                    // One struct with time and solution JSON
                    std::cout << "{\"time\": " << time_to_solve << ", \"solution\": " << solution.to_json() << "}" << std::endl;
                } else {
                    // plain: use operator<< on solution
                    std::cout << "Game solved in " << time_to_solve << " ms." << std::endl;
                    std::cout << "Solution:\n"
                              << solution << std::endl;
                }
            }

            return 0;

        } catch (const std::exception &e) {
            LGG_ERROR("Exception caught: ", e.what());
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }
};

/**
 * @brief Macro to create main functions for game solvers
 * @param GraphType The game graph type (e.g., graphs::ParityGraph)
 * @param ParserFunc The parser function name (e.g., parse_Parity_graph)
 * @param SolverType The solver class
 */
#define GGG_GAME_SOLVER_MAIN(GraphType, ParserFunc, SolverType)                                    \
    int main(int argc, char *argv[]) {                                                             \
        auto parser_func = [](auto &&input) { return ParserFunc(input); };                         \
        return ggg::utils::GameSolverWrapper<GraphType, SolverType>::run(argc, argv, parser_func); \
    }

} // namespace utils
} // namespace ggg
