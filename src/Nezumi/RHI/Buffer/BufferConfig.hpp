// =============================================================================
// BufferConfig.hpp
//
// Config of a Buffer.
//
// WebGPU: GPUBufferDescriptor
// Vulkan: VkBufferCreateInfo
// =============================================================================

#pragma once
#include "Nezumi/RHI/Buffer/BufferUsageFlags.hpp"
#include "Nezumi/RHI/Buffer/MemoryUsageFlags.hpp"
#include <cstddef>     // byte
#include <cstdint>     // uint64_t
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::RHI {

struct BufferConfig {
    std::string_view name   = "";
    std::uint64_t    size   = 0;
    BufferUsageFlags usage  = BufferUsageFlags::None;
    MemoryUsageFlags memory = MemoryUsageFlags::GpuOnly;

    // Optional. If provided, must be <= size. Uploaded at creation, so a
    // GpuOnly buffer with initial data doesn't need BufferUsage::CopyDst
    // (the backend adds whatever internal flags it needs for the upload).
    std::span<const std::byte> initial_data = {};
};

} // namespace Nezumi::RHI
