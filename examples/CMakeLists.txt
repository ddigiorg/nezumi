# ==============================================================================
# examples/CMakeLists.txt
# ==============================================================================

# Setup output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/bin)

# ------------------------------------------------------------------------------
# add_example(EXAMPLE_NAME)
#
# Registers an example executable.
#
# Expects: examples/<NAME>/main.cpp
# Output:  <build>/examples/bin/<NAME>
#
# Each example:
#   - Links against the engine (Nezumi::Engine)
#   - Gets assets/ copied next to its executable
#
# No DLLs need copying: SDL3 and FreeType are linked statically.
# ------------------------------------------------------------------------------

function(add_example EXAMPLE_NAME)
    set(EXAMPLE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME}/main.cpp)

    if(NOT EXISTS ${EXAMPLE_SOURCE})
        message(WARNING "add_example: source not found: ${EXAMPLE_SOURCE}")
        return()
    endif()

    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE Nezumi::Engine)

    nezumi_copy_assets(${EXAMPLE_NAME})
endfunction()

# ------------------------------------------------------------------------------
# Examples
# ------------------------------------------------------------------------------

add_example(00-hello-world)
add_example(01-renderer)
add_example(02-world)
add_example(03-unit-combat)
add_example(04-unit-group-combat)
