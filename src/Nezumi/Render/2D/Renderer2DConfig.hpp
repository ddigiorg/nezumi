// =============================================================================
// Renderer2DConfig.hpp
//
// Config of the 2D renderer. Shared settings (clear color, GPU stream chunk
// size) live in RenderCoreConfig.
// =============================================================================

#pragma once
#include <cstddef> // size_t

namespace Nezumi::Render {

struct Renderer2DConfig {
    // Screen pixels per world unit at zoom 1.0 for the default View::World
    // camera (see CameraConfig::pixels_per_unit). A tile-space game sets its
    // on-screen tile size here, e.g. 32.0f. View::Screen always uses 1.0.
    float       world_pixels_per_unit = 1.0f;

    // Initial CPU reservations for submitted draws. Both grow if needed and
    // keep their capacity across frames.
    std::size_t reserve_arena_bytes = 1 << 20; //  1 MB
    std::size_t reserve_commands    = 1 << 14; // 16 KB
};

} // namespace Nezumi::Render
