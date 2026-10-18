// =============================================================================
// GLBindGroupEntry.hpp
//
// Overrides frontend BindGroupEntry for OpenGL-specifics.
// =============================================================================

#pragma once
#include "Nezumi/RHI/BindGroup/BindType.hpp"
#include "Nezumi/RHI/Buffer/BufferHandle.hpp"
#include "Nezumi/RHI/Texture/TextureHandle.hpp"
#include <cstdint> // uint32_t, uint_64_t

namespace Nezumi::RHI::GL {

struct GLBindGroupEntry {
    std::uint32_t binding            = 0;
    BindType      type               = BindType::UniformBuffer;
    bool          has_dynamic_offset = false;

    BufferHandle  buffer  = {};     // buffer bindings
    std::uint64_t offset  = 0;
    std::uint64_t size    = 0;      // resolved: never 0

    TextureHandle texture = {};     // texture bindings
};

} // namespace Nezumi::RHI::GL
