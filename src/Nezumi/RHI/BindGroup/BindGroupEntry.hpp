// =============================================================================
// BindGroupEntry.hpp
//
// A single slot that points to a GPU resource (e.g. buffer or textures).
//
// WebGPU: GPUBindGroupEntry
// =============================================================================

#pragma once
#include "Nezumi/RHI/Buffer/BufferHandle.hpp"
#include "Nezumi/RHI/Sampler/SamplerHandle.hpp"
#include "Nezumi/RHI/Texture/TextureHandle.hpp"
#include <cstdint> // uint32_t, uint64_t

namespace Nezumi::RHI {

struct BindGroupEntry {
    std::uint32_t binding = 0;

    BufferHandle  buffer  = {};
    std::uint64_t offset  = 0;
    std::uint64_t size    = 0; // 0 = from offset to end of buffer

    TextureHandle texture = {};
    SamplerHandle sampler = {};
};

} // namespace Nezumi::RHI
