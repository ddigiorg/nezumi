// =============================================================================
// ShaderConfig.hpp
//
// Config of a Shader.
//
// WebGPU: GPUShaderModuleDescriptor
// Vulkan: VkShaderModuleCreateInfo
// =============================================================================

#pragma once
#include "Nezumi/RHI/Shader/ShaderFormat.hpp"
#include "Nezumi/RHI/Shader/ShaderStageFlags.hpp"
#include <cstddef>     // byte
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::RHI {

struct ShaderConfig {
    std::string_view           name        = "";
    ShaderStageFlags           stage       = ShaderStageFlags::None;
    ShaderFormat               format      = ShaderFormat::Spirv;
    std::span<const std::byte> code        = {}; // SPIR-V words or GLSL source text
    std::string_view           entry_point = "main";
};

} // namespace Nezumi::RHI
