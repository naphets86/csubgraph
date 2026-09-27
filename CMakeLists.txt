cmake_minimum_required(VERSION 3.10)
project(SubgraphAlgorithm)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Flags für Windows/MSVC
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Hauptbibliothek
add_library(subgraphlib
    SubgraphAlgorithm.cpp
    SubgraphAlgorithm.h
)

target_include_directories(subgraphlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# FetchContent für externe Dependencies
include(FetchContent)

# nlohmann/json einbinden (für CLI JSON I/O)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

# Google Test einbinden
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# CLI-Executable für JSON-basierte Subgraph-Vergleiche
add_executable(subgraph-cli Cli.cpp)
target_link_libraries(subgraph-cli PRIVATE subgraphlib nlohmann_json::nlohmann_json)

# Test-Executable
add_executable(subgraph-tests tests/TestSubgraphAlgorithm.cpp)
target_link_libraries(subgraph-tests PRIVATE subgraphlib gtest gtest_main)

# Code Coverage aktivieren
if(MSVC)
    # Visual Studio / OpenCppCoverage
    target_compile_options(subgraph-tests PRIVATE /Zi)
    target_link_options(subgraph-tests PRIVATE /DEBUG)
else()
    # GCC / Clang mit gcov
    target_compile_options(subgraphlib PRIVATE --coverage)
    target_link_libraries(subgraphlib PRIVATE --coverage)
    target_compile_options(subgraph-tests PRIVATE --coverage)
    target_link_libraries(subgraph-tests PRIVATE --coverage)
endif()

# Test-Ziel
enable_testing()
add_test(NAME SubgraphTests COMMAND subgraph-tests)
