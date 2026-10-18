// =============================================================================
// SurfaceConfig.hpp
//
// Config of a surface.
// =============================================================================

#pragma once
#include "Nezumi/World/Chunk/ChunkSpan.hpp"
#include <string> // string

namespace Nezumi {

struct SurfaceConfig {
    std::string name           = {};
    ChunkSpan   initial_chunks = {}; // optional: chunks created up front
};

} // namespace Nezumi
