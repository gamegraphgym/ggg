# Usage Guide {#quick_start}

## Prerequisites

Ensure you have the required dependencies installed:
The following lists packages as in Ubuntu/Debian. For other platforms (macOS, Fedora/RHEL, Windows), see the [detailed build instructions](building.md#platform-specific-setup).

```bash
sudo apt-get install -y build-essential cmake \
    libboost-graph-dev libboost-program-options-dev \
    libboost-filesystem-dev libboost-system-dev libboost-test-dev
```

## Building Tools and Solvers

```bash
# Clone the repository
git clone https://github.com/gamegraphgym/ggg.git
cd ggg

# Configure build with all components
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DTOOLS_ALL=ON

# Build the project
cmake --build build -j$(nproc)
```

## Running Your First Example

After building, try running the basic usage example. All binaries are placed in `build/bin/`.

```bash
# Generate some test games (generator executables live in build/bin)
# Example: parity generator
./build/bin/ggg_parity_generate -o test_games -n 5 -v 20

# Run a solver on the generated games (solver CLIs are in build/bin/)
./build/bin/ggg_parity_solver_recursive test_games/game_0.dot

# As above but with debug log on stderr (and moved to log.txt)
./build/bin/ggg_parity_solver_recursive test_games/game_0.dot -vv 2&> log.txt
```

## Using the API in Your Code

Here's a minimal example of using Game Graph Gym in your own project:

```cpp
#include <libggg/libggg.hpp>
#include "libggg/parity/solvers/recursive.hpp"  // get hold of some solver

int main() {
    // Load a Parity Graph from DOT file
    auto game = ggg::graphs::parse_parity_graph_from_file("game.dot");
    
    // Solve using a solver that provides regions and strategies (RS solution)
    auto solver = std::make_unique<ggg::parity::RecursiveParitySolver>();
    auto solution = solver->solve(game);

    // Inspect results (solved-state flag removed)
    std::cout << "Vertex v0 won by player: "
              << solution.get_winning_player(v0) << std::endl;
}
```

## Next Steps

- Read the [detailed build instructions](building.md)
