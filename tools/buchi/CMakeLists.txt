cmake_minimum_required(VERSION 3.15)

# Büchi CLI tools build
# All solver libraries and executables are built as SHARED (dynamic linking)
# Requires: target 'ggg' from top-level build

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

if(NOT TARGET ggg)
    message(FATAL_ERROR "tools/buchi requires target 'ggg' from the top-level build")
endif()

include(GNUInstallDirs)
find_package(Boost QUIET CONFIG REQUIRED COMPONENTS program_options filesystem)
if(NOT Boost_FOUND)
    find_package(Boost REQUIRED COMPONENTS program_options filesystem)
endif()

# Helper to define a buechi solver CLI and its implementation as SHARED
function(ggg_add_buechi_solver_cli solver_short main_src impl_src)
    # solver_short is the short name (e.g. attractor)
    set(exe_name "ggg_buechi_solver_${solver_short}")
    set(lib_name "ggg_buechi_${solver_short}_solver")

    add_library(${lib_name} SHARED ${impl_src})
    target_link_libraries(${lib_name} PUBLIC ggg)
    target_include_directories(${lib_name} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    set_target_properties(${lib_name} PROPERTIES VERSION 1.0.0 SOVERSION 1)

    add_executable(${exe_name} ${main_src})
    target_link_libraries(${exe_name} PRIVATE ${lib_name} Boost::program_options)
    target_link_libraries(${exe_name} PUBLIC ggg)
    set_target_properties(${exe_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    install(TARGETS ${lib_name}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT libs)
    install(TARGETS ${exe_name}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT bin)
endfunction()

# Solver CLIs
ggg_add_buechi_solver_cli(attractor solvers/attractor.cpp ${CMAKE_SOURCE_DIR}/src/libggg/buechi/solvers/attractor.cpp)

# (No generator for buchi currently). If added later, install it as COMPONENT bin
