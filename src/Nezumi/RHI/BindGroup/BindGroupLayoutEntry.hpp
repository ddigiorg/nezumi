// =============================================================================
// BindGroupLayoutEntry.hpp
//
// WebGPU: GPUBindGroupLayoutEntry
// =============================================================================

#pragma once
#include "Nezumi/RHI/BindGroup/BindType.hpp"
#include "Nezumi/RHI/Shader/ShaderStageFlags.hpp"
#include <cstdint>     // uint32_t
#include <string_view> // string_view

namespace Nezumi::RHI {

struct BindGroupLayoutEntry {
    std::uint32_t    binding = 0;
    BindType         type    = BindType::UniformBuffer;
    ShaderStageFlags stage   = ShaderStageFlags::None;

    // Buffers only. Lets set_bind_group supply an offset at bind time, so one
    // bind group can point at a different slice of a ring buffer each draw.
    bool has_dynamic_offset = false;

    // The GLSL name this binding goes by: the uniform block's name for a
    // UniformBuffer, the sampler uniform's name for a SampledTexture. OpenGL
    // 3.3 can't declare bindings in GLSL (layout(binding = N) is 4.20), so the
    // GL backend looks the name up after linking and assigns the binding
    // itself. Required there for those two types; ignored by other backends,
    // which read bindings from the shader.
    std::string_view name = "";
};

} // namespace Nezumi::RHI
