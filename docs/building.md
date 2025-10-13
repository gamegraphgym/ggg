# Building Game Graph Gym {#building}

This page provides detailed instructions for building Game Graph Gym with various configurations.
Here, "libggg" refers to the library, "ggg" is the short name for the project as well as the c++ namespace stuff is defined in.

## Requirements

### System Requirements and Dependencies

- **C++20 compatible compiler** (GCC 10+, Clang 10+, MSVC 2019+)
- **[CMake](https://cmake.org/) 3.15 or later**
- **[Boost Libraries](https://www.boost.org/) 1.70 or later**. Specifically, these components:

    - [`graph`](https://www.boost.org/doc/libs/release/libs/graph/) - Required for core graph data structures and algorithms
    - [`program_options`](https://www.boost.org/doc/libs/release/libs/program_options/) - Required for macros related to command-line tools
    - [`filesystem`](https://www.boost.org/doc/libs/release/libs/filesystem/) and [`system`](https://www.boost.org/doc/libs/release/libs/system/) - Optional, needed for command-line tools (build options `-DBUILD_TESTING=ON` or `-DTOOLS_ALL=ON`)
    - [`unit_test_framework`](https://www.boost.org/doc/libs/release/libs/test/) - Optional, needed for unit tests (build option `-DBUILD_TESTING=ON`)

### Platform-Specific Setup

**Ubuntu/Debian:**

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake \
    libboost-graph-dev libboost-program-options-dev \
    libboost-filesystem-dev libboost-system-dev libboost-test-dev
```

**Fedora/RHEL:**

```bash
sudo dnf install gcc-c++ cmake \
    boost-graph-devel boost-program-options-devel \
    boost-filesystem-devel boost-system-devel boost-test-devel
```

**macOS:**

```bash
brew install cmake boost
# Note: Homebrew's boost formula includes all components by default
```

**Windows (vcpkg):**

```bash
vcpkg install boost-graph boost-program-options boost-filesystem boost-system boost-test
```


## Building GGG

### Basic Build

```bash
git clone https://github.com/gamegraphgym/ggg.git
cd ggg
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Build Options

Game Graph Gym provides several CMake options to customize the build:

| Option | Default | Description |
|--------|---------|-------------|
| `TOOLS_ALL` | OFF | Build all tool families (equivalent to enabling all `TOOLS_*` options below) |
| `TOOLS_BUECHI` | OFF | Build executables for buechi game tools (generators, utilities) |
| `TOOLS_PARITY` | OFF | Build executables for parity game tools |
| `TOOLS_MEAN_PAYOFF` | OFF | Build executables for mean-payoff game tools |
| `TOOLS_STOCHASTIC_DISCOUNTED` | OFF | Build executables for discounted stochastic games |
| `BUILD_TESTING` | OFF | Build unit tests and enable CTest integration |
| `CMAKE_BUILD_TYPE` | None | Build configuration (Debug/Release/RelWithDebInfo/MinSizeRel) |

### Complete Build

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DTOOLS_ALL=ON \
    -DBUILD_TESTING=ON
    
cmake --build build -j$(nproc)
```

## Installation

You can optionally install the library system-wide:

```bash
cmake --install build
```

Or to a custom prefix:

```bash
cmake --install build --prefix /custom/path
```

You can also install specific components (when using the componentized setup):

```bash
# Development headers and CMake package files
cmake --install build --component dev --prefix /usr

# Shared solver libraries
cmake --install build --component libs --prefix /usr

# CLI executables (generators and solver CLIs)
cmake --install build --component bin --prefix /usr
```

This will install headers and CMake package files under standard locations:

- Headers under `${CMAKE_INSTALL_INCLUDEDIR}` (e.g., `/usr/include/libggg/...`)
- CMake package config under `${CMAKE_INSTALL_LIBDIR}/cmake/GameGraphGym`

The solver libraries (shared) and CLI executables are also installed when those tool families are enabled:

- Shared solver libraries under `${CMAKE_INSTALL_LIBDIR}` (component: `libs`)
- Executables under `${CMAKE_INSTALL_BINDIR}` (component: `bin`)

## Using Game Graph Gym in Your Project

### CMake Integration

Add Game Graph Gym to your CMake project:

```cmake
find_package(GameGraphGym REQUIRED)
target_link_libraries(your_target PRIVATE GameGraphGym::ggg)
```

### Manual Integration

If building without installation:

```cmake
# Find Boost library. Libggg requires graph and program_options components 
# (also add `system`, `filesystem` for tools, and `unit_test_framework` for tests)
find_package(Boost REQUIRED COMPONENTS graph program_options)

# Add Game Graph Gym
add_subdirectory(path/to/libggg)
target_link_libraries(your_target PRIVATE ggg Boost::graph Boost::program_options)
target_compile_features(your_target PRIVATE cxx_std_20)
```

## Troubleshooting

### Common Issues

**Boost not found:**

```bash
CMAKE_ERROR: Could not find Boost
```

Solution: Install Boost development packages or set `BOOST_ROOT`

**C++20 not supported:**

```bash
error: 'auto' not allowed in non-static class member
```

Solution: Use a newer compiler or set `CMAKE_CXX_STANDARD=20`

**Link errors with Boost:**

```bash
undefined reference to boost::graph
```

Solution: Ensure CMake policy is set: `cmake_policy(SET CMP0167 NEW)`

### Packaging with CPack (DEB)

This project supports component-based packaging using CPack. Components:

- `dev`: headers and CMake package configuration
- `libs`: shared solver libraries
- `bin`: command-line executables (generators and solver CLIs)

Build and create .deb packages (example for Release):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTOOLS_ALL=ON
cmake --build build -j$(nproc)
cmake --install build --prefix /usr  # optional install step for local testing
cd build && cpack -C Release -G DEB
```

This produces separate packages like `gamegraphgym-dev`, `gamegraphgym-libs`, and `gamegraphgym-bin`.

### Performance Builds

For maximum performance:

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG -march=native"
```

### Debug Builds

For debugging with full symbols:

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-g -O0"
```
