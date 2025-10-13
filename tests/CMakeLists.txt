cmake_minimum_required(VERSION 3.15)

# Set CMake policy for Boost finding
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find required packages for testing
find_package(Boost REQUIRED COMPONENTS unit_test_framework CONFIG)

# Test executable
add_executable(test_ggg
    libggg/graphs/test_parity_graph.cpp
    libggg/graphs/test_mean_payoff_graph.cpp
    libggg/graphs/test_priority_utilities.cpp
    libggg/graphs/test_player_utilities.cpp
    libggg/graphs/test_stochastic_discounted_graph.cpp
    main.cpp
)

# Link libraries
target_link_libraries(test_ggg 
    PRIVATE 
        ggg
        Boost::unit_test_framework
)

# Include directories
target_include_directories(test_ggg 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
)

# Add test
add_test(NAME ggg_tests COMMAND test_ggg)
