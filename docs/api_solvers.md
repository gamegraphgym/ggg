# API Guide {#api_solvers}

## Adding New Solvers

### Implementing a Solver

To implement a new solver, inherit from the unified `Solver` interface with the appropriate [solution type](#solution-types) and implement the `solve()` method. For example, implement a new solver for parity games, that operates on `ggg::parity::graph::Graph` and produces solutions which encode winning regions only (`RSolution<ggg::parity::graph::Graph>`) as follows.

```cpp
#include "libggg/parity/graph.hpp"
#include "libggg/solvers/solver.hpp"

// Example solver providing winning regions and strategies
class YourSolver : public ggg::solvers::Solver<ggg::parity::graph::Graph, ggg::parity::RSolution> {
public:
    ggg::parity::RSolution solve(const ggg::parity::graph::Graph &game) override {
        ggg::parity::RSolution solution; // Default construct

        const auto [vertices_begin, vertices_end] = boost::vertices(game);

        for (auto it = vertices_begin; it != vertices_end; ++it) {
            const auto vertex = *it;
            const auto winner = computeWinner(game, vertex);
            solution.setWinningPlayer(vertex, winner);
        }

        return solution;
    }

    std::string get_name() const override {
        return "Your Custom Solver Name";
    }
};
```

You can use the project macro to expose this solver as a CLI, following the pattern used by the shipped tools:

```C++
// inside the game-specific tools/ or main.cpp
GGG_GAME_SOLVER_MAIN(ggg::parity::graph::Graph, ggg::parity::graph::parse, YourSolver)
```

See the implementations of existing solvers under `solvers/` as examples.

## Solution Types

We define different solution capabilities via C++ concepts and inheritance.
Solution types are named `XSolution`, where the prefix `X` indicates what is encoded:
initial state only (`X=I`), winning regions (`R`), a winning strategy (`S`), and quantitative (vertex) values (`Q`).
For instance, `RSSolution<graphs::ParityGraph>`C++ is a type of solution operating on `ParityGraph`s and which can compute a winning `R`egion as well as synthesize a `S`trategy. In more details,

### ISolution<GraphType>

The most basic solution type on `<GraphType>` graphs. It records only the initial winner label for convenience and provides serialization for solution payloads. There is no solved/valid flag in solutions; solvers either return a result or signal errors via logs/exceptions.

See also: [`include/libggg/solutions/isolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/isolution.hpp)

### RSolution<GraphType>

Contains info about winning **R**egions.

In addition to the inherited methods of `ISolution`, solutions of this kind implement:

- `is_won_by_player0(vertex)` - Check if vertex is won by player 0
- `is_won_by_player1(vertex)` - Check if vertex is won by player 1
- `get_winning_player(vertex)` - Get winning player (0, 1, or -1)
- `set_winning_player(vertex, player)` - Set winning player for vertex

See also: [`include/libggg/solutions/rsolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/rsolution.hpp)

### SSolution<GraphType>

Contains info about a winning **S**trategy

In addition to the inherited methods of `ISolution`, solutions of this kind implement:

- `get_strategy(vertex)` - Get strategic choice for vertex
- `has_strategy(vertex)` - Check if vertex has strategy defined
- `set_strategy(vertex, strategy)` - Set strategy for vertex

There is actually a second generic parameter specifying the type of strategy contained (mixing, deterministic, positional, finite-memory etc). The default is `DeterministicStrategy`, representing deterministic and positional strategies (i.e. mappings from vertices to vertices).

See also: [`include/libggg/solutions/ssolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/ssolution.hpp)

### QSolution<GraphType, ValueType>

Contains info about **Q**ualitative (vertex) values of type `ValueType`.

In addition to the inherited methods of `ISolution`, solutions of this kind implement:

- `get_value(vertex)` - Get numerical value for vertex
- `has_value(vertex)` - Check if vertex has value defined
- `set_value(vertex, value)` - Set value for vertex

See also: [`include/libggg/solutions/qsolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/qsolution.hpp)

### Combined Solution Types

- **`RSSolution<GraphType>`**: Regions + Strategies (inherits R and S methods)
- **`RQSolution<GraphType, ValueType>`**: Regions + Quantitative values (inherits R and Q methods)
- **`RSQSolution<GraphType, ValueType>`**: All capabilities (inherits R, S, and Q methods)

See also: [`include/libggg/solutions/rssolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/rssolution.hpp), [`include/libggg/solutions/rsqsolution.hpp`](https://github.com/gamegraphgym/ggg/blob/main/include/libggg/solutions/rsqsolution.hpp)

Choose the appropriate solution type based on what your solver provides.
