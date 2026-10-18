// =============================================================================
// BindGroupLayoutEntry.hpp
//
// WebGPU: GPUBindGroupLayoutEntry
// =============================================================================

#pragma once
#include "Nezumi/RHI/BindGroup/BindType.hpp"
#include "Nezumi/RHI/Shader/ShaderStageFlags.hpp"
#include <cstdint> // uint32_t

namespace Nezumi::RHI {

struct BindGroupLayoutEntry {
    std::uint32_t    binding = 0;
    BindType         type    = BindType::UniformBuffer;
    ShaderStageFlags stage   = ShaderStageFlags::None;

    // Buffers only. Lets set_bind_group supply an offset at bind time, so one
    // bind group can point at a different slice of a ring buffer each draw.
    bool has_dynamic_offset = false;
};

} // namespace Nezumi::RHI
