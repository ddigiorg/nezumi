# ==============================================================================
# CMakeLists.txt
#
# Nezumi can be built on its own (examples and tests included) or consumed by a
# game through add_subdirectory / FetchContent, in which case only the engine
# libraries are built.
# ==============================================================================

cmake_minimum_required(VERSION 3.25)
project(Nezumi VERSION 0.1.0 LANGUAGES C CXX)

option(NEZUMI_BUILD_EXAMPLES "Build the Nezumi examples" ${PROJECT_IS_TOP_LEVEL})
option(NEZUMI_BUILD_TESTS    "Build the Nezumi tests"    ${PROJECT_IS_TOP_LEVEL})

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

include(cmake/NezumiDependencies.cmake)
include(cmake/NezumiHelpers.cmake)

add_subdirectory(src)

if(NEZUMI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(NEZUMI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
