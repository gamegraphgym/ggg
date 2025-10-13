cmake_minimum_required(VERSION 3.15)

# Parity CLI tools and generator build
# All solver libraries and executables are built as SHARED (dynamic linking)
# Requires: target 'ggg' from top-level build

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

if(NOT TARGET ggg)
    message(FATAL_ERROR "tools/parity requires target 'ggg' from the top-level build")
endif()

include(GNUInstallDirs)
find_package(Boost QUIET CONFIG REQUIRED COMPONENTS program_options filesystem)
if(NOT Boost_FOUND)
    find_package(Boost REQUIRED COMPONENTS program_options filesystem)
endif()


# Helper to define a parity solver CLI and its implementation as SHARED
function(ggg_add_parity_solver_cli solver_short main_src impl_src)
    # solver_short is the short name (e.g. priority_promotion, recursive)
    set(exe_name "ggg_parity_solver_${solver_short}")
    set(lib_name "ggg_parity_${solver_short}_solver")

    add_library(${lib_name} SHARED ${impl_src})
    target_link_libraries(${lib_name} PUBLIC ggg)
    target_include_directories(${lib_name} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    # Provide SONAME and VERSION for packaging
    set_target_properties(${lib_name} PROPERTIES VERSION 1.0.0 SOVERSION 1)

    add_executable(${exe_name} ${main_src})
    target_link_libraries(${exe_name} PRIVATE ${lib_name} Boost::program_options)
    target_link_libraries(${exe_name} PUBLIC ggg)
    target_include_directories(${exe_name} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    set_target_properties(${exe_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    # Install shared lib and CLI into standard locations with components
    install(TARGETS ${lib_name}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT libs)
    install(TARGETS ${exe_name}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT bin)
endfunction()

# Parity solver CLIs
ggg_add_parity_solver_cli(priority_promotion solvers/priority_promotion.cpp ${CMAKE_SOURCE_DIR}/src/libggg/parity/solvers/priority_promotion.cpp)
ggg_add_parity_solver_cli(progressive_small_progress_measures solvers/progressive_small_progress_measures.cpp ${CMAKE_SOURCE_DIR}/src/libggg/parity/solvers/progressive_small_progress_measures.cpp)
ggg_add_parity_solver_cli(recursive solvers/recursive.cpp ${CMAKE_SOURCE_DIR}/src/libggg/parity/solvers/recursive.cpp)

# Parity generator CLI (generate_parity.cpp)
add_executable(ggg_parity_generate ${CMAKE_CURRENT_SOURCE_DIR}/generate_parity.cpp)
target_link_libraries(ggg_parity_generate PUBLIC ggg)
target_link_libraries(ggg_parity_generate PRIVATE Boost::program_options Boost::filesystem)
target_include_directories(ggg_parity_generate PRIVATE ${CMAKE_SOURCE_DIR}/include)
set_target_properties(ggg_parity_generate PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Install generator CLI
install(TARGETS ggg_parity_generate
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT bin)
