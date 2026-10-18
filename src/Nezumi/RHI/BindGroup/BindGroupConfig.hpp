// =============================================================================
// BindGroupConfig.hpp
//
// Config of a BindGroup.
//
// WebGPU: GPUBindGroupDescriptor
// Vulkan: VkDescriptorSetAllocateInfo
// =============================================================================

#pragma once
#include "Nezumi/RHI/BindGroup/BindGroupEntry.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupLayoutHandle.hpp"
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::RHI {

struct BindGroupConfig {
    std::string_view                name    = "";
    BindGroupLayoutHandle           layout  = {};
    std::span<const BindGroupEntry> entries = {};
};

} // namespace Nezumi::RHI
