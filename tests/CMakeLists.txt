# ==============================================================================
# tests/CMakeLists.txt
#
# Only added when Nezumi is the top-level project (NEZUMI_BUILD_TESTS).
# ==============================================================================

# Setup output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/bin)

# ------------------------------------------------------------------------------
# Test support library
#
# Header-only library providing:
#   - Fixture: partial context builder for subsystem tests
#
# All test targets link against this automatically via add_test_exe().
# ------------------------------------------------------------------------------

add_library(nezumi_test_support INTERFACE)

target_include_directories(nezumi_test_support INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_link_libraries(nezumi_test_support INTERFACE Nezumi::Engine)

# ------------------------------------------------------------------------------
# add_test_exe(NAME [NEEDS_DISPLAY])
#
# Registers a test executable and hooks it into CTest.
# Expects: tests/<NAME>/main.cpp
# Output:  <build>/tests/bin/<NAME>
#
# Options:
#   NEEDS_DISPLAY  Label the test "display" so headless CI can skip it.
#                  Use for any test that opens a Window or Renderer.
#
# Usage:
#   add_test_exe(test_logger)
#   add_test_exe(test_window   NEEDS_DISPLAY)
#   add_test_exe(test_renderer NEEDS_DISPLAY)
# ------------------------------------------------------------------------------

function(add_test_exe TEST_NAME)
    cmake_parse_arguments(ARG "NEEDS_DISPLAY" "" "" ${ARGN})

    set(TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}/main.cpp)

    if(NOT EXISTS ${TEST_SOURCE})
        message(WARNING "add_test_exe: source not found: ${TEST_SOURCE}")
        return()
    endif()

    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE nezumi_test_support)

    # Register with CTest so `ctest --preset debug` runs it
    add_test(
        NAME    ${TEST_NAME}
        COMMAND ${TEST_NAME}
    )

    # Run headless-only tests with: ctest --preset debug -L headless
    # Run all tests with:           ctest --preset debug
    if(ARG_NEEDS_DISPLAY)
        set_tests_properties(${TEST_NAME} PROPERTIES
            LABELS      "display"
            ENVIRONMENT "SDL_VIDEODRIVER=dummy"   # fallback: SDL dummy driver
        )
    else()
        set_tests_properties(${TEST_NAME} PROPERTIES LABELS "headless")
    endif()
endfunction()

# ------------------------------------------------------------------------------
# Tests
#
# Naming convention:  test_<subsystem>
# Add NEEDS_DISPLAY if the test opens a window.
# ------------------------------------------------------------------------------

# add_test_exe(test_entity_list)
# add_test_exe(test_entity_pool)
# add_test_exe(test_event_bus)
# add_test_exe(test_logger)
# add_test_exe(test_subsystems)
add_test_exe(test_unit_group)

# add_test_exe(test_window   NEEDS_DISPLAY)
# add_test_exe(test_renderer NEEDS_DISPLAY)
