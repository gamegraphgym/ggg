cmake_minimum_required(VERSION 3.15)
project(libggg VERSION 1.0.0 LANGUAGES CXX)

# Set CMake policy for Boost finding (if supported)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Logging configuration options
option(ENABLE_LOGGING "Enable logging system" ON)

# Handle LOG_LEVEL with proper defaults based on build type
if(NOT DEFINED LOG_LEVEL)
    # Set default log levels based on build type when not explicitly set
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(LOG_LEVEL "5" CACHE STRING "Default logging level for Debug builds")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(LOG_LEVEL "4" CACHE STRING "Default logging level for Release builds")
    else()
        set(LOG_LEVEL "4" CACHE STRING "Default logging level: 0=NONE, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=TRACE")
    endif()
else()
    # User explicitly set LOG_LEVEL, keep their value
    set(LOG_LEVEL "${LOG_LEVEL}" CACHE STRING "User-specified logging level" FORCE)
endif()

# Use GNUInstallDirs for multi-arch friendly install locations
include(GNUInstallDirs)

# Find required packages (prefer Config if available, fall back to FindBoost)
# The core library only needs Boost.Graph; CLI tools link program_options themselves.
find_package(Boost QUIET CONFIG COMPONENTS graph)
if(NOT Boost_FOUND)
    find_package(Boost REQUIRED COMPONENTS graph)
endif()

# Set up directories
# Set up directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Header files
file(GLOB_RECURSE HEADERS 
    "${INCLUDE_DIR}/*.hpp"
    "${INCLUDE_DIR}/*.h"
)

# Create the library target (header-only interface)
add_library(ggg INTERFACE)
# Provide a Boost-style namespaced alias for consumers and export
add_library(GameGraphGym::ggg ALIAS ggg)

# Set include directories
target_include_directories(ggg 
    INTERFACE
        $<BUILD_INTERFACE:${INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link Boost libraries needed by the interface
target_link_libraries(ggg INTERFACE Boost::graph)

# Configure logging preprocessor definitions
if(ENABLE_LOGGING)
    target_compile_definitions(ggg INTERFACE ENABLE_LOGGING)
    target_compile_definitions(ggg INTERFACE LOG_LEVEL=${LOG_LEVEL})
    message(STATUS "Logging enabled with level ${LOG_LEVEL}")
else()
    message(STATUS "Logging disabled")
endif()

# Compiler-specific options
target_compile_features(ggg INTERFACE cxx_std_20)


# Build options
option(BUILD_TESTING "Build unit tests" OFF)
# New global switch to enable all tool families at once. When ON it will
# turn on the per-family TOOLS_* options below so the corresponding
# subdirectories are configured/built.
option(TOOLS_ALL "Enable building all CLI tool families (parity, stochastic_discounted, mean_payoff, buchi)" OFF)


# Enable testing early if requested
if(BUILD_TESTING)
    enable_testing()
endif()

# Add main tests (after solvers so targets exist)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Optional: Build tools
# If TOOLS_ALL is set, enable the per-family options and add the top-level
# tools directory which contains shared tool helpers.
if(TOOLS_ALL)
    message(STATUS "TOOLS_ALL=ON: enabling all tool families")
    # Enable individual tool families by default when TOOLS_ALL is true
    set(TOOLS_PARITY ON CACHE BOOL "Build parity CLI tools from tools/parity" FORCE)
    set(TOOLS_STOCHASTIC_DISCOUNTED ON CACHE BOOL "Build stochastic discounted CLI tools from tools/stochastic_discounted" FORCE)
    set(TOOLS_MEAN_PAYOFF ON CACHE BOOL "Build mean-payoff CLI tools from tools/mean_payoff" FORCE)
    set(TOOLS_BUECHI ON CACHE BOOL "Build Büchi CLI tools from tools/buchi" FORCE)
endif()


# Tools: Parity-specific CLIs
option(TOOLS_PARITY "Build parity CLI tools from tools/parity" OFF)
if(TOOLS_PARITY)
    message(STATUS "Building parity CLI tools (TOOLS_PARITY=ON)")
    add_subdirectory(tools/parity)
endif()

# Tools: Stochastic Discounted-specific CLIs
option(TOOLS_STOCHASTIC_DISCOUNTED "Build stochastic discounted CLI tools from tools/stochastic_discounted" OFF)
if(TOOLS_STOCHASTIC_DISCOUNTED)
    message(STATUS "Building stochastic discounted CLI tools (TOOLS_STOCHASTIC_DISCOUNTED=ON)")
    add_subdirectory(tools/stochastic_discounted)
endif()

# Tools: Mean-Payoff-specific CLIs
option(TOOLS_MEAN_PAYOFF "Build mean-payoff CLI tools from tools/mean_payoff" OFF)
if(TOOLS_MEAN_PAYOFF)
    message(STATUS "Building mean-payoff CLI tools (TOOLS_MEAN_PAYOFF=ON)")
    add_subdirectory(tools/mean_payoff)
endif()

# Tools: Büchi-specific CLIs
option(TOOLS_BUECHI "Build Büchi CLI tools from tools/buchi" OFF)
if(TOOLS_BUECHI)
    message(STATUS "Building Büchi CLI tools (TOOLS_BUECHI=ON)")
    add_subdirectory(tools/buchi)
endif()

# Installation
install(TARGETS ggg
    EXPORT GameGraphGymTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT dev
)

install(DIRECTORY ${INCLUDE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT dev
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

install(EXPORT GameGraphGymTargets
    FILE GameGraphGymTargets.cmake
    NAMESPACE GameGraphGym::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GameGraphGym
    COMPONENT dev
)

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    GameGraphGymConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GameGraphGymConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/GameGraphGymConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GameGraphGym
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/GameGraphGymConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/GameGraphGymConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GameGraphGym
    COMPONENT dev
)

# Windows: export all symbols for SHARED libs in tools if no explicit export is provided
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# CPack componentized packaging (Option A)
set(CPACK_GENERATOR "DEB")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL dev libs bin)
set(CPACK_COMPONENT_DEV_DESCRIPTION "Development headers and CMake package configuration for Game Graph Gym")
set(CPACK_COMPONENT_LIBS_DESCRIPTION "Shared solver libraries for Game Graph Gym")
set(CPACK_COMPONENT_BIN_DESCRIPTION "Command-line tools (generators and solver CLIs) for Game Graph Gym")

# Friendly Debian package names per component
set(CPACK_DEBIAN_DEV_PACKAGE_NAME "gamegraphgym-dev")
set(CPACK_DEBIAN_LIBS_PACKAGE_NAME "gamegraphgym-libs")
set(CPACK_DEBIAN_BIN_PACKAGE_NAME "gamegraphgym-bin")

include(CPack)
