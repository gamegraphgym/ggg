# Development Guide {#developing}

This guide covers setting up a development environment and contributing to Game Graph Gym.

## Project Structure

The repository is organized as follows:

```text
.
├── docs/         # Source files for the documentation (built with MkDocs)
├── cmake/        # CMake configuration modules
├── include/      # Public C++ headers for the library
├── src/          # Implementations
├── tests/        # Unit and integration tests
└── tools/        # CLI tools (e.g., game generators, benchmarks)
```

## Development Setup

Set up development build as follows.

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DTOOLS_ALL=ON \
    -DBUILD_TESTING=ON

cmake --build build
```

!!! note "Language Server Support"

    To use [clangd](https://clangd.llvm.org/) for C++ language server support, generate a `compile_commands.json` file:

    ```bash
    cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    ln -s build/compile_commands.json .
    ```

## Code Style Guidelines

### Use Boost Graph Library features where possible

For example, use iterators provided by Boost Graph functions when iterating over vertices or edges, as follows.

```cpp
// iterate over all vertices
const auto [vertices_begin, vertices_end] = boost::vertices(graph);
for (const auto& vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
    // Process vertex
}
```

Access properties directly:

```cpp
// if graph is a ParityGraph and vertex one of its vertices
const auto player = graph[vertex].player;
const auto priority = graph[vertex].priority;
```

### Code Formatting

To maintain a consistent code format across the project,
format your code using `clang-format`, according to the style set up in `/.clang-format`.

- In VS Code with the official C/C++ extension, just use <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>I</kbd> to reformat;

- To format all files within the (project) directory, use

    ```bash
    find . -name '*.hpp' -o -name '*.cpp' | xargs clang-format -i
    ```

- Consider setting up a [git hook](https://www.atlassian.com/git/tutorials/git-hooks) `.git/hooks/pre-commit` to automatically format code before committing:

    ```bash
    C_CHANGED_FILES = $(git diff --cached --name-only --diff-filter=ACM | grep -Ee "\.[ch]$")
    CXX_CHANGED_FILES = $(git diff --cached --name-only --diff-filter=ACM | grep -Ee "\.([chi](pp|xx)|(cc|hh|ii)|[CHI])$")
    /usr/bin/clang-format -i -style=file ${CXX_CHANGED_FILES}
    ```

## Testing

Tests are disabled by default. To build and run tests, enable them during configuration:

### Running Tests

```bash
# Configure with tests enabled
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build

# Run all tests
cd build && ctest --output-on-failure

# Alternative test runner
cd build && make test

# Run specific test
ctest -R test_name

# Run with verbose output
ctest --verbose
```

### Writing Tests

Tests use a simple assertion framework. Place test files in `tests/`:

```cpp
// test/test_new_feature.cpp
#include "test_common.hpp"
#include <libggg/your_header.hpp>

void test_basic_functionality() {
    // Create test data
    ParityGame game;
    const auto vertex = boost::add_vertex({0, "test", 0}, game);

    // Test functionality
    ASSERT_EQ(boost::num_vertices(game), 1);
    ASSERT_EQ(game[vertex].name, "test");
}

int main() {
    test_basic_functionality();
    return 0;
}
```

### Test Coverage

- **Cover normal cases** and edge cases
- **Test error conditions** and input validation
- **Use descriptive test names** explaining what is tested
- **Keep tests independent** and deterministic

## Contributing

We welcome contributions! Please follow these guidelines:

### Pull Request Process

1. **Fork the repository** and create your feature branch from main
2. **Create feature branch** from main: `git checkout -b feature/new-solver`
3. **Implement changes** following code style guidelines
4. **Add/update tests** for new functionality  
5. **Update documentation** if needed
6. **Ensure all tests pass**: `ctest --output-on-failure`
7. **Submit pull request** with clear description


### Git Workflow

```bash
# Fork and clone your fork
git clone https://github.com/your-username/ggg.git
cd ggg

# Create feature branch
git checkout -b feature/new-solver

# Make changes and commit
git add .
git commit -m "Add new solver implementation"

# Push and create PR
git push origin feature/new-solver
```


## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](https://github.com/gamegraphgym/ggg/blob/main/LICENSE) file for details.
