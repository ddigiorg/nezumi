// =============================================================================
// RenderCoreConfig.hpp
//
// Config of the shared render services (RenderCore).
// =============================================================================

#pragma once
#include "Nezumi/RHI/Common/Color.hpp"
#include <cstddef> // size_t

namespace Nezumi::Render {

struct RenderCoreConfig {
    // Clear color of the default recipe's "main" pass (see Engine).
    RHI::Color  clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Size of each GPU instance chunk in the shared StreamBuffer. More chunks
    // are added as a frame needs them and they are never resized.
    std::size_t stream_chunk_size = 1 << 20; // 1 MB
};

} // namespace Nezumi::Render
