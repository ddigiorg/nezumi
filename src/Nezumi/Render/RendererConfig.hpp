// =============================================================================
// RendererConfig.hpp
//
// Config of the renderer.
// =============================================================================

#pragma once
#include "Nezumi/RHI/Common/Color.hpp"
#include <cstddef> // size_t

namespace Nezumi::Render {

struct RendererConfig {
    RHI::Color  clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Size of each GPU instance chunk. More chunks are added as a frame needs
    // them and they are never resized.
    std::size_t stream_chunk_size = 1 << 20; // 1 MB

    // Initial CPU reservations for submitted draws. Both grow if needed and
    // keep their capacity across frames.
    std::size_t reserve_arena_bytes = 1 << 20; //  1 MB
    std::size_t reserve_commands    = 1 << 14; // 16 KB
};

} // namespace Nezumi::Render
