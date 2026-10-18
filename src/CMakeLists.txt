# ==============================================================================
# src/CMakeLists.txt
#
# NOTE: Third-party targets come from cmake/NezumiDependencies.cmake.
# ==============================================================================

# ------------------------------------------------------------------------------
# Version header
#
# Generated into the build tree so the source tree stays clean.
# Included as <Nezumi/Version.hpp>.
# ------------------------------------------------------------------------------

set(NEZUMI_GENERATED_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/include)

configure_file(
    Version.hpp.in
    ${NEZUMI_GENERATED_INCLUDE_DIR}/Nezumi/Version.hpp
    @ONLY
)

# ------------------------------------------------------------------------------
# Warnings (applied PRIVATE to each of our libraries)
#
# -Werror=switch: a switch over an enum that misses a case is an error.
# ------------------------------------------------------------------------------

set(NEZUMI_WARNINGS
    "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Werror=switch>"
    "$<$<CXX_COMPILER_ID:MSVC>:/W4;/we4062>"
)

# ------------------------------------------------------------------------------
# Core
# ------------------------------------------------------------------------------

add_library(nezumi_core STATIC
    Nezumi/Core/Clock.cpp
    Nezumi/Core/EventPump.cpp
    Nezumi/Core/Window.cpp
)

target_compile_features(nezumi_core PUBLIC cxx_std_20)

target_include_directories(nezumi_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NEZUMI_GENERATED_INCLUDE_DIR}
)

target_compile_options(nezumi_core PRIVATE ${NEZUMI_WARNINGS})

target_link_libraries(nezumi_core
    PRIVATE
        SDL3::SDL3
)

# ------------------------------------------------------------------------------
# Logger
# ------------------------------------------------------------------------------

add_library(nezumi_logger STATIC
    Nezumi/Logger/Logger.cpp
)

target_compile_features(nezumi_logger PUBLIC cxx_std_20)
target_include_directories(nezumi_logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(nezumi_logger PRIVATE ${NEZUMI_WARNINGS})

# ------------------------------------------------------------------------------
# RHI
#
# nezumi_glad is linked PRIVATE by nezumi_rhi, so nothing that links the rhi
# (or the engine) can include glad/gl.h.
# ------------------------------------------------------------------------------

add_library(nezumi_rhi STATIC
    Nezumi/RHI/Backends/OpenGL/GLCommandEncoder.cpp
    Nezumi/RHI/Backends/OpenGL/GLContext.cpp
    Nezumi/RHI/Backends/OpenGL/GLDebug.cpp
    Nezumi/RHI/Backends/OpenGL/GLDevice.cpp
    Nezumi/RHI/Backends/OpenGL/GLEnumConvert.cpp
    Nezumi/RHI/Backends/OpenGL/GLFramebufferCache.cpp
    Nezumi/RHI/Backends/OpenGL/GLInfo.cpp
    Nezumi/RHI/Backends/OpenGL/GLStateCache.cpp
    Nezumi/RHI/Backends/OpenGL/GLWindow.cpp
    Nezumi/RHI/Device.cpp
    Nezumi/RHI/Window.cpp
)

target_compile_features(nezumi_rhi PUBLIC cxx_std_20)
target_include_directories(nezumi_rhi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(nezumi_rhi PRIVATE ${NEZUMI_WARNINGS})

target_link_libraries(nezumi_rhi
    PUBLIC
        nezumi_core
        nezumi_logger
    PRIVATE
        nezumi_glad
        SDL3::SDL3
)

# ------------------------------------------------------------------------------
# Render
# ------------------------------------------------------------------------------

add_library(nezumi_render STATIC
    Nezumi/Render/Atlas/Atlas.cpp
    Nezumi/Render/Atlas/Image.cpp
    Nezumi/Render/Frame/BatchBuilder.cpp
    Nezumi/Render/Frame/FrameBindings.cpp
    Nezumi/Render/Frame/StreamBuffer.cpp
    Nezumi/Render/Kinds/ArrowKind.cpp
    Nezumi/Render/Kinds/CircleKind.cpp
    Nezumi/Render/Kinds/LineKind.cpp
    Nezumi/Render/Kinds/RectangleFilledKind.cpp
    Nezumi/Render/Kinds/RectangleUnfilledKind.cpp
    Nezumi/Render/Kinds/RectangleSDFKind.cpp
    Nezumi/Render/Kinds/SpriteKind.cpp
    Nezumi/Render/Kinds/TextGlyphSDFKind.cpp
    Nezumi/Render/Material/MaterialTable.cpp
    Nezumi/Render/Shader/ShaderLibrary.cpp
    Nezumi/Render/Text/Font.cpp
    Nezumi/Render/View/Camera.cpp
    Nezumi/Render/Renderer.cpp
)

target_compile_features(nezumi_render PUBLIC cxx_std_20)
target_include_directories(nezumi_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(nezumi_render PRIVATE ${NEZUMI_WARNINGS})

target_link_libraries(nezumi_render
    PUBLIC
        nezumi_core
        nezumi_logger
        nezumi_rhi
        glm::glm
    PRIVATE
        Freetype::Freetype
        nezumi_stb
)

# ------------------------------------------------------------------------------
# World
# ------------------------------------------------------------------------------

add_library(nezumi_world STATIC
    Nezumi/World/Chunk/Chunk.cpp
    Nezumi/World/Physics/Sector.cpp
    Nezumi/World/Surface/Surface.cpp
    Nezumi/World/World.cpp
)

target_compile_features(nezumi_world PUBLIC cxx_std_20)
target_include_directories(nezumi_world PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(nezumi_world PRIVATE ${NEZUMI_WARNINGS})

target_link_libraries(nezumi_world
    PUBLIC
        nezumi_core
        nezumi_logger
        glm::glm
)

# ------------------------------------------------------------------------------
# Engine
# ------------------------------------------------------------------------------

add_library(nezumi_engine STATIC
    Nezumi/Engine.cpp
)

target_compile_features(nezumi_engine PUBLIC cxx_std_20)
target_include_directories(nezumi_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(nezumi_engine PRIVATE ${NEZUMI_WARNINGS})

target_link_libraries(nezumi_engine
    PUBLIC
        nezumi_core
        nezumi_logger
        nezumi_rhi
        nezumi_render
        nezumi_world
)

# The name games link against
add_library(Nezumi::Engine ALIAS nezumi_engine)
